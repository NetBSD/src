/*	$NetBSD: tsrtc.c,v 1.7.12.1 2017/12/03 11:36:07 jdolecek Exp $	*/

/*
 * Copyright (c) 1995, 1996 Carnegie-Mellon University.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tsrtc.c,v 1.7.12.1 2017/12/03 11:36:07 jdolecek Exp $");

#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

#include <evbarm/tsarm/tspldvar.h>
#include <evbarm/tsarm/tsarmreg.h>

int	tsrtc_match(device_t, cfdata_t, void *);
void	tsrtc_attach(device_t, device_t, void *);

struct tsrtc_softc {
	struct mc146818_softc	sc_mc;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_dath;
	bus_space_handle_t	sc_idxh;
};

CFATTACH_DECL_NEW(tsrtc, sizeof (struct tsrtc_softc),
    tsrtc_match, tsrtc_attach, NULL, NULL);

void	tsrtc_write(struct mc146818_softc *, u_int, u_int);
u_int	tsrtc_read(struct mc146818_softc *, u_int);


int
tsrtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct tspld_attach_args *aa = aux;
	struct tsrtc_softc tsrtc, *tsc;
	struct mc146818_softc *sc;
	unsigned int t1, t2;
	static int found = -1;

	if (found != -1)
		return found;

	tsc = &tsrtc;
	sc = &tsc->sc_mc;
	tsc->sc_iot = aa->ta_iot;
	if (bus_space_map(tsc->sc_iot, TS7XXX_IO8_HWBASE + TS7XXX_RTCIDX, 1, 0,
		 &tsc->sc_idxh))
		return (0);
	if (bus_space_map(tsc->sc_iot, TS7XXX_IO8_HWBASE + TS7XXX_RTCDAT, 1, 0,
		 &tsc->sc_dath))
		return (0);

	/* Read from the seconds counter. */
	t1 = bcdtobin(tsrtc_read(sc, MC_SEC));
	if (t1 > 59)
		goto unmap;

	/* Wait, then look again. */
	DELAY(1100000);
	t2 = bcdtobin(tsrtc_read(sc, MC_SEC));
	if (t2 > 59)
		goto unmap;

        /* If [1,2) seconds have passed since, call it a clock. */
	if ((t1 + 1) % 60 == t2 || (t1 + 2) % 60 == t2)
		found = 1;

 unmap:
	bus_space_unmap(tsc->sc_iot, tsc->sc_idxh, 1);
	bus_space_unmap(tsc->sc_iot, tsc->sc_dath, 1);

	return (found);
}

void
tsrtc_attach(device_t parent, device_t self, void *aux)
{
	struct tsrtc_softc *tsc = device_private(self);
	struct mc146818_softc *sc = &tsc->sc_mc;
	struct tspld_attach_args *aa = aux;

	sc->sc_dev = self;
	tsc->sc_iot = aa->ta_iot;
	if (bus_space_map(tsc->sc_iot, TS7XXX_IO8_HWBASE + TS7XXX_RTCIDX,
	    1, 0, &tsc->sc_idxh))
		panic("tsrtc_attach: couldn't map clock I/O space");
	if (bus_space_map(tsc->sc_iot, TS7XXX_IO8_HWBASE + TS7XXX_RTCDAT,
	    1, 0, &tsc->sc_dath))
		panic("tsrtc_attach: couldn't map clock I/O space");

	sc->sc_year0 = 2000;
	sc->sc_mcread = tsrtc_read;
	sc->sc_mcwrite = tsrtc_write;
	sc->sc_flag = MC146818_BCD;
	mc146818_attach(sc);

	aprint_normal("\n");

	(*sc->sc_mcwrite)(sc, MC_REGB, MC_REGB_24HR);
}

void
tsrtc_write(struct mc146818_softc *mc_sc, u_int reg, u_int datum)
{
	struct tsrtc_softc *sc = (struct tsrtc_softc *)mc_sc;

	bus_space_write_1(sc->sc_iot, sc->sc_idxh, 0, reg);
	bus_space_write_1(sc->sc_iot, sc->sc_dath, 0, datum);
}

u_int
tsrtc_read(struct mc146818_softc *mc_sc, u_int reg)
{
	struct tsrtc_softc *sc = (struct tsrtc_softc *)mc_sc;
	u_int datum;

	bus_space_write_1(sc->sc_iot, sc->sc_idxh, 0, reg);
	datum = bus_space_read_1(sc->sc_iot, sc->sc_dath, 0);

	return (datum);
}
