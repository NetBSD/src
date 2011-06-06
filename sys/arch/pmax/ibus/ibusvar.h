/*	$NetBSD: ibusvar.h,v 1.18.6.1 2011/06/06 09:06:22 jruoho Exp $	*/

#ifndef _PMAX_IBUS_IBUSVAR_H_
#define _PMAX_IBUS_IBUSVAR_H_

#include <machine/bus.h>

struct ibus_attach_args;

/*
 * Arguments used to attach an ibus "device" to its parent
 */
struct ibus_dev_attach_args {
	const char *ida_busname;		/* XXX should be common */
	bus_space_tag_t	ida_memt;

	int	ida_ndevs;
	struct ibus_attach_args	*ida_devs;
};

/*
 * Arguments used to attach devices to an ibus
 */
struct ibus_attach_args {
	const char *ia_name;		/* device name */
	intptr_t ia_cookie;		/* device cookie */
	bus_addr_t ia_addr;		/* device address (KSEG1) */
	bus_size_t ia_basz;		/* badaddr() size */
};

void	ibusattach(device_t, device_t, void *);
int	ibusprint(void *, const char *);
void	ibus_intr_establish(device_t, void *cookie, int level,
	    int (*handler)(void *), void *arg);

#endif	/* !_PMAX_IBUS_IBUSVAR_H_ */
