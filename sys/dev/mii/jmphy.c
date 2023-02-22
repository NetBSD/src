/*	$NetBSD: jmphy.c,v 1.5 2023/02/22 08:09:09 msaitoh Exp $ */
/*	$OpenBSD: jmphy.c,v 1.6 2015/03/14 03:38:48 jsg Exp $	*/
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
 *
 * $FreeBSD: src/sys/dev/mii/jmphy.c,v 1.1 2008/05/27 01:16:40 yongari Exp $
 * $DragonFly: src/sys/dev/netif/mii_layer/jmphy.c,v 1.1 2008/07/22 11:28:49 sephe Exp $
 */

/*
 * Driver for the JMicron JMP211 10/100/1000, JMP202 10/100 PHY.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/miidevs.h>
#include <dev/mii/jmphyreg.h>

static int	jmphy_service(struct mii_softc *, struct mii_data *, int);
static void	jmphy_status(struct mii_softc *);
static int	jmphy_match(device_t, cfdata_t, void *);
static void	jmphy_attach(device_t, device_t, void *);
static void	jmphy_reset(struct mii_softc *);
static uint16_t	jmphy_anar(struct ifmedia_entry *);
static int	jmphy_auto(struct mii_softc *, struct ifmedia_entry *);

static const struct mii_phy_funcs jmphy_funcs = {
	jmphy_service, jmphy_status, jmphy_reset,
};

CFATTACH_DECL_NEW(jmphy, sizeof (struct mii_softc),
    jmphy_match, jmphy_attach, mii_phy_detach, mii_phy_activate);

static const struct mii_phydesc jmphys[] = {
	MII_PHY_DESC(JMICRON, JMP202),
	MII_PHY_DESC(JMICRON, JMP211),
	MII_PHY_END,
};

static int
jmphy_match(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, jmphys) != NULL)
		return 10;

	return 0;
}

static void
jmphy_attach(device_t parent, device_t self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, jmphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &jmphy_funcs;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;

	sc->mii_flags |= MIIF_NOISOLATE | MIIF_NOLOOP;

	mii_lock(mii);

	PHY_RESET(sc);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static int
jmphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t bmcr, ssr;

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

		if (jmphy_auto(sc, ife) != EJUSTRETURN)
			return EINVAL;
		break;

	case MII_TICK:
		/* If we're not currently selected, just return. */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return 0;

		/* Is the interface even up? */
		if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
			return 0;

		/* Only used for autonegotiation. */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO)
			break;

		/* Check for link. */
		PHY_READ(sc, JMPHY_SSR, &ssr);
		if (ssr & JMPHY_SSR_LINK_UP) {
			sc->mii_ticks = 0;
			break;
		}

		/* Announce link loss right after it happens. */
		if (sc->mii_ticks++ == 0)
			break;
		if (sc->mii_ticks < sc->mii_anegticks)
			return 0;

		sc->mii_ticks = 0;
		jmphy_auto(sc, ife);
		break;
	}

	/* Update the media status. */
	jmphy_status(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return 0;
}

