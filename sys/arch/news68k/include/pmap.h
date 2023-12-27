/*	$NetBSD: pmap.h,v 1.15 2023/12/27 19:26:29 thorpej Exp $	*/

#ifndef _NEWS68K_PMAP_H_
#define	_NEWS68K_PMAP_H_

#include <m68k/pmap_motorola.h>
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
#define	NEWS68K_TT_IO		(0xe0000000 |				\
				 __SHIFTIN(0x1f,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RWM |		\
				 __SHIFTIN(4,TT30_FCBASE) |		\
				 __SHIFTIN(3,TT30_FCMASK))

#define	NEWS68K_TT_PROM		(0xc0000000 |				\
				 __SHIFTIN(0x1f,TT30_LAM) |		\
				 TT30_E | TT30_RWM |			\
				 __SHIFTIN(4,TT30_FCBASE) |		\
				 __SHIFTIN(3,TT30_FCMASK))

#endif /* _NEWS68K_PMAP_H_ */
