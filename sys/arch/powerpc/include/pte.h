/*	$NetBSD: pte.h,v 1.4.8.2 2002/12/11 06:11:41 thorpej Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_MPC6XX
#include <powerpc/mpc6xx/pte.h>
#elif defined(PPC_IBM4XX)
#include <powerpc/ibm4xx/pte.h>
#endif
