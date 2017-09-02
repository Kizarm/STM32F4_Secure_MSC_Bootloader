#ifndef DACPLAYER_H
#define DACPLAYER_H
#include "audio.h"
#include "exti0.h"

bool PlatformMainLoop (void);
/// Prototyp pro funkci ukončení melodie.
typedef void (*callBackFunc) (void);
/// Třída, která hraje čistě na pozadí.
class MidiPlayer {
  // Veřejné metody
  public:
    /// Konstruktor
    MidiPlayer ();
    /// Start melodie v poli score
    void start  (unsigned char const* score);
    /// Pokud je potřeba vědět, kdy je konec
    void setEnd (callBackFunc fnc);
    /// Obsluha softwarového přerušení
    static void ToneChange    (void);
  // Chráněné metody
  protected:
    /// Obsluha vzorku
    static int16_t nextSample (void);
  private:
    /// vnitřní ukazatel na tóny
    static unsigned char const* melody;
    static Audio          dac;      //!< Výstup
    static volatile int   pause;    //!< Časování
    static callBackFunc   func;     //!< Uživatelská funkce na konci melodie
    static Exti0          soft;     //!< Softwarové přerušení prostřednictvím hardware
};

#endif // DACPLAYER_H
