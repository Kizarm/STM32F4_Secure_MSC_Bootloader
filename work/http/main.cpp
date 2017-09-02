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

/**
 * @mainpage MSC_Secure_Bootloader
 * 
 * NXP má pro své procesory s USB docela pěkný bootloader, který se umí chovat jako USB mass storage class,
 * přičemž emuluje FAT filesystém, takže nejsou potřeba vůbec žádné externí nástroje - ve Windows lze
 * na takto emulovaný disk prostě nakopírovat (přeháhnutím) nový firmware a je hotovo. V Linuxu toto nefunguje
 * (bude diskutováno dále), musí se používat různé obezličky. Je to však cesta po které se lze vydat dále.
 * 
 * @section secta Jak to funguje.
 * Dalším požadavkem je alespoň částečná ochrana firmware proti nelegálnímu kopírování. K tomu je použito
 * unikátní číslo (UUID), které dává výrobce natvrdo do čipu a slabé šifrování TEA. Vyžaduje to trochu čarování
 * s linker skripty a startovacím kódem, ale dá se to zprovoznit. Pro generování nového produkčního firmware
 * je použit tento jednoduchý HTTP server. Ten může běžet lokálně ale i na síti.
 * - 1. Vlastní produkční firmware musí mít několik identifikačních prvků, podle kterých bootloader pozná,
 *      že je firmware validní a může ho použít (skok do firmware - jump.s). Zde se používá
 *   + a. Magic number 0x12345678 je sice debilní, ale lze změnit.
 *   + b. délka firmware v bytech - musí být určena aby bylo možné spočítat
 *   + c. kontrolní suma - číslo, které je určeno tak, aby prostý součet všech 32-bit unsigned přes celou délku
 *        byl nulový.
 * 
 *      To je uloženo hned za vektory na pevném ofsetu od začátku, který lze vyčíst z mapy paměti bootloaderu.
 * - 2. FAT je dost jednoduše emulována - začátek obsahuje systémové informace, vlastní FAT tabulku a adresář.
 *      Protože je pro šifrování použit pro každý kus hardware jiný klíč (odvozený z UUID), je zde ještě soubor
 *      index.html (read only), který lze otevřít prohlížečem a obsahuje odkaz na zmíněný HTTP server. Tento odkaz je generován
 *      bootloderem - obsahuje (v cestě) informaci o UUID, kterou server použije pro generování klíče pro šifrování.
 *      Soubor FIRMWARE.BIN pak může být vygenerován serverem na základě této informace správně šifrovaný,
 *      je možné ho zkopírovat na disk a odtud pak do zařízení - na Linuxu např. příkazem
 * @code
 * #cp ~/Downloads/FIRMWARE.BIN /media/$USER/BOOTLOADER/
 * @endcode
 *      což korektně přepíše soubor FIRMWARE.BIN na zařízení. Na Linuxu to pouhým přetažením nejde, protože systém
 *      zapisuje do FAT v několika krocích - napřed zapíše do adresáře informace o novém souboru (včetně dlouhého jména),
 *      pak i do FAT na volné místo (má ho tam dost), data za existující soubor a nakonec (sync nebo umount) zjistí,
 *      že tento soubor má stejný název jako původní, tak to sesynchronizuje - ale data zůstanou na původním místě,
 *      což je špatně. Příkaz cp soubor prostě nahradí, včetně přepisu FAT a adresáře a tak to bylo zamýšleno.
 *      Lze zapsat i soubor s jiným jménem, ale nebude to fungovat a po resetu z adresáře zmizí.
 * - 3. Průběh přenosu na klienta je popsán v dokumentaci třídy ContextServer.
 * 
 * @section sectb Poznámky k realizaci.
 * Použití blokové šifry zde není vhodné - tabulka vektorů je pak docela průhledná, lepší by bylo použít nějakou proudovou
 * šifru, pak by i tam byl rozsypaný čaj. HTTP server je opravdu jen náznak funkce. Co se týká FAT není to tak jednoduché.
 * Co se děje uvnitř operačního systému (OS) může být různé - různé operace povedou ke stejnému výsledku, který vidí OS,
 * ale mohou mít odlišnou vnitřní reprezentaci dat. A protože to funguje tak, že systém napřed zapisuje bloky
 * dat na volné místo a pak teprve upravuje FAT a adresáře, nelze zevnitř MSC predikovat co kam přijde. OS zapisuje data
 * na lineárně stoupající adresy, takže FAT není rozházená, ale nějak přesouvat bloky v tak malé paměti (a notabene flash)
 * se mi nejevilo jako účelné. Program cp v Linuxu funguje správně, další možnost je použít dd a prostě přepsat bloky
 * dat přidělené FIRMWARE.BIN a na adresáře i FAT se prostě vykašlat. Tak nějak funguje i ten bootloader NXP.
 * Ve Windows jsem to nezkoušel (jsem líný je nastartovat), ale předpokládám, že to fungovat bude a není to podstatné.
 * Zatím je to vše jen v RAM STM32F407 (je jí docela dost), předělat to do flash není tak velký problém, pokud to bude potřeba.
 * Šlo čistě je o ověření principu, včetně generování dat HTTP serverem. I tak je to už docela složité.
 *
 * 31.09.2017 Dodělán běh ve FLASH, trochu divné, ale celkem funkční (patch -p2 <flash.patch).
 *
 * @file
 * Převzato z webu a upraveno - je to primitivní, ale funkční řešení bez závislostí.
 * Muselo by se to hodně předělat, aby to mohl být produkční server - třeba přepsat do WT
 * a doplnit databázi placených systémů.
 * */
