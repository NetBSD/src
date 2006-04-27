/*	$NetBSD: brgphy.c,v 1.27 2006/04/27 16:43:14 jonathan Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Manuel Bouyer.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 * driver for the Broadcom BCM5400 Gig-E PHY.
 *
 * Programming information for this PHY was gleaned from FreeBSD
 * (they were apparently able to get a datasheet from Broadcom).
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: brgphy.c,v 1.27 2006/04/27 16:43:14 jonathan Exp $");

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

#include <dev/mii/brgphyreg.h>

static int	brgphymatch(struct device *, struct cfdata *, void *);
static void	brgphyattach(struct device *, struct device *, void *);

CFATTACH_DECL(brgphy, sizeof(struct mii_softc),
    brgphymatch, brgphyattach, mii_phy_detach, mii_phy_activate);

static int	brgphy_service(struct mii_softc *, struct mii_data *, int);
static void	brgphy_status(struct mii_softc *);

static void	brgphy_5401_reset(struct mii_softc *);
static void	brgphy_5411_reset(struct mii_softc *);
static void	brgphy_5703_reset(struct mii_softc *);
static void	brgphy_5704_reset(struct mii_softc *);
static void	brgphy_5705_reset(struct mii_softc *);
static void	brgphy_5750_reset(struct mii_softc *);

static const struct mii_phy_funcs brgphy_funcs = {
	brgphy_service, brgphy_status, mii_phy_reset,
};

static const struct mii_phy_funcs brgphy_5401_funcs = {
	brgphy_service, brgphy_status, brgphy_5401_reset,
};

static const struct mii_phy_funcs brgphy_5411_funcs = {
	brgphy_service, brgphy_status, brgphy_5411_reset,
};

static const struct mii_phy_funcs brgphy_5703_funcs = {
	brgphy_service, brgphy_status, brgphy_5703_reset,
};

static const struct mii_phy_funcs brgphy_5704_funcs = {
	brgphy_service, brgphy_status, brgphy_5704_reset,
};

static const struct mii_phy_funcs brgphy_5705_funcs = {
	brgphy_service, brgphy_status, brgphy_5705_reset,
};

const struct mii_phy_funcs brgphy_5750_funcs = {
	brgphy_service, brgphy_status, brgphy_5750_reset,
};


static const struct mii_phydesc brgphys[] = {
	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5400,
	  MII_STR_BROADCOM_BCM5400 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5401,
	  MII_STR_BROADCOM_BCM5401 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5411,
	  MII_STR_BROADCOM_BCM5411 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5421,
	  MII_STR_BROADCOM_BCM5421 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5701,
	  MII_STR_BROADCOM_BCM5701 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5703,
	  MII_STR_BROADCOM_BCM5703 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5704,
	  MII_STR_BROADCOM_BCM5704 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5705,
	  MII_STR_BROADCOM_BCM5705 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5714,
	  MII_STR_BROADCOM_BCM5714 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5750,
	  MII_STR_BROADCOM_BCM5750 },

	{ MII_OUI_BROADCOM,		MII_MODEL_BROADCOM_BCM5780,
	  MII_STR_BROADCOM_BCM5780 },

	{ 0,				0,
	  NULL },
};

static void bcm5401_load_dspcode(struct mii_softc *);
static void bcm5411_load_dspcode(struct mii_softc *);
static void bcm5703_load_dspcode(struct mii_softc *);
static void bcm5704_load_dspcode(struct mii_softc *);
static void bcm5750_load_dspcode(struct mii_softc *);

static int
brgphymatch(struct device *parent, struct cfdata *match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, brgphys) != NULL)
		return (10);

	return (0);
}

static void
brgphyattach(struct device *parent, struct device *self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, brgphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_anegticks = 5;

	switch (MII_MODEL(ma->mii_id2)) {
	case MII_MODEL_BROADCOM_BCM5400:
		sc->mii_funcs = &brgphy_5401_funcs;
		aprint_normal("%s: using BCM5401 DSP patch\n",
		    sc->mii_dev.dv_xname);
		break;

	case MII_MODEL_BROADCOM_BCM5401:
		if (MII_REV(ma->mii_id2) == 1 || MII_REV(ma->mii_id2) == 3) {
			sc->mii_funcs = &brgphy_5401_funcs;
			aprint_normal("%s: using BCM5401 DSP patch\n",
			    sc->mii_dev.dv_xname);
		} else
			sc->mii_funcs = &brgphy_funcs;
		break;

	case MII_MODEL_BROADCOM_BCM5411:
		sc->mii_funcs = &brgphy_5411_funcs;
		aprint_normal("%s: using BCM5411 DSP patch\n",
		    sc->mii_dev.dv_xname);
		break;

#ifdef notyet /* unverified, untested */
	case MII_MODEL_BROADCOM_BCM5703:
		sc->mii_funcs = &brgphy_5703_funcs;
		aprint_normal("%s: using BCM5703 DSP patch\n",
		    sc->mii_dev.dv_xname);
		break;
