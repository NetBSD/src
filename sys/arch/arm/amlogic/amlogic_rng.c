/* $NetBSD: amlogic_rng.c,v 1.3.16.2 2017/12/03 11:35:51 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: amlogic_rng.c,v 1.3.16.2 2017/12/03 11:35:51 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/callout.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <arm/amlogic/amlogic_reg.h>
#include <arm/amlogic/amlogic_var.h>

struct amlogic_rng_softc;

static int	amlogic_rng_match(device_t, cfdata_t, void *);
static void	amlogic_rng_attach(device_t, device_t, void *);

static void	amlogic_rng_get(struct amlogic_rng_softc *);
static void	amlogic_rng_get_intr(void *);
static void	amlogic_rng_get_cb(size_t, void *);

struct amlogic_rng_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	void *			sc_sih;

	krndsource_t		sc_rndsource;
	size_t			sc_bytes_wanted;

	kmutex_t		sc_intr_lock;
	kmutex_t		sc_rnd_lock;
};

CFATTACH_DECL_NEW(amlogic_rng, sizeof(struct amlogic_rng_softc),
	amlogic_rng_match, amlogic_rng_attach, NULL, NULL);

static int
amlogic_rng_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

static void
amlogic_rng_attach(device_t parent, device_t self, void *aux)
{
	struct amlogic_rng_softc * const sc = device_private(self);
	struct amlogicio_attach_args * const aio = aux;
	const struct amlogic_locators * const loc = &aio->aio_loc;

	sc->sc_dev = self;
	sc->sc_bst = aio->aio_core_bst;
	bus_space_subregion(aio->aio_core_bst, aio->aio_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SERIAL);
	mutex_init(&sc->sc_rnd_lock, MUTEX_DEFAULT, IPL_SERIAL);
	
	amlogic_rng_init();

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    amlogic_rng_get_intr, sc);
	if (sc->sc_sih == NULL) {
		aprint_error_dev(self, "couldn't establish softint\n");
		return;
	}

	rndsource_setcb(&sc->sc_rndsource, amlogic_rng_get_cb, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	amlogic_rng_get_cb(RND_POOLBITS / NBBY, sc);
}

static void
amlogic_rng_get(struct amlogic_rng_softc *sc)
{
	uint32_t data[2];

	mutex_spin_enter(&sc->sc_intr_lock);
	while (sc->sc_bytes_wanted) {
		bus_space_read_region_4(sc->sc_bst, sc->sc_bsh, 0, data, 2);
		mutex_spin_exit(&sc->sc_intr_lock);
		mutex_spin_enter(&sc->sc_rnd_lock);
		rnd_add_data(&sc->sc_rndsource, data, sizeof(data),
		    sizeof(data) * NBBY);
		mutex_spin_exit(&sc->sc_rnd_lock);
		mutex_spin_enter(&sc->sc_intr_lock);
		sc->sc_bytes_wanted -= MIN(sc->sc_bytes_wanted, sizeof(data));
	}
	explicit_memset(data, 0, sizeof(data));
	mutex_spin_exit(&sc->sc_intr_lock);
}

static void
amlogic_rng_get_cb(size_t bytes_wanted, void *priv)
{
	struct amlogic_rng_softc * const sc = priv;

	mutex_spin_enter(&sc->sc_intr_lock);
	if (sc->sc_bytes_wanted == 0) {
		softint_schedule(sc->sc_sih);
	}
	if (bytes_wanted > (UINT_MAX - sc->sc_bytes_wanted)) {
		sc->sc_bytes_wanted = UINT_MAX;
	} else {
		sc->sc_bytes_wanted += bytes_wanted;
	}
	mutex_spin_exit(&sc->sc_intr_lock);
}

static void
amlogic_rng_get_intr(void *priv)
{
	struct amlogic_rng_softc * const sc = priv;

	amlogic_rng_get(sc);
}
