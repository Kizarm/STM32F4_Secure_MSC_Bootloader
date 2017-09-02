#include "usbuser.h"
#include "debug.h"
#include "fat12emul.h"
#include "keys.h"

static Fat12Emul img;

int main (void) {
  generate_keys ();
  img.Prepare   ();
  BuildDevice   ();     // Tohle je uděláno v usbuser.
  disc.Attach   (img, false);
  usb.Start     ();     // Nutno spustit.
  print ("--- Device connected ---\n");
  
  for (;;) {
    // Tohle je fakticky zbytečné, nutné pouze pro unix emulaci.
    if (usb.MainLoop()) break;
  }
  return 0;
}
