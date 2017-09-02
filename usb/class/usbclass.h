#ifndef USBCLASS_H
#define USBCLASS_H
#include "usbinterface.h"

class UsbClass {
  public:
    UsbClass ();
    void append   (UsbClass     * p);
    void addIface (UsbInterface * p);
  private:
  public:
    UsbClass     * next;
    UsbInterface * list;
/** ************************************/
#ifdef DSCBLD
  public:
    virtual void     blddsc    (void);
    virtual unsigned enumerate (unsigned in);
#endif // DSCBLD
/** ************************************/    
};

#endif // USBCLASS_H
