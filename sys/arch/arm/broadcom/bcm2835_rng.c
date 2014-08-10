/*	$NetBSD: bcm2835_rng.c,v 1.10 2014/08/10 16:44:33 tls Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_rng.c,v 1.10 2014/08/10 16:44:33 tls Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/rnd.h>
#include <sys/atomic.h>
#include <sys/intr.h>

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

	kmutex_t		sc_intr_lock;
	unsigned int		sc_bytes_wanted;
	void 			*sc_sih;

	kmutex_t		sc_rnd_lock;
	krndsource_t		sc_rndsource;
};

static int bcmrng_match(device_t, cfdata_t, void *);
static void bcmrng_attach(device_t, device_t, void *);
static void bcmrng_get(struct bcm2835rng_softc *);
static void bcmrng_get_cb(size_t, void *);
static void bcmrng_get_intr(void *);

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
		goto fail0;
	}

	/* discard initial numbers, broadcom says they are "less random" */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RNG_STATUS, 0x40000);

	/* enable rng */
	ctrl = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RNG_CTRL);
	ctrl |= RNG_CTRL_EN;
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, RNG_CTRL, ctrl);

	/* set up a softint for adding data */
	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_SERIAL);
	sc->sc_bytes_wanted = 0;
	sc->sc_sih = softint_establish(SOFTINT_SERIAL|SOFTINT_MPSAFE,
	    &bcmrng_get_intr, sc);
	if (sc->sc_sih == NULL) {
		aprint_error_dev(sc->sc_dev, "unable to establish softint");
		goto fail1;
	}

	/* set up an rndsource */
	mutex_init(&sc->sc_rnd_lock, MUTEX_DEFAULT, IPL_SERIAL);
	rndsource_setcb(&sc->sc_rndsource, &bcmrng_get_cb, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);

	/* get some initial entropy ASAP */
	bcmrng_get_cb(RND_POOLBITS / NBBY, sc);

	/* Success!  */
	return;

fail1:	mutex_destroy(&sc->sc_intr_lock);
	bus_space_unmap(aaa->aaa_iot, sc->sc_ioh, BCM2835_RNG_SIZE);
fail0:	return;
}

static void
bcmrng_get(struct bcm2835rng_softc *sc)
{
	uint32_t status, cnt;
	uint32_t buf[RNG_DATA_MAX]; /* 1k on the stack */

	mutex_spin_enter(&sc->sc_intr_lock);
	while (sc->sc_bytes_wanted) {
		status = bus_space_read_4(sc->sc_iot, sc->sc_ioh, RNG_STATUS);
		cnt = __SHIFTOUT(status, RNG_STATUS_CNT);
		KASSERT(cnt < RNG_DATA_MAX);
		if (cnt == 0)
			continue;	/* XXX Busy-waiting seems wrong...  */
		bus_space_read_multi_4(sc->sc_iot, sc->sc_ioh, RNG_DATA, buf,
		    cnt);

		/*
		 * This lock dance is necessary because rnd_add_data
		 * may call bcmrng_get_cb which takes the intr lock.
		 */
		mutex_spin_exit(&sc->sc_intr_lock);
		mutex_spin_enter(&sc->sc_rnd_lock);
		rnd_add_data(&sc->sc_rndsource, buf, (cnt * 4),
		    (cnt * 4 * NBBY));
		mutex_spin_exit(&sc->sc_rnd_lock);
		mutex_spin_enter(&sc->sc_intr_lock);
		sc->sc_bytes_wanted -= MIN(sc->sc_bytes_wanted, (cnt * 4));
	}
	explicit_memset(buf, 0, sizeof(buf));
	mutex_spin_exit(&sc->sc_intr_lock);
}

static void
bcmrng_get_cb(size_t bytes_wanted, void *arg)
{
	struct bcm2835rng_softc *sc = arg;

	/*
	 * Deferring to a softint is necessary until the rnd(9) locking
	 * is fixed.
	 */
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
bcmrng_get_intr(void *arg)
{
	struct bcm2835rng_softc *const sc = arg;

	bcmrng_get(sc);
}
