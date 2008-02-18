/*	$NetBSD: bat.h,v 1.5.86.1 2008/02/18 21:04:58 mjf Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/bat.h>
#endif
