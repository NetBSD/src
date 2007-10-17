/*	$NetBSD: types.h,v 1.14 2007/10/17 19:57:05 garbled Exp $	*/

#define _MIPS_PADDR_T_64BIT

#include <mips/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_TODR
#define	__HAVE_TIMECOUNTER

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
