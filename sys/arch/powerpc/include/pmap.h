/*	$NetBSD: pmap.h,v 1.30 2003/02/03 17:10:01 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_OEA)
#include <powerpc/oea/pmap.h>
#endif
