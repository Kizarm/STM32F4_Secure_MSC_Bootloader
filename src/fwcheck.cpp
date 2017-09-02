#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<signal.h>
#include<fcntl.h>
#include "fwcheck.h"

struct UserDescriptor {
  uint32_t magic;
  uint32_t size;
  uint32_t check;
};
const uint32_t DescriptorOffset = 0x188;
const uint32_t UserMagicNumber  = 0x12345678;

static bool UserCheck (uint8_t * ptr, uint32_t size) {
  UserDescriptor * ud = reinterpret_cast<UserDescriptor*>(ptr + DescriptorOffset);
  if (ud->magic != UserMagicNumber) return false;
  if (ud->size  != size)            return false;
  uint32_t   len  = ud->size >> 2, cs = 0u;
  uint32_t * wptr = reinterpret_cast<uint32_t*>(ptr);
  for (unsigned n=0; n<len; n++) cs += wptr [n];
  if (cs)                           return false;
  printf ("User Image OK.\n");
  return true;
}


unsigned char * FwCheck (const char * image_file, unsigned & filesize) {
  struct stat info;
  int res = stat (image_file, &info);
  if (res) return 0;
  filesize = info.st_size;
  printf ("File %s has %d bytes\n", image_file, filesize);
  if (filesize & 7) {
    filesize = ((filesize >> 3) + 1) << 3;
  }
  uint8_t * image = new uint8_t [filesize];
  memset (image, 0, filesize);
  FILE * in = fopen (image_file, "r");
  res = fread  (image, info.st_size, 1, in);
  fclose (in);
  if (!res) return 0;
  UserDescriptor * ud = reinterpret_cast<UserDescriptor*>(image + DescriptorOffset);
  printf("Info: 0x%08X, cs=0x%08X, len=%d\n", ud->magic, ud->check, ud->size);
  if (ud->magic != UserMagicNumber) {
    fprintf (stderr, "Bad magic %08X\n", ud->magic);
    return 0;
  }
  ud->size  = filesize;
  ud->check = 0u;           // sichr
  uint32_t len = filesize >> 2, cs = 0u;
  uint32_t * wptr = reinterpret_cast<uint32_t*>(image);
  for (unsigned n=0; n<len; n++) cs += wptr [n];
  ud->check = 0u - cs;
  
  UserCheck (image, filesize);
  
  return image;
}
