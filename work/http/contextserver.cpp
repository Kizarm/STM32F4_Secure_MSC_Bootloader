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

struct imageDesc {
  const char * filename;
  const char * descript;
};
/// Obrazy firmware
static const imageDesc Images[] = {
  {"../blik/firmware.bin", "Blikání modrou LEDkou"},
  {"../midi/playmidi.bin", "Zjednodušený MIDI player (hudba z filmu Podraz)"},
  {"../text/gsm.bin",      "Pan Werich čte Haškova Švejka (GSM komprese)"},
  {0,0}
};
struct UUID {
  uint32_t part [TEA_KEY_SIZE];
};
/// Zaplaceno ...
static const UUID payed [] = {  // UUID z STM32F4 (3*4 byte) + Flash size (4 byte)
  {0x001e0027, 0x33314718, 0x35373532, 0x00000400},
  {0x001e0027, 0x33314718, 0x35373532, 0x00000000},  // Linux test, zmena 1 bitu
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
/* Takhle vyrábět a obsluhovat formulář je asi blbost, ale funguje to
 * */
bool ContextServer::sendSel (int sockfd) {
  const unsigned ctxlen = 0x1000;
  char ctxbuf [ctxlen];
  unsigned n = 0, i = 0, cnt = 0;
  n += snprintf (ctxbuf + n, ctxlen - n, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 Transitional//EN\">\n<html>\n<head>\n  <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\">\n"
                                         "<title>SECURE BOOT</title>\n  <style type=\"text/css\"></style>\n</head>\n<body lang=\"cs-CZ\" dir=\"ltr\">\n<p>Zde si můžete něco vybrat:</p>\n");
  n += snprintf (ctxbuf + n, ctxlen - n, "<FORM action=\"FIRMWARE.BIN\">\n  <SELECT name=\"volby\" size=1>\n");
  for (;;) {
    const char * filename = Images [i].filename;
    if (!filename) break;
    int res = access (filename, R_OK);
    if (!res) {
      cnt += 1;
      const char * descript = Images [i].descript;
      n += snprintf (ctxbuf + n, ctxlen - n, "    <OPTION value=%d> %s\n", i, descript);
    }
    i += 1;
  }
  n += snprintf (ctxbuf + n, ctxlen - n, "  </SELECT>\n");
  if (cnt) {
    n += snprintf (ctxbuf + n, ctxlen - n, "  <INPUT name=\"ok\" type=submit value=\"Odeslat\">\n");
    n += snprintf (ctxbuf + n, ctxlen - n, "</FORM>\n</body>\n</html>\n");
    n += snprintf (ctxbuf + n, ctxlen - n, "<p>Stažený soubor normálně uložte. Do zařízení ho lze dostat přetažením, ale <b>soubor FIRMWARE.BIN v zařízení musíte předem smazat !</b></p>");
    n += snprintf (ctxbuf + n, ctxlen - n, "<p><i>Pozn.: Smazání souboru v zařízení jen přepíše adresář, což nevadí, po RESETu se soubor opět objeví. "
                                           "Pokud ho ale přepíšete třeba tím přetažením, je to nevratné. <b>Linuxový příkaz cp soubor FIRMWARE.BIN prostě přepíše, není nutné předem nic mazat.</b></i></p>");
  }
  send (sockfd, page_200, strlen(page_200), 0);
  int txmit = write (sockfd, ctxbuf, n);
  if (!txmit) fprintf (stderr, "No send data: %d\n", txmit);

  return true;
}

bool ContextServer::path (int sockfd, char * rq) {
  const unsigned maxmsg = 10000;
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
  bool ok = true;
  const unsigned buflen = 0x400;
  char strbuf [buflen];
  int res = sscanf (name, "/%08X%08X%08X%08X/%s", cuid.part + 0, cuid.part + 1, cuid.part + 2, cuid.part + 3, strbuf);
  if (res != TEA_KEY_SIZE + 1) return false;
  printf ("REQUEST \"%s\"\n", strbuf);
  if (!strcmp ("FIRMWARE.BIN", strbuf)) {
    UUID uuid;
    for (unsigned n=0; n<TEA_KEY_SIZE; n++) uuid.part[n] = cuid.part[n] ^ SecretKeys.server[n];
    printf ("UUID: .part = { ");
    for (unsigned n=0; n<TEA_KEY_SIZE; n++) printf ("0x%08x, ", uuid.part[n]);
    printf ("}\n");
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

    ok = sendSel(sockfd);
  } else {
    unsigned value = 0;
    res = sscanf (strbuf, "FIRMWARE.BIN?volby=%d&ok=Odeslat", &value);
    if (res != 1) return false;
    const char * name = Images[value].filename;
    ok = sendFw (sockfd, name);
  }
  return ok;  
}
bool ContextServer::sendFw (int sockfd, const char * name) {
  bool ok = true; int res;
  unsigned filesize = 0;
  uint8_t * image = FwCheck (name, filesize);
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
  delete [] image;
  printf ("Request = [%s] -> %d, ok=%d\n", name, filesize, ok);
  return ok;
}
