/*	$NetBSD: elinkxl.c,v 1.19 1999/11/17 17:56:52 thorpej Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden.
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

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/bus.h>
#include <machine/intr.h>

#if BYTE_ORDER == BIG_ENDIAN
#include <machine/bswap.h>
#define	htopci(x)	bswap32(x)
#define	pcitoh(x)	bswap32(x)
#else
#define	htopci(x)	(x)
#define	pcitoh(x)	(x)
#endif

#include <vm/vm.h>
#include <vm/pmap.h>

#include <dev/mii/miivar.h>
#include <dev/mii/mii.h>
#include <dev/mii/mii_bitbang.h>

#include <dev/ic/elink3reg.h>
/* #include <dev/ic/elink3var.h> */
#include <dev/ic/elinkxlreg.h>
#include <dev/ic/elinkxlvar.h>

#ifdef DEBUG
int exdebug = 0;
#endif

/* ifmedia callbacks */
int ex_media_chg __P((struct ifnet *ifp));
void ex_media_stat __P((struct ifnet *ifp, struct ifmediareq *req));

void ex_probe_media __P((struct ex_softc *));
void ex_set_filter __P((struct ex_softc *));
void ex_set_media __P((struct ex_softc *));
struct mbuf *ex_get __P((struct ex_softc *, int));
u_int16_t ex_read_eeprom __P((struct ex_softc *, int));
void ex_init __P((struct ex_softc *));
void ex_read __P((struct ex_softc *));
void ex_reset __P((struct ex_softc *));
void ex_set_mc __P((struct ex_softc *));
void ex_getstats __P((struct ex_softc *));
void ex_printstats __P((struct ex_softc *));
void ex_tick __P((void *));

static int ex_eeprom_busy __P((struct ex_softc *));
static int ex_add_rxbuf __P((struct ex_softc *, struct ex_rxdesc *));
static void ex_init_txdescs __P((struct ex_softc *));

static void ex_shutdown __P((void *));
static void ex_start __P((struct ifnet *));
static void ex_txstat __P((struct ex_softc *));
static u_int16_t ex_mchash __P((u_char *));

int ex_mii_readreg __P((struct device *, int, int));
void ex_mii_writereg __P((struct device *, int, int, int));
void ex_mii_statchg __P((struct device *));

void ex_probemedia __P((struct ex_softc *));

/*
 * Structure to map media-present bits in boards to ifmedia codes and
 * printable media names.  Used for table-driven ifmedia initialization.
 */
struct ex_media {
	int	exm_mpbit;		/* media present bit */
	const char *exm_name;		/* name of medium */
	int	exm_ifmedia;		/* ifmedia word for medium */
	int	exm_epmedia;		/* ELINKMEDIA_* constant */
};

/*
 * Media table for 3c90x chips.  Note that chips with MII have no
 * `native' media.
 */
struct ex_media ex_native_media[] = {
	{ ELINK_PCI_10BASE_T,	"10baseT",	IFM_ETHER|IFM_10_T,
	  ELINKMEDIA_10BASE_T },
	{ ELINK_PCI_10BASE_T,	"10baseT-FDX",	IFM_ETHER|IFM_10_T|IFM_FDX,
	  ELINKMEDIA_10BASE_T },
	{ ELINK_PCI_AUI,	"10base5",	IFM_ETHER|IFM_10_5,
	  ELINKMEDIA_AUI },
	{ ELINK_PCI_BNC,	"10base2",	IFM_ETHER|IFM_10_2,
	  ELINKMEDIA_10BASE_2 },
	{ ELINK_PCI_100BASE_TX,	"100baseTX",	IFM_ETHER|IFM_100_TX,
	  ELINKMEDIA_100BASE_TX },
	{ ELINK_PCI_100BASE_TX,	"100baseTX-FDX",IFM_ETHER|IFM_100_TX|IFM_FDX,
	  ELINKMEDIA_100BASE_TX },
	{ ELINK_PCI_100BASE_FX,	"100baseFX",	IFM_ETHER|IFM_100_FX,
	  ELINKMEDIA_100BASE_FX },
	{ ELINK_PCI_100BASE_MII,"manual",	IFM_ETHER|IFM_MANUAL,
	  ELINKMEDIA_MII },
	{ ELINK_PCI_100BASE_T4,	"100baseT4",	IFM_ETHER|IFM_100_T4,
	  ELINKMEDIA_100BASE_T4 },
	{ 0,			NULL,		0,
	  0 },
};

/*
 * MII bit-bang glue.
 */
u_int32_t ex_mii_bitbang_read __P((struct device *));
void ex_mii_bitbang_write __P((struct device *, u_int32_t));

const struct mii_bitbang_ops ex_mii_bitbang_ops = {
	ex_mii_bitbang_read,
	ex_mii_bitbang_write,
	{
		ELINK_PHY_DATA,		/* MII_BIT_MDO */
		ELINK_PHY_DATA,		/* MII_BIT_MDI */
		ELINK_PHY_CLK,		/* MII_BIT_MDC */
		ELINK_PHY_DIR,		/* MII_BIT_DIR_HOST_PHY */
		0,			/* MII_BIT_DIR_PHY_HOST */
	}
};

/*
 * Back-end attach and configure.
 */
void
ex_config(sc)
	struct ex_softc *sc;
{
	struct ifnet *ifp;
	u_int16_t val;
	u_int8_t macaddr[ETHER_ADDR_LEN] = {0};
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	bus_dma_segment_t useg, dseg;
	int urseg, drseg, i, error, attach_stage;

	ex_reset(sc);

	val = ex_read_eeprom(sc, EEPROM_OEM_ADDR0);
	macaddr[0] = val >> 8;
	macaddr[1] = val & 0xff;
	val = ex_read_eeprom(sc, EEPROM_OEM_ADDR1);
	macaddr[2] = val >> 8;
	macaddr[3] = val & 0xff;
	val = ex_read_eeprom(sc, EEPROM_OEM_ADDR2);
	macaddr[4] = val >> 8;
	macaddr[5] = val & 0xff;

