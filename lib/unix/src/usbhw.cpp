#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <memory.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
/*

#include <asm/byteorder.h>
// Následující hlavičky můžou být problematické, lze použít ty z kernelu
#include <linux/types.h>
#include <linux/usb/gadgetfs.h>
#include <linux/usb/ch9.h>
*/
#include "usbhw.h"
#include "usbdevice.h"
#include "dscbld.h"
#include "debug.h"

extern "C" {				// Locker for Fifo
  //pthread_mutex_t GlobalFifoLocker;
  
  void LockerInit (void) {
    //pthread_mutex_init   (&GlobalFifoLocker, NULL);
  }
  void Lock  (void) {
    //pthread_mutex_lock   (&GlobalFifoLocker);
  }
  void UnLock(void) {
    //pthread_mutex_unlock (&GlobalFifoLocker);
  }
};


  UsbHw * UsbHwInstance = NULL;

static const char* gadgetDeviceNames[] = {
  "/dev/gadget/dummy_udc", "/dev/gadget/dummy_udc",   // EP0 control, pro kompatibilitu 2x
  /// Pri zmene zarizeni nutno zkontrolovat poradi - musi sedet typ. Algoritmus pro spravne prirazeni neni.
  "/dev/gadget/ep1out-bulk", "/dev/gadget/ep1in-bulk",	// 1 MSC
  "/dev/gadget/ep8out",      "/dev/gadget/ep5in-int",	// 2 CDC1 Ctrl
  "/dev/gadget/ep2out-bulk", "/dev/gadget/ep2in-bulk",	// 3 CDC1 Data
  "/dev/gadget/ep10out",     "/dev/gadget/ep10in-int",	// 4 CDC2 Ctrl
  "/dev/gadget/ep12out-bulk","/dev/gadget/ep11in-bulk",	// 5 CDC2 Data - cip jich ma o 1 mene !
  //"/dev/gadget/ep7out-bulk", "/dev/gadget/ep6in-bulk",

  "/dev/gadget/ep4out-iso", "/dev/gadget/ep3in-iso",
  "/dev/gadget/ep14out-iso","/dev/gadget/ep13in-iso",
  "/dev/gadget/ep9out-iso", "/dev/gadget/ep8in-iso",
  
  NULL
};
//extern "C" {
  static void close_fd (void *n_ptr);

  static void clearBuf (USB_DataBuffer* b) {
    b->len = 0;
    b->ofs = 0;
  }
  static void signothing (int sig, siginfo_t *info, void *ptr) {
    // if (verbose)
      fprintf (stderr, "%s %d\n", __FUNCTION__, sig);
    UsbHwInstance->loop = 0;
    // exit (-1);
  }
  /// Nastavení signálů pro ukončení programu, obsluhuje signothing
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
  }

  static void *ep0_thread (void * param) {
    return UsbHwInstance->wep0_thread (param);
  }
  static void *inp_thread_handler (void *param) {
    return UsbHwInstance->winp_thread_handler (param);
  };
  static void *out_thread_handler (void *param) {
    return UsbHwInstance->wout_thread_handler (param);
  };
//};

UsbHw::UsbHw (UsbDevice * p) {
  UsbHwInstance = this;
  parent = p;
  
  Init ();
}

#define NEVENT  5

