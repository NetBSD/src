/*	$NetBSD: bus.h,v 1.4 2003/03/16 07:07:19 matt Exp $	*/

/*
 * This is a total hack to workaround the fact that we have
 * a needless proliferation of subtly incompatible bus_space(9)
 * implementations.
 */

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/bus.h>
#else
#define	PHYS_TO_BUS_MEM(t, addr)	(addr)	/* XXX */
#define	BUS_MEM_TO_PHYS(t, addr)	(addr)	/* XXX */

#include <powerpc/bus.h>
#endif
