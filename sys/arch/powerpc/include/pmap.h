/*	$NetBSD: pmap.h,v 1.31 2003/02/04 01:31:49 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_OEA)
#include <powerpc/oea/pmap.h>
#else
typedef struct pmap *pmap_t;
#endif
