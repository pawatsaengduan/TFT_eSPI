        ////////////////////////////////////////////////////
        // TFT_eSPI driver functions for ESP32 processors //
        ////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
// Global variables
////////////////////////////////////////////////////////////////////////////////////////

// Select the SPI port to use, ESP32 has 2 options
#if !defined (TFT_PARALLEL_8_BIT)
  #ifdef USE_HSPI_PORT
    SPIClass spi = SPIClass(HSPI);
  #else // use default VSPI port
    //SPIClass& spi = SPI;
    SPIClass spi = SPIClass(VSPI);
  #endif
#endif

#ifdef ESP32_DMA
  // DMA SPA handle
  spi_device_handle_t dmaHAL;
  #ifdef USE_HSPI_PORT
    spi_host_device_t spi_host = HSPI_HOST;
  #else
    spi_host_device_t spi_host = VSPI_HOST;
  #endif
#endif

////////////////////////////////////////////////////////////////////////////////////////
#if defined (TFT_SDA_READ) && !defined (TFT_PARALLEL_8_BIT)
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           beginSDA
** Description:             Detach SPI from pin to permit software SPI
***************************************************************************************/
void TFT_eSPI::begin_SDA_Read(void)
{
  pinMatrixOutDetach(TFT_MOSI, false, false);
  pinMode(TFT_MOSI, INPUT);
  pinMatrixInAttach(TFT_MOSI, VSPIQ_IN_IDX, false);
}

/***************************************************************************************
** Function name:           endSDA
** Description:             Attach SPI pins after software SPI
***************************************************************************************/
void TFT_eSPI::end_SDA_Read(void)
{
  pinMode(TFT_MOSI, OUTPUT);
  pinMatrixOutAttach(TFT_MOSI, VSPID_OUT_IDX, false, false);
  pinMode(TFT_MISO, INPUT);
  pinMatrixInAttach(TFT_MISO, VSPIQ_IN_IDX, false);
}
////////////////////////////////////////////////////////////////////////////////////////
#endif // #if defined (TFT_SDA_READ)
////////////////////////////////////////////////////////////////////////////////////////


/***************************************************************************************
** Function name:           read byte  - supports class functions
** Description:             Read a byte from ESP32 8 bit data port
***************************************************************************************/
// Parallel bus MUST be set to input before calling this function!
uint8_t TFT_eSPI::readByte(void)
{
  uint8_t b = 0xAA;

#if defined (TFT_PARALLEL_8_BIT)
  RD_L;
  uint32_t reg;           // Read all GPIO pins 0-31
  reg = gpio_input_get(); // Read three times to allow for bus access time
  reg = gpio_input_get();
  reg = gpio_input_get(); // Data should be stable now
  RD_H;

  // Check GPIO bits used and build value
  b  = (((reg>>TFT_D0)&1) << 0);
  b |= (((reg>>TFT_D1)&1) << 1);
  b |= (((reg>>TFT_D2)&1) << 2);
  b |= (((reg>>TFT_D3)&1) << 3);
  b |= (((reg>>TFT_D4)&1) << 4);
  b |= (((reg>>TFT_D5)&1) << 5);
  b |= (((reg>>TFT_D6)&1) << 6);
  b |= (((reg>>TFT_D7)&1) << 7);
#endif

  return b;
}

////////////////////////////////////////////////////////////////////////////////////////
#ifdef TFT_PARALLEL_8_BIT
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           GPIO direction control  - supports class functions
** Description:             Set parallel bus to INPUT or OUTPUT
***************************************************************************************/
void TFT_eSPI::busDir(uint32_t mask, uint8_t mode)
{
  gpioMode(TFT_D0, mode);
  gpioMode(TFT_D1, mode);
  gpioMode(TFT_D2, mode);
  gpioMode(TFT_D3, mode);
  gpioMode(TFT_D4, mode);
  gpioMode(TFT_D5, mode);
  gpioMode(TFT_D6, mode);
  gpioMode(TFT_D7, mode);
  return;
  /*
  // Arduino generic native function, but slower
  pinMode(TFT_D0, mode);
  pinMode(TFT_D1, mode);
  pinMode(TFT_D2, mode);
  pinMode(TFT_D3, mode);
  pinMode(TFT_D4, mode);
  pinMode(TFT_D5, mode);
  pinMode(TFT_D6, mode);
  pinMode(TFT_D7, mode);
  return; //*/
}

