/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#define RNG_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_rng.c,v 1.1.2.4 2017/12/03 11:35:52 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/rndsource.h>
#include <sys/systm.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static int bcmrng_ccb_match(device_t, cfdata_t, void *);
static void bcmrng_ccb_attach(device_t, device_t, void *);

struct bcmrng_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	krndsource_t sc_rnd_source;
	struct callout sc_rnd_callout;
	kmutex_t *sc_lock;
#ifdef RNG_USE_INTR
	size_t sc_intr_amount;
	void *sc_ih;
#endif
};

static void bcmrng_callout(void *arg);
static size_t bcmrng_empty(struct bcmrng_softc *);
#ifdef RNG_USE_INTR
static int bcmrng_intr(void *);
#endif

CFATTACH_DECL_NEW(bcmrng_ccb, sizeof(struct bcmrng_softc),
	bcmrng_ccb_match, bcmrng_ccb_attach, NULL, NULL);

static int
bcmrng_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	KASSERT(cf->cf_loc[BCMCCBCF_PORT] == BCMCCBCF_PORT_DEFAULT);

	return 1;
}

static inline uint32_t
bcmrng_read_4(struct bcmrng_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmrng_read_multi_4(struct bcmrng_softc *sc, bus_size_t o, uint32_t *p,
    size_t c)
{
	return bus_space_read_multi_4(sc->sc_bst, sc->sc_bsh, o, p, c);
}

__unused static inline void
bcmrng_write_4(struct bcmrng_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

/*
 * To be called from initarm for an early start.
 */
void
bcm53xx_rng_start(bus_space_tag_t bst, bus_space_handle_t bsh)
{
	/*
	 * Disable the interrupt and enable the RNG.
	 */
	bus_space_write_4(bst, bsh, CCB_BASE + RNG_BASE + RNG_INT_MASK, RNG_INT_OFF);
	bus_space_write_4(bst, bsh, CCB_BASE + RNG_BASE + RNG_CTRL,
	    bus_space_read_4(bst, bsh, CCB_BASE + RNG_BASE + RNG_CTRL) | RNG_RBGEN);
}

static void
bcmrng_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmrng_softc * const sc = device_private(self);
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	sc->sc_dev = self;

	/* make sure it's running */
	bcm53xx_rng_start(ccbaa->ccbaa_ccb_bst, ccbaa->ccbaa_ccb_bsh);

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": Random Number Generator (%u bit FIFO)\n",
	    32 * bcmrng_read_4(sc, RNG_FF_THRESHOLD));

	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTCLOCK);

	callout_init(&sc->sc_rnd_callout, CALLOUT_MPSAFE);

	rnd_attach_source(&sc->sc_rnd_source, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE);

#ifdef RNG_USE_INTR
	sc->sc_ih = intr_establish(loc->loc_intrs[0], IPL_VM, IST_LEVEL,
	    bcmrng_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intrs[0]);
	} else {
		aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intrs[0]);
	}
#else
	bcmrng_callout(sc);
#endif
}

size_t
bcmrng_empty(struct bcmrng_softc *sc)
{
	mutex_enter(sc->sc_lock);
	uint32_t data[128];
	size_t nwords = __SHIFTOUT(bcmrng_read_4(sc, RNG_STATUS), RNG_VAL);
	if (nwords == 0) {
		mutex_exit(sc->sc_lock);
		return 0;
	}
	if (nwords > __arraycount(data))
		nwords = __arraycount(data);

	bcmrng_read_multi_4(sc, RNG_DATA, data, nwords);

	rnd_add_data(&sc->sc_rnd_source, data, sizeof(data), sizeof(data) * 8);
	mutex_exit(sc->sc_lock);
	//aprint_debug_dev(sc->sc_dev, "added %zu words\n", nwords);
	return nwords;
}

void
bcmrng_callout(void *arg)
{
	struct bcmrng_softc * const sc = arg;

	bcmrng_empty(sc);

	callout_reset(&sc->sc_rnd_callout, 1, bcmrng_callout, sc);
}

#ifdef RNG_USE_INTR
int
bcmrng_intr(void *arg)
{
	struct bcmrng_softc * const sc = arg;

	sc->sc_intr_amount += bcmrng_empty(sc);
	if (sc->sc_intr_amount >= 0x10000) {
		bcmrng_callout(sc);
		bcmrng_write_4(sc, RNG_INT_MASK, RNG_INT_OFF);
	}
	return 1;
}
#endif
