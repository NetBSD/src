/*	$NetBSD: bus.h,v 1.1 2002/12/09 12:15:58 scw Exp $	*/

/*
 * This is a total hack to workaround the fact that we have
 * a needless proliferation of subtly incompatible bus_space(9)
 * implementations.
 */

#ifdef _KERNEL_OPT
#include "opt_ppcarch.h"
#endif

#ifdef PPC_IBM4XX
#include <powerpc/ibm4xx/bus.h>
#else
#include <powerpc/bus.h>
#endif
