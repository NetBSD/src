/*	$NetBSD: vmparam.h,v 1.14 2011/06/20 08:01:14 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined(PPC_BOOKE)
#include <powerpc/booke/vmparam.h>
#elif defined(PPC_IBM4XX)
#include <powerpc/ibm4xx/vmparam.h>
#elif defined(PPC_OEA) || defined (PPC_OEA64) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/vmparam.h>
#else
#error unknown PPC variant
#endif
