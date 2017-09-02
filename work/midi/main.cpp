/**
  * @file    main.cpp 
  * @author  Mrazik
  * @version V0.0.2
  * @date    29-12-2013
  * @brief   Main program body
  
  */ 
#include "main.h"
/**
  * @brief  Main program.
  * @retval 0 (none)
  */
extern "C" const unsigned char score [];

// Efekt je na F4 kolečko led s pwm, jinak rozsviť led
static Efect blink;
static MidiPlayer player;


// Když dohrajeme, skončí efekt
void EndPlaying (void) {
  blink.start (false);
}

int main(void) {
  player.setEnd (EndPlaying);
  blink .start  (true);
  player.start  (score);    // Hrajeme
  // smycka je platforme zavisla, v mcu muze byt prazdna
  while (PlatformMainLoop());
  return 0;
}

