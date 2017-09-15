#include <string.h>
#include "minimad.h"
#include "debug.h"
#include "fat32ro.h"
#include "fatstruct.h"
/** *************************************************************************************/
static inline unsigned right_space_ignore (char * dst, char * src, const unsigned max) {
  char * end = src + max;
  unsigned n = 0;
  while (--end >= src) {        // vynech mezery zprava
    if (*end == ' ') continue;
    else             break;
  }
  char * beg = src;
  while (beg <= end) {          // a zbytek zkopiruj
    *dst++ = *beg++;
    n++;
  }
  return n;                     // vrat kolik
}
static bool ismp3file (const char * name) {
  bool result = false;
  const char * mp3 = ".MP3";
  unsigned i = 0;
  while (char c = * name++) {
    if (c == mp3[i]) {
      i++;
      if (i > 3) {
        result = true;
        break;
      }
    }
  }
  return result;
}
/** *************************************************************************************/
FAT32RO::FAT32RO () : block_size(512) {
}
bool FAT32RO::Attach (DiskBase & io, MiniMad & mad) {
  reader = & io;
  player = & mad;
  player->setParent (this);
  ok = Identify();
  return ok;
}

bool FAT32RO::Identify (void) {
  char tmp_buffer [block_size];
  if (!reader->Read (tmp_buffer, 0)) return false;
  BootSect * boot = reinterpret_cast<BootSect*>(tmp_buffer);
  debug ("Boot_Sector_Signature = 0x%04X\n", boot->Boot_Sector_Signature);
  if (boot->Boot_Sector_Signature != Valid_Boot_Sector_Signature) {
    debug ("Invalid Boot_Sector_Signature 0x%04X", boot->Boot_Sector_Signature);
    return false;
  }
  // OEM_Name parser
  char name [9];
  memcpy (name, boot->OEM_Name, 8);
  name[8] = '\0';
  debug("OEM_Name = %s, Physical_Drive_Number = %d\n", name, boot->Physical_Drive_Number);
  if (boot->BPB.Bytes_Per_Logical_Sector != block_size) {
    error ("FAT32 musi mit BPB.Bytes_Per_Logical_Sector roven 512\n");
    return false;
  }
  if (boot->BPB.Logical_Sectors_Per_FAT) {
    error ("FAT32 musi mit BPB.Logical_Sectors_Per_FAT nulovy\n");
    return false;
  }
  debug ("Bytes_Per_Logical_Sector    = %d\n", block_size);
  debug ("Logical_Sectors_Per_Cluster = %d\n", boot->BPB.Logical_Sectors_Per_Cluster);
  debug ("Reserved_Logical_Sectors    = %d\n", boot->BPB.Reserved_Logical_Sectors);
  debug ("Number_Of_FATs              = %d\n", boot->BPB.Number_Of_FATs);
  if (!boot->BPB.Maximum_Directory_Entries) {
    debug ("Dir Entry  start  cluster   = %d\n", boot->EBPB.Cluster_directory_start);
    RootDirCluster = boot->EBPB.Cluster_directory_start;
  } else {
    debug ("Maximum_Directory_Entries   = %d\n", boot->BPB.Maximum_Directory_Entries);
  }
  if (!boot->BPB.Total_Logical_Sectors) {
    debug ("Total_Logical_Sectors (32)  = %d\n", boot->BPB.Total_Sectors_Greater);
  } else {
    debug ("Total_Logical_Sectors       = %d\n", boot->BPB.Total_Logical_Sectors);
  }
  debug ("Media_Descriptor            = 0x%02X\n", boot->BPB.Media_Descriptor);
  Logical_Sectors_Per_FAT = boot->EBPB.Logical_Sectors_Per_FAT;
  debug ("Logical_Sectors_Per_FAT(32) = 0x%lX\n", Logical_Sectors_Per_FAT);
  // debug ("E0 = 0x%04X, E1 = 0x%04X, E2 = 0x%04X\n", boot->BPB.E0, boot->BPB.E1, boot->BPB.E2);
  
  size_t offset = boot->BPB.Reserved_Logical_Sectors;   // zacatek = 1. sektor, FAT
  FatTable1 = offset;
  offset += Logical_Sectors_Per_FAT;
  if (boot->BPB.Number_Of_FATs > 1) {
    // posun o delku 1. FAT
    FatTable2 = offset;
    offset += Logical_Sectors_Per_FAT;
  } else {
    FatTable2 = FatTable1;
  }
  ClusterSize = boot->BPB.Logical_Sectors_Per_Cluster;
  // zde offset ukazuje na adresar
  ReferenceClusterZero = offset;
  debug ("ReferenceClusterZero (+ 2 unused entry !) at 0x%lX. block, ClusterSize=%d\n",
          ReferenceClusterZero, ClusterSize);
  debug ("FAT TABLE 1 = 0x%X, FAT TABLE 2 = 0x%X block, RootDirCluster=%d\n\n",
          FatTable1, FatTable2, RootDirCluster);
  return true;
}
bool FAT32RO::ReadRootDir (void) {
  return ReadDir (RootDirCluster);
}
bool FAT32RO::ReadDir (const unsigned int cluster) {
  char tmp_buffer [block_size];
  unsigned start_addr = (cluster - 2) * ClusterSize + ReferenceClusterZero;
  bool flag = false;
  for (unsigned m=0; m<ClusterSize; m++) {  // predpoklad : directory je v jedinem clusteru
    if (!reader->Read (tmp_buffer, start_addr + m)) return false;
    DirectoryEntry * de = reinterpret_cast<DirectoryEntry*>(tmp_buffer);
    for (unsigned n=0; n<16; n++) {
      if (!ReadEntry (de + n))  {
        flag = true; break;
      }
    }
    if (flag) break;
  }
  return true;
}
bool FAT32RO::ReadEntry (DirectoryEntry * de) {
  const unsigned oslen = 16;
  char  osname  [oslen];
  unsigned  m = 0, dp = 0, flc = 0;
  if (de->f.Attributes.Attributes != 0x0F) {
    if ((unsigned char) de->f.Filename[0] == 0xE5u) return true;    // smazany soubor
    flc = de->f.First_Logical_Cluster;
    flc += (uint32_t) de->f.Ignore_in_FAT << 16;
    m = 0;
    memset (osname, 0, oslen);
    m += right_space_ignore (osname + m, de->f.Filename,  8);
    const unsigned dotp = m++;
    dp = right_space_ignore (osname + m, de->f.Extension, 3);
    if (dp) {
      m += dp;
      osname[dotp] = '.';
    } else --m;
    osname[m] = '\0';
    m = strlen(osname);
    if (!m) return false;
    debug ("\"%s\"%*s %6XH, size = %d\n", osname, 12-m, " ", flc, de->f.File_Size);
    if (ismp3file(osname)) OpenFile (flc, de->f.File_Size);
  }
  if (de->f.Attributes.Subdirectory) {        // rekurze v adresarich
    if (!strcmp (osname, "." ))    return true;
    if (!strcmp (osname, ".."))    return true;
    ReadDir (flc);  // rekurze
  }
  return true;
}
unsigned int FAT32RO::ClusterToBlock (const unsigned int cluster) {
  return (cluster - 2) * ClusterSize + ReferenceClusterZero;
}
unsigned int FAT32RO::BlockToCluster (const unsigned int block) {
  return ((block - ReferenceClusterZero) / ClusterSize) + 2;
}
bool FAT32RO::OpenFile (const unsigned int start, const unsigned int len) {
  debug ("-> Open, start=%X, len=%d\n", start, len);
  current_cluster = start;
  current_block   = ClusterToBlock (start);
  pass_block      = 0;
  Segment = 0;
  remain  = len;
  player->decode (len);
  return true;
}
unsigned int FAT32RO::NextCluster (const unsigned int cluster) {
  const unsigned clusters_in_block = block_size >> 2;
  unsigned segment = cluster / clusters_in_block + FatTable1;
  if (segment != Segment) {
    if (!reader->Read (block_buffer, segment)) return 0xFFFFFFF8u;
    Segment = segment;
  }
  unsigned index = cluster % clusters_in_block;
  uint32_t * data = reinterpret_cast<uint32_t*>(block_buffer);
  uint32_t result = data [index];
  /* clustery jdou vetsinou postupne
  if (cluster+1 != result) {
    printf ("%X:%X\n", cluster, result);
  }
  */
  return result;
}
// block = od pocatku souboru v blocich
void FAT32RO::ReadBlock (const unsigned int block) {
  unsigned ofs_block = block + current_block;
  unsigned cluster = BlockToCluster (ofs_block);
  
  if (pass_block >= ofs_block) {
    if (cluster != current_cluster) {
      current_cluster = NextCluster (current_cluster);
      if (current_cluster >= 0x0FFFFFFFu) return;
      ofs_block = ClusterToBlock (current_cluster);
      current_block = ofs_block - block;
    }
  }
  if (pass_block != ofs_block) {
    reader->Read (mdata_buffer, ofs_block);
    pass_block = ofs_block;
  }
}
void FAT32RO::get_mpg (char * data, unsigned int ofs, unsigned int len) {
  unsigned ofs_block = ofs / block_size;
  unsigned ofs_bytes = ofs % block_size;
  /*
  printf ("ofs=%d, len=%d <%d>\r", ofs, len, ofs_block);
  fflush (stdout);
  */
  ReadBlock (ofs_block);
  ofs_block += 1;
  unsigned tocpy = block_size - ofs_bytes;
  memcpy (data, mdata_buffer  + ofs_bytes, tocpy);
  len -= tocpy;
  if (!len) return;
  ReadBlock (ofs_block);
  memcpy (data + tocpy, mdata_buffer, len);
}
/*
bool FAT32RO::ReadCluster (const unsigned int cluster) {
  char buf [block_size];
  unsigned start_addr = ClusterToBlock (cluster);
  unsigned chunk = block_size;
  for (unsigned n=0; n<ClusterSize; n++) {
    if (!reader->Read (buf, start_addr + n)) return false;
    if (chunk > remain) chunk = remain;
    //fwrite (buf, 1, chunk, out);
    remain -= chunk;
  }
  return true;
}
bool FAT32RO::ReadFile (const char * name, const unsigned int start, const unsigned int len) {
  debug ("-> Save [%s], start=%X, len=%d\n", name, start, len);
  unsigned cluster = start;
  Segment = 0;
  remain  = len;
  //out = fopen (name, "w");
  while (cluster < 0x0FFFFFFFu) {
    if (!ReadCluster(cluster)) return false;
    cluster = NextCluster (cluster);
  }
  //fclose (out);
  printf ("remain = %d\n", remain);
  return true;
}
*/