	printf("%s: MAC address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(macaddr));

	if (sc->intr_ack) { /* 3C575BTX specific */
	    GO_WINDOW(2);
	    bus_space_write_2(sc->sc_iot, ioh, 12, 0x10|bus_space_read_2(sc->sc_iot, ioh, 12));
	}

	attach_stage = 0;

	/*
	 * Allocate the upload descriptors, and create and load the DMA
	 * map for them.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    EX_NUPD * sizeof (struct ex_upd), NBPG, 0, &useg, 1, &urseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't allocate upload descriptors, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 1;

	if ((error = bus_dmamem_map(sc->sc_dmat, &useg, urseg,
	    EX_NUPD * sizeof (struct ex_upd), (caddr_t *)&sc->sc_upd,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: can't map upload descriptors, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 2;

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    EX_NUPD * sizeof (struct ex_upd), 1,
	    EX_NUPD * sizeof (struct ex_upd), 0, BUS_DMA_NOWAIT,
	    &sc->sc_upd_dmamap)) != 0) {
		printf("%s: can't create upload desc. DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 3;

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_upd_dmamap,
	    sc->sc_upd, EX_NUPD * sizeof (struct ex_upd), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't load upload desc. DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 4;

	/*
	 * Allocate the download descriptors, and create and load the DMA
	 * map for them.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    EX_NDPD * sizeof (struct ex_dpd), NBPG, 0, &dseg, 1, &drseg,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't allocate download descriptors, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 5;

	if ((error = bus_dmamem_map(sc->sc_dmat, &dseg, drseg,
	    EX_NDPD * sizeof (struct ex_dpd), (caddr_t *)&sc->sc_dpd,
	    BUS_DMA_NOWAIT|BUS_DMA_COHERENT)) != 0) {
		printf("%s: can't map download descriptors, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}
	bzero(sc->sc_dpd, EX_NDPD * sizeof (struct ex_dpd));

	attach_stage = 6;

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    EX_NDPD * sizeof (struct ex_dpd), 1,
	    EX_NDPD * sizeof (struct ex_dpd), 0, BUS_DMA_NOWAIT,
	    &sc->sc_dpd_dmamap)) != 0) {
		printf("%s: can't create download desc. DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 7;

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dpd_dmamap,
	    sc->sc_dpd, EX_NDPD * sizeof (struct ex_dpd), NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		printf("%s: can't load download desc. DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail;
	}

	attach_stage = 8;


	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < EX_NDPD; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    EX_NTFRAGS, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->sc_tx_dmamaps[i])) != 0) {
			printf("%s: can't create tx DMA map %d, error = %d\n",
			    sc->sc_dev.dv_xname, i, error);
			goto fail;
		}
	}

	attach_stage = 9;

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < EX_NUPD; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    EX_NRFRAGS, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->sc_rx_dmamaps[i])) != 0) {
			printf("%s: can't create rx DMA map %d, error = %d\n",
			    sc->sc_dev.dv_xname, i, error);
			goto fail;
		}
	}

	attach_stage = 10;

	/*
	 * Create ring of upload descriptors, only once. The DMA engine
	 * will loop over this when receiving packets, stalling if it
	 * hits an UPD with a finished receive.
	 */
	for (i = 0; i < EX_NUPD; i++) {
		sc->sc_rxdescs[i].rx_dmamap = sc->sc_rx_dmamaps[i];
		sc->sc_rxdescs[i].rx_upd = &sc->sc_upd[i];
		sc->sc_upd[i].upd_frags[0].fr_len =
		    htopci((MCLBYTES - 2) | EX_FR_LAST);
		if (ex_add_rxbuf(sc, &sc->sc_rxdescs[i]) != 0) {
			printf("%s: can't allocate or map rx buffers\n",
			    sc->sc_dev.dv_xname);
			goto fail;
		}
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_upd_dmamap, 0,
	    EX_NUPD * sizeof (struct ex_upd),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	ex_init_txdescs(sc);

	attach_stage = 11;


	GO_WINDOW(3);
	val = bus_space_read_2(iot, ioh, ELINK_W3_RESET_OPTIONS);
	if (val & ELINK_MEDIACAP_MII)
		sc->ex_conf |= EX_CONF_MII;

	ifp = &sc->sc_ethercom.ec_if;

	/*
	 * Initialize our media structures and MII info.  We'll
	 * probe the MII if we discover that we have one.
	 */
	sc->ex_mii.mii_ifp = ifp;
	sc->ex_mii.mii_readreg = ex_mii_readreg;
	sc->ex_mii.mii_writereg = ex_mii_writereg;
	sc->ex_mii.mii_statchg = ex_mii_statchg;
	ifmedia_init(&sc->ex_mii.mii_media, 0, ex_media_chg,
	    ex_media_stat);

	if (sc->ex_conf & EX_CONF_MII) {
		/*
		 * Find PHY, extract media information from it.
		 * First, select the right transceiver.
		 */
		u_int32_t icfg;

		GO_WINDOW(3);
		icfg = bus_space_read_4(iot, ioh, ELINK_W3_INTERNAL_CONFIG);
		icfg &= ~(CONFIG_XCVR_SEL << 16);
		if (val & (ELINK_MEDIACAP_MII | ELINK_MEDIACAP_100BASET4))
			icfg |= ELINKMEDIA_MII << (CONFIG_XCVR_SEL_SHIFT + 16);
		if (val & ELINK_MEDIACAP_100BASETX)
			icfg |= ELINKMEDIA_AUTO << (CONFIG_XCVR_SEL_SHIFT + 16);
		if (val & ELINK_MEDIACAP_100BASEFX)
			icfg |= ELINKMEDIA_100BASE_FX 
				<< (CONFIG_XCVR_SEL_SHIFT + 16);
		bus_space_write_4(iot, ioh, ELINK_W3_INTERNAL_CONFIG, icfg);

		mii_phy_probe(&sc->sc_dev, &sc->ex_mii, 0xffffffff,
		    MII_PHY_ANY, MII_OFFSET_ANY);
		if (LIST_FIRST(&sc->ex_mii.mii_phys) == NULL) {
			ifmedia_add(&sc->ex_mii.mii_media, IFM_ETHER|IFM_NONE,
			    0, NULL);
			ifmedia_set(&sc->ex_mii.mii_media, IFM_ETHER|IFM_NONE);
		} else {
			ifmedia_set(&sc->ex_mii.mii_media, IFM_ETHER|IFM_AUTO);
		}
	} else
		ex_probemedia(sc);

	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = ex_start;
	ifp->if_ioctl = ex_ioctl;
	ifp->if_watchdog = ex_watchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	if_attach(ifp);
	ether_ifattach(ifp, macaddr);

	GO_WINDOW(1);

	sc->tx_start_thresh = 20;
	sc->tx_succ_ok = 0;

	/* TODO: set queues to 0 */

#if NBPFILTER > 0
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
		  sizeof(struct ether_header));
#endif

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_NET, 0);
#endif

