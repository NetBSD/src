/*	$NetBSD: ioasic_subr.c,v 1.4.2.3 2004/09/18 14:51:45 skrll Exp $	*/

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Keith Bostic, Chris G. Demetriou
 * 
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND 
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */


#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ioasic_subr.c,v 1.4.2.3 2004/09/18 14:51:45 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>

#include "locators.h"

int     ioasicprint(void *, const char *);
int ioasicsubmatch(struct device *, struct cfdata *,
		   const locdesc_t *, void *);

int
ioasicprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct ioasicdev_attach_args *d = aux;

	if (pnp)
		aprint_normal("%s at %s", d->iada_modname, pnp);
	aprint_normal(" offset 0x%x", d->iada_offset);
	return (UNCONF);
}

int
ioasicsubmatch(struct device *parent, struct cfdata *cf,
		const locdesc_t *ldesc, void *aux)
{

	if ((cf->cf_loc[IOASICCF_OFFSET] != IOASICCF_OFFSET_DEFAULT) &&
	    (cf->cf_loc[IOASICCF_OFFSET] != ldesc->locs[IOASICCF_OFFSET]))
		return (0);

	return (config_match(parent, cf, aux));
}

void
ioasic_attach_devs(sc, ioasic_devs, ioasic_ndevs)
	struct ioasic_softc *sc;
	struct ioasic_dev *ioasic_devs;
	int ioasic_ndevs;
{
	struct ioasicdev_attach_args idev;
	int i;
	int help[2];
	locdesc_t *ldesc = (void *)help; /* XXX */

        /*
	 * Try to configure each device.
	 */
        for (i = 0; i < ioasic_ndevs; i++) {
		strlcpy(idev.iada_modname, ioasic_devs[i].iad_modname,
		    sizeof(idev.iada_modname));
		idev.iada_offset = ioasic_devs[i].iad_offset;
		idev.iada_addr = sc->sc_base + ioasic_devs[i].iad_offset;
		idev.iada_cookie = ioasic_devs[i].iad_cookie;

                /* Tell the autoconfig machinery we've found the hardware. */
		ldesc->len = 1;
		ldesc->locs[IOASICCF_OFFSET] = ioasic_devs[i].iad_offset;
		config_found_sm_loc(&sc->sc_dv, "ioasic", ldesc, &idev,
				    ioasicprint, ioasicsubmatch);
        }
}
