/* $NetBSD: mcclock_pnpbus.c,v 1.3.52.1 2008/01/10 23:43:58 bouyer Exp $ */
/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tim Rightnour
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * PNPBus driver for the mc146818 compatible time-of-day clock found on all
 * IBM Prep machines.  This one is only discernable from the motorola clock
 * part by the interface portion of the residual data. (or by the small vendor
 * item chip id)
 * NOTE: The IBM machines actually have either a dallas 1385 or a dallas 1585
 * chip.  This driver should probably be replaced by something that does that
 * one day.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcclock_pnpbus.c,v 1.3.52.1 2008/01/10 23:43:58 bouyer Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/isa_machdep.h>
#include <machine/residual.h>
#include <machine/chpidpnp.h>
/* XXX */
#include <machine/pio.h>

#include <dev/clock_subr.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>
#include <dev/ic/ds1687reg.h>

#include <dev/isa/isavar.h>

#include <prep/pnpbus/pnpbusvar.h>

static int	mcclock_pnpbus_probe(struct device *, struct cfdata *, void *);
static void	mcclock_pnpbus_attach(struct device *, struct device *, void *);

extern struct cfdriver mcclock_cd;

CFATTACH_DECL(mcclock_pnpbus, sizeof (struct mc146818_softc),
    mcclock_pnpbus_probe, mcclock_pnpbus_attach, NULL, NULL);

void	mcclock_pnpbus_write(struct mc146818_softc *, u_int, u_int);
u_int	mcclock_pnpbus_read(struct mc146818_softc *, u_int);
void	ds1585_reboot(void);

int have_ds1585 = 0;
#define MCCLOCK_STD_DEV		0

static int
mcclock_pnpbus_probe(struct device *parent, struct cfdata *match, void *aux)
{
	struct pnpbus_dev_attach_args *pna = aux;
	int ret = 0;

	if (strcmp(pna->pna_devid, "PNP0B00") == 0 &&
	    pna->subtype == RealTimeClock && pna->chipid == DallasRTC)
		ret = 1;
	if (strcmp(pna->pna_devid, "PNP0B00") == 0 &&
	    pna->subtype == RealTimeClock && pna->chipid == Dallas1585)
		ret = 1;

	if (ret)
		pnpbus_scan(pna, pna->pna_ppc_dev);
	return ret;
}

static void
mcclock_pnpbus_attach(struct device *parent, struct device *self, void *aux)
{
	struct mc146818_softc *sc = (void *)self;
	struct pnpbus_dev_attach_args *pna = aux;

	sc->sc_bst = pna->pna_iot;
	if (pnpbus_io_map(&pna->pna_res, 0, &sc->sc_bst, &sc->sc_bsh)) {
		/* XXX should we panic instead? */
		aprint_error("%s: couldn't map clock I/O space",
		    device_xname(self));
		return;
	}

	if (pna->chipid == Dallas1585)
		have_ds1585 = 1;
	sc->sc_year0 = 1900;
	sc->sc_mcread = mcclock_pnpbus_read;
	sc->sc_mcwrite = mcclock_pnpbus_write;
	sc->sc_flag = MC146818_BCD;
	mc146818_attach(sc);

	aprint_normal("\n");

	(*sc->sc_mcwrite)(sc, MC_REGB, MC_REGB_24HR);
}

void
ds1585_reboot(void)
{
	struct mc146818_softc *sc = mcclock_cd.cd_devs[MCCLOCK_STD_DEV];
	int i, j;

	if (!have_ds1585)
		return;

	/* monitors b0: 05,03,01  b1: 49, WIE=1 */

	(*sc->sc_mcwrite)(sc, DS1687_ASEC, 0);

	/* If we are nearing 59 minutes on the hour, the math is too complex
	 * to compute out when to reboot, so we just wait for the clock to
	 * flip back over to zero, then set the alarm time.
	 */
	i = (*sc->sc_mcread)(sc, DS1687_MIN);
	if (i == 0x58) {
		(*sc->sc_mcwrite)(sc, DS1687_AMIN, 0x59);
		(*sc->sc_mcwrite)(sc, DS1687_ASEC, 0x59);
		i = 0x59;
	} else {
		while (i == 0x59) {
			delay(100);
			i = (*sc->sc_mcread)(sc, DS1687_MIN);
		}
		i += 2;
		if ((i & 0x0f) > 8) 
			i = (i & 0xf0) + 0x10;
		(*sc->sc_mcwrite)(sc, DS1687_AMIN, i);
	}
	i = (*sc->sc_mcread)(sc, DS1687_HOUR);
	(*sc->sc_mcwrite)(sc, DS1687_AHOUR, i);
	j = (*sc->sc_mcread)(sc, DS1687_DOM); /* get date */
	i = (*sc->sc_mcread)(sc, DS1687_CONTROLA);
	i |= DS1687_BANK1; /* set DV0 on */
	(*sc->sc_mcwrite)(sc, DS1687_CONTROLA, i);
	/* write today into wakeup */
	(*sc->sc_mcwrite)(sc, DS1687_BANK1_ADATE, j);
	i = (*sc->sc_mcread)(sc, 0x4b); /* bank1 reg B */
	i |= 0x82; /* WIE + ABE */
	(*sc->sc_mcwrite)(sc, 0x4b, i);
	i = (*sc->sc_mcread)(sc, 0x4b);
	/* XXX power control bit on 7024/7025 */
	outb(PREP_BUS_SPACE_IO + 0x856, 1);
	for (;;);
}

void
mcclock_pnpbus_write(struct mc146818_softc *sc, u_int reg, u_int datum)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	bus_space_write_1(iot, ioh, 1, datum);
}

u_int
mcclock_pnpbus_read(struct mc146818_softc *sc, u_int reg)
{
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
	u_int datum;

	iot = sc->sc_bst;
	ioh = sc->sc_bsh;

	bus_space_write_1(iot, ioh, 0, reg);
	datum = bus_space_read_1(iot, ioh, 1);

	return (datum);
}
