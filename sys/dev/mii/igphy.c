/*	$NetBSD: igphy.c,v 1.8 2006/10/12 01:31:25 christos Exp $	*/

/*
 * The Intel copyright applies to the analog register setup, and the
 * (currently disabled) SmartSpeed workaround code.
 */

/*******************************************************************************

  Copyright (c) 2001-2003, Intel Corporation
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   3. Neither the name of the Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/


/*-
 * Copyright (c) 1998, 1999, 2000, 2003 The NetBSD Foundation, Inc.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: igphy.c,v 1.8 2006/10/12 01:31:25 christos Exp $");

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

#include <dev/mii/igphyreg.h>

struct igphy_softc {
	struct mii_softc sc_mii;
	int sc_smartspeed;
};

static void igphy_reset(struct mii_softc *);
static void igphy_load_dspcode(struct mii_softc *);
static void igphy_smartspeed_workaround(struct mii_softc *sc);

static int	igphymatch(struct device *, struct cfdata *, void *);
static void	igphyattach(struct device *, struct device *, void *);

CFATTACH_DECL(igphy, sizeof(struct igphy_softc),
    igphymatch, igphyattach, mii_phy_detach, mii_phy_activate);

static int	igphy_service(struct mii_softc *, struct mii_data *, int);
static void	igphy_status(struct mii_softc *);

static const struct mii_phy_funcs igphy_funcs = {
	igphy_service, igphy_status, igphy_reset,
};

static const struct mii_phydesc igphys[] = {
	{ MII_OUI_yyINTEL,		MII_MODEL_yyINTEL_IGP01E1000,
	  MII_STR_yyINTEL_IGP01E1000 },

	{0,				0,
	 NULL },
};

static int
igphymatch(struct device *parent __unused, struct cfdata *match __unused,
    void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, igphys) != NULL)
		return 10;

	return 0;
}

static void
igphyattach(struct device *parent __unused, struct device *self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	const struct mii_phydesc *mpd;

	mpd = mii_phy_match(ma, igphys);
	aprint_naive(": Media interface\n");
	aprint_normal(": %s, rev. %d\n", mpd->mpd_name, MII_REV(ma->mii_id2));

	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &igphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_anegticks = 10;

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

static void
igphy_load_dspcode(struct mii_softc *sc)
{
	static const struct {
		int reg;
		uint16_t val;
	} dspcode[] = {
		{ 0x1f95, 0x0001 },
		{ 0x1f71, 0xbd21 },
		{ 0x1f79, 0x0018 },
		{ 0x1f30, 0x1600 },
		{ 0x1f31, 0x0014 },
		{ 0x1f32, 0x161c },
		{ 0x1f94, 0x0003 },
		{ 0x1f96, 0x003f },
		{ 0x2010, 0x0008 },
		{ 0, 0 },
	};
	int i;

	delay(10);

	PHY_WRITE(sc, MII_IGPHY_PAGE_SELECT, 0x0000);
	PHY_WRITE(sc, 0x0000, 0x0140);

	delay(5);

	for (i = 0; dspcode[i].reg != 0; i++)
		IGPHY_WRITE(sc, dspcode[i].reg, dspcode[i].val);

	PHY_WRITE(sc, MII_IGPHY_PAGE_SELECT,0x0000);
	PHY_WRITE(sc, 0x0000, 0x3300);
}

static void
igphy_reset(struct mii_softc *sc)
{
	uint16_t fused, fine, coarse;

	mii_phy_reset(sc);
	igphy_load_dspcode(sc);

	fused = IGPHY_READ(sc, MII_IGPHY_ANALOG_SPARE_FUSE_STATUS);
	if ((fused & ANALOG_SPARE_FUSE_ENABLED) == 0) {
		fused = IGPHY_READ(sc, MII_IGPHY_ANALOG_FUSE_STATUS);

		fine = fused & ANALOG_FUSE_FINE_MASK;
		coarse = fused & ANALOG_FUSE_COARSE_MASK;

		if (coarse > ANALOG_FUSE_COARSE_THRESH) {
			coarse -= ANALOG_FUSE_COARSE_10;
			fine -= ANALOG_FUSE_FINE_1;
		} else if (coarse == ANALOG_FUSE_COARSE_THRESH)
			fine -= ANALOG_FUSE_FINE_10;

		fused = (fused & ANALOG_FUSE_POLY_MASK) |
			(fine & ANALOG_FUSE_FINE_MASK) |
			(coarse & ANALOG_FUSE_COARSE_MASK);

		IGPHY_WRITE(sc, MII_IGPHY_ANALOG_FUSE_CONTROL, fused);
		IGPHY_WRITE(sc, MII_IGPHY_ANALOG_FUSE_BYPASS,
		    ANALOG_FUSE_ENABLE_SW_CONTROL);
	}
	PHY_WRITE(sc, MII_IGPHY_PAGE_SELECT,0x0000);
}


static int
igphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t reg;

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

		mii_phy_setmedia(sc);
		break;

	case MII_TICK:
		/*
		 * If we're not currently selected, just return.
		 */
		if (IFM_INST(ife->ifm_media) != sc->mii_inst)
			return (0);

		igphy_smartspeed_workaround(sc);

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
igphy_status(struct mii_softc *sc)
{
	struct mii_data *mii = sc->mii_pdata;
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	uint16_t bmcr, pssr, gtsr, bmsr;

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	pssr = PHY_READ(sc, MII_IGPHY_PORT_STATUS);

	if (pssr & PSSR_LINK_UP)
		mii->mii_media_status |= IFM_ACTIVE;

	bmcr = PHY_READ(sc, MII_BMCR);
	if (bmcr & BMCR_ISO) {
		mii->mii_media_active |= IFM_NONE;
		mii->mii_media_status = 0;
		return;
	}

	if (bmcr & BMCR_LOOP)
		mii->mii_media_active |= IFM_LOOP;

	bmsr = PHY_READ(sc, MII_BMSR) | PHY_READ(sc, MII_BMSR);

	/*
	 * XXX can't check if the info is valid, no
	 * 'negotiation done' bit?
	 */
	if (bmcr & BMCR_AUTOEN) {
		if ((bmsr & BMSR_ACOMP) == 0) {
			mii->mii_media_active |= IFM_NONE;
			return;
		}
		switch (pssr & PSSR_SPEED_MASK) {
		case PSSR_SPEED_1000MBPS:
			mii->mii_media_active |= IFM_1000_T;
			gtsr = PHY_READ(sc, MII_100T2SR);
			if (gtsr & GTSR_MS_RES)
				mii->mii_media_active |= IFM_ETH_MASTER;
			break;

		case PSSR_SPEED_100MBPS:
			mii->mii_media_active |= IFM_100_TX;
			break;

		case PSSR_SPEED_10MBPS:
			mii->mii_media_active |= IFM_10_T;
			break;

		default:
			mii->mii_media_active |= IFM_NONE;
			mii->mii_media_status = 0;
			return;
		}

		if (pssr & PSSR_FULL_DUPLEX)
			mii->mii_media_active |=
			    IFM_FDX | mii_phy_flowstatus(sc);
	} else
		mii->mii_media_active = ife->ifm_media;
}

