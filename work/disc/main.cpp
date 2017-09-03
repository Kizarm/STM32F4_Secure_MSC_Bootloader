#include "usbuser.h"
#include "debug.h"
#include "storagememory.h"

static StorageMemory img;

int main (void) {
  
  BuildDevice ();	// Tohle je uděláno v usbuser.
  disc.Attach (img, false);
  //echo += vcom;
  usb.Start   ();	// Nutno spustit.
  print ("--- Device connected ---\n");
  
  for (;;) {
    // Tohle je fakticky zbytečné, nutné pouze pro unix emulaci.
    if (usb.MainLoop()) break;
  }
  return 0;
}
