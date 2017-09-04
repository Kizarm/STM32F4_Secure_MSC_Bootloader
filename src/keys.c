#include "keys.h"

const PrivateKeys_t SecretKeys = {
  .server = { 0x548BE7C9, 0x6A192FFF, 0x4F60B9AA, 0x25B2F8A2 },
  .loader = { 0x7090F460, 0x142A17AB, 0x5CFA0569, 0x37F47998 }
};

PrivateKeys_t GeneratedKeys;

enum { UUID_SIZE=4 };
struct UUID_s {
  uint32_t part [UUID_SIZE];
};
/*
 * uuid  = {part = {0x001e_0027, 0x3331_4718, 0x3537_3532, 0x0000_0400}};
 * */
static void xor_key (const uint32_t * inkey, uint32_t * outkey, struct UUID_s * u) {
  for (unsigned n=0; n<UUID_SIZE; n++) outkey[n] = inkey[n] ^ u->part[n];
}

#ifdef __arm__
struct UUID_r {
  volatile uint32_t ID [3];
};
struct FLID_r {
  volatile uint32_t SIZE;
};
static struct UUID_r * const UUIDR = (struct UUID_r * const)(0x1FFF7A10);
static struct FLID_r * const FLIDR = (struct FLID_r * const)(0x1FFF7A22);

static void GetUUID (struct UUID_s * u) {
  uint32_t n;
  for (n=0; n<3; n++)
    u->part[n] = UUIDR->ID[n];
  u->part[3] = FLIDR->SIZE & 0xFFFFu;
}

void generate_keys (void) {
  struct UUID_s uuid;
  GetUUID (&uuid);
  xor_key (SecretKeys.server, GeneratedKeys.server, &uuid);
  xor_key (SecretKeys.loader, GeneratedKeys.loader, &uuid);
}
#else // __arm__
void generate_keys (void) {
  struct UUID_s uuid = {.part = {0x001e0027, 0x33314718, 0x35373532, 0x00000000}};
  xor_key (SecretKeys.server, GeneratedKeys.server, &uuid);
  xor_key (SecretKeys.loader, GeneratedKeys.loader, &uuid);
}
#endif // __arm__
// http://127.0.0.1:8080/5495E7EE592868E77A578C9825B2FCA2/FIRMWARE.BIN
