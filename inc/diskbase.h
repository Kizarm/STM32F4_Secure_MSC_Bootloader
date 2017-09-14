#ifndef DISKBASE_H
#define DISKBASE_H

class DiskBase {
  public:
    virtual bool Open  (const char * name = 0) = 0;
    virtual void Close (void) = 0;
    virtual bool Read  (char * data, unsigned block) = 0;
};

#endif // DISKBASE_H
