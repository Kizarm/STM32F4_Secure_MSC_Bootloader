#ifndef MIRROR_H
#define MIRROR_H

#include <stdio.h>
#include "baselayer.h"

/**
 * @file
 * @brief Obraceč datového toku.
 * @class Mirror
 * @brief Obraceč datového toku má 2 třídy.
 * 
 * TwoTop je vlastní obraceč, dědí vlastnosti BaseLayer, pouze metoda Up je přetížena -
 * to je to vlastní převrácení. Hlavní třída Mirror, jejíž instance je pak v kódu použita
 * obsahuje 2 rovnocenné instance  třídy TwoTop, které musí být ve stacku vždy navrchu.
 * První část stacku volá Up jedné instance, druhá Up té druhé. Proto tam musí být dvě.
 * Podobně by šel udělat něco jako Fork, odbočení datového toku.
  
 * A zase - všechny metody jsou tak jednoduché, že jsou celé v hlavičce, tedy inline.
 * 
 * @class TwoTop
 * @brief Vlastní obraceč.
*/

class TwoTop  : public BaseLayer {
  public:
    /// Konstruktor
    TwoTop () : BaseLayer () {};
// ~TwoTop () {};
    /// Setter pro x
    void setX (BaseLayer* bl) { x = bl; };
    /// Přetížení metody Up, nic jiného není potřeba
    uint32_t    Up   (const char* data, uint32_t len) {
      // debug ("   TT.Up   %d\r\n", len);
      if (!x) return 0;
      // To, co přišlo zespoda, pošlu druhou instancí zase dolů
      return x->Down (data, len);
    };
  private:
    /// Fakticky ukazatel na druhou instanci TwoTop
    BaseLayer* x;
};

class Mirror {
  public:    
    /// Konstruktor
    Mirror () : L(), R() {
      L.setX  (&R);   // Překřížení
      R.setX  (&L);
    };
    /**
    Zřetězení voláme 2x. Poprvé pro jednu, podruhé pro druhou instanci TwoTop.
    Protože je tato třída navrchu (vlevo), operátor nic nevrací. Protože L a R
    jsou rovnoprávné, je jedno která část stacku se postaví dřív.
    @param bl Left
    */
    void operator += (BaseLayer& bl) {
      if (!L.getDown()) { L += bl; return; };
      if (!R.getDown()) { R += bl; return; };
    };
  private:
    TwoTop       L, R;   //!< 2 instance Top
};

#endif // MIRROR_H