	/*  Establish callback to reset card when we reboot. */
	shutdownhook_establish(ex_shutdown, sc);
	return;

 fail:
	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall though.
	 */
	switch (attach_stage) {
	case 11:
	    {
		struct ex_rxdesc *rxd;

		for (i = 0; i < EX_NUPD; i++) {
			rxd = &sc->sc_rxdescs[i];
			if (rxd->rx_mbhead != NULL) {
				bus_dmamap_unload(sc->sc_dmat, rxd->rx_dmamap);
				m_freem(rxd->rx_mbhead);
			}
		}
	    }
		/* FALLTHROUGH */

	case 10:
		for (i = 0; i < EX_NUPD; i++)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_rx_dmamaps[i]);
		/* FALLTHROUGH */

	case 9:
		for (i = 0; i < EX_NDPD; i++)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_tx_dmamaps[i]);
		/* FALLTHROUGH */
	case 8:
		bus_dmamap_unload(sc->sc_dmat, sc->sc_dpd_dmamap);
		/* FALLTHROUGH */

	case 7:
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_dpd_dmamap);
		/* FALLTHROUGH */

	case 6:
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_dpd,
		    EX_NDPD * sizeof (struct ex_dpd));
		/* FALLTHROUGH */

	case 5:
		bus_dmamem_free(sc->sc_dmat, &dseg, drseg);
		break;

	case 4:
		bus_dmamap_unload(sc->sc_dmat, sc->sc_upd_dmamap);
		/* FALLTHROUGH */

	case 3:
		bus_dmamap_destroy(sc->sc_dmat, sc->sc_upd_dmamap);
		/* FALLTHROUGH */

	case 2:
		bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_upd,
		    EX_NUPD * sizeof (struct ex_upd));
		/* FALLTHROUGH */

	case 1:
		bus_dmamem_free(sc->sc_dmat, &useg, urseg);
		break;
	}

}

/*
 * Find the media present on non-MII chips.
 */
void
ex_probemedia(sc)
	struct ex_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct ifmedia *ifm = &sc->ex_mii.mii_media;
	struct ex_media *exm;
	u_int16_t config1, reset_options, default_media;
	int defmedia = 0;
	const char *sep = "", *defmedianame = NULL;

	GO_WINDOW(3);
	config1 = bus_space_read_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG + 2);
	reset_options = bus_space_read_1(iot, ioh, ELINK_W3_RESET_OPTIONS);
	GO_WINDOW(0);

	default_media = (config1 & CONFIG_MEDIAMASK) >> CONFIG_MEDIAMASK_SHIFT;

	printf("%s: ", sc->sc_dev.dv_xname);

	/* Sanity check that there are any media! */
	if ((reset_options & ELINK_PCI_MEDIAMASK) == 0) {
		printf("no media present!\n");
		ifmedia_add(ifm, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(ifm, IFM_ETHER|IFM_NONE);
		return;
	}

#define	PRINT(s)	printf("%s%s", sep, s); sep = ", "

	for (exm = ex_native_media; exm->exm_name != NULL; exm++) {
		if (reset_options & exm->exm_mpbit) {
			/*
			 * Default media is a little complicated.  We
			 * support full-duplex which uses the same
			 * reset options bit.
			 *
			 * XXX Check EEPROM for default to FDX?
			 */
			if (exm->exm_epmedia == default_media) {
				if ((exm->exm_ifmedia & IFM_FDX) == 0) {
					defmedia = exm->exm_ifmedia;
					defmedianame = exm->exm_name;
				}
			} else if (defmedia == 0) {
				defmedia = exm->exm_ifmedia;
				defmedianame = exm->exm_name;
			}
			ifmedia_add(ifm, exm->exm_ifmedia, exm->exm_epmedia,
			    NULL);
			PRINT(exm->exm_name);
		}
	}

#undef PRINT

#ifdef DIAGNOSTIC
	if (defmedia == 0)
		panic("ex_probemedia: impossible");
#endif

	printf(", default %s\n", defmedianame);
	ifmedia_set(ifm, defmedia);
}

/*
 * Bring device up.
 */
void
ex_init(sc)
	struct ex_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int s, i;

	s = splnet();

	ex_waitcmd(sc);
	ex_stop(sc);

	/*
	 * Set the station address and clear the station mask. The latter
	 * is needed for 90x cards, 0 is the default for 90xB cards.
	 */
	GO_WINDOW(2);
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		bus_space_write_1(iot, ioh, ELINK_W2_ADDR_0 + i,
		    LLADDR(ifp->if_sadl)[i]);
		bus_space_write_1(iot, ioh, ELINK_W2_RECVMASK_0 + i, 0);
	}

	GO_WINDOW(3);

	bus_space_write_2(iot, ioh, ELINK_COMMAND, RX_RESET);
	ex_waitcmd(sc);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, TX_RESET);
	ex_waitcmd(sc);

	/*
	 * Disable reclaim threshold for 90xB, set free threshold to
	 * 6 * 256 = 1536 for 90x.
	 */
	if (sc->ex_conf & EX_CONF_90XB)
		bus_space_write_2(iot, ioh, ELINK_COMMAND,
		    ELINK_TXRECLTHRESH | 255);
	else
		bus_space_write_1(iot, ioh, ELINK_TXFREETHRESH, 6);

	bus_space_write_2(iot, ioh, ELINK_COMMAND,
	    SET_RX_EARLY_THRESH | ELINK_THRESH_DISABLE);

	bus_space_write_4(iot, ioh, ELINK_DMACTRL,
	    bus_space_read_4(iot, ioh, ELINK_DMACTRL) | ELINK_DMAC_UPRXEAREN);

	bus_space_write_2(iot, ioh, ELINK_COMMAND, SET_RD_0_MASK | S_MASK);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, SET_INTR_MASK | S_MASK);

	bus_space_write_2(iot, ioh, ELINK_COMMAND, ACK_INTR | 0xff);
	if (sc->intr_ack)
	    (* sc->intr_ack)(sc);
	ex_set_media(sc);
	ex_set_mc(sc);


	bus_space_write_2(iot, ioh, ELINK_COMMAND, STATS_ENABLE);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, TX_ENABLE);
	bus_space_write_4(iot, ioh, ELINK_UPLISTPTR, sc->sc_upddma);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, RX_ENABLE);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, ELINK_UPUNSTALL);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ex_start(ifp);

	GO_WINDOW(1);

	splx(s);

	timeout(ex_tick, sc, hz);
}

