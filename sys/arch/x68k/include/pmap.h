/*	$NetBSD: pmap.h,v 1.28 2025/11/16 03:11:47 tsutsui Exp $	*/

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>

#define __HAVE_MACHINE_BOOTMAP
