/*	$NetBSD: makphy.c,v 1.42.14.1 2018/06/25 07:25:51 pgoyette Exp $	*/
/*	$OpenBSD: eephy.c,v 1.56 2015/03/14 03:38:48 jsg Exp $	*/

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
 * Principal Author: Parag Patel
 * Copyright (c) 2001
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
 *
 * Additonal Copyright (c) 2001 by Traakan Software under same licence.
 * Secondary Author: Matthew Jacob
 */
/*
 * Driver for the Marvell 88E1000 ``Alaska'' 10/100/1000 PHY.
 */

/*
 * Support added for the Marvell 88E1011 (Alaska) 1000/100/10baseTX and
 * 1000baseSX PHY.
 * Nathan Binkert <nate@openbsd.org>
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: makphy.c,v 1.42.14.1 2018/06/25 07:25:51 pgoyette Exp $");

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

#include <dev/mii/e1000phyreg.h>

static int	makphymatch(device_t, cfdata_t, void *);
static void	makphyattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(makphy, sizeof(struct mii_softc),
    makphymatch, makphyattach, mii_phy_detach, mii_phy_activate);

static int	makphy_service(struct mii_softc *, struct mii_data *, int);
static void	makphy_status(struct mii_softc *);
static void	makphy_reset(struct mii_softc *);

static const struct mii_phy_funcs eephy_funcs = {
	makphy_service, makphy_status, makphy_reset,
};

static const struct mii_phydesc eephys[] = {
	{ MII_OUI_MARVELL,		MII_MODEL_MARVELL_E1000_0,
	  MII_STR_MARVELL_E1000_0 },

	{ MII_OUI_MARVELL,		MII_MODEL_MARVELL_E1000_3,
	  MII_STR_MARVELL_E1000_3 },

	{ MII_OUI_MARVELL,		MII_MODEL_MARVELL_E1000_5,
	  MII_STR_MARVELL_E1000_5 },

	{ MII_OUI_MARVELL,		MII_MODEL_MARVELL_E1000_6,
	  MII_STR_MARVELL_E1000_6 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1000_3,
	  MII_STR_xxMARVELL_E1000_3 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1000_5,
	  MII_STR_xxMARVELL_E1000_5 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1000S,
	  MII_STR_xxMARVELL_E1000S },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1011,
	  MII_STR_xxMARVELL_E1011 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1111,
	  MII_STR_xxMARVELL_E1111 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1112,
	  MII_STR_xxMARVELL_E1112 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1116,
	  MII_STR_xxMARVELL_E1116 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1116R,
	  MII_STR_xxMARVELL_E1116R },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1116R_29,
	  MII_STR_xxMARVELL_E1116R_29 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1118,
	  MII_STR_xxMARVELL_E1118 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1145,
	  MII_STR_xxMARVELL_E1145 },

	/* XXX: reported not to work on eg. HP XW9400 */
	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1149,
	  MII_STR_xxMARVELL_E1149 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1149R,
	  MII_STR_xxMARVELL_E1149R },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E1543,
	  MII_STR_xxMARVELL_E1543 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E3016,
	  MII_STR_xxMARVELL_E3016 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_E3082,
	  MII_STR_xxMARVELL_E3082 },

	{ MII_OUI_xxMARVELL,		MII_MODEL_xxMARVELL_PHYG65G,
	  MII_STR_xxMARVELL_PHYG65G },
	{ 0,				0,
	  NULL },
};

static int
makphymatch(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, eephys) != NULL)
		return (10);

	return (0);
}

