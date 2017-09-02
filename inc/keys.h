#ifndef __KEYS_H__
#define __KEYS_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
  enum { TEA_KEY_SIZE = 4 };
  typedef struct PrivateKeys_s {
    uint32_t server [TEA_KEY_SIZE];
    uint32_t loader [TEA_KEY_SIZE];
  } PrivateKeys_t;
  extern const PrivateKeys_t SecretKeys;
  extern       PrivateKeys_t GeneratedKeys;
  extern void  generate_keys (void);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif /* __KEYS_H__ */
