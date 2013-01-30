/*	$NetBSD: bcm2835_rng.c,v 1.2 2013/01/30 11:52:54 jmcneill Exp $ */

/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared D. McNeill
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_rng.c,v 1.2 2013/01/30 11:52:54 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/rnd.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#define RNG_CTRL		0x00
#define  RNG_CTRL_EN		__BIT(0)
#define RNG_STATUS		0x04
#define  RNG_STATUS_CNT_MASK	__BITS(31,24)
#define  RNG_STATUS_CNT_SHIFT	24
#define RNG_DATA		0x08

#define RNG_DATA_MAX		256

struct bcm2835rng_softc {
	device_t sc_dev;

	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;

	krndsource_t sc_rnd;
	callout_t sc_tick;

	uint32_t sc_data[RNG_DATA_MAX];
};

static int bcmrng_match(device_t, cfdata_t, void *);
static void bcmrng_attach(device_t, device_t, void *);

static void bcmrng_tick(void *);

CFATTACH_DECL_NEW(bcmrng_amba, sizeof(struct bcm2835rng_softc),
    bcmrng_match, bcmrng_attach, NULL, NULL);

/* ARGSUSED */
static int
bcmrng_match(device_t parent, cfdata_t match, void *aux)
{
	struct amba_attach_args *aaa = aux;

	if (strcmp(aaa->aaa_name, "bcmrng") != 0)
		return 0;

	return 1;
}

static void
bcmrng_attach(device_t parent, device_t self, void *aux)
{
        struct bcm2835rng_softc *sc = device_private(self);
 	struct amba_attach_args *aaa = aux;
	uint32_t ctrl;

	aprint_naive("\n");
	aprint_normal(": RNG\n");

	sc->sc_dev = self;
	sc->sc_iot = aaa->aaa_iot;

	if (bus_space_map(aaa->aaa_iot, aaa->aaa_addr, BCM2835_RNG_SIZE, 0,
	    &sc->sc_ioh)) {
		aprint_error_dev(sc->sc_dev, "unable to map device\n");
		return;
	}

	rnd_attach_source(&sc->sc_rnd, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_NO_ESTIMATE);

	callout_init(&sc->sc_tick, CALLOUT_MPSAFE);
	callout_setfunc(&sc->sc_tick, bcmrng_tick, sc);

	/* discard initial numbers, broadcom says they are "less random" */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RNG_STATUS, 0x40000);

	/* enable rng */
	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RNG_CTRL);
	ctrl |= RNG_CTRL_EN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RNG_CTRL, ctrl);

	/* start timer */
	bcmrng_tick(sc);
}

static void
bcmrng_tick(void *priv)
{
	struct bcm2835rng_softc *sc = priv;
	uint32_t status;
	int cnt, i;

	status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RNG_STATUS);
	cnt = (status & RNG_STATUS_CNT_MASK) >> RNG_STATUS_CNT_SHIFT;
	for (i = 0; i < cnt; i++) {
		sc->sc_data[i] = bus_space_read_4(sc->sc_iot, sc->sc_ioh,
		    RNG_DATA);
	}
	if (cnt > 0) {
		rnd_add_data(&sc->sc_rnd, sc->sc_data,
		    cnt * 4, cnt * 4 * NBBY);
	}

	callout_schedule(&sc->sc_tick, 1);
}
