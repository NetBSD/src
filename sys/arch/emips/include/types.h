/*	$NetBSD: types.h,v 1.2.68.1 2021/04/03 21:44:42 thorpej Exp $	*/

#include <mips/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
/* We'll use the FreeRunning counter everywhere */
#define	__HAVE_TIMECOUNTER

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
