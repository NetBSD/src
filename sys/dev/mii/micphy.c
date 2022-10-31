/*	$NetBSD: micphy.c,v 1.15 2022/10/31 22:45:13 jmcneill Exp $	*/

/*-
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center, and by Frank van der Linden.
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
 * Driver for Micrel KSZ8xxx 10/100 and KSZ9xxx 10/100/1000 PHY.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: micphy.c,v 1.15 2022/10/31 22:45:13 jmcneill Exp $");

#include "opt_mii.h"

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

static int	micphymatch(device_t, cfdata_t, void *);
static void	micphyattach(device_t, device_t, void *);
static void	micphy_reset(struct mii_softc *);
static int	micphy_service(struct mii_softc *, struct mii_data *, int);

CFATTACH_DECL_NEW(micphy, sizeof(struct mii_softc),
    micphymatch, micphyattach, mii_phy_detach, mii_phy_activate);

static int	micphy_service(struct mii_softc *, struct mii_data *, int);
static void	micphy_status(struct mii_softc *);
static void	micphy_fixup(struct mii_softc *, int, int, device_t);

static const struct mii_phy_funcs micphy_funcs = {
	micphy_service, micphy_status, micphy_reset,
};

struct micphy_softc {
	struct mii_softc sc_mii;
	uint32_t sc_lstype;	/* Type of link status register */
};

static const struct mii_phydesc micphys[] = {
	MII_PHY_DESC(MICREL, KSZ8041),
	MII_PHY_DESC(MICREL, KSZ8051), /* +8021,8031 */
	MII_PHY_DESC(MICREL, KSZ8061),
	MII_PHY_DESC(MICREL, KSZ8081), /* +8051,8091 */
	MII_PHY_DESC(MICREL, KS8737),
	MII_PHY_DESC(MICREL, KSZ9021_8001_8721),
	MII_PHY_DESC(MICREL, KSZ9031),
	MII_PHY_DESC(MICREL, KSZ9131),
	MII_PHY_DESC(MICREL, KSZ9477), /* +LAN7430internal */
	MII_PHY_END,
};

/*
 * Model Rev. Media  LSTYPE Devices
 *
 * 0x11	      100    1F_42  KSZ8041
 * 0x13	      100    1F_42? KSZ8041RNLI
 * 0x15	   ?  100    1E_20  KSZ8051
 * 	 0x5  100    1E_20  KSZ8021
 * 	 0x6  100    1E_20  KSZ8031
 * 0x16	   ?  100    1E_20  KSZ8081
 * 	   ?  100    1E_20  KSZ8091
 * 0x17	      100    1E_20  KSZ8061
 * 0x21	 0x0  giga   GIGA   KSZ9021
 * 	 0x1  giga   GIGA   KSZ9021RLRN
 * 	 0x9  100    1F_42  KSZ8721BL/SL
 * 	 0x9  100    none?  KSZ8721CL
 * 	 0xa  100    1F_42  KSZ8001
 * 0x22	      giga   GIGA   KSZ9031
 * 0x23	   1? gigasw GIGA   KSZ9477 (No master/slave bit)
 * 	   5? giga   GIGA   LAN7430internal
 * 0x24	      giga   GIGA   KSZ9131
 * 0x32	      100    1F_42  KS8737
 */

/* Type of link status register */
#define MICPHYF_LSTYPE_DEFAULT	0
#define MICPHYF_LSTYPE_1F_42	1
#define MICPHYF_LSTYPE_1E_20	2
#define MICPHYF_LSTYPE_GIGA	3

/* Return if the device is Gigabit (KSZ9021) */
#define KSZ_MODEL21H_GIGA(rev)			\
	((((rev) & 0x0e) == 0) ? true : false)

#define KSZ_XREG_CONTROL	0x0b
#define KSZ_XREG_WRITE		0x0c
#define KSZ_XREG_READ		0x0d
#define KSZ_XREG_CTL_SEL_READ	0x0000
#define KSZ_XREG_CTL_SEL_WRITE	0x8000

#define REG_RGMII_CLOCK_AND_CONTROL	0x104
#define REG_RGMII_RX_DATA		0x105

