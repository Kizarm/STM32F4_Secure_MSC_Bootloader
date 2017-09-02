// pro clang nechci pouzit knihovny - tohle je jedine pouzite.
void* memset (void* ptr, int fill, unsigned long size) {
  unsigned char* cp = (unsigned char*) ptr;
  unsigned i;
  for (i=0; i<size; i++) cp[i] = fill;
  return ptr;
}
