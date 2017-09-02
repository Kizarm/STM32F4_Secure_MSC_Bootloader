#include "gpio.h"
// Tabulka 40 bytu ve flash, kód to zkrátí i zrychlí.
static const GpioAssocPort cPortTab[] = {
  {GPIOA, RCC_AHB1Periph_GPIOA},
  {GPIOB, RCC_AHB1Periph_GPIOB},
  {GPIOC, RCC_AHB1Periph_GPIOC},
  {GPIOD, RCC_AHB1Periph_GPIOD},
  {GPIOE, RCC_AHB1Periph_GPIOE},
  {GPIOF, RCC_AHB1Periph_GPIOF},
  {GPIOG, RCC_AHB1Periph_GPIOG},
  {GPIOH, RCC_AHB1Periph_GPIOH},
  {GPIOI, RCC_AHB1Periph_GPIOI},
};

GpioClass::GpioClass (GpioPortNum const port, const uint32_t no, const GPIOMode_TypeDef type) :
  io(cPortTab[port].portAdr), pos(1UL << no), num(no) {
  // Povol hodiny
  RCC->AHB1ENR |= cPortTab[port].clkMask;
  // A nastav pin (pořadí dle ST knihovny).
  setSpeed (GPIO_Speed_100MHz);
//setOType (GPIO_OType_PP);
  setMode  (type);
//setPuPd  (GPIO_PuPd_NOPULL);
}

GpioGroup::GpioGroup (const GpioPortNum port, const uint8_t * const list, const uint32_t num, const GPIOMode_TypeDef type) :
  io(cPortTab[port].portAdr), pins (list), size (num){
  // Povol hodiny
  RCC->AHB1ENR |= cPortTab[port].clkMask;
  // A nastav pin (pořadí dle ST knihovny).
  setSpeed (GPIO_Speed_100MHz);
//setOType (GPIO_OType_PP);
  setMode  (type);
//setPuPd  (GPIO_PuPd_NOPULL);
}

