/* $Id: malloc.c,v 1.5 2004/04/29 15:41:47 pefo Exp $ */

/*
 * Zde použitý algoritmus podle http://mirror.fsf.org/pmon2000/pmon/src/lib/libc/malloc.c
 * není příliš otestován. A jako všechny ostatní algoritmy pro správu haldy není příliš
 * průhledný. Je však relativně jednoduchý. Pro CM0 musí být začátek _HEAP_START zarovnán na
 * celé slovo (align 4), jinak to nechodí. To není zcela samozřejmé a na jiných architekturách
 * to chodí (bohužel) i nezarovnané. Zde fixováno vytvořením sekce .heap, ALIGN 4.
 *
 * Ten algoritmus chodí, nevrací paměť od začátku, ale cik-cak !!!
 * */


#define HEAPLEN 0x400

#ifdef __thumb__
#define HEAPSECT __attribute__((section(".heap")))
#define printf(...)
#else  // ladeni na PC
#define HEAPSECT
#include <stdio.h>
#endif //__thumb__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

typedef unsigned long ALIGN;

union header {
  struct {
    union header   *ptr;
    unsigned long   size;
  } s;
  ALIGN             x;
};

typedef union header HEADER;

static HEADER  base = {{0}};
static HEADER  *allocp = (HEADER*) 0;

#define NULL   ((void *)0)
#define NALLOC 16


static HEADER *morecore (unsigned int);

/* static inline */void *malloc (unsigned long nbytes) {
  HEADER *p, *q;                // K&R called q, prevp
  unsigned nunits;

  nunits = (nbytes + sizeof (HEADER) - 1) / sizeof (HEADER) + 1;

  if ( (q = allocp) == NULL) {  // no free list yet
    base.s.ptr = allocp = q = &base;
    base.s.size = 0;
  }
  for (p = q->s.ptr;; q = p, p = p->s.ptr) {
    if (p->s.size >= nunits) {  // big enough
      if (p->s.size == nunits)  // exactly
        q->s.ptr = p->s.ptr;
      else {                    // allocate tail end
        p->s.size -= nunits;
        p += p->s.size;
        p->s.size  = nunits;
      }
      allocp = q;
      return ( (char *) (p + 1));
    }
    if (p == allocp)
      if ((p = morecore (nunits)) == NULL)  return NULL;
  }
}

//int allocsize (void *ap);

/* static inline */void free (void *ap) {
  HEADER *p, *q;

  p = (HEADER *) ap - 1;
  for (q = allocp; !(p > q && p < q->s.ptr); q = q->s.ptr)
    if (q >= q->s.ptr && (p > q || p < q->s.ptr)) break;

  if (p + p->s.size == q->s.ptr) {
    p->s.size += q->s.ptr->s.size;
    p->s.ptr = q->s.ptr->s.ptr;
  } else
    p->s.ptr = q->s.ptr;
  if (q + q->s.size == p) {
    q->s.size += p->s.size;
    q->s.ptr = p->s.ptr;
  } else
    q->s.ptr = p;
  allocp = q;
}
#if 0 // STATIC_HEAP
static char  HEAPSECT  _HEAP_START[HEAPLEN];
static const char*     _HEAP_MAX = _HEAP_START + HEAPLEN;

const char* gHeapPtr = _HEAP_START;

static const char *xsbrk (unsigned int size) {
  static const char * heap_ptr;
  const char *        old_heap_ptr;
  static unsigned int init_sbrk = 0;
  /// heap_ptr is initialized to HEAP_START
  if (init_sbrk == 0) {
    heap_ptr = _HEAP_START;
    init_sbrk = 1;
  }
  old_heap_ptr = heap_ptr;
  if ( (heap_ptr + size) > _HEAP_MAX) {
    return (char *) 0;
  }
  heap_ptr += size;
  return old_heap_ptr;
}
#else // HEAP je definovan za .bss, stack jde proti heap dokud se nepotkaji
extern   char   _end;		// Defined by the linker.
//register char *  stack_ptr asm ("sp");	// takhle je to v newlib
static inline char * stack_ptr (void) {		// pochopitelneji
  register char * res;
  asm volatile (
    "mov %0,sp\r\n"		// sp do vysledku
    :"=r"(res) :: );		// standardni zapis
  return res;
}

static const char * xsbrk (int incr) {
  static char * heap_end;
  char *        prev_heap_end;

  if (heap_end == NULL) heap_end = & _end;
  
  prev_heap_end = heap_end;
  
  if (heap_end + incr > stack_ptr())  {
/*
      extern void abort (void);
      _write (1, "_sbrk: Heap and stack collision\n", 32);
      abort (); // or
      asm volatile ("bkpt 0");
*/
    return (char *) 0;
  }
  
  heap_end += incr;

  return (const char *) prev_heap_end;
}
#endif // 0
static HEADER  *morecore (unsigned int nu) {
  const char   *cp;
  HEADER       *up;
  int           rnu;

  rnu = NALLOC * ( (nu + NALLOC - 1) / NALLOC);
  cp  = xsbrk (rnu * sizeof (HEADER));
  if (cp == NULL) return (HEADER*) 0;
  up = (HEADER *) cp;
  up->s.ptr  = (HEADER*) 0;
  up->s.size = rnu;
  free (up + 1);
  return (allocp);
}
/*
int allocsize (void *ap) {
  HEADER *p;

  p = (HEADER *) ap - 1;
  return (p->s.size * sizeof (HEADER));
}
*/
#ifdef __cplusplus
}
#endif // __cplusplus

void* operator new (unsigned int size) {
  return malloc(size);
}
void operator delete (void* p) {
  free (p);
}

void* operator new[] (unsigned int size) {
  return malloc(size);
}
void operator delete[] (void* p) {
  free (p);
}

