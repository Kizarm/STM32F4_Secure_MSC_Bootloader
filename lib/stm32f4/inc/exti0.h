#ifndef EXTI0_H
#define EXTI0_H
#include "stm32f4xx.h"
/// Softwarové přerušení na nízké prioritě.
/// Není ekvivalentní volání podprogramu (funkce).
/// Pokud je to voláno z přerušení o vyšší prioritě, provede se toto celé a až pak následně
/// to softwarové (které může být i nadále přerušováno). Tedy odložená rutina, která probíhá
/// na pozadí (mimo hlavní smyčku) a nemusí být hotova ihned.
/// Kód je navržen jen pro toto jediné přerušení, jeho obsluha je mimo třídu !
class Exti0 {
  public:
    /// Konfigurace se nijak nedotýká připojeného GPIO pinu (Knihovna ST to neumí).
    void Config (void) {
      RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
      // NVIC - nízká priorita
      NVIC_SetPriority (EXTI0_IRQn, 3);
      NVIC_EnableIRQ   (EXTI0_IRQn);
    };
    /// Vyvolej přerušení
    void Intr (void) {
      EXTI->SWIER |= 1;
    };
    /// Nuluj příznak přerušení.
    void Clr (void) {
      EXTI->PR    |= 1;
    };
    /// Povolí přerušení od EXTI0
    void Start(void) {
      EXTI->IMR   |= 1;
    };
    /// Zakáže přerušení od EXTI0
    void Stop (void) {
      EXTI->IMR   &= ~1;
    };
};

#endif // EXTI0_H
