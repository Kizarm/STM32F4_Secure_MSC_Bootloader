#include "stm32f4xx.h"
#include "system_stm32f4xx.h"
#include "spisimple.h"

#include "storagemmc.h"
/// pro pouziti ve spolupraci k MMC kartou
extern StorageMmc * MmcInstance; 
extern "C" void SysTick_Handler (void);

void SysTick_Handler (void) {
  // asm volatile ("bkpt 0");
  if (MmcInstance)
    MmcInstance->disk_timerproc();
}
//!///////////////////////////////////////

struct SpiPin {
  GpioPortNum   port;
  uint8_t       no;
  uint8_t       alt;
};
struct SpiPins {
  SpiPin sck;
  SpiPin miso;
  SpiPin mosi;
};
static const SpiPins Pins = {
  .sck  = {GpioPortB, 13, 5},
  .miso = {GpioPortB, 14, 5},
  .mosi = {GpioPortB, 15, 5}
};
// Pomocná funkce pro vastavení alternativní funkce pinu.
static inline void setAlt (const SpiPin & p) {
  GpioClass pin (p.port, p.no, GPIO_Mode_AF);
  pin.setAF     (p.alt);
}
static inline void setOut (const SpiPin & p) {
  GpioClass pin (p.port, p.no, GPIO_Mode_AF);
  pin.setAF     (p.alt);
  pin.setPuPd   (GPIO_PuPd_UP);
}

SpiSimple::SpiSimple() : sel(GpioPortB, 12) {
  // Init(); // vola se externe
  +sel;
}
void SpiSimple::Init (void) {
  SysTick_Config (SystemCoreClock / 100);
  RCC->APB1ENR   |= RCC_APB1ENR_SPI2EN;
  RCC->AHB1LPENR |= RCC_AHB1LPENR_GPIOBLPEN;
  setAlt (Pins.sck );
  setAlt (Pins.mosi);
  setOut (Pins.miso); // ! problem - mozna lepe hardware rezistor !
  /*
  GpioClass umiso (GpioPortB, 14, GPIO_Mode_AF);
  umiso.setAF     (5);
  umiso.setPuPd   (GPIO_PuPd_UP);
  */
  SPI2->I2SCFGR &= ~ SPI_I2SCFGR_I2SMOD; {
    SPI_CR1_REG reg;
    reg.WB       = SPI2->CR1;
    reg.BR       = 7;	// baudrate
    reg.CPOL     = 0;	// polarita CK to 0 when idle
    reg.CPHA     = 0;	// faze mode 0
    reg.DFF      = 0;	// 8-bit data frame format is selected for transmission/reception
    reg.LSBFIRST = 0;	// 0 - MSB first, 1 LSB
    reg.SSM      = 1;	// software NSS management
    reg.SSI      = 1;
    reg.MSTR     = 1;	// master
    SPI2->CR1    = reg.WB;
    reg.SPE      = 1;	// enable
    SPI2->CR1    = reg.WB;
  }
  uint8_t dummy  = SPI2->DR; // clear RXNE and OVR flag
  while (SPI2->SR & SPI_SR_RXNE & SPI_SR_OVR) dummy = SPI2->DR;
  (void)  dummy;
}
uint8_t SpiSimple::Transaction (const uint8_t byte) {
  while (!(SPI2->SR & SPI_SR_TXE ));		// wait for TXE
  SPI2->DR = byte;
  while (!(SPI2->SR & SPI_SR_RXNE)) {		// wait for RXNE
    // if (!(SPI2->SR & SPI_SR_BSY)) break;
  }
  return SPI2->DR;
}

void SpiSimple::Clock (SpiClockValues clock) {
  SPI_CR1_REG reg;
  reg.WB    = SPI2->CR1;
  reg.SPE   = 0;
  SPI2->CR1 = reg.WB;
  reg.SPE   = 1;
  reg.BR    = clock & 7;
  SPI2->CR1 = reg.WB;
}
void SpiSimple::Select (bool on) {
  // true = log. 0
  if (on) -sel;
  else    +sel;
}

uint32_t SpiSimple::Recv (uint8_t * buf, const uint32_t len) {
  for (uint32_t i=0; i<len; i++) buf [i] = Read();
  return len;
}
uint32_t SpiSimple::Send (const uint8_t * buf, const uint32_t len) {
  for (uint32_t i=0; i<len; i++) Write (buf [i]);
  return len;
}
