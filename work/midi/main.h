#ifndef MAIN_H
#define MAIN_H

#include "gpio.h"
#include "leds.h"
#include "midiplayer.h"
#include "efect.h"

/** 
 * 
 * @author  Mrazik
 * @version V0.0.b
 * @date    20-12-2013
 
   @section Preface Orchestrion
 I malý ARM dokáže generovat polyfonní melodii s překvapivou kvalitou. Kromě výkonu to nezabere zase tak moc
 systémových prostředků. A výkonu máme dost. Takže s chutí do toho a můžeme pěkně hrát.
 
   @section Motivation Jak to celé vzniklo.
 Původně to měl být jen melodický zvonek s tím nejlevnějším procesorem Cortex-M0. Protože to fungovalo
 opravdu dobře, hledal jsem po netu, jak do toho dostat co nejjednodušeji melodii. Nabízí se midi formát.
 Sice mě dost odrazovalo jak velké jsou ty knihovny vzorků pro softwarové přehrávání, ale kdo hledá, ten najde. Je možné ty midi tóny hodně zjednodušit. Tón má poměrně dost různých atributů - kromě výšky, což je základ, ještě hlasitost, barvu (obsah harmonických, šum), náběh, doběh a čerti vědí co ještě. Pro každý nástroj je to jiné a to pak dotváří celkový dojem. Nemlaťte mě za nepřesnosti v terminologii, už je to moc dlouho, co jsem chodil do Lidušky a pak muzika mě zase tak moc nikdy nezajímala.
 
 Jediné co o tom umím říct - je možné z tónu ponechat prakticky jen jeho frekvenci (výšku) a pokud je midi soubor
 alespoň trochu rozumný, dá se to pak poslouchat.
 
   @section Function A jak to funguje.
 Začněme tím, jak generovat tón. Obvykle se to dělalo tak, že se použil časovač, ten po přetečení přerušil,
 zacvičilo se s výstupem a tak dále pořád kolem dokola. Modernější časovače už ani nemusí přerušit a umí
 cvičit s výstupem samostatně. Výstupem je obdélníkové napětí, které i když se prožene jednoduchým RC filtrem,
 stejně zní plechově. Nakonec polyfonní přehrávač, který mě k tomuto 
 inspiroval - <a href="https://code.google.com/p/arduino-playtune/">playtune</a> používá jen více těch časovačů.
 
 Ale nebojte se, půjdeme na to jinak. Máme výkonný 32-bit procesor, tak si můžeme dovolit použít stejný princip
 jako DDS od Analog Devices. Do podrobností nepůjdu, jsou na stránkách AD. Možná to bude lepší znázornit kouskem zdrojáku:
 @code
  int Tone::step (void) {
    unsigned int k,x;
    int y;
    // Spočteme index x pro přístup do tabulky
    x  = (base >> 8) & 0xFF;
    y  = onePeriod [x];     // vzorek vezmeme z tabulky
    // k je horní půlka amplitudy
    k  = ampl >> 16;
    y  *= k;                // vzorek násobíme amplitudou (tedy tím vrškem)
    y >>= 12;               // a vezmeme jen to, co potřebuje DAC
    k  *= fall;             // Konstanta fall určuje rychlost poklesu amplitudy,
    // čím více, tím je rychlejší. Pokud by bylo 1, pokles je 2^16 vzorků, což už je pomalé.
    base += freq;        // časová základna pro další vzorek
    ampl -= k;           // exponenciální pokles amplitudy
    // a je to
    return y;
  }
 @endcode
 Je to poněkud komplikovanější o exponenciální pokles amplitudy, ale bez toho ten zvuk nestojí za moc.
 Výhoda tohoto přístupu je výstup čistého sinusového napětí, bez nutnosti nějaké komplikované filtrace.
 Nevýhoda je výpočtová náročnost - funkce step() je volána periodicky s poměrně velkou frekvencí - zde cca 24kHz.
 Maximální výstupní frekvence je pak polovinou této - viz Nyquistův-Shannonův teorém. A zkomplikujme
 to ještě víc - čistý sinus je sice hezký, ale pro muziku příliš chudý zvuk. Tady je možné vygenerovat
 tabulku vzorků onePeriod[] tak, že kromě základní harmonické obsahuje i některé vyšší. Pokud to chceme
 mít polyfonní, pak je nutné mít těchto generátorů (gens) víc a jejich výstupy sčítat.
 @code
  static inline int genSample (void) {
    int res = 0;
    for (unsigned int i = 0; i < maxGens; i++) res += gens[i].step();
    // Pro jistotu omezíme - předejdeme chrastění
    if (res > maxValue) res = maxValue;
    if (res < minValue) res = minValue;
    res += 0x800;
    return res;
  }
 @endcode
 Tohle pak už jde rovnou do AD převodníku (proto to přičtení 0x800 - polovina rozsahu na konci).
 Je vidět, že je to hodně výpočtů - vyzkoušíme, jestli to bude procesor stíhat. genSample() dáme
 do smyčky a změříme opakovací frekvenci - na mém Discovery F0, taktovaném 48MHz to dělá pro 12 generátorů
   zhruba 35kHz, takže frekvence opakování 24kHz vyhoví, ale moc toho na další činnost nezbude.
   Ale dá se to zoptimalizovat - 12 generátorů je poměrně dost, někde stačí 4 a pro telefonní kvalitu
   stačí vzorkování 8kHz. Pak by to zabralo tak asi 10% strojového času, takže to může pracovat
   na pozadí a neobtěžovat příliš. Protože AD převodník není v procesorech moc běžný,
   bylo by asi možné použít jako výstup PWM, ale bude to asi trochu šumět. Příslušný časovač
   by pak bylo možné použít jako generátor vzorkovací frekvence. Zde je použit SysTick.
 
 Klíčový problém jak dostat do procesoru melodii vyřešil projekt
 <a href="http://code.google.com/p/miditones/">MIDITONES</a>. Umí to parsovat midi soubor
 a vytvoří z něj pole v jazyce C, obsahující jednoduché příkazy jaké tóny a jak dloho hrát.
 Není to dokonalé - zanedbává skoro všechny atributy tónu kromě výšky, ale vhodné midi soubory
 lze pak přehrát s překvapivě dobrou kvalitou. Velké orchestrální skladby samosebou nejsou vhodné,
 ale lze najít spoustu slušných skladeb např. typu ragtime, které jsou pro midi přímo určeny
 a ty jsou bez problému. Samotný program zabere asi 2KB FLASH a něco přes 200B RAM.
 Je to opravdu velice úsporné - mimo procesorového času.
 
 @section End Závěr.
 Tohle zase není stavební návod, jen funkční příklad pro inspiraci. Za použití C++ se omlouvám,
 ale mě se v tom fakt líp dělá. Funguje doplnění o targety Discovery F4 a Linux (unix) ALSA,
 kde je možné použít víc generátorů a vyšší vzorkování 44100 Hz. V F4 Discovery je použit zvuklový
 kodek na desce, takže i časování je jinak - vychází právě z tohoto kodeku. Nakonec je na tom doplněna
 i blbinka - blikací efekt, kerý pro ty 4 LED na desce používá PWM což dává plynulé přechody.
 
 * @file main.h
 * @brief Include pro main.cpp
 * */

#if defined (__cplusplus)
extern "C" {
#endif
  /// Tabulka vektorů je v čistém C
  extern  void SysTick_Handler (void);
  /// A volání main ostatně taky
  extern  int  main  (void);
#if defined (__cplusplus)
}
#endif


#endif // MAIN_H