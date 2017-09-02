#include "stm32f4xx.h"
#include "adcdma.h"
#include "gpio.h"

static const uint32_t AdcChanel = 1;
static const uint32_t AdcPin    = 1;

dataCallBack  AdcDma::proces;
uint16_t*     AdcDma::pbuf0;
uint16_t*     AdcDma::pbuf1;
uint16_t      AdcDma::bufer[2 * DmaBufLen];

static inline void EnableClock (void) {
  RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
  RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
}
static inline void Timer3Init (uint32_t us) {
  TIM3->PSC  = 83;
  TIM3->ARR  = us - 1;
  TIM3->RCR  = 0;
  // Preload, enable
  TIM3->CR1 |= TIM_CR1_ARPE;
  // TRGO update for ADC
  TIM3->CR2 |= TIM_CR2_MMS_1;
}
// ADC1 mapped DMA2 channel 0 stream 0 
static inline void Dma1Init (uint16_t* ptr) {  
  // Configure the peripheral data register address
  DMA2_Stream0->PAR  = (uint32_t) (& (ADC1->DR));
  // Configure the memory address
  DMA2_Stream0->M0AR = (uint32_t) (ptr);
  // Configure the number of DMA tranfer to be performs on DMA channel 1
  DMA2_Stream0->NDTR = 2 * DmaBufLen;
  // Configure increment, size, interrupts and circular mode
  DMA2_Stream0->CR  |= DMA_SxCR_MINC | DMA_SxCR_MSIZE_0 | DMA_SxCR_PSIZE_0
                    |  DMA_SxCR_HTIE |  DMA_SxCR_TCIE   | DMA_SxCR_CIRC | ((uint32_t)0x00000000);
  // Enable DMA Stream 5
  DMA2_Stream0->CR  |= DMA_SxCR_EN;

  // Enable DMA transfer on ADC and circular mode
  ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_DDS;
}
static inline void AdcInit (void) {
  // Common init
  ADC->CCR  |= 0;
  // Select TRG TIM3
  ADC1->CR2 |= ADC_CR2_EXTEN_0 | ADC_CR2_EXTSEL_3;
  // Select CHSELx, sampling mode
  ADC1->SQR3 |=        AdcChanel;
  ADC1->SMPR1 = (0<<(3*AdcChanel));
}
static inline void AdcStart (void) {
  TIM3->CR1 |= TIM_CR1_CEN;
  ADC1->CR2 |= ADC_CR2_ADON;
  ADC1->CR2 |= ADC_CR2_SWSTART;
}
AdcDma::AdcDma (uint32_t us) {
  pbuf0 = bufer;
  pbuf1 = bufer + DmaBufLen;
  proces = 0;
  EnableClock ();
  GpioClass in (GpioPortA, AdcPin, GPIO_Mode_AN);
  Timer3Init  (us);
  NVIC_EnableIRQ (DMA2_Stream0_IRQn);
  Dma1Init    (pbuf0);
  AdcInit     ();
  AdcStart    ();
}

void AdcDma::setCallback (dataCallBack cb) {
  proces = cb;
}

void AdcDma::irq (void) {
  uint16_t* cur = 0;
  uint32_t  status = DMA2->LISR;
  if (status & DMA_LISR_HTIF0) cur = pbuf0;
  if (status & DMA_LISR_TCIF0) cur = pbuf1;
  // znuluj příznaky
  DMA2->LIFCR = status;
  if (!cur)    return;
  if (!proces) return;
  // zpracuj data, pokud je potřeba
  proces (cur, DmaBufLen);
}
extern "C" void DMA2_Stream0_IRQHandler (void) __attribute__((alias("_ZN6AdcDma3irqEv")));
