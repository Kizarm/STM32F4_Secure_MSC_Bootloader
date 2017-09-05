#include <string.h>
#include "debug.h"
#include "msclass.h"
#include "fat12emul.h"

/** NXP bootloader používá zřejmě fintu - disk se zdá být plný, tj. firmware.bin zabírá
 * jakoby všechno volné místo. To nutí operační systém (OS) aby data přepsal, nemůže je
 * prostě dát na volné místo protože žádné nemá. To je zřejmě příčina, proč to ve Windows
 * funguje. V Linuxu cp opět funguje, přetažení selže, ale pokud před přetažením soubor
 * FIRMWARE.BIN z disku smažeme, funguje to. Problém je, že při případném čtení souboru
 * z disku se zdá být tento velký a načteme moc dat, která k ničemu nejsou. Ovšem tuto
 * drobnou vadu má i bootloader NXP (a zřejmě všechny podobné). Tak si vyber.
 * 
 * */
#define  NXP_LIKE
//#undef NXP_LIKE

/** ***********************************************************************/
extern "C" const char          index_ctxt []; // in context.s
extern "C" const unsigned long index_size;
extern "C" char                image_ctxt [];
extern "C" unsigned long       image_size;
/** ***********************************************************************/
struct BIOS_Parameter_Block {
  uint16_t Bytes_Per_Logical_Sector;
  uint8_t  Logical_Sectors_Per_Cluster;
  uint16_t Reserved_Logical_Sectors;
  uint8_t  Number_Of_FATs;
  uint16_t Maximum_Directory_Entries;
  uint16_t Total_Logical_Sectors;
  uint8_t  Media_Descriptor;
  uint16_t Logical_Sectors_Per_FAT;
  /// DOS 3.0
  uint16_t E0,E1,E2;
}__attribute__((packed));

struct BootSect {
  uint8_t   jump                  [3];
  uint8_t   OEM_Name              [8];
  struct BIOS_Parameter_Block BPB;        // 19 -> 30
//uint8_t   Boot_Code             [479];
//uint8_t   Physical_Drive_Number; // 0
//uint16_t  Boot_Sector_Signature; // dame na konec
}__attribute__((packed));
static const uint16_t Valid_Boot_Sector_Signature = 0xAA55;
static const BootSect BootSector = {
  .jump     = {0xEB, 0x3C, 0x90},
  .OEM_Name = {0x6D, 0x6B, 0x66, 0x73, 0x2E, 0x66, 0x61, 0x74},
  .BPB = {
    .Bytes_Per_Logical_Sector    = 512,
    .Logical_Sectors_Per_Cluster = 2,         // 1KiB, asi lze i 4KiB (vetsi disk)
    .Reserved_Logical_Sectors    = 1,
    .Number_Of_FATs              = 1,
    .Maximum_Directory_Entries   = 16,
    .Total_Logical_Sectors       = 128, 
    .Media_Descriptor            = 0xF8,
    .Logical_Sectors_Per_FAT     = 2
  }
};
/** ***********************************************************************/
union two_fat_entry12 {
  uint8_t  u [3];
  struct {
    uint16_t l : 12;
    uint16_t h : 12;
  }__attribute__((packed));
};
static inline uint16_t read_fat_entry_n (two_fat_entry12 * f, unsigned n) {
  unsigned m = n >> 1;
  if (n&1) return f[m].h;
  else     return f[m].l;
}
static inline void write_fat_entry_n (two_fat_entry12 * f, unsigned n, uint16_t t) {
  unsigned m = n >> 1;
  if (n&1) f[m].h = t;
  else     f[m].l = t;
}
/** ***********************************************************************/
union FileAttributes {
  struct {
  uint8_t Read_only    : 1;
  uint8_t Hidden       : 1;
  uint8_t System       : 1;
  uint8_t Volume_label : 1;
  