/* PHY control 1 register for 10/100 PHYs (KSZ80[235689]1) */
#define KSZ8051_PHYCTL1		0x1e
#define KSZ8051_PHY_LINK	0x0100
#define KSZ8051_PHY_MDIX	0x0020
#define KSZ8051_PHY_FDX		0x0004
#define KSZ8051_PHY_SPD_MASK	0x0003
#define KSZ8051_PHY_SPD_10T	0x0001
#define KSZ8051_PHY_SPD_100TX	0x0002

/* PHY control 2 register for 10/100 PHYs (KSZ8041, KSZ8721 and KSZ8001) */
#define	KSZ8041_PHYCTL2		0x1f
#define KSZ8041_PHY_ACOMP	0x0080
#define KSZ8041_PHY_SPD_MASK	0x001c
#define KSZ8041_PHY_SPD_10T	0x0004
#define KSZ8041_PHY_SPD_100TX	0x0008
#define KSZ8041_PHY_FDX		0x0010
#define KSZ8051_PHYCTL2		0x1f

/* PHY control register for Gigabit PHYs */
#define KSZ_GPHYCTL		0x1f
#define KSZ_GPHY_SPD_1000T	0x0040
#define KSZ_GPHY_SPD_100TX	0x0020
#define KSZ_GPHY_SPD_10T	0x0010
#define KSZ_GPHY_FDX		0x0008
#define KSZ_GPHY_1000T_MS	0x0004

static int
micphymatch(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, micphys) != NULL)
		return 10;

	return 1;
}

static void
micphyattach(device_t parent, device_t self, void *aux)
{
	struct micphy_softc *msc = device_private(self);
	struct mii_softc *sc = &msc->sc_mii;
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	int model = MII_MODEL(ma->mii_id2);
	int rev = MII_REV(ma->mii_id2);
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, micphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, rev);

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &micphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;

	if ((sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8041)
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8041RNLI)
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KS8737)
	    || ((sc->mii_mpd_model == MII_MODEL_MICREL_KSZ9021_8001_8721)
		&& !KSZ_MODEL21H_GIGA(sc->mii_mpd_rev))) {
		msc->sc_lstype = MICPHYF_LSTYPE_1F_42;
	} else if ((sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8051)
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8081)
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8061)) {
		msc->sc_lstype = MICPHYF_LSTYPE_1E_20;
	} else if (((sc->mii_mpd_model == MII_MODEL_MICREL_KSZ9021_8001_8721)
		&& KSZ_MODEL21H_GIGA(sc->mii_mpd_rev))
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ9031)
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ9477)
	    || (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ9131)) {
		msc->sc_lstype = MICPHYF_LSTYPE_GIGA;
	} else
		msc->sc_lstype = MICPHYF_LSTYPE_DEFAULT;

	mii_lock(mii);

	PHY_RESET(sc);

	micphy_fixup(sc, model, rev, parent);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static void
micphy_reset(struct mii_softc *sc)
{
	uint16_t reg;

	KASSERT(mii_locked(sc->mii_pdata));

	/*
	 * The 8081 has no "sticky bits" that survive a soft reset; several
	 * bits in the Phy Control Register 2 must be preserved across the
	 * reset. These bits are set up by the bootloader; they control how the
	 * phy interfaces to the board (such as clock frequency and LED
	 * behavior).
	 */
	if (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8081)
		PHY_READ(sc, KSZ8051_PHYCTL2, &reg);
	mii_phy_reset(sc);
	if (sc->mii_mpd_model == MII_MODEL_MICREL_KSZ8081)
		PHY_WRITE(sc, KSZ8051_PHYCTL2, reg);
}

static int
micphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t reg;

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
			PHY_READ(sc, MII_BMCR, &reg);
			PHY_WRITE(sc, MII_BMCR, reg | BMCR_ISO);
			return 0;
		}

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

static void
micphy_writexreg(struct mii_softc *sc, uint32_t reg, uint32_t wval)
{
	uint16_t rval __debugused;

	PHY_WRITE(sc, KSZ_XREG_CONTROL, KSZ_XREG_CTL_SEL_WRITE | reg);
	PHY_WRITE(sc, KSZ_XREG_WRITE, wval);
	PHY_WRITE(sc, KSZ_XREG_CONTROL, KSZ_XREG_CTL_SEL_READ | reg);
	PHY_READ(sc, KSZ_XREG_READ, &rval);
	KDASSERT(wval == rval);
}