/*
 * Multicast hash filter according to the 3Com spec.
 */
static u_int16_t
ex_mchash(addr)
	u_char *addr;
{
	u_int32_t crc, carry;
	int i, j;
	u_char c;

	/* Compute CRC for the address value. */
	crc = 0xffffffff; /* initial value */

	for (i = 0; i < 6; i++) {
		c = addr[i];
		for (j = 0; j < 8; j++) {
			carry = ((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01);
			crc <<= 1;
			c >>= 1;
			if (carry)
				crc = (crc ^ 0x04c11db6) | carry;
		}
	}

	/* Return the filter bit position. */
	return(crc & 0x000000ff);
}


/*
 * Set multicast receive filter. Also take care of promiscuous mode
 * here (XXX).
 */
void
ex_set_mc(sc)
	register struct ex_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ether_multi *enm;
	struct ether_multistep estep;
	int i;
	u_int16_t mask = FIL_INDIVIDUAL | FIL_BRDCST;

	if (ifp->if_flags & IFF_PROMISC)
		mask |= FIL_PROMISC;
	
	if (!(ifp->if_flags & IFF_MULTICAST))
		goto out;

	if (!(sc->ex_conf & EX_CONF_90XB) || ifp->if_flags & IFF_ALLMULTI) {
		mask |= (ifp->if_flags & IFF_MULTICAST) ? FIL_MULTICAST : 0;
	} else {
		ETHER_FIRST_MULTI(estep, ec, enm);
		while (enm != NULL) {
			if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN) != 0)
				goto out;
			i = ex_mchash(enm->enm_addrlo);
			bus_space_write_2(sc->sc_iot, sc->sc_ioh,
			    ELINK_COMMAND, ELINK_SETHASHFILBIT | i);
			ETHER_NEXT_MULTI(estep, enm);
		}
		mask |= FIL_MULTIHASH;
	}
 out:
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ELINK_COMMAND,
	    SET_RX_FILTER | mask);
}


static void
ex_txstat(sc)
	struct ex_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;

	/*
	 * We need to read+write TX_STATUS until we get a 0 status
	 * in order to turn off the interrupt flag.
	 */
	while ((i = bus_space_read_1(iot, ioh, ELINK_TXSTATUS)) & TXS_COMPLETE) {
		bus_space_write_1(iot, ioh, ELINK_TXSTATUS, 0x0);

		if (i & TXS_JABBER) {
			++sc->sc_ethercom.ec_if.if_oerrors;
			if (sc->sc_ethercom.ec_if.if_flags & IFF_DEBUG)
				printf("%s: jabber (%x)\n",
				       sc->sc_dev.dv_xname, i);
			ex_init(sc);
			/* TODO: be more subtle here */
		} else if (i & TXS_UNDERRUN) {
			++sc->sc_ethercom.ec_if.if_oerrors;
			if (sc->sc_ethercom.ec_if.if_flags & IFF_DEBUG)
				printf("%s: fifo underrun (%x) @%d\n",
				       sc->sc_dev.dv_xname, i,
				       sc->tx_start_thresh);
			if (sc->tx_succ_ok < 100)
				    sc->tx_start_thresh = min(ETHER_MAX_LEN,
					    sc->tx_start_thresh + 20);
			sc->tx_succ_ok = 0;
			ex_init(sc);
			/* TODO: be more subtle here */
		} else if (i & TXS_MAX_COLLISION) {
			++sc->sc_ethercom.ec_if.if_collisions;
			bus_space_write_2(iot, ioh, ELINK_COMMAND, TX_ENABLE);
			sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;
		} else
			sc->tx_succ_ok = (sc->tx_succ_ok+1) & 127;
	}
}

int
ex_media_chg(ifp)
	struct ifnet *ifp;
{
	struct ex_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		ex_init(sc);
	return 0;
}

void
ex_set_media(sc)
	struct ex_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int config0, config1;

	if (((sc->ex_conf & EX_CONF_MII) &&
	    (sc->ex_mii.mii_media_active & IFM_FDX))
	    || (!(sc->ex_conf & EX_CONF_MII) &&
	    (sc->ex_mii.mii_media.ifm_media & IFM_FDX))) {
		bus_space_write_2(iot, ioh, ELINK_W3_MAC_CONTROL,
		    MAC_CONTROL_FDX);
	} else {
		bus_space_write_2(iot, ioh, ELINK_W3_MAC_CONTROL, 0);
	}

	/*
	 * If the device has MII, select it, and then tell the
	 * PHY which media to use.
	 */
	if (sc->ex_conf & EX_CONF_MII) {
		GO_WINDOW(3);

		config0 = (u_int)bus_space_read_2(iot, ioh,
		    ELINK_W3_INTERNAL_CONFIG);
		config1 = (u_int)bus_space_read_2(iot, ioh,
		    ELINK_W3_INTERNAL_CONFIG + 2);

		config1 = config1 & ~CONFIG_MEDIAMASK;
		config1 |= (ELINKMEDIA_MII << CONFIG_MEDIAMASK_SHIFT);

		bus_space_write_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG, config0);
		bus_space_write_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG + 2, config1);
		mii_mediachg(&sc->ex_mii);
		return;
	}

	GO_WINDOW(4);
	bus_space_write_2(iot, ioh, ELINK_W4_MEDIA_TYPE, 0);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, STOP_TRANSCEIVER);
	delay(800);

	/*
	 * Now turn on the selected media/transceiver.
	 */
	switch (IFM_SUBTYPE(sc->ex_mii.mii_media.ifm_cur->ifm_media)) {
	case IFM_10_T:
		bus_space_write_2(iot, ioh, ELINK_W4_MEDIA_TYPE,
		    JABBER_GUARD_ENABLE|LINKBEAT_ENABLE);
		break;

	case IFM_10_2:
		bus_space_write_2(iot, ioh, ELINK_COMMAND, START_TRANSCEIVER);
		DELAY(800);
		break;

	case IFM_100_TX:
	case IFM_100_FX:
		bus_space_write_2(iot, ioh, ELINK_W4_MEDIA_TYPE,
		    LINKBEAT_ENABLE);
		DELAY(800);
		break;

	case IFM_10_5:
		bus_space_write_2(iot, ioh, ELINK_W4_MEDIA_TYPE,
		    SQE_ENABLE);
		DELAY(800);
		break;

	case IFM_MANUAL:
		break;

	case IFM_NONE:
		return;

	default:
		panic("ex_set_media: impossible");
	}

	GO_WINDOW(3);
	config0 = (u_int)bus_space_read_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG);
	config1 = (u_int)bus_space_read_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG + 2);

	config1 = config1 & ~CONFIG_MEDIAMASK;
	config1 |= (sc->ex_mii.mii_media.ifm_cur->ifm_data <<
	    CONFIG_MEDIAMASK_SHIFT);

	bus_space_write_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG, config0);
	bus_space_write_2(iot, ioh, ELINK_W3_INTERNAL_CONFIG + 2, config1);
}