  uint8_t Subdirectory : 1;
  uint8_t Archive      : 1;
  uint8_t Unused       : 2;
  }__attribute__((packed));
  uint8_t Attributes;
};
struct DateBits {
  uint16_t Day     : 5;
  uint16_t Month   : 4;
  uint16_t Year    : 7; // 0 = 1980
}__attribute__((packed));
struct TimeBits {
  uint16_t Seconds : 5; // 0 - 30, polovina
  uint16_t Minutes : 6;
  uint16_t Hours   : 5;
}__attribute__((packed));
struct DirectoryEntryFat {
  char                 Filename  [8];
  char                 Extension [3];
  union FileAttributes Attributes;
  uint16_t             Reserved;
  struct TimeBits      Creation_Time;
  struct DateBits      Creation_Date;
  struct DateBits      Last_Access_Date;
  uint16_t             Ignore_in_FAT12;
  struct TimeBits      Last_Write_Time;
  struct DateBits      Last_Write_Date;
  uint16_t             First_Logical_Cluster;
  uint32_t             File_Size;
}__attribute__((packed));
struct DirectoryEntryLFN {
  uint8_t              Sequence_Number;
  uint16_t             Name1 [5];
  union FileAttributes Attributes; // 0x0F, jinde nepouzito
  uint8_t              Type;       // 0
  uint8_t              Checksum;   // of DOS file name
  uint16_t             Name2 [6];
  uint16_t             First_Logical_Cluster;
  uint16_t             Name3 [2];
}__attribute__((packed));

union DirectoryEntry {
  struct DirectoryEntryFat f;
  struct DirectoryEntryLFN l;
  uint8_t EntryByte [sizeof (DirectoryEntryFat)];
};
static const DirectoryEntry DirEntries [] = {
  {.EntryByte = { 0x42, 0x4F, 0x4F, 0x54, 0x4C, 0x4F, 0x41, 0x44, 0x45, 0x52, 0x20, 0x08, 0x00, 0x00, 0xF5, 0x8B,
                  0x17, 0x4B, 0x17, 0x4B, 0x00, 0x00, 0xF5, 0x8B, 0x17, 0x4B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }},
  {.EntryByte = { 0x41, 0x69, 0x00, 0x6E, 0x00, 0x64, 0x00, 0x65, 0x00, 0x78, 0x00, 0x0F, 0x00, 0x33, 0x2E, 0x00,
                  0x68, 0x00, 0x74, 0x00, 0x6D, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF }},
  {.EntryByte = { 0x49, 0x4E, 0x44, 0x45, 0x58, 0x7E, 0x31, 0x20, 0x48, 0x54, 0x4D, 0x21, 0x00, 0x64, 0xF5, 0x7B,
                  0x17, 0x4B, 0x17, 0x4B, 0x00, 0x00, 0xF5, 0x7B, 0x17, 0x4B, 0x02, 0x00, 0x90, 0x01, 0x00, 0x00 }},
  {.EntryByte = { 0x46, 0x49, 0x52, 0x4D, 0x57, 0x41, 0x52, 0x45, 0x42, 0x49, 0x4E, 0x20, 0x00, 0x64, 0xF5, 0x7B,
                  0x17, 0x4B, 0x17, 0x4B, 0x00, 0x00, 0xF5, 0x7B, 0x17, 0x4B, 0x03, 0x00, 0x00, 0x80, 0x00, 0x00 }}
};
/** ***********************************************************************/
#include "keys.h"
static uint32_t toHex (uint8_t * p, uint32_t n) {
  const unsigned w = 8;
  const char * hex = "0123456789ABCDEF";
  p += w;
  for (unsigned i=0; i<w; i++) {
    *(--p) = hex [n & 0x0F];
    n >>= 4;
  }
  return w;
}
/** ***********************************************************************/
#ifdef __arm__
static inline void printerr (const unsigned char * o, const unsigned char * n, unsigned len, unsigned ofs) {};
#else  //  __arm__
static void check_chunk (const unsigned char * o, const unsigned char * n, unsigned len, unsigned no) {
  unsigned errcnt = 0;
  char sign [len];
  memset (sign, 0, len);
  for (unsigned i=0; i<len; i++) {
    if (o[i] != n[i]) {
      errcnt += 1;
      sign[i] = 1;
    }
  }
  if (errcnt) {
    printf ("0x%04X:", no);
    for (unsigned i=0; i<len; i++) {
      if (sign[i]) printf ("\x1B[34;1m %02X\x1b[0m", o[i]);
      else         printf ("\x1B[32;1m %02X\x1b[0m", o[i]);
    }
    printf ("\n       ");
    for (unsigned i=0; i<len; i++) {
      if (sign[i]) printf ("\x1B[31;1m %02X\x1b[0m", n[i]);
      else         printf ("   ");
    }
    printf("\n");
  }
}
static void printerr (const unsigned char * o, const unsigned char * n, unsigned len, unsigned ofs) {
  const unsigned chunk_size = 32;
  unsigned index=ofs, count=0, step=chunk_size;
  while (len) {
    if (len < step) step = len;
    check_chunk (o + count, n + count, step, index);
    index += step;
    len   -= step;
    count += step;
  }
}
#endif //  __arm__
/** ***********************************************************************/

