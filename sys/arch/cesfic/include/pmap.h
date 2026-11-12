/*	$NetBSD: pmap.h,v 1.8 2026/11/12 16:09:14 thorpej Exp $	*/

#ifndef _CESFIC_PMAP_H_
#define	_CESFIC_PMAP_H_

#include <m68k/pmap_motorola.h>

#include <m68k/mmu_40.h>

/*
 * Transparent translation register values for kernel text/data,
 * only used during early boot.
 */

#define	CESFIC_TT40_KERN	(0x20000000 |				\
				 TTR40_E | TTR40_SUPER)

#endif /* _CESFIC_PMAP_H_ */