/*
 * Get currently-selected media from card.
 * (if_media callback, may be called before interface is brought up).
 */
void
ex_media_stat(ifp, req)
	struct ifnet *ifp;
	struct ifmediareq *req;
{
	struct ex_softc *sc = ifp->if_softc;

	if (sc->ex_conf & EX_CONF_MII) {
		mii_pollstat(&sc->ex_mii);
		req->ifm_status = sc->ex_mii.mii_media_status;
		req->ifm_active = sc->ex_mii.mii_media_active;
	} else {
		GO_WINDOW(4);
		req->ifm_status = IFM_AVALID;
		req->ifm_active = sc->ex_mii.mii_media.ifm_cur->ifm_media;
		if (bus_space_read_2(sc->sc_iot, sc->sc_ioh,
		    ELINK_W4_MEDIA_TYPE) & LINKBEAT_DETECT)
			req->ifm_status |= IFM_ACTIVE;
                GO_WINDOW(1);
	}
}



/*
 * Start outputting on the interface.
 */
static void
ex_start(ifp)
	struct ifnet *ifp;
{
	struct ex_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	volatile struct ex_fraghdr *fr = NULL;
	volatile struct ex_dpd *dpd = NULL, *prevdpd = NULL;
	struct ex_txdesc *txp;
	bus_dmamap_t dmamap;
	int offset, totlen;

	if (sc->tx_head || sc->tx_free == NULL)
		return;

	txp = NULL;

	/*
	 * We're finished if there is nothing more to add to the list or if
	 * we're all filled up with buffers to transmit.
	 */
	while (ifp->if_snd.ifq_head != NULL && sc->tx_free != NULL) {
		struct mbuf *mb_head;
		int segment, error;

		/*
		 * Grab a packet to transmit.
		 */
		IF_DEQUEUE(&ifp->if_snd, mb_head);

		/*
		 * Get pointer to next available tx desc.
		 */
		txp = sc->tx_free;
		sc->tx_free = txp->tx_next;
		txp->tx_next = NULL;
		dmamap = txp->tx_dmamap;

		/*
		 * Go through each of the mbufs in the chain and initialize
		 * the transmit buffer descriptors with the physical address
		 * and size of the mbuf.
		 */
 reload:
		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
		    mb_head, BUS_DMA_NOWAIT);
		switch (error) {
		case 0:
			/* Success. */
			break;

		case EFBIG:
		    {
			struct mbuf *mn;

			/*
			 * We ran out of segments.  We have to recopy this
			 * mbuf chain first.  Bail out if we can't get the
			 * new buffers.
			 */
			printf("%s: too many segments, ", sc->sc_dev.dv_xname);

			MGETHDR(mn, M_DONTWAIT, MT_DATA);
			if (mn == NULL) {
				m_freem(mb_head);
				printf("aborting\n");
				goto out;
			}
			if (mb_head->m_pkthdr.len > MHLEN) {
				MCLGET(mn, M_DONTWAIT);
				if ((mn->m_flags & M_EXT) == 0) {
					m_freem(mn);
					m_freem(mb_head);
					printf("aborting\n");
					goto out;
				}
			}
			m_copydata(mb_head, 0, mb_head->m_pkthdr.len,
			    mtod(mn, caddr_t));
			mn->m_pkthdr.len = mn->m_len = mb_head->m_pkthdr.len;
			m_freem(mb_head);
			mb_head = mn;
			printf("retrying\n");
			goto reload;
		    }

		default:
			/*
			 * Some other problem; report it.
			 */
			printf("%s: can't load mbuf chain, error = %d\n",
			    sc->sc_dev.dv_xname, error);
			m_freem(mb_head);
			goto out;
		}

		fr = &txp->tx_dpd->dpd_frags[0];
		totlen = 0;
		for (segment = 0; segment < dmamap->dm_nsegs; segment++, fr++) {
			fr->fr_addr = htopci(dmamap->dm_segs[segment].ds_addr);
			fr->fr_len = htopci(dmamap->dm_segs[segment].ds_len);
			totlen += dmamap->dm_segs[segment].ds_len;
		}
		fr--;
		fr->fr_len |= htopci(EX_FR_LAST);
		txp->tx_mbhead = mb_head;

		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		dpd = txp->tx_dpd;
		dpd->dpd_nextptr = 0;
		dpd->dpd_fsh = htopci(totlen);

		bus_dmamap_sync(sc->sc_dmat, sc->sc_dpd_dmamap,
		    ((caddr_t)dpd - (caddr_t)sc->sc_dpd),
		    sizeof (struct ex_dpd),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * No need to stall the download engine, we know it's
		 * not busy right now.
		 *
		 * Fix up pointers in both the "soft" tx and the physical
		 * tx list.
		 */
		if (sc->tx_head != NULL) {
			prevdpd = sc->tx_tail->tx_dpd;
			offset = ((caddr_t)prevdpd - (caddr_t)sc->sc_dpd);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dpd_dmamap,
			    offset, sizeof (struct ex_dpd),
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
			prevdpd->dpd_nextptr = htopci(DPD_DMADDR(sc, txp));
			bus_dmamap_sync(sc->sc_dmat, sc->sc_dpd_dmamap,
			    offset, sizeof (struct ex_dpd),
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); 
			sc->tx_tail->tx_next = txp;
			sc->tx_tail = txp;
		} else {
			sc->tx_tail = sc->tx_head = txp;
		}

#if NBPFILTER > 0
		/*
		 * Pass packet to bpf if there is a listener.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, mb_head);
#endif
	}
 out:
	if (sc->tx_head) {
		sc->tx_tail->tx_dpd->dpd_fsh |= htopci(EX_DPD_DNIND);
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dpd_dmamap,
		    ((caddr_t)sc->tx_tail->tx_dpd - (caddr_t)sc->sc_dpd),
		    sizeof (struct ex_dpd),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		ifp->if_flags |= IFF_OACTIVE;
		bus_space_write_2(iot, ioh, ELINK_COMMAND, ELINK_DNUNSTALL);
		bus_space_write_4(iot, ioh, ELINK_DNLISTPTR,
		    DPD_DMADDR(sc, sc->tx_head));

		/* trigger watchdog */
		ifp->if_timer = 5;
	}
}


