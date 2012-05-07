/* $NetBSD: bus.h,v 1.2 2012/05/07 18:16:38 tsutsui Exp $ */

/*
 * XXX: A dummy <machine/bus.h> for MI <sys/bus.h>.
 */

#ifndef _MACHINE_BUS_H_
#define	_MACHINE_BUS_H_

typedef paddr_t	bus_addr_t;
typedef psize_t	bus_size_t;

typedef int     bus_space_tag_t;
typedef int     bus_space_handle_t;

/*
 * There is no bus_dma(9)'fied bus drivers on this port.
 */
#define __HAVE_NO_BUS_DMA

#endif /* _MACHINE_BUS_H_ */
