/*	$NetBSD: pmap.h,v 1.13 2025/11/14 15:07:41 thorpej Exp $	*/

#ifndef _LUNA68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>

/*
 * Transparent translation register values for IO space 0x40000000-0xffffffff
 *
 * map via TT0: 0x40000000-0x7fffffff
 * map via TT1: 0x80000000-0xffffffff
 *
 * On 030 both use Function Codes 4-7 (to get SUPERD and SUPERP).
 * XXX: they can probably just use SUPERD.
 */

#define	__HAVE_MACHINE_BOOTMAP

#define	LUNA68K_IO0_TT_BASE	0x40000000
#define	LUNA68K_IO0_TT_SIZE	0x40000000

#define	LUNA68K_IO1_TT_BASE	0x80000000
#define	LUNA68K_IO1_TT_SIZE	0x80000000

#define	LUNA68K_TT30_IO0	(LUNA68K_IO0_TT_BASE |			\
				 __SHIFTIN(0x3f,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RWM |	\
				__SHIFTIN(4,TT30_FCBASE) |		\
				__SHIFTIN(3,TT30_FCMASK))
#define	LUNA68K_TT30_IO1	(LUNA68K_IO1_TT_BASE |			\
				 __SHIFTIN(0x7f,TT30_LAM) |		\
				 TT30_E | TT30_CI | TT30_RWM |	\
				__SHIFTIN(4,TT30_FCBASE) |		\
				__SHIFTIN(3,TT30_FCMASK))

#define	LUNA68K_TT40_IO0	(LUNA68K_IO0_TT_BASE |			\
				 __SHIFTIN(0x3f,TTR40_LAM) |		\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)
#define	LUNA68K_TT40_IO1	(LUNA68K_IO1_TT_BASE |			\
				 __SHIFTIN(0x7f,TTR40_LAM) |		\
				 TTR40_E | TTR40_SUPER |		\
				 PTE40_CM_NC_SER)

#endif /* _LUNA68K_PMAP_H_ */
