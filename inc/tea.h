#ifndef _TEA_H_
#define _TEA_H_
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
void encrypt_block (void * b, const uint32_t len, uint32_t const * const k);
void decrypt_block (void * b, const uint32_t len, uint32_t const * const k);
#ifdef __cplusplus
};
#endif //__cplusplus
#endif // _TEA_H_
