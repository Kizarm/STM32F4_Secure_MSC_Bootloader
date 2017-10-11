#include "fat32ro.h"
#include "diskfile.h"
#include "minimad.h"

static GpioClass ledg (GpioPortD, 12);
static AudioDma  dac;
       Interface ifc;
static DiskFile  df;
static FAT32RO   fat;
static MiniMad   mad;
/**
 * Nechtěl jsem používat plnou implemenaci FAT od elm-chan, takže je potřeba počítat s tím, že
 * FAT je hodně ořezaná a pouze pro čtení, je nutné kartu předem přesně "naformátovat" (což lze)
 * # mkfs.vfat -v -I -F 32 -S 512 -s 8 -n LABEL /dev/sdx
 * !!! bez čísla jednotky na konci !!!
 * t.j. FAT musí být 32-bitová, blok má přesnou délku 512 B, cluster je 8 bloků
 * 
 * Pro mp3 soubor to umí sáhnout do podadresáře, ale rekurze rychle spotřebuje RAM.
 * Formát mp3 je pevný 16-bit sample, mono/stereo, 44100 Hz (64KB/s), dá se do toho sáhnout
 * a parametry změnit. Úplný přehrávač dělat nebudu, chtěl jsem jen vyzkoušet, že to hraje,
 * procesor to stíhá a ta knihovna mad není právě vhodná pro spolupráci s kartou,
 * původně to bylo mapování souboru do paměti, což zde nelze. Jde to očůrat, ale dost
 * divně. Plný mp3 dekodér je moc náročný na paměť. Tady se ani nepoužívé dynamická
 * alokace - není proč. Stejně ja tam 40KB různých buferů, nepočítaje ty na zásobníku.
 * */

int main (void) {
  ifc += dac;
  +ledg;
  if (!df.Open("./tmp/disk.img")) return 1;
  fat.Attach (df, mad);
  fat.ReadRootDir();    // <- player zde
  df.Close();
  -ledg;
  dac.Turn (false);
  
  PlatformExit ();
  return 0;
}

