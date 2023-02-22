/*	$NetBSD: atphy.c,v 1.31 2023/02/22 08:09:09 msaitoh Exp $ */
/*	$OpenBSD: atphy.c,v 1.1 2008/09/25 20:47:16 brad Exp $	*/

/*-
 * Copyright (c) 2008, Pyun YongHyeon <yongari@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Driver for the Attansic F1 10/100/1000 PHY.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: atphy.c,v 1.31 2023/02/22 08:09:09 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/miidevs.h>

/* Special Control Register */
#define ATPHY_SCR			0x10
#define ATPHY_SCR_JABBER_DISABLE	0x0001
#define ATPHY_SCR_POLARITY_REVERSAL	0x0002
#define ATPHY_SCR_SQE_TEST		0x0004
#define ATPHY_SCR_MAC_PDOWN		0x0008
#define ATPHY_SCR_CLK125_DISABLE	0x0010
#define ATPHY_SCR_MDI_MANUAL_MODE	0x0000
#define ATPHY_SCR_MDIX_MANUAL_MODE	0x0020
#define ATPHY_SCR_AUTO_X_1000T		0x0040
#define ATPHY_SCR_AUTO_X_MODE		0x0060
#define ATPHY_SCR_10BT_EXT_ENABLE	0x0080
#define ATPHY_SCR_MII_5BIT_ENABLE	0x0100
#define ATPHY_SCR_SCRAMBLER_DISABLE	0x0200
#define ATPHY_SCR_FORCE_LINK_GOOD	0x0400
#define ATPHY_SCR_ASSERT_CRS_ON_TX	0x0800

/* Special Status Register. */
#define ATPHY_SSR			0x11
#define ATPHY_SSR_SPD_DPLX_RESOLVED	0x0800
#define ATPHY_SSR_DUPLEX		0x2000
#define ATPHY_SSR_SPEED_MASK		0xC000
#define ATPHY_SSR_10MBS			0x0000
#define ATPHY_SSR_100MBS		0x4000
#define ATPHY_SSR_1000MBS		0x8000

#define ATPHY_DEBUG_PORT_ADDR		0x1d
#define ATPHY_DEBUG_PORT_DATA		0x1e
#define ATPHY_RGMII_RX_CLK_DLY		__BIT(15)
#define ATPHY_RGMII_TX_CLK_DLY		__BIT(8)

static int atphy_match(device_t, cfdata_t, void *);
static void atphy_attach(device_t, device_t, void *);

static int atphy_service(struct mii_softc *, struct mii_data *, int);
static void atphy_reset(struct mii_softc *);
static void atphy_status(struct mii_softc *);
static int atphy_mii_phy_auto(struct mii_softc *);
static bool atphy_is_gige(const struct mii_phydesc *);

struct atphy_softc {
	struct mii_softc mii_sc;
	int mii_clk_25m;
	bool rgmii_tx_internal_delay;
	bool rgmii_rx_internal_delay;
};

CFATTACH_DECL_NEW(atphy, sizeof(struct atphy_softc),
	atphy_match, atphy_attach, mii_phy_detach, mii_phy_activate);

const struct mii_phy_funcs atphy_funcs = {
	atphy_service, atphy_status, atphy_reset,
};

static const struct mii_phydesc atphys[] = {
	MII_PHY_DESC(ATTANSIC, L1),
	MII_PHY_DESC(ATTANSIC, L2),
	MII_PHY_DESC(ATTANSIC, AR8021),
	MII_PHY_DESC(ATTANSIC, AR8035),
	MII_PHY_END,
};

static void
atphy_clk_25m(struct atphy_softc *asc)
{
	struct mii_softc *sc = &asc->mii_sc;
	struct {
		uint32_t hz;
		uint16_t data;
	} select_clk[] = {
		{  25000000, 0x0 },
		{  50000000, 0x1 },
		{  62500000, 0x2 },
		{ 125000000, 0x3 }
	};
	uint16_t data = 0;
	uint16_t reg = 0;

	for (int i = 0; i < __arraycount(select_clk); i++) {
		if (asc->mii_clk_25m <= select_clk[i].hz)
			data = select_clk[i].data;
	}

	PHY_WRITE(sc, 0x0d, 0x0007);
	PHY_WRITE(sc, 0x0e, 0x8016);
	PHY_WRITE(sc, 0x0d, 0x4007);
	PHY_READ(sc, 0x0e, &reg);
	PHY_WRITE(sc, 0x0e, reg | __SHIFTIN(data, __BITS(4, 3)));
}


static bool
atphy_is_gige(const struct mii_phydesc *mpd)
{
	switch (mpd->mpd_oui) {
	case MII_OUI_ATTANSIC:
		switch (mpd->mpd_model) {
		case MII_MODEL_ATTANSIC_L2:
			return false;
		}
	}

	return true;
}

static int
atphy_match(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, atphys) != NULL)
		return 10;

	return 0;
}

