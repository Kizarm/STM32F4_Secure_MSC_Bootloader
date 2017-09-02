#include "gpio.h"
#include "usart.h"
#include "print.h"

/**
 * Test blikání při spolupráci se základem bootloaderu (base).
 * Zatím to funguje v RAM, ověřuje test firmware a skok do něj, včetně tabulky vektorů.
 * Obě části mají specifické linker skripty a specifický startovací kód.
 * 
 * 1.Linker skript zde má posunutý počátek - kam lze zjistit příkazem (pro base)
 * arm-none-eabi-nm firmware.elf | grep _external_code
 * 
 * 2.Na konci tabulky vektorů (ofset proti počátku 0x188) jsou 3 čísla charakteristická
 * pro firmware, správné doplnění zajistí externí program v adresáři post.
 * */

/*
static GpioClass ledg (GpioPortD, 12);
static GpioClass ledo (GpioPortD, 13);
static GpioClass ledr (GpioPortD, 14);
*/
static GpioClass ledb (GpioPortD, 15);

static volatile uint32_t TimerCount = 0;

extern "C" void SysTick_Handler (void) {
  TimerCount += 1;
  if (TimerCount > 100) {
    TimerCount = 0;
    ~ledb;
  }
}

int main (void) {
  SysTick_Config (SystemCoreClock / 1000); // 1 ms
  for (;;) {
  }
  return 0;
}