int
ex_intr(arg)
	void *arg;
{
	struct ex_softc *sc = arg;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int16_t stat;
	int ret = 0;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (sc->enabled == 0) {
	  return ret;
	}
	for (;;) {
		stat = bus_space_read_2(iot, ioh, ELINK_STATUS);
		if (!(stat & S_MASK))
			break;
		/*
		 * Acknowledge interrupts.
		 */
		bus_space_write_2(iot, ioh, ELINK_COMMAND, ACK_INTR |
				      (stat & S_MASK));
		if (sc->intr_ack)
		    (*sc->intr_ack)(sc);
		ret = 1;
		if (stat & S_HOST_ERROR) {
			printf("%s: adapter failure (%x)\n",
			    sc->sc_dev.dv_xname, stat);
			bus_space_write_2(iot, ioh, ELINK_COMMAND,
			    C_INTR_LATCH);
			ex_reset(sc);
			ex_init(sc);
			return 1;
		}
		if (stat & S_TX_COMPLETE) {
			ex_txstat(sc);
		}
		if (stat & S_UPD_STATS) {
			ex_getstats(sc);
		}
		if (stat & S_DN_COMPLETE) {
			struct ex_txdesc *txp, *ptxp = NULL;
			bus_dmamap_t txmap;

			/* reset watchdog timer, was set in ex_start() */
			ifp->if_timer = 0;

			for (txp = sc->tx_head; txp != NULL;
			    txp = txp->tx_next) {
				bus_dmamap_sync(sc->sc_dmat,
				    sc->sc_dpd_dmamap,
				    (caddr_t)txp->tx_dpd - (caddr_t)sc->sc_dpd,
				    sizeof (struct ex_dpd),
				    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
				if (txp->tx_mbhead != NULL) {
					txmap = txp->tx_dmamap;
					bus_dmamap_sync(sc->sc_dmat, txmap,
					    0, txmap->dm_mapsize,
					    BUS_DMASYNC_POSTWRITE);
					bus_dmamap_unload(sc->sc_dmat, txmap);
					m_freem(txp->tx_mbhead);
					txp->tx_mbhead = NULL;
				}
				ptxp = txp;
			}

			/*
			 * Move finished tx buffers back to the tx free list.
			 */
			if (sc->tx_free) {
				sc->tx_ftail->tx_next = sc->tx_head;
				sc->tx_ftail = ptxp;
			} else
				sc->tx_ftail = sc->tx_free = sc->tx_head;

			sc->tx_head = sc->tx_tail = NULL;
			ifp->if_flags &= ~IFF_OACTIVE;
		}

		if (stat & S_UP_COMPLETE) {
			struct ex_rxdesc *rxd;
			struct mbuf *m;
			struct ex_upd *upd;
			bus_dmamap_t rxmap;
			u_int32_t pktstat;

 rcvloop:
			rxd = sc->rx_head;
			rxmap = rxd->rx_dmamap;
			m = rxd->rx_mbhead;
			upd = rxd->rx_upd;
			pktstat = pcitoh(upd->upd_pktstatus);

			bus_dmamap_sync(sc->sc_dmat, rxmap, 0,
			    rxmap->dm_mapsize,
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
			bus_dmamap_sync(sc->sc_dmat, sc->sc_upd_dmamap,
			    ((caddr_t)upd - (caddr_t)sc->sc_upd), 
			    sizeof (struct ex_upd),
			    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

			if (pktstat & EX_UPD_COMPLETE) {
				/*
				 * Remove first packet from the chain.
				 */
				sc->rx_head = rxd->rx_next;
				rxd->rx_next = NULL;

				/*
				 * Add a new buffer to the receive chain.
				 * If this fails, the old buffer is recycled
				 * instead.
				 */
				if (ex_add_rxbuf(sc, rxd) == 0) {
					struct ether_header *eh;
					u_int16_t total_len;


					if (pktstat & EX_UPD_ERR) {
						ifp->if_ierrors++;
						m_freem(m);
						goto rcvloop;
					}

					total_len = pktstat & EX_UPD_PKTLENMASK;
					if (total_len <
					    sizeof(struct ether_header)) {
						m_freem(m);
						goto rcvloop;
					}
					m->m_pkthdr.rcvif = ifp;
					m->m_pkthdr.len = m->m_len = total_len;
					eh = mtod(m, struct ether_header *);
#if NBPFILTER > 0
					if (ifp->if_bpf) {
						bpf_tap(ifp->if_bpf,
						    mtod(m, caddr_t),
						    total_len); 
						/*
						 * Only pass this packet up
						 * if it is for us.
						 */
						if ((ifp->if_flags &
						    IFF_PROMISC) &&
						    (eh->ether_dhost[0] & 1)
						    == 0 &&
						    bcmp(eh->ether_dhost,
							LLADDR(ifp->if_sadl),
							sizeof(eh->ether_dhost))
							    != 0) {
							m_freem(m);
							goto rcvloop;
						}
					}
#endif /* NBPFILTER > 0 */
					(*ifp->if_input)(ifp, m);
				}
				goto rcvloop;
			}
			/*
			 * Just in case we filled up all UPDs and the DMA engine
			 * stalled. We could be more subtle about this.
			 */
			if (bus_space_read_4(iot, ioh, ELINK_UPLISTPTR) == 0) {
				printf("%s: uplistptr was 0\n",
				       sc->sc_dev.dv_xname);
				ex_init(sc);
			} else if (bus_space_read_4(iot, ioh, ELINK_UPPKTSTATUS)
				   & 0x2000) {
				printf("%s: receive stalled\n",
				       sc->sc_dev.dv_xname);
				bus_space_write_2(iot, ioh, ELINK_COMMAND,
						  ELINK_UPUNSTALL);
			}
		}
	}
	if (ret) {
		bus_space_write_2(iot, ioh, ELINK_COMMAND, C_INTR_LATCH);
		if (ifp->if_snd.ifq_head != NULL)
			ex_start(ifp);
	}
	return ret;
}

int
ex_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ex_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			ex_init(sc);
			arp_ifinit(&sc->sc_ethercom.ec_if, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)
				    LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
				    ifp->if_addrlen);
			/* Set new address. */
			ex_init(sc);
			break;
		    }
#endif
		default:
			ex_init(sc);
			break;
		}
		break;
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->ex_mii.mii_media, cmd);
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			ex_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			ex_init(sc);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Deal with other flags that change hardware
			 * state, i.e. IFF_PROMISC.
			 */
			ex_set_mc(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			ex_set_mc(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);
	return (error);
}

