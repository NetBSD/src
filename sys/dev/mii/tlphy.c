/*  $NetBSD: tlphy.c,v 1.1.2.2 1997/11/17 02:07:07 thorpej Exp $   */
 
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
 *  This product includes software developed by Manuel Bouyer.
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

 /* Driver for Texas Instruments's ThunderLAN internal PHY */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_media.h>


#include <dev/mii/mii_adapter.h>
#include <dev/mii/mii_adapters_id.h>
#include <dev/mii/mii_phy.h>
#include <dev/mii/generic_phy.h>
#include <dev/mii/tlphy.h>

static int tlphy_up __P((struct phy_softc *sc));
void tlphy_pdown __P((void *v));
int tlphy_media_set __P((int, void *));
int tlphy_media_set_aui __P((struct phy_softc *));
int tlphy_status __P((int, void*));

#ifdef __BROKEN_INDIRECT_CONFIG
int tlphymatch __P((struct device *, void *, void *));
#else
int tlphymatch __P((struct device *, struct cfdata *, void *));
#endif
void tlphyattach __P((struct device *, struct device *, void *));

struct cfattach tlphy_ca = {
	sizeof(struct phy_softc), tlphymatch, tlphyattach
};

struct cfdriver tlphy_cd = {
	NULL, "tlphy", DV_IFNET
};

int
tlphymatch(parent, match, aux)
    struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	mii_phy_t *phy = aux;

	if (phy->phy_id == 0x40005013 || phy->phy_id == 0x40005014 ||
	    phy->phy_id == 0x40005015)
		return 1;
	return 0;
}

void
tlphyattach(parent, self, aux)
    struct device *parent, *self;
	void *aux;
{
	struct phy_softc *sc = (struct phy_softc *)self;

	sc->phy_link = aux;
	sc->phy_link->phy_softc = sc;
	sc->phy_link->phy_media_set = tlphy_media_set; 
	sc->phy_link->phy_status = tlphy_status;
	sc->phy_link->phy_pdown = tlphy_pdown;

	phy_reset(sc);

	switch (sc->phy_link->adapter_id) {
	case COMPAQ_INT_NETLIGENT_10_100:
		sc->phy_link->phy_media = PHY_BNC;
		break;
	case COMPAQ_NETLIGENT_10_100:
		sc->phy_link->phy_media = 0;
		break;
	case COMPAQ_INT_NETFLEX:
	case COMPAQ_NETFLEX_BNC:
		sc->phy_link->phy_media = PHY_BNC | PHY_10baseT;
		break;
	case TI_TLAN:
		sc->phy_link->phy_media = PHY_10baseT;
		break;
	default:
		sc->phy_link->phy_media = PHY_AUI;
		if (phy_media_probe(sc) != 0) {
			printf(" (autoconfig failed)");
			break;
		}
	}

	if(sc->phy_link->phy_media) {
		printf(": ");
		phy_media_print(sc->phy_link->phy_media);
	}
	printf("\n");
}

void tlphy_pdown(v)
	void *v;
{
	struct phy_softc *sc = v;
	mii_phy_t *phy_link = sc->phy_link;
	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_CONTROL,
		CTRL_ISO | CTRL_PDOWN);
}

static int tlphy_up(sc)
	struct phy_softc *sc;
{
	int s;
	mii_phy_t *phy_link = sc->phy_link;
	int control, i;
	s = splnet();

	/* set control registers to a reasonable value, wait for power_up */
	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_CONTROL,
		CTRL_LOOPBK);
	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_TL_CTRL,
		0);
	i = 0;
	do {
		DELAY(100000);
		control = mii_readreg(phy_link->mii_softc, phy_link->dev,
			PHY_TL_ST);
	} while ((control < 0 || (control & TL_ST_PHOK) == 0) && ++i < 10);
	if (control < 0 || (control & TL_ST_PHOK) == 0) {
		splx(s);
		printf("%s: power-up failed\n", sc->sc_dev.dv_xname);
		return 0;
	}
	if ( phy_reset(sc) == 0) {
		splx(s);
		return 0;
	}
	splx(s);
	return 1;
}

int tlphy_media_set(media, v)
	int media;
	void *v;
{
	struct phy_softc *sc = v;
	int subtype;

	if (IFM_TYPE(media) != IFM_ETHER)
		return EINVAL;

	if (tlphy_up(sc) == 0)
		return EIO;
	
	subtype = IFM_SUBTYPE(media);
	switch (subtype) {
	case IFM_10_2:
	case IFM_10_5:
		if ((subtype == IFM_10_2 && (sc->phy_link->phy_media & PHY_BNC)) ||
			(subtype == IFM_10_5 && (sc->phy_link->phy_media & PHY_AUI)))
			return tlphy_media_set_aui(sc);
		else
			return EINVAL;
	case IFM_10_T:
		return phy_media_set_10_100(sc, media);
	case IFM_AUTO:
		return ENODEV;
	default:
		return EINVAL;
	}
}

int
tlphy_media_set_aui(sc)
	struct phy_softc *sc;
{
	mii_phy_t *phy_link = sc->phy_link;

	mii_writereg(phy_link->mii_softc, phy_link->dev,
		PHY_CONTROL, 0);
	mii_writereg(phy_link->mii_softc, phy_link->dev,
		PHY_TL_CTRL, TL_CTRL_AUISEL);
	DELAY(100000);
	return 0;
}

int tlphy_status(media, v)
	int media;
	void *v;
{
	struct phy_softc *sc = v;
	int reg;
	
	if (IFM_SUBTYPE(media) == IFM_10_T)
		return phy_status(media, sc);
	reg = mii_readreg(sc->phy_link->mii_softc, sc->phy_link->dev,
		PHY_TL_ST);
	if ((reg & PHY_TL_ST) == 0)
		return ENETDOWN;
	return 0;
}