static void
igphy_smartspeed_workaround(struct mii_softc *sc)
{
	struct igphy_softc *igsc = (struct igphy_softc *) sc;
	uint16_t reg, gtsr, gtcr;

	if ((PHY_READ(sc, MII_BMCR) & BMCR_AUTOEN) == 0)
		return;

	/* XXX Assume 1000TX-FDX is advertized if doing autonegotiation. */

	reg = PHY_READ(sc, MII_BMSR) | PHY_READ(sc, MII_BMSR);
	if ((reg & BMSR_LINK) == 0) {
		switch (igsc->sc_smartspeed) {
		case 0:
			gtsr = PHY_READ(sc, MII_100T2SR);
			if (!(gtsr & GTSR_MAN_MS_FLT))
				break;
			gtsr = PHY_READ(sc, MII_100T2SR);
			if (gtsr & GTSR_MAN_MS_FLT) {
				gtcr = PHY_READ(sc, MII_100T2CR);
				if (gtcr & GTCR_MAN_MS) {
					gtcr &= ~GTCR_MAN_MS;
					PHY_WRITE(sc, MII_100T2CR,
					    gtcr);
				}
				mii_phy_auto(sc, 0);
			}
			break;
		case IGPHY_TICK_DOWNSHIFT:
			gtcr = PHY_READ(sc, MII_100T2CR);
			gtcr |= GTCR_MAN_MS;
			PHY_WRITE(sc, MII_100T2CR, gtcr);
			mii_phy_auto(sc, 0);
			break;
		default:
			break;
		}
		if (igsc->sc_smartspeed++ == IGPHY_TICK_MAX)
			igsc->sc_smartspeed = 0;
	} else
		igsc->sc_smartspeed = 0;
}
