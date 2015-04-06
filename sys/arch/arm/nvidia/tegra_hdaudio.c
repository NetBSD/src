/* $NetBSD: tegra_hdaudio.c,v 1.1.2.2 2015/04/06 15:17:53 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "locators.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: tegra_hdaudio.c,v 1.1.2.2 2015/04/06 15:17:53 skrll Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <dev/hdaudio/hdaudioreg.h>
#include <dev/hdaudio/hdaudiovar.h>

#include <arm/nvidia/tegra_var.h>

static int	tegra_hdaudio_match(device_t, cfdata_t, void *);
static void	tegra_hdaudio_attach(device_t, device_t, void *);
static int	tegra_hdaudio_detach(device_t, int);
static int	tegra_hdaudio_rescan(device_t, const char *, const int *);
static void	tegra_hdaudio_childdet(device_t, device_t);

static int	tegra_hdaudio_intr(void *);

struct tegra_hdaudio_softc {
	struct hdaudio_softc	sc;
	void			*sc_ih;
};

CFATTACH_DECL2_NEW(tegra_hdaudio, sizeof(struct tegra_hdaudio_softc),
	tegra_hdaudio_match, tegra_hdaudio_attach, tegra_hdaudio_detach, NULL,
	tegra_hdaudio_rescan, tegra_hdaudio_childdet);

static int
tegra_hdaudio_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
tegra_hdaudio_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);
	struct tegraio_attach_args * const tio = aux;
	const struct tegra_locators * const loc = &tio->tio_loc;

	sc->sc.sc_memt = tio->tio_bst;
	bus_space_subregion(tio->tio_bst, tio->tio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc.sc_memh);
	sc->sc.sc_memvalid = true;
	sc->sc.sc_dmat = tio->tio_dmat;

	aprint_naive("\n");
	aprint_normal(": SATA\n");

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_AUDIO, IST_LEVEL,
	    tegra_hdaudio_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	hdaudio_attach(self, &sc->sc);
}

static int
tegra_hdaudio_detach(device_t self, int flags)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);

	hdaudio_detach(&sc->sc, flags);

	if (sc->sc_ih) {
		intr_disestablish(sc->sc_ih);
		sc->sc_ih = NULL;
	}

	sc->sc.sc_memvalid = false;

	return 0;
}

static int
tegra_hdaudio_rescan(device_t self, const char *ifattr, const int *locs)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);

	return hdaudio_rescan(&sc->sc, ifattr, locs);
}

static void
tegra_hdaudio_childdet(device_t self, device_t child)
{
	struct tegra_hdaudio_softc * const sc = device_private(self);

	hdaudio_childdet(&sc->sc, child);
}

static int
tegra_hdaudio_intr(void *priv)
{
	struct tegra_hdaudio_softc * const sc = priv;

	return hdaudio_intr(&sc->sc);
}
