/*	$NetBSD: vmparam.h,v 1.3 2003/02/03 23:09:29 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/vmparam.h>
#elif defined(PPC_OEA)
#include <powerpc/oea/vmparam.h>
#endif
