/*	$NetBSD: pmap.h,v 1.1.2.2 2002/12/11 06:29:18 thorpej Exp $	*/

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/pmap.h>
#elif defined(PPC_MPC6XX)
#include <powerpc/mpc6xx/pmap.h>
#else
#include <powerpc/pmap.h>	/* XXXSCW: Shouldn't be using old pmap */
#endif
