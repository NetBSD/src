/*	$NetBSD: pmap.h,v 1.29 2025/11/24 16:53:01 thorpej Exp $	*/

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#define __HAVE_MACHINE_BOOTMAP
