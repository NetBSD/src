/*	$NetBSD: if_mec.c,v 1.3 2000/07/03 12:50:09 soren Exp $	*/

/*
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
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
 * MACE MAC-110 ethernet driver
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <machine/endian.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#if NBPFILTER > 0 
#include <net/bpf.h>
#endif 

#ifdef INET
#include <netinet/in.h> 
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <sgimips/dev/macevar.h>

#include <sgimips/dev/if_mecreg.h>

struct mec_softc {
	struct device sc_dev;

	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;
	bus_dma_tag_t sc_dmat;

	struct ethercom sc_ethercom;

	unsigned char sc_enaddr[6];

	void *sc_sdhook;

	struct mii_data sc_mii;
	struct callout sc_callout;

#if NRND > 0
	rndsource_element_t rnd_source;	/* random source */
#endif
};

static int	mec_match(struct device *, struct cfdata *, void *);
static void	mec_attach(struct device *, struct device *, void *);
#if 0
static void	epic_start(struct ifnet *);
static void	epic_watchdog(struct ifnet *);
static int	epic_ioctl(struct ifnet *, u_long, caddr_t);
#endif
static int	mec_mii_readreg(struct device *, int, int);
static void	mec_mii_writereg(struct device *, int, int, int);
static int	mec_mii_wait(struct mec_softc *);
static void	mec_statchg(struct device *);
static int	mec_mediachange(struct ifnet *);
static void	mec_mediastatus(struct ifnet *, struct ifmediareq *);

struct cfattach mec_ca = {
	sizeof(struct mec_softc), mec_match, mec_attach
};

static int
mec_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	return 1;
}

static void
mec_attach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	struct mec_softc *sc = (void *)self;
	struct mace_attach_args *maa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if; 
	u_int64_t address, command;
	int i;

	sc->sc_st = maa->maa_st;
	sc->sc_sh = maa->maa_sh;

	printf(": MAC-110 Ethernet, ");
	command = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL);
	printf("rev %lld\n", (command & MAC_REVISION) >> MAC_REVISION_SHIFT);

	/*
	 * The firmware has left us the station address.
	 */
	address = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_STATION);
	for (i = 0; i < 6; i++) {
		sc->sc_enaddr[5 - i] = address & 0xff;
		address >>= 8;
	}

	printf("%s: station address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(sc->sc_enaddr));

	/*
	 * Reset device.
	 */
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, 0);
	delay(1000);

	printf("%s: sorry, this is not a real driver\n", sc->sc_dev.dv_xname);

#if 0
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = mec_ioctl;
	ifp->if_start = mec_start;
	ifp->if_watchdog = mec_watchdog;
#endif

        sc->sc_mii.mii_ifp = ifp;
        sc->sc_mii.mii_readreg = mec_mii_readreg;
        sc->sc_mii.mii_writereg = mec_mii_writereg;
        sc->sc_mii.mii_statchg = mec_statchg;

        ifmedia_init(&sc->sc_mii.mii_media, 0, mec_mediachange,
	    mec_mediastatus);
        mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
            MII_OFFSET_ANY, 0);
        if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
                ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
                ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
        } else
                ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

return; /* XXX */

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

#if NBPFILTER > 0
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf,
	    ifp, DLT_EN10MB, sizeof (struct ether_header));
#endif  

}

int
mec_mii_readreg(self, phy, reg)
	struct device *self;
	int phy;
	int reg;
{
	struct mec_softc *sc = (struct mec_softc *)self;
	u_int64_t val;
	int i;

	if (mec_mii_wait(sc) != 0)
		return 0;

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_ADDRESS,
	    phy << PHY_ADDR_DEVSHIFT | reg);

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_READ_INITATE, 1);

	for (i = 0; i < 20; i++) {
		delay(30);

		val = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_PHY_DATA);

		if ((val & PHY_DATA_BUSY) == 0)
			return (int)val & PHY_DATA_VALUE;
	}

	return 0;
}

void
mec_mii_writereg(self, phy, reg, val)
	struct device *self;
	int phy, reg, val;
{
	struct mec_softc *sc = (struct mec_softc *)self;

	if (mec_mii_wait(sc) != 0)
		return;

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_ADDRESS,
	    phy << PHY_ADDR_DEVSHIFT | reg);

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_DATA,
	    val & PHY_DATA_VALUE);

	(void)mec_mii_wait(sc);

	return;
}

int
mec_mii_wait(sc)
	struct mec_softc *sc;
{
	int i;

	for (i = 0; i < 100; i++) {
		u_int64_t busy;

		delay(30);

		busy = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_PHY_DATA);

		if ((busy & PHY_DATA_BUSY) == 0)
			return 0;
		if (busy == 0xffff)
			return 0;
	}

	printf("%s: MII timed out\n", sc->sc_dev.dv_xname);
	return 1;
}

void
mec_statchg(self)
	struct device *self;
{
#if 0
	struct mec_softc *sc = (void *)self;
#endif

	return;
}

void
mec_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct mec_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

int
mec_mediachange(ifp)
	struct ifnet *ifp;
{
	struct mec_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return 0;
}
