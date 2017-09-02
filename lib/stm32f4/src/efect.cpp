#include "stm32f4xx.h"
#include "gpio.h"
#include "efect.h"

static Efect * pEfectInstance = 0;
/*
static void setAFpin (unsigned pos) {
  GpioClass pin (GpioPortD, pos, GPIO_Mode_AF);
  pin.setAF (2);
}
*/
Efect::Efect() {
  cyc = 0; sta = 0;
  pEfectInstance = this;
  RCC->APB1ENR  |= RCC_APB1ENR_TIM4EN;
  const uint8_t pins [4] = {12,13,14,15};
  GpioGroup     grpi (GpioPortD, pins, 4, GPIO_Mode_AF);
  grpi.setAF (2);
  /*
  setAFpin (12);
  setAFpin (13);
  setAFpin (14);
  setAFpin (15);
  */
  TIM4->PSC    = 512;
  TIM4->ARR    = 256;
  TIM4->RCR    = 0;
  TIM4->EGR   |= TIM_EGR_UG;
  TIM4->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 |
                 TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
  TIM4->CCMR2 |= TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 |
                 TIM_CCMR2_OC4PE | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4M_2;
  // povol piny
  TIM4->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;
  // pocatecni pwm
  for (unsigned i=0; i<NoOfChannels; i++)
    set (i, 0x7F);
  // interrupt
  // TIM4->DIER |= TIM_DIER_UIE;
  NVIC_EnableIRQ (TIM4_IRQn);
  // nakonec cely timer - autoreload + enable
  // TIM4->CR1  |= TIM_CR1_ARPE | TIM_CR1_CEN;
}
void Efect::set (uint8_t channel, uint8_t value) {
  channel &= 3;
  volatile uint32_t * const ccr = & (TIM4->CCR1);
  ccr [channel] = value;
}
void Efect::irq (void) {
  uint8_t dop = 0xFF - cyc;
  switch (sta) {
    /**/
    case 0: set (0,cyc); set (1,0);   set (2,0);   set (3,dop); break;
    case 1: set (0,dop); set (1,cyc); set (2,0);   set (3,0);   break;
    case 2: set (0,0);   set (1,dop); set (2,cyc); set (3,0);   break;
    case 3: set (0,0);   set (1,0);   set (2,dop); set (3,cyc); break;
    /*
    case 0: set (0,cyc); set (1,0);   set (2,dop); set (3,0xFF);break;
    case 1: set (0,0xFF);set (1,cyc); set (2,0);   set (3,dop); break;
    case 2: set (0,dop); set (1,0xFF);set (2,cyc); set (3,0);   break;
    case 3: set (0,0);   set (1,dop); set (2,0xFF);set (3,cyc); break;
    */
  }
  if (++cyc > 0xFF) {
    cyc  = 0;
    sta += 1;
    sta &= 3;
  }
}
void Efect::start (bool on) {
  if (on) {
    TIM4->DIER |= TIM_DIER_UIE;
    TIM4->CR1  |= TIM_CR1_ARPE | TIM_CR1_CEN;
  } else {
    TIM4->CNT   = 0;
    TIM4->DIER &= ~ TIM_DIER_UIE;
    set (0,0); set (1,0); set (2,0); set (3,0);
  }
}

extern "C" void TIM4_IRQHandler (void) {
  TIM4->SR &= ~ TIM_SR_UIF;
  if (pEfectInstance) pEfectInstance->irq();
}
