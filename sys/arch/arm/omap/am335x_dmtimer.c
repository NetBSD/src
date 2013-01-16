/*	$NetBSD: am335x_dmtimer.c,v 1.1.2.2 2013/01/16 05:32:49 yamt Exp $	*/

/*
 * TI OMAP Dual-mode timer attachments for AM335x
 */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: am335x_dmtimer.c,v 1.1.2.2 2013/01/16 05:32:49 yamt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <arm/omap/am335x_prcm.h>

#include <arm/omap/omap2_obiovar.h>
#include <arm/omap/omap_dmtimervar.h>

#ifndef STATHZ
#  define STATHZ	97
#endif

struct am335x_dmtimer {
	const char *ad_name;
	unsigned int ad_version;
	bus_addr_t ad_base_addr;
	int ad_intr;
	struct omap_module ad_module;
};

static const struct am335x_dmtimer am335x_dmtimers[] = {
	{ "DMTIMER0",	2, 0x44e05000, 66, { AM335X_PRCM_CM_WKUP, 0x10 } },
	{ "DMTIMER1ms",	1, 0x44e31000, 67, { AM335X_PRCM_CM_WKUP, 0xc4 } },
	{ "DMTIMER2",	2, 0x48040000, 68, { AM335X_PRCM_CM_PER, 0x80 } },
	{ "DMTIMER3",	2, 0x48042000, 69, { AM335X_PRCM_CM_PER, 0x84 } },
	{ "DMTIMER4",	2, 0x48044000, 92, { AM335X_PRCM_CM_PER, 0x88 } },
	{ "DMTIMER5",   2, 0x48046000, 93, { AM335X_PRCM_CM_PER, 0xec } },
	{ "DMTIMER6",	2, 0x48048000, 94, { AM335X_PRCM_CM_PER, 0xf0 } },
	{ "DMTIMER7",	2, 0x4804a000, 95, { AM335X_PRCM_CM_PER, 0x7c } },
};

static int	am335x_dmtimer_match(device_t, cfdata_t, void *);
static void	am335x_dmtimer_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(omap_dmtimer_obio, sizeof(struct omap_dmtimer_softc),
    am335x_dmtimer_match, am335x_dmtimer_attach, NULL, NULL);

static int
am335x_dmtimer_match(device_t parent, cfdata_t match, void *aux)
{
	struct obio_attach_args *obio = aux;
	size_t i;

	if (obio->obio_size != 0x1000)
		return 0;

	for (i = 0; i < __arraycount(am335x_dmtimers); i++)
		if ((obio->obio_addr == am335x_dmtimers[i].ad_base_addr) &&
		    (obio->obio_intr == am335x_dmtimers[i].ad_intr))
			return 1;

	return 0;
}

static void
am335x_dmtimer_attach(device_t parent, device_t self, void *aux)
{
	struct omap_dmtimer_softc *sc = device_private(self);
	struct obio_attach_args *obio = aux;
	const struct am335x_dmtimer *ad = NULL;
	size_t i;

	for (i = 0; i < __arraycount(am335x_dmtimers); i++)
		if ((obio->obio_addr == am335x_dmtimers[i].ad_base_addr) &&
		    (obio->obio_intr == am335x_dmtimers[i].ad_intr)) {
			ad = &am335x_dmtimers[i];
			break;
		}

	KASSERT(ad != NULL);
	aprint_normal(": %s\n", ad->ad_name);

	sc->sc_dev = self;
	sc->sc_module = &ad->ad_module;
	sc->sc_version = ad->ad_version;
	sc->sc_intr = ad->ad_intr;
	sc->sc_iot = obio->obio_iot;

	KASSERT(obio->obio_size == 0x1000);
	if (bus_space_map(obio->obio_iot, obio->obio_addr, obio->obio_size, 0,
		&sc->sc_ioh) != 0)
		panic("%s: unable to map bus space", device_xname(self));

	/* XXX Crock.  */
	switch (device_unit(self)) {
	case 0:
		omap_dmtimer_attach_hardclock(sc);
		break;

	case 1:
		omap_dmtimer_attach_timecounter(sc);
		break;

	case 2:
		KASSERT(stathz == 0);
		profhz = stathz = STATHZ;
		omap_dmtimer_attach_statclock(sc);
		break;

	default:
		panic("%s: but what am I to do?", device_xname(self));
	}
}
