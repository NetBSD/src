/*	$NetBSD: pmap.h,v 1.4 2025/11/14 15:07:42 thorpej Exp $	*/

#ifndef _VIRT68K_PMAP_H_
#define	_VIRT68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>

/*
 * Qemu places the virtual devices in the last 16MB of physical
 * address space (0xff00.0000).  We use a Transparent Translation
 * register to map these VA==PA for convenience.
 */

#define	__HAVE_MACHINE_BOOTMAP

#define	VIRT68K_IO_BASE		0xff000000
#define	VIRT68K_IO_SIZE		0x01000000

#define	VIRT68K_TT30_IO		(VIRT68K_IO_BASE |			\
				 TT30_E | TT30_CI | TT30_RWM |		\
				 TT30_SUPERD)

#define	VIRT68K_TT40_IO		(VIRT68K_IO_BASE |			\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)

#endif /* _VIRT68K_PMAP_H_ */
