#ifndef ECHO_H
#define ECHO_H
/**
 * @file
 * @brief Příklad zpracování dat - Echo.
 * 
 * @class Echo
 * @brief Příklad zpracování dat - Echo.
 */

#include "baselayer.h"

class Echo : public BaseLayer {
  public:
    Echo      () : BaseLayer() {};
    // Pro konec, který je napsán jako Bottom
    uint32_t  Up (const char* data, uint32_t len) {
      // debug ("Echo.Up/Down %d\r\n", len);
      Down  (data, len);
      return BaseLayer::Up (data, len);
    };

};

#endif // ECHO_H