static void
micphy_fixup(struct mii_softc *sc, int model, int rev, device_t parent)
{

	KASSERT(mii_locked(sc->mii_pdata));

	switch (model) {
	case MII_MODEL_MICREL_KSZ9021_8001_8721:
		if (!device_is_a(parent, "cpsw"))
			break;

		aprint_normal_dev(sc->mii_dev,
		    "adjusting RGMII signal timing for cpsw\n");

		// RGMII RX Data Pad Skew
		micphy_writexreg(sc, REG_RGMII_RX_DATA, 0x0000);

		// RGMII Clock and Control Pad Skew
		micphy_writexreg(sc, REG_RGMII_CLOCK_AND_CONTROL, 0x9090);

		break;
	default:
		break;
	}

	return;
}

static void
micphy_status(struct mii_softc *sc)
{
	struct micphy_softc *msc = device_private(sc->mii_dev);
	struct mii_data *mii = sc->mii_pdata;
	uint16_t bmsr, bmcr, sr;

	KASSERT(mii_locked(mii));

	/* For unknown devices */
	if (msc->sc_lstype == MICPHYF_LSTYPE_DEFAULT) {
		ukphy_status(sc);
		return;
	}

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	PHY_READ(sc, MII_BMCR, &bmcr);

	PHY_READ(sc, MII_BMSR, &bmsr);
	PHY_READ(sc, MII_BMSR, &bmsr);
	if (bmsr & BMSR_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	if (bmcr & BMCR_AUTOEN) {
		if ((bmsr & BMSR_ACOMP) == 0) {
			mii->mii_media_active |= IFM_NONE;
			return;
		}
	}

	if (msc->sc_lstype == MICPHYF_LSTYPE_1F_42) {
		PHY_READ(sc, KSZ8041_PHYCTL2, &sr);
		if ((sr & KSZ8041_PHY_SPD_MASK) == 0)
			mii->mii_media_active |= IFM_NONE;
		else if (sr & KSZ8041_PHY_SPD_100TX)
			mii->mii_media_active |= IFM_100_TX;
		else if (sr & KSZ8041_PHY_SPD_10T)
			mii->mii_media_active |= IFM_10_T;
		if (sr & KSZ8041_PHY_FDX)
			mii->mii_media_active |= IFM_FDX
			    | mii_phy_flowstatus(sc);
	} else if (msc->sc_lstype == MICPHYF_LSTYPE_1E_20) {
		PHY_READ(sc, KSZ8051_PHYCTL1, &sr);
		if ((sr & KSZ8051_PHY_SPD_MASK) == 0)
			mii->mii_media_active |= IFM_NONE;
		else if (sr & KSZ8051_PHY_SPD_100TX)
			mii->mii_media_active |= IFM_100_TX;
		else if (sr & KSZ8051_PHY_SPD_10T)
			mii->mii_media_active |= IFM_10_T;
		if (sr & KSZ8051_PHY_FDX)
			mii->mii_media_active |= IFM_FDX
			    | mii_phy_flowstatus(sc);
	} else if (msc->sc_lstype == MICPHYF_LSTYPE_GIGA) {
		/* 9021/9031/7430/9131 gphy */
		PHY_READ(sc, KSZ_GPHYCTL, &sr);
		if (sr & KSZ_GPHY_SPD_1000T)
			mii->mii_media_active |= IFM_1000_T;
		else if (sr & KSZ_GPHY_SPD_100TX)
			mii->mii_media_active |= IFM_100_TX;
		else if (sr & KSZ_GPHY_SPD_10T)
			mii->mii_media_active |= IFM_10_T;
		else
			mii->mii_media_active |= IFM_NONE;
		if ((mii->mii_media_active & IFM_1000_T)
		    && (sr & KSZ_GPHY_1000T_MS))
			mii->mii_media_active |= IFM_ETH_MASTER;
		if (sr & KSZ_GPHY_FDX)
			mii->mii_media_active |= IFM_FDX
			    | mii_phy_flowstatus(sc);
		else
			mii->mii_media_active |= IFM_HDX;
	}
}