/***************************************************************************************
** Function name:           GPIO direction control  - supports class functions
** Description:             Set ESP32 GPIO pin to input or output (set high) ASAP
***************************************************************************************/
void TFT_eSPI::gpioMode(uint8_t gpio, uint8_t mode)
{
  if(mode == INPUT) GPIO.enable_w1tc = ((uint32_t)1 << gpio);
  else GPIO.enable_w1ts = ((uint32_t)1 << gpio);

  ESP_REG(DR_REG_IO_MUX_BASE + esp32_gpioMux[gpio].reg) // Register lookup
    = ((uint32_t)2 << FUN_DRV_S)                        // Set drive strength 2
    | (FUN_IE)                                          // Input enable
    | ((uint32_t)2 << MCU_SEL_S);                       // Function select 2
  GPIO.pin[gpio].val = 1;                               // Set pin HIGH
}
////////////////////////////////////////////////////////////////////////////////////////
#endif // #ifdef TFT_PARALLEL_8_BIT
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
#if defined (RPI_WRITE_STROBE) && !defined (TFT_PARALLEL_8_BIT) // Code for RPi TFT
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for ESP32 or ESP8266 RPi TFT
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
  uint8_t colorBin[] = { (uint8_t) (color >> 8), (uint8_t) color };
  if(len) spi.writePattern(&colorBin[0], 2, 1); len--;
  while(len--) {WR_L; WR_H;}
}

/***************************************************************************************
** Function name:           pushPixels - for ESP32 or ESP8266 RPi TFT
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len)
{
  uint8_t *data = (uint8_t*)data_in;

  if(_swapBytes) {
      while ( len-- ) {tft_Write_16(*data); data++;}
      return;
  }

  while ( len >=64 ) {spi.writePattern(data, 64, 1); data += 64; len -= 64; }
  if (len) spi.writePattern(data, len, 1);
}

////////////////////////////////////////////////////////////////////////////////////////
#elif !defined (ILI9488_DRIVER) && !defined (TFT_PARALLEL_8_BIT) // Most displays
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for ESP32
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){
  
  uint32_t color32 = (color<<8 | color >>8)<<16 | (color<<8 | color >>8);

  if (len > 31)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 511);
    while(len>31)
    {
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), color32);
      WRITE_PERI_REG(SPI_W15_REG(SPI_PORT), color32);
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 32;
    }
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
  }

  if (len)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), (len << 4) - 1);
    for (uint32_t i=0; i <= (len<<1); i+=4) WRITE_PERI_REG(SPI_W0_REG(SPI_PORT) + i, color32);
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
  }
}

/***************************************************************************************
** Function name:           pushSwapBytePixels - for ESP32
** Description:             Write a sequence of pixels with swapped bytes
***************************************************************************************/
void TFT_eSPI::pushSwapBytePixels(const void* data_in, uint32_t len){

  uint8_t* data = (uint8_t*)data_in;
  uint32_t color[16];

  if (len > 31)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 511);
    while(len>31)
    {
      uint32_t i = 0;
      while(i<16)
      {
        color[i++] = DAT8TO32(data);
        data+=4;
      }
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT),  color[0]); 
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT),  color[1]);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT),  color[2]);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT),  color[3]);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT),  color[4]);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT),  color[5]);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT),  color[6]);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT),  color[7]);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT),  color[8]);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT),  color[9]);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), color[10]);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), color[11]);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), color[12]);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), color[13]);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), color[14]);
      WRITE_PERI_REG(SPI_W15_REG(SPI_PORT), color[15]);
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 32;
    }
  }

  if (len > 15)
  {
    uint32_t i = 0;
    while(i<8)
    {
      color[i++] = DAT8TO32(data);
      data+=4;
    }
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 255);
    WRITE_PERI_REG(SPI_W0_REG(SPI_PORT),  color[0]); 
    WRITE_PERI_REG(SPI_W1_REG(SPI_PORT),  color[1]);
    WRITE_PERI_REG(SPI_W2_REG(SPI_PORT),  color[2]);
    WRITE_PERI_REG(SPI_W3_REG(SPI_PORT),  color[3]);
    WRITE_PERI_REG(SPI_W4_REG(SPI_PORT),  color[4]);
    WRITE_PERI_REG(SPI_W5_REG(SPI_PORT),  color[5]);
    WRITE_PERI_REG(SPI_W6_REG(SPI_PORT),  color[6]);
    WRITE_PERI_REG(SPI_W7_REG(SPI_PORT),  color[7]);
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
    len -= 16;
  }

  if (len)
  {
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), (len << 4) - 1);
    for (uint32_t i=0; i <= (len<<1); i+=4) {
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT)+i, DAT8TO32(data)); data+=4;
    }
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
  }
  while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);

}

