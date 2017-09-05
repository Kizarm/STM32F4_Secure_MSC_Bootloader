#ifndef FIFO_H
#define FIFO_H
#include "carmlocker.h"

/** @file
 * @brief Fifo čili fronta.
 * 
 * @class Fifo
 * @brief Fifo čili fronta.
 * 
 * Fifo použité v obsluze periferií zkusíme napsat jako šablonu. Fakt je, že v tomto případě kód neroste.
 * Zřejmě je překladač docela inteligentní. A jde tak ukládat do fifo různá data, včetně složitých objektů.
 * Fifo je tak jednoduché, že může mít všechny metody v hlavičce, tedy default inline.
 * Je však třeba zajistit atomičnost inkrementace a dekrementace délky dat. Použita je metoda se zákazem
 * přerušení a snad funguje. Ona totiž pravděpodobnost, že se proces v přerušení zrovna trefí do té
 * in(de)krementace je tak malá, že je problém to otestovat na selhání, takže to celkem spolehlivě funguje
 * i bez ošetření té bezpečnosti. A u některých architektur je inkrementace či dekrementace buňky v paměti
 * atomická v principu. U této však nikoli. Viz :
 * 
  @code
      ...    safeDec();  ...
    8000364:       b672            cpsid   i
    8000366:       6d44            ldr     r4, [r0, #84]   ; 0x54
    8000368:       3c01            subs    r4, #1
    800036a:       6544            str     r4, [r0, #84]   ; 0x54
    8000328:       b662            cpsie   i
  @endcode
  
*/
//! [Fifo class example]
template <const unsigned N, class T> class Fifo {
  public:   // veřejné metody
    /// Parametr konstruktoru by měla být hloubka FIFO, ale pak by musela být dynamická alokace paměti.
    Fifo () {
      rdi = 0; wri = 0; len = 0;
    };
    /// Zápis do FIFO
    /// @param  c odkaz na jeden prvek co bude zapsán
    /// @return true, pokud není plný
    bool Write (const T & c) {
      if (len < N) { buf [wri++] = c; saturate (wri); safeInc(); return true; }
      else return false;
    };
    /// Čtení z FIFO
    /// @param  c odkaz na jeden prvek, kam to bude načteno
    /// @return true, pokud není prázdný
    bool Read  (T & c) {
      if (len > 0) { c = buf [rdi++]; saturate (rdi); safeDec(); return true; }
      else return false;
    };
  protected:  // chráněné metody
    /// Saturace indexu - ochrana před zápisem/čtením mimo pole dat
    /// @param index odkaz na saturovaný index
    void saturate (volatile unsigned & index) {
      if (index < N) return;
      index = 0; // FIFODEPTH obecně int
    };
    /// Atomická inkrementace délky dat
    void safeInc (void) { Lock(); len++; UnLock(); };
    /// Atomická dekrementace délky dat
    void safeDec (void) { Lock(); len--; UnLock(); };
  private:    // privátní data
    T                 buf[N];          //!< buffer na data
    volatile unsigned rdi, wri, len;   //!< pomocné proměnné
};
//! [Fifo class example]

#endif // FIFO_H
