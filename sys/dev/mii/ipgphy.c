/*	$NetBSD: ipgphy.c,v 1.11 2023/02/22 08:09:09 msaitoh Exp $ */
/*	$OpenBSD: ipgphy.c,v 1.19 2015/07/19 06:28:12 yuo Exp $	*/

/*-
 * Copyright (c) 2006, Pyun YongHyeon <yongari@FreeBSD.org>
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
 */

/*
 * Driver for the IC Plus IP1000A/IP1001 10/100/1000 PHY.
 */
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ipgphy.c,v 1.11 2023/02/22 08:09:09 msaitoh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <prop/proplib.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/miidevs.h>

#include <dev/mii/ipgphyreg.h>

static int ipgphy_match(device_t, cfdata_t, void *);
static void ipgphy_attach(device_t, device_t, void *);

struct ipgphy_softc {
	struct mii_softc sc_mii;
	bool need_loaddspcode;
};

CFATTACH_DECL_NEW(ipgphy, sizeof(struct ipgphy_softc),
    ipgphy_match, ipgphy_attach, mii_phy_detach, mii_phy_activate);

static int	ipgphy_service(struct mii_softc *, struct mii_data *, int);
static void	ipgphy_status(struct mii_softc *);
static int	ipgphy_mii_phy_auto(struct mii_softc *, u_int);
static void	ipgphy_load_dspcode(struct mii_softc *);
static void	ipgphy_reset(struct mii_softc *);

static const struct mii_phy_funcs ipgphy_funcs = {
	ipgphy_service, ipgphy_status, ipgphy_reset,
};

static const struct mii_phydesc ipgphys[] = {
	MII_PHY_DESC(xxICPLUS, IP1000A),
	MII_PHY_DESC(xxICPLUS, IP1001),
	MII_PHY_END,
};

static int
ipgphy_match(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, ipgphys) != NULL)
		return 10;

	return 0;
}

static void
ipgphy_attach(device_t parent, device_t self, void *aux)
{
	struct ipgphy_softc *isc = device_private(self);
	struct mii_softc *sc = &isc->sc_mii;
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;
	prop_dictionary_t dict;

	mpd = mii_phy_match(ma, ipgphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_funcs = &ipgphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_flags |= MIIF_NOISOLATE;

	if (device_is_a(parent, "stge")) {
		dict = device_properties(parent);
		prop_dictionary_get_bool(dict, "need_loaddspcode",
		    &isc->need_loaddspcode);
	}

	mii_lock(mii);

	PHY_RESET(sc);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	//sc->mii_capabilities &= ~BMSR_ANEG;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static int
ipgphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t reg, speed;

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

		PHY_RESET(sc);

		switch (IFM_SUBTYPE(ife->ifm_media)) {
		case IFM_AUTO:
		case IFM_1000_T:
			/*
			 * This device is required to do auto negotiation
			 * on 1000BASE-T.
			 */
			(void)ipgphy_mii_phy_auto(sc, ife->ifm_media);
			goto done;
			break;

		case IFM_100_TX:
			speed = BMCR_S100;
			break;

		case IFM_10_T:
			speed = BMCR_S10;
			break;

		default:
			return EINVAL;
		}

		if ((ife->ifm_media & IFM_FDX) != 0)
			speed |= BMCR_FDX;

		PHY_WRITE(sc, MII_100T2CR, 0);
		PHY_WRITE(sc, MII_BMCR, speed);
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
		 * Check to see if we have link.  If we do, we don't
		 * need to restart the autonegotiation process.  Read
		 * the BMSR twice in case it's latched.
		 */
		PHY_READ(sc, MII_BMSR, &reg);
		PHY_READ(sc, MII_BMSR, &reg);
		if (reg & BMSR_LINK) {
			/*
			 * Reset autonegotiation timer to 0 in case the link
			 * goes down in the next tick.
			 */
			sc->mii_ticks = 0;
			/* See above. */
			break;
		}

		/* Announce link loss right after it happens */
		if (sc->mii_ticks++ == 0)
			break;

		/* Only retry autonegotiation every mii_anegticks seconds. */
		if (sc->mii_ticks < sc->mii_anegticks)
			break;

		sc->mii_ticks = 0;
		ipgphy_mii_phy_auto(sc, ife->ifm_media);
		break;
	}

	/* Update the media status. */
	ipgphy_status(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return 0;
}

static void
ipgphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t bmsr, bmcr, stat, gtsr;

	KASSERT(mii_locked(mii));

	/* For IP1000A, use generic way */
	if (sc->mii_mpd_model == MII_MODEL_xxICPLUS_IP1000A) {
		ukphy_status(sc);
		return;
	}

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	PHY_READ(sc, MII_BMSR, &bmsr);
	PHY_READ(sc, MII_BMSR, &bmsr);
	if (bmsr & BMSR_LINK)
		mii->mii_media_status |= IFM_ACTIVE;

	PHY_READ(sc, MII_BMCR, &bmcr);
	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	if (bmcr & BMCR_AUTOEN) {
		if ((bmsr & BMSR_ACOMP) == 0) {
			/* Erg, still trying, I guess... */
			mii->mii_media_active |= IFM_NONE;
			return;
		}

		PHY_READ(sc, IPGPHY_LSR, &stat);
		switch (stat & IPGPHY_LSR_SPEED_MASK) {
		case IPGPHY_LSR_SPEED_10:
			mii->mii_media_active |= IFM_10_T;
			break;
		case IPGPHY_LSR_SPEED_100:
			mii->mii_media_active |= IFM_100_TX;
			break;
		case IPGPHY_LSR_SPEED_1000:
			mii->mii_media_active |= IFM_1000_T;
			break;
		default:
			mii->mii_media_active |= IFM_NONE;
			return;
		}

		if (stat & IPGPHY_LSR_FULL_DUPLEX)
			mii->mii_media_active |= IFM_FDX;
		else
			mii->mii_media_active |= IFM_HDX;

		if (mii->mii_media_active & IFM_FDX)
			mii->mii_media_active |= mii_phy_flowstatus(sc);

		if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
			PHY_READ(sc, MII_100T2SR, &gtsr);
			if (gtsr & GTSR_MS_RES)
				mii->mii_media_active |= IFM_ETH_MASTER;
		}
	} else
		mii->mii_media_active = ife->ifm_media;
}

