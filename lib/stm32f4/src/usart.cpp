#include "gpio.h"
#include "usart.h"

struct UsartPin {
  GpioPortNum   port;
  uint8_t       no;
  uint8_t       alt;
};
struct UsartConstParam {
  USART_TypeDef* usart;
  uint32_t       clock;
  IRQn_Type      irq;
  UsartPin       txp,
                 rxp;
};

static const UsartConstParam UsartConstMap[MAXUSARTS] = {
  { .usart = USART1, .clock = RCC_APB2ENR_USART1EN, .irq = USART1_IRQn,
    /*
    .txp = { GpioPortA, 9,  7 },
    .rxp = { GpioPortA, 10, 7 }
    */
    /* Alternate pins */
    .txp = { GpioPortB, 6,  7 },
    .rxp = { GpioPortB, 7,  7 }
    /**/
  },
  { .usart = USART2, .clock = RCC_APB1ENR_USART2EN, .irq = USART2_IRQn,
    .txp = { GpioPortA, 2, 7 },
    .rxp = { GpioPortA, 3, 7 }
    /* Alternate pins
    .txp = { GpioPortD, 5, 7 },
    .rxp = { GpioPortD, 6, 7 }
    */
  },
  { .usart = USART3, .clock = RCC_APB1ENR_USART3EN, .irq = USART3_IRQn,
    .txp = { GpioPortB, 10, 7 },
    .rxp = { GpioPortB, 11, 7 }
    /* Alternate pins
    .txp = { GpioPortC, 10, 7 },
    .rxp = { GpioPortC, 11, 7 }
    
    .txp = { GpioPortD, 8, 7 },
    .rxp = { GpioPortD, 9, 7 }
    */
  },
  { .usart = UART4,  .clock = RCC_APB1ENR_UART4EN,  .irq = UART4_IRQn,
    .txp = { GpioPortA, 0, 8 },
    .rxp = { GpioPortA, 1, 8 }
    /* Alternate pins
    .txp = { GpioPortC, 10, 8 },
    .rxp = { GpioPortC, 11, 8 }
    */
  },
};
static UsartClass * UsartIrqTable [MAXUSARTS];

// Obal na obsluhu prerušení.
void UsartClass::irq (void) {
  volatile register uint32_t status;
  char rdata, tdata;
  
  status = port->SR;                         // načti status přerušení
  if (status & USART_SR_TXE) {                 // od vysílače
    if (tx.Read (tdata))                       // pokud máme data
      port->DR = (uint32_t) tdata & 0xFF;      // zapíšeme do výstupu
    else                                       // pokud ne
      port->CR1 &= ~USART_CR1_TXEIE;           // je nutné zakázat přerušení od vysílače
  }
  if (status & USART_SR_RXNE) {                // od přijímače
    rdata = (port->DR) & 0xFF;                 // načteme data
    Up (&rdata, 1);
  }
}

void UsartClass::USART1_IRQ (void) {
  UsartClass * inst = UsartIrqTable [UsartNo1];
  if (inst) inst->irq ();
}
void UsartClass::USART2_IRQ (void) {
  UsartClass * inst = UsartIrqTable [UsartNo2];
  if (inst) inst->irq ();
}
void UsartClass::USART3_IRQ (void) {
  UsartClass * inst = UsartIrqTable [UsartNo2];
  if (inst) inst->irq ();
}
void UsartClass::UART4_IRQ (void) {
  UsartClass * inst = UsartIrqTable [UsartNo2];
  if (inst) inst->irq ();
}


/// Voláno z čistého C - startup.c.
extern "C" void USART1_IRQHandler (void)
  __attribute__((alias("_ZN10UsartClass10USART1_IRQEv")));
extern "C" void USART2_IRQHandler (void)
  __attribute__((alias("_ZN10UsartClass10USART2_IRQEv")));
extern "C" void USART3_IRQHandler (void)
  __attribute__((alias("_ZN10UsartClass10USART3_IRQEv")));
extern "C" void UART4_IRQHandler  (void)
  __attribute__((alias("_ZN10UsartClass9UART4_IRQEv")));

const uint8_t APBAHBPrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};
extern "C"  uint32_t      SystemCoreClock;

