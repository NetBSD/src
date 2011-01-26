/*	$NetBSD: types.h,v 1.1 2011/01/26 01:18:52 pooka Exp $	*/

#include <mips/types.h>

#define	__HAVE_DEVICE_REGISTER
#define	__HAVE_GENERIC_SOFT_INTERRUPTS
/* We'll use the FreeRunning counter everywhere */
#define __HAVE_TIMECOUNTER
/* ..and that's good enough for a 58k year TODR as well */
#define __HAVE_GENERIC_TODR

/* MIPS specific options */
#define	__HAVE_BOOTINFO_H
#define	__HAVE_MIPS_MACHDEP_CACHE_CONFIG
