/*	$NetBSD: ibusvar.h,v 1.17.58.1 2009/05/13 17:18:12 jym Exp $	*/

#ifndef _PMAX_IBUS_IBUSVAR_H_
#define _PMAX_IBUS_IBUSVAR_H_

#include <machine/bus.h>

struct ibus_attach_args;

struct ibus_softc {
	struct device	sc_dev;
};

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
	int	ia_cookie;		/* device cookie */
	u_int32_t ia_addr;		/* device address (KSEG1) */
	int	ia_basz;		/* badaddr() size */
};

void	ibusattach(struct device *, struct device *, void *);
int	ibusprint(void *, const char *);
void	ibus_intr_establish(struct device *, void *cookie, int level,
	    int (*handler)(void *), void *arg);

#endif	/* !_PMAX_IBUS_IBUSVAR_H_ */
