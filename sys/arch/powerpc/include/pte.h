/*	$NetBSD: pte.h,v 1.6 2003/02/03 17:10:02 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_OEA
#include <powerpc/oea/pte.h>
#elif defined(PPC_IBM4XX)
#include <powerpc/ibm4xx/pte.h>
#endif
