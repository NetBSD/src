/* $NetBSD: ioasic.c,v 1.1.2.15 1999/11/30 08:49:52 nisimura Exp $ */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Keith Bostic, Chris G. Demetriou, Jonathan Stone
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

__KERNEL_RCSID(0, "$NetBSD: ioasic.c,v 1.1.2.15 1999/11/30 08:49:52 nisimura Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <pmax/pmax/pmaxtype.h>
#include <dev/tc/tcvar.h>
#include <dev/tc/ioasicvar.h>
#include <pmax/tc/ioasicreg.h>

#include "opt_dec_3min.h"
#include "opt_dec_maxine.h"
#include "opt_dec_3maxplus.h"

int	ioasicmatch __P((struct device *, struct cfdata *, void *));
void	ioasicattach __P((struct device *, struct device *, void *));

struct cfattach ioasic_ca = {
	sizeof(struct ioasic_softc), ioasicmatch, ioasicattach,
};

struct ioasic_dev *ioasic_devs;
int ioasic_ndevs, builtin_ndevs;

tc_addr_t ioasic_base;

extern struct ioasic_dev xine_ioasic_devs[];
extern int xine_builtin_ndevs, xine_ioasic_ndevs;
extern struct ioasic_dev kmin_ioasic_devs[];
extern int kmin_builtin_ndevs, kmin_ioasic_ndevs;
extern struct ioasic_dev kn03_ioasic_devs[];
extern int kn03_builtin_ndevs, kn03_ioasic_ndevs;

int
ioasicmatch(parent, cfdata, aux)
	struct device *parent;
	struct cfdata *cfdata;
	void *aux;
{
	struct tc_attach_args *ta = aux;

	/* Make sure that we're looking for this type of device. */
	if (strncmp("IOCTL   ", ta->ta_modname, TC_ROM_LLEN))
		return (0);

	if (cfdata->cf_unit > 0)
		return (0);

	return (1);
}

void
ioasicattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ioasic_softc *sc = (struct ioasic_softc *)self;
	struct tc_attach_args *ta = aux;
	int i, imsk;

	sc->sc_bst = ta->ta_memt;
	if (bus_space_map(ta->ta_memt, ta->ta_addr,
			0x400000, 0, &sc->sc_bsh)) {
		printf("%s: unable to map device\n", sc->sc_dv.dv_xname);
		return;
	}
	sc->sc_dmat = ta->ta_dmat;
	sc->sc_cookie = ta->ta_cookie;

	sc->sc_base = ta->ta_addr; /* XXX XXX XXX */

	printf("\n");

	switch (systype) {
#if defined(DEC_MAXINE)
	case DS_MAXINE:
		ioasic_devs = xine_ioasic_devs;
		ioasic_ndevs = xine_ioasic_ndevs;
		builtin_ndevs = xine_builtin_ndevs;
		break;
#endif
#if defined(DEC_3MIN)
	case DS_3MIN:
		ioasic_devs = kmin_ioasic_devs;
		ioasic_ndevs = kmin_ioasic_ndevs;
		builtin_ndevs = kmin_builtin_ndevs;
		break;
#endif
#if defined(DEC_3MAXPLUS)
	case DS_3MAXPLUS:
		ioasic_devs = kn03_ioasic_devs;
		ioasic_ndevs = kn03_ioasic_ndevs;
		builtin_ndevs = kn03_builtin_ndevs;
		break;
#endif
	default:
		panic("ioasicmatch: how did we get here?");
	}

	/*
	 * Turn off all device interrupt bits.
	 * (This _does_ include TC option slot bits.
	 */
	imsk = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	for (i = 0; i < ioasic_ndevs; i++)
		imsk &= ~ioasic_devs[i].iad_intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, imsk);

	/*
	 * Try to configure each device.
	 */
	ioasic_attach_devs(sc, ioasic_devs, builtin_ndevs);
}

void
ioasic_intr_establish(ioa, cookie, level, func, arg)
	struct device *ioa;
	void *cookie, *arg;
	int level;
	int (*func) __P((void *));
{
	struct ioasic_softc *sc = (void *)ioasic_cd.cd_devs[0];
	int i, intrbits;

	for (i = 0; i < ioasic_ndevs; i++) {
		if (ioasic_devs[i].iad_cookie == cookie)
			goto found;
	}
	panic("ioasic_intr_establish: invalid cookie %d", (int)cookie);
found:

	intrtab[(int)cookie].ih_func = func;
	intrtab[(int)cookie].ih_arg = arg;
	
	intrbits = ioasic_devs[i].iad_intrbits;
	i = bus_space_read_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK);
	i |= intrbits;
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, IOASIC_IMSK, i);
	iplmask[level] |= intrbits;
}

void
ioasic_intr_disestablish(ioa, cookie)
	struct device *ioa;
	void *cookie;
{
	panic("ioasic_intr_disestablish: cookie %d", (int)cookie);
}

/*
 * spl(9) for IOASIC DECstations
 */
int _splraise_ioasic __P((int));
int _spllower_ioasic __P((int));
int _splrestore_ioasic __P((int));

int
_splraise_ioasic(lvl)
	int lvl;
{
	u_int32_t new;

	new = oldiplmask[lvl] = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
	new &= iplmask[IPL_HIGH] &~ iplmask[lvl];
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = new;
	tc_wmb();
	return lvl;
}

int
_spllower_ioasic(lvl)
{
	u_int32_t new;

	new = oldiplmask[lvl] = *(u_int32_t *)(ioasic_base + IOASIC_IMSK);
	*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = iplmask[IPL_HIGH];
	tc_wmb();
	return lvl;
}

int
_splrestore_ioasic(lvl)
	int lvl;
{
	if (lvl > IPL_HIGH)
		_splset(MIPS_SR_INT_IE | lvl);
	else {
		*(u_int32_t *)(ioasic_base + IOASIC_IMSK) = oldiplmask[lvl];
		tc_wmb();
	}
	return lvl;
}
