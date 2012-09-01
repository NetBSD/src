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

#define GMAC_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_eth.c,v 1.1 2012/09/01 00:04:44 matt Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static int bcmeth_ccb_match(device_t, cfdata_t, void *);
static void bcmeth_ccb_attach(device_t, device_t, void *);

struct bcmeth_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	kmutex_t *sc_lock;
	kmutex_t *sc_hwlock;
	struct ethercom sc_ec;
#define	sc_if		sc_ec.ec_if;
	void *sc_sih;
	void *sc_ih;
};

static int bcmeth_intr(void *);
static void bcmeth_softint(void *);

static inline uint32_t
bcmeth_read_4(struct bcmeth_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmeth_write_4(struct bcmeth_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

CFATTACH_DECL_NEW(bcmeth_ccb, sizeof(struct bcmeth_softc),
	bcmeth_ccb_match, bcmeth_ccb_attach, NULL, NULL);

static int
bcmeth_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

#ifdef DIAGNOSTIC
	const int port = cf->cf_loc[BCMCCBCF_PORT];
#endif
	KASSERT(port == BCMCCBCF_PORT_DEFAULT || port == loc->loc_port);

	return 1;
}

static void
bcmeth_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmeth_softc * const sc = device_private(self);
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	sc->sc_dev = self;
	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_SOFTNET);
	sc->sc_hwlock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	sc->sc_dmat = ccbaa->ccbaa_dmat;
	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	bcmeth_write_4(sc, GMAC_INTMASK, 0);	// disable interrupts

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	sc->sc_sih = softint_establish(SOFTINT_MPSAFE | SOFTINT_NET,
	    bcmeth_softint, sc);

	sc->sc_ih = intr_establish(loc->loc_intrs[0], IPL_VM, IST_LEVEL,
	    bcmeth_intr, sc);

	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		     loc->loc_intrs[0]);
	} else {
		aprint_normal_dev(self, "interrupting on irq %d\n",
		     loc->loc_intrs[0]);
	}
}

static void
bcmeth_softint(void *arg)
{
	struct bcmeth_softc * const sc = arg;

	mutex_enter(sc->sc_lock);
	mutex_exit(sc->sc_lock);
}

static int
bcmeth_intr(void *arg)
{
	struct bcmeth_softc * const sc = arg;
	int rv = 0;

	mutex_enter(sc->sc_hwlock);

	uint32_t intstatus = bcmeth_read_4(sc, GMAC_INTSTATUS);
	bcmeth_write_4(sc, GMAC_INTSTATUS, intstatus);

	mutex_exit(sc->sc_hwlock);

	return rv;
}
