/*	$NetBSD: bus.h,v 1.1 1996/03/27 10:09:10 jonathan Exp $	*/

#ifndef _PICA_BUS_H_
#define _PICA_BUS_H_

/*
 * ISA  "I/O space" macros
 */
#include <machine/pio.h>

/*
 * I/O addresses (in bus space)
 */
typedef u_int32_t bus_io_addr_t;
typedef u_int32_t bus_io_size_t;

/*
 * Memory addresses (in bus space)
 */
typedef u_int32_t bus_mem_addr_t;
typedef u_int32_t bus_mem_size_t;

/*
 * Access methods for bus resources, I/O space, and memory space.
 */
typedef void *bus_chipset_tag_t;
typedef u_int32_t bus_io_handle_t;
typedef caddr_t bus_mem_handle_t;

#endif	/* _PICA_BUS_H_ */
