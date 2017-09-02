#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "keys.h"
#include "tea.h"
#include "fwcheck.h"

//static const char *  filename = "../midi/playmidi";
static const char *  filename = "../blik/firmware";
// http://127.0.0.1:8080/5495E7EE592868E77A578C9825B2FCA2/FIRMWARE.BIN

  #define SRED   "\x1B[31;1m"
  #define SGREEN "\x1B[32;1m"
  #define SBLUE  "\x1B[34;1m"
  #define SDEFC  "\x1B[0m"

static unsigned Logging (const char * buff) {
  unsigned n, res;
  //fprintf (stdout, "Funkce:%s\n", __FUNCTION__);
  fprintf (stdout, SGREEN"%s"SDEFC, buff);
  n = sscanf (buff, "%08X", &res);
  if (!n) res = 0;
  return res;
}

/// replace std system() call - redirect stdout to user fnc.
static int RunCmd (const char * cmd, unsigned & value) {
  value = 0;
  // printf ("cmd: \"%s\"\n", cmd);
  int pipefd[2], result;
  result = pipe (pipefd);
  if (result) return result;

  pid_t cpid = fork();
  if (cpid < 0) return -1;
  if (cpid == 0) {   // child
    close (pipefd[0]);    // close reading end in the child
    dup2  (pipefd[1], 1); // send stdout to the pipe
//  dup2  (pipefd[1], 2); // send stderr to the pipe
    result = execl ("/bin/sh", "sh", "-c", cmd, (char *) 0);
    close (pipefd[1]);    // this descriptor is no longer needed
  } else {           // parent
    const unsigned max = 1024;
    char buffer [max + 4];
    close (pipefd[1]); // close the write end of the pipe in the parent
    for (;;) {
      int n = read (pipefd[0], buffer, max);
      if (n <= 0) break;
      buffer[n] = '\0';
      value = Logging (buffer);
    }
    close (pipefd[0]);
  }
  return result;
}
static void CheckStart (void) {
  const unsigned len = strlen (filename) + 64;
  char cmd [len];
  const char * ref = "arm-none-eabi-nm ../boot/boot.elf | grep _external_code";
  snprintf (cmd, len, "arm-none-eabi-nm %s.elf | grep _vect_tab_begin", filename);
  unsigned sb, sf;
  if (RunCmd (ref, sb)) {
    fprintf (stderr, "Chyba : Nelze najit symbol \"_external_code\" v bootloderu\n");
    return;
  }
  if (RunCmd (cmd, sf)) {
    fprintf (stderr, "Chyba : Nelze najit symbol \"_vect_tab_begin\" v %s.elf\n", filename);
    return;
  }
  if (sb != sf) {
    fprintf (stderr, SRED"ERROR: Firmware musi zacinat na %08X, ale zacina na %08X !!!\n"
                     " !! Upravte ld script firmware !! - napr. lze pouzit (v adresari lib/stm32f4) :\n"SDEFC, sb, sf);
    unsigned mlen = 112 * 0x400 - (sb & ~0x28000000);
    fprintf (stderr, "sed -r \'s/RAM[^K]+?K[.]+?/RAM (xrw)       : ORIGIN = 0x%08X,"
                     " LENGTH = 0x%X/\' stm32_ram.ld >user_ram.ld\n", sb, mlen);
  }
}

int main (void) {
  CheckStart ();
  const char * outname = "../boot/fil/FIRMWARE.BIN";
  const unsigned len = strlen (filename) + 16;
  char inname [len];
  
  snprintf (inname, len, "%s.bin", filename);
  unsigned filesize = 0;
  uint8_t * image = FwCheck (inname, filesize);
  if (!image) return -1;
  //encrypt_block (image, filesize, GeneratedKeys.loader);
  
  FILE * out = fopen(outname, "w");
  if (!out) return -1;
  int res = fwrite (image, filesize, 1, out);
  if (!res) fprintf (stderr, "Write Error\n");
  fclose (out);
  delete [] image;
  return 0;
}
