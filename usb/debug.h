#ifndef USB_DEBUG_H
#define USB_DEBUG_H

/**
@file debug.h
@brief Povoluje nebo zakazuje ladící informace.

Ve firmware jsou zakázány.
*/

#ifdef __arm__
#define dbg()
#define print(fmt,args...)
#define error(fmt,args...)
#define debug(...)
#else
#include <stdio.h>
#define dbg() printf("\t%s\n",__func__)
#define print(fmt,args...)  printf("\t%s: "fmt,__func__, ## args)
#define error(fmt,args...) fprintf(stderr,"! %s: "fmt,__func__, ## args)
#define debug printf
#endif


#endif // USB_DEBUG_H
