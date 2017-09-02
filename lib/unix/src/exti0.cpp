/// Pro midi player.
#include <stdio.h>
#include "midiplayer.h"

extern MidiPlayer * MidiPlayerInstance;

void Exti0::Intr (void) {
  MidiPlayerInstance->ToneChange();
  //printf("ToneChange\n");
}