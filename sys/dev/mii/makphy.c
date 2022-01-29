/*	$NetBSD: makphy.c,v 1.60.2.3 2022/01/29 16:54:42 martin Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for the Marvell 88E1000 ``Alaska'' 10/100/1000 PHY.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: makphy.c,v 1.60.2.3 2022/01/29 16:54:42 martin Exp $");

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

#include <dev/mii/makphyreg.h>
#include <dev/mii/makphyvar.h>

static int	makphymatch(device_t, cfdata_t, void *);
static void	makphyattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(makphy, sizeof(struct makphy_softc),
    makphymatch, makphyattach, mii_phy_detach, mii_phy_activate);

static int	makphy_service(struct mii_softc *, struct mii_data *, int);
static void	makphy_status(struct mii_softc *);
static void	makphy_reset(struct mii_softc *);

static const struct mii_phy_funcs makphy_funcs = {
	makphy_service, makphy_status, makphy_reset,
};

static const struct mii_phydesc makphys[] = {
	MII_PHY_DESC(MARVELL, E1000_0),
	MII_PHY_DESC(MARVELL, E1000_3),
	MII_PHY_DESC(MARVELL, E1000_5),
	MII_PHY_DESC(MARVELL, E1000_6),
	MII_PHY_DESC(xxMARVELL, E1000_3),
	MII_PHY_DESC(xxMARVELL, E1000_5),
	MII_PHY_DESC(xxMARVELL, E1000S),
	MII_PHY_DESC(xxMARVELL, E1011),
	MII_PHY_DESC(xxMARVELL, E1101),
	MII_PHY_DESC(xxMARVELL, E1111),
	MII_PHY_DESC(xxMARVELL, E1112),
	MII_PHY_DESC(xxMARVELL, E1116),
	MII_PHY_DESC(xxMARVELL, E1116R),
	MII_PHY_DESC(xxMARVELL, E1118),
	MII_PHY_DESC(xxMARVELL, E1145),
	MII_PHY_DESC(xxMARVELL, E1149),
	MII_PHY_DESC(xxMARVELL, E1149R),
	MII_PHY_DESC(xxMARVELL, E1240),
	MII_PHY_DESC(xxMARVELL, E1318S),
	MII_PHY_DESC(xxMARVELL, E1512),
	MII_PHY_DESC(xxMARVELL, E1543),
	MII_PHY_DESC(xxMARVELL, E3016),
	MII_PHY_DESC(xxMARVELL, E3082),
	MII_PHY_DESC(xxMARVELL, PHYG65G),
	MII_PHY_END,
};

#define MAKARG_PDOWN	true	/* Power DOWN */
#define MAKARG_PUP	false	/* Power UP */

static bool
makphy_isi210(device_t parent, struct mii_attach_args *ma)
{

	/* I21[01]'s model number is 0 */
	if ((MII_OUI(ma->mii_id1, ma->mii_id2) == MII_OUI_xxMARVELL)
	    && (MII_MODEL(ma->mii_id2) == MII_MODEL_xxMARVELL_I210)
	    && (device_is_a(parent, "wm")))
		return true;
	return false;
}

static int
makphymatch(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, makphys) != NULL)
		return 10;

	if (makphy_isi210(parent, ma))
		return 10;

	return 0;
}

