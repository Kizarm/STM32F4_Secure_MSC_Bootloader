#include "audiodma.h"
#include "gpio.h"

#define RCC_APB1Periph_I2C1              ((uint32_t)0x00200000)
#define RCC_AHB1Periph_DMA1              ((uint32_t)0x00200000)
#define RCC_APB1Periph_SPI3              ((uint32_t)0x00008000)

void PlatformExit (void) {
}

static AudioDma * Instance = 0;

// SPI3 mapped DMA1 channel 0 stream 7 
static inline void Dma1Init (Sample * ptr) {  
  // configure increment, size, interrupts and circular mode
  DMA1_Stream7->CR  = DMA_SxCR_MINC | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0 | DMA_SxCR_CIRC
                    | DMA_SxCR_HTIE | DMA_SxCR_TCIE    | DMA_SxCR_PL_0    | DMA_SxCR_DIR_0
                  /*|  DMA_SxCR_CHSEL_0 | DMA_SxCR_CHSEL_1 | DMA_SxCR_CHSEL_2*/ ;
  // Configure the peripheral data register address
  DMA1_Stream7->PAR  = (uint32_t) (& (SPI3->DR));
  // Configure the memory address
  DMA1_Stream7->M0AR = (uint32_t) (ptr);
  // Configure the number of DMA tranfer to be performs on DMA channel 1
  DMA1_Stream7->NDTR = 4 * DmaBufLen;
//DMA1_Stream7->FCR  = DMA_SxFCR_DMDIS;
  // Enable DMA Stream 7
  DMA1_Stream7->CR  |= DMA_SxCR_EN;
}


AudioDma::AudioDma () {
  Instance = this;
  pbuf0 = buf;
  pbuf1 = buf + DmaBufLen;
  // Intitialize state.
  RCC->AHB1ENR |= RCC_AHB1Periph_DMA1;
  RCC->APB1ENR |= RCC_APB1Periph_I2C1;
  RCC->APB1ENR |= RCC_APB1Periph_SPI3;

  GpioClass resetPin (GpioPortD, 4);
  
  GpioClass sclPin (GpioPortB, 6, GPIO_Mode_AF);
  GpioClass sdaPin (GpioPortB, 9, GPIO_Mode_AF);
  sclPin.setOType(GPIO_OType_OD);
  sdaPin.setOType(GPIO_OType_OD);
  sclPin.setAF(4);
  sdaPin.setAF(4);
  
  GpioClass mckPin (GpioPortC, 7, GPIO_Mode_AF);
  GpioClass sckPin (GpioPortC,10, GPIO_Mode_AF);
  GpioClass sddPin (GpioPortC,12, GPIO_Mode_AF);
  mckPin.setAF (6);
  sckPin.setAF (6);
  sddPin.setAF (6);
    
  GpioClass iisPin (GpioPortA, 4, GPIO_Mode_AF);
  iisPin.setAF (6);
  
  // Reset the codec.
  -resetPin;
  volatile int i;
  for (i = 0; i < 0x4fff; i++) {
    __asm__ volatile ("nop");
  }
  +resetPin;

  // Reset I2C.
  RCC->APB1RSTR |=  RCC_APB1Periph_I2C1;
  RCC->APB1RSTR &= ~RCC_APB1Periph_I2C1;

  // Configure I2C.
  uint32_t pclk1 = 42000000;

  I2C1 ->CR2 = pclk1 / 1000000; // Configure frequency and disable interrupts and DMA.
  I2C1 ->OAR1 = I2C_OAR1_ADDMODE | 0x33;

  // Configure I2C speed in standard mode.
  const uint32_t i2c_speed = 100000;
  int ccrspeed = pclk1 / (i2c_speed * 2);
  if (ccrspeed < 4) {
    ccrspeed = 4;
  }
  I2C1 ->CCR = ccrspeed;
  I2C1 ->TRISE = pclk1 / 1000000 + 1;

  I2C1 ->CR1 = I2C_CR1_ACK | I2C_CR1_PE; // Enable and configure the I2C peripheral.

  // Configure codec.
  WriteRegister (0x02, 0x01); // Keep codec powered off.
  WriteRegister (0x04, 0xaf); // SPK always off and HP always on.

  WriteRegister (0x05, 0x81); // Clock configuration: Auto detection.
  WriteRegister (0x06, 0x04); // Set slave mode and Philips audio standard.

  SetVolume (206);      // -12 dB

  // Power on the codec.
  WriteRegister (0x02, 0x9e);

  // Configure codec for fast shutdown.
  WriteRegister (0x0a, 0x00); // Disable the analog soft ramp.
  WriteRegister (0x0e, 0x04); // Disable the digital soft ramp.

  WriteRegister (0x27, 0x00); // Disable the limiter attack level.
  WriteRegister (0x1f, 0x0f); // Adjust bass and treble levels.

  WriteRegister (0x1a, 0x0a); // Adjust PCM volume level.
  WriteRegister (0x1b, 0x0a);

  // Disable I2S.
  SPI3 ->I2SCFGR = 0;

  RCC ->CR &=~RCC_CR_PLLI2SON;  // zakaz PLL, jinak nezapise nove hodnoty
  int plln=271,pllr=2,i2sdiv=6, i2sodd=0;       // 44.1 kHz
//int plln=271,pllr=2,i2sdiv=12,i2sodd=0;       // 22.05kHz
  // I2S clock configuration
  RCC ->CFGR &= ~RCC_CFGR_I2SSRC; // PLLI2S clock used as I2S clock source.
  RCC ->PLLI2SCFGR = (pllr << 28) | (plln << 6);

  // Enable PLLI2S and wait until it is ready.
  RCC ->CR |= RCC_CR_PLLI2SON;
  while (! (RCC ->CR & RCC_CR_PLLI2SRDY));

  // Configure I2S.     // 6 , 0
  SPI3 ->I2SPR   = i2sdiv | (i2sodd << 8) | SPI_I2SPR_MCKOE;
  SPI3 ->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SCFG_1
                |  SPI_I2SCFGR_I2SE; // Master transmitter, Phillips mode, 16 bit values, clock polarity low, enable.

  NVIC_EnableIRQ (DMA1_Stream7_IRQn);
  Dma1Init       (buf);
  SPI3 ->CR2    |= SPI_CR2_TXDMAEN;

}

