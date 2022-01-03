/* $NetBSD: mcommphy.c,v 1.1 2022/01/03 17:18:12 jmcneill Exp $ */

/*
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

/*
 * Motorcomm YT8511C / YT8511H Integrated 10/100/1000 Gigabit Ethernet
 * Transceiver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mcommphy.c,v 1.1 2022/01/03 17:18:12 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/miidevs.h>

#define	MCOMMPHY_OUI			0x000000
#define	MCOMMPHY_MODEL			0x10
#define	MCOMMPHY_REV			0x0a

#define	EXT_REG_ADDR			0x1e
#define	EXT_REG_DATA			0x1f

/* Extended registers */
#define	PHY_CLOCK_GATING_REG		0x0c
#define	 TX_CLK_DELAY_SEL		__BITS(7,4)
#define	 CLK_25M_SEL			__BITS(2,1)
#define	 CLK_25M_SEL_125M		3
#define	 RX_CLK_DELAY_EN		__BIT(0)
#define	PHY_SLEEP_CONTROL1_REG		0x27
#define	 PLLON_IN_SLP			__BIT(14)

static int	mcommphymatch(device_t, cfdata_t, void *);
static void	mcommphyattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(mcommphy, sizeof(struct mii_softc),
    mcommphymatch, mcommphyattach, mii_phy_detach, mii_phy_activate);

static int	mcommphy_service(struct mii_softc *, struct mii_data *, int);

static const struct mii_phy_funcs mcommphy_funcs = {
	mcommphy_service, ukphy_status, mii_phy_reset,
};

static int
mcommphymatch(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	/*
	 * The YT8511C reports an OUI of 0. Best we can do here is to match
	 * exactly the contents of the PHY identification registers.
	 */
	if (MII_OUI(ma->mii_id1, ma->mii_id2) == MCOMMPHY_OUI &&
	    MII_MODEL(ma->mii_id2) == MCOMMPHY_MODEL &&
	    MII_REV(ma->mii_id2) == MCOMMPHY_REV) {
		return 10;
	}

	return 0;
}

static void
mcommphyattach(device_t parent, device_t self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	uint16_t oldaddr, data;

	aprint_normal(": Motorcomm YT8511 GbE PHY\n");
	aprint_naive(": Media interface\n");

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_funcs = &mcommphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;

	mii_lock(mii);

	PHY_RESET(sc);

	PHY_READ(sc, EXT_REG_ADDR, &oldaddr);

	PHY_WRITE(sc, EXT_REG_ADDR, PHY_CLOCK_GATING_REG);
	PHY_READ(sc, EXT_REG_DATA, &data);
	data &= ~CLK_25M_SEL;
	data |= __SHIFTIN(CLK_25M_SEL_125M, CLK_25M_SEL);
	if (ISSET(sc->mii_flags, MIIF_RXID)) {
		data |= RX_CLK_DELAY_EN;
	} else {
		data &= ~RX_CLK_DELAY_EN;
	}
	data &= ~TX_CLK_DELAY_SEL;
	if (ISSET(sc->mii_flags, MIIF_TXID)) {
		data |= __SHIFTIN(0xf, TX_CLK_DELAY_SEL);
	} else {
		data |= __SHIFTIN(0x2, TX_CLK_DELAY_SEL);
	}
	PHY_WRITE(sc, EXT_REG_DATA, data);

	PHY_WRITE(sc, EXT_REG_ADDR, PHY_SLEEP_CONTROL1_REG);
	PHY_READ(sc, EXT_REG_DATA, &data);
	data |= PLLON_IN_SLP;
	PHY_WRITE(sc, EXT_REG_DATA, data);

	PHY_WRITE(sc, EXT_REG_ADDR, oldaddr);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static int
mcommphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;

	KASSERT(mii_locked(mii));

	switch (cmd) {
	case MII_POLLSTAT:
		/* If we're not polling our PHY instance, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;
		break;

	case MII_MEDIACHG:
		/* If the interface is not up, don't do anything. */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			break;

		mii_phy_setmedia(sc);
		break;

	case MII_TICK:
		/* If we're not currently selected, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;

		if (mii_phy_tick(sc) == EJUSTRETURN)
			return 0;
		break;

	case MII_DOWN:
		mii_phy_down(sc);
		return 0;
	}

	/* Update the media status. */
	mii_phy_status(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return 0;
}
