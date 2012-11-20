/* $NetBSD: rsbus.h,v 1.3.12.1 2012/11/20 03:00:54 tls Exp $ */

#ifndef _RSBUS_H_
#define _RSBUS_H_

#include <sys/conf.h>
#include <sys/device.h>
#include <sys/queue.h>
#include <sys/bus.h>

struct rsbus_softc {
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