void
atphy_attach(device_t parent, device_t self, void *aux)
{
	struct atphy_softc *asc = device_private(self);
	prop_dictionary_t parent_prop = device_properties(parent);
	prop_dictionary_t prop = device_properties(self);
	struct mii_softc *sc = &asc->mii_sc;
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;
	uint16_t bmsr;

	mpd = mii_phy_match(ma, atphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_funcs = &atphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_flags |= MIIF_NOLOOP;

	prop_dictionary_get_bool(parent_prop, "tx_internal_delay",
	    &asc->rgmii_tx_internal_delay);
	prop_dictionary_get_bool(parent_prop, "rx_internal_delay",
	    &asc->rgmii_rx_internal_delay);

	prop_dictionary_get_uint32(prop, "clk_25m", &asc->mii_clk_25m);
	if (asc->mii_clk_25m != 0)
		atphy_clk_25m(asc);

	mii_lock(mii);

	PHY_RESET(sc);

	PHY_READ(sc, MII_BMSR, &bmsr);
	PHY_READ(sc, MII_BMSR, &bmsr);
	sc->mii_capabilities = bmsr & ma->mii_capmask;
	if (atphy_is_gige(mpd) && (sc->mii_capabilities & BMSR_EXTSTAT))
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

int
atphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t anar, bmcr, bmsr;

	KASSERT(mii_locked(mii));

	switch (cmd) {
	case MII_POLLSTAT:
		/* If we're not polling our PHY instance, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;
		break;

	case MII_MEDIACHG:
		/*
		 * If the media indicates a different PHY instance,
		 * isolate ourselves.
		 */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst) {
			PHY_READ(sc, MII_BMCR, &bmcr);
			PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_ISO);
			return 0;
		}

		/* If the interface is not up, don't do anything. */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			break;

		bmcr = 0;
		switch (IFM_SUBTYPE(ife->ifm_media)) {
		case IFM_AUTO:
		case IFM_1000_T:
			atphy_mii_phy_auto(sc);
			goto done;
		case IFM_100_TX:
			bmcr = BMCR_S100;
			break;
		case IFM_10_T:
			bmcr = BMCR_S10;
			break;
		case IFM_NONE:
			PHY_READ(sc, MII_BMCR, &bmcr);
			/*
			 * XXX
			 * Due to an unknown reason powering down PHY resulted
			 * in unexpected results such as inaccessibility of
			 * hardware of freshly rebooted system. Disable
			 * powering down PHY until I got more information for
			 * Attansic/Atheros PHY hardwares.
			 */
			PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_ISO);
			goto done;
		default:
			return EINVAL;
		}

		anar = mii_anar(ife);
		if ((ife->ifm_media & IFM_FDX) != 0) {
			bmcr |= BMCR_FDX;
			/* Enable pause. */
			if (sc->mii_flags & MIIF_DOPAUSE)
				anar |= ANAR_PAUSE_TOWARDS;
		}

		if ((sc->mii_extcapabilities & (EXTSR_1000TFDX |
		    EXTSR_1000THDX)) != 0)
			PHY_WRITE(sc, MII_100T2CR, 0);
		PHY_WRITE(sc, MII_ANAR, anar);

		/* Start autonegotiation. */
		PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_AUTOEN | BMCR_STARTNEG);
done:
		break;

	case MII_TICK:
		/* If we're not currently selected, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;

		/* Is the interface even up? */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			return 0;

		/* Only used for autonegotiation. */
		if ((IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO) &&
		    (IFM_SUBTYPE(ife->ifm_media) != IFM_1000_T)) {
			sc->mii_ticks = 0;
			break;
		}

		/*
		 * Check for link.
		 * Read the status register twice; BMSR_LINK is latch-low.
		 */
		PHY_READ(sc, MII_BMSR, &bmsr);
		PHY_READ(sc, MII_BMSR, &bmsr);
		if (bmsr & BMSR_LINK) {
			sc->mii_ticks = 0;
			break;
		}

		/* Announce link loss right after it happens. */
		if (sc->mii_ticks++ == 0)
			break;

		/* Only retry autonegotiation every mii_anegticks seconds. */
		if (sc->mii_ticks < sc->mii_anegticks)
			break;

		atphy_mii_phy_auto(sc);
		break;
	}

	/* Update the media status. */
	mii_phy_status(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return 0;
}

