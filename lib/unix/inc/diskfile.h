#ifndef DISKFILE_H
#define DISKFILE_H
#include "diskbase.h"
#include <stdio.h>

class DiskFile : public DiskBase {
  public:
    DiskFile();
    ~DiskFile();
    bool Open  (const char * name = 0);
    void Close (void);
    bool Read  (char * data, unsigned block);
  private:
    const unsigned block_size;
    FILE * input;
};

#endif // DISKFILE_H