static int
ipgphy_mii_phy_auto(struct mii_softc *sc, u_int media)
{
	uint16_t reg = 0;
	u_int subtype = IFM_SUBTYPE(media);

	KASSERT(mii_locked(sc->mii_pdata));

	/* XXX Is it requreid ? */
	if (sc->mii_mpd_model == MII_MODEL_xxICPLUS_IP1001) {
		PHY_READ(sc, MII_ANAR, &reg);
		reg &= ~(ANAR_PAUSE_SYM | ANAR_PAUSE_ASYM);
		reg |= ANAR_NP;
	}

	if (subtype == IFM_AUTO)
		reg |= ANAR_10 | ANAR_10_FD | ANAR_TX | ANAR_TX_FD;

	if (sc->mii_flags & MIIF_DOPAUSE)
		reg |= ANAR_PAUSE_SYM | ANAR_PAUSE_ASYM;

	PHY_WRITE(sc, MII_ANAR, reg | ANAR_CSMA);

	if (subtype == IFM_AUTO)
		reg = GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
	else if (subtype == IFM_1000_T) {
		if ((media & IFM_FDX) != 0)
			reg = GTCR_ADV_1000TFDX;
		else
			reg = GTCR_ADV_1000THDX;
	} else
		reg = 0;

	PHY_WRITE(sc, MII_100T2CR, reg);

	PHY_WRITE(sc, MII_BMCR, BMCR_FDX | BMCR_AUTOEN | BMCR_STARTNEG);

	return EJUSTRETURN;
}

static void
ipgphy_load_dspcode(struct mii_softc *sc)
{

	PHY_WRITE(sc, 31, 0x0001);
	PHY_WRITE(sc, 27, 0x01e0);
	PHY_WRITE(sc, 31, 0x0002);
	PHY_WRITE(sc, 27, 0xeb8e);
	PHY_WRITE(sc, 31, 0x0000);
	PHY_WRITE(sc, 30, 0x005e);
	PHY_WRITE(sc, 9, 0x0700);

	DELAY(50);
}

static void
ipgphy_reset(struct mii_softc *sc)
{
	struct ipgphy_softc *isc = device_private(sc->mii_dev);
	uint16_t reg;

	KASSERT(mii_locked(sc->mii_pdata));

	mii_phy_reset(sc);

	/* Clear autoneg/full-duplex as we don't want it after reset */
	PHY_READ(sc, MII_BMCR, &reg);
	reg &= ~(BMCR_AUTOEN | BMCR_FDX);
	PHY_WRITE(sc, MII_BMCR, reg);

	if (isc->need_loaddspcode)
		ipgphy_load_dspcode(sc);
}