static void
jmphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	uint16_t bmcr, ssr, gtsr;

	KASSERT(mii_locked(mii));

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	PHY_READ(sc, JMPHY_SSR, &ssr);
	if ((ssr & JMPHY_SSR_LINK_UP) != 0)
		mii->mii_media_status |= IFM_ACTIVE;

	PHY_READ(sc, MII_BMCR, &bmcr);
	if ((bmcr & BMCR_ISO) != 0) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if ((bmcr & BMCR_LOOP) != 0)
		mii->mii_media_active |= IFM_LOOP;

	if ((ssr & JMPHY_SSR_SPD_DPLX_RESOLVED) == 0) {
		/* Erg, still trying, I guess... */
		mii->mii_media_active |= IFM_NONE;
		return;
	}

	switch ((ssr & JMPHY_SSR_SPEED_MASK)) {
	case JMPHY_SSR_SPEED_1000:
		mii->mii_media_active |= IFM_1000_T;
		/*
		 * jmphy(4) got a valid link so reset mii_ticks.
		 * Resetting mii_ticks is needed in order to
		 * detect link loss after auto-negotiation.
		 */
		sc->mii_ticks = 0;
		break;
	case JMPHY_SSR_SPEED_100:
		mii->mii_media_active |= IFM_100_TX;
		sc->mii_ticks = 0;
		break;
	case JMPHY_SSR_SPEED_10:
		mii->mii_media_active |= IFM_10_T;
		sc->mii_ticks = 0;
		break;
	default:
		mii->mii_media_active |= IFM_NONE;
		return;
	}

	if ((ssr & JMPHY_SSR_DUPLEX) != 0)
		mii->mii_media_active |= IFM_FDX | mii_phy_flowstatus(sc);
	else
		mii->mii_media_active |= IFM_HDX;

	if (IFM_SUBTYPE(mii->mii_media_active) == IFM_1000_T) {
		PHY_READ(sc, MII_GTSR, &gtsr);
		if ((gtsr & GTSR_MS_RES) != 0)
			mii->mii_media_active |= IFM_ETH_MASTER;
	}
}

static void
jmphy_reset(struct mii_softc *sc)
{
	int i;
	uint16_t val;

	KASSERT(mii_locked(sc->mii_pdata));

	/* Disable sleep mode. */
	PHY_READ(sc, JMPHY_TMCTL, &val);
	PHY_WRITE(sc, JMPHY_TMCTL, val & ~JMPHY_TMCTL_SLEEP_ENB);

	PHY_READ(sc, MII_BMCR, &val);
	PHY_WRITE(sc, MII_BMCR, val | BMCR_RESET);

	for (i = 0; i < 1000; i++) {
		DELAY(1);
		PHY_READ(sc, MII_BMCR, &val);
		if ((val & BMCR_RESET) == 0)
			break;
	}
}

static uint16_t
jmphy_anar(struct ifmedia_entry *ife)
{
	uint16_t anar;

	anar = 0;
	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
		anar |= ANAR_TX_FD | ANAR_TX | ANAR_10_FD | ANAR_10;
		break;
	case IFM_1000_T:
		break;
	case IFM_100_TX:
		anar |= ANAR_TX | ANAR_TX_FD;
		break;
	case IFM_10_T:
		anar |= ANAR_10 | ANAR_10_FD;
		break;
	default:
		break;
	}

	return anar;
}

static int
jmphy_auto(struct mii_softc *sc, struct ifmedia_entry *ife)
{
	uint16_t anar, bmcr, gig;

	KASSERT(mii_locked(sc->mii_pdata));

	gig = 0;
	PHY_READ(sc, MII_BMCR, &bmcr);
	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
		gig |= GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		break;
	case IFM_1000_T:
		gig |= GTCR_ADV_1000TFDX | GTCR_ADV_1000THDX;
		break;
	case IFM_100_TX:
	case IFM_10_T:
		break;
	case IFM_NONE:
		PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_ISO | BMCR_PDOWN);
		return EJUSTRETURN;
	default:
		return EINVAL;
	}

	if ((ife->ifm_media & IFM_LOOP) != 0)
		bmcr |= BMCR_LOOP;

	anar = jmphy_anar(ife);
	if (sc->mii_flags & MIIF_DOPAUSE)
		anar |= ANAR_PAUSE_TOWARDS;

	if ((sc->mii_flags & MIIF_HAVE_GTCR) != 0) {
#ifdef notyet
		struct mii_data *mii;

		mii = sc->mii_pdata;
		if ((mii->mii_media.ifm_media & IFM_ETH_MASTER) != 0)
			gig |= GTCR_MAN_MS | GTCR_MAN_ADV;
#endif
		PHY_WRITE(sc, MII_100T2CR, gig);
	}
	PHY_WRITE(sc, MII_ANAR, anar | ANAR_CSMA);
	PHY_WRITE(sc, MII_BMCR, bmcr | BMCR_AUTOEN | BMCR_STARTNEG);

	return EJUSTRETURN;
}
