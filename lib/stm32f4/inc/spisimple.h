#ifndef SPISIMPLE_H
#define SPISIMPLE_H
#include <stdint.h>
#include "gpio.h"

union SPI_CR1_REG {
  struct {
    uint16_t	CPHA     : 1;
    uint16_t	CPOL     : 1;
    uint16_t	MSTR     : 1;
    uint16_t	BR       : 3;
    uint16_t	SPE      : 1;
    uint16_t	LSBFIRST : 1;
    
    uint16_t	SSI      : 1;
    uint16_t	SSM      : 1;
    uint16_t	RXONLY   : 1;
    uint16_t	DFF      : 1;
    uint16_t	CRCNEXT  : 1;
    uint16_t	CRCEN    : 1;
    uint16_t	BIDIOE   : 1;
    uint16_t	BIDIMODE : 1;
  };
  uint16_t WB;
};
union SPI_CR2_REG {
  struct {
    uint16_t	TXDMAEN  : 1;
    uint16_t	RXDMAEN  : 1;
    uint16_t	SSOE     : 1;
    uint16_t	unused1  : 1;
    uint16_t	FRF      : 1;
    uint16_t	ERRIE    : 1;
    uint16_t	RXNEIE   : 1;
    uint16_t	TXEIE    : 1;
    
    uint16_t	unused2  : 8;
  };
  uint16_t WB;
};
enum SpiClockValues {
  SpiClockLow  = 7,
  SpiClockHigh = 2
};

class SpiSimple {
  public:
    SpiSimple();
    void     Init   (void);
    uint8_t  Transaction  (const uint8_t byte);
    void     Write  (const uint8_t byte) {
      Transaction (byte);
    }
    uint8_t  Read   (void) {
      return Transaction (0xFF);
    }
    uint32_t Send   (const uint8_t * buf, const uint32_t len);
    uint32_t Recv   (uint8_t       * buf, const uint32_t len);
    void     Clock  (SpiClockValues clock);
    void     Select (bool on);
  private:
    GpioClass sel;
};

#endif // SPISIMPLE_H
