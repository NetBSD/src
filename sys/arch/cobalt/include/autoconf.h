/*	$NetBSD: autoconf.h,v 1.1 2000/03/19 23:07:45 soren Exp $	*/

#include <machine/bus.h>

struct mainbus_attach_args {
	char		*ma_name;
	unsigned long	ma_addr;
	bus_space_tag_t	ma_iot;
	bus_space_handle_t ma_ioh;
	int		ma_level;
};
