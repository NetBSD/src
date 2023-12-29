/*	$NetBSD: pmap.h,v 1.11 2023/12/29 02:30:35 tsutsui Exp $	*/

#ifndef _LUNA68K_PMAP_H_

#include <m68k/pmap_motorola.h>
#include <m68k/mmu_30.h>

/*
 * Transparent translation register values for IO space 0x40000000-0xffffffff
 *
 * map via TT0: 0x40000000-0x7fffffff
 * map via TT1: 0x80000000-0xffffffff
 *
 * On 030 both use Function Codes 4-7 (to get SUPERD and SUPERP).
 * XXX: they can probably just use SUPERD.
 */

#define	LUNA68K_TT30_IO0	(0x40000000 |				\
				 __SHIFTIN(0x3f,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RWM |	\
				__SHIFTIN(4,TT30_FCBASE) |		\
				__SHIFTIN(3,TT30_FCMASK))
#define	LUNA68K_TT30_IO1	(0x80000000 |				\
				 __SHIFTIN(0x7f,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RWM |	\
				__SHIFTIN(4,TT30_FCBASE) |		\
				__SHIFTIN(3,TT30_FCMASK))

#define	LUNA68K_TT40_IO0	(0x40000000 |				\
				 __SHIFTIN(0x3f,TTR40_LAM) |		\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)
#define	LUNA68K_TT40_IO1	(0x80000000 |				\
				 __SHIFTIN(0x7f,TTR40_LAM) |		\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)

#endif /* _LUNA68K_PMAP_H_ */
