/*	$NetBSD: types.h,v 1.13 2007/07/17 04:03:23 macallan Exp $	*/

#define _MIPS_PADDR_T_64BIT

#include <mips/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_TODR
#define	__HAVE_TIMECOUNTER

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
