/* $NetBSD: gxlphy.c,v 1.5 2021/06/29 21:03:36 pgoyette Exp $ */

/*
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
 * Amlogic Meson GXL 10/100 internal PHY
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gxlphy.c,v 1.5 2021/06/29 21:03:36 pgoyette Exp $");

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

#define	TSTCNTL		20
#define	 TSTCNTL_READ		__BIT(15)
#define	 TSTCNTL_WRITE		__BIT(14)
#define	 TSTCNTL_REG_BANK_SEL	__BITS(12,11)
#define	 TSTCNTL_TEST_MODE	__BIT(10)
#define	 TSTCNTL_READ_ADDR	__BITS(9,5)
#define	 TSTCNTL_WRITE_ADDR	__BITS(4,0)
#define	TSTREAD1	21
#define	TSTWRITE	23

#define	BANK_WOL	1
#define	BANK_BIST	3

#define	LPI_STATUS	0x0c
#define	 LPI_STATUS_RSV12	__BIT(12)

#define	BIST_PLL_CTRL	0x1b
#define	BIST_PLL_DIV0	0x1c
#define	BIST_PLL_DIV1	0x1d

static int	gxlphymatch(device_t, cfdata_t, void *);
static void	gxlphyattach(device_t, device_t, void *);

CFATTACH_DECL_NEW(gxlphy, sizeof(struct mii_softc),
    gxlphymatch, gxlphyattach, mii_phy_detach, mii_phy_activate);

static int	gxlphy_service(struct mii_softc *, struct mii_data *, int);
static void	gxlphy_status(struct mii_softc *);

static const struct mii_phy_funcs gxlphy_funcs = {
	gxlphy_service, gxlphy_status, mii_phy_reset,
};

static const struct mii_phydesc gxlphys[] = {
	MII_PHY_DESC(xxAMLOGIC, GXL),
	MII_PHY_END,
};

static void
gxl_openbanks(struct mii_softc *sc)
{
	PHY_WRITE(sc, TSTCNTL, 0);
	PHY_WRITE(sc, TSTCNTL, TSTCNTL_TEST_MODE);
	PHY_WRITE(sc, TSTCNTL, 0);
	PHY_WRITE(sc, TSTCNTL, TSTCNTL_TEST_MODE);
}

static void
gxl_closebanks(struct mii_softc *sc)
{
	PHY_WRITE(sc, TSTCNTL, 0);
}

static uint16_t
gxl_readreg(struct mii_softc *sc, u_int bank, u_int reg)
{
	uint16_t val;

	gxl_openbanks(sc);
	PHY_WRITE(sc, TSTCNTL,
	    TSTCNTL_READ | TSTCNTL_TEST_MODE |
	    __SHIFTIN(bank, TSTCNTL_REG_BANK_SEL) |
	    __SHIFTIN(reg, TSTCNTL_READ_ADDR));
	PHY_READ(sc, TSTREAD1, &val);
	gxl_closebanks(sc);

	return val;
}

static void
gxl_writereg(struct mii_softc *sc, u_int bank, u_int reg, uint16_t val)
{
	gxl_openbanks(sc);
	PHY_WRITE(sc, TSTWRITE, val);
	PHY_WRITE(sc, TSTCNTL,
	    TSTCNTL_WRITE | TSTCNTL_TEST_MODE |
	    __SHIFTIN(bank, TSTCNTL_REG_BANK_SEL) |
	    __SHIFTIN(reg, TSTCNTL_WRITE_ADDR));
	gxl_closebanks(sc);
}

static int
gxlphymatch(device_t parent, cfdata_t match, void *aux)
{
	struct mii_attach_args *ma = aux;

	if (mii_phy_match(ma, gxlphys) != NULL)
		return 20;

	return 0;
}

static void
gxlphyattach(device_t parent, device_t self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	int oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	int model = MII_MODEL(ma->mii_id2);
	int rev = MII_REV(ma->mii_id2);
	char descr[MII_MAX_DESCR_LEN];

	mii_get_descr(descr, sizeof(descr), oui, model);
	if (descr[0])
		aprint_normal(": %s (OUI 0x%06x, model 0x%04x), rev. %d\n",
		       descr, oui, model, rev);
	else
		aprint_normal(": OUI 0x%06x, model 0x%04x, rev. %d\n",
		       oui, model, rev);
	aprint_naive(": Media interface\n");

	sc->mii_dev = self;
	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_mpd_oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	sc->mii_mpd_model = MII_MODEL(ma->mii_id2);
	sc->mii_mpd_rev = MII_REV(ma->mii_id2);
	sc->mii_funcs = &gxlphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;

	mii_lock(mii);

	PHY_RESET(sc);

	gxl_writereg(sc, BANK_BIST, BIST_PLL_CTRL, 0x5);
	gxl_writereg(sc, BANK_BIST, BIST_PLL_DIV1, 0x029a);
	gxl_writereg(sc, BANK_BIST, BIST_PLL_DIV0, 0xaaaa);

	PHY_READ(sc, MII_BMSR, &sc->mii_capabilities);
	sc->mii_capabilities &= ma->mii_capmask;
	if (sc->mii_capabilities & BMSR_EXTSTAT)
		PHY_READ(sc, MII_EXTSR, &sc->mii_extcapabilities);

	mii_unlock(mii);

	mii_phy_add_media(sc);
}

static int
gxlphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
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
gxlphy_status(struct mii_softc *sc)
{
	uint16_t bmcr, bmsr, wol, lpa, aner;

	KASSERT(mii_locked(sc->mii_pdata));

	PHY_READ(sc, MII_BMCR, &bmcr);
	if ((bmcr & BMCR_AUTOEN) == 0)
		goto done;

	PHY_READ(sc, MII_BMSR, &bmsr);
	if ((bmsr & BMSR_ACOMP) == 0)
		goto done;

	wol = gxl_readreg(sc, BANK_WOL, LPI_STATUS);
	PHY_READ(sc, MII_ANLPAR, &lpa);
	PHY_READ(sc, MII_ANER, &aner);

	if ((wol & LPI_STATUS_RSV12) == 0 ||
	    ((aner & ANER_LPAN) != 0 && (lpa & ANLPAR_ACK) == 0)) {
		device_printf(sc->mii_dev, "LPA corruption - aneg restart\n");

		bmcr &= ~BMCR_ISO;
		bmcr |= BMCR_AUTOEN;
		bmcr |= BMCR_STARTNEG;
		PHY_WRITE(sc, MII_BMCR, bmcr);

		return;
	}

done:
	ukphy_status(sc);
}
