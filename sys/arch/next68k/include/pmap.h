/*	$NetBSD: pmap.h,v 1.16 2023/12/27 19:47:00 thorpej Exp $	*/

#ifndef _NEXT68K_PMAP_H_
#define	_NEXT68K_PMAP_H_

#include <m68k/pmap_motorola.h>
#include <m68k/mmu_30.h>

/*
 * Transparent translation register values for IO space and the
 * kernel text/data.  These are only used temporarily during
 * early boot.
 *
 * XXX BOTH?  Really?  But that matches the historical value.  But
 * just SUPER should be sufficient.
 */
#define	NEXT68K_TT40_IO		(0x02000000 |				\
				 TTR40_E | TTR40_BOTH |			\
				 PTE40_CM_NC_SER)

#define	NEXT68K_TT40_KERN	(0x04000000 |				\
				 __SHIFTIN(0x03,TTR40_LAM) |		\
				TTR40_E | TTR40_BOTH)

#endif /* _NEXT68K_PMAP_H_ */
