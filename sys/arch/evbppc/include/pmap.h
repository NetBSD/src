/*	$NetBSD: pmap.h,v 1.2 2003/02/03 05:15:51 matt Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_MPC6XX)
#include <powerpc/mpc6xx/pmap.h>
#endif