#endif

	case MII_MODEL_BROADCOM_BCM5704:
		sc->mii_funcs = &brgphy_5704_funcs;
		aprint_normal("%s: using BCM5704 DSP patch\n",
		    sc->mii_dev.dv_xname);
		break;

	case MII_MODEL_BROADCOM_BCM5705:
		sc->mii_funcs = &brgphy_5705_funcs;
		break;

	case MII_MODEL_BROADCOM_BCM5714:
	case MII_MODEL_BROADCOM_BCM5780:
	case MII_MODEL_BROADCOM_BCM5750:
		sc->mii_funcs = &brgphy_5750_funcs;
		break;

	default:
		sc->mii_funcs = &brgphy_funcs;
		break;
	}

	PHY_RESET(sc);

	sc->mii_capabilities =
	    PHY_READ(sc, MII_BMSR) & ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		sc->mii_extcapabilities = PHY_READ(sc, MII_EXTSR);

	aprint_normal("%s: ", sc->mii_dev.dv_xname);
	if ((sc->mii_capabilities & BMSR_MEDIAMASK) == 0 &&
	    (sc->mii_extcapabilities & EXTSR_MEDIAMASK) == 0)
		aprint_error("no media present");
	else
		mii_phy_add_media(sc);
	aprint_normal("\n");
}

static int
brgphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	int reg;

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
			reg = PHY_READ(sc, MII_BMCR);
			PHY_WRITE(sc, MII_BMCR, reg | BMCR_ISO);
			return (0);
		}

		/*
		 * If the interface is not up, don't do anything.
		 */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			break;

		if (sc->mii_funcs != &brgphy_5705_funcs)
			mii_phy_reset(sc);    /* XXX hardware bug work-around */
		mii_phy_setmedia(sc);
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

	/*
	 * Callback if something changed.  Note that we need to poke
	 * the DSP on the Broadcom PHYs if the media changes.
	 */
	if (sc->mii_media_active != mii->mii_media_active ||
	    sc->mii_media_status != mii->mii_media_status ||
	    cmd == MII_MEDIACHG) {
		mii_phy_update(sc, cmd);
		if (sc->mii_funcs == &brgphy_5401_funcs)
			bcm5401_load_dspcode(sc);
		else if (sc->mii_funcs == &brgphy_5411_funcs)
			bcm5411_load_dspcode(sc);
	}
	return (0);
}

static void
brgphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	int bmcr, auxsts, gtsr;

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	auxsts = PHY_READ(sc, BRGPHY_MII_AUXSTS);

	if (auxsts & BRGPHY_AUXSTS_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	bmcr = PHY_READ(sc, MII_BMCR);
	if (bmcr & BMCR_ISO) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	if (bmcr & BMCR_AUTOEN) {
		/*
		 * The media status bits are only valid of autonegotiation
		 * has completed (or it's disabled).
		 */
		if ((auxsts & BRGPHY_AUXSTS_ACOMP) == 0) {
			/* Erg, still trying, I guess... */
			mii->mii_media_active |= IFM_NONE;
			return;
		}

		switch (auxsts & BRGPHY_AUXSTS_AN_RES) {
		case BRGPHY_RES_1000FD:
			mii->mii_media_active |= IFM_1000_T|IFM_FDX;
			gtsr = PHY_READ(sc, MII_100T2SR);
			if (gtsr & GTSR_MS_RES)
				mii->mii_media_active |= IFM_ETH_MASTER;
			break;

		case BRGPHY_RES_1000HD:
			mii->mii_media_active |= IFM_1000_T;
			gtsr = PHY_READ(sc, MII_100T2SR);
			if (gtsr & GTSR_MS_RES)
				mii->mii_media_active |= IFM_ETH_MASTER;
			break;

		case BRGPHY_RES_100FD:
			mii->mii_media_active |= IFM_100_TX|IFM_FDX;
			break;

		case BRGPHY_RES_100T4:
			mii->mii_media_active |= IFM_100_T4;
			break;

		case BRGPHY_RES_100HD:
			mii->mii_media_active |= IFM_100_TX;
			break;

		case BRGPHY_RES_10FD:
			mii->mii_media_active |= IFM_10_T|IFM_FDX;
			break;

		case BRGPHY_RES_10HD:
			mii->mii_media_active |= IFM_10_T;
			break;

		default:
			mii->mii_media_active |= IFM_NONE;
			mii->mii_media_status = 0;
		}
		if (mii->mii_media_active & IFM_FDX)
			mii->mii_media_active |= mii_phy_flowstatus(sc);
	} else
		mii->mii_media_active = ife->ifm_media;
}

