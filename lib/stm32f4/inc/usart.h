#ifndef USARTCLASS_H
#define USARTCLASS_H

#include "baselayer.h"
#include "fifo.h"
/** @file
  * @brief Inicializace a obsluha obecného USARTu.

  * @class UsartClass
  * @brief Ve stacku dole - Bottom, pod tím jsou už jen dráty.
  *
  * Ukazuje použití jak dědičnosti - dědí vlastnosti třídy BaseLayer, tak polymorfizmu,
  * protože BaseLayer používá virtuální veřejné metody pro začlenění do stacku.
  * Kromě toho je sem zapouzdřena (kompozice) i třída Fifo, což je šablona.
  * Takže deklarace na několika řádcích obsahuje snad všechny vymoženosti,
  * které C++ nabízí a přitom bude generovat smysluplný a celkem použitelný kód.

  * Není to psáno obecně jako třeba periferní knihovny od ST, ale to je úmysl,
  * kód by měl být co nejmenší a přesto opakovatelně použitelný. Prakticky je potřeba
  * opravdu nastavit jen přenosovou rychlost, málokdy je potřeba měnit formát 8N1
  * nebo zapínat hardware flow control. A USART na tomto procesoru je tak složitý,
  * že prakticky všechny jeho speciální módy činnosti knihovna ST stejně neumí postihnout.
  */

enum UsartEnum {
  UsartNo1 = 0,
  UsartNo2,
  UsartNo3,
  UsartNo4,
  MAXUSARTS
};
struct USART_Type_s;

class UsartClass : public BaseLayer {
  public:
    /// Konstruktor
    ///  @param no   Číslo portu v procesoru (enum).
    ///  @param baud Přenosová rychlost v Bd.
    UsartClass (UsartEnum no, uint32_t baud );
    haveDestructor(~UsartClass () {Fini();};)
    /// Přetížení - data (přes FIFO) přímo na drát
    /// @param data ukazatel na data
    /// @param len  délka dat v bytech
    /// @return počet skutečně přenesených bytů (může se od len dost lišit)
    uint32_t    Down ( const char* data, uint32_t len );
    /// Takhle by se daly přidávat další nastavení
    void        SetHalfDuplex ( bool on ) const;
    void        SetBaud       ( uint32_t baud ) const;
    /// pro komatibilitu s USB CDC
    void        SetLineParams ( uint32_t baud, uint8_t lcr ) {
    }
    void        Init          ( uint32_t baud );
    void        Fini          (void);
  protected:
    /// Obsluha přerušení.
    void        irq  ( void );
    static void USART1_IRQ (void);
    static void USART2_IRQ (void);
    static void USART3_IRQ (void);
    static void UART4_IRQ  (void);
  private:
    USART_Type_s* const  port; //!< Adresa registrů
    uint32_t      const  uno;  //!< číslo usartu (fakticky UsartEnum no)
    Fifo<64, char>       tx;   //!< Zajišťuje, že metoda Down neblokuje program
};

#endif // USARTCLASS_H
