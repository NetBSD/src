/*	$NetBSD: sh3.h,v 1.1 1999/09/13 10:30:59 itojun Exp $	*/

/* following definitions are depend on our MMTA arch */
/* MMTA memory map defienitions */
#define IOM_RAM_BEGIN 0x8c000000
#if 1
#define IOM_RAM_END   0x8cffffff
#else
#define IOM_RAM_END   0x8c3fffff
#endif
#define IOM_ROM_BEGIN 0x80000000
#define IOM_ROM_END   0x801fffff
