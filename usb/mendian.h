#ifndef MENDIAN_H
#define MENDIAN_H
#if 1
// Pro 16.bit musíme struktury pakovat
#define PACKSTRUCT __attribute__((packed))
// Specificky podle pořadí bytů - LE je pro ARM i X86 normální.
// Není třeba nic definovat - __BYTE_ORDER__,__ORDER_XXXX_ENDIAN__,
#include <stdint.h>
#if __GNUC__ < 4
#error "Not supported compiler"
#endif // __GNUC__
#if __GNUC_MINOR__ < 8
/// V nizsich verzich gcc neni 16.bit swap builtin
static volatile uint16_t __builtin_bswap16 (uint16_t n) {
  return ((n >> 8) & 0x00FF) | ((n << 8) & 0xFF00);
}
#endif // __GNUC_MINOR__
// __builtin_bswapNN jsou interní makra překladače, správně definované
// pro danou architekturu. V gcc je to tedy platform independent. 
#if   __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__         // 1234
  #define cpu_to_be32 __builtin_bswap32
  #define be32_to_cpu __builtin_bswap32
  #define cpu_to_le32
  #define le32_to_cpu
  
  #define cpu_to_be16 __builtin_bswap16
  #define be16_to_cpu __builtin_bswap16
  #define cpu_to_le16
  #define le16_to_cpu
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__            // 4321
  #define cpu_to_be32
  #define be32_to_cpu
  #define cpu_to_le32 __builtin_bswap32
  #define le32_to_cpu __builtin_bswap32
  
  #define cpu_to_be16
  #define be16_to_cpu
  #define cpu_to_le16 __builtin_bswap16
  #define le16_to_cpu __builtin_bswap16
#else
#error "Unknown ENDIAN"
#endif

/* ******************** 32. bit **************************/
class __be32 {
  public:
    __be32 () {num=0;};
    __be32 (const uint32_t p)  {num = cpu_to_be32 (p);};
    __be32 (const __be32 & p)  {num = p.num;          };
    /*
    uint32_t  operator ~ (void) const {		// unary ~ prefix, nelze jednoduse =
      return be32_to_cpu (num);
    }
    */
    uint32_t  get (void) const {return be32_to_cpu (num);};
    __be32 &  operator = (const uint32_t p) {	// Zpet ano, binary
      num  = cpu_to_be32 (p);
      return * this;
    }
    __be32 &  operator = (const __be32   p) {	// Pretizime
      num  = p.num;
      return * this;
    }
  private:
    uint32_t  num;
}PACKSTRUCT;

class __le32 {
  public:
    __le32 () {num=0;};
    __le32 (const uint32_t p)  {num = cpu_to_le32 (p);};
    __le32 (const __le32 & p)  {num = p.num;          };
    /*
    uint32_t  operator ~ (void) const {		// unary ~ prefix, nelze jednoduse =
      return le32_to_cpu (num);
    }
    */
    uint32_t  get (void) const {return le32_to_cpu (num);};
    __le32 &  operator = (const uint32_t p) {	// Zpet ano, binary
      num  = cpu_to_le32 (p);
      return * this;
    }
    __le32 &  operator = (const __le32   p) {	// Pretizime
      num  = p.num;
      return * this;
    }
    
    const bool operator != (const __le32 p) const {
      if (num != p.num) return true;
      else              return false;
    }
    const bool operator == (const __le32 p) const {
      if (num == p.num) return true;
      else              return false;
    }
    
  private:
    uint32_t  num;
}PACKSTRUCT;
/* ******************** 16. bit **************************/
class __be16 {
  public:
    __be16 () {num=0;};
    __be16 (const uint16_t p)  {num = cpu_to_be16 (p);};
    __be16 (const __be16 & p)  {num = p.num;          };
    /*
    uint16_t  operator ~ (void) const {		// unary ~ prefix, nelze jednoduse =
      return be16_to_cpu (num);
    }
    */
    uint16_t  get (void) const {return be16_to_cpu (num);};
    __be16 &  operator = (const uint16_t p) {	// Zpet ano, binary
      num  = cpu_to_be16 (p);
      return * this;
    }
    __be16 &  operator = (const __be16   p) {	// Pretizime
      num  = p.num;
      return * this;
    }
  private:
    uint16_t  num;
}PACKSTRUCT;

class __le16 {
  public:
    __le16 () {num=0;};
    __le16 (const uint16_t p)  {num = cpu_to_le16 (p);};
    __le16 (const __le16 & p)  {num = p.num;          };
    /*
    uint16_t  operator ~ (void) const {		// unary ~ prefix, nelze jednoduse =
      return le16_to_cpu (num);
    }
    */
    uint16_t  get (void) const {return le16_to_cpu (num);};
    __le16 &  operator = (const uint16_t p) {	// Zpet ano, binary
      num  = cpu_to_le16 (p);
      return * this;
    }
    __le16 &  operator = (const __le16   p) {	// Pretizime
      num  = p.num;
      return * this;
    }
  private:
    uint16_t  num;
}PACKSTRUCT;

typedef unsigned char __u8;
#else
#include <stdint.h>
typedef uint8_t   __u8;
typedef uint16_t  __le16;
typedef uint32_t  __le32;
#endif // 0

#endif // MENDIAN_H