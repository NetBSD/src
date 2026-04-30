/*	$NetBSD: pmap.h,v 1.6 2026/04/30 05:46:14 thorpej Exp $	*/

#ifndef _VIRT68K_PMAP_H_
#define	_VIRT68K_PMAP_H_

#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>

/*
 * Qemu places the virtual devices in the last 16MB of physical
 * address space (0xff00.0000).  We use a Transparent Translation
 * register to map these VA==PA for convenience.
 */

#define	VIRT68K_IO_BASE		0xff000000
#define	VIRT68K_IO_SIZE		0x01000000

#define	VIRT68K_TT30_IO		(VIRT68K_IO_BASE |			\
				 TT30_E | TT30_CI | TT30_RWM |		\
				 TT30_SUPERD)

#define	VIRT68K_TT40_IO		(VIRT68K_IO_BASE |			\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#define	SYSMAP_VA	((vaddr_t)(VIRT68K_IO_BASE-PAGE_SIZE*NPTEPG))
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#endif /* _VIRT68K_PMAP_H_ */
