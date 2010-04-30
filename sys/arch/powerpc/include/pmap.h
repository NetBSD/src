/*	$NetBSD: pmap.h,v 1.34.4.1 2010/04/30 14:39:43 uebayasi Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_BOOKE)
#include <powerpc/booke/pmap.h>
#elif defined(PPC_OEA) || defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/pmap.h>
#else
#endif
