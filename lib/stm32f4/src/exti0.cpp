/// Pro midi player.
#include "midiplayer.h"
// Obsluhy přerušení nutno vložit do tabulky vektorů - v g++ takto i když to vypadá divně.
extern "C" void EXTI0_IRQHandler (void) {
  MidiPlayer::ToneChange ();
}

