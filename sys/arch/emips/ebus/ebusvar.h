/*	$NetBSD: ebusvar.h,v 1.3 2011/06/12 04:00:33 tsutsui Exp $	*/

#ifndef _EMIPS_EBUS_EBUSVAR_H_
#define _EMIPS_EBUS_EBUSVAR_H_

#include <machine/bus.h>

struct ebus_attach_args;

/*
 * Arguments used to attach an ebus "device" to its parent
 */
struct ebus_dev_attach_args {
	const char *ida_busname;		/* XXX should be common */
	bus_space_tag_t	ida_memt;

	int	ida_ndevs;
	struct ebus_attach_args	*ida_devs;
};

/*
 * Arguments used to attach devices to an ebus
 */
struct ebus_attach_args {
	const char *ia_name;	/* device name */
	int ia_cookie;		/* device cookie */
	uint32_t ia_paddr;	/* device address (PHYSICAL) */
	void *ia_vaddr;		/* device address (VIRTUAL) */
	int ia_basz;		/* device size
				   (for min regset at probe, else 0) */
};

void	ebusattach(device_t , device_t , void *);
int	ebusprint(void *, const char *);
void	ebus_intr_establish(device_t , void *, int,
	    int (*)(void *, void *), void *);

#endif	/* !_EMIPS_EBUS_EBUSVAR_H_ */
