/*	$NetBSD: bus_defs.h,v 1.1 2011/07/01 17:10:00 dyoung Exp $	*/

#ifndef _OFPPC_BUS_DEFS_H_
#define _OFPPC_BUS_DEFS_H_

#define PHYS_TO_BUS_MEM(t,x)	(x)
#define BUS_MEM_TO_PHYS(t,x)	(x)

#include <powerpc/ofw_bus_defs.h>
#include <powerpc/bus_defs.h>

#endif /* _OFPPC_BUS_DEFS_H_ */
