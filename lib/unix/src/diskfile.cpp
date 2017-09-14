#include "diskfile.h"

DiskFile::DiskFile() : block_size(512) {

}

DiskFile::~DiskFile() {

}
bool DiskFile::Open (const char * name) {
  input = fopen (name, "r");
  return true;
}
void DiskFile::Close (void) {
  fclose (input);
}
bool DiskFile::Read (char * data, unsigned int block) {
  size_t offset = block * block_size;
  if (fseek (input, offset, SEEK_SET) < 0) return false;
  size_t result = fread (data, 1, block_size, input);
  if (result != block_size)                return false;
  return true;
}
