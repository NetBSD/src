/* $NetBSD: bus.h,v 1.2.2.2 2010/08/11 22:52:41 yamt Exp $ */

/*
 * A very basic <machine/bus.h>.  If/once "proper" bus.h support is needed,
 * this can simply include:
 *
 *	#include <mips/bus_space.h>
 *	#include <mips/bus_dma.h>
 *
 * as sys/arch/evbmips/include/bus.h does.
 */

#ifndef _SBMIPS_BUS_H_
#define	_SBMIPS_BUS_H_

typedef paddr_t	bus_addr_t;
typedef psize_t	bus_size_t;

typedef int     bus_space_tag_t;
typedef int     bus_space_handle_t;

#endif /* _SBMIPS_BUS_H_ */