/***************************************************************************************
** Function name:           pushPixels - for ESP32
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  if(_swapBytes) {
    pushSwapBytePixels(data_in, len);
    return;
  }

  uint32_t *data = (uint32_t*)data_in;

  if (len > 31)
  {
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), 511);
    while(len>31)
    {
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT),  *data++);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), *data++);
      WRITE_PERI_REG(SPI_W15_REG(SPI_PORT), *data++);
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 32;
    }
  }

  if (len)
  {
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
    WRITE_PERI_REG(SPI_MOSI_DLEN_REG(SPI_PORT), (len << 4) - 1);
    for (uint32_t i=0; i <= (len<<1); i+=4) WRITE_PERI_REG((SPI_W0_REG(SPI_PORT) + i), *data++);
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
  }
  while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
}

////////////////////////////////////////////////////////////////////////////////////////
#elif defined (ILI9488_DRIVER) && !defined (TFT_PARALLEL_8_BIT)// Now code for ILI9488
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for ESP32 and 3 byte RGB display
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len)
{
  // Split out the colours
  uint32_t r = (color & 0xF800)>>8;
  uint32_t g = (color & 0x07E0)<<5;
  uint32_t b = (color & 0x001F)<<19;
  // Concatenate 4 pixels into three 32 bit blocks
  uint32_t r0 = r<<24 | b | g | r;
  uint32_t r1 = r0>>8 | g<<16;
  uint32_t r2 = r1>>8 | b<<8;

  if (len > 19)
  {
    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_PORT), SPI_USR_MOSI_DBITLEN, 479, SPI_USR_MOSI_DBITLEN_S);

    while(len>19)
    {
      while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
      WRITE_PERI_REG(SPI_W0_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W1_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W2_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W3_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W4_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W5_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), r2);
      SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
      len -= 20;
    }
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
  }

  if (len)
  {
    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_PORT), SPI_USR_MOSI_DBITLEN, (len * 24) - 1, SPI_USR_MOSI_DBITLEN_S);
    WRITE_PERI_REG(SPI_W0_REG(SPI_PORT), r0);
    WRITE_PERI_REG(SPI_W1_REG(SPI_PORT), r1);
    WRITE_PERI_REG(SPI_W2_REG(SPI_PORT), r2);
    WRITE_PERI_REG(SPI_W3_REG(SPI_PORT), r0);
    WRITE_PERI_REG(SPI_W4_REG(SPI_PORT), r1);
    WRITE_PERI_REG(SPI_W5_REG(SPI_PORT), r2);
    if (len > 8 )
    {
      WRITE_PERI_REG(SPI_W6_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W7_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W8_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W9_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W10_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W11_REG(SPI_PORT), r2);
      WRITE_PERI_REG(SPI_W12_REG(SPI_PORT), r0);
      WRITE_PERI_REG(SPI_W13_REG(SPI_PORT), r1);
      WRITE_PERI_REG(SPI_W14_REG(SPI_PORT), r2);
    }

    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_PORT), SPI_USR);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_PORT))&SPI_USR);
  }
}

/***************************************************************************************
** Function name:           pushPixels - for ESP32 and 3 byte RGB display
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  // ILI9488 write macro is not endianess dependant, hence !_swapBytes
  if(!_swapBytes) { while ( len-- ) {tft_Write_16S(*data); data++;} }
  else { while ( len-- ) {tft_Write_16(*data); data++;} }
}

/***************************************************************************************
** Function name:           pushSwapBytePixels - for ESP32 and 3 byte RGB display
** Description:             Write a sequence of pixels with swapped bytes
***************************************************************************************/
void TFT_eSPI::pushSwapBytePixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  // ILI9488 write macro is not endianess dependant, so swap byte macro not used here
  while ( len-- ) {tft_Write_16(*data); data++;}
}

////////////////////////////////////////////////////////////////////////////////////////
#elif defined (TFT_PARALLEL_8_BIT) // Now the code for ESP32 8 bit parallel
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           pushBlock - for ESP32 and parallel display
** Description:             Write a block of pixels of the same colour
***************************************************************************************/
void TFT_eSPI::pushBlock(uint16_t color, uint32_t len){
  if ( (color >> 8) == (color & 0x00FF) )
  { if (!len) return;
    tft_Write_16(color);
    while (--len) {WR_L; WR_H; WR_L; WR_H;}
  }
  else while (len--) {tft_Write_16(color);}
}

