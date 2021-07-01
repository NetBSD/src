/*	$NetBSD: dl10019.c,v 1.17 2021/07/01 20:39:15 thorpej Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dl10019.c,v 1.17 2021/07/01 20:39:15 thorpej Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/mii/miivar.h>
#include <dev/mii/mii.h>
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/dp8390reg.h>
#include <dev/ic/dp8390var.h>

#include <dev/ic/ne2000reg.h>
#include <dev/ic/ne2000var.h>

#include <dev/ic/dl10019reg.h>
#include <dev/ic/dl10019var.h>

int	dl10019_mii_readreg(device_t, int, int, uint16_t *);
int	dl10019_mii_writereg(device_t, int, int, uint16_t);
void	dl10019_mii_statchg(struct ifnet *);

/*
 * MII bit-bang glue.
 */
u_int32_t dl10019_mii_bitbang_read(device_t);
void dl10019_mii_bitbang_write(device_t, u_int32_t);

const struct mii_bitbang_ops dl10019_mii_bitbang_ops = {
	dl10019_mii_bitbang_read,
	dl10019_mii_bitbang_write,
	{
		DL0_GPIO_MII_DATAOUT,	/* MII_BIT_MDO */
		DL0_GPIO_MII_DATAIN,	/* MII_BIT_MDI */
		DL0_GPIO_MII_CLK,	/* MII_BIT_MDC */
		DL0_19_GPIO_MII_DIROUT,	/* MII_BIT_DIR_HOST_PHY */
		0,			/* MII_BIT_DIR_PHY_HOST */
	}
};

const struct mii_bitbang_ops dl10022_mii_bitbang_ops = {
	dl10019_mii_bitbang_read,
	dl10019_mii_bitbang_write,
	{
		DL0_GPIO_MII_DATAOUT,	/* MII_BIT_MDO */
		DL0_GPIO_MII_DATAIN,	/* MII_BIT_MDI */
		DL0_GPIO_MII_CLK,	/* MII_BIT_MDC */
		DL0_22_GPIO_MII_DIROUT,	/* MII_BIT_DIR_HOST_PHY */
		0,			/* MII_BIT_DIR_PHY_HOST */
	}
};

static void
dl10019_mii_reset(struct dp8390_softc *sc)
{
	struct ne2000_softc *nsc = (void *) sc;
	int i;

	if (nsc->sc_type != NE2000_TYPE_DL10022)
		return;

	for (i = 0; i < 2; i++) {
		bus_space_write_1(sc->sc_regt, sc->sc_regh, NEDL_DL0_GPIO,
		    0x08);
		delay(1);
		bus_space_write_1(sc->sc_regt, sc->sc_regh, NEDL_DL0_GPIO,
		    0x0c);
		delay(1);
	}
	bus_space_write_1(sc->sc_regt, sc->sc_regh, NEDL_DL0_GPIO, 0x00);
}

static void
dl10019_tick(void *arg)
{
	struct dp8390_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_schedule(&sc->sc_tick_ch, hz);
}

void
dl10019_media_init(struct dp8390_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ec.ec_if;
	struct mii_data *mii = &sc->sc_mii;

	callout_setfunc(&sc->sc_tick_ch, dl10019_tick, sc);

	sc->sc_ec.ec_mii = mii;

	mii->mii_ifp = ifp;
	mii->mii_readreg = dl10019_mii_readreg;
	mii->mii_writereg = dl10019_mii_writereg;
	mii->mii_statchg = dl10019_mii_statchg;
	ifmedia_init(&mii->mii_media, IFM_IMASK, dp8390_mediachange,
	    dp8390_mediastatus);

	dl10019_mii_reset(sc);

	mii_attach(sc->sc_dev, mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);
}

void
dl10019_media_fini(struct dp8390_softc *sc)
{

	callout_stop(&sc->sc_tick_ch);
	mii_detach(&sc->sc_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	/* dp8390_detach() will call ifmedia_fini(). */
}

int
dl10019_mediachange(struct dp8390_softc *sc)
{
	int rc;

	if ((rc = mii_mediachg(&sc->sc_mii)) == ENXIO)
		return 0;
	return rc;
}

void
dl10019_mediastatus(struct dp8390_softc *sc, struct ifmediareq *ifmr)
{

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

void
dl10019_init_card(struct dp8390_softc *sc)
{

	dl10019_mii_reset(sc);
	mii_mediachg(&sc->sc_mii);
	callout_schedule(&sc->sc_tick_ch, hz);
}

void
dl10019_stop_card(struct dp8390_softc *sc)
{

	callout_stop(&sc->sc_tick_ch);
	mii_down(&sc->sc_mii);
}

u_int32_t
dl10019_mii_bitbang_read(device_t self)
{
	struct dp8390_softc *sc = device_private(self);

	/* We're already in Page 0. */
	return (bus_space_read_1(sc->sc_regt, sc->sc_regh, NEDL_DL0_GPIO) &
	    ~DL0_GPIO_PRESERVE);
}

void
dl10019_mii_bitbang_write(device_t self, u_int32_t val)
{
	struct dp8390_softc *sc = device_private(self);
	u_int8_t gpio;

	/* We're already in Page 0. */
	gpio = bus_space_read_1(sc->sc_regt, sc->sc_regh, NEDL_DL0_GPIO);
	bus_space_write_1(sc->sc_regt, sc->sc_regh, NEDL_DL0_GPIO,
	    (val & ~DL0_GPIO_PRESERVE) | (gpio & DL0_GPIO_PRESERVE));
}

int
dl10019_mii_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	struct ne2000_softc *nsc = device_private(self);
	const struct mii_bitbang_ops *ops;

	ops = (nsc->sc_type == NE2000_TYPE_DL10022) ?
	    &dl10022_mii_bitbang_ops : &dl10019_mii_bitbang_ops;

	return mii_bitbang_readreg(self, ops, phy, reg, val);
}

int
dl10019_mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct ne2000_softc *nsc = device_private(self);
	const struct mii_bitbang_ops *ops;

	ops = (nsc->sc_type == NE2000_TYPE_DL10022) ?
	    &dl10022_mii_bitbang_ops : &dl10019_mii_bitbang_ops;

	return mii_bitbang_writereg(self, ops, phy, reg, val);
}

void
dl10019_mii_statchg(struct ifnet *ifp)
{
	struct ne2000_softc *nsc = ifp->if_softc;
	struct dp8390_softc *sc = &nsc->sc_dp8390;

	/*
	 * Disable collision detection on the DL10022 if
	 * we are on a full-duplex link.
	 */
	if (nsc->sc_type == NE2000_TYPE_DL10022) {
		u_int8_t diag;

		if (sc->sc_mii.mii_media_active & IFM_FDX)
			diag = DL0_DIAG_NOCOLLDETECT;
		else
			diag = 0;
		bus_space_write_1(sc->sc_regt, sc->sc_regh,
		    NEDL_DL0_DIAG, diag);
	}
}
