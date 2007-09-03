/*	$NetBSD: types.h,v 1.7.16.2 2007/09/03 14:29:18 yamt Exp $	*/

#define _MIPS_PADDR_T_64BIT

#include <mips/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_TODR
#define	__HAVE_TIMECOUNTER

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
