/*	$NetBSD: pmap.h,v 1.1 2024/01/02 07:41:01 thorpej Exp $	*/

#ifndef _VIRT68K_PMAP_H_
#define	_VIRT68K_PMAP_H_

#include <m68k/pmap_motorola.h>
#include <m68k/mmu_30.h>

/*
 * Qemu places the virtual devices in the last 16MB of physical
 * address space (0xff00.0000).  We use a Transparent Translation
 * register to map these VA==PA for convenience.
 */

#define	VIRT68K_IO_BASE		0xff000000

#define	VIRT68K_TT30_IO		(VIRT68K_IO_BASE |			\
				 TT30_E | TT30_CI | TT30_RWM |		\
				 TT30_SUPERD)

#define	VIRT68K_TT40_IO		(VIRT68K_IO_BASE |			\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)

#endif /* _VIRT68K_PMAP_H_ */