#define CONNMAX 1000
#define BYTES 0x1000

static char * ROOT;
static int listenfd, clients[CONNMAX];
//static void error (char *);
static void startServer (char *);
static void respond (int);
static ContextServer * paths;
static volatile bool running;

static void signothing (int sig, siginfo_t * info, void * ptr) {
  fprintf (stderr, "%s %d\n", __FUNCTION__, sig);
}
// Nastavení signálů pro ukončení programu, obsluhuje signothing
static void signals_set (void) {
  struct sigaction action;
  action.sa_sigaction = signothing;
  sigfillset (&action.sa_mask);
  action.sa_flags = SA_SIGINFO;
  if (sigaction (SIGINT,  &action, NULL) < 0) {
    perror ("SIGINT");
    return;
  }
  if (sigaction (SIGQUIT, &action, NULL) < 0) {
    perror ("SIGQUIT");
    return;
  }
  running = true;
}

int main (int argc, char* argv[]) {
  struct sockaddr_in clientaddr;
  socklen_t addrlen;

  signals_set();
  //Default Values PATH = ~/ and PORT=8080
  char PORT[6];
  ROOT = getenv ("PWD");
  strcpy (PORT,"8080");
  paths = new ContextServer (ROOT);

  int slot=0;
  printf ("Server started at port no. %s%s%s with root directory as %s%s%s\n","\033[92m",PORT,
          "\033[0m","\033[92m",ROOT,"\033[0m");
  // Setting all elements to -1: signifies there is no client connected
  int i;
  for (i=0; i<CONNMAX; i++) clients[i]=-1;
  startServer (PORT);

  // ACCEPT connections
  while (running) {
    addrlen = sizeof (clientaddr);
    clients[slot] = accept (listenfd, (struct sockaddr *) &clientaddr, &addrlen);

    if (clients[slot] < 0) {
      fprintf (stderr, "accept() error ... ");
      break;
    } else {
      if (fork() == 0) {
        respond (slot);
        exit (0);
      }
    }

    while (clients[slot] != -1) slot = (slot+1) %CONNMAX;
  }

  printf ("Destruction\n");
  delete paths;
  return 0;
}

//start server
static void startServer (char * port) {
  struct addrinfo hints, * res, * p;

  // getaddrinfo for host
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo (NULL, port, &hints, &res) != 0) {
    perror ("getaddrinfo() error");
    exit (1);
  }
  // socket and bind
  for (p=res; p!=NULL; p=p->ai_next) {
    listenfd = socket (p->ai_family, p->ai_socktype, 0);
    if (listenfd == -1) continue;
    if (bind (listenfd, p->ai_addr, p->ai_addrlen) == 0) break;
  }
  if (p == NULL) {
    perror ("socket() or bind()");
    exit (1);
  }

  freeaddrinfo (res);

  // listen for incoming connections
  if (listen (listenfd, 1000000) != 0) {
    perror ("listen() error");
    exit (1);
  }
}

//client connection
static void respond (int n) {
  const unsigned maxmsg = 100000;
  char mesg[maxmsg], *reqline[3];
  int rcvd, txmit;

  memset ( (void*) mesg, (int) '\0', maxmsg);

  rcvd = recv (clients[n], mesg, maxmsg, 0);

  if (rcvd < 0)          // receive error
    fprintf (stderr, ("recv() error\n"));
  else if (rcvd == 0)    // receive socket closed
    fprintf (stderr,"Client disconnected upexpectedly.\n");
  else {                 // message received
    // printf ("From server:\"%s\"\n", mesg);
    reqline[0] = strtok (mesg, " \t\n");
    if (strncmp (reqline[0], "GET\0", 4) == 0) {
      reqline[1] = strtok (NULL, " \t");
      reqline[2] = strtok (NULL, " \t\n");
      if (strncmp (reqline[2], "HTTP/1.0", 8) != 0 && strncmp (reqline[2], "HTTP/1.1", 8) != 0) {
        txmit = write (clients[n], "HTTP/1.0 400 Bad Request\n", 25);
        if (!txmit) fprintf (stderr, "No send data: %d\n", txmit);
      } else {
        if (strncmp (reqline[1], "/\0", 2) == 0)
          reqline[1] = (char*) "/index.html";        // Because if no file is specified, index.html will be opened by default (like it happens in APACHE...)

        paths->path (clients[n], reqline[1]);
      }
    }
  }

  //Closing SOCKET
  shutdown (clients[n], SHUT_RDWR);         //All further send and recieve operations are DISABLED...
  close (clients[n]);
  clients[n]=-1;
}