// Pomocná funkce pro vastavení alternativní funkce pinu.
static inline void setAlt (const UsartPin & p) {
  GpioClass pin (p.port, p.no, GPIO_Mode_AF);
  pin.setAF     (p.alt);
}
// Class is Bottom
UsartClass::UsartClass (UsartEnum no, uint32_t baud) : BaseLayer(),
  port (UsartConstMap[no].usart), uno(no), tx() {
  if (UsartIrqTable[no]) return;     // Chyba - jedina instance pro jeden USART
  UsartIrqTable[no] = this;
  Init (baud);
}
void UsartClass::Init (uint32_t baud) {
  const UsartConstParam * prm  = UsartConstMap + uno;
  // 1. Clock Enable
  if (uno) RCC->APB1ENR  |=   prm->clock;
  else     RCC->APB2ENR  |=   prm->clock;
  
  // 1a. Peripheral reset
  if (uno) RCC->APB1RSTR |=   prm->clock;
  else     RCC->APB2RSTR |=   prm->clock;
  if (uno) RCC->APB1RSTR &= ~ prm->clock;
  else     RCC->APB2RSTR &= ~ prm->clock;
  
  // 2. GPIO Alternate Config
  setAlt (prm->txp);
  setAlt (prm->rxp);
  // 4. NVIC
  NVIC_EnableIRQ (prm->irq);
  // 5. USART registry 8.bit bez parity
  port->CR1 &= ~USART_CR1_UE;       // pro jistotu
  port->CR1  =  USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE;
  port->CR2  =  0;
  
  SetBaud (baud);
}
void UsartClass::Fini (void) {
  const UsartConstParam * prm  = UsartConstMap + uno;
  // 1. NVIC
  NVIC_DisableIRQ (prm->irq);
  UsartIrqTable [uno] = 0;
  // 2. Clock Disable
  if (uno) RCC->APB1ENR &= ~prm->clock;
  else     RCC->APB2ENR &= ~prm->clock;
}

void UsartClass::SetBaud ( uint32_t baud ) const {
  uint32_t tmp = 0;

  port->CR1 &= ~USART_CR1_UE;       // pro jistotu
  // Determine Baudrate
  uint32_t i;
  if (uno) i = (RCC->CFGR & RCC_CFGR_PPRE1) >> 10;
  else     i = (RCC->CFGR & RCC_CFGR_PPRE2) >> 13;
  i = APBAHBPrescTable[i];
  uint32_t apbclock = SystemCoreClock >> i;
  uint32_t integerdivider, fractionaldivider;
  /* Determine the integer part */
  if ((port->CR1 & USART_CR1_OVER8) != 0) {
    /* Integer part computing in case Oversampling mode is 8 Samples */
    integerdivider = ((25 * apbclock) / (2 * (baud)));    
  } else /* if ((USARTx->CR1 & CR1_OVER8) == 0) */ {
    /* Integer part computing in case Oversampling mode is 16 Samples */
    integerdivider = ((25 * apbclock) / (4 * (baud)));    
  }
  tmp = (integerdivider / 100) << 4;
  /* Determine the fractional part */
  fractionaldivider = integerdivider - (100 * (tmp >> 4));
  /* Implement the fractional part in the register */
  if ((port->CR1 & USART_CR1_OVER8) != 0) {
    tmp |= ((((fractionaldivider * 8) +  50) / 100)) & ((uint8_t)0x07);
  } else /* if ((USARTx->CR1 & CR1_OVER8) == 0) */ {
    tmp |= ((((fractionaldivider * 16) + 50) / 100)) & ((uint8_t)0x0F);
  }
  /* Write to USART BRR */
  port->BRR = (uint16_t)tmp;  
  port->CR1 |= USART_CR1_UE;        // nakonec povolit globálně
}

// Nahustí data do fifo, na nic nečeká
uint32_t UsartClass::Down (const char* data, uint32_t len) {
  uint32_t res;
  for (res=0; res<len; res++) if (!tx.Write(data[res])) break;
  if (!res) return res;		  // nevešlo se nic, nevadí, skonči
  port->CR1 |= USART_CR1_TXEIE;   // po povolení přerušení okamžitě přeruší
  // while (port->CR1 & USART_CR1_TXEIE); // blokovani do konce vysilani na linku, nepoužívat !!!
  return res; // Je naspodu - Bottom, takže dál už není třeba nikam předávat
}
// Podobných funkcí pro nastavení je možné přidat více. Garbage collector je případně odstraní.
void UsartClass::SetHalfDuplex (bool on) const {
  port->CR1   &= ~USART_CR1_UE;     // zakázat, jinak nelze nastavovat
  if (on)
    port->CR3 |=  USART_CR3_HDSEL;  // poloduplex on
  else
    port->CR3 &= ~USART_CR3_HDSEL;  // poloduplex off
  port->CR1   |=  USART_CR1_UE;     // nakonec povolit globálně
}

