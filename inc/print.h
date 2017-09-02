#ifndef PRINT_H
#define PRINT_H

#include "baselayer.h"

#define EOL "\r\n"
#define BUFLEN 64

/**
 * @file
 * @brief Něco jako ostream.
 * 
 */

/// Základy pro zobrazení čísla.
enum PrintBases {
  BIN=1, OCT=3, DEC=10, HEX=4
};

/**
 * @class Print
 * @brief Třída pro výpisy do Down().
 * 
 * @section Part3 STM32F0 a C++, část 3.
 * 
 * V minulé části jsme postavili jednoduchý Stack pro obecný komunikační protokol.
 * Teď do něj začleníme třídu, která umožní vypsat string a číslo třeba na sériový port.
 * A nakonec popíšeme a opravíme známé chyby z minulých částí.
 * 
 * @section Print 3. Process z předešlé části - zde začleníme třídu Print.
 * 
 * V main pak přibude jen definice instance této třídy
 * @code
    static  Print             print;                // obrázek - 3
 * @endcode
 * a ukázka, jak se s tím pracuje:
 * @snippet main.cpp Main print example
 * Nic na tom není - operátor << má přetížení pro string, číslo a volbu formátu čísla (enum PrintBases).
 * Výstup je pak do bufferu a aby nám to "neutíkalo", tedy aby se vypsalo vše,
 * zavedeme blokování, vycházející z toho, že spodní třída vrátí jen počet bytů,
 * které skutečně odeslala. Při čekání spí, takže nepoužívat v přerušení.
 * @snippet src/print.cpp Block example
 * Toto blokování pak není použito ve vrchních třídách stacku,
 * blokovaná metoda je BlockDown(). Pokud bychom použili přímo Down(), blokování by pak
 * používaly všechny vrstvy nad tím. A protože mohou Down() používat v přerušení, byl by problém.
 *
 * Metody pro výpisy jsou sice dost zjednodušené, ale zase to nezabere
 * moc místa - pro ladění se to použít dá. Délka vypisovaného stringu není omezena
 * délkou použitého buferu.
 * 
 * @section Errors Známé chyby.
 * 
 * V minulých částech bylo několik nesrovnalostí, které mohou (ale nemusí) působit problémy.
 * Zkusím to zde uvést trochu na pravou míru.
 * -# Přetypování const char* na char*.\n
 *      To opravdu není dobrý nápad. Zde to bylo použito dokonce na více místech a víceméně
 * se nic strašného nestalo. Co by se však stát mohlo je, že const char* pochopí překladač
 * (zcela správně) jako ukazatel na data ve flash a pokud pak ukazatel už jako nekonstantní předá funkci,
 * která se bude snažit na toto místo zapsat, protože už netuší, že je to konstanta ve flash,
 * program zhavaruje na hard fault. Takže takto opravdu ne.
 * -# Zarovnání sekce .init_array.\n
 *      To mi nějak uniklo a je to dost podstatná chyba, vyskytující se v obou předchozích
 * částech. Prostě ne vše se projeví ihned. Takže správně je:
 * @code
    . = ALIGN(4); //  <---  Tohle je ten problém.
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end   = .);
 * @endcode
 * 
 * @section Finit Finále.
 * Nakonec jsem si ještě trochu pohrál s GpioClass a trochu změnil strukturu adresářů.
 * Všechny ty funkce Up a Down mají první parametr const char* místo char*. Je to tak lepší a vlastně
 * to ničemu nevadí. Při uřezávání hlaviček se stejně jen posune ukazatel, a pokud budu chtít
 * hlavičky přidávat nebo jinak manipulovat s daty, stejně musím vytvořit lokální kopii dat,
 * protože uvnitř funkce netuším, jak velký bufer mi byl předán. Dokonce to umožnilo zkrátit kód o
 * pár bytů.
 *  
 * Pro ladění v RAM byl přidán linker skript, i když zapnout příslušnou option, tak
 * aby program běžel z RAM byl fakt porod. OpenOcd skutečně není dokonalé. Text minulého článku
 * opravovat nebudu, nechť je vidět, že chybami se člověk učí, opravená verze se dá vygenerovat
 * z tohoto doxygenem. A stejně i v tomhle asi nějaké chyby zbyly.
 * 
 * A nakonec zase zdrojáky k této části.
 */

class Print : public BaseLayer {
  public:
    /// Konstruktor @param b Default decimální výpisy.
    Print     (PrintBases b = DEC);
    /// Blokování výstupu
    /// @param buf Ukazatel na data
    /// @param len Délka přenášených dat
    /// @return Počet přenesených bytů (rovno len)
    uint32_t  BlockDown   (const char* buf, uint32_t len);
    /// Výstup řetězce bytů
    /// @param str Ukazatel na řetězec
    /// @return Odkaz na tuto třídu kvůli řetězení.
    Print&    operator << (const char* str);
    /// Výstup celého čísla podle base
    /// @param num Číslo
    /// @return Odkaz na tuto třídu kvůli řetězení.
    Print&    operator << (const int   num);
    /// Změna základu pro výstup čísla
    /// @param num enum PrintBases
    /// @return Odkaz na tuto třídu kvůli řetězení.
    Print&    operator << (const PrintBases num);
    void out (const void* p, uint32_t l);
  private:
    PrintBases base;    //!< Základ pro výstup čísla.
    char  buf[BUFLEN];  //!< Buffer pro výstup čísla.
};

#endif // PRINT_H
