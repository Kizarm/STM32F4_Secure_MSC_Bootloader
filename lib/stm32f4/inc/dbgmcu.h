#ifndef DBG_MCU_H
#define DBG_MCU_H
/**
 * @file
 * @brief Povolení debug v režimu sleep.
 * 
 * Pro STM32F4 je nutné povolit debug v úsporných režimech, včetně sleep. To není nutné např.
 * pro STM32F0. Režimy Standby a Stop nebyly vyzkoušeny, patrně by bylo nutné použít i pro F0.
 */
//#include "core_cm4.h"
/// Control registr DEBUG MCU
typedef union {
  struct {
    uint32_t DBG_SLEEP   : 1;   //!< Debug Sleep mode
    uint32_t DBG_STOP    : 1;   //!< Debug Stop mode
    uint32_t DBG_STANDBY : 1;   //!< Debug Standby mode
    uint32_t UNUSED1     : 2;
    uint32_t TRACE_IOEN  : 1;   //!< Trace pin assignment control
    uint32_t TRACE_MODE  : 2;   //!< TRACE pin assignment for Asynchronous/Synchronous Mode, TRACEDATA size
    uint32_t UNUSED2     : 24;
  }        BIT;                 //!< jednotlivé bity
  uint32_t VAL;                 //!< Celková hodnota
} DBGMCU_CR;
/// Blok řízení DEBUG MCU
typedef struct {
  volatile uint32_t  IDCODE;    //!< ID procesoru
  volatile DBGMCU_CR CR;        //!< Řízení debug při low power modech.
  volatile uint32_t  APB1_FZ;   //!< chování periferií při debug
  volatile uint32_t  APB2_FZ;   //!< chování periferií při debug
} DBGMCU_F4;
/// Adresa DBGMCU bloku
static DBGMCU_F4 * const  MDBGMCU = (DBGMCU_F4 *) 0xE0042000;
/// Pokud použijeme v programu wfi/wfe, je nutné povolit debug i v tt. módu.
static inline void EnableDebugOnSleep (void) {
  MDBGMCU->CR.BIT.DBG_SLEEP = 1;
}

#endif //  DBG_MCU_H