static void
makphyattach(device_t parent, device_t self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;
	struct makphy_softc *maksc = (struct makphy_softc *)sc;
	const char *name;
	uint16_t reg, model;

	mpd = mii_phy_match(ma, makphys);
	aprint_naive(": Media interface\n");
	if (mpd)
		name = mpd->mpd_name;
	else if (makphy_isi210(parent, ma)) {
		name = MII_STR_xxMARVELL_I210;
		maksc->sc_flags |= MAKPHY_F_I210;
	} else
		panic("Unknown PHY");
	aprint_normal(": %s, rev. %d\n", name, MII_REV(ma->mii_id2));

	sc->mii_dev = self;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &makphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_anegticks = MII_ANEGTICKS;

	switch (model) {
	case MII_MODEL_xxMARVELL_E1000:
		if ((maksc->sc_flags & MAKPHY_F_I210) != 0)
			goto page0;
		/* FALLTHROUGH */
	case MII_MODEL_xxMARVELL_E1000_3:
	case MII_MODEL_xxMARVELL_E1000S:
	case MII_MODEL_xxMARVELL_E1000_5:
		/* 88E1000 series has no EADR */
		break;
	default:
page0:
		/* Make sure page 0 is selected. */
		if (PHY_WRITE(sc, MAKPHY_EADR, 0) != 0)
			aprint_verbose_dev(self,
			    "Failed to access EADR. Are you an emulator?\n");
		break;
	}

	PHY_RESET(sc);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	if (((sc->mii_extcapabilities & (EXTSR_1000TFDX | EXTSR_1000THDX))
		!= 0)
	    && ((sc->mii_extcapabilities & (EXTSR_1000XFDX | EXTSR_1000XHDX))
		!= 0)) {
		bool fiberonly = false, copperonly = false;

		/* Both copper and fiber are set. check MODE[] */
		switch (sc->mii_mpd_model) {
		case MII_MODEL_xxMARVELL_E1011:
		case MII_MODEL_xxMARVELL_E1111:
			/* These devices have ESSR register */
			PHY_READ(sc, MAKPHY_ESSR, &reg);
			if ((reg & ESSR_AUTOSEL_DISABLE) != 0) {
				switch (reg & ESSR_HWCFG_MODE) {
				case ESSR_RTBI_FIBER:
				case ESSR_RGMII_FIBER:
				case ESSR_RGMII_SGMII: /* right? */
				case ESSR_TBI_FIBER:
				case ESSR_GMII_FIBER:
					fiberonly = true;
					break;
				case ESSR_SGMII_WC_COPPER:
				case ESSR_SGMII_WOC_COPPER:
				case ESSR_RTBI_COPPER:
				case ESSR_RGMII_COPPER:
				case ESSR_GMII_COPPER:
					copperonly = true;
				default:
					break;
				}
			}
			break;
		default:
			break;
		}
		if (fiberonly || copperonly)
			aprint_debug_dev(self, "both copper and fiber are set "
			    "but MODE[] is %s only.\n",
			    fiberonly ? "fiber" : "copper");
		if (fiberonly)
			sc->mii_extcapabilities
			    &= ~(EXTSR_1000TFDX | EXTSR_1000THDX);
		else if (copperonly) {
			sc->mii_extcapabilities
			    &= ~(EXTSR_1000XFDX | EXTSR_1000XHDX);
			sc->mii_flags &= ~MIIF_IS_1000X;
		}
	}

	aprint_normal_dev(self, "");
	if ((sc->mii_capabilities & BMSR_MEDIAMASK) == 0 &&
	    (sc->mii_extcapabilities & EXTSR_MEDIAMASK) == 0)
		aprint_error("no media present");
	else
		mii_phy_add_media(sc);
	aprint_normal("\n");
}

static void
makphy_reset(struct mii_softc *sc)
{
	struct makphy_softc *maksc = (struct makphy_softc *)sc;
	uint16_t reg;

	mii_phy_reset(sc);

	/* Initialize PHY Specific Control Register. */
	PHY_READ(sc, MAKPHY_PSCR, &reg);

	/* Assert CRS on transmit. */
	switch (sc->mii_mpd_model) {
	case MII_MODEL_MARVELL_E1000_0:
		if ((maksc->sc_flags & MAKPHY_F_I210) != 0)
			break;
		/* FALLTHROUGH */
	case MII_MODEL_MARVELL_E1000_3:
	case MII_MODEL_MARVELL_E1000_5:
	case MII_MODEL_MARVELL_E1000_6:
	case MII_MODEL_xxMARVELL_E1000S:
	case MII_MODEL_xxMARVELL_E1011:
	case MII_MODEL_xxMARVELL_E1111:
		reg |= PSCR_CRS_ON_TX;
		break;
	default: /* No PSCR_CRS_ON_TX bit */
		break;
	}

	/* Enable scrambler if necessary. */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E3016)
		reg &= ~E3016_PSCR_SCRAMBLE_DIS;

	/*
	 * Store next page in the Link Partner Next Page register for
	 * compatibility with 802.3ab.
	 */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E3016)
		reg |= E3016_PSCR_REG8NXTPG;

	PHY_WRITE(sc, MAKPHY_PSCR, reg);

	/* Configure LEDs if they were left unconfigured. */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E3016) {
		PHY_READ(sc, 0x16, &reg);
		if (reg == 0) {
			reg = (0x0b << 8) | (0x05 << 4) | 0x04;	/* XXX */
			PHY_WRITE(sc, 0x16, reg);
		}
	}

	mii_phy_reset(sc);
}

