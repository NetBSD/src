/*	$NetBSD: pmap.h,v 1.32.18.1 2006/12/30 20:46:44 yamt Exp $	*/

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
