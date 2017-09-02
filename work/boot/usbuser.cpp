#include "usbuser.h"

/// Definice. Global USB classes.
  UsbDevice usb;
  MsClass   disc;
  
/// Následně použito v aplikaci
void BuildDevice (void) {
  usb.addClass (&disc);		/// Přidáme všechny potřebné třídy.
  usb.buildDev ();		    /// A nakonec postavíme strom zařízení.
}
/** ************************************/
#ifdef DSCBLD
#include <stdio.h>
/// main pro vytvoření deskriptorů
int main (void) {
  BuildDevice ();	/// Stejně jako v aplikaci.
  /// Zde se definují ASCII řetězce pro String Descriptory (musí být správné pořadí)
  usb.setStrings ("Mrazik labs.","MSC Secure BootLoader","00ABCD");
  disc.setString ("USB Mass Storage");	/// I každá třída má popis
  usb.blddsc  ();	/// Vytvoří soubor dscbld.c obsahující všechny konstantní Descriptory
  printf ("GCC %d.%d\n", __GNUC__, __GNUC_MINOR__);
  return 0;
}

#endif // DSCBLD

