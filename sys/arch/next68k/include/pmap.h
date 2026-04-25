/*	$NetBSD: pmap.h,v 1.20 2026/04/25 13:49:07 thorpej Exp $	*/

#ifndef _NEXT68K_PMAP_H_
#define	_NEXT68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#define	PMBM_I_INTIO		0
#define	PMBM_I_FB		1

#include <m68k/mmu_30.h>
#include <m68k/mmu_40.h>

/*
 * Transparent translation register values for IO space and the
 * kernel text/data.  These are only used temporarily during
 * early boot.
 */

/*
 * 68040 version:
 * XXX BOTH?  Really?  But that matches the historical value.  But
 * just SUPER should be sufficient.
 */
#define	NEXT68K_TT40_IO		(0x02000000 |				\
				 TTR40_E | TTR40_BOTH |			\
				 PTE40_CM_NC_SER)

#define	NEXT68K_TT40_KERN	(0x04000000 |				\
				 __SHIFTIN(0x03,TTR40_LAM) |		\
				TTR40_E | TTR40_BOTH)

/*
 * 68030 version:
 * For KERN, use Function Codes 4-7 (to get SUPERD and SUPERP).
 */
#define	NEXT68K_TT30_IO		(0x02000000 |				\
				 TT30_E | TT30_CI | TT30_RWM |		\
				 TT30_SUPERD)

#define	NEXT68K_TT30_KERN	(0x04000000 |				\
				 __SHIFTIN(0x03,TT30_LAM) |		\
				 TT30_E | TT30_RWM |			\
				 __SHIFTIN(4,TT30_FCBASE) |		\
				 __SHIFTIN(3,TT30_FCMASK))

#endif /* _NEXT68K_PMAP_H_ */
