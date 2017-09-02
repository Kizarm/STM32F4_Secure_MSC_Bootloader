#include <stdio.h>
#include <string.h>
#include "msclass.h"
#include "fat12emul.h"

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
//  uint8_t   Boot_Code             [479];
//  uint8_t   Physical_Drive_Number; // 0
//  uint16_t  Boot_Sector_Signature; // dame na konec
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
    .Logical_Sectors_Per_FAT     = 1
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
                  0x17, 0x4B, 0x17, 0x4B, 0x00, 0x00, 0xF5, 0x7B, 0x17, 0x4B, 0x03, 0x00, 0x90, 0x01, 0x00, 0x00 }},
  {.EntryByte = { 0x46, 0x49, 0x52, 0x4D, 0x57, 0x41, 0x52, 0x45, 0x42, 0x49, 0x4E, 0x20, 0x00, 0x64, 0xF5, 0x7B,
                  0x17, 0x4B, 0x17, 0x4B, 0x00, 0x00, 0xF5, 0x7B, 0x17, 0x4B, 0x04, 0x00, 0x00, 0x80, 0x00, 0x00 }}
};
//static const unsigned image_data_start = 7u;
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

Fat12Emul::Fat12Emul () : StorageBase(), memory(), block (BootSector.BPB.Bytes_Per_Logical_Sector) {
  rwlen = 0u;
  rwofs = 0u;
  capacity = BootSector.BPB.Total_Logical_Sectors;
  
  //Prepare();
}

void Fat12Emul::CmdRead (uint32_t offset, uint32_t lenght) {
  rwofs = offset * block;
  rwlen = lenght * block;
}
void Fat12Emul::CmdWrite (uint32_t offset, uint32_t lenght) {
  rwofs = offset * block;
  rwlen = lenght * block;
  // prepare flash for write
  int start = offset - pt2;
  int erase = lenght;
  if  (start < 0) {
    erase += start;
    start  = 0;
  }
  //printf ("CmdWrite : from=%d, len=%d sects\n", offset, lenght);
  if (erase > 0) {
    memory.eraseBlocks (start, erase);
    write_size = erase * block;
    //printf ("start = %d, erase = %d\n", start, erase);
  }
}
uint32_t Fat12Emul::GetCapacity (void) {
  return capacity;
}
void Fat12Emul::Prepare (void) {
  memset (&IO.boot, 0, block << 2);   // 4 bloky
  // BootSector
  memcpy (&IO.boot, &BootSector, sizeof(BootSect));
  uint16_t * p = reinterpret_cast<uint16_t*>(&IO.boot);
  p  [255] = Valid_Boot_Sector_Signature;
  // FAT
  two_fat_entry12 * entries = reinterpret_cast<two_fat_entry12*>(&IO.fat);
  unsigned n = 0;
  write_fat_entry_n (entries, n++, 0xFF8);
  write_fat_entry_n (entries, n++, 0xFFF);
  write_fat_entry_n (entries, n++, 0x000);
  write_fat_entry_n (entries, n++, 0xFFF);      // entry for index.html
  unsigned max, i;
  const unsigned cluster_size = BootSector.BPB.Logical_Sectors_Per_Cluster * block;
  const unsigned cluster_mask = cluster_size - 1;
  max = image_size / cluster_size;                       // cluster = 2 * block
  if (image_size & cluster_mask) max += 1;  
  if (max) {
    for (i=0; i<max-1; i++) {
      write_fat_entry_n (entries, n, n+1);
      n++;
    }
    write_fat_entry_n (entries, n++, 0xFFF);
  }
  /*
  for (i=0; i<n; i++) {
    printf ("%03X\n", read_fat_entry_n (entries, i));
  }
  */
  // DIR
  DirectoryEntry  * dirent  = reinterpret_cast<DirectoryEntry*>(&IO.dir);
  memcpy (dirent, DirEntries, 4 * sizeof(DirectoryEntry));
  //printf ("File Size %d=%ld\n", dirent[2].f.File_Size, index_size + 31);
  dirent[2].f.File_Size = index_size + 31;       // set size index.html
  dirent[3].f.File_Size = image_size;
  // index.html
//memcpy (&IO.data, file_ctxt, file_size);
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
  //printf ("Start at %d, Index at %d, Image at %d\n", pt0, pt1, pt2);
}
mmcIOStates Fat12Emul::Read (uint8_t * buf, uint32_t len) {
  const uint32_t tag = rwofs >> 9;   // blok no.
  if         (tag < pt0) {          // 0 .. 2
    systRead (buf, len);
  } else if  (tag < pt1) {          // 3 .. 4
    zeroRead (buf, len);
  } else if  (tag < (pt1+1)) {      // 5 (index.html ma jen 1 blok)
    indxRead (buf, len);
  } else if  (tag < pt2) {          // 6
    zeroRead (buf, len);
  } else {                          // >= 7 (7 * 512 = 0xE00) data image
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
    // printf ("offset=%d, len=%d\n", ofs, len);
    memory.readBytes (buf, ofs, len);
  } else {
    memset (buf, 0, len);
  }
}

mmcIOStates Fat12Emul::Write (uint8_t * buf, uint32_t len) {
  const uint32_t tag = rwofs >> 9;   // blok no.
  if         (tag < pt0) {          // 0 .. 2
    systWrite (buf, len);
  } else if  (tag < pt1) {          // 3 .. 4
    zeroWrite (buf, len);
  } else if  (tag < (pt1+1)) {      // 5 (index.html ma jen 1 blok)
    indxWrite (buf, len);
  } else if  (tag < pt2) {          // 6
    zeroWrite (buf, len);
  } else {                          // >= 7 (7 * 512 = 0xE00) data image
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
    // printf ("offset=%d, len=%d\n", ofs, len);
    memory.writeBytes (buf, ofs, len);
  }
}