void * UsbHw::wep0_thread (void * param) {
  int    fd, n = * (int*) param;     // n = 0
  unsigned  i;
  pthread_t id;

  id = pthread_self ();
  for (i=0; i<MaxPhyEps; i++) {      // pro všechny EP vč. tohoto (0) nastav thread ID
    ep[i].out.thr = id;
    ep[i].in.thr  = id;
  }
  fd = ep[n].in.fd;
  
  pthread_cleanup_push (close_fd, &ep[n].in.fd);

  loop      = 1;
  connected = 0;
  /* event loop */
  while (loop) {
    int    tmp;
    struct usb_gadgetfs_event event [NEVENT];
    int    i, nevent;

    tmp = read (fd, &event, sizeof event[0]);
    // print("nacteno: %d\n", tmp);
    if (tmp < 0) {
      perror ("ep0 read after poll\n");
      break;
    }
    nevent = tmp / sizeof event [0];
    if (nevent != 1 && verbose)
      fprintf (stderr, "read %d ep0 events\n",
               nevent);

    for (i = 0; i < nevent; i++) {
      switch (event [i].type) {
      case GADGETFS_NOP:
        if (verbose)
          fprintf (stderr, "NOP\n");
        break;
      case GADGETFS_CONNECT:
        connected = 1;
        current_speed = event [i].u.speed;
        // if (verbose)
          fprintf (stderr, "CONNECT %d\n", event [i].u.speed);
        break;
      case GADGETFS_SETUP:
        connected   = 1;
        // USB_Configuration = 1; /// ???
        handle_control (&event [i].u.setup);
        break;
      case GADGETFS_DISCONNECT:
        connected = 0;
        current_speed = USB_SPEED_UNKNOWN;
        // if (verbose)
          fprintf (stderr, "DISCONNECT\n");
        stop_io ();
        break;
      case GADGETFS_SUSPEND:
        // connected = 1;
        // if (verbose)
          fprintf (stderr, "SUSPEND\n");
        break;
      default:
        fprintf (stderr, "* unhandled event %d\n", event [i].type);
      }
    }
    continue;
  }
  // if (verbose)
    fprintf (stderr, "%s done\n", __func__);
  fflush (stdout);

  pthread_cleanup_pop (1);
  
  return NULL;

}

/**
Inicializace systému. Původní funkce, tělo je však nahrazeno emulací. Jen základní nastavení proměnných.
*/
void UsbHw::Init (void) {
  LockerInit();
  unsigned n = 0;
  // počáteční nastavení dat.
  for (n=0; n<MaxPhyEps; n++) {
    ep[n].out.fd   = -1;
    ep[n].out.name = (char*)gadgetDeviceNames[2*n];
    ep[n].out.pD   = NULL;
    ep[n].out.thr  = 0;
    
    ep[n].in.fd    = -1;
    ep[n].in.name  = (char*)gadgetDeviceNames[2*n+1];
    ep[n].in.pD    = NULL;
    ep[n].in.thr   = 0;
    
    ep[n].id       = n;
    clearBuf(&ep[n].out.buf);
    clearBuf(&ep[n].in.buf);
    
    pthread_mutex_init(&ep[n].mut, NULL);
    pthread_cond_init (&ep[n].flg, NULL);
  }
  device_desc = (struct usb_device_descriptor*) UsbDeviceDescriptor;
  config      = (struct usb_config_descriptor*) UsbConfigDescriptor;
  
  verbose = 0;
}
/**
Otevře dummy_hcd, zapíše deskriptory a spustí obsluhu EP0.
@param con 1 = connect, inicializace, 0 nedělá nic.
*/
void UsbHw::Connect (uint32_t con) {
  // dbg();
  char  buf [4096], *cp = buf;
  int  fd;
  int  status;
  
  if (!con) return;
  status = autoconfig ();
  if (status < 0) {
    fprintf (stderr, "?? don't recognize %s device bulk\n", ep[0].in.name);
    return;
  }

  fd = open (ep[0].in.name, O_RDWR);
  if (fd < 0) {
    perror (ep[0].in.name);
    return;
  }

  * (uint32_t *) cp = 0; /* tag for this format */
  cp += 4;

  /* write full speed config */
  cp = build_config (cp);
  /* and device descriptor at the end */
  memcpy (cp, device_desc, sizeof (struct usb_device_descriptor));
  cp += sizeof (struct usb_device_descriptor);
  /*
  int ii;
  char* px = (char*) device_desc;
  for (ii=0; ii<sizeof (struct usb_device_descriptor); ii++) printf ("<%02X>", px[ii]);
  putchar ('\n');
  */
  int xlen = cp - buf;
  //printf ("xlen = %d\n", xlen);
  status = write (fd, buf, xlen);
  if (status < 0) {
    perror ("write dev descriptors");
    close (fd);
    return;
  } else if (status != (cp - buf)) {
#if __SIZEOF_POINTER__ == 8
    fprintf (stderr, "dev init, wrote %d expected %ld\n", status, cp - buf);
#else
    fprintf (stderr, "dev init, wrote %d expected %d\n", status, cp - buf);
#endif
    close (fd);
    return;
  }
  int n = 0;
  ep[n].in.fd  = fd;    // pro kompatibitilu do obou, stejně
  ep[n].out.fd = fd;
  print   (" dev=%s, fd=%d\n", ep[0].in.name, fd);
  fprintf (stderr, "%s ep0 configured\n", ep[n].in.name);
  fflush  (stderr);
  if (pthread_create (&ep[n].in.thr, NULL,
                      ep0_thread, (void *) &ep[n].id) != 0) {
    perror ("can't create source thread");
  }
  signals_set();    // Nastaví signály tak, aby to šlo ukončit Ctrl-C
  return;
}
int UsbHw::autoconfig (void) {
  struct stat statb;

  // dummy_hcd, full speed
  if (stat (ep[0].in.name, &statb) == 0) {
  } else {
    ep[0].in.name  = NULL;
    ep[0].out.name = NULL;
    return -ENODEV;
  }
  return 0;
}
char * UsbHw::build_config (char * cp) {
  int len = /*__cpu_to_le16*/ (config->wTotalLength.get());
  /*
  int ii;
  uint8_t* pi = (uint8_t*) config;
  for (ii=0; ii<len; ii++) printf ("<%02X>", pi[ii]);
  printf ("\nbuild_config, len=0x%X\n", len);
  */
  memcpy (cp, config, len);
  cp += len;
  return cp;
}