void AudioDma::WriteRegister (uint8_t address, uint8_t value) {

  while (I2C1 ->SR2 & I2C_SR2_BUSY);

  I2C1 ->CR1 |= I2C_CR1_START; // Start the transfer sequence.
  while (! (I2C1 ->SR1 & I2C_SR1_SB));   // Wait for start bit.

  I2C1 ->DR = 0x94;
  while (! (I2C1 ->SR1 & I2C_SR1_ADDR)); // Wait for master transmitter mode.
  I2C1 ->SR2;

  I2C1 ->DR = address; // Transmit the address to write to.
  while (! (I2C1 ->SR1 & I2C_SR1_TXE));  // Wait for byte to move to shift register.

  I2C1 ->DR = value; // Transmit the value.

  while (! (I2C1 ->SR1 & I2C_SR1_BTF));  // Wait for all bytes to finish.
  I2C1 ->CR1 |= I2C_CR1_STOP; // End the transfer sequence.
}

void AudioDma::SetVolume (int volume) {
  WriteRegister (0x20, (volume + 0x19) & 0xff);
  WriteRegister (0x21, (volume + 0x19) & 0xff);
}
void AudioDma::Turn (bool on) {
  if (on) {
    WriteRegister (0x02, 0x9e);
    SPI3 ->I2SCFGR = SPI_I2SCFGR_I2SMOD | SPI_I2SCFGR_I2SCFG_1
                    | SPI_I2SCFGR_I2SE; // Master transmitter, Phillips mode, 16 bit values, clock polarity low, enable.
  } else {
    WriteRegister (0x02, 0x01);
    SPI3 ->I2SCFGR = 0;
  }
}
void AudioDma::irq (void) {
  Sample * cur = 0;
  volatile uint32_t  status = DMA1->HISR;
  if (status & DMA_HISR_HTIF7) cur = pbuf0;
  if (status & DMA_HISR_TCIF7) cur = pbuf1;
  // znuluj příznaky
  // asm volatile ("bkpt 1");
  DMA1->HIFCR |= status;
  if (!cur)    return;
  // zpracuj data, pokud je potřeba
  Up (cur, DmaBufLen);
}
extern "C" void DMA1_Stream7_IRQHandler (void) {
  if (Instance) Instance->irq();
}

