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

#include "contextserver.h"
#include "keys.h"
#include "tea.h"
#include "fwcheck.h"

struct UUID {
  uint32_t part [TEA_KEY_SIZE];
};
/// Obraz firmware
  static const char * image_file = "../midi/playmidi.bin";
//static const char * image_file = "../blik/firmware.bin";
/// Zaplaceno ...
static const UUID payed [] = {  // UUID z STM32F4 (3*4 byte) + Flash size (4 byte)
  {0x001e0027, 0x33314718, 0x35373532, 0x00000400},
};
/** ********************************************************************************/
static UUID cuid;
static const unsigned BYTES = 0x400;
static const char * page_200 = "HTTP/1.0 200 OK\n\n";
static const char * page_404 = "HTTP/1.0 404 Not Found\n";

ContextServer::ContextServer (const char * r) : root(r) {

}

ContextServer::~ContextServer() {

}
bool ContextServer::path (int sockfd, char * rq) {
  const unsigned maxmsg = 100000;
  char path [maxmsg];
  
  if (sendImage (sockfd, rq)) return true;
  strcpy (path, root);
  strcpy (path + strlen (root), rq);
  printf ("file: %s\n", path);

  if (!sendFile (sockfd, path)) {
    notFound (sockfd);
  }
  return true;
}
bool ContextServer::sendFile (int sockfd, const char * path) {
  int  txmit, fd, bytes_read;
  char data_to_send [BYTES];
  if ((fd = open (path, O_RDONLY)) != -1) {                    // FILE FOUND
    send (sockfd, page_200, strlen(page_200), 0);
    while ((bytes_read = read (fd, data_to_send, BYTES)) > 0) {
      txmit = write (sockfd, data_to_send, bytes_read);
      if (!txmit) fprintf (stderr, "No send data: %d\n", txmit);
    }
    close(fd);
    return true;
  } else {
    return false;
  }
}
void ContextServer::notFound (int sockfd) {
  int  txmit, fd, bytes_read;
  char data_to_send [BYTES];
  txmit =  write (sockfd, page_404, strlen(page_404));         // FILE NOT FOUND
  
  if (!txmit) fprintf (stderr, "No send data: %d\n", txmit);
  if ((fd = open ("not_found.html", O_RDONLY)) != -1) {
    while ((bytes_read = read (fd, data_to_send, BYTES)) > 0) {
      txmit = write (sockfd, data_to_send, bytes_read);
      if (!txmit) fprintf (stderr, "No send data: %d\n", txmit);
    }
    close(fd);
  }
}

bool ContextServer::sendImage (int sockfd, const char * name) {
  int res = sscanf (name, "/%08X%08X%08X%08X/FIRMWARE.BIN", cuid.part + 0, cuid.part + 1, cuid.part + 2, cuid.part + 3);
  if (res != TEA_KEY_SIZE) return false;
  UUID uuid;
  for (unsigned n=0; n<TEA_KEY_SIZE; n++) uuid.part[n] = cuid.part[n] ^ SecretKeys.server[n];
  printf ("UUID: .part = { ");
  for (unsigned n=0; n<TEA_KEY_SIZE; n++) printf ("0x%08x, ", uuid.part[n]);
  printf ("}\n");
  bool ok;
  unsigned i, n, m = sizeof(payed) / sizeof(UUID);
  for (i=0; i<m; i++) {
    ok = true;
    for (n=0; n<TEA_KEY_SIZE; n++) {
      if (uuid.part[n] != payed[i].part[n]) {
        ok = false; break;
      }
    }
    if (ok) break;
  }
  if (!ok) {
    sendFile(sockfd, "not_payed.html");
    return true;
  }
  for (n=0; n<TEA_KEY_SIZE; n++) GeneratedKeys.loader[n] = uuid.part[n] ^ SecretKeys.loader[n];
  
  unsigned filesize = 0;
  uint8_t * image = FwCheck (image_file, filesize);
  if (!image) return false;
  
  encrypt_block (image, filesize, GeneratedKeys.loader);
  
  unsigned xlen = 1024;
  char xbuf [xlen];
  int  xres = snprintf (xbuf, xlen, "HTTP/1.0 200 OK\nContent-Type: application/binary\nContent-Length: %d\n\r\n", filesize);
  send (sockfd, xbuf, xres, 0);
  
  unsigned chunk = BYTES, start = 0, lenght = filesize;
  while (lenght) {
    if (chunk > lenght) chunk = lenght;
    res = write (sockfd, image + start, chunk);
    if (!res) {
      fprintf (stderr, "No send data: %d\n", res);
      break;
    }
    start  += chunk;
    lenght -= chunk;
  }
  printf ("Request = [%s] -> %d, i=%d, ok=%d\n", name, filesize, i, ok);
  delete [] image;
  return ok;
}