static void
atphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	uint16_t bmsr, bmcr, gsr, ssr;

	KASSERT(mii_locked(mii));

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	PHY_READ(sc, MII_BMSR, &bmsr);
	PHY_READ(sc, MII_BMSR, &bmsr);
	if (bmsr & BMSR_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	PHY_READ(sc, MII_BMCR, &bmcr);
	if (bmcr & BMCR_ISO) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	PHY_READ(sc, ATPHY_SSR, &ssr);
	if (!(ssr & ATPHY_SSR_SPD_DPLX_RESOLVED)) {
		/* Erg, still trying, I guess... */
		mii->mii_media_active |= IFM_NONE;
		return;
	}

	switch (ssr & ATPHY_SSR_SPEED_MASK) {
	case ATPHY_SSR_1000MBS:
		mii->mii_media_active |= IFM_1000_T;
		/*
		 * atphy(4) has a valid link so reset mii_ticks.
		 * Resetting mii_ticks is needed in order to
		 * detect link loss after auto-negotiation.
		 */
		sc->mii_ticks = 0;
		break;
	case ATPHY_SSR_100MBS:
		mii->mii_media_active |= IFM_100_TX;
		sc->mii_ticks = 0;
		break;
	case ATPHY_SSR_10MBS:
		mii->mii_media_active |= IFM_10_T;
		sc->mii_ticks = 0;
		break;
	default:
		mii->mii_media_active |= IFM_NONE;
		return;
	}

	if (ssr & ATPHY_SSR_DUPLEX)
		mii->mii_media_active |= IFM_FDX | mii_phy_flowstatus(sc);
	else
		mii->mii_media_active |= IFM_HDX;

	PHY_READ(sc, MII_100T2SR, &gsr);
	if ((IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) &&
	    gsr & GTSR_MS_RES)
		mii->mii_media_active |= IFM_ETH_MASTER;
}

static void
atphy_reset(struct mii_softc *sc)
{
	struct atphy_softc *asc = (struct atphy_softc *)sc;
	uint16_t reg;
	int i;

	KASSERT(mii_locked(sc->mii_pdata));

	/*
	 * Take PHY out of power down mode.
	 *
	 * XXX AR8021 document has no description about the power saving
	 * control register. Shouldn't we write it?
	 */
	PHY_WRITE(sc, 29, 0x29);
	/*
	 * XXX AR8031 document says the lower 14 bits are reserved and the
	 * default value is 0x36d0. Shouldn't we clear those bits?
	 * I have no document neither L1(F1) nor L2(F2).
	 */
	PHY_WRITE(sc, 30, 0);

	if ((sc->mii_mpd_model == MII_MODEL_ATTANSIC_L2)
	    && (sc->mii_mpd_rev == 1)) {
		/*
		 * On NVIDIA MCP61 with Attansic L2 rev. 1, changing debug
		 * port 0x29's value makes the next PHY read fail with error.
		 * This is observed on ASUS M2N-MX SE Plus. Read any register
		 * to ignore this problem.
		 */
		(void)PHY_READ(sc, ATPHY_SCR, &reg);
	}
	PHY_READ(sc, ATPHY_SCR, &reg);
	/* Enable automatic crossover. */
	reg |= ATPHY_SCR_AUTO_X_MODE;
	/* Disable power down. */
	reg &= ~ATPHY_SCR_MAC_PDOWN;
	/* Enable CRS on Tx. */
	reg |= ATPHY_SCR_ASSERT_CRS_ON_TX;
	/* Auto correction for reversed cable polarity. */
	reg |= ATPHY_SCR_POLARITY_REVERSAL;
	PHY_WRITE(sc, ATPHY_SCR, reg);

	atphy_mii_phy_auto(sc);

	/* Workaround F1 bug to reset phy. */
	PHY_READ(sc, MII_BMCR, &reg);
	reg |= BMCR_RESET;
	PHY_WRITE(sc, MII_BMCR, reg);

	for (i = 0; i < 1000; i++) {
		DELAY(1);
		PHY_READ(sc, MII_BMCR, &reg);
		if ((reg & BMCR_RESET) == 0)
			break;
	}

	if (asc->rgmii_tx_internal_delay) {
		PHY_WRITE(sc, ATPHY_DEBUG_PORT_ADDR, 0x05);
		PHY_WRITE(sc, ATPHY_DEBUG_PORT_DATA, ATPHY_RGMII_TX_CLK_DLY);
	}
	if (asc->rgmii_rx_internal_delay) {
		PHY_WRITE(sc, ATPHY_DEBUG_PORT_ADDR, 0x00);
		PHY_WRITE(sc, ATPHY_DEBUG_PORT_DATA, ATPHY_RGMII_RX_CLK_DLY);
	}
}

static int
atphy_mii_phy_auto(struct mii_softc *sc)
{
	uint16_t anar;

	KASSERT(mii_locked(sc->mii_pdata));

	sc->mii_ticks = 0;
	anar = BMSR_MEDIA_TO_ANAR(sc->mii_capabilities) | ANAR_CSMA;
	if (sc->mii_flags & MIIF_DOPAUSE)
		anar |= ANAR_PAUSE_TOWARDS;
	PHY_WRITE(sc, MII_ANAR, anar);
	if (sc->mii_extcapabilities & (EXTSR_1000TFDX | EXTSR_1000THDX))
		PHY_WRITE(sc, MII_100T2CR, GTCR_ADV_1000TFDX |
		    GTCR_ADV_1000THDX);
	PHY_WRITE(sc, MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);

	return EJUSTRETURN;
}