void
ex_getstats(sc)
	struct ex_softc *sc;
{
	bus_space_handle_t ioh = sc->sc_ioh;
	bus_space_tag_t iot = sc->sc_iot;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int8_t upperok;

	GO_WINDOW(6);
	upperok = bus_space_read_1(iot, ioh, UPPER_FRAMES_OK);
	ifp->if_ipackets += bus_space_read_1(iot, ioh, RX_FRAMES_OK);
	ifp->if_ipackets += (upperok & 0x03) << 8;
	ifp->if_opackets += bus_space_read_1(iot, ioh, TX_FRAMES_OK);
	ifp->if_opackets += (upperok & 0x30) << 4;
	ifp->if_ierrors += bus_space_read_1(iot, ioh, RX_OVERRUNS);
	ifp->if_collisions += bus_space_read_1(iot, ioh, TX_COLLISIONS);
	/*
	 * There seems to be no way to get the exact number of collisions,
	 * this is the number that occured at the very least.
	 */
	ifp->if_collisions += 2 * bus_space_read_1(iot, ioh,
	    TX_AFTER_X_COLLISIONS);
	ifp->if_ibytes += bus_space_read_2(iot, ioh, RX_TOTAL_OK);
	ifp->if_obytes += bus_space_read_2(iot, ioh, TX_TOTAL_OK);

	/*
	 * Clear the following to avoid stats overflow interrupts
	 */
	bus_space_read_1(iot, ioh, TX_DEFERRALS);
	bus_space_read_1(iot, ioh, TX_AFTER_1_COLLISION);
	bus_space_read_1(iot, ioh, TX_NO_SQE);
	bus_space_read_1(iot, ioh, TX_CD_LOST);
	GO_WINDOW(4);
	bus_space_read_1(iot, ioh, ELINK_W4_BADSSD);
	upperok = bus_space_read_1(iot, ioh, ELINK_W4_UBYTESOK);
	ifp->if_ibytes += (upperok & 0x0f) << 16;
	ifp->if_obytes += (upperok & 0xf0) << 12;
	GO_WINDOW(1);
}

void
ex_printstats(sc)
	struct ex_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	ex_getstats(sc);
	printf("in %ld out %ld ierror %ld oerror %ld ibytes %ld obytes %ld\n",
	    ifp->if_ipackets, ifp->if_opackets, ifp->if_ierrors,
	    ifp->if_oerrors, ifp->if_ibytes, ifp->if_obytes);
}

void
ex_tick(arg)
	void *arg;
{
	struct ex_softc *sc = arg;
	int s = splnet();

	if (sc->ex_conf & EX_CONF_MII)
		mii_tick(&sc->ex_mii);

	if (!(bus_space_read_2((sc)->sc_iot, (sc)->sc_ioh, ELINK_STATUS)
	    & S_COMMAND_IN_PROGRESS))
		ex_getstats(sc);

	splx(s);

	timeout(ex_tick, sc, hz);
}


void
ex_reset(sc)
	struct ex_softc *sc;
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ELINK_COMMAND, GLOBAL_RESET);
	delay(400);
	ex_waitcmd(sc);
}

void
ex_watchdog(ifp)
	struct ifnet *ifp;
{
	struct ex_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++sc->sc_ethercom.ec_if.if_oerrors;

	ex_reset(sc);
	ex_init(sc);
}

void
ex_stop(sc)
	struct ex_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ex_txdesc *tx;
	struct ex_rxdesc *rx;
	int i;

	bus_space_write_2(iot, ioh, ELINK_COMMAND, RX_DISABLE);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, TX_DISABLE);
	bus_space_write_2(iot, ioh, ELINK_COMMAND, STOP_TRANSCEIVER);

	for (tx = sc->tx_head ; tx != NULL; tx = tx->tx_next) {
		if (tx->tx_mbhead == NULL)
			continue;
		m_freem(tx->tx_mbhead);
		tx->tx_mbhead = NULL;
		bus_dmamap_unload(sc->sc_dmat, tx->tx_dmamap);
		tx->tx_dpd->dpd_fsh = tx->tx_dpd->dpd_nextptr = 0;
		bus_dmamap_sync(sc->sc_dmat, sc->sc_dpd_dmamap,
		    ((caddr_t)tx->tx_dpd - (caddr_t)sc->sc_dpd),
		    sizeof (struct ex_dpd),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	}
	sc->tx_tail = sc->tx_head = NULL;
	ex_init_txdescs(sc);

	sc->rx_tail = sc->rx_head = 0;
	for (i = 0; i < EX_NUPD; i++) {
		rx = &sc->sc_rxdescs[i];
		if (rx->rx_mbhead != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rx->rx_dmamap);
			m_freem(rx->rx_mbhead);
			rx->rx_mbhead = NULL;
		}
		ex_add_rxbuf(sc, rx);
	}

	bus_space_write_2(iot, ioh, ELINK_COMMAND, C_INTR_LATCH);

	untimeout(ex_tick, sc);
	if (sc->ex_conf & EX_CONF_MII)
		mii_down(&sc->ex_mii);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

static void
ex_init_txdescs(sc)
	struct ex_softc *sc;
{
	int i;

	for (i = 0; i < EX_NDPD; i++) {
		sc->sc_txdescs[i].tx_dmamap = sc->sc_tx_dmamaps[i];
		sc->sc_txdescs[i].tx_dpd = &sc->sc_dpd[i];
		if (i < EX_NDPD - 1)
			sc->sc_txdescs[i].tx_next = &sc->sc_txdescs[i + 1];
		else
			sc->sc_txdescs[i].tx_next = NULL;
	}
	sc->tx_free = &sc->sc_txdescs[0];
	sc->tx_ftail = &sc->sc_txdescs[EX_NDPD-1];
}


