/*	$NetBSD: bus.h,v 1.10.2.2 2007/12/03 18:38:05 ad Exp $	*/

#define PHYS_TO_BUS_MEM(t,x)	(x)
#define BUS_MEM_TO_PHYS(t,x)	(x)

#define BUS_MEM_TO_PHYS(t,x)	(x)
#include <powerpc/bus.h>

#include <powerpc/ofw_bus.h>
#include <powerpc/bus.h>
