#ifndef _FAT_STRUCTURES
#define _FAT_STRUCTURES
/** ***********************************************************************/
struct FAT32_Extended_BIOS_Parameter_Block {
  uint32_t Logical_Sectors_Per_FAT; // dlouha FAT, 32.bit
  uint16_t Drive_description;
  uint16_t Version;
  uint32_t Cluster_directory_start;
  uint16_t E0, E1, R[6];
}__attribute__((packed));

struct BIOS_Parameter_Block {
  uint16_t Bytes_Per_Logical_Sector;
  uint8_t  Logical_Sectors_Per_Cluster;
  uint16_t Reserved_Logical_Sectors;
  uint8_t  Number_Of_FATs;
  uint16_t Maximum_Directory_Entries;
  uint16_t Total_Logical_Sectors;
  uint8_t  Media_Descriptor;
  uint16_t Logical_Sectors_Per_FAT;       // 13 Bytes
  /// DOS 3.3.1
  uint16_t E0,E1;
  uint32_t E2;
  uint32_t Total_Sectors_Greater;
}__attribute__((packed));

struct BootSect {
  uint8_t   jump                  [3];
  uint8_t   OEM_Name              [8];
  struct BIOS_Parameter_Block                BPB;        // 19 -> 30
  struct FAT32_Extended_BIOS_Parameter_Block EBPB;
  uint8_t   Boot_Code             [445];
  uint8_t   Physical_Drive_Number;
  uint16_t  Boot_Sector_Signature;
}__attribute__((packed));
static const uint16_t Valid_Boot_Sector_Signature = 0xAA55;
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
  uint16_t             Ignore_in_FAT;
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
};

#endif //  _FAT_STRUCTURES
