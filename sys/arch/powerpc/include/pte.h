/*	$NetBSD: pte.h,v 1.5 2002/12/09 12:28:13 scw Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_MPC6XX
#include <powerpc/mpc6xx/pte.h>
#elif defined(PPC_IBM4XX)
#include <powerpc/ibm4xx/pte.h>
#endif