static void
makphyattach(device_t parent, device_t self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, eephys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_dev = self;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &eephy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_anegticks = MII_ANEGTICKS;

	/* XXX No loopback support yet, although the hardware can do it. */
	sc->mii_flags |= MIIF_NOLOOP;

	/* Make sure page 0 is selected. */
	PHY_WRITE(sc, E1000_EADR, 0);

	/* Switch to copper-only mode if necessary. */
	if (sc->mii_mpd_model == MII_MODEL_MARVELL_E1111 &&
	    (sc->mii_flags & MIIF_HAVEFIBER) == 0) {
		/*
		 * The onboard 88E1111 PHYs on the Sun X4100 M2 come
		 * up with fiber/copper auto-selection enabled, even
		 * though the machine only has copper ports.  This
		 * makes the chip autoselect to 1000baseX, and makes
		 * it impossible to select any other media.  So
		 * disable fiber/copper autoselection.
		 */
		int reg = PHY_READ(sc, E1000_ESSR);
		if ((reg & E1000_ESSR_HWCFG_MODE) == E1000_ESSR_RGMII_COPPER) {
			reg |= E1000_ESSR_DIS_FC;
			PHY_WRITE(sc, E1000_ESSR, reg);
		}
	}

	/* Switch to fiber-only mode if necessary. */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E1112 &&
	    sc->mii_flags & MIIF_HAVEFIBER) {
		int page = PHY_READ(sc, E1000_EADR);
		PHY_WRITE(sc, E1000_EADR, 2);
		int reg = PHY_READ(sc, E1000_SCR);
		reg &= ~E1000_SCR_MODE_MASK;
		reg |= E1000_SCR_MODE_1000BX;
		PHY_WRITE(sc, E1000_SCR, reg);
		PHY_WRITE(sc, E1000_EADR, page);
	}

	PHY_RESET(sc);

	sc->mii_capabilities = PHY_READ(sc, MII_BMSR) & ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		sc->mii_extcapabilities = PHY_READ(sc, MII_EXTSR);

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
	int reg, i;

	reg = PHY_READ(sc, MII_BMCR);
	reg |= BMCR_RESET;
	PHY_WRITE(sc, MII_BMCR, reg);
	
	for (i = 0; i < 500; i++) {
		DELAY(1);
		reg = PHY_READ(sc, MII_BMCR);
		if (!(reg & BMCR_RESET))
			break;
	}

	/*
	 * Initialize PHY Specific Control Register.
	 */
	reg = PHY_READ(sc, E1000_SCR);

	/* Assert CRS on transmit. */
	reg |= E1000_SCR_ASSERT_CRS_ON_TX;

	/* Enable auto crossover. */
	switch (sc->mii_mpd_model) {
	case MII_MODEL_xxMARVELL_E3016:
	case MII_MODEL_xxMARVELL_E3082:
		/* Bits are in a different position.  */
		reg |= (E1000_SCR_AUTO_X_MODE >> 1);
		break;
	default:
		/* Automatic crossover causes problems for 1000baseX. */
		if (sc->mii_flags & MIIF_IS_1000X)
			reg &= ~E1000_SCR_AUTO_X_MODE;
		else
			reg |= E1000_SCR_AUTO_X_MODE;
	}

	/* Disable energy detect; only available on some models. */
	switch(sc->mii_mpd_model) {
	case MII_MODEL_xxMARVELL_E3016:
		reg &= ~E3000_SCR_EN_DETECT_MASK;
		break;
	case MII_MODEL_MARVELL_E1011:
	case MII_MODEL_MARVELL_E1111:
	case MII_MODEL_xxMARVELL_E1112:
	case MII_MODEL_xxMARVELL_PHYG65G:
		reg &= ~E1000_SCR_EN_DETECT_MASK;
		break;
	}

	/* Enable scrambler if necessary. */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E3016)
		reg &= ~E3000_SCR_SCRAMBLER_DISABLE;

	/*
	 * Store next page in the Link Partner Next Page register for
	 * compatibility with 802.3ab.
	 */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E3016)
		reg |= E3000_SCR_REG8_NEXT_PAGE;

	PHY_WRITE(sc, E1000_SCR, reg);

	/* 25 MHz TX_CLK should always work. */
	reg = PHY_READ(sc, E1000_ESCR);
	reg |= E1000_ESCR_TX_CLK_25;
	PHY_WRITE(sc, E1000_ESCR, reg);

	/* Configure LEDs if they were left unconfigured. */
	if (sc->mii_mpd_model == MII_MODEL_xxMARVELL_E3016 &&
	    PHY_READ(sc, 0x16) == 0) {
		reg = (0x0b << 8) | (0x05 << 4) | 0x04;	/* XXX */
		PHY_WRITE(sc, 0x16, reg);
	}

	/*
	 * Do a software reset for these settings to take effect.
	 * Disable autonegotiation, such that all capabilities get
	 * advertised when it is switched back on.
	 */
	reg = PHY_READ(sc, MII_BMCR);
	reg &= ~BMCR_AUTOEN;
	PHY_WRITE(sc, MII_BMCR, reg | BMCR_RESET);
}

static int
makphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	int bmcr;

	if (!device_is_active(sc->mii_dev))
		return (ENXIO);

	switch (cmd) {
	case MII_POLLSTAT:
		/*
		 * If we're not polling our PHY instance, just return.
		 */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return (0);
		break;

	case MII_MEDIACHG:
		/*
		 * If the media indicates a different PHY instance,
		 * isolate ourselves.
		 */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst) {
			bmcr = PHY_READ(sc, MII_BMCR);
			PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_ISO);
			return (0);
		}

		/*
		 * If the interface is not up, don't do anything.
		 */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			break;

		mii_phy_setmedia(sc);

		/*
		 * If autonegitation is not enabled, we need a
		 * software reset for the settings to take effect.
		 */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO) {
			bmcr = PHY_READ(sc, MII_BMCR);
			PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_RESET);
		}
		break;

	case MII_TICK:
		/*
		 * If we're not currently selected, just return.
		 */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return (0);

		if (mii_phy_tick(sc) == EJUSTRETURN)
			return (0);
		break;

	case MII_DOWN:
		mii_phy_down(sc);
		return (0);
	}

	/* Update the media status. */
	mii_phy_status(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return (0);
}

static void
makphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	int bmcr, gsr, ssr;

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	bmcr = PHY_READ(sc, MII_BMCR);
	ssr = PHY_READ(sc, E1000_SSR);

	if (ssr & E1000_SSR_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	if (bmcr & BMCR_ISO) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	if (!(ssr & E1000_SSR_SPD_DPLX_RESOLVED)) {
		/* Erg, still trying, I guess... */
		mii->mii_media_active |= IFM_NONE;
		return;
	}

	if (sc->mii_flags & MIIF_IS_1000X) {
		mii->mii_media_active |= IFM_1000_SX;
	} else {
		if (ssr & E1000_SSR_1000MBS)
			mii->mii_media_active |= IFM_1000_T;
		else if (ssr & E1000_SSR_100MBS)
			mii->mii_media_active |= IFM_100_TX;
		else
			mii->mii_media_active |= IFM_10_T;
	}

	if (ssr & E1000_SSR_DUPLEX)
		mii->mii_media_active |= mii_phy_flowstatus(sc) | IFM_FDX;
	else
		mii->mii_media_active |= IFM_HDX;

	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
		gsr = PHY_READ(sc, MII_100T2SR) | PHY_READ(sc, MII_100T2SR);
		if (gsr & GTSR_MS_RES)
			mii->mii_media_active |= IFM_ETH_MASTER;
	}
}
