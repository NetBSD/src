/*	$NetBSD: pte.h,v 1.8.4.1 2006/09/09 02:42:28 rpaulo Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE) || defined (PPC_OEA64)
#include <powerpc/oea/pte.h>
#endif
