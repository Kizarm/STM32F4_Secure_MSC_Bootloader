#ifndef GPIO_H
#define GPIO_H
#include <stdint.h>
/** 
  * @brief  GPIO Configuration Mode enumeration 
  */   
typedef enum { 
  GPIO_Mode_IN   = 0x00, /*!< GPIO Input Mode */
  GPIO_Mode_OUT  = 0x01, /*!< GPIO Output Mode */
  GPIO_Mode_AF   = 0x02, /*!< GPIO Alternate function Mode */
  GPIO_Mode_AN   = 0x03  /*!< GPIO Analog Mode */
}GPIOMode_TypeDef;

/** 
  * @brief  GPIO Output type enumeration 
  */  
typedef enum { 
  GPIO_OType_PP = 0x00,
  GPIO_OType_OD = 0x01
}GPIOOType_TypeDef;
/** 
  * @brief  GPIO Output Maximum frequency enumeration 
  */  
typedef enum { 
  GPIO_Speed_2MHz   = 0x00, /*!< Low speed */
  GPIO_Speed_25MHz  = 0x01, /*!< Medium speed */
  GPIO_Speed_50MHz  = 0x02, /*!< Fast speed */
  GPIO_Speed_100MHz = 0x03  /*!< High speed on 30 pF (80 MHz Output max speed on 15 pF) */
}GPIOSpeed_TypeDef;
/** 
  * @brief  GPIO Configuration PullUp PullDown enumeration 
  */ 
typedef enum { 
  GPIO_PuPd_NOPULL = 0x00,
  GPIO_PuPd_UP     = 0x01,
  GPIO_PuPd_DOWN   = 0x02
}GPIOPuPd_TypeDef;
typedef enum {
  GPIO_Dir_Mode_IN   = 0x00, /*!< GPIO Input Mode              */
  GPIO_Dir_Mode_OUT  = 0x01, /*!< GPIO Output Mode             */
} GPIODir_TypeDef;

/// Enum pro PortNumber
typedef enum {
  GpioPortA,
  GpioPortB,
  GpioPortC,
  GpioPortD,
  GpioPortE,
  GpioPortF,
  GpioPortG,
  GpioPortH,
  GpioPortI
} GpioPortNum;
/// Asociace port Adress a RCC clock
struct GpioAssocPort {
  uint32_t      clkMask;
};
/** @file
  * @brief Obecný GPIO pin.
  * 
  * @class GpioClass
  * @brief Obecný GPIO pin.
  * 
  *  Ukázka přetížení operátorů. Návratové hodnoty jsou v tomto případě celkem zbytečné,
  * ale umožňují řetězení, takže je možné napsat např.
    @code
    +-+-+-led;
    @endcode
  * a máme na led 3 pulsy. Je to sice blbost, ale funguje.
  * Všechny metody jsou konstantní, protože nemění data uvnitř třídy.
  * Vlastně ani nemohou, protože data jsou konstantní.
*/
#include <stdio.h>

class GpioClass {
  public:
    /** Konstruktor
    @param port GpioPortA | GpioPortB | GpioPortC | GpioPortD | GpioPortF
    @param no   číslo pinu na portu
    @param type IN, OUT, AF, AN default OUT 
    */
    GpioClass (GpioPortNum const port, const uint32_t no, const GPIOMode_TypeDef type = GPIO_Mode_OUT);
    /// Nastav pin @param b na tuto hodnotu
    const GpioClass& operator<< (const bool b) const {
      printf ("LED %d -> %d\n", num, b);
      return *this;
    }
//![Gpio example]
    /// Nastav pin na log. H
    const GpioClass& operator+ (void) const {
      printf ("LED %d -> H\n", num);
      return *this;
    }
    /// Nastav pin na log. L
    const GpioClass& operator- (void) const {
      printf ("LED %d -> L\n", num);
      return *this;
    }
    /// Změň hodnotu pinu
    const GpioClass& operator~ (void) const {
      printf ("LED %d -> ~\n", num);
      return *this;
    };
    /// Načti logickou hodnotu na pinu
    const bool get (void) const {
      return true;
    };
    /// A to samé jako operátor
    const GpioClass& operator>> (bool& b) const {
      b = get();
      return *this;
    }
//![Gpio example]
    void setMode (GPIOMode_TypeDef p) {
    }
    void setOType (GPIOOType_TypeDef p) {
    }
    void setSpeed (GPIOSpeed_TypeDef p) {
    }
    void setPuPd (GPIOPuPd_TypeDef p) {
    }
    void setAF (unsigned af) {
    }
    void setDir (GPIODir_TypeDef p) {
    }
  private:
    /// A pozice pinu na něm, stačí 16.bit
    const uint16_t      pos;
    /// pro funkce setXXX necháme i číslo pinu
    const uint16_t      num;
  
};

#endif // GPIO_H