/***************************************************************************************
** Function name:           pushSwapBytePixels - for ESP32 and parallel display
** Description:             Write a sequence of pixels with swapped bytes
***************************************************************************************/
void TFT_eSPI::pushSwapBytePixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  while ( len-- ) {tft_Write_16(*data); data++;}
}

/***************************************************************************************
** Function name:           pushPixels - for ESP32 and parallel display
** Description:             Write a sequence of pixels
***************************************************************************************/
void TFT_eSPI::pushPixels(const void* data_in, uint32_t len){

  uint16_t *data = (uint16_t*)data_in;
  if(_swapBytes) { while ( len-- ) {tft_Write_16(*data); data++; } }
  else { while ( len-- ) {tft_Write_16S(*data); data++;} }
}

////////////////////////////////////////////////////////////////////////////////////////
#endif // End of display interface specific functions
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
#if defined ESP32_DMA && !defined (TFT_PARALLEL_8_BIT) //       DMA FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************
** Function name:           dmaBusy
** Description:             Check if DMA is busy (currently blocking!)
***************************************************************************************/
bool TFT_eSPI::dmaBusy(void)
{
  if (!DMA_Enabled || !spiBusyCheck) return false;
  //spi_transaction_t rtrans;
  //bool trans_result=spi_device_polling_transmit(dmaHAL, &rtrans);
  //return trans_result;
  // This works but blocks
  dmaWait();
  return false;
}


