/*	$NetBSD: bcm2835_rng.c,v 1.3.6.5 2017/12/03 11:35:52 jdolecek Exp $ */

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_rng.c,v 1.3.6.5 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/rndpool.h>
#include <sys/rndsource.h>

#include <arm/broadcom/bcm_amba.h>
#include <arm/broadcom/bcm2835reg.h>
#include <arm/broadcom/bcm2835_intr.h>

#define RNG_CTRL		0x00
#define  RNG_CTRL_EN		__BIT(0)
#define RNG_STATUS		0x04
#define  RNG_STATUS_CNT		__BITS(31,24)
#define RNG_DATA		0x08

#define RNG_DATA_MAX		256

struct bcm2835rng_softc {
	device_t		sc_dev;

	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;

	kmutex_t		sc_lock;
	krndsource_t		sc_rndsource;
};

static int bcmrng_match(device_t, cfdata_t, void *);
static void bcmrng_attach(device_t, device_t, void *);
static void bcmrng_get(size_t, void *);

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

	/* discard initial numbers, broadcom says they are "less random" */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RNG_STATUS, 0x40000);

	/* enable rng */
	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RNG_CTRL);
	ctrl |= RNG_CTRL_EN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RNG_CTRL, ctrl);

	/* set up an rndsource */
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	rndsource_setcb(&sc->sc_rndsource, &bcmrng_get, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	/* get some initial entropy ASAP */
	bcmrng_get(RND_POOLBITS / NBBY, sc);
}

static void
bcmrng_get(size_t bytes_wanted, void *arg)
{
	struct bcm2835rng_softc *sc = arg;
	uint32_t status, cnt;
	uint32_t buf[RNG_DATA_MAX]; /* 1k on the stack */

	mutex_spin_enter(&sc->sc_lock);
	while (bytes_wanted) {
		status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RNG_STATUS);
		cnt = __SHIFTOUT(status, RNG_STATUS_CNT);
		KASSERT(cnt < RNG_DATA_MAX);
		if (cnt == 0)
			continue;	/* XXX Busy-waiting seems wrong...  */
		bus_space_read_multi_4(sc->sc_iot, sc->sc_ioh, RNG_DATA, buf,
		    cnt);
		rnd_add_data_sync(&sc->sc_rndsource, buf, (cnt * 4),
		    (cnt * 4 * NBBY));
		bytes_wanted -= MIN(bytes_wanted, (cnt * 4));
	}
	explicit_memset(buf, 0, sizeof(buf));
	mutex_spin_exit(&sc->sc_lock);
}
