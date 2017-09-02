#ifndef _DEC_BLD_H
#define _DEC_BLD_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#ifdef DSCBLD
static const unsigned char UsbDeviceDescriptor []={0};
static const unsigned char UsbConfigDescriptor []={0};
static const unsigned char UsbStringDescriptor []={0};

static const unsigned UsbDeviceDescriptorLen=0;
static const unsigned UsbConfigDescriptorLen=0;
static const unsigned UsbStringDescriptorLen=0;

#else // DSCBLD
extern const unsigned char UsbDeviceDescriptor [];
extern const unsigned char UsbConfigDescriptor [];
extern const unsigned char UsbStringDescriptor [];

extern const unsigned UsbDeviceDescriptorLen;
extern const unsigned UsbConfigDescriptorLen;
extern const unsigned UsbStringDescriptorLen;
#endif // DSCBLD

#ifdef __cplusplus
};
/** ************************************/
#ifdef DSCBLD
#include <stdio.h>

const unsigned MaxUsbStrings = 32;
struct StringNumbering {
  unsigned     index;
  char const * string;
};

extern FILE*    cDesc;
extern unsigned IfCount, StrCount;
extern StringNumbering StringField [MaxUsbStrings];

extern void     StructOut (const void * p,   unsigned l);
extern unsigned AddString (char const * str, unsigned & index);

/** ************************************/
#endif // DSCBLD
#endif //__cplusplus


#endif // _DEC_BLD_H
