/*	$NetBSD: bus.h,v 1.2 2003/03/04 07:50:59 matt Exp $	*/

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
#ifdef ev64260
#define	PHYS_TO_BUS_MEM(t, addr)	(addr)	/* XXX */
#define	BUS_MEM_TO_PHYS(t, addr)	(addr)	/* XXX */

void ev64260_bus_space_init(void);
void ev64260_bus_space_mallocok(void);
#endif

#include <powerpc/bus.h>
#endif
