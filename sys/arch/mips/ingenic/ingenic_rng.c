/*	$NetBSD: ingenic_rng.c,v 1.2.2.3 2015/12/27 12:09:38 skrll Exp $ */

/*-
 * Copyright (c) 2015 Michael McConville
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
__KERNEL_RCSID(0, "$NetBSD: ingenic_rng.c,v 1.2.2.3 2015/12/27 12:09:38 skrll Exp $");

/*
 * adapted from Jared McNeill's amlogic_rng.c
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/callout.h>
#include <sys/bus.h>
#include <sys/workqueue.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <mips/ingenic/ingenic_var.h>
#include <mips/ingenic/ingenic_regs.h>

#include "opt_ingenic.h"

struct ingenic_rng_softc;

static int	ingenic_rng_match(device_t, cfdata_t, void *);
static void	ingenic_rng_attach(device_t, device_t, void *);

static void	ingenic_rng_get(struct ingenic_rng_softc *);
static void	ingenic_rng_get_intr(void *);
static void	ingenic_rng_get_cb(size_t, void *);

struct ingenic_rng_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	void *			sc_sih;

	krndsource_t		sc_rndsource;
	size_t			sc_bytes_wanted;

	kmutex_t		sc_intr_lock;
	kmutex_t		sc_rnd_lock;
};

CFATTACH_DECL_NEW(ingenic_rng, sizeof(struct ingenic_rng_softc),
	ingenic_rng_match, ingenic_rng_attach, NULL, NULL);

static int
ingenic_rng_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct apbus_attach_args *aa = aux;

	return !(strcmp(aa->aa_name, "jzrng"));
}

static void
ingenic_rng_attach(device_t parent, device_t self, void *aux)
{
	struct ingenic_rng_softc * const sc = device_private(self);
	const struct apbus_attach_args * const aa = aux;
	bus_addr_t addr = aa->aa_addr;
	int error;

	sc->sc_dev = self;
	sc->sc_bst = aa->aa_bst;
	if (addr == 0)
		addr = JZ_RNG;

	error = bus_space_map(aa->aa_bst, addr, 4, 0, &sc->sc_bsh);
	if (error) {
		aprint_error_dev(self,
		    "can't map registers for %s: %d\n", aa->aa_name, error);
		return;
	}

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SERIAL);
	mutex_init(&sc->sc_rnd_lock, MUTEX_DEFAULT, IPL_SERIAL);
	
	aprint_naive(": Ingenic random number generator\n");
	aprint_normal(": Ingenic random number generator\n");

	sc->sc_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    ingenic_rng_get_intr, sc);
	if (sc->sc_sih == NULL) {
		aprint_error_dev(self, "couldn't establish softint\n");
		return;
	}

	rndsource_setcb(&sc->sc_rndsource, ingenic_rng_get_cb, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	ingenic_rng_get_cb(RND_POOLBITS / NBBY, sc);
}

static void
ingenic_rng_get(struct ingenic_rng_softc *sc)
{
	uint32_t data;

	mutex_spin_enter(&sc->sc_intr_lock);
	while (sc->sc_bytes_wanted) {
		data = bus_space_read_4(sc->sc_bst, sc->sc_bsh, 0);
		mutex_spin_exit(&sc->sc_intr_lock);
		mutex_spin_enter(&sc->sc_rnd_lock);
		rnd_add_data(&sc->sc_rndsource, &data, sizeof(data),
		    sizeof(data) * NBBY);
		mutex_spin_exit(&sc->sc_rnd_lock);
		mutex_spin_enter(&sc->sc_intr_lock);
		sc->sc_bytes_wanted -= MIN(sc->sc_bytes_wanted, sizeof(data));
	}
	explicit_memset(&data, 0, sizeof(data));
	mutex_spin_exit(&sc->sc_intr_lock);
}

static void
ingenic_rng_get_cb(size_t bytes_wanted, void *priv)
{
	struct ingenic_rng_softc * const sc = priv;

	mutex_spin_enter(&sc->sc_intr_lock);
	if (sc->sc_bytes_wanted == 0)
		softint_schedule(sc->sc_sih);

	if (bytes_wanted > (UINT_MAX - sc->sc_bytes_wanted))
		sc->sc_bytes_wanted = UINT_MAX;
	else
		sc->sc_bytes_wanted += bytes_wanted;

	mutex_spin_exit(&sc->sc_intr_lock);
}

static void
ingenic_rng_get_intr(void *priv)
{
	struct ingenic_rng_softc * const sc = priv;

	ingenic_rng_get(sc);
}
