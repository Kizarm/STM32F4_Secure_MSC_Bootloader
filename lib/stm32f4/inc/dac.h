#ifndef DAC_H
#define DAC_H

#include "stm32f4xx.h"

class Dac {

  public:
    Dac () {
      RCC->APB1ENR |= RCC_APB1ENR_DACEN;
      RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
      GPIOA->MODER |= (3 << 8);                 // A4
      DAC->CR       = DAC_CR_EN1;
    };
    void out (unsigned int value) {
      const unsigned int max = 0x0FFF;
      if (value > max) value = max;
      DAC->DHR12R1  = value;
    };
};

#endif // DAC_H
