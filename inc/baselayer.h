#ifndef BASELAYER_H
#define BASELAYER_H
#include <stdlib.h>
#include <stdint.h>

#include "destruct.h"
/** @file
 * @brief Bázová třída pro stack trochu obecnějšího komunikačního protokolu.
 * 
 * @class BaseLayer
 * @brief Od této třídy budeme dále odvozovat ostatní.
 * 
 * Použití virtuálních metod umožňuje polymorfizmus. Pokud v odvozené třídě přetížíme nějakou
 * virtuální metodu, bude se používat ta přetížená, polud ne, použije se ta virtuální.
 * Třída nemá *.cpp, všechny metody jsou jednoduché a tedy inline.
*/
//! [BaseLayer example]
class BaseLayer {
  public:
    /** Konstruktor
    */
    BaseLayer () {
      pUp   = NULL;
      pDown = NULL;
    };
    // s virtualnim destruktorem jsou jen problemy, bez nej taky, ale mensi
    haveDestructor(virtual ~BaseLayer () = 0;)
    
    /** Virtuální metoda, přesouvající data směrem nahoru, pokud s nimi nechceme dělat něco jiného.
    @param data ukazatel na pole dat
    @param len  delka dat v bytech
    @return počet přenesených bytů
    */
    virtual uint32_t    Up   (const char* data, uint32_t len) {
      if (pUp) return pUp->Up (data, len);
      return 0;
    };
    /** Virtuální metoda, přesouvající data směrem dolů, pokud s nimi nechceme dělat něco jiného.
    @param data ukazatel na pole dat
    @param len  delka dat v bytech
    @return počet přenesených bytů
    */
    virtual uint32_t    Down (const char* data, uint32_t len) {
      if (pDown) return pDown->Down (data, len); 
      return len;
    };
    /** Zřetězení stacku
    @param bl Třída, ležící pod, spodní
    @return Odkaz na tuto třídu (aby se to dalo řetězit)
    */
    virtual BaseLayer& operator += (BaseLayer& bl) {
      bl.setUp (this);  // ta spodní bude volat při Up tuto třídu
      setDown  (&bl );  // a tato třída bude volat při Down tu spodní
      return *this;
    };
    /** Getter pro pDown
    @return pDown
    */
    BaseLayer* getDown (void) const { return pDown; };
  protected:
    /** Lokální setter pro pUp
    @param p Co budeme do pUp dávat
    */
    void setUp   (BaseLayer* p) { pUp   = p; };
    /** Lokální setter pro pDown
    @param p Co budeme do pDown dávat
    */
    void setDown (BaseLayer* p) { pDown = p; };
  private:
    // Ono to je vlastně oboustranně vázaný spojový seznam.
    BaseLayer * /*volatile*/  pUp;        //!< Ukazatel na třídu, která bude dále volat Up
    BaseLayer * /*volatile*/  pDown;      //!< Ukazatel na třídu, která bude dále volat Down
};
//! [BaseLayer example]

#endif // BASELAYER_H