static void
makphy_pdown(struct mii_softc *sc, bool pdown)
{
	uint16_t bmcr, new;
	bool need_reset = false;

	/*
	 * XXX
	 * PSCR (register 16) should be modified on some chips.
	 */

	PHY_READ(sc, MII_BMCR, &bmcr);
	if (pdown)
		new = bmcr | BMCR_PDOWN;
	else
		new = bmcr & ~BMCR_PDOWN;
	if (bmcr != new)
		need_reset = true;

	if (need_reset)
		new |= BMCR_RESET;
	PHY_WRITE(sc, MII_BMCR, new);
}

static int
makphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t bmcr;

	if (!device_is_active(sc->mii_dev))
		return ENXIO;

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

		/* Try to power up the PHY in case it's down */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_NONE)
			makphy_pdown(sc, MAKARG_PUP);

		mii_phy_setmedia(sc);

		/*
		 * If autonegitation is not enabled, we need a
		 * software reset for the settings to take effect.
		 */
		if (IFM_SUBTYPE(ife->ifm_media) == IFM_NONE)
			makphy_pdown(sc, MAKARG_PDOWN);
		else if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO) {
			PHY_READ(sc, MII_BMCR, &bmcr);
			PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_RESET);
		}
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

static void
makphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	uint16_t bmcr, gsr, pssr, essr;

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	PHY_READ(sc, MII_BMCR, &bmcr);
	/* XXX FIXME: Use different page for Fiber on newer chips */
	PHY_READ(sc, MAKPHY_PSSR, &pssr);

	if (pssr & MAKPHY_PSSR_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	if (bmcr & (BMCR_ISO | BMCR_PDOWN)) {
		mii->mii_media_active |= IFM_NONE;
		return;
	}

	if ((bmcr & BMCR_AUTOEN) != 0) {
		/*
		 * Check Speed and Duplex Resolved bit.
		 * Note that this bit is always 1 when autonego is not enabled.
		 */
		if (!(pssr & MAKPHY_PSSR_RESOLVED)) {
			/* Erg, still trying, I guess... */
			mii->mii_media_active |= IFM_NONE;
			return;
		}
	} else {
		if ((pssr & MAKPHY_PSSR_LINK) == 0) {
			mii->mii_media_active |= IFM_NONE;
			return;
		}
	}

	/*
	 * XXX The following code support Fiber/Copper auto select mode
	 * only for 88E1011, 88E1111 and 88E1112. For other chips, the document
	 * is required.
	 */
	if (sc->mii_flags & MIIF_IS_1000X) {
		/* Not in Fiber/Copper auto select mode */
		mii->mii_media_active |= IFM_1000_SX;
	} else if ((sc->mii_mpd_model == MII_MODEL_xxMARVELL_E1011) ||
	    (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E1111)) {
		/* Fiber/Copper auto select mode */

		PHY_READ(sc, MAKPHY_ESSR, &essr);
		if ((essr & ESSR_FIBER_LINK) == 0)
			goto copper;

		/* XXX Assume 1000BASE-SX only */
		mii->mii_media_active |= IFM_1000_SX;
	} else if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E1112) {
		/* Fiber/Copper auto select mode */

		PHY_READ(sc, MAKPHY_PSSR, &pssr);
		if ((pssr & MAKPHY_PSSR_RESOLUTION_FIBER) == 0)
			goto copper;

		switch (MAKPHY_PSSR_SPEED_get(pssr)) {
		case SPEED_1000:
			mii->mii_media_active |= IFM_1000_SX;
			break;
		case SPEED_100:
			mii->mii_media_active |= IFM_100_FX;
			break;
		default: /* Undefined (reserved) value */
			mii->mii_media_active |= IFM_NONE;
			mii->mii_media_status = 0;
			return;
		}
	} else {
copper:
		switch (MAKPHY_PSSR_SPEED_get(pssr)) {
		case SPEED_1000:
			mii->mii_media_active |= IFM_1000_T;
			break;
		case SPEED_100:
			mii->mii_media_active |= IFM_100_TX;
			break;
		case SPEED_10:
			mii->mii_media_active |= IFM_10_T;
			break;
		default: /* Undefined (reserved) value */
			mii->mii_media_active |= IFM_NONE;
			mii->mii_media_status = 0;
			return;
		}
	}

	if (pssr & MAKPHY_PSSR_DUPLEX)
		mii->mii_media_active |= mii_phy_flowstatus(sc) | IFM_FDX;
	else
		mii->mii_media_active |= IFM_HDX;

	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
		PHY_READ(sc, MII_100T2SR, &gsr);
		if (gsr & GTSR_MS_RES)
			mii->mii_media_active |= IFM_ETH_MASTER;
	}
}