Fat12Emul::Fat12Emul () : StorageBase(), memory(), block (BootSector.BPB.Bytes_Per_Logical_Sector) {
  rwlen = 0u;
  rwofs = 0u;
  capacity = BootSector.BPB.Total_Logical_Sectors;
  write_size = full_size = 0u;
  pt0 = pt1 = pt2 = 0u;
}

void Fat12Emul::CmdRead (uint32_t offset, uint32_t lenght) {
  debug ("CmdRead  from %3d - > %d blocks\n", offset, lenght);
  rwofs = offset * block;
  rwlen = lenght * block;
}
void Fat12Emul::CmdWrite (uint32_t offset, uint32_t lenght) {
  debug ("CmdWrite from %3d - > %d blocks\n", offset, lenght);
  rwofs = offset * block;
  rwlen = lenght * block;
  // prepare flash for write
  int start = offset - pt2;
  int erase = lenght;
  if  (start < 0) {
    erase += start;
    start  = 0;
  }
  if (erase > 0) {
    memory.eraseBlocks (start, erase);
    write_size = erase * block;
  }
}
uint32_t Fat12Emul::GetCapacity (void) {
  return capacity;
}
void Fat12Emul::Prepare (void) {
  memset (&IO.boot, 0, block << 2);   // 4 bloky
  DirectoryEntry * const dirent  = reinterpret_cast<DirectoryEntry*>(&IO.dir);
  memcpy (dirent, DirEntries, 4 * sizeof(DirectoryEntry));
  
  uint32_t offset = BootSector.BPB.Reserved_Logical_Sectors;   // zacatek = 1. sektor
  offset += BootSector.BPB.Logical_Sectors_Per_FAT;
  if (BootSector.BPB.Number_Of_FATs > 1) {
    offset += BootSector.BPB.Logical_Sectors_Per_FAT;
  }
  offset += (BootSector.BPB.Maximum_Directory_Entries * sizeof (union DirectoryEntry)) >> 9;
  pt0 = offset;
  uint32_t ClusterSize = BootSector.BPB.Logical_Sectors_Per_Cluster;
  pt1 = (dirent[2].f.First_Logical_Cluster - 2) * ClusterSize + pt0;
  pt2 = (dirent[3].f.First_Logical_Cluster - 2) * ClusterSize + pt0;
  full_size = ((BootSector.BPB.Total_Logical_Sectors - pt2) & ~(BootSector.BPB.Logical_Sectors_Per_Cluster-1)) * block;
  debug ("Start at %d, Index at %d, Image at %d (blocks), full_size=%dB\n", pt0, pt1, pt2, full_size);
  
  // BootSector
  memcpy (&IO.boot, &BootSector, sizeof(BootSect));
  uint16_t * p = reinterpret_cast<uint16_t*>(&IO.boot);
  p  [255] = Valid_Boot_Sector_Signature;
  // FAT
  two_fat_entry12 * entries = reinterpret_cast<two_fat_entry12*>(&IO.fat);
  unsigned n = 0;
  write_fat_entry_n (entries, n++, 0xFF8);
  write_fat_entry_n (entries, n++, 0xFFF);
  write_fat_entry_n (entries, n++, 0xFFF);      // entry for index.html
  unsigned i, max;
  const unsigned cluster_size = BootSector.BPB.Logical_Sectors_Per_Cluster * block;
#ifdef NXP_LIKE
  max = full_size / cluster_size;                        // cluster = 2 * block
#else  // NXP_LIKE
  const unsigned cluster_mask = cluster_size - 1;
  max = image_size / cluster_size;                       // cluster = 2 * block
  if (image_size & cluster_mask) max += 1;  
#endif // NXP_LIKE
  if (max) {
    for (i=0; i<max-1; i++) {
      write_fat_entry_n (entries, n, n+1);
      n++;
    }
    write_fat_entry_n (entries, n++, 0xFFF);
  }
  
  for (i=0; i<n; i++) {
    debug ("%03X-%03X ", i, read_fat_entry_n (entries, i));
  }
  debug ("\nCluster begin index=%d, boot=%d\n",
         dirent[2].f.First_Logical_Cluster, dirent[3].f.First_Logical_Cluster);
  // DIR
  dirent[2].f.File_Size = index_size + 31;       // set size index.html
#ifdef NXP_LIKE
  dirent[3].f.File_Size = full_size;
#else  // NXP_LIKE
  dirent[3].f.File_Size = image_size;
#endif // NXP_LIKE
  // index.html
  unsigned m = 0;
  for (n=0; n<index_size; n++) {
    char c = index_ctxt [n];
    if (c == '%') {
      for (i=0; i<TEA_KEY_SIZE; i++) {
        m += toHex (IO.data + m, GeneratedKeys.server[i]);
      }
    } else {
      IO.data [m++] = c;
    }
  }
}
mmcIOStates Fat12Emul::Read (uint8_t * buf, uint32_t len) {
  const uint32_t tag = rwofs >> 9;  // blok no.
  if         (tag < pt0) {          // 0 .. 3
    systRead (buf, len);
  } else if  (tag < pt1) {          // nic, ale muze byt jinak
    zeroRead (buf, len);
  } else if  (tag < (pt1+1)) {      // 4 (index.html ma jen 1 blok)
    indxRead (buf, len);
  } else if  (tag < pt2) {          // 5
    zeroRead (buf, len);
  } else {                          // >= 6 (7 * 512 = 0xE00) data image
    fileRead (buf, len);
  }
  rwofs += len;
  rwlen -= len;
  parent->DecResidue(len);
  if (!rwlen) {
    return mmcIOEND;
  }
  return mmcIOPEN;
}
void Fat12Emul::zeroRead (uint8_t * buf, uint32_t len) {
  memset (buf, 0, len);
}
void Fat12Emul::systRead (uint8_t * buf, uint32_t len) {
  uint32_t ofs = rwofs;
  memcpy (buf, IO.boot + ofs, len);
}
void Fat12Emul::indxRead (uint8_t * buf, uint32_t len) {
  uint32_t ofs = rwofs & 0x01FFu;
  memcpy (buf, IO.data + ofs, len);
}
void Fat12Emul::fileRead (uint8_t * buf, uint32_t len) {
  uint32_t ofs = rwofs - (pt2 * block);
  if (ofs < image_size) {
    // TODO: zpresnit, kopiruje i kousek dal za konec souboru (asi nevadi)
    // debug ("offset=%d, len=%d\n", ofs, len);
    memory.readBytes (buf, ofs, len);
  } else {
    memset (buf, 0, len);
  }
}