void UsbHw::handle_control (usb_ctrlrequest * setup) {
  unsigned  status, tmp;
  uint16_t  value, index, length;

  value  = /*__le16_to_cpu*/ (setup->wValue);
  index  = /*__le16_to_cpu*/ (setup->wIndex);
  length = /*__le16_to_cpu*/ (setup->wLength);

  // if (verbose)
    fprintf (stderr, "SETUP %02x.%02x v%04x i%04x %d\n",
             setup->bRequestType, setup->bRequest, value, index, length);
  // obsluhujeme jen SETUP, jiná data nelze z gadgetfs vyčíst.
  tmp = sizeof (struct usb_ctrlrequest);
  clearBuf (&ep[0].in.buf);
  clearBuf (&ep[0].out.buf);
  memcpy(ep[0].in.buf.buf, setup, tmp);
  ep[0].in.buf.len += tmp;
  noack = 0;                  // ack pouze v SETUP stage
  parent->Ep0Handler (USB_EVT_SETUP);
  ///EpHandlers[0].pEpn (EpHandlers[0].data, USB_EVT_SETUP);      // předhodíme to usbcore - parametr je event SETUP
  // Pokud přišla nějaká data navíc (DATA1), čtou se dost divně, ale jde to.
  if (length && (!(setup->bRequestType & 0x80))) {
    tmp = read (ep[0].in.fd, ep[0].in.buf.buf + ep[0].in.buf.ofs, length);
    ep[0].in.buf.len += length;
    noack = 1;                // jdeme do DATA1 stage, ack už ne
    parent->Ep0Handler (USB_EVT_OUT);
    ///EpHandlers[0].pEpn (EpHandlers[0].data, USB_EVT_OUT);      // předhodíme to usbcore - parametr je event OUT
  }
  if (ep[0].out.buf.len) {
    // TODO: při delších odpovědích by to asi chtělo volat něco jako USB_P_EP[0] (USB_EVT_IN)
    status = write (ep[0].out.fd, ep[0].out.buf.buf, ep[0].out.buf.len);
    if (verbose) printf("%s: %d bytes written\n", __func__, status);
    if (status == ep[0].out.buf.len) clearBuf (&ep[0].out.buf);
    else fprintf(stderr, "%s: write error\n", __func__);
  }

}
void UsbHw::stop_io (void) {
  //dbg();
  unsigned n;
  for (n=1; n<MaxPhyEps; n++) {
    if (ep[n].in.pD) {        // Pouze aktivní endpoint
      if (verbose)
        printf ("Stop thread for EP%02X\n", ep[n].in.pD->bEndpointAddress.w);
      if (!pthread_equal (ep[n].in.thr, ep[0].in.thr)) {  // ale ne EP0
        pthread_cancel (ep[n].in.thr);
        if (pthread_join (ep[n].in.thr, 0) != 0)
          perror ("can't join source thread");
        ep[n].in.thr = ep[0].in.thr;
      }
      ep[n].in.pD = NULL;     // bude dále neaktivní
    }
    if (ep[n].out.pD) {
      if (verbose)
        printf ("Stop thread for EP%02X\n", ep[n].out.pD->bEndpointAddress.w);
      if (!pthread_equal (ep[n].out.thr, ep[0].out.thr)) {
        pthread_cancel (ep[n].out.thr);
        if (pthread_join (ep[n].out.thr, 0) != 0)
          perror ("can't join sink thread");
        ep[n].out.thr = ep[0].out.thr;
      }
      ep[n].in.pD = NULL;
    }
  }

}
static void close_fd (void * fd_ptr) {
  int status;
  int fd;

  fd = * (int *) fd_ptr;
  * (int *)      fd_ptr = -1;

  printf ("\tClose_fd: %2d", fd);

  /* test the FIFO ioctls (non-ep0 code paths) */
  if (pthread_self () != UsbHwInstance->ep[0].in.thr) {
    /**/
    //status = ioctl (fd, GADGETFS_FIFO_FLUSH);
    //printf (", flush: %d", status);
    status = ioctl (fd, GADGETFS_FIFO_STATUS);
    printf (", state: %d (%s)", status, strerror(errno));
    if (status < 0) {
      // ENODEV reported after disconnect
      if (errno != ENODEV && errno != EOPNOTSUPP)
        perror ("get fifo status");
      status = ioctl (fd, GADGETFS_FIFO_FLUSH);
      printf (", flush: %d", status);
    } else {
      fprintf (stderr, "fd %d, unclaimed = %d\n", fd, status);
      if (status) {
        status = ioctl (fd, GADGETFS_FIFO_FLUSH);
        if (status < 0)
          perror ("fifo flush");
      }
    }
    /**/
  }

  printf ("\n");

  if (close (fd) < 0)
    perror ("close");

}
#define USB_DT_ENDPOINT_SIZE sizeof(usb_endpoint_descriptor)

