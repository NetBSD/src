/*	$NetBSD: pmap.h,v 1.9 2026/11/13 01:23:13 thorpej Exp $	*/

#ifndef _CESFIC_PMAP_H_
#define	_CESFIC_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_40.h>

/*
 * Transparent translation register values for kernel text/data,
 * only used during early boot.
 */

#define	CESFIC_TT40_KERN	(0x20000000 |				\
				 TTR40_E | TTR40_SUPER)

#endif /* _CESFIC_PMAP_H_ */
