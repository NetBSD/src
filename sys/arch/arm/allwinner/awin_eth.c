/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_eth.c,v 1.3 2013/09/08 04:06:44 matt Exp $");

#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/mutex.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int awin_eth_match(device_t, cfdata_t, void *);
static void awin_eth_attach(device_t, device_t, void *);

static int awin_eth_miibus_read_reg(device_t, int, int);
static void awin_eth_miibus_write_reg(device_t, int, int, int);
static void awin_eth_miibus_statchg(struct ifnet *);

struct awin_eth_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	struct ethercom sc_ec;
	struct mii_data sc_mii;
	kmutex_t sc_mdio_lock;
};

CFATTACH_DECL_NEW(awin_eth, sizeof(struct awin_eth_softc),
	awin_eth_match, awin_eth_attach, NULL, NULL);

static const struct awin_gpio_pinset awin_eth_pinsets[2] = {
        [0] = { 'A', AWIN_PIO_PA_EMAC_FUNC, AWIN_PIO_PA_EMAC_PINS },
        [1] = { 'H', AWIN_PIO_PH_EMAC_FUNC, AWIN_PIO_PH_EMAC_PINS },
};

static inline uint32_t
awin_eth_read_4(struct awin_eth_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
awin_eth_write_4(struct awin_eth_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static int
awin_eth_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
#ifdef DIAGNOSTIC
	const struct awin_locators * const loc = &aio->aio_loc;
#endif
	const struct awin_gpio_pinset * const pinset =
	    &awin_eth_pinsets[cf->cf_flags & 1];

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);

	if (!awin_gpio_pinset_available(pinset))
		return 0;

	return 1;
}

static void
awin_eth_attach(device_t parent, device_t self, void *aux)
{
	cfdata_t cf = device_cfdata(self);
	struct awin_eth_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const struct awin_gpio_pinset * const pinset =
	    &awin_eth_pinsets[cf->cf_flags & 1];
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct mii_data * const mii = &sc->sc_mii;

	sc->sc_dev = self;

	awin_gpio_pinset_acquire(pinset);

	mutex_init(&sc->sc_mdio_lock, MUTEX_DEFAULT, IPL_NET);

	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": 10/100 Ethernet Controller\n");

	ifp->if_softc = sc;

	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);

        mii->mii_ifp = ifp;
        mii->mii_readreg = awin_eth_miibus_read_reg;
        mii->mii_writereg = awin_eth_miibus_write_reg;
        mii->mii_statchg = awin_eth_miibus_statchg;

	int mii_flags = 0;

        mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, mii_flags);
                
        if (LIST_EMPTY(&mii->mii_phys)) { 
                aprint_error_dev(self, "no PHY found!\n");
                ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
                ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_MANUAL);
        } else {
                ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);
        }
}

int
awin_eth_miibus_read_reg(device_t self, int phy, int reg)
{
	struct awin_eth_softc * const sc = device_private(self);

	mutex_enter(&sc->sc_mdio_lock);

	awin_eth_write_4(sc, AWIN_EMAC_MAC_MADR_REG, (phy << 8) | reg);
	awin_eth_write_4(sc, AWIN_EMAC_MAC_MCMD_REG, 1);

	delay(100);

	awin_eth_write_4(sc, AWIN_EMAC_MAC_MCMD_REG, 0);
	const uint32_t rv = awin_eth_read_4(sc, AWIN_EMAC_MAC_MRDD_REG);

	mutex_exit(&sc->sc_mdio_lock);

	return rv;
}

void
awin_eth_miibus_write_reg(device_t self, int phy, int reg, int val)
{
	struct awin_eth_softc * const sc = device_private(self);

	mutex_enter(&sc->sc_mdio_lock);

	awin_eth_write_4(sc, AWIN_EMAC_MAC_MADR_REG, (phy << 8) | reg);
	awin_eth_write_4(sc, AWIN_EMAC_MAC_MCMD_REG, 1);

	delay(100);

	awin_eth_write_4(sc, AWIN_EMAC_MAC_MCMD_REG, 0);
	awin_eth_write_4(sc, AWIN_EMAC_MAC_MWTD_REG, val);

	mutex_exit(&sc->sc_mdio_lock);
}

void
awin_eth_miibus_statchg(struct ifnet *ifp)
{
	struct awin_eth_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;

	/*
	 * Set MII or GMII interface based on the speed
	 * negotiated by the PHY.                                           
	 */                                                                 
	switch (IFM_SUBTYPE(mii->mii_media_active)) {
	case IFM_10_T:
	case IFM_100_TX:
		;
	}
}
