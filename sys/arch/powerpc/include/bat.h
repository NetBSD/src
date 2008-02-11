/*	$NetBSD: bat.h,v 1.5.18.1 2008/02/11 14:59:28 yamt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/bat.h>
#endif
