/*	$NetBSD: bat.h,v 1.6 2008/02/05 18:10:46 garbled Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#if defined (PPC_OEA) || defined (PPC_OEA64_BRIDGE)
#include <powerpc/oea/bat.h>
#endif
