/*	$NetBSD: bus_funcs.h,v 1.1.2.1 2012/04/17 00:06:49 yamt Exp $	*/
/*	$OpenBSD: bus.h,v 1.1 1997/10/13 10:53:42 pefo Exp $	*/

#ifndef _SANDPOINT_BUS_FUNCS_H_
#define _SANDPOINT_BUS_FUNCS_H_

#ifdef _KERNEL
extern struct powerpc_bus_space sandpoint_io_space_tag;
extern struct powerpc_bus_space genppc_isa_io_space_tag;
extern struct powerpc_bus_space sandpoint_mem_space_tag;
extern struct powerpc_bus_space genppc_isa_mem_space_tag;
extern struct powerpc_bus_space sandpoint_eumb_space_tag;
extern struct powerpc_bus_space sandpoint_flash_space_tag;
extern struct powerpc_bus_space sandpoint_nhgpio_space_tag;
#endif

#include <powerpc/bus_funcs.h>

#endif /* _SANDPOINT_BUS_FUNCS_H_ */
