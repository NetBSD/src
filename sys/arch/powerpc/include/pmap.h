/*	$NetBSD: pmap.h,v 1.32.32.1 2006/09/09 02:42:28 rpaulo Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_OEA) || defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/pmap.h>
#else
#ifndef _LOCORE
typedef struct pmap *pmap_t;
#endif
#endif
