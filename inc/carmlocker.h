#ifndef CARMLOCKER_H
#define CARMLOCKER_H
/** @file
 *  @brief  Architecture specific C/C++ locker / unlocker
 * 
 * Zákaz a povolení přerušení, specificky pro ARM Cortex-Mx.
 */

#if defined (__cplusplus)
extern "C" {
#endif
  // __ARM_ARCH_xM__ je predefined macro překladače pro Cortex-M0,M1,M3,M4
  #if __ARM_ARCH_6M__ | __ARM_ARCH_7M__ | __ARM_ARCH_7EM__
  //! [ARM locker]
    static inline void Lock (void) {
      asm volatile ("cpsid i");
    }
    static inline void UnLock (void) {
      asm volatile ("cpsie i");
    }
  //! [ARM locker]
  #else   //__ARM_ARCH_xM__
    static void Lock  (void){};
    static void UnLock(void){};
    //#warning "Bylo by dobre definovat zamykani pro tuto architekturu"
  #endif  //__ARM_ARCH_xM__
#if defined (__cplusplus)
}
#endif


#endif // CARMLOCKER_H