int UsbHw::ep_config (int n, int dir, const char * label) {
  int   fd, status;
  char* name;
  physicalEndpoint        * pE;
  usb_endpoint_descriptor * pD;
  char  buf [USB_BUFSIZE];

  if (dir) pE = &ep[n].in;
  else     pE = &ep[n].out;
  name = pE->name;
  fd   = pE->fd;
  pD   = pE->pD;
  /* open and initialize with endpoint descriptor(s) */
  printf ("%s called from %s[%d]: %s=0x%02X\n",
          __func__, label, n, name, pD->bEndpointAddress.w);
  fd = open (name, O_RDWR);
  if (fd < 0) {
    status = -errno;
    fprintf (stderr, "%s open %s error %d (%s)\n",
             label, name, errno, strerror (errno));
    exit (-1);
    return status;
  }
  uint32_t * tmp = (uint32_t *) buf;
  /* one fs sets of config descriptors */
  *tmp = 1; /* tag for this format */
  memcpy (buf + 4, pD, USB_DT_ENDPOINT_SIZE);
  status = write (fd, buf, 4 + USB_DT_ENDPOINT_SIZE);
  
  if (status < 0) {
    status = -errno;
    fprintf (stderr, "Write %s config %s error %d (%s)\n",
             label, name, errno, strerror (errno));
    close (fd);
    return status;
  } else if (verbose) {
    unsigned long id;

    id = pthread_self ();
    fprintf (stderr, "%s start %ld fd %d\n", label, id, fd);
  }
  return fd;
}
void UsbHw::ConfigEP (usb_endpoint_descriptor * pEPD) {
  uint8_t Adr = pEPD->bEndpointAddress.w;
  uint8_t   n = Adr & 0x0F;
  
  print ("0x%02X\n", pEPD->bEndpointAddress.w);
  if (Adr & 0x80) {
    if (ep[n].in.pD) {    // EP already configured
      return;
    }
    ep[n].in.pD = pEPD;
    if (pthread_create (&ep[n].in.thr, NULL,
                        inp_thread_handler, (void *) &ep[n].id) != 0) {
      perror ("can't create source thread");
      return;
    }
  }
  else {
    if (ep[n].out.pD) {
      return;
    }
    ep[n].out.pD = pEPD;
    if (pthread_create (&ep[n].out.thr, NULL,
                        out_thread_handler, (void *) &ep[n].id) != 0) {
      perror ("can't create sink thread");
      return;
    }
  }
}
void * UsbHw::winp_thread_handler (void * param) {
  int  n = *(int *) param;
  int  fd, status, towrite;

  status = ep_config (n, 1, __FUNCTION__);
  if (status < 0)
    return NULL;
  fd = status;
  ep[n].in.fd = fd;

  pthread_cleanup_push (close_fd, &ep[n].in.fd);
  
  USB_DataBuffer * pbuf = & ep[n].out.buf;
  do {
    /* original LinuxThreads cancelation didn't work right
     * so test for it explicitly.
     */
    pthread_testcancel ();
    pthread_mutex_lock  (&ep[n].mut);                   // vstup do kritické sekce
    if (!pbuf->len)    // TODO: tohle by taky asi mělo chodit po 64 bytech (a de fakto chodí)
      pthread_cond_wait (&ep[n].flg, &ep[n].mut); // počkej na signál (přijde z USB_WriteEP)
    towrite = pbuf->len;                        // výstup z kritické sekce
    pthread_mutex_unlock(&ep[n].mut);

    status = write (fd, pbuf->buf, towrite);
    if (status == towrite) {
      pthread_mutex_lock  (&ep[n].mut);           // další kritická sekce
      clearBuf (pbuf);
      if (verbose > 2) printf ("%d Bytes really written\n", status);
      if (n) {
	eps[n]->Handler (USB_EVT_IN);
      } else parent->Ep0Handler (USB_EVT_IN);
      /// EpHandlers[n].pEpn(EpHandlers[n].data, USB_EVT_IN);                          // tohle emuluje obsluhu přerušení od IN
      pthread_mutex_unlock(&ep[n].mut);           // konec kritické sekce
    }
  } while (status > 0);
  if (status == 0) {
    if (verbose)
      fprintf (stderr, "done %s\n", __FUNCTION__);
  } else if (verbose > 2 || errno != ESHUTDOWN)     // normal disconnect
    perror ("write");
  fflush (stdout);
  fflush (stderr);
  
  pthread_cleanup_pop (1);

  return NULL;
}
void * UsbHw::wout_thread_handler (void * param) {
  int  n = *(int *) param;
  int  status, toread, fd;

  status = ep_config (n, 0, __FUNCTION__);
  if (status < 0)
    return NULL;
  fd = status;
  ep[n].out.fd = fd;

  physicalEndpoint * pE = &ep[n].in;
  /* synchronous reads of endless streams of data */
  pthread_cleanup_push (close_fd, &ep[n].out.fd);
  
  do {
    /* original LinuxThreads cancelation didn't work right
     * so test for it explicitly.
     */
    pthread_testcancel ();
    errno  = 0;
    toread = 64;    // Odladěno: dlouhý požadavek (USB_BUFSIZE - rx.len) to zasekne
    if (verbose > 2) printf ("%d Sink enter %d\n", n, toread);
    status = read (fd, pE->buf.buf + pE->buf.len, toread);
    if (verbose > 2) printf ("%d Sink  read %d\n", n, status);

    if (status < 0) {
      fprintf (stderr, "%s: read error status=%d, error=%d (%s)\n",
               __func__, status, errno, strerror(errno));
      break;
    }
    pthread_mutex_lock (&ep[n].mut);   // kritická sekce
    pE->buf.len += status;
    /**/
    while (pE->buf.len) {
      if (n) {
	eps[n]->Handler (USB_EVT_OUT);
      } else parent->Ep0Handler (USB_EVT_OUT);
    }
      /// EpHandlers[n].pEpn (EpHandlers[n].data, USB_EVT_OUT);             // emulace obsluhy přerušení od OUT
    /**/
    clearBuf (&pE->buf);
    pthread_mutex_unlock (&ep[n].mut); // konec kritické sekce
  } while (status > 0);
  if (status == 0) {
    if (verbose)
      fprintf (stderr, "done %s\n", __FUNCTION__);
  } else if (verbose > 2 || errno != ESHUTDOWN) /* normal disconnect */
    perror ("read");
  fflush (stdout);
  fflush (stderr);
  
  pthread_cleanup_pop (1);

  return NULL;
}
#define CONFIG_VALUE  1
void UsbHw::Configure (uint32_t cfg) {
  if (verbose) printf ("%d\n", cfg);
  switch (cfg) {
    case 0:
      stop_io ();
    case CONFIG_VALUE:    // Přeneseno do USB_ConfigEP
      break;
    default:
      /* kernel bug -- "can't happen" */
      fprintf (stderr, "? illegal config %d\n", cfg);
      break;
  }

}
void UsbHw::Cleanup (void) {
  int n = 0;
  if (connected)  stop_io ();
  pthread_cancel (ep[n].in.thr);
  if (pthread_join (ep[n].in.thr, 0) != 0)
    perror ("can't join source thread");

}
int UsbHw::MainLoop (void) {
  usleep (10000);
  if (loop) {
    return 0;
  }
  Cleanup();
  return 1;
}

