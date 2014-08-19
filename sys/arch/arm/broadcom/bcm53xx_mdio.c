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

#define MII_PRIVATE

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: bcm53xx_mdio.c,v 1.1.2.1 2014/08/20 00:02:45 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>

#include <arm/broadcom/bcm53xx_reg.h>
#include <arm/broadcom/bcm53xx_var.h>

static int bcmmdio_ccb_match(device_t, cfdata_t, void *);
static void bcmmdio_ccb_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcmmdio_ccb, sizeof(struct bcmmdio_softc),
	bcmmdio_ccb_match, bcmmdio_ccb_attach, NULL, NULL);

static inline uint32_t
bcmmdio_read_4(struct bcmmdio_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
bcmmdio_write_4(struct bcmmdio_softc *sc, bus_size_t o, uint32_t v)
{
	bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static int
bcmmdio_ccb_match(device_t parent, cfdata_t cf, void *aux)
{
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	if (strcmp(cf->cf_name, loc->loc_name))
		return 0;

	KASSERT(cf->cf_loc[BCMCCBCF_PORT] == BCMCCBCF_PORT_DEFAULT);

	return 1;
}

static void
bcmmdio_ccb_attach(device_t parent, device_t self, void *aux)
{
	struct bcmmdio_softc * const sc = device_private(self);
	struct bcmccb_attach_args * const ccbaa = aux;
	const struct bcm_locators * const loc = &ccbaa->ccbaa_loc;

	sc->sc_dev = self;

	sc->sc_bst = ccbaa->ccbaa_ccb_bst;
	sc->sc_lock = mutex_obj_alloc(MUTEX_DEFAULT, IPL_VM);

	bus_space_subregion(sc->sc_bst, ccbaa->ccbaa_ccb_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	uint32_t miimgt = bcmmdio_read_4(sc, MIIMGT);
	uint32_t div = __SHIFTOUT(miimgt, MIIMGT_MDCDIV);
	if (div == 0) {
		div = 33;			// divisor to get 2MHz
		miimgt &= ~MIIMGT_MDCDIV;
		miimgt |= __SHIFTIN(div, MIIMGT_MDCDIV);
		bcmmdio_write_4(sc, MIIMGT, miimgt);
	}
	uint32_t freq = 66000000 / div;

	aprint_naive("\n");
	aprint_normal(": MDIO bus @ %u MHz\n", freq / 1000000);
}

static void
bcmmdio_select_bus(struct bcmmdio_softc *sc, int phy)
{
	uint32_t mgt = bcmmdio_read_4(sc, MIIMGT);
	uint32_t new_mgt = mgt;

	KASSERT((mgt & MIIMGT_BSY) == 0);
	if (__BIT(phy) & MII_INTERNAL) {
		new_mgt &= ~MIIMGT_EXT;
	} else {
		new_mgt |= MIIMGT_EXT;
	}

	if (mgt != new_mgt)
		bcmmdio_write_4(sc, MIIMGT, new_mgt);
}

static void
bcmmdio_busywait(struct bcmmdio_softc *sc)
{
	while (bcmmdio_read_4(sc, MIIMGT) & MIIMGT_BSY) {
		delay(1);
	}
}

int
bcmmdio_mii_readreg(device_t self, int phy, int reg)
{
	struct bcmmdio_softc * const sc = device_private(self);

	mutex_enter(sc->sc_lock);

	bcmmdio_select_bus(sc, phy);

	bcmmdio_write_4(sc, MIICMD, MIICMD_RD(phy, reg));

	bcmmdio_busywait(sc);

	int val = __SHIFTOUT(bcmmdio_read_4(sc, MIICMD), MIICMD_DATA);
	mutex_exit(sc->sc_lock);
	return val;
}

void
bcmmdio_mii_writereg(device_t self, int phy, int reg, int val)
{
	struct bcmmdio_softc * const sc = device_private(self);

	KASSERT((val & 0xffff) == val);

	mutex_enter(sc->sc_lock);

	bcmmdio_select_bus(sc, phy);

	bcmmdio_write_4(sc, MIICMD, MIICMD_WR(phy, reg, val));

	bcmmdio_busywait(sc);

	mutex_exit(sc->sc_lock);
}
