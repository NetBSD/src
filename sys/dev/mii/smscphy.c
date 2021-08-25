/* $NetBSD: smscphy.c,v 1.4 2021/08/25 21:50:29 andvar Exp $ */

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2006 Benno Rice.  All rights reserved.
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

/* $FreeBSD: head/sys/dev/mii/smscphy.c 326255 2017-11-27 14:52:40Z pfg $ */

/*
 * Driver for the SMSC LAN8710A
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: smscphy.c,v 1.4 2021/08/25 21:50:29 andvar Exp $");

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

/* PHY special control/status register */
#define SMSCPHY_SPCSR	0x1f
#define  SPCSR_SPDIND_10	0x0004
#define  SPCSR_SPDIND_100	0x0008
#define  SPCSR_SPDIND_SPDMASK	0x000c
#define  SPCSR_SPDIND_FDX	0x0010

static int	smscphy_match(device_t, cfdata_t, void *);
static void	smscphy_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(smscphy, sizeof (struct mii_softc),
    smscphy_match, smscphy_attach, mii_phy_detach, mii_phy_activate);

static void	smscphy_power(struct mii_softc *, bool);
static int	smscphy_service(struct mii_softc *, struct mii_data *, int);
static void	smscphy_auto(struct mii_softc *, int);
static void	smscphy_status(struct mii_softc *);

static const struct mii_phydesc smscphys[] = {
	MII_PHY_DESC(SMSC, LAN8700),
	MII_PHY_DESC(SMSC, LAN8710_LAN8720),
	MII_PHY_END
};

static const struct mii_phy_funcs smscphy_funcs = {
	smscphy_service,
	smscphy_status,
	mii_phy_reset
};

static int
smscphy_match(device_t dev, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, smscphys) != NULL)
		return 10;

	return 0;
}

static void
smscphy_attach(device_t parent, device_t self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, smscphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &smscphy_funcs;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;

	mii_lock(mii);

	PHY_RESET(sc);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static void
smscphy_power(struct mii_softc *sc, bool power)
{
	uint16_t bmcr, new;

	PHY_READ(sc, MII_BMCR, &bmcr);
	if (power)
		new = bmcr & ~BMCR_PDOWN;
	else
		new = bmcr | BMCR_PDOWN;
	if (bmcr != new)
		PHY_WRITE(sc, MII_BMCR, new);
}

static int
smscphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct	ifmedia_entry *ife;
	uint16_t reg;

	KASSERT(mii_locked(mii));

	ife = mii->mii_media.ifm_cur;

	switch (cmd) {
	case MII_POLLSTAT:
		break;

	case MII_MEDIACHG:
		/* Try to power up the PHY in case it's down */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_NONE)
			smscphy_power(sc, true);

		switch (IFM_SUBTYPE(ife->ifm_media)) {
		case IFM_AUTO:
			smscphy_auto(sc, ife->ifm_media);
			break;

		default:
			mii_phy_setmedia(sc);
			if (IFM_SUBTYPE(ife->ifm_media) == IFM_NONE)
				smscphy_power(sc, false);
			break;
		}
		break;

	case MII_TICK:
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO)
			break;

		PHY_READ(sc, MII_BMSR, &reg);
		PHY_READ(sc, MII_BMSR, &reg);
		if (reg & BMSR_LINK) {
			sc->mii_ticks = 0;
			break;
		}

		if (++sc->mii_ticks <= MII_ANEGTICKS)
			break;

		PHY_RESET(sc);
		smscphy_auto(sc, ife->ifm_media);
		break;
	}

	/* Update the media status. */
	PHY_STATUS(sc);

	/* Callback if something changed. */
	mii_phy_update(sc, cmd);
	return 0;
}

static void
smscphy_auto(struct mii_softc *sc, int media)
{
	uint16_t anar;

	sc->mii_ticks = 0;
	anar = BMSR_MEDIA_TO_ANAR(sc->mii_capabilities) | ANAR_CSMA;
	if ((media & IFM_FLOW) != 0)
		anar |= ANAR_FC;
	PHY_WRITE(sc, MII_ANAR, anar);
	/* Apparently this helps. */
	PHY_READ(sc, MII_ANAR, &anar);
	PHY_WRITE(sc, MII_BMCR, BMCR_AUTOEN | BMCR_STARTNEG);
}

static void
smscphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	uint16_t bmcr, bmsr, status;

	KASSERT(mii_locked(mii));

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	PHY_READ(sc, MII_BMSR, &bmsr);
	PHY_READ(sc, MII_BMSR, &bmsr);
	if ((bmsr & BMSR_LINK) != 0)
		mii->mii_media_status |= IFM_ACTIVE;

	PHY_READ(sc, MII_BMCR, &bmcr);
	if ((bmcr & BMCR_ISO) != 0) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if ((bmcr & BMCR_LOOP) != 0)
		mii->mii_media_active |= IFM_LOOP;

	if ((bmcr & BMCR_AUTOEN) != 0) {
		if ((bmsr & BMSR_ACOMP) == 0) {
			/* Erg, still trying, I guess... */
			mii->mii_media_active |= IFM_NONE;
			return;
		}
	}

	PHY_READ(sc, SMSCPHY_SPCSR, &status);
	if ((status & SPCSR_SPDIND_SPDMASK) == SPCSR_SPDIND_100)
		mii->mii_media_active |= IFM_100_TX;
	else
		mii->mii_media_active |= IFM_10_T;
	if (status & SPCSR_SPDIND_FDX)
		mii->mii_media_active |= IFM_FDX | mii_phy_flowstatus(sc);
	else
		mii->mii_media_active |= IFM_HDX;
}
