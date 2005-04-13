/*	$NetBSD: intr.h,v 1.1.18.1 2005/04/13 21:38:38 tron Exp $	*/


#ifndef _XEN_INTR_H_
#define	_XEN_INTR_H_

#include <x86/intr.h>

#ifndef _LOCORE

#define	IPL_DEBUG	0x1	/* domain debug interrupt */
#define	IPL_DIE		0xf	/* domain die interrupt */

extern struct intrstub xenev_stubs[];

#endif /* _LOCORE */

/* XXX intrdefs.h */
#define	SIR_XENEVT	26	/* not really a soft interrupt. */
#define	IPL_SOFTXENEVT	IPL_BIO
#if !defined(_LOCORE)
#define	splsoftxenevt()	splbio()
#endif

#endif /* _XEN_INTR_H_ */
