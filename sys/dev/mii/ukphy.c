/*	$NetBSD: ukphy.c,v 1.30.34.1 2007/12/26 19:46:38 ad Exp $	*/

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
 * driver for generic unknown PHYs
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ukphy.c,v 1.30.34.1 2007/12/26 19:46:38 ad Exp $");

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

#ifdef MIIVERBOSE
struct mii_knowndev {
	int oui;
	int model;
	const char *descr;
};
#include <dev/mii/miidevs.h>
#include <dev/mii/miidevs_data.h>
#endif

static int	ukphymatch(struct device *, struct cfdata *, void *);
static void	ukphyattach(struct device *, struct device *, void *);

CFATTACH_DECL(ukphy, sizeof(struct mii_softc),
    ukphymatch, ukphyattach, mii_phy_detach, mii_phy_activate);

static int	ukphy_service(struct mii_softc *, struct mii_data *, int);

static const struct mii_phy_funcs ukphy_funcs = {
	ukphy_service, ukphy_status, mii_phy_reset,
};

static int
ukphymatch(struct device *parent, struct cfdata *match,
    void *aux)
{

	/*
	 * We know something is here, so always match at a low priority.
	 */
	return (1);
}

static void
ukphyattach(struct device *parent, struct device *self, void *aux)
{
	struct mii_softc *sc = device_private(self);
	struct mii_attach_args *ma = aux;
	struct mii_data *mii = ma->mii_data;
	int oui = MII_OUI(ma->mii_id1, ma->mii_id2);
	int model = MII_MODEL(ma->mii_id2);
	int rev = MII_REV(ma->mii_id2);
#ifdef MIIVERBOSE
	int i;
#endif

	aprint_naive(": Media interface\n");
	aprint_normal(": Generic IEEE 802.3u media interface\n");
#ifdef MIIVERBOSE
	for (i = 0; mii_knowndevs[i].descr != NULL; i++)
		if (mii_knowndevs[i].oui == oui &&
		    mii_knowndevs[i].model == model)
			break;
	if (mii_knowndevs[i].descr != NULL)
		aprint_normal("%s: %s (OUI 0x%06x, model 0x%04x), rev. %d\n",
		       sc->mii_dev.dv_xname, mii_knowndevs[i].descr,
		       oui, model, rev);
	else
#endif
		aprint_normal("%s: OUI 0x%06x, model 0x%04x, rev. %d\n",
		       sc->mii_dev.dv_xname, oui, model, rev);

	sc->mii_inst = mii->mii_instance;
	sc->mii_phy = ma->mii_phyno;
	sc->mii_funcs = &ukphy_funcs;
	sc->mii_pdata = mii;
	sc->mii_flags = ma->mii_flags;
	sc->mii_anegticks = MII_ANEGTICKS;

	/*
	 * Don't do loopback on unknown PHYs.  It might confuse some of them.
	 */
	sc->mii_flags |= MIIF_NOLOOP;

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

	if (!pmf_device_register(self, NULL, mii_phy_resume))
		aprint_error_dev(self, "couldn't establish power handler\n");
}

static int
ukphy_service(struct mii_softc *sc, struct mii_data *mii, int cmd)
{
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	int reg;

	if (!device_is_active(&sc->mii_dev))
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

		/*
		 * Only used for autonegotiation.
		 */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO)
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