/*
 * Before reboots, reset card completely.
 */
static void
ex_shutdown(arg)
	void *arg;
{
	register struct ex_softc *sc = arg;

	ex_stop(sc);
}

/*
 * Read EEPROM data.
 * XXX what to do if EEPROM doesn't unbusy?
 */
u_int16_t
ex_read_eeprom(sc, offset)
	struct ex_softc *sc;
	int offset;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int16_t data = 0;

	GO_WINDOW(0);
	if (ex_eeprom_busy(sc))
		goto out;
	switch (sc->ex_bustype) {
	case EX_BUS_PCI:
		bus_space_write_1(iot, ioh, ELINK_W0_EEPROM_COMMAND,
 		    READ_EEPROM | (offset & 0x3f));
		break;
	case EX_BUS_CARDBUS:
		bus_space_write_2(iot, ioh, ELINK_W0_EEPROM_COMMAND,
		    0x230 + (offset & 0x3f));
		break;
	}
	if (ex_eeprom_busy(sc))
		goto out;
	data = bus_space_read_2(iot, ioh, ELINK_W0_EEPROM_DATA);
out:
	return data;
}

static int
ex_eeprom_busy(sc)
	struct ex_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i = 100;

	while (i--) {
		if (!(bus_space_read_2(iot, ioh, ELINK_W0_EEPROM_COMMAND) &
		    EEPROM_BUSY))
			return 0;
		delay(100);
	}
	printf("\n%s: eeprom stays busy.\n", sc->sc_dev.dv_xname);
	return (1);
}

/*
 * Create a new rx buffer and add it to the 'soft' rx list.
 */
static int
ex_add_rxbuf(sc, rxd)
	struct ex_softc *sc;
	struct ex_rxdesc *rxd;
{
	struct mbuf *m, *oldm;
	bus_dmamap_t rxmap;
	int error, rval = 0;

	oldm = rxd->rx_mbhead;
	rxmap = rxd->rx_dmamap;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m != NULL) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_freem(m);
			if (oldm == NULL)
				return 1;
			m = oldm;
			m->m_data = m->m_ext.ext_buf;
			rval = 1;
		}
	} else {
		if (oldm == NULL)
			return 1;
		m = oldm;
		m->m_data = m->m_ext.ext_buf;
		rval = 1;
	}

	/*
	 * Setup the DMA map for this receive buffer.
	 */
	if (m != oldm) {
		if (oldm != NULL)
			bus_dmamap_unload(sc->sc_dmat, rxmap);
		error = bus_dmamap_load(sc->sc_dmat, rxmap,
		    m->m_ext.ext_buf, MCLBYTES, NULL, BUS_DMA_NOWAIT);
		if (error) {
			printf("%s: can't load rx buffer, error = %d\n",
			    sc->sc_dev.dv_xname, error);
			panic("ex_add_rxbuf");	/* XXX */
		}
	}

	/*
	 * Align for data after 14 byte header.
	 */
	m->m_data += 2;

	rxd->rx_mbhead = m;
	rxd->rx_upd->upd_pktstatus = htopci(MCLBYTES - 2);
	rxd->rx_upd->upd_frags[0].fr_addr =
	    htopci(rxmap->dm_segs[0].ds_addr + 2);
	rxd->rx_upd->upd_nextptr = 0;

	/*
	 * Attach it to the end of the list.
	 */
	if (sc->rx_head != NULL) {
		sc->rx_tail->rx_next = rxd;
		sc->rx_tail->rx_upd->upd_nextptr = htopci(sc->sc_upddma +
		    ((caddr_t)rxd->rx_upd - (caddr_t)sc->sc_upd));
		bus_dmamap_sync(sc->sc_dmat, sc->sc_upd_dmamap,
		    (caddr_t)sc->rx_tail->rx_upd - (caddr_t)sc->sc_upd,
		    sizeof (struct ex_upd),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	} else {
		sc->rx_head = rxd;
	}
	sc->rx_tail = rxd;

	bus_dmamap_sync(sc->sc_dmat, rxmap, 0, rxmap->dm_mapsize,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_upd_dmamap,
	    ((caddr_t)rxd->rx_upd - (caddr_t)sc->sc_upd),
	    sizeof (struct ex_upd), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	return (rval);
}

u_int32_t
ex_mii_bitbang_read(self)
	struct device *self;
{
	struct ex_softc *sc = (void *) self;

	/* We're already in Window 4. */
	return (bus_space_read_2(sc->sc_iot, sc->sc_ioh, ELINK_W4_PHYSMGMT));
}

void
ex_mii_bitbang_write(self, val)
	struct device *self;
	u_int32_t val;
{
	struct ex_softc *sc = (void *) self;

	/* We're already in Window 4. */
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, ELINK_W4_PHYSMGMT, val);
}

int
ex_mii_readreg(v, phy, reg)
	struct device *v;
	int phy, reg;
{
	struct ex_softc *sc = (struct ex_softc *)v;
	int val;

	if ((sc->ex_conf & EX_CONF_INTPHY) && phy != ELINK_INTPHY_ID)
		return 0;

	GO_WINDOW(4);

	val = mii_bitbang_readreg(v, &ex_mii_bitbang_ops, phy, reg);

	GO_WINDOW(1);

	return (val);
}

void
ex_mii_writereg(v, phy, reg, data)
        struct device *v;
        int phy;
        int reg;
        int data;
{
	struct ex_softc *sc = (struct ex_softc *)v;

	GO_WINDOW(4);

	mii_bitbang_writereg(v, &ex_mii_bitbang_ops, phy, reg, data);

	GO_WINDOW(1);
}

void
ex_mii_statchg(v)
	struct device *v;
{
	struct ex_softc *sc = (struct ex_softc *)v;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int mctl;
 
	/* XXX Update ifp->if_baudrate */
 
	GO_WINDOW(3);
	mctl = bus_space_read_2(iot, ioh, ELINK_W3_MAC_CONTROL);
	if (sc->ex_mii.mii_media_active & IFM_FDX)
		mctl |= MAC_CONTROL_FDX;
	else
		mctl &= ~MAC_CONTROL_FDX;
	bus_space_write_2(iot, ioh, ELINK_W3_MAC_CONTROL, mctl);
	GO_WINDOW(1);   /* back to operating window */
}
