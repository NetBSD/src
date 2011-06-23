/*	$NetBSD: types.h,v 1.1.6.1 2011/06/23 14:19:06 cherry Exp $	*/

#include <mips/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
/* We'll use the FreeRunning counter everywhere */
#define __HAVE_TIMECOUNTER

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
