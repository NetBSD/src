/*	$NetBSD: pmap.h,v 1.17 2025/11/14 15:07:41 thorpej Exp $	*/

#ifndef _NEWS68K_PMAP_H_
#define	_NEWS68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_30.h>

/*
 * Transparent translation register values for:
 *
 * I/O space: 0xe0000000-0xffffffff
 * RAM to use PROM calls: 0xc0000000-0xdfffffff
 *
 * Both use Function Codes 4-7 (to get SUPERD and SUPERP).
 * XXX I/O space can probably just use SUPERD.
 */

#define	__HAVE_MACHINE_BOOTMAP

#define	NEWS68K_PROM_TT_BASE	0xc0000000
#define	NEWS68K_PROM_TT_SIZE	0x20000000

#define	NEWS68K_IO_TT_BASE	0xe0000000
#define	NEWS68K_IO_TT_SIZE	0x20000000

#define	NEWS68K_TT_IO		(NEWS68K_IO_TT_BASE |			\
				 __SHIFTIN(0x1f,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RWM |		\
				 __SHIFTIN(4,TT30_FCBASE) |		\
				 __SHIFTIN(3,TT30_FCMASK))

#define	NEWS68K_TT_PROM		(NEWS68K_PROM_TT_BASE |			\
				 __SHIFTIN(0x1f,TT30_LAM) |		\
				 TT30_E | TT30_RWM |			\
				 __SHIFTIN(4,TT30_FCBASE) |		\
				 __SHIFTIN(3,TT30_FCMASK))

#endif /* _NEWS68K_PMAP_H_ */
