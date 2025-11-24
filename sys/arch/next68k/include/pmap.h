/*	$NetBSD: pmap.h,v 1.18 2025/11/24 16:58:01 thorpej Exp $	*/

#ifndef _NEXT68K_PMAP_H_
#define	_NEXT68K_PMAP_H_

#ifdef __HAVE_NEW_PMAP_68K
#include <m68k/pmap_68k.h>
#else
#include <m68k/pmap_motorola.h>
#endif /* __HAVE_NEW_PMAP_68K */

#include <m68k/mmu_40.h>

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
