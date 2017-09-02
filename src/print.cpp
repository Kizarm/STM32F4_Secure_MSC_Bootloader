#include "print.h"

#if ARCH_CM0
// Pomocné datové typy pro dělení.
union udiv {
  struct {
    uint32_t div;         // R0
    uint32_t rem;         // R1
  }        ds;
  uint64_t ll;
};
// Divné, ale pokud aeabi funguje správně, neměl by být problém. Vstup R0 num, R1 den.
// Funkce vrací v R0 výsledek dělení num/den a v R1 zbytek - eabi by mělo umět použít.
extern "C" uint64_t __aeabi_uidivmod (uint32_t num, uint32_t den); // interně v gcc, g++

static inline void sleep (void) {
  asm volatile ("wfi");
}
#else  // ARCH_CM0
  #define sleep()
#endif //ARCH_CM0

static const char*    hexStr   = "0123456789ABCDEF";
static const uint16_t numLen[] = {1, 32, 1, 11, 8, 0};

Print::Print (PrintBases b) : BaseLayer () {
  base = b;
}
//![Block example]
// Výstup blokujeme podle toho, co se vrací ze spodní vrstvy
uint32_t Print::BlockDown (const char* buf, uint32_t len) {
  uint32_t n, ofs = 0, req = len;
  for (;;) {
    // spodní vrstva může vrátit i nulu, pokud je FIFO plné
    n = BaseLayer::Down (buf + ofs, req);
    ofs += n;   // Posuneme ukazatel
    req -= n;   // Zmenšíme další požadavek
    if (!req) break;
    sleep();    // A klidně můžeme spát
  }
  return ofs;
}
//![Block example]

Print& Print::operator<< (const char* str) {
  uint32_t i = 0;
  while (str[i++]);             // strlen
  BlockDown (str, --i);
  return *this;
}

Print& Print::operator<< (const int num) {
  uint32_t i = BUFLEN;
  
  if (base == DEC) {
    unsigned int  u;
    if (num < 0) u = -num;
    else         u =  num;
    do {
      // Knihovní div() je nevhodné - dělí 2x.
#if ARCH_CM0
      udiv ud; // Dělení trvá dlouho, proto to zkusíme udělat najednou
      // Pokud eabi funguje správně, mělo by to chodit.
      ud.ll = __aeabi_uidivmod (u, (unsigned) DEC);
      buf [--i] = hexStr [ud.ds.rem];
      u = ud.ds.div;
#else // ARCH_CM0
      // Přímočaré a funkční řešení 
      uint32_t rem;
      rem = u % (unsigned) DEC;        // 1.dělení
      u   = u / (unsigned) DEC;        // 2.dělení
      buf [--i] = hexStr [rem];
#endif// ARCH_CM0
    } while (u);
    if (num < 0) buf [--i] = '-';
  } else {
    uint32_t m = (1U << (uint32_t) base) - 1U;
    uint32_t l = (uint32_t) numLen [(int) base];
    uint32_t u = (uint32_t) num;
    for (unsigned n=0; n<l; n++) {
      buf [--i] = hexStr [u & m];
      u >>= (unsigned) base;
    }
    if (base == BIN) buf [--i] = 'b';
    if (base == HEX) buf [--i] = 'x';
    buf [--i] = '0';
  }
  BlockDown (buf+i, BUFLEN-i);
  return *this;
}

Print& Print::operator<< (const PrintBases num) {
  base = num;
  return *this;
}
void Print::out (const void* p, uint32_t l) {
  const unsigned char* q = (const unsigned char*) p;
  unsigned char uc;
  uint32_t      k, n = 0;
  for (uint32_t i=0; i<l; i++) {
    uc = q[i];
    buf[n++] = '<';
    k = uc >> 4;
    buf[n++] = hexStr [k];
    k = uc  & 0x0f;
    buf[n++] = hexStr [k];
    buf[n++] = '>';
  }
  buf[n++] = '\r';
  buf[n++] = '\n';
  BlockDown (buf, n);
}

