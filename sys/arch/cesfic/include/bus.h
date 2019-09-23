/* $NetBSD: bus.h,v 1.3 2019/09/23 16:17:55 skrll Exp $ */

/*
 * XXX: A dummy <machine/bus.h> for MI <sys/bus.h>.
 */

#ifndef _MACHINE_BUS_H_
#define	_MACHINE_BUS_H_

typedef paddr_t	bus_addr_t;
typedef psize_t	bus_size_t;

#define PRIxBUSADDR	PRIxPADDR
#define PRIxBUSSIZE	PRIxPSIZE
#define PRIuBUSSIZE	PRIuPSIZE

typedef int     bus_space_tag_t;
typedef int     bus_space_handle_t;

#define PRIxBSH		"x"

/*
 * There is no bus_dma(9)'fied bus drivers on this port.
 */
#define __HAVE_NO_BUS_DMA

#endif /* _MACHINE_BUS_H_ */
