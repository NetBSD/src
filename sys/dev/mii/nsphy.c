/*	$NetBSD: nsphy.c,v 1.2 1997/11/17 09:02:27 thorpej Exp $	*/
 
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
  * driver for National Semiconductor's DP83840A ethernet 10/100 PHY
  * Data Sheet available from www.national.com
  */

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

void	nsphy_pdown __P((void *v));
int	nsphy_media_set __P((int, void *));

#ifdef __BROKEN_INDIRECT_CONFIG
int	nsphymatch __P((struct device *, void *, void *));
#else
int	nsphymatch __P((struct device *, struct cfdata *, void *));
#endif
void	nsphyattach __P((struct device *, struct device *, void *));

struct cfattach nsphy_ca = {
	sizeof(struct phy_softc), nsphymatch, nsphyattach
};

struct cfdriver nsphy_cd = {
	NULL, "nsphy", DV_IFNET
};

int
nsphymatch(parent, match, aux)
	struct device *parent;
#ifdef __BROKEN_INDIRECT_CONFIG
	void *match;
#else
	struct cfdata *match;
#endif
	void *aux;
{
	mii_phy_t *phy = aux;

	if (phy->phy_id == 0x20005c01)
		return 1;
	return 0;
}

void
nsphyattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct phy_softc *sc = (struct phy_softc *)self;

	sc->phy_link = aux;
	sc->phy_link->phy_softc = sc;
	sc->phy_link->phy_media_set = nsphy_media_set; 
	sc->phy_link->phy_status = phy_status;
	sc->phy_link->phy_pdown = nsphy_pdown;

	phy_reset(sc);

	sc->phy_link->phy_media = 0;
	if (phy_media_probe(sc) != 0) {
		printf(": autoconfig failed\n");
		return;
	}
	printf(": ");
	phy_media_print(sc->phy_link->phy_media);
	printf("\n");
}

void
nsphy_pdown(v)
	void *v;
{
	struct phy_softc *sc = v;
	mii_phy_t *phy_link = sc->phy_link;

	mii_writereg(phy_link->mii_softc, phy_link->dev, PHY_CONTROL,
	    CTRL_ISO);
}

int
nsphy_media_set(media, v)
	int media;
	void *v;
{
	struct phy_softc *sc = v;
	int subtype;

	if (IFM_TYPE(media) != IFM_ETHER)
		return (EINVAL);

	if (phy_reset(sc) == 0)
		return (EIO);
	
	subtype = IFM_SUBTYPE(media);
	switch (subtype) {
	case IFM_10_2:
	case IFM_10_5:
		return (EINVAL);
	case IFM_10_T:
	case IFM_100_TX:
	case IFM_100_T4:
		return (phy_media_set_10_100(sc, media));
	case IFM_AUTO:
		return (ENODEV);
	default:
		return (EINVAL);
	}
}
