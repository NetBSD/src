/*	$NetBSD: pmap.h,v 1.32 2003/02/05 01:27:34 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_OEA)
#include <powerpc/oea/pmap.h>
#else
#ifndef _LOCORE
typedef struct pmap *pmap_t;
#endif
#endif
