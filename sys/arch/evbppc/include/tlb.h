/*	$NetBSD: tlb.h,v 1.1.2.2 2002/12/11 06:29:24 thorpej Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/tlb.h>

#define	ppc4xx_tlbflags(va, pa)        (0)
#endif