static void
brgphy_5401_reset(struct mii_softc *sc)
{

	mii_phy_reset(sc);
	bcm5401_load_dspcode(sc);
}

static void
brgphy_5411_reset(struct mii_softc *sc)
{

	mii_phy_reset(sc);
	bcm5411_load_dspcode(sc);
}


static void
brgphy_5703_reset(struct mii_softc *sc)
{

	mii_phy_reset(sc);
	bcm5703_load_dspcode(sc);
}

static void
brgphy_5704_reset(struct mii_softc *sc)
{

	mii_phy_reset(sc);
	bcm5704_load_dspcode(sc);
}

/*
 * Hardware bug workaround.  Do nothing since after
 * reset the 5705 PHY would get stuck in 10/100 MII mode.
 */

static void
brgphy_5705_reset(struct mii_softc *sc)
{
}

static void
brgphy_5750_reset(struct mii_softc *sc)
{
	mii_phy_reset(sc);
	bcm5750_load_dspcode(sc);
}

/* Turn off tap power management on 5401. */
static void
bcm5401_load_dspcode(struct mii_softc *sc)
{
	static const struct {
		int		reg;
		uint16_t	val;
	} dspcode[] = {
		{ BRGPHY_MII_AUXCTL,		0x0c20 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x0012 },
		{ BRGPHY_MII_DSP_RW_PORT,	0x1804 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x0013 },
		{ BRGPHY_MII_DSP_RW_PORT,	0x1204 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x8006 },
		{ BRGPHY_MII_DSP_RW_PORT,	0x0132 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x8006 },
		{ BRGPHY_MII_DSP_RW_PORT,	0x0232 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x201f },
		{ BRGPHY_MII_DSP_RW_PORT,	0x0a20 },
		{ 0,				0 },
	};
	int i;

	for (i = 0; dspcode[i].reg != 0; i++)
		PHY_WRITE(sc, dspcode[i].reg, dspcode[i].val);
    delay(40);
}

static void
bcm5411_load_dspcode(struct mii_softc *sc)
{
	static const struct {
		int		reg;
		uint16_t	val;
	} dspcode[] = {
		{ 0x1c,				0x8c23 },
		{ 0x1c,				0x8ca3 },
		{ 0x1c,				0x8c23 },
		{ 0,				0 },
	};
	int i;

	for (i = 0; dspcode[i].reg != 0; i++)
		PHY_WRITE(sc, dspcode[i].reg, dspcode[i].val);
}

static void
bcm5703_load_dspcode(struct mii_softc *sc)
{
	static const struct {
		int		reg;
		uint16_t	val;
	} dspcode[] = {
		{ BRGPHY_MII_AUXCTL,		0x0c00 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x201f },
		{ BRGPHY_MII_DSP_RW_PORT,	0x2aaa },
		{ 0,				0 },
	};
	int i;

	for (i = 0; dspcode[i].reg != 0; i++)
		PHY_WRITE(sc, dspcode[i].reg, dspcode[i].val);
}

static void
bcm5704_load_dspcode(struct mii_softc *sc)
{
	static const struct {
		int		reg;
		uint16_t	val;
	} dspcode[] = {
		{ 0x1c,				0x8d68 },
   		{ 0x1c,				0x8d68 },
		{ 0,				0 },
	};
	int i;

	for (i = 0; dspcode[i].reg != 0; i++)
		PHY_WRITE(sc, dspcode[i].reg, dspcode[i].val);
}

static void
bcm5750_load_dspcode(struct mii_softc *sc)
{
	static const struct {
		int		reg;
		uint16_t	val;
	} dspcode[] = {
		{ BRGPHY_MII_AUXCTL,		0x0c00 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x000a },
		{ BRGPHY_MII_DSP_RW_PORT,	0x310b },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x201f },
		{ BRGPHY_MII_DSP_RW_PORT,	0x9506 },
		{ BRGPHY_MII_DSP_ADDR_REG,	0x401f },
		{ BRGPHY_MII_DSP_RW_PORT,	0x14e2 },
		{ BRGPHY_MII_AUXCTL,		0x0400 },
		{ 0,				0 },
	};
	int i;

	for (i = 0; dspcode[i].reg != 0; i++)
		PHY_WRITE(sc, dspcode[i].reg, dspcode[i].val);
}
