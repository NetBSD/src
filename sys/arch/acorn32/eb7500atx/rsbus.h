/* $NetBSD: rsbus.h,v 1.1 2004/01/03 14:31:28 chris Exp $ */

#ifndef _RSBUS_H_
#define _RSBUS_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <machine/bus.h>

struct rsbus_softc {
	struct device sc_dev;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
};

struct rsbus_attach_args {
	bus_space_tag_t		sa_iot;		/* Bus tag */
	bus_addr_t		sa_addr;	/* i/o address  */
	bus_size_t		sa_size;
	int			sa_intr;
};

#endif /* _RSBUS_H_ */
