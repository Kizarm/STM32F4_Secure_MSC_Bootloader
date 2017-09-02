#include "midiplayer.h"
#include "tone.h"
#include "gpio.h"
//#include "filtr.h"
#include "audio.h"
#include "carmlocker.h"

/**
 * @file
 * @brief Jednoduchý přehrávač midi souborů.
 * 
 * Kompletní midi obsahuje zvukové fonty, které jsou obrovské. Tohle je velice zjednodušené,
 * takže docela dobře přehrává skladby typu ragtime, orchestrální midi jsou skoro nepoužitelné.
 * Přesto se to pro jednoduché zvuky může hodit, protože je to poměrně nenáročné na systémové
 * prostředky. Může to fungovat dokonce i na 8-bitovém uP.
 * */
/// Instance
MidiPlayer * MidiPlayerInstance = 0;

/// Generátory tónů
static Tone  gens[maxGens];
/// Statické členské privátní proměnné
volatile int          MidiPlayer::pause;
Audio                 MidiPlayer::dac;
unsigned char const * MidiPlayer::melody;
callBackFunc          MidiPlayer::func;
Exti0                 MidiPlayer::soft;
/// Jednoduchá dolní propust - původně to mělo moc výšek
//  static Filtr iir;
/// Generuj vzorek pro všechny tóny  @return Vzorek
static inline int genSample (void) {
  int res = 0;
  for (unsigned int i=0; i<maxGens; i++) res += gens[i].step();
  // Ořežeme výšky
  // res = iir.run (res);
  // Pro jistotu omezíme - předejdeme chrastění
  if (res > maxValue) res = maxValue;
  if (res < minValue) res = minValue;
  // res += 0x800; // DAC mel ofset, zde neni
  return res;
}
/// Počítá další vzorek
int16_t MidiPlayer::nextSample (void) {
  if (pause) pause -= 1;  // Časování tónu
  else soft.Intr();       // Nový tón - vyvolá přerušení, obsloužené MidiPlayer::ToneChange
  return (int16_t) genSample ();
}
/// Konstruktor
MidiPlayer::MidiPlayer() {
  MidiPlayerInstance = this;
  dac.IrqEnable (true);
  soft.Config ();
  func = 0;
}
/// Nastavení callbacku pro ukončení playeru
void MidiPlayer::setEnd (callBackFunc fnc) {
  func = fnc;
}
/// Spušťění přehrávání
void MidiPlayer::start (unsigned char const* score) {
  dac.SetDriver (nextSample);
  melody = score;
  soft.Start ();    // Povolíme soft přerušení
  soft.Intr  ();    // a spustíme první.
}
/// Voláno fakticky pomocí soft.Intr()
void MidiPlayer::ToneChange (void) {
  soft.Clr();       // Vynuluj příznak
  
  unsigned char cmd, midt;
  unsigned int  geno;
  
  for (;;) {              // Pro všechny tóny před pauzou
    cmd = *melody++;
    if (cmd & 0x80) {     // event
      geno  = cmd & 0x0F;
      cmd >>= 4;
      switch (cmd) {
        case 0x8:         // off
          gens[geno].setMidiOff();
          break;
        case 0x9:         // on
          midt = *melody++;
          gens[geno].setMidiOn (midt); 
          break;
        default:
          soft.Stop ();
          dac.SetDriver (0);
          if (func) func ();
          return;         // melodie končí eventem 0xf0
      }
    } else {              // pause
      midt   = *melody++;
      // Výpočet hodnoty pausy by měl být v kritické sekci, ale funguje to i bez toho.
      // Když to trochu uteče, zase se z toho nestřílí, tak to nechme být.
      Lock ();
      pause  = ((unsigned int) cmd << 8) + midt;  // v ms
      pause *= AudioMidiDelay;        // ale máme vzorkování cca 44 kHz 
      UnLock ();
      return;
    }
  }
}
