/*	$NetBSD: pte.h,v 1.8.8.1 2006/08/11 15:42:40 yamt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE) || defined (PPC_OEA64)
#include <powerpc/oea/pte.h>
#endif
