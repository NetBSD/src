/* $NetBSD: mcclock_ioasic.c,v 1.1 1997/06/22 09:34:50 jonathan Exp $ */

/*
 * Copyright (c) 1994, 1995, 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
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

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: mcclock_ioasic.c,v 1.1 1997/06/22 09:34:50 jonathan Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <dev/tc/clockvar.h>
#include <dev/tc/mcclockvar.h>
#include <dev/ic/mc146818reg.h>
#include <dev/tc/tcreg.h>
#include <dev/tc/tcvar.h> 
#include <dev/tc/ioasicvar.h>                   /* XXX */

struct mcclock_ioasic_clockdatum {
	u_char	datum;
	char	pad[3];
};

struct mcclock_ioasic_softc {
	struct mcclock_softc	sc_mcclock;

	struct mcclock_ioasic_clockdatum *sc_dp;
};

int	mcclock_ioasic_match __P((struct device *, struct cfdata *, void *));
void	mcclock_ioasic_attach __P((struct device *, struct device *, void *));

struct cfattach mcclock_ioasic_ca = {
	sizeof (struct mcclock_ioasic_softc), (void *)mcclock_ioasic_match,
	    mcclock_ioasic_attach, 
};

void	mcclock_ioasic_write __P((struct mcclock_softc *, u_int, u_int));
u_int	mcclock_ioasic_read __P((struct mcclock_softc *, u_int));

const struct mcclock_busfns mcclock_ioasic_busfns = {
	mcclock_ioasic_write, mcclock_ioasic_read,
};

volatile struct chiptime *Mach_clock_addr; /* XXX glue for pmax heritage */

int
mcclock_ioasic_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
#define	CFMATCH(cf,x) !strcmp((cf)->dv_cfdata->cf_driver->cd_name,(x))
	char *name;
	vm_offset_t addr;

	if (CFMATCH(parent, "mainbus")) {
		struct confargs *ca = aux;
		addr = (vm_offset_t)ca->ca_addr;
		name = ca->ca_name;
	}
	else
	if (CFMATCH(parent, "asic")) {
		struct ioasicdev_attach_args *d = aux;
		addr = d->iada_addr;
		name = d->iada_modname;
	}
	else {
		/*panic("clock must be attached with mainbus or ioasic\n");*/
		printf("clock on %s: must be attached at mainbus or ioasic\n",
		       parent->dv_cfdata->cf_driver->cd_name);
		return(0);
	}
	if (strcmp("mc146818", name))
		return (0);
	Mach_clock_addr = (struct chiptime *)MIPS_PHYS_TO_KSEG1(addr);
	return (1);
#undef	CFMATCH
}

void
mcclock_ioasic_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ioasicdev_attach_args *ioasicdev = aux;
	struct mcclock_ioasic_softc *sc = (struct mcclock_ioasic_softc *)self;

	sc->sc_dp = (struct mcclock_ioasic_clockdatum *)ioasicdev->iada_addr;

	mcclock_attach(&sc->sc_mcclock, &mcclock_ioasic_busfns);
}

void
mcclock_ioasic_write(dev, reg, datum)
	struct mcclock_softc *dev;
	u_int reg, datum;
{
	struct mcclock_ioasic_softc *sc = (struct mcclock_ioasic_softc *)dev;

	sc->sc_dp[reg].datum = datum;
}

u_int
mcclock_ioasic_read(dev, reg)
	struct mcclock_softc *dev;
	u_int reg;
{
	struct mcclock_ioasic_softc *sc = (struct mcclock_ioasic_softc *)dev;

	return (sc->sc_dp[reg].datum);
}

/*XXX*/
/*
 * Wait "n" microseconds. (scsi code needs this).
 */
void
delay(n)
        int n;
{
        DELAY(n);
}
