#include "usbuser.h"
#include "debug.h"
#include "storagemmc.h"

static StorageMmc mmc;
/**
 * Jen STM32F4 Discovery.
 * Unix nemá SPI i když by v zásadě šlo emulovat přes paralelní port PC.
 * Asi bych to někde i našel, ale nechce se mi to sem dávat, když už to
 * chodí, je to celkem zbytečné. Pokud má karta nečitelné bloky, zřejmě
 * to moc chodit nebude, ošetření chyb je jen základní.
 * */

int main (void) {
  BuildDevice ();	// Tohle je uděláno v usbuser.
  disc.Attach (mmc);	// Přiřadíme úložný prostor - MMC nebo SDHC kartu přes SPI
  usb.Start   ();	// Nutno spustit.
  print ("--- Device connected ---\n");
  
  for (;;) {
    // Tohle je fakticky zbytečné, nutné pouze pro unix emulaci.
    if (usb.MainLoop()) break;
  }
  return 0;
}
