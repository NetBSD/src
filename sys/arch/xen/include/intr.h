/*	$NetBSD: intr.h,v 1.1.4.2 2004/08/03 10:43:11 skrll Exp $	*/


#ifndef _XEN_INTR_H_
#define	_XEN_INTR_H_

#include <x86/intr.h>

#ifndef _LOCORE

#define	IPL_DEBUG	0x1	/* domain debug interrupt */
#define	IPL_DIE		0xf	/* domain die interrupt */

extern struct intrstub xenev_stubs[];

#endif /* _LOCORE */

#endif /* _XEN_INTR_H_ */