/***************************************************************************************
** Function name:           dmaWait
** Description:             Check if DMA is busy (blocking!)
***************************************************************************************/
void TFT_eSPI::dmaWait(void)
{
  if (!DMA_Enabled || !spiBusyCheck) return;
  spi_transaction_t *rtrans;
  esp_err_t ret;
  for (int i = 0; i < spiBusyCheck; ++i)
  {
    ret = spi_device_get_trans_result(dmaHAL, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
  }
  spiBusyCheck = 0;
}


/***************************************************************************************
** Function name:           pushImageDMA
** Description:             Push pixels to TFT (len must be less than 32767)
***************************************************************************************/
// This will byte swap the original image if setSwapBytes(true) was called by sketch.
void TFT_eSPI::pushPixelsDMA(uint16_t* image, uint32_t len)
{
  if ((len == 0) || (!DMA_Enabled)) return;
  dmaWait();
  esp_err_t ret;
  static spi_transaction_t trans;

  memset(&trans, 0, sizeof(spi_transaction_t));

  trans.user = (void *)1;
  trans.tx_buffer = image;  //finally send the line data
  trans.length = len * 16;        //Data length, in bits
  trans.flags = 0;                //SPI_TRANS_USE_TXDATA flag

  ret = spi_device_queue_trans(dmaHAL, &trans, portMAX_DELAY);
  assert(ret == ESP_OK);

  spiBusyCheck = 1;
}


/***************************************************************************************
** Function name:           pushImageDMA
** Description:             Push image to a window (w*h must be less than 65536)
***************************************************************************************/
// This will clip and also swap bytes if setSwapBytes(true) was called by sketch
void TFT_eSPI::pushImageDMA(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t* image, uint16_t* buffer)
{
  if ((x >= _width) || (y >= _height) || (!DMA_Enabled)) return;

  int32_t dx = 0;
  int32_t dy = 0;
  int32_t dw = w;
  int32_t dh = h;

  if (x < 0) { dw += x; dx = -x; x = 0; }
  if (y < 0) { dh += y; dy = -y; y = 0; }

  if ((x + dw) > _width ) dw = _width  - x;
  if ((y + dh) > _height) dh = _height - y;

  if (dw < 1 || dh < 1) return;

  if (buffer == nullptr) buffer = image;

  uint32_t len = dw*dh;

  dmaWait();
  
  // If image is clipped, copy pixels into a contiguous block
  if ( (dw != w) || (dh != h) ) {
    if(_swapBytes) {
      for (int32_t yb = 0; yb < dh; yb++) {
        for (int32_t xb = 0; xb < dw; xb++) {
          uint32_t src = xb + dx + w * (yb + dy);
          (buffer[xb + yb * dw] = image[src] << 8 | image[src] >> 8);
        }
      }
    }
    else {
      for (int32_t yb = 0; yb < dh; yb++) {
        memcpy((uint8_t*) (buffer + yb * dw), (uint8_t*) (image + dx + w * (yb + dy)), dw << 1);
      }
    }
  }
  // else, if a buffer pointer has been provided copy whole image to the buffer
  else if (buffer != image || _swapBytes) {
    if(_swapBytes) {
      for (uint32_t i = 0; i < len; i++) (buffer[i] = image[i] << 8 | image[i] >> 8);
    }
    else {
      memcpy(buffer, image, len*2);
    }
  }

  esp_err_t ret;
  static spi_transaction_t trans[6];
  for (int i = 0; i < 6; i++)
  {
    memset(&trans[i], 0, sizeof(spi_transaction_t));
    if ((i & 1) == 0)
    {
      trans[i].length = 8;
      trans[i].user = (void *)0;
    }
    else
    {
      trans[i].length = 8 * 4;
      trans[i].user = (void *)1;
    }
    trans[i].flags = SPI_TRANS_USE_TXDATA;
  }
  trans[0].tx_data[0] = 0x2A;                //Column Address Set
  trans[1].tx_data[0] = x >> 8;              //Start Col High
  trans[1].tx_data[1] = x & 0xFF;            //Start Col Low
  trans[1].tx_data[2] = (x + dw - 1) >> 8;   //End Col High
  trans[1].tx_data[3] = (x + dw - 1) & 0xFF; //End Col Low
  trans[2].tx_data[0] = 0x2B;                //Page address set
  trans[3].tx_data[0] = y >> 8;              //Start page high
  trans[3].tx_data[1] = y & 0xFF;            //start page low
  trans[3].tx_data[2] = (y + dh - 1) >> 8;   //end page high
  trans[3].tx_data[3] = (y + dh - 1) & 0xFF; //end page low
  trans[4].tx_data[0] = 0x2C;                //memory write
  trans[5].tx_buffer = buffer;               //finally send the line data
  trans[5].length = dw * 2 * 8 * dh;         //Data length, in bits
  trans[5].flags = 0;                        //undo SPI_TRANS_USE_TXDATA flag
  for (int i = 0; i < 6; i++)
  {
    ret = spi_device_queue_trans(dmaHAL, &trans[i], portMAX_DELAY);
    assert(ret == ESP_OK);
  }
  spiBusyCheck = 6;
}

////////////////////////////////////////////////////////////////////////////////////////
// Processor specific DMA initialisation
////////////////////////////////////////////////////////////////////////////////////////

// The DMA functions here work with SPI only (not parallel)
/***************************************************************************************
** Function name:           dc_callback
** Description:             Toggles DC line during transaction
***************************************************************************************/
extern "C" void dc_callback();

void IRAM_ATTR dc_callback(spi_transaction_t *spi_tx)
{
  if ((bool)spi_tx->user) DC_D;
  else DC_C;
}

/***************************************************************************************
** Function name:           initDMA
** Description:             Initialise the DMA engine - returns true if init OK
***************************************************************************************/
bool TFT_eSPI::initDMA(void)
{
  if (DMA_Enabled) return false;

  esp_err_t ret;
  spi_bus_config_t buscfg = {
    .mosi_io_num = TFT_MOSI,
    .miso_io_num = TFT_MISO,
    .sclk_io_num = TFT_SCLK,
    .quadwp_io_num = -1,
    .quadhd_io_num = -1,
    .max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * 2 + 8, // TFT screen size
    .flags = 0,
    .intr_flags = 0
  };
  spi_device_interface_config_t devcfg = {
    .command_bits = 0,
    .address_bits = 0,
    .dummy_bits = 0,
    .mode = TFT_SPI_MODE,
    .duty_cycle_pos = 0,
    .cs_ena_pretrans = 0,
    .cs_ena_posttrans = 0,
    .clock_speed_hz = SPI_FREQUENCY,
    .input_delay_ns = 0,
    .spics_io_num = TFT_CS,
    .flags = 0,
    .queue_size = 7,
    .pre_cb = dc_callback, //Callback to handle D/C line
    .post_cb = 0
  };
  ret = spi_bus_initialize(spi_host, &buscfg, 1);
  ESP_ERROR_CHECK(ret);
  ret = spi_bus_add_device(spi_host, &devcfg, &dmaHAL);
  ESP_ERROR_CHECK(ret);

  DMA_Enabled = true;
  spiBusyCheck = 0;
  return true;
}

/***************************************************************************************
** Function name:           deInitDMA
** Description:             Disconnect the DMA engine from SPI
***************************************************************************************/
void TFT_eSPI::deInitDMA(void)
{
  if (!DMA_Enabled) return;

  spi_bus_free(spi_host);
  DMA_Enabled = false;
}

////////////////////////////////////////////////////////////////////////////////////////
#endif // End of DMA FUNCTIONS    
////////////////////////////////////////////////////////////////////////////////////////
