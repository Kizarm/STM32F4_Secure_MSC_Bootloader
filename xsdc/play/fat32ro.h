#ifndef FAT32RO_H
#define FAT32RO_H
#include "diskbase.h"
#include <stdio.h>
#include <stdint.h>

union  DirectoryEntry;
class  MiniMad;

class FAT32RO {
  public:
    FAT32RO ();
    bool Attach (DiskBase & io, MiniMad & mad);
    bool ReadRootDir (void);
    void get_mpg (char * data, unsigned ofs, unsigned len);
  protected:
    bool     Identify    (void);
    bool     ReadDir     (const unsigned cluster);
    unsigned NextCluster (const unsigned cluster);
    bool     ReadEntry   (DirectoryEntry * de);
    bool     OpenFile    (const unsigned start, const unsigned len);
    void     ReadBlock      (const unsigned block);
    unsigned ClusterToBlock (const unsigned cluster);
    unsigned BlockToCluster (const unsigned block);
    /* Test only
    bool     ReadCluster (const unsigned cluster);
    bool     ReadFile    (const char * name, const unsigned start, const unsigned len);
    */
  private:
    bool ok;
    char block_buffer [512];
    char mdata_buffer [512];
    const unsigned block_size;
    DiskBase     * reader;
    MiniMad      * player;
    
    unsigned       Segment;
    unsigned long  Logical_Sectors_Per_FAT;
    /// vse v blocich 512 B
    unsigned       RootDirCluster;
    unsigned       ClusterSize;
    unsigned long  ReferenceClusterZero;
    uint32_t       FatTable1, FatTable2;
    
    unsigned remain;
    unsigned current_block;
    unsigned current_cluster;
    unsigned pass_block;
    /* Test only
    FILE   * out;
    */
};

#endif // FAT32RO_H