mmcIOStates Fat12Emul::Write (uint8_t * buf, uint32_t len) {
  const uint32_t tag = rwofs >> 9;  // blok no.
  if         (tag < pt0) {
    systWrite (buf, len);
  } else if  (tag < pt1) {
    zeroWrite (buf, len);
  } else if  (tag < (pt1+1)) {
    indxWrite (buf, len);
  } else if  (tag < pt2) {
    zeroWrite (buf, len);
  } else {
    fileWrite (buf, len);
  }
  rwofs += len;
  rwlen -= len;
  parent->DecResidue(len);
  if (!rwlen) {
    return mmcIOEND;
  }
  return mmcIOPEN;
}
void Fat12Emul::zeroWrite (uint8_t* buf, uint32_t len) {
  // do nothing
}
void Fat12Emul::systWrite (uint8_t* buf, uint32_t len) {
  uint32_t ofs = rwofs;
  printerr (IO.boot + ofs, buf, len, ofs);
  memcpy (IO.boot + ofs, buf, len);
}
void Fat12Emul::indxWrite (uint8_t* buf, uint32_t len) {
  uint32_t ofs = rwofs & 0x01FFu;   // zbytecne - read only
  memcpy (IO.data + ofs, buf, len);
}
void Fat12Emul::fileWrite (uint8_t* buf, uint32_t len) {
  uint32_t ofs = rwofs - (pt2 * block);
  if (ofs < write_size) {
    // TODO: zpresnit, kopiruje i kousek dal za konec souboru (asi nevadi)
    debug ("fileWrite -> offset=%d, len=%d\r", ofs, len);
    memory.writeBytes (buf, ofs, len);
  }
}
#ifdef __arm__
void Fat12Emul::Save (const char * name) {};
#else  // __arm__
void Fat12Emul::Save (const char * name) {
  DirectoryEntry  * dirent  = reinterpret_cast<DirectoryEntry*>(&IO.dir);
  uint32_t len = dirent[3].f.File_Size;
  debug ("Save FILE %s (%d bytes)\n", name, len);
  FILE * out = fopen (name, "w");
  if (!out) return;
  size_t res = fwrite (image_ctxt, 1, len, out);
  (void) res;
  fclose (out);
}
#endif // __arm__