uint32_t UsbEndpoint::Read (uint8_t * pData) {
  uint32_t EpNum = addr;
  return UsbHwInstance->ReadEPHW (EpNum, pData);
}
uint32_t UsbEndpoint::Write (uint8_t * pData, uint32_t cnt) {
  uint32_t EpNum = addr | 0x80;
  return UsbHwInstance->WriteEPHW (EpNum, pData, cnt);
}
void UsbEndpoint::SetStallEP (UsbEndpointDir dir) {
  uint32_t EpNum = addr;
  if (dir == EndpointIn) EpNum |= 0x80;
  UsbHwInstance->SetStallEPHW (EpNum);
}

void UsbEndpoint::EnableEP      (UsbEndpointDir dir) {
}
void UsbEndpoint::DisableEP     (UsbEndpointDir dir) {
}
void UsbEndpoint::ResetEP       (UsbEndpointDir dir) {
}
void UsbEndpoint::ClrStallEP    (UsbEndpointDir dir) {
}
void UsbEndpoint::DeactivateEP  (UsbEndpointDir dir) {
}

/** *****************************************************************************/
uint32_t UsbHw::ReadEPHW (uint32_t EPNum, uint8_t * pData) {
  uint32_t n, len = 0;
  
  if (verbose > 2) printf(" (%02X)\n", EPNum);
  if (EPNum & 0x80) fprintf(stderr, "%s: Dir err\n", __func__);
  n = EPNum & 0x0F;
  physicalEndpoint * pe = &ep[n].in;
  if (pe->buf.len > 64)  len = 64;
  else                   len = pe->buf.len;
  memcpy (pData, pe->buf.buf + pe->buf.ofs, len);
  pe->buf.len -= len;
  pe->buf.ofs += len;
  // print(" (%02X) %d\n", EPNum, len);
  return len;

}
uint32_t UsbHw::WriteEPHW (uint32_t EPNum, uint8_t * pData, uint32_t cnt) {
  uint32_t n, len = cnt;
  
  if (verbose > 2) printf ("(0x%02X, %d)\n", EPNum, cnt);
  if ((EPNum == 0x80) && (cnt == 0)) {    // potvrzení na EP0 (čtení naprázdno)
    if (noack) return 0;            // pouze pokud je potřeba
    // ack ...
    len = read (ep[0].in.fd, &len, 0);
    if (len)
      perror ("ack SET_CONFIGURATION");
    return 0;
  }
  if (!cnt) return 0;
  if ((EPNum & 0x80) == 0) fprintf(stderr, "%s: Dir err\n", __func__);
  n =  EPNum & 0x0F;
  USB_DataBuffer * pbuf = & ep[n].out.buf;
  memcpy (pbuf->buf + pbuf->ofs, pData, len);
  pbuf->len += len;
  pbuf->ofs += len;
  pthread_cond_signal (&ep[n].flg);       // probudí inp thread (zápis do EP)
  if (pbuf->ofs > USB_BUFSIZE)
    fprintf(stderr, "%s buffer owerflow :%d\n", __func__, pbuf->ofs);
  return len;

}


