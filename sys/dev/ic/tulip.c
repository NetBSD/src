/*	$NetBSD: tulip.c,v 1.19 1999/09/27 19:14:01 thorpej Exp $	*/

/*-
 * Copyright (c) 1998, 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Device driver for the Digital Semiconductor ``Tulip'' (21x4x)
 * Ethernet controller family, and a variety of clone chips.
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/mbuf.h>   
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <vm/vm.h>		/* for PAGE_SIZE */
 
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

#include <dev/ic/tulipreg.h>
#include <dev/ic/tulipvar.h>

/*
 * The following tables compute the transmit threshold mode.  We start
 * at index 0.  When ever we get a transmit underrun, we increment our
 * index, falling back if we encounter the NULL terminator.
 *
 * Note: Store and forward mode is only available on the 100mbps chips
 * (21140 and higher).
 */
const struct tulip_txthresh_tab tlp_10_txthresh_tab[] = {
	{ OPMODE_TR_72,		"72 bytes" },
	{ OPMODE_TR_96,		"96 bytes" },
	{ OPMODE_TR_128,	"128 bytes" },
	{ OPMODE_TR_160,	"160 bytes" },
	{ 0,			NULL },
};

const struct tulip_txthresh_tab tlp_10_100_txthresh_tab[] = {
	{ OPMODE_TR_72,		"72/128 bytes" },
	{ OPMODE_TR_96,		"96/256 bytes" },
	{ OPMODE_TR_128,	"128/512 bytes" },
	{ OPMODE_TR_160,	"160/1024 bytes" },
	{ OPMODE_SF,		"store and forward mode" },
	{ 0,			NULL },
};

#define	TXTH_72		0
#define	TXTH_96		1
#define	TXTH_128	2
#define	TXTH_160	3
#define	TXTH_SF		4

/*
 * The Winbond 89C840F does transmit threshold control totally
 * differently.  It simply has a 7-bit field which indicates
 * the threshold:
 *
 *	txth = ((OPMODE & OPMODE_WINB_TTH) >> OPMODE_WINB_TTH_SHIFT) * 16;
 *
 * However, we just do Store-and-Forward mode on these chips, since
 * the DMA engines seem to be flaky.
 */
const struct tulip_txthresh_tab tlp_winb_txthresh_tab[] = {
	{ 0,			"store and forward mode" },
	{ 0,			NULL },
};

#define	TXTH_WINB_SF	0

void	tlp_start __P((struct ifnet *));
void	tlp_watchdog __P((struct ifnet *));
int	tlp_ioctl __P((struct ifnet *, u_long, caddr_t));

void	tlp_shutdown __P((void *));

void	tlp_reset __P((struct tulip_softc *));
int	tlp_init __P((struct tulip_softc *));
void	tlp_rxdrain __P((struct tulip_softc *));
void	tlp_stop __P((struct tulip_softc *, int));
int	tlp_add_rxbuf __P((struct tulip_softc *, int));
void	tlp_idle __P((struct tulip_softc *, u_int32_t));
void	tlp_srom_idle __P((struct tulip_softc *));

void	tlp_filter_setup __P((struct tulip_softc *));
void	tlp_winb_filter_setup __P((struct tulip_softc *));

void	tlp_rxintr __P((struct tulip_softc *));
void	tlp_txintr __P((struct tulip_softc *));

void	tlp_mii_tick __P((void *));
void	tlp_mii_statchg __P((struct device *));
void	tlp_winb_mii_statchg __P((struct device *));

void	tlp_mii_getmedia __P((struct tulip_softc *, struct ifmediareq *));
int	tlp_mii_setmedia __P((struct tulip_softc *));

void	tlp_sio_mii_sync __P((struct tulip_softc *));
void	tlp_sio_mii_sendbits __P((struct tulip_softc *, u_int32_t, int));
int	tlp_sio_mii_readreg __P((struct device *, int, int));
void	tlp_sio_mii_writereg __P((struct device *, int, int, int));

int	tlp_pnic_mii_readreg __P((struct device *, int, int));
void	tlp_pnic_mii_writereg __P((struct device *, int, int, int));

void	tlp_2114x_preinit __P((struct tulip_softc *));
void	tlp_pnic_preinit __P((struct tulip_softc *));

void	tlp_21140_reset __P((struct tulip_softc *));

u_int32_t tlp_crc32 __P((const u_int8_t *, size_t));
#define	tlp_mchash(addr)	(tlp_crc32((addr), ETHER_ADDR_LEN) &	\
				 (TULIP_MCHASHSIZE - 1))

#ifdef TLP_DEBUG
#define	DPRINTF(sc, x)	if ((sc)->sc_ethercom.ec_if.if_flags & IFF_DEBUG) \
				printf x
#else
#define	DPRINTF(sc, x)	/* nothing */
#endif

/*
 * tlp_attach:
 *
 *	Attach a Tulip interface to the system.
 */
void
tlp_attach(sc, enaddr)
	struct tulip_softc *sc;
	const u_int8_t *enaddr;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i, rseg, error;
	bus_dma_segment_t seg;

	/*
	 * NOTE: WE EXPECT THE FRONT-END TO INITIALIZE sc_regshift!
	 */

	/*
	 * Setup the transmit threshold table.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_DE425:
	case TULIP_CHIP_21040:
	case TULIP_CHIP_21041:
		sc->sc_txth = tlp_10_txthresh_tab;
		break;

	default:
		sc->sc_txth = tlp_10_100_txthresh_tab;
		break;
	}

	/*
	 * Setup the filter setup function.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_WB89C840F:
		sc->sc_filter_setup = tlp_winb_filter_setup;
		break;

	default:
		sc->sc_filter_setup = tlp_filter_setup;
		break;
	}

	/*
	 * Set up the media status change function.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_WB89C840F:
		sc->sc_statchg = tlp_winb_mii_statchg;
		break;

	default:
		/*
		 * We may override this if we have special media
		 * handling requirements (e.g. flipping GPIO pins).
		 *
		 * The pure-MII statchg function covers the basics.
		 */
		sc->sc_statchg = tlp_mii_statchg;
		break;
	}

	/*
	 * Set up various chip-specific quirks.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21140:
	case TULIP_CHIP_21140A:
	case TULIP_CHIP_21142:
	case TULIP_CHIP_21143:
		sc->sc_preinit = tlp_2114x_preinit;
		break;

	case TULIP_CHIP_82C168:
	case TULIP_CHIP_82C169:
		sc->sc_preinit = tlp_pnic_preinit;

		/*
		 * These chips seem to have busted DMA engines; just put them
		 * in Store-and-Forward mode from the get-go.
		 */
		sc->sc_txthresh = TXTH_SF;
		break;

	case TULIP_CHIP_WB89C840F:
		sc->sc_flags |= TULIPF_IC_FS;
		break;

	default:
		/* Nothing. */
	}

	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct tulip_control_data), PAGE_SIZE, 0, &seg, 1, &rseg,
	    0)) != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct tulip_control_data), (caddr_t *)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct tulip_control_data), 1,
	    sizeof(struct tulip_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct tulip_control_data), NULL,
	    0)) != 0) {
		printf("%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < TULIP_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    TULIP_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			printf("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
	}

	/*
	 * Create the recieve buffer DMA maps.
	 */
	for (i = 0; i < TULIP_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			printf("%s: unable to create rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/*
	 * Reset the chip to a known state.
	 */
	tlp_reset(sc);

	/* Announce ourselves. */
	printf("%s: %s%sEthernet address %s\n", sc->sc_dev.dv_xname,
	    sc->sc_name[0] != '\0' ? sc->sc_name : "",
	    sc->sc_name[0] != '\0' ? ", " : "",
	    ether_sprintf(enaddr));

	/*
	 * Initialize our media structures.  This may probe the MII, if
	 * present.
	 */
	(*sc->sc_mediasw->tmsw_init)(sc);

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = tlp_ioctl;
	ifp->if_start = tlp_start;
	ifp->if_watchdog = tlp_watchdog;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
#if NBPFILTER > 0
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#endif

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(tlp_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
 fail_5:
	for (i = 0; i < TULIP_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_4:
	for (i = 0; i < TULIP_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_control_data,
	    sizeof(struct tulip_control_data));
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 fail_0:
	return;
}

/*
 * tlp_shutdown:
 *
 *	Make sure the interface is stopped at reboot time.
 */
void
tlp_shutdown(arg)
	void *arg;
{
	struct tulip_softc *sc = arg;

	tlp_stop(sc, 1);
}

/*
 * tlp_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
void
tlp_start(ifp)
	struct ifnet *ifp;
{
	struct tulip_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct tulip_txsoft *txs, *last_txs;
	bus_dmamap_t dmamap;
	int error, firsttx, nexttx, lasttx, ofree, seg;

	DPRINTF(sc, ("%s: tlp_start: sc_flags 0x%08x, if_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags, ifp->if_flags));

	/*
	 * If we want a filter setup, it means no more descriptors were
	 * available for the setup routine.  Let it get a chance to wedge
	 * itself into the ring.
	 */
	if (sc->sc_flags & TULIPF_WANT_SETUP)
		ifp->if_flags |= IFF_OACTIVE;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors and
	 * the first descriptor we'll use.
	 */
	ofree = sc->sc_txfree;
	firsttx = sc->sc_txnext;

	DPRINTF(sc, ("%s: tlp_start: txfree %d, txnext %d\n",
	    sc->sc_dev.dv_xname, ofree, firsttx));

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txfreeq)) != NULL &&
	       sc->sc_txfree != 0) {
		/*
		 * Grab a packet off the queue.
		 */
		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		dmamap = txs->txs_dmamap;

		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the alloted number of segments, or we were
		 * short on resources.  In this case, we'll copy and try
		 * again.
		 */
		if (bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_NOWAIT) != 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				printf("%s: unable to allocate Tx mbuf\n",
				    sc->sc_dev.dv_xname);
				IF_PREPEND(&ifp->if_snd, m0);
				break;
			}
			if (m0->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					    "cluster\n", sc->sc_dev.dv_xname);
					m_freem(m);
					IF_PREPEND(&ifp->if_snd, m0);
					break;
				}
			}
			m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, caddr_t));
			m->m_pkthdr.len = m->m_len = m0->m_pkthdr.len;
			m_freem(m0);
			m0 = m;
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
			    m0, BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
				IF_PREPEND(&ifp->if_snd, m0);
				break;
			}
		}

		/*
		 * Ensure we have enough descriptors free to describe
		 * the packet.
		 */
		if (dmamap->dm_nsegs > sc->sc_txfree) {
			/*
			 * Not enough free descriptors to transmit this
			 * packet.  We haven't committed to anything yet,
			 * so just unload the DMA map, put the packet
			 * back on the queue, and punt.  Notify the upper
			 * layer that there are no more slots left.
			 *
			 * XXX We could allocate an mbuf and copy, but
			 * XXX it is worth it?
			 */
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			IF_PREPEND(&ifp->if_snd, m0);
			break;
		}

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/*
		 * Initialize the transmit descriptors.
		 */
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs;
		     seg++, nexttx = TULIP_NEXTTX(nexttx)) {
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.  That could cause a race condition.
			 * We'll do it below.
			 */
			sc->sc_txdescs[nexttx].td_status =
			    (nexttx == firsttx) ? 0 : TDSTAT_OWN;
			sc->sc_txdescs[nexttx].td_bufaddr1 =
			    dmamap->dm_segs[seg].ds_addr;
			sc->sc_txdescs[nexttx].td_ctl =
			    (dmamap->dm_segs[seg].ds_len << TDCTL_SIZE1_SHIFT) |
			    TDCTL_CH;
			lasttx = nexttx;
		}

		/* Set `first segment' and `last segment' appropriately. */
		sc->sc_txdescs[sc->sc_txnext].td_ctl |= TDCTL_Tx_FS;
		sc->sc_txdescs[lasttx].td_ctl |= TDCTL_Tx_LS;

#ifdef TLP_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			printf("     txsoft %p trainsmit chain:\n", txs);
			for (seg = sc->sc_txnext;; seg = TULIP_NEXTTX(seg)) {
				printf("     descriptor %d:\n", seg);
				printf("       td_status:   0x%08x\n",
				    sc->sc_txdescs[seg].td_status);
				printf("       td_ctl:      0x%08x\n",
				    sc->sc_txdescs[seg].td_ctl);
				printf("       td_bufaddr1: 0x%08x\n",
				    sc->sc_txdescs[seg].td_bufaddr1);
				printf("       td_bufaddr2: 0x%08x\n",
				    sc->sc_txdescs[seg].td_bufaddr2);
				if (seg == lasttx)
					break;
			}
		}
#endif

		/* Sync the descriptors we're using. */
		TULIP_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * Store a pointer to the packet so we can free it later,
		 * and remember what txdirty will be once the packet is
		 * done.
		 */
		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;

		/* Advance the tx pointer. */
		sc->sc_txfree -= dmamap->dm_nsegs;
		sc->sc_txnext = nexttx;

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txfreeq, txs, txs_q);
		SIMPLEQ_INSERT_TAIL(&sc->sc_txdirtyq, txs, txs_q);

		last_txs = txs;

#if NBPFILTER > 0
		/*
		 * Pass the packet to any BPF listeners.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif /* NBPFILTER > 0 */
	}

	if (txs == NULL || sc->sc_txfree == 0) {
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}

	if (sc->sc_txfree != ofree) {
		DPRINTF(sc, ("%s: packets enqueued, IC on %d, OWN on %d\n",
		    sc->sc_dev.dv_xname, lasttx, firsttx));
		/*
		 * Cause a transmit interrupt to happen on the
		 * last packet we enqueued.
		 */
		sc->sc_txdescs[lasttx].td_ctl |= TDCTL_Tx_IC;
		TULIP_CDTXSYNC(sc, lasttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * Some clone chips want IC on the *first* segment in
		 * the packet.  Appease them.
		 */
		if ((sc->sc_flags & TULIPF_IC_FS) != 0 &&
		    last_txs->txs_firstdesc != lasttx) {
			sc->sc_txdescs[last_txs->txs_firstdesc].td_ctl |=
			    TDCTL_Tx_IC;
			TULIP_CDTXSYNC(sc, last_txs->txs_firstdesc, 1,
			    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
		}

		/*
		 * The entire packet chain is set up.  Give the
		 * first descriptor to the chip now.
		 */
		sc->sc_txdescs[firsttx].td_status |= TDSTAT_OWN;
		TULIP_CDTXSYNC(sc, firsttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Wake up the transmitter. */
		/* XXX USE AUTOPOLLING? */
		TULIP_WRITE(sc, CSR_TXPOLL, TXPOLL_TPD);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

/*
 * tlp_watchdog:	[ifnet interface function]
 *
 *	Watchdog timer handler.
 */
void
tlp_watchdog(ifp)
	struct ifnet *ifp;
{
	struct tulip_softc *sc = ifp->if_softc;
	int doing_setup, doing_transmit;

	doing_setup = (sc->sc_flags & TULIPF_DOING_SETUP);
	doing_transmit = (SIMPLEQ_FIRST(&sc->sc_txdirtyq) != NULL);

	if (doing_setup && doing_transmit) {
		printf("%s: filter setup and transmit timeout\n",
		    sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
	} else if (doing_transmit) {
		printf("%s: transmit timeout\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
	} else if (doing_setup)
		printf("%s: filter setup timeout\n", sc->sc_dev.dv_xname);
	else
		printf("%s: spurious watchdog timeout\n", sc->sc_dev.dv_xname);

	(void) tlp_init(sc);

	/* Try to get more packets going. */
	tlp_start(ifp);
}

/*
 * tlp_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
int
tlp_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct tulip_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			if ((error = tlp_init(sc)) != 0)
				break;
			arp_ifinit(ifp, ifa);
			break;
#endif /* INET */
#ifdef NS
		case AF_NS:
		    {
			struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)
				    LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
				    ifp->if_addrlen);
			/* Set new address. */
			error = tlp_init(sc);
			break;
		    }
#endif /* NS */
		default:
			error = tlp_init(sc);
			break;
		}
		break;

	case SIOCSIFMTU:
		if (ifr->ifr_mtu > ETHERMTU)
			error = EINVAL;
		else
			ifp->if_mtu = ifr->ifr_mtu;
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			tlp_stop(sc, 1);
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interfase it marked up and it is stopped, then
			 * start it.
			 */
			error = tlp_init(sc);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect the hardware state.
			 */
			error = tlp_init(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed.  Set the filter
			 * accordingly.
			 */
			(*sc->sc_filter_setup)(sc);
			error = 0;
		}
		break;

	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;

	default:
		error = EINVAL;
		break;
	}

	/* Try to get more packets going. */
	tlp_start(ifp);

	splx(s);
	return (error);
}

/*
 * tlp_intr:
 *
 *	Interrupt service routine.
 */
int
tlp_intr(arg)
	void *arg;
{
	struct tulip_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int32_t status, rxstatus, txstatus;
	int handled = 0, txthresh;

	DPRINTF(sc, ("%s: tlp_intr\n", sc->sc_dev.dv_xname));

	/*
	 * If the interface isn't running, the interrupt couldn't
	 * possibly have come from us.
	 */
	if ((ifp->if_flags & IFF_RUNNING) == 0)
		return (0);

	for (;;) {
		status = TULIP_READ(sc, CSR_STATUS);
		if (status)
			TULIP_WRITE(sc, CSR_STATUS, status);

		if ((status & sc->sc_inten) == 0)
			break;

		handled = 1;

		rxstatus = status & sc->sc_rxint_mask;
		txstatus = status & sc->sc_txint_mask;

		if (rxstatus) {
			/* Grab new any new packets. */
			tlp_rxintr(sc);

			if (rxstatus & STATUS_RWT)
				printf("%s: receive watchdog timeout\n",
				    sc->sc_dev.dv_xname);

			if (rxstatus & STATUS_RU) {
				printf("%s: receive ring overrun\n",
				    sc->sc_dev.dv_xname);
				/* Get the receive process going again. */
				tlp_idle(sc, OPMODE_SR);
				TULIP_WRITE(sc, CSR_RXLIST,
				    TULIP_CDRXADDR(sc, sc->sc_rxptr));
				TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);
				TULIP_WRITE(sc, CSR_RXPOLL, RXPOLL_RPD);
				break;
			}
		}

		if (txstatus) {
			/* Sweep up transmit descriptors. */
			tlp_txintr(sc);

			if (txstatus & STATUS_TJT)
				printf("%s: transmit jabber timeout\n",
				    sc->sc_dev.dv_xname);

			if (txstatus & STATUS_UNF) {
				/*
				 * Increase our transmit threshold if
				 * another is available.
				 */
				txthresh = sc->sc_txthresh + 1;
				if (sc->sc_txth[txthresh].txth_name != NULL) {
					/* Idle the transmit process. */
					tlp_idle(sc, OPMODE_ST);

					sc->sc_txthresh = txthresh;
					sc->sc_opmode &= ~(OPMODE_TR|OPMODE_SF);
					sc->sc_opmode |=
					    sc->sc_txth[txthresh].txth_opmode;
					printf("%s: transmit underrun; new "
					    "threshold: %s\n",
					    sc->sc_dev.dv_xname,
					    sc->sc_txth[txthresh].txth_name);

					/*
					 * Set the new threshold and restart
					 * the transmit process.
					 */
					TULIP_WRITE(sc, CSR_OPMODE,
					    sc->sc_opmode);
				}
					/*
					 * XXX Log every Nth underrun from
					 * XXX now on?
					 */
			}
		}

		if (status & (STATUS_TPS|STATUS_RPS)) {
			if (status & STATUS_TPS)
				printf("%s: transmit process stopped\n",
				    sc->sc_dev.dv_xname);
			if (status & STATUS_RPS)
				printf("%s: receive process stopped\n",
				    sc->sc_dev.dv_xname);
			(void) tlp_init(sc);
			break;
		}

		if (status & STATUS_SE) {
			const char *str;
			switch (status & STATUS_EB) {
			case STATUS_EB_PARITY:
				str = "parity error";
				break;

			case STATUS_EB_MABT:
				str = "master abort";
				break;

			case STATUS_EB_TABT:
				str = "target abort";
				break;

			default:
				str = "unknown error";
				break;
			}
			printf("%s: fatal system error: %s\n",
			    sc->sc_dev.dv_xname, str);
			(void) tlp_init(sc);
			break;
		}

		/*
		 * Not handled:
		 *
		 *	Transmit buffer unavailable -- normal
		 *	condition, nothing to do, really.
		 *
		 *	General purpose timer experied -- we don't
		 *	use the general purpose timer.
		 *
		 *	Early receive interrupt -- not available on
		 *	all chips, we just use RI.  We also only
		 *	use single-segment receive DMA, so this
		 *	is mostly useless.
		 */
	}

	/* Try to get more packets going. */
	tlp_start(ifp);

	return (handled);
}

/*
 * tlp_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
void
tlp_rxintr(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_header *eh;
	struct tulip_rxsoft *rxs;
	struct mbuf *m;
	u_int32_t rxstat;
	int i, len;

	for (i = sc->sc_rxptr;; i = TULIP_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		TULIP_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].td_status;

		if (rxstat & TDSTAT_OWN) {
			/*
			 * We have processed all of the receive buffers.
			 */
			break;
		}

		/*
		 * Make sure the packet fit in one buffer.  This should
		 * always be the case.  But the Lite-On PNIC, rev 33
		 * has an awful receive engine bug, which may require
		 * a very icky work-around.
		 */
		if ((rxstat & (TDSTAT_Rx_FS|TDSTAT_Rx_LS)) !=
		    (TDSTAT_Rx_FS|TDSTAT_Rx_LS)) {
			printf("%s: incoming packet spilled, resetting\n",
			    sc->sc_dev.dv_xname);
			(void) tlp_init(sc);
			return;
		}

		/*
		 * If any collisions were seen on the wire, count one.
		 */
		if (rxstat & TDSTAT_Rx_CS)
			ifp->if_collisions++;

		/*
		 * If an error occured, update stats, clear the status
		 * word, and leave the packet buffer in place.  It will
		 * simply be reused the next time the ring comes around.
		 */
		if (rxstat & TDSTAT_ES) {
#define	PRINTERR(bit, str)						\
			if (rxstat & (bit))				\
				printf("%s: receive error: %s\n",	\
				    sc->sc_dev.dv_xname, str)
			ifp->if_ierrors++;
			PRINTERR(TDSTAT_Rx_DE, "descriptor error");
			PRINTERR(TDSTAT_Rx_RF, "runt frame");
			PRINTERR(TDSTAT_Rx_TL, "frame too long");
			PRINTERR(TDSTAT_Rx_RE, "MII error");
			PRINTERR(TDSTAT_Rx_DB, "dribbling bit");
			PRINTERR(TDSTAT_Rx_CE, "CRC error");
#undef PRINTERR
			TULIP_INIT_RXDESC(sc, i);
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		/*
		 * No errors; receive the packet.  Note the Tulip
		 * includes the CRC with every packet; trim it.
		 */
		len = TDSTAT_Rx_LENGTH(rxstat) - ETHER_CRC_LEN;

#ifdef __NO_STRICT_ALIGNMENT
		/*
		 * Allocate a new mbuf cluster.  If that fails, we are
		 * out of memory, and must drop the packet and recycle
		 * the buffer that's already attached to this descriptor.
		 */
		m = rxs->rxs_mbuf;
		if (tlp_add_rxbuf(sc, i) != 0) {
			ifp->if_ierrors++;
			TULIP_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			continue;
		}
#else
		/*
		 * The Tulip's receive buffers must be 4-byte aligned.
		 * But this means that the data after the Ethernet header
		 * is misaligned.  We must allocate a new buffer and
		 * copy the data, shifted forward 2 bytes.
		 */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
 dropit:
			ifp->if_ierrors++;
			TULIP_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			continue;
		}
		if (len > (MHLEN - 2)) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(m);
				goto dropit;
			}
		}
		m->m_data += 2;

		/*
		 * Note that we use clusters for incoming frames, so the
		 * buffer is virtually contiguous.
		 */
		memcpy(mtod(m, caddr_t), mtod(rxs->rxs_mbuf, caddr_t), len);

		/* Allow the receive descriptor to continue using its mbuf. */
		TULIP_INIT_RXDESC(sc, i);
		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
#endif /* __NO_STRICT_ALIGNMENT */

		ifp->if_ipackets++;
		eh = mtod(m, struct ether_header *);
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass it up the stack if its for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NPBFILTER > 0 */

		/*
		 * This test is outside the NBPFILTER block because
		 * on the 21140 we have to use Hash-Only mode due to
		 * a bug in the filter logic.
		 */
		if ((ifp->if_flags & IFF_PROMISC) != 0 ||
		    sc->sc_filtmode == TDCTL_Tx_FT_HASHONLY) {
			if (memcmp(LLADDR(ifp->if_sadl), eh->ether_dhost,
				 ETHER_ADDR_LEN) != 0 &&
			    ETHER_IS_MULTICAST(eh->ether_dhost) == 0) {
				m_freem(m);
				continue;
			}
		}

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the recieve pointer. */
	sc->sc_rxptr = i;
}

/*
 * tlp_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
void
tlp_txintr(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct tulip_txsoft *txs;
	u_int32_t txstat;

	DPRINTF(sc, ("%s: tlp_txintr: sc_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags));

	ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * If we were doing a filter setup, check to see if it completed.
	 */
	if (sc->sc_flags & TULIPF_DOING_SETUP) {
		TULIP_CDSDSYNC(sc, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);
		if ((sc->sc_setup_desc.td_status & TDSTAT_OWN) == 0)
			sc->sc_flags &= ~TULIPF_DOING_SETUP;
	}

	/*
	 * Go through our Tx list and free mbufs for those
	 * frames that have been transmitted.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		TULIP_CDTXSYNC(sc, txs->txs_firstdesc,
		    txs->txs_dmamap->dm_nsegs,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

#ifdef TLP_DEBUG
		if (ifp->if_flags & IFF_DEBUG) {
			int i;
			printf("    txsoft %p trainsmit chain:\n", txs);
			for (i = txs->txs_firstdesc;; i = TULIP_NEXTTX(i)) {
				printf("     descriptor %d:\n", i);
				printf("       td_status:   0x%08x\n",
				    sc->sc_txdescs[i].td_status);
				printf("       td_ctl:      0x%08x\n",
				    sc->sc_txdescs[i].td_ctl);
				printf("       td_bufaddr1: 0x%08x\n",
				    sc->sc_txdescs[i].td_bufaddr1);
				printf("       td_bufaddr2: 0x%08x\n",
				    sc->sc_txdescs[i].td_bufaddr2);
				if (i == txs->txs_lastdesc)
					break;
			}
		}
#endif

		txstat = sc->sc_txdescs[txs->txs_firstdesc].td_status;
		if (txstat & TDSTAT_OWN)
			break;

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs, txs_q);

		sc->sc_txfree += txs->txs_dmamap->dm_nsegs;

		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize,
		    BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;

		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);

		/*
		 * Check for errors and collisions.
		 */
		if (txstat &
		    (TDSTAT_Tx_UF|TDSTAT_Tx_NC|TDSTAT_Tx_LO|TDSTAT_Tx_TO)) {
			ifp->if_oerrors++;
#if 0
			/*
			 * XXX Can't check for late or excessive collisions;
			 * XXX Some 21040s seem to register those even on
			 * XXX successful transmissions!
			 */
			if (txstat & TDSTAT_Tx_EC)
				ifp->if_collisions += 16;
			if (txstat & TDSTAT_Tx_LC)
				ifp->if_collisions++;
#endif
		} else {
			/* Packet was transmitted successfully. */
			ifp->if_opackets++;
			ifp->if_collisions += TDSTAT_Tx_COLLISIONS(txstat);
		}
	}

	/*
	 * If there are no more pending transmissions, cancel the watchdog
	 * timer.
	 */
	if (txs == NULL && (sc->sc_flags & TULIPF_DOING_SETUP) == 0)
		ifp->if_timer = 0;

	/*
	 * If we have a receive filter setup pending, do it now.
	 */
	if (sc->sc_flags & TULIPF_WANT_SETUP)
		(*sc->sc_filter_setup)(sc);
}

/*
 * tlp_reset:
 *
 *	Perform a soft reset on the Tulip.
 */
void
tlp_reset(sc)
	struct tulip_softc *sc;
{
	int i;

	TULIP_WRITE(sc, CSR_BUSMODE, BUSMODE_SWR);

	for (i = 0; i < 1000; i++) {
		/*
		 * Wait at least 50 PCI cycles for the reset to
		 * complete before peeking at the Tulip again.
		 * 10 uSec is a bit longer than 50 PCI cycles
		 * (at 33MHz), but it doesn't hurt have the extra
		 * wait.
		 */
		delay(10);
		if (TULIP_ISSET(sc, CSR_BUSMODE, BUSMODE_SWR) == 0)
			break;
	}

	if (TULIP_ISSET(sc, CSR_BUSMODE, BUSMODE_SWR))
		printf("%s: reset failed to complete\n", sc->sc_dev.dv_xname);

	delay(1000);

	/*
	 * If the board has any GPIO reset sequences to issue, do them now.
	 */
	if (sc->sc_reset != NULL)
		(*sc->sc_reset)(sc);
}

/*
 * tlp_init:
 *
 *	Initialize the interface.  Must be called at splnet().
 */
int
tlp_init(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct tulip_txsoft *txs;
	struct tulip_rxsoft *rxs;
	int i, error = 0;

	/*
	 * Cancel any pending I/O.
	 */
	tlp_stop(sc, 0);

	/*
	 * Initialize `opmode' to 0, and call the pre-init routine, if
	 * any.  This is required because the 2114x and some of the
	 * clones require that the media-related bits in `opmode' be
	 * set before performing a soft-reset in order to get internal
	 * chip pathways are correct.  Yay!
	 */
	sc->sc_opmode = 0;
	if (sc->sc_preinit != NULL)
		(*sc->sc_preinit)(sc);

	/*
	 * Reset the Tulip to a known state.
	 */
	tlp_reset(sc);

	/*
	 * Initialize the BUSMODE register.
	 *
	 * XXX What about read-multiple/read-line/write-line on
	 * XXX the 21140 and up?
	 */
	sc->sc_busmode = BUSMODE_BAR;
	switch (sc->sc_cacheline) {
	default:
		/*
		 * Note: We must *always* set these bits; a cache
		 * alignment of 0 is RESERVED.
		 */
	case 8:
		sc->sc_busmode |= BUSMODE_CAL_8LW;
		break;
	case 16:
		sc->sc_busmode |= BUSMODE_CAL_16LW;
		break;
	case 32:
		sc->sc_busmode |= BUSMODE_CAL_32LW;
		break;
	}
	switch (sc->sc_chip) {
	case TULIP_CHIP_82C168:
	case TULIP_CHIP_82C169:
		sc->sc_busmode |= BUSMODE_PBL_16LW | BUSMODE_PNIC_MBO;
		break;
	default:
		sc->sc_busmode |= BUSMODE_PBL_DEFAULT;
		break;
	}
#if BYTE_ORDER == BIG_ENDIAN
	/*
	 * XXX There are reports that this doesn't work properly
	 * in the old Tulip driver, but BUSMODE_DBO does.  However,
	 * BUSMODE_DBO is not available on the 21040, and requires
	 * us to byte-swap the setup packet.  What to do?
	 */
	sc->sc_busmode |= BUSMODE_BLE;
#endif
	TULIP_WRITE(sc, CSR_BUSMODE, sc->sc_busmode);

	/*
	 * Initialize the OPMODE register.  We don't write it until
	 * we're ready to begin the transmit and receive processes.
	 *
	 * Media-related OPMODE bits are set in the media callbacks
	 * for each specific chip/board.
	 */
	sc->sc_opmode |= OPMODE_SR | OPMODE_ST |
	    sc->sc_txth[sc->sc_txthresh].txth_opmode;

	/*
	 * Magical mystery initialization on the Macronix chips.
	 * The MX98713 uses its own magic value, the rest share
	 * a common one.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_MX98713:
		TULIP_WRITE(sc, CSR_PMAC_TOR, PMAC_TOR_98713);
		break;

	case TULIP_CHIP_MX98713A:
	case TULIP_CHIP_MX98715:
	case TULIP_CHIP_MX98725:
		TULIP_WRITE(sc, CSR_PMAC_TOR, PMAC_TOR_98715);
		break;

	default:
		/* Nothing. */
	}

	/*
	 * Initialize the transmit descriptor ring.
	 */
	memset(sc->sc_txdescs, 0, sizeof(sc->sc_txdescs));
	for (i = 0; i < TULIP_NTXDESC; i++) {
		sc->sc_txdescs[i].td_ctl = TDCTL_CH;
		sc->sc_txdescs[i].td_bufaddr2 =
		    TULIP_CDTXADDR(sc, TULIP_NEXTTX(i));
	}
	TULIP_CDTXSYNC(sc, 0, TULIP_NTXDESC,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = TULIP_NTXDESC;
	sc->sc_txnext = 0;

	/*
	 * Initialize the transmit job descriptors.
	 */
	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);
	for (i = 0; i < TULIP_TXQUEUELEN; i++) {
		txs = &sc->sc_txsoft[i];
		txs->txs_mbuf = NULL;
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	/*
	 * Initialize the receive descriptor and receive job
	 * descriptor rings.
	 */
	for (i = 0; i < TULIP_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf == NULL) {
			if ((error = tlp_add_rxbuf(sc, i)) != 0) {
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				tlp_rxdrain(sc);
				goto out;
			}
		}
	}
	sc->sc_rxptr = 0;

	/*
	 * Initialize the interrupt mask and enable interrupts.
	 */
	/* normal interrupts */
	sc->sc_inten =  STATUS_TI | STATUS_TU | STATUS_RI | STATUS_NIS;

	/* abnormal interrupts */
	sc->sc_inten |= STATUS_TPS | STATUS_TJT | STATUS_UNF |
	    STATUS_RU | STATUS_RPS | STATUS_RWT | STATUS_SE | STATUS_AIS;

	sc->sc_rxint_mask = STATUS_RI|STATUS_RU|STATUS_RWT;
	sc->sc_txint_mask = STATUS_TI|STATUS_UNF|STATUS_TJT;

	switch (sc->sc_chip) {
	case TULIP_CHIP_WB89C840F:
		/*
		 * Clear bits that we don't want that happen to
		 * overlap or don't exist.
		 */
		sc->sc_inten &= ~(STATUS_WINB_REI|STATUS_RWT);
		break;

	default:
		/* Nothing. */
	}

	sc->sc_rxint_mask &= sc->sc_inten;
	sc->sc_txint_mask &= sc->sc_inten;

	TULIP_WRITE(sc, CSR_INTEN, sc->sc_inten);
	TULIP_WRITE(sc, CSR_STATUS, 0xffffffff);

	/*
	 * Give the transmit and receive rings to the Tulip.
	 */
	TULIP_WRITE(sc, CSR_TXLIST, TULIP_CDTXADDR(sc, sc->sc_txnext));
	TULIP_WRITE(sc, CSR_RXLIST, TULIP_CDRXADDR(sc, sc->sc_rxptr));

	/*
	 * On chips that do this differently, set the station address.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_WB89C840F:
	    {
		/* XXX Do this with stream writes? */
		bus_addr_t cpa = TULIP_CSR_OFFSET(sc, CSR_WINB_CPA0);

		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			bus_space_write_1(sc->sc_st, sc->sc_sh,
			    cpa + i, LLADDR(ifp->if_sadl)[i]);
		}
		break;
	    }

	default:
		/* Nothing. */
	}

	/*
	 * Set the receive filter.  This will start the transmit and
	 * receive processes.
	 */
	(*sc->sc_filter_setup)(sc);

	/*
	 * Set the current media.
	 */
	(void) (*sc->sc_mediasw->tmsw_set)(sc);

	/*
	 * Start the receive process.
	 */
	TULIP_WRITE(sc, CSR_RXPOLL, RXPOLL_RPD);

	if (sc->sc_tick != NULL) {
		/* Start the one second clock. */
		timeout(sc->sc_tick, sc, hz);
	}

	/*
	 * Note that the interface is now running.
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

 out:
	if (error)
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	return (error);
}

/*
 * tlp_rxdrain:
 *
 *	Drain the receive queue.
 */
void
tlp_rxdrain(sc)
	struct tulip_softc *sc;
{
	struct tulip_rxsoft *rxs;
	int i;

	for (i = 0; i < TULIP_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/*
 * tlp_stop:
 *
 *	Stop transmission on the interface.
 */
void
tlp_stop(sc, drain)
	struct tulip_softc *sc;
	int drain;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct tulip_txsoft *txs;

	if (sc->sc_tick != NULL) {
		/* Stop the one second clock. */
		untimeout(sc->sc_tick, sc);
	}

	/* Disable interrupts. */
	TULIP_WRITE(sc, CSR_INTEN, 0);

	/* Stop the transmit and receive processes. */
	TULIP_WRITE(sc, CSR_OPMODE, 0);
	TULIP_WRITE(sc, CSR_RXLIST, 0);
	TULIP_WRITE(sc, CSR_TXLIST, 0);

	/*
	 * Release any queued transmit buffers.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs, txs_q);
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	if (drain) {
		/*
		 * Release the receive buffers.
		 */
		tlp_rxdrain(sc);
	}

	sc->sc_flags &= ~(TULIPF_WANT_SETUP|TULIPF_DOING_SETUP);

	/*
	 * Mark the interface down and cancel the watchdog timer.
	 */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

#define	SROM_EMIT(sc, x)						\
do {									\
	TULIP_WRITE((sc), CSR_MIIROM, (x));				\
	delay(1);							\
} while (0)

/*
 * tlp_srom_idle:
 *
 *	Put the SROM in idle state.
 */
void
tlp_srom_idle(sc)
	struct tulip_softc *sc;
{
	u_int32_t miirom;
	int i;

	miirom = MIIROM_SR;
	SROM_EMIT(sc, miirom);

	miirom |= MIIROM_RD;
	SROM_EMIT(sc, miirom);

	miirom |= MIIROM_SROMCS;
	SROM_EMIT(sc, miirom);

	SROM_EMIT(sc, miirom|MIIROM_SROMSK);

	/* Strobe the clock 25 times. */
	for (i = 0; i < 25; i++) {
		SROM_EMIT(sc, miirom);
		SROM_EMIT(sc, miirom|MIIROM_SROMSK);
	}

	SROM_EMIT(sc, miirom);

	miirom &= ~MIIROM_SROMCS;
	SROM_EMIT(sc, miirom);

	SROM_EMIT(sc, 0);
}

/*
 * tlp_read_srom:
 *
 *	Read the Tulip SROM.
 */
void
tlp_read_srom(sc, word, wordcnt, data)
	struct tulip_softc *sc;
	int word, wordcnt;
	u_int8_t *data;
{
	u_int32_t miirom;
	u_int16_t datain;
	int i, x;

	tlp_srom_idle(sc);

	/* Select the SROM. */
	miirom = MIIROM_SR;
	SROM_EMIT(sc, miirom);

	miirom |= MIIROM_RD;
	SROM_EMIT(sc, miirom);

	for (i = 0; i < wordcnt; i++) {
		/* Send CHIP SELECT for one clock tick. */
		miirom |= MIIROM_SROMCS;
		SROM_EMIT(sc, miirom);

		/* Shift in the READ opcode. */
		for (x = 3; x > 0; x--) {
			if (TULIP_SROM_OPC_READ & (1 << (x - 1)))
				miirom |= MIIROM_SROMDI;
			else
				miirom &= ~MIIROM_SROMDI;
			SROM_EMIT(sc, miirom);
			SROM_EMIT(sc, miirom|MIIROM_SROMSK);
			SROM_EMIT(sc, miirom);
		}

		/* Shift in address. */
		for (x = 6; x > 0; x--) {
			if ((word + i) & (1 << (x - 1)))
				miirom |= MIIROM_SROMDI;
			else
				miirom &= ~MIIROM_SROMDI;
			SROM_EMIT(sc, miirom);
			SROM_EMIT(sc, miirom|MIIROM_SROMSK);
			SROM_EMIT(sc, miirom);
		}

		/* Shift out data. */
		miirom &= ~MIIROM_SROMDI;
		datain = 0;
		for (x = 16; x > 0; x--) {
			SROM_EMIT(sc, miirom|MIIROM_SROMSK);
			if (TULIP_ISSET(sc, CSR_MIIROM, MIIROM_SROMDO))
				datain |= (1 << (x - 1));
			SROM_EMIT(sc, miirom);
		}
		data[2 * i] = datain & 0xff;
		data[(2 * i) + 1] = datain >> 8;

		/* Clear CHIP SELECT. */
		miirom &= ~MIIROM_SROMCS;
		SROM_EMIT(sc, miirom);
	}

	/* Deselect the SROM. */
	SROM_EMIT(sc, 0);

	/* ...and idle it. */
	tlp_srom_idle(sc);
}

#undef SROM_EMIT

/*
 * tlp_add_rxbuf:
 *
 *	Add a receive buffer to the indicated descriptor.
 */
int
tlp_add_rxbuf(sc, idx)	
	struct tulip_softc *sc;
	int idx;
{
	struct tulip_rxsoft *rxs = &sc->sc_rxsoft[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (ENOBUFS);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return (ENOBUFS);
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, idx, error);
		panic("tlp_add_rxbuf");	/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	TULIP_INIT_RXDESC(sc, idx);

	return (0);
}

/*
 * tlp_crc32:
 *
 *	Compute the 32-bit CRC of the provided buffer.
 */
u_int32_t
tlp_crc32(buf, len)
	const u_int8_t *buf;
	size_t len;
{
	static const u_int32_t crctab[] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};
	u_int32_t crc;
	int i;

	crc = 0xffffffff;
	for (i = 0; i < len; i++) {
		crc ^= buf[i];
		crc = (crc >> 4) ^ crctab[crc & 0xf];
		crc = (crc >> 4) ^ crctab[crc & 0xf];
	}
	return (crc);
}

/*
 * tlp_srom_crcok:
 *
 *	Check the CRC of the Tulip SROM.
 */
int
tlp_srom_crcok(romdata)
	const u_int8_t *romdata;
{
	u_int32_t crc;

	crc = tlp_crc32(romdata, TULIP_ROM_CRC32_CHECKSUM);
	crc = (crc & 0xffff) ^ 0xffff;
	if (crc == TULIP_ROM_GETW(romdata, TULIP_ROM_CRC32_CHECKSUM))
		return (1);
	return (0);
}

/*
 * tlp_isv_srom:
 *
 *	Check to see if the SROM is in the new standardized format.
 */
int
tlp_isv_srom(romdata)
	const u_int8_t *romdata;
{
	int i;
	u_int16_t cksum;

	if (tlp_srom_crcok(romdata)) {
		/*
		 * SROM CRC checks out; must be in the new format.
		 */
		return (1);
	}

	cksum = TULIP_ROM_GETW(romdata, TULIP_ROM_CRC32_CHECKSUM);
	if (cksum == 0xffff || cksum == 0) {
		/*
		 * No checksum present.  Check the SROM ID; 18 bytes of 0
		 * followed by 1 (version) followed by the number of
		 * adapters which use this SROM (should be non-zero).
		 */
		for (i = 0; i < TULIP_ROM_SROM_FORMAT_VERION; i++) {
			if (romdata[i] != 0)
				return (0);
		}
		if (romdata[TULIP_ROM_SROM_FORMAT_VERION] != 1)
			return (0);
		if (romdata[TULIP_ROM_CHIP_COUNT] == 0)
			return (0);
		return (1);
	}

	return (0);
}

/*
 * tlp_isv_srom_enaddr:
 *
 *	Get the Ethernet address from an ISV SROM.
 */
int
tlp_isv_srom_enaddr(sc, enaddr)
	struct tulip_softc *sc;
	u_int8_t *enaddr;
{
	int i, devcnt;

	if (tlp_isv_srom(sc->sc_srom) == 0)
		return (0);

	devcnt = sc->sc_srom[TULIP_ROM_CHIP_COUNT];
	for (i = 0; i < devcnt; i++) {
		if (sc->sc_srom[TULIP_ROM_CHIP_COUNT] == 1)
			break;
		if (sc->sc_srom[TULIP_ROM_CHIPn_DEVICE_NUMBER(i)] ==
		    sc->sc_devno)
			break;
	}

	if (i == devcnt)
		return (0);

	memcpy(enaddr, &sc->sc_srom[TULIP_ROM_IEEE_NETWORK_ADDRESS],
	    ETHER_ADDR_LEN);
	enaddr[5] += i;

	return (1);
}

/*
 * tlp_parse_old_srom:
 *
 *	Parse old-format SROMs.
 *
 *	This routine is largely lifted from Matt Thomas's `de' driver.
 */
int
tlp_parse_old_srom(sc, enaddr)
	struct tulip_softc *sc;
	u_int8_t *enaddr;
{
	static const u_int8_t testpat[] =
	    { 0xff, 0, 0x55, 0xaa, 0xff, 0, 0x55, 0xaa };
	int i;
	u_int32_t cksum;

	if (memcmp(&sc->sc_srom[0], &sc->sc_srom[16], 8) != 0) {
		/*
		 * Some vendors (e.g. ZNYX) don't use the standard
		 * DEC Address ROM format, but rather just have an
		 * Ethernet address in the first 6 bytes, maybe a
		 * 2 byte checksum, and then all 0xff's.
		 */
		for (i = 8; i < 32; i++) {
			if (sc->sc_srom[i] != 0xff)
				return (0);
		}

		/*
		 * Sanity check the Ethernet address:
		 *
		 *	- Make sure it's not multicast or locally
		 *	  assigned
		 *	- Make sure it has a non-0 OUI
		 */
		if (sc->sc_srom[0] & 3)
			return (0);
		if (sc->sc_srom[0] == 0 && sc->sc_srom[1] == 0 &&
		    sc->sc_srom[2] == 0)
			return (0);

		memcpy(enaddr, sc->sc_srom, ETHER_ADDR_LEN);
		return (1);
	}

	/*
	 * Standard DEC Address ROM test.
	 */

	if (memcmp(&sc->sc_srom[24], testpat, 8) != 0)
		return (0);

	for (i = 0; i < 8; i++) {
		if (sc->sc_srom[i] != sc->sc_srom[15 - i])
			return (0);
	}

	memcpy(enaddr, sc->sc_srom, ETHER_ADDR_LEN);

	cksum = *(u_int16_t *) &enaddr[0];

	cksum <<= 1;
	if (cksum > 0xffff)
		cksum -= 0xffff;

	cksum += *(u_int16_t *) &enaddr[2];
	if (cksum > 0xffff)
		cksum -= 0xffff;

	cksum <<= 1;
	if (cksum > 0xffff)
		cksum -= 0xffff;

	cksum += *(u_int16_t *) &enaddr[4];
	if (cksum >= 0xffff)
		cksum -= 0xffff;

	if (cksum != *(u_int16_t *) &sc->sc_srom[6])
		return (0);

	return (1);
}

/*
 * tlp_filter_setup:
 *
 *	Set the Tulip's receive filter.
 */
void
tlp_filter_setup(sc)
	struct tulip_softc *sc;
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	__volatile u_int32_t *sp;
	u_int8_t enaddr[ETHER_ADDR_LEN];
	u_int32_t hash;
	int cnt;

	DPRINTF(sc, ("%s: tlp_filter_setup: sc_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags));

	memcpy(enaddr, LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);

	/*
	 * If there are transmissions pending, wait until they have
	 * completed.
	 */
	if (SIMPLEQ_FIRST(&sc->sc_txdirtyq) != NULL ||
	    (sc->sc_flags & TULIPF_DOING_SETUP) != 0) {
		sc->sc_flags |= TULIPF_WANT_SETUP;
		DPRINTF(sc, ("%s: tlp_filter_setup: deferring\n",
		    sc->sc_dev.dv_xname));
		return;
	}
	sc->sc_flags &= ~TULIPF_WANT_SETUP;

	/*
	 * If we're running, idle the transmit and receive engines.  If
	 * we're NOT running, we're being called from tlp_init(), and our
	 * writing OPMODE will start the transmit and receive processes
	 * in motion.
	 */
	if (ifp->if_flags & IFF_RUNNING) {
		/*
		 * Actually, some chips seem to need a really hard
		 * kick in the head for this to work.  The genuine
		 * DEC chips can just be idled, but some of the
		 * clones seem to REALLY want a reset here.  Doing
		 * the reset will end up here again, but with
		 * IFF_RUNNING cleared.
		 */
		switch (sc->sc_chip) {
		case TULIP_CHIP_82C168:
		case TULIP_CHIP_82C169:
			tlp_init(sc);
			return;

		default:
			tlp_idle(sc, OPMODE_ST|OPMODE_SR);
		}
	}

	sc->sc_opmode &= ~(OPMODE_PR|OPMODE_PM);

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_opmode |= OPMODE_PR;
		goto allmulti;
	}

	/*
	 * Try Perfect filtering first.
	 */

	sc->sc_filtmode = TDCTL_Tx_FT_PERFECT;
	sp = TULIP_CDSP(sc);
	memset(TULIP_CDSP(sc), 0, TULIP_SETUP_PACKET_LEN);
	cnt = 0;
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}
		if (cnt == (TULIP_MAXADDRS - 2)) {
			/*
			 * We already have our multicast limit (still need
			 * our station address and broadcast).  Go to
			 * Hash-Perfect mode.
			 */
			goto hashperfect;
		}
		*sp++ = ((u_int16_t *) enm->enm_addrlo)[0];
		*sp++ = ((u_int16_t *) enm->enm_addrlo)[1];
		*sp++ = ((u_int16_t *) enm->enm_addrlo)[2];
		ETHER_NEXT_MULTI(step, enm);
	}
	
	if (ifp->if_flags & IFF_BROADCAST) {
		/* ...and the broadcast address. */
		cnt++;
		*sp++ = 0xffff;
		*sp++ = 0xffff;
		*sp++ = 0xffff;
	}

	/* Pad the rest with our station address. */
	for (; cnt < TULIP_MAXADDRS; cnt++) {
		*sp++ = ((u_int16_t *) enaddr)[0];
		*sp++ = ((u_int16_t *) enaddr)[1];
		*sp++ = ((u_int16_t *) enaddr)[2];
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 hashperfect:
	/*
	 * Try Hash-Perfect mode.
	 */

	/*
	 * Some 21140 chips have broken Hash-Perfect modes.  On these
	 * chips, we simply use Hash-Only mode, and put our station
	 * address into the filter.
	 */
	if (sc->sc_chip == TULIP_CHIP_21140)
		sc->sc_filtmode = TDCTL_Tx_FT_HASHONLY;
	else
		sc->sc_filtmode = TDCTL_Tx_FT_HASH;
	sp = TULIP_CDSP(sc);
	memset(TULIP_CDSP(sc), 0, TULIP_SETUP_PACKET_LEN);
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}
		hash = tlp_mchash(enm->enm_addrlo);
		sp[hash >> 4] |= 1 << (hash & 0xf);
		ETHER_NEXT_MULTI(step, enm);
	}

	if (ifp->if_flags & IFF_BROADCAST) {
		/* ...and the broadcast address. */
		hash = tlp_mchash(etherbroadcastaddr);
		sp[hash >> 4] |= 1 << (hash & 0xf);
	}

	if (sc->sc_filtmode == TDCTL_Tx_FT_HASHONLY) {
		/* ...and our station address. */
		hash = tlp_mchash(enaddr);
		sp[hash >> 4] |= 1 << (hash & 0xf);
	} else {
		/*
		 * Hash-Perfect mode; put our station address after
		 * the hash table.
		 */
		sp[39] = ((u_int16_t *) enaddr)[0];
		sp[40] = ((u_int16_t *) enaddr)[1];
		sp[41] = ((u_int16_t *) enaddr)[2];
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 allmulti:
	/*
	 * Use Perfect filter mode.  First address is the broadcast address,
	 * and pad the rest with our station address.  We'll set Pass-all-
	 * multicast in OPMODE below.
	 */
	sc->sc_filtmode = TDCTL_Tx_FT_PERFECT;
	sp = TULIP_CDSP(sc);
	memset(TULIP_CDSP(sc), 0, TULIP_SETUP_PACKET_LEN);
	cnt = 0;
	if (ifp->if_flags & IFF_BROADCAST) {
		cnt++;
		*sp++ = 0xffff;
		*sp++ = 0xffff;
		*sp++ = 0xffff;
	}
	for (; cnt < TULIP_MAXADDRS; cnt++) {
		*sp++ = ((u_int16_t *) enaddr)[0];
		*sp++ = ((u_int16_t *) enaddr)[1];
		*sp++ = ((u_int16_t *) enaddr)[2];
	}
	ifp->if_flags |= IFF_ALLMULTI;

 setit:
	if (ifp->if_flags & IFF_ALLMULTI)
		sc->sc_opmode |= OPMODE_PM;

	/* Sync the setup packet buffer. */
	TULIP_CDSPSYNC(sc, BUS_DMASYNC_PREWRITE);

	/*
	 * Fill in the setup packet descriptor.
	 */
	sc->sc_setup_desc.td_bufaddr1 = TULIP_CDSPADDR(sc);
	sc->sc_setup_desc.td_bufaddr2 = TULIP_CDTXADDR(sc, sc->sc_txnext);
	sc->sc_setup_desc.td_ctl =
	    (TULIP_SETUP_PACKET_LEN << TDCTL_SIZE1_SHIFT) |
	    sc->sc_filtmode | TDCTL_Tx_SET | TDCTL_Tx_FS | TDCTL_Tx_LS |
	    TDCTL_Tx_IC | TDCTL_CH;
	sc->sc_setup_desc.td_status = TDSTAT_OWN;
	TULIP_CDSDSYNC(sc, BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/*
	 * Write the address of the setup descriptor.  This also has
	 * the side effect of giving the transmit ring to the chip,
	 * since the setup descriptor points to the next available
	 * descriptor in the ring.
	 */
	TULIP_WRITE(sc, CSR_TXLIST, TULIP_CDSDADDR(sc));

	/*
	 * Set the OPMODE register.  This will also resume the
	 * transmit transmit process we idled above.
	 */
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);

	sc->sc_flags |= TULIPF_DOING_SETUP;

	/*
	 * Kick the transmitter; this will cause the Tulip to
	 * read the setup descriptor.
	 */
	/* XXX USE AUTOPOLLING? */
	TULIP_WRITE(sc, CSR_TXPOLL, TXPOLL_TPD);

	/* Set up a watchdog timer in case the chip flakes out. */
	ifp->if_timer = 5;

	DPRINTF(sc, ("%s: tlp_filter_setup: returning\n", sc->sc_dev.dv_xname));
}

/*
 * tlp_winb_filter_setup:
 *
 *	Set the Winbond 89C840F's receive filter.
 */
void
tlp_winb_filter_setup(sc)
	struct tulip_softc *sc;
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int32_t hash, mchash[2];

	DPRINTF(sc, ("%s: tlp_winb_filter_setup: sc_flags 0x%08x\n",
	    sc->sc_dev.dv_xname, sc->sc_flags));

	sc->sc_opmode &= ~(OPMODE_WINB_APP|OPMODE_WINB_AMP|OPMODE_WINB_ABP);

	if (ifp->if_flags & IFF_MULTICAST)
		sc->sc_opmode |= OPMODE_WINB_AMP;

	if (ifp->if_flags & IFF_BROADCAST)
		sc->sc_opmode |= OPMODE_WINB_ABP;

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_opmode |= OPMODE_WINB_APP;
		goto allmulti;
	}

	mchash[0] = mchash[1] = 0;

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}

		/*
		 * According to the FreeBSD `wb' driver, yes, you
		 * really do invert the hash.
		 */
		hash = (~(tlp_crc32(enm->enm_addrlo, ETHER_ADDR_LEN) >> 26))
		    & 0x3f;
		mchash[hash >> 5] |= 1 << (hash & 0x1f);
		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	mchash[0] = mchash[1] = 0xffffffff;

 setit:
	TULIP_WRITE(sc, CSR_WINB_CMA0, mchash[0]);
	TULIP_WRITE(sc, CSR_WINB_CMA1, mchash[1]);
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);
	DPRINTF(sc, ("%s: tlp_winb_filter_setup: returning\n",
	    sc->sc_dev.dv_xname));
}

/*
 * tlp_idle:
 *
 *	Cause the transmit and/or receive processes to go idle.
 */
void
tlp_idle(sc, bits)
	struct tulip_softc *sc;
	u_int32_t bits;
{
	static const char *tx_state_names[] = {
		"STOPPED",
		"RUNNING - FETCH",
		"RUNNING - WAIT",
		"RUNNING - READING",
		"-- RESERVED --",
		"RUNNING - SETUP",
		"SUSPENDED",
		"RUNNING - CLOSE",
	};
	static const char *rx_state_names[] = {
		"STOPPED",
		"RUNNING - FETCH",
		"RUNNING - CHECK",
		"RUNNING - WAIT",
		"SUSPENDED",
		"RUNNING - CLOSE",
		"RUNNING - FLUSH",
		"RUNNING - QUEUE",
	};
	u_int32_t csr, ackmask = 0;
	int i;

	if (bits & OPMODE_ST)
		ackmask |= STATUS_TPS;

	if (bits & OPMODE_SR)
		ackmask |= STATUS_RPS;

	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode & ~bits);

	for (i = 0; i < 1000; i++) {
		if (TULIP_ISSET(sc, CSR_STATUS, ackmask) == ackmask)
			break;
		delay(10);
	}

	csr = TULIP_READ(sc, CSR_STATUS);
	if ((csr & ackmask) != ackmask) {
		if ((bits & OPMODE_ST) != 0 && (csr & STATUS_TPS) == 0 &&
		    (csr & STATUS_TS) != STATUS_TS_STOPPED)
			printf("%s: transmit process failed to idle: "
			    "state %s\n", sc->sc_dev.dv_xname,
			    tx_state_names[(csr & STATUS_TS) >> 20]);
		if ((bits & OPMODE_SR) != 0 && (csr & STATUS_RPS) == 0 &&
		    (csr & STATUS_RS) != STATUS_RS_STOPPED)
			printf("%s: receive process failed to idle: "
			    "state %s\n", sc->sc_dev.dv_xname,
			    rx_state_names[(csr & STATUS_RS) >> 17]);
	}
	TULIP_WRITE(sc, CSR_STATUS, ackmask);
}

/*****************************************************************************
 * Generic media support functions.
 *****************************************************************************/

/*
 * tlp_mediastatus:	[ifmedia interface function]
 *
 *	Query the current media.
 */
void
tlp_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct tulip_softc *sc = ifp->if_softc;

	(*sc->sc_mediasw->tmsw_get)(sc, ifmr);
}

/*
 * tlp_mediachange:	[ifmedia interface function]
 *
 *	Update the current media.
 */
int
tlp_mediachange(ifp)
	struct ifnet *ifp;
{
	struct tulip_softc *sc = ifp->if_softc;

	return ((*sc->sc_mediasw->tmsw_set)(sc));
}

/*****************************************************************************
 * Support functions for MII-attached media.
 *****************************************************************************/

/*
 * tlp_mii_tick:
 *
 *	One second timer, used to tick the MII.
 */
void
tlp_mii_tick(arg)
	void *arg;
{
	struct tulip_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	timeout(sc->sc_tick, sc, hz);
}

/*
 * tlp_mii_statchg:	[mii interface function]
 *
 *	Callback from PHY when media changes.
 */
void
tlp_mii_statchg(self)
	struct device *self;
{
	struct tulip_softc *sc = (struct tulip_softc *)self;

	/* Idle the transmit and receive processes. */
	tlp_idle(sc, OPMODE_ST|OPMODE_SR);

	/*
	 * XXX What about Heartbeat Disable?  Is it magically frobbed
	 * XXX by the PHY?  I hope so...
	 */

	sc->sc_opmode &= ~(OPMODE_TTM|OPMODE_FD);

	if (IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_10_T)
		sc->sc_opmode |= OPMODE_TTM;

	if (sc->sc_mii.mii_media_active & IFM_FDX)
		sc->sc_opmode |= OPMODE_FD;

	/*
	 * Write new OPMODE bits.  This also restarts the transmit
	 * and receive processes.
	 */
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);

	/* XXX Update ifp->if_baudrate */
}

/*
 * tlp_winb_mii_statchg: [mii interface function]
 *
 *	Callback from PHY when media changes.  This version is
 *	for the Winbond 89C840F, which has different OPMODE bits.
 */
void
tlp_winb_mii_statchg(self)
	struct device *self;
{
	struct tulip_softc *sc = (struct tulip_softc *)self;

	/* Idle the transmit and receive processes. */
	tlp_idle(sc, OPMODE_ST|OPMODE_SR);

	/*
	 * XXX What about Heartbeat Disable?  Is it magically frobbed
	 * XXX by the PHY?  I hope so...
	 */

	sc->sc_opmode &= ~(OPMODE_WINB_FES|OPMODE_FD);

	if (IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_100_TX)
		sc->sc_opmode |= OPMODE_WINB_FES;

	if (sc->sc_mii.mii_media_active & IFM_FDX)
		sc->sc_opmode |= OPMODE_FD;

	/*
	 * Write new OPMODE bits.  This also restarts the transmit
	 * and receive processes.
	 */
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);

	/* XXX Update ifp->if_baudrate */
}

/*
 * tlp_mii_getmedia:
 *
 *	Callback from ifmedia to request current media status.
 */
void
tlp_mii_getmedia(sc, ifmr)
	struct tulip_softc *sc;
	struct ifmediareq *ifmr;
{

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

/*
 * tlp_mii_setmedia:
 *
 *	Callback from ifmedia to request new media setting.
 */
int
tlp_mii_setmedia(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return (0);
}

#define	MII_EMIT(sc, x)							\
do {									\
	TULIP_WRITE((sc), CSR_MIIROM, (x));				\
	delay(1);							\
} while (0)

/*
 * tlp_sio_mii_sync:
 *
 *	Synchronize the SIO-attached MII.
 */
void
tlp_sio_mii_sync(sc)
	struct tulip_softc *sc;
{
	u_int32_t miirom;
	int i;

	miirom = MIIROM_MDO;

	MII_EMIT(sc, miirom);
	for (i = 0; i < 32; i++) {
		MII_EMIT(sc, miirom | MIIROM_MDC);
		MII_EMIT(sc, miirom);
	}
}

/*
 * tlp_sio_mii_sendbits:
 *
 *	Send a series of bits out the SIO to the MII.
 */
void
tlp_sio_mii_sendbits(sc, data, nbits)
	struct tulip_softc *sc;
	u_int32_t data;
	int nbits;
{
	u_int32_t miirom, i;

	miirom = 0;
	MII_EMIT(sc, miirom);

	for (i = 1 << (nbits - 1); i != 0; i >>= 1) {
		if (data & i)
			miirom |= MIIROM_MDO;
		else
			miirom &= ~MIIROM_MDO;
		MII_EMIT(sc, miirom);
		MII_EMIT(sc, miirom|MIIROM_MDC);
		MII_EMIT(sc, miirom);
	}
}

/*
 * tlp_sio_mii_readreg:
 *
 *	Read a PHY register via SIO-attached MII.
 */
int
tlp_sio_mii_readreg(self, phy, reg)
	struct device *self;
	int phy, reg;
{
	struct tulip_softc *sc = (void *) self;
	int val = 0, err = 0, i;

	tlp_sio_mii_sync(sc);

	tlp_sio_mii_sendbits(sc, MII_COMMAND_START, 2);
	tlp_sio_mii_sendbits(sc, MII_COMMAND_READ, 2);
	tlp_sio_mii_sendbits(sc, phy, 5);
	tlp_sio_mii_sendbits(sc, reg, 5);

	/* Switch direction to PHY->host, without a clock transition. */
	MII_EMIT(sc, MIIROM_MIIDIR);

	MII_EMIT(sc, MIIROM_MIIDIR|MIIROM_MDC);
	MII_EMIT(sc, MIIROM_MIIDIR);

	err = TULIP_ISSET(sc, CSR_MIIROM, MIIROM_MDI);

	MII_EMIT(sc, MIIROM_MIIDIR|MIIROM_MDC);
	MII_EMIT(sc, MIIROM_MIIDIR);

	for (i = 0; i < 16; i++) {
		val <<= 1;
		/* Read data prior to clock low-high transition. */
		if (err == 0 && TULIP_ISSET(sc, CSR_MIIROM, MIIROM_MDI))
			val |= 1;

		MII_EMIT(sc, MIIROM_MIIDIR|MIIROM_MDC);
		MII_EMIT(sc, MIIROM_MIIDIR);
	}

	/* Set direction to host->PHY, without a clock transition. */
	MII_EMIT(sc, 0);

	return (err ? 0 : val);
}

/*
 * tlp_sio_mii_writereg:
 *
 *	Write a PHY register via SIO-attached MII.
 */
void
tlp_sio_mii_writereg(self, phy, reg, val)
	struct device *self;
	int phy, reg, val;
{
	struct tulip_softc *sc = (void *) self;

	tlp_sio_mii_sync(sc);

	tlp_sio_mii_sendbits(sc, MII_COMMAND_START, 2);
	tlp_sio_mii_sendbits(sc, MII_COMMAND_WRITE, 2);
	tlp_sio_mii_sendbits(sc, phy, 5);
	tlp_sio_mii_sendbits(sc, reg, 5);
	tlp_sio_mii_sendbits(sc, MII_COMMAND_ACK, 2);
	tlp_sio_mii_sendbits(sc, val, 16);

	MII_EMIT(sc, 0);
}

#undef MII_EMIT

/*
 * tlp_pnic_mii_readreg:
 *
 *	Read a PHY register on the Lite-On PNIC.
 */
int
tlp_pnic_mii_readreg(self, phy, reg)
	struct device *self;
	int phy, reg;
{
	struct tulip_softc *sc = (void *) self;
	u_int32_t val;
	int i;

	TULIP_WRITE(sc, CSR_PNIC_MII,
	    PNIC_MII_MBO | PNIC_MII_RESERVED |
	    PNIC_MII_READ | (phy << PNIC_MII_PHYSHIFT) |
	    (reg << PNIC_MII_REGSHIFT));

	for (i = 0; i < 1000; i++) {
		delay(10);
		val = TULIP_READ(sc, CSR_PNIC_MII);
		if ((val & PNIC_MII_BUSY) == 0) {
			if ((val & PNIC_MII_DATA) == PNIC_MII_DATA)
				return (0);
			else
				return (val & PNIC_MII_DATA);
		}
	}
	printf("%s: MII read timed out\n", sc->sc_dev.dv_xname);
	return (0);
}

/*
 * tlp_pnic_mii_writereg:
 *
 *	Write a PHY register on the Lite-On PNIC.
 */
void
tlp_pnic_mii_writereg(self, phy, reg, val)
	struct device *self;
	int phy, reg, val;
{
	struct tulip_softc *sc = (void *) self;
	int i;

	TULIP_WRITE(sc, CSR_PNIC_MII,
	    PNIC_MII_MBO | PNIC_MII_RESERVED |
	    PNIC_MII_WRITE | (phy << PNIC_MII_PHYSHIFT) |
	    (reg << PNIC_MII_REGSHIFT) | val);

	for (i = 0; i < 1000; i++) {
		delay(10);
		if (TULIP_ISSET(sc, CSR_PNIC_MII, PNIC_MII_BUSY) == 0)
			return;
	}
	printf("%s: MII write timed out\n", sc->sc_dev.dv_xname);
}

/*****************************************************************************
 * Chip-specific pre-init and reset functions.
 *****************************************************************************/

/*
 * tlp_2114x_preinit:
 *
 *	Pre-init function shared by DECchip 21140, 21140A, 21142, and 21143.
 */
void
tlp_2114x_preinit(sc)
	struct tulip_softc *sc;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_2114x_media *tm = ife->ifm_aux;

	/*
	 * Always set the Must-Be-One bit.
	 */
	sc->sc_opmode |= OPMODE_MBO;

	if (tm != NULL &&
	    (tm->tm_type == TULIP_ROM_MB_21140_MII ||
	     tm->tm_type == TULIP_ROM_MB_21142_MII)) {
		/*
		 * MII case: just set the port-select bit; we will never
		 * be called during a media change.
		 */
		sc->sc_opmode |= OPMODE_PS;
		goto set_opmode;
	}

	/*
	 * ENDEC/PCS mode; set according to selected media type.
	 * XXX Auto-sense not supported yet.
	 */
	sc->sc_opmode |= tm->tm_opmode;

 set_opmode:
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);
}

/*
 * tlp_pnic_preinit:
 *
 *	Pre-init function for the Lite-On 82c168 and 82c169.
 */
void
tlp_pnic_preinit(sc)
	struct tulip_softc *sc;
{

	if (sc->sc_flags & TULIPF_HAS_MII) {
		/*
		 * MII case: just set the port-select bit; we will never
		 * be called during a media change.
		 */
		sc->sc_opmode |= OPMODE_PS;
	} else {
		/*
		 * ENDEC/PCS/Nway mode; enable the Tx backoff counter.
		 */
		sc->sc_opmode |= OPMODE_PNIC_TBEN;
	}
}

/*
 * tlp_21140_reset:
 *
 *	Issue a reset sequence on the 21140 via the GPIO facility.
 */
void
tlp_21140_reset(sc)
	struct tulip_softc *sc;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_2114x_media *tm = ife->ifm_aux;
	int i;

	/* First, set the direction on the GPIO pins. */
	TULIP_WRITE(sc, CSR_GPP, GPP_GPC|sc->sc_gp_dir);

	/* Now, issue the reset sequence. */
	for (i = 0; i < tm->tm_reset_length; i++) {
		delay(10);
		TULIP_WRITE(sc, CSR_GPP, sc->sc_srom[tm->tm_reset_offset + i]);
	}

	/* Now, issue the selection sequence. */
	for (i = 0; i < tm->tm_gp_length; i++) {
		delay(10);
		TULIP_WRITE(sc, CSR_GPP, sc->sc_srom[tm->tm_gp_offset + i]);
	}

	/* If there were no sequences, just lower the pins. */
	if (tm->tm_reset_length == 0 && tm->tm_gp_length == 0)
		TULIP_WRITE(sc, CSR_GPP, 0);
}

/*****************************************************************************
 * Chip/board-specific media switches.  The ones here are ones that
 * are potentially common to multiple front-ends.
 *****************************************************************************/

const struct tulip_srom_to_ifmedia tulip_srom_to_ifmedia_table[] = {
	{ TULIP_ROM_MB_MEDIA_TP,	IFM_10_T,	0,
	  "10baseT",			SIACONN_21041_10BASET,
	  SIATXRX_21041_10BASET,	SIAGEN_21041_10BASET },

	{ TULIP_ROM_MB_MEDIA_BNC,	IFM_10_2,	0,
	  "10base2",			SIACONN_21041_BNC,
	  SIATXRX_21041_BNC,		SIAGEN_21041_BNC },

	{ TULIP_ROM_MB_MEDIA_AUI,	IFM_10_5,	0,
	  "10base5",			SIACONN_21041_AUI,
	  SIATXRX_21041_AUI,		SIAGEN_21041_AUI },

	{ TULIP_ROM_MB_MEDIA_100TX,	IFM_100_TX,	0,
	  "100baseTX",			0,
	  0,				0 },

	{ TULIP_ROM_MB_MEDIA_TP_FDX,	IFM_10_T,	IFM_FDX,
	  "10baseT-FDX",		SIACONN_21041_10BASET_FDX,
	  SIATXRX_21041_10BASET_FDX,	SIAGEN_21041_10BASET_FDX },

	{ TULIP_ROM_MB_MEDIA_100TX_FDX,	IFM_100_TX,	IFM_FDX,
	  "100baseTX-FDX",		0,
	  0,				0 },

	{ TULIP_ROM_MB_MEDIA_100T4,	IFM_100_T4,	0,
	  "100baseT4",			0,
	  0,				0 },

	{ TULIP_ROM_MB_MEDIA_100FX,	IFM_100_FX,	0,
	  "100baseFX",			0,
	  0,				0 },

	{ TULIP_ROM_MB_MEDIA_100FX_FDX,	IFM_100_FX,	IFM_FDX,
	  "100baseFX-FDX",		0,
	  0,				0 },

	{ 0,				0,		0,
	  NULL,				0,
	  0,				0 },
};

const struct tulip_srom_to_ifmedia *tulip_srom_to_ifmedia __P((u_int8_t));

const struct tulip_srom_to_ifmedia *
tulip_srom_to_ifmedia(sm)
	u_int8_t sm;
{
	const struct tulip_srom_to_ifmedia *tsti;

	for (tsti = tulip_srom_to_ifmedia_table;
	     tsti->tsti_name != NULL; tsti++) {
		if (tsti->tsti_srom == sm)
			return (tsti);
	}

	return (NULL);
}

/*
 * 21040 and 21041 media switches.
 */
void	tlp_21040_tmsw_init __P((struct tulip_softc *));
void	tlp_21040_tp_tmsw_init __P((struct tulip_softc *));
void	tlp_21040_auibnc_tmsw_init __P((struct tulip_softc *));
void	tlp_21041_tmsw_init __P((struct tulip_softc *));
void	tlp_21040_21041_tmsw_get __P((struct tulip_softc *,
	    struct ifmediareq *));
int	tlp_21040_21041_tmsw_set __P((struct tulip_softc *));

const struct tulip_mediasw tlp_21040_mediasw = {
	tlp_21040_tmsw_init, tlp_21040_21041_tmsw_get, tlp_21040_21041_tmsw_set
};

const struct tulip_mediasw tlp_21040_tp_mediasw = {
	tlp_21040_tp_tmsw_init, tlp_21040_21041_tmsw_get,
	    tlp_21040_21041_tmsw_set
};

const struct tulip_mediasw tlp_21040_auibnc_mediasw = {
	tlp_21040_auibnc_tmsw_init, tlp_21040_21041_tmsw_get,
	    tlp_21040_21041_tmsw_set
};

const struct tulip_mediasw tlp_21041_mediasw = {
	tlp_21041_tmsw_init, tlp_21040_21041_tmsw_get, tlp_21040_21041_tmsw_set
};

#define	ADD(m, t)	ifmedia_add(&sc->sc_mii.mii_media, (m), 0, (t))
#define	PRINT(s)	printf("%s%s", sep, s); sep = ", "

void
tlp_21040_tmsw_init(sc)
	struct tulip_softc *sc;
{
	struct tulip_21040_21041_sia_media *tsm;
	const char *sep = "";

	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);

	printf("%s: ", sc->sc_dev.dv_xname);

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_10BASET;
	tsm->tsm_siatxrx = SIATXRX_21040_10BASET;
	tsm->tsm_siagen  = SIAGEN_21040_10BASET;
	ADD(IFM_ETHER|IFM_10_T, tsm);
	PRINT("10baseT");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_10BASET_FDX;
	tsm->tsm_siatxrx = SIATXRX_21040_10BASET_FDX;
	tsm->tsm_siagen  = SIAGEN_21040_10BASET_FDX;
	ADD(IFM_ETHER|IFM_10_T|IFM_FDX, tsm);
	PRINT("10baseT-FDX");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_AUI;
	tsm->tsm_siatxrx = SIATXRX_21040_AUI;
	tsm->tsm_siagen  = SIAGEN_21040_AUI;
	ADD(IFM_ETHER|IFM_10_5, tsm);
	PRINT("10base5");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_EXTSIA;
	tsm->tsm_siatxrx = SIATXRX_21040_EXTSIA;
	tsm->tsm_siagen  = SIAGEN_21040_EXTSIA;
	ADD(IFM_ETHER|IFM_MANUAL, tsm);
	PRINT("manual");

	/*
	 * XXX Autosense not yet supported.
	 */

	/* XXX This should be auto-sense. */
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_10_T);
	printf(", default 10baseT");

	printf("\n");
}

void
tlp_21040_tp_tmsw_init(sc)
	struct tulip_softc *sc;
{
	struct tulip_21040_21041_sia_media *tsm;
	const char *sep = "";

	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);

	printf("%s: ", sc->sc_dev.dv_xname);

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_10BASET;
	tsm->tsm_siatxrx = SIATXRX_21040_10BASET;
	tsm->tsm_siagen  = SIAGEN_21040_10BASET;
	ADD(IFM_ETHER|IFM_10_T, tsm);
	PRINT("10baseT");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_10BASET_FDX;
	tsm->tsm_siatxrx = SIATXRX_21040_10BASET_FDX;
	tsm->tsm_siagen  = SIAGEN_21040_10BASET_FDX;
	ADD(IFM_ETHER|IFM_10_T|IFM_FDX, tsm);
	PRINT("10baseT-FDX");

	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_10_T);
	printf(", default 10baseT");

	printf("\n");
}

void
tlp_21040_auibnc_tmsw_init(sc)
	struct tulip_softc *sc;
{
	struct tulip_21040_21041_sia_media *tsm;
	const char *sep = "";

	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);

	printf("%s: ", sc->sc_dev.dv_xname);

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21040_AUI;
	tsm->tsm_siatxrx = SIATXRX_21040_AUI;
	tsm->tsm_siagen  = SIAGEN_21040_AUI;
	ADD(IFM_ETHER|IFM_10_5, tsm);
	PRINT("10base5");

	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_10_5);

	printf("\n");
}

void
tlp_21041_tmsw_init(sc)
	struct tulip_softc *sc;
{
	int i, defmedia, devcnt, leaf_offset, mb_offset, m_cnt;
	const struct tulip_srom_to_ifmedia *tsti;
	struct tulip_21040_21041_sia_media *tsm;
	const char *sep = "", *defstr;
	u_int16_t romdef;
	u_int8_t mb;

	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);

	printf("%s: ", sc->sc_dev.dv_xname);

	if (tlp_isv_srom(sc->sc_srom) == 0)
		goto not_isv_srom;

	devcnt = sc->sc_srom[TULIP_ROM_CHIP_COUNT];
	for (i = 0; i < devcnt; i++) {
		if (sc->sc_srom[TULIP_ROM_CHIP_COUNT] == 1)
			break;
		if (sc->sc_srom[TULIP_ROM_CHIPn_DEVICE_NUMBER(i)] ==
		    sc->sc_devno)
			break;
	}

	if (i == devcnt)
		goto not_isv_srom;

	leaf_offset = TULIP_ROM_GETW(sc->sc_srom,
	    TULIP_ROM_CHIPn_INFO_LEAF_OFFSET(i));
	mb_offset = leaf_offset + TULIP_ROM_IL_MEDIAn_BLOCK_BASE;
	m_cnt = sc->sc_srom[leaf_offset + TULIP_ROM_IL_MEDIA_COUNT];

	for (; m_cnt != 0;
	     m_cnt--, mb_offset += TULIP_ROM_MB_SIZE(mb)) {
		mb = sc->sc_srom[mb_offset];
		tsm = malloc(sizeof(struct tulip_21040_21041_sia_media),
		    M_DEVBUF, M_WAITOK);
		switch (mb & TULIP_ROM_MB_MEDIA_CODE) {
		case TULIP_ROM_MB_MEDIA_TP:
		case TULIP_ROM_MB_MEDIA_BNC:
		case TULIP_ROM_MB_MEDIA_AUI:
		case TULIP_ROM_MB_MEDIA_TP_FDX:
			tsti = tulip_srom_to_ifmedia(mb &
			    TULIP_ROM_MB_MEDIA_CODE);

			tsm->tsm_siaconn = (mb & TULIP_ROM_MB_EXT) ?
			    TULIP_ROM_GETW(sc->sc_srom,
			      mb_offset + TULIP_ROM_MB_CSR13) :
			    tsti->tsti_21041_siaconn;
			tsm->tsm_siatxrx = (mb & TULIP_ROM_MB_EXT) ?
			    TULIP_ROM_GETW(sc->sc_srom,
			      mb_offset + TULIP_ROM_MB_CSR14) :
			    tsti->tsti_21041_siatxrx;
			tsm->tsm_siagen  = (mb & TULIP_ROM_MB_EXT) ?
			    TULIP_ROM_GETW(sc->sc_srom,
			      mb_offset + TULIP_ROM_MB_CSR15) :
			    tsti->tsti_21041_siagen;

			ifmedia_add(&sc->sc_mii.mii_media,
			    IFM_MAKEWORD(IFM_ETHER, tsti->tsti_subtype,
			    tsti->tsti_options, 0), 0, tsm);
			PRINT(tsti->tsti_name);
			break;

		default:
			printf("%s<unknown 0x%02x>", sep,
			    (mb & TULIP_ROM_MB_MEDIA_CODE));
			sep = ", ";
			free(tsm, M_DEVBUF);
		}
	}

	/*
	 * XXX Autosense not yet supported.
	 */

	romdef = TULIP_ROM_GETW(sc->sc_srom, leaf_offset +
	    TULIP_ROM_IL_SELECT_CONN_TYPE);
	switch (romdef) {
	case SELECT_CONN_TYPE_TP:
	case SELECT_CONN_TYPE_TP_AUTONEG:
	case SELECT_CONN_TYPE_TP_NOLINKPASS:
		defmedia = IFM_ETHER|IFM_10_T;
		defstr = "10baseT";
		break;

	case SELECT_CONN_TYPE_TP_FDX:
		defmedia = IFM_ETHER|IFM_10_T|IFM_FDX;
		defstr = "10baseT-FDX";
		break;

	case SELECT_CONN_TYPE_BNC:
		defmedia = IFM_ETHER|IFM_10_2;
		defstr = "10base2";
		break;

	case SELECT_CONN_TYPE_AUI:
		defmedia = IFM_ETHER|IFM_10_5;
		defstr = "10base5";
		break;
#if 0 /* XXX */
	case SELECT_CONN_TYPE_ASENSE:
	case SELECT_CONN_TYPE_ASENSE_AUTONEG:
		defmedia = IFM_ETHER|IFM_AUTO;
		defstr = "auto";
		break;
#endif
	default:
		defmedia = 0;
		defstr = NULL;
	}

	if (defmedia != 0)
		printf(", default %s\n", defstr);
	else {
		/*
		 * XXX We should default to auto-sense.
		 */
		defmedia = IFM_ETHER|IFM_10_T;
		defstr = "10baseT";

		printf("\n%s: unknown default media in SROM (0x%04x), "
		    "using %s\n", sc->sc_dev.dv_xname, romdef, defstr);
	}

	ifmedia_set(&sc->sc_mii.mii_media, defmedia);
	return;

 not_isv_srom:
	/*
	 * If we have a board without the standard 21041 SROM format,
	 * we just assume all media are present and try and pick a
	 * reasonable default.
	 */
	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21041_10BASET;
	tsm->tsm_siatxrx = SIATXRX_21041_10BASET;
	tsm->tsm_siagen  = SIAGEN_21041_10BASET;
	ADD(IFM_ETHER|IFM_10_T, tsm);
	PRINT("10baseT");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21041_10BASET_FDX;
	tsm->tsm_siatxrx = SIATXRX_21041_10BASET_FDX;
	tsm->tsm_siagen  = SIAGEN_21041_10BASET_FDX;
	ADD(IFM_ETHER|IFM_10_T|IFM_FDX, tsm);
	PRINT("10baseT-FDX");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21041_BNC;
	tsm->tsm_siatxrx = SIATXRX_21041_BNC;
	tsm->tsm_siagen  = SIAGEN_21041_BNC;
	ADD(IFM_ETHER|IFM_10_2|IFM_FDX, tsm);
	PRINT("10base2");

	tsm = malloc(sizeof(struct tulip_21040_21041_sia_media), M_DEVBUF,
	    M_WAITOK);
	tsm->tsm_siaconn = SIACONN_21041_AUI;
	tsm->tsm_siatxrx = SIATXRX_21041_AUI;
	tsm->tsm_siagen  = SIAGEN_21041_AUI;
	ADD(IFM_ETHER|IFM_10_5|IFM_FDX, tsm);
	PRINT("10base5");

	/*
	 * XXX Autosense not yet supported.
	 */

	/* XXX This should be auto-sense. */
	ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_10_T);
	printf(", default 10baseT");

	printf("\n");
}

#undef ADD
#undef PRINT

void
tlp_21040_21041_tmsw_get(sc, ifmr)
	struct tulip_softc *sc;
	struct ifmediareq *ifmr;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;

	ifmr->ifm_status = 0;

	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
		/*
		 * XXX Implement autosensing case.
		 */
		break;

	case IFM_10_T:
		/*
		 * We're able to detect link directly on twisted pair.
		 */
		ifmr->ifm_status = IFM_AVALID;
		if (TULIP_ISSET(sc, CSR_SIASTAT, SIASTAT_LKF) == 0)
			ifmr->ifm_status |= IFM_ACTIVE;
		/* FALLTHROUGH */
	default:
		/*
		 * If not autosensing, active media is the currently
		 * selected media.
		 */
		ifmr->ifm_active = ife->ifm_media;
	}
}

int
tlp_21040_21041_tmsw_set(sc)
	struct tulip_softc *sc;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_21040_21041_sia_media *tsm;

	if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO) {
		/*
		 * If not autosensing, just pull the SIA settings out
		 * of the media entry.
		 */
		tsm = ife->ifm_aux;
		TULIP_WRITE(sc, CSR_SIACONN, SIACONN_SRL);
		TULIP_WRITE(sc, CSR_SIATXRX, tsm->tsm_siatxrx);
		TULIP_WRITE(sc, CSR_SIAGEN,  tsm->tsm_siagen);
		TULIP_WRITE(sc, CSR_SIACONN, tsm->tsm_siaconn);

		tlp_idle(sc, OPMODE_ST|OPMODE_SR);
		sc->sc_opmode &= ~OPMODE_FD;
		if (ife->ifm_media & IFM_FDX)
			sc->sc_opmode |= OPMODE_FD;
		TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);
	} else {
		/*
		 * XXX Implement autosensing case.
		 */
	}

	return (0);
}

/*
 * DECchip 2114x ISV media switch.
 * XXX Currently only handles 21140[A] GPR and MII.
 */
void	tlp_2114x_isv_tmsw_init __P((struct tulip_softc *));
void	tlp_2114x_isv_tmsw_get __P((struct tulip_softc *, struct ifmediareq *));
int	tlp_2114x_isv_tmsw_set __P((struct tulip_softc *));

const struct tulip_mediasw tlp_2114x_isv_mediasw = {
	tlp_2114x_isv_tmsw_init, tlp_2114x_isv_tmsw_get, tlp_2114x_isv_tmsw_set
};

void	tlp_21140_gpr_getmedia __P((struct tulip_softc *sc,
	    struct ifmediareq *ifmr));
int	tlp_21140_gpr_setmedia __P((struct tulip_softc *sc));

void
tlp_2114x_isv_tmsw_init(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ifmedia_entry *ife;
	struct mii_softc *phy;
	struct tulip_2114x_media *tm;
	const struct tulip_srom_to_ifmedia *tsti;
	int i, devcnt, leaf_offset, m_cnt, type, length, seen, defmedia, minst;
	u_int16_t word;
	u_int8_t *cp, *ncp;

	seen = defmedia = 0;

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = tlp_sio_mii_readreg;
	sc->sc_mii.mii_writereg = tlp_sio_mii_writereg;
	sc->sc_mii.mii_statchg = sc->sc_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);

	devcnt = sc->sc_srom[TULIP_ROM_CHIP_COUNT];
	for (i = 0; i < devcnt; i++) {
		if (sc->sc_srom[TULIP_ROM_CHIP_COUNT] == 1)
			break;
		if (sc->sc_srom[TULIP_ROM_CHIPn_DEVICE_NUMBER(i)] ==
		    sc->sc_devno)
			break;
	}

	if (i == devcnt) {
		printf("%s: unable to locate info leaf in SROM\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	leaf_offset = TULIP_ROM_GETW(sc->sc_srom,
	    TULIP_ROM_CHIPn_INFO_LEAF_OFFSET(i));

	/* XXX SELECT CONN TYPE */

	cp = &sc->sc_srom[leaf_offset + TULIP_ROM_IL_MEDIA_COUNT];

	/*
	 * On some chips, the first thing in the Info Leaf is the
	 * GPIO pin direction data.
	 */
	switch (sc->sc_chip) {
	case TULIP_CHIP_21140:
	case TULIP_CHIP_21140A:
	case TULIP_CHIP_MX98713:
	case TULIP_CHIP_MX98713A:
	case TULIP_CHIP_AX88140:
	case TULIP_CHIP_AX88141:
		sc->sc_gp_dir = *cp++;
		break;

	default:
		/* Nothing. */
	}

	/* Get the media count. */
	m_cnt = *cp++;

	for (; m_cnt != 0; cp = ncp, m_cnt--) {
		/*
		 * Determine the type and length of this media block.
		 */
		if ((*cp & 0x80) == 0) {
			length = 4;
			type = TULIP_ROM_MB_21140_GPR;
		} else {
			length = (*cp++ & 0x7f) - 1;
			type = *cp++ & 0x3f;
		}

		/* Compute the start of the next block. */
		ncp = cp + length;

		/* Now, parse the block. */
		switch (type) {
		case TULIP_ROM_MB_21140_GPR:
			seen |= 1 << TULIP_ROM_MB_21140_GPR;

			tm = malloc(sizeof(*tm), M_DEVBUF, M_WAITOK);
			memset(tm, 0, sizeof(*tm));

			tm->tm_type = TULIP_ROM_MB_21140_GPR;
			tm->tm_get = tlp_21140_gpr_getmedia;
			tm->tm_set = tlp_21140_gpr_setmedia;

			minst = 0;	/* XXX compute new instance */

			/* First is the media type code. */
			tsti = tulip_srom_to_ifmedia(cp[0] &
			    TULIP_ROM_MB_MEDIA_CODE);
			if (tsti == NULL) {
				/* Invalid media code. */
				free(tm, M_DEVBUF);
				break;
			}
			tm->tm_name = tsti->tsti_name;

			/* Next is any GPIO info for this media. */
			tm->tm_gpdata = cp[1];

			/*
			 * Next is a word containing OPMODE information
			 * and info on how to detect if this media is
			 * active.
			 */
			word = TULIP_ROM_GETW(cp, 2);
			tm->tm_opmode = TULIP_ROM_MB_OPMODE(word);
			if ((word & TULIP_ROM_MB_NOINDICATOR) == 0) {
				tm->tm_actmask =
				    TULIP_ROM_MB_BITPOS(word);
				tm->tm_actdata =
				    (word & TULIP_ROM_MB_POLARITY) ?
				    0 : tm->tm_actmask;
			}

			/*
			 * Now, add the media to our list.  We will
			 * print them out later.
			 */
			ifmedia_add(&sc->sc_mii.mii_media,
			    IFM_MAKEWORD(IFM_ETHER, tsti->tsti_subtype,
			    tsti->tsti_options, minst), 0, tm);
			break;

		case TULIP_ROM_MB_21140_MII:
			seen |= 1 << TULIP_ROM_MB_21140_MII;

			tm = malloc(sizeof(*tm), M_DEVBUF, M_WAITOK);
			memset(tm, 0, sizeof(*tm));

			tm->tm_type = TULIP_ROM_MB_21140_MII;
			tm->tm_get = tlp_mii_getmedia;
			tm->tm_set = tlp_mii_setmedia;

			if (sc->sc_reset == NULL)
				sc->sc_reset = tlp_21140_reset;

			/* First is the PHY number. */
			tm->tm_phyno = *cp++;

			/* Next is the MII select sequence length and offset. */
			tm->tm_gp_length = *cp++;
			tm->tm_gp_offset = cp - &sc->sc_srom[0];
			cp += tm->tm_gp_length;

			/* Next is the MII reset sequence length and offset. */
			tm->tm_reset_length = *cp++;
			tm->tm_reset_offset = cp - &sc->sc_srom[0];
			cp += tm->tm_reset_length;

			/*
			 * The following items are left in the media block
			 * that we don't particularly care about:
			 *
			 *	capabilities		W
			 *	advertisement		W
			 *	full duplex		W
			 *	tx threshold		W
			 *
			 * These appear to be bits in the PHY registers,
			 * which our MII code handles on its own.
			 */

			/*
			 * Before we probe the MII bus, we need to reset
			 * it and issue the selection sequence.
			 */

			/* Set the direction of the pins... */
			TULIP_WRITE(sc, CSR_GPP, GPP_GPC|sc->sc_gp_dir);

			for (i = 0; i < tm->tm_reset_length; i++) {
				delay(10);
				TULIP_WRITE(sc, CSR_GPP,
				    sc->sc_srom[tm->tm_reset_offset + i]);
			}

			for (i = 0; i < tm->tm_gp_length; i++) {
				delay(10);
				TULIP_WRITE(sc, CSR_GPP,
				    sc->sc_srom[tm->tm_gp_offset + i]);
			}

			/* If there were no sequences, just lower the pins. */
			if (tm->tm_reset_length == 0 && tm->tm_gp_length == 0) {
				delay(10);
				TULIP_WRITE(sc, CSR_GPP, 0);
			}

			/*
			 * Now, probe the MII for the PHY.  Note, we know
			 * the location of the PHY on the bus, but we don't
			 * particularly care; the MII code just likes to
			 * search the whole thing anyhow.
			 */
			mii_phy_probe(&sc->sc_dev, &sc->sc_mii, 0xffffffff);

			/*
			 * Now, search for the PHY we hopefully just
			 * configured.  If it's not configured into the
			 * kernel, we lose.  The PHY's default media always
			 * takes priority.
			 */
			for (phy = LIST_FIRST(&sc->sc_mii.mii_phys);
			     phy != NULL;
			     phy = LIST_NEXT(phy, mii_list))
				if (phy->mii_offset == tm->tm_phyno)
					break;
			if (phy == NULL) {
				printf("%s: unable to configure MII\n",
				    sc->sc_dev.dv_xname);
				break;
			}

			sc->sc_flags |= TULIPF_HAS_MII;
			sc->sc_tick = tlp_mii_tick;
			defmedia = IFM_MAKEWORD(IFM_ETHER, IFM_AUTO, 0,
			    phy->mii_inst);

			/*
			 * Okay, now that we've found the PHY and the MII
			 * layer has added all of the media associated
			 * with that PHY, we need to traverse the media
			 * list, and add our `tm' to each entry's `aux'
			 * pointer.
			 *
			 * We do this by looking for media with our
			 * PHY's `instance'.
			 */
			for (ife = LIST_FIRST(&sc->sc_mii.mii_media.ifm_list);
			     ife != NULL;
			     ife = LIST_NEXT(ife, ifm_list)) {
				if (IFM_INST(ife->ifm_media) != phy->mii_inst)
					continue;
				ife->ifm_aux = tm;
			}
			break;

		case TULIP_ROM_MB_21142_SIA:
			printf("%s: 21142 SIA block\n", sc->sc_dev.dv_xname);
			break;

		case TULIP_ROM_MB_21142_MII:
			printf("%s: 21142 MII block\n", sc->sc_dev.dv_xname);
			break;

		case TULIP_ROM_MB_21143_SYM:
			printf("%s: 21143 SYM block\n", sc->sc_dev.dv_xname);
			break;

		case TULIP_ROM_MB_21143_RESET:
			printf("%s: 21143 reset block\n", sc->sc_dev.dv_xname);
			break;

		default:
			printf("%s: unknown ISV media block type 0x%02x\n",
			    sc->sc_dev.dv_xname, type);
		}
	}

	/*
	 * Deal with the case where no media is configured.
	 */
	if (LIST_FIRST(&sc->sc_mii.mii_media.ifm_list) == NULL) {
		printf("%s: no media found!\n", sc->sc_dev.dv_xname);
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		defmedia = IFM_ETHER|IFM_NONE;
		goto set_default;
	}

#define	PRINT(s)	printf("%s%s", sep, s); sep = ", "

	/*
	 * Display any non-MII media we've located.
	 */
	if (seen & (1 << TULIP_ROM_MB_21140_GPR)) {
		const char *sep = "";
		printf("%s: GPR media: ", sc->sc_dev.dv_xname);
		for (ife = LIST_FIRST(&sc->sc_mii.mii_media.ifm_list);
		     ife != NULL;
		     ife = LIST_NEXT(ife, ifm_list)) {
			minst = IFM_INST(ife->ifm_media);
			tm = ife->ifm_aux;
			if (tm->tm_type != TULIP_ROM_MB_21140_GPR)
				continue;
			PRINT(tm->tm_name);
		}

		/*
		 * XXX Pick a better default.  Should come
		 * XXX from SROM on 21140[A], and should
		 * XXX be "auto" on Macronix chips (which
		 * XXX have an internal NWay block).
		 */
		if (defmedia == 0) {
			defmedia = IFM_MAKEWORD(IFM_ETHER, IFM_10_T, 0,
			    minst);
			printf(", default 10baseT");
		}
		printf("\n");
	}

	if (seen & (1 << TULIP_ROM_MB_21142_SIA)) {
		printf("%s: SIA media: ", sc->sc_dev.dv_xname);
		/* XXX */
		printf("\n");
	}

	if (seen & (1 << TULIP_ROM_MB_21143_SYM)) {
		printf("%s: SYM media: ", sc->sc_dev.dv_xname);
		/* XXX */
		printf("\n");
	}

	/*
	 * XXX Display default media if not MII.
	 */

#undef PRINT

 set_default:
	/*
	 * Set the default media.
	 *
	 * XXX Should make some attempt to care about the SROM default
	 * setting, but we don't.
	 */
#ifdef DIAGNOSTIC
	if (defmedia == 0)
		panic("tlp_2114x_isv_tmsw_init: no default media");
#endif
	ifmedia_set(&sc->sc_mii.mii_media, defmedia);
}

void
tlp_2114x_isv_tmsw_get(sc, ifmr)
	struct tulip_softc *sc;
	struct ifmediareq *ifmr;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_2114x_media *tm = ife->ifm_aux;

	(*tm->tm_get)(sc, ifmr);
}

int
tlp_2114x_isv_tmsw_set(sc)
	struct tulip_softc *sc;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_2114x_media *tm = ife->ifm_aux;

	return ((*tm->tm_set)(sc));
}

void
tlp_21140_gpr_getmedia(sc, ifmr)
	struct tulip_softc *sc;
	struct ifmediareq *ifmr;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_2114x_media *tm = ife->ifm_aux;

	ifmr->ifm_status = 0;

	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
		/*
		 * XXX Implement autosensing case.
		 */
		break;

	default:
		/*
		 * If not autosensing, active media is the currently
		 * selected media.
		 */
		ifmr->ifm_active = ife->ifm_media;

		/*
		 * If we can sense the active status of the link,
		 * so do.
		 */
		if (tm->tm_actmask != 0) {
			ifmr->ifm_status |= IFM_AVALID;
			if (TULIP_ISSET(sc, CSR_GPP, tm->tm_actmask) ==
			    tm->tm_actdata)
				ifmr->ifm_status |= IFM_ACTIVE;
		}
	}
}

int
tlp_21140_gpr_setmedia(sc)
	struct tulip_softc *sc;
{
	struct ifmedia_entry *ife = sc->sc_mii.mii_media.ifm_cur;
	struct tulip_2114x_media *tm = ife->ifm_aux;

	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
		/*
		 * XXX Implement autosensing case.
		 */
		break;

	default:
		/*
		 * The ifmedia entry contains the OPMODE bits necessary
		 * to enable this media type.  It may be necessary to
		 * perform a reset of the chip; see tlp_21140_reset().
		 */
		if ((tm->tm_opmode & OPMODE_MEDIA_BITS) !=
		    (sc->sc_opmode & OPMODE_MEDIA_BITS)) {
			/*
			 * We have to reset the chip.  Note that we
			 * won't recurse into this path again as
			 * the OPMODE bits will be correct this
			 * next time through.
			 */
			return (tlp_init(sc));
		}

		/*
		 * Set new OPMODE bits and write the OPMODE register.
		 */
		tlp_idle(sc, OPMODE_ST|OPMODE_SR);
		sc->sc_opmode = (sc->sc_opmode & ~OPMODE_MEDIA_BITS) |
		    tm->tm_opmode;
		TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);

		/*
		 * Set the GPIO pins for this media, to flip any
		 * relays, etc.
		 */
		TULIP_WRITE(sc, CSR_GPP, GPP_GPC|sc->sc_gp_dir);
		delay(10);
		TULIP_WRITE(sc, CSR_GPP, tm->tm_gpdata);
		break;
	}

	return (0);
}

/*
 * MII-on-SIO media switch.  Handles only MII attached to the SIO.
 */
void	tlp_sio_mii_tmsw_init __P((struct tulip_softc *));

const struct tulip_mediasw tlp_sio_mii_mediasw = {
	tlp_sio_mii_tmsw_init, tlp_mii_getmedia, tlp_mii_setmedia
};

void
tlp_sio_mii_tmsw_init(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = tlp_sio_mii_readreg;
	sc->sc_mii.mii_writereg = tlp_sio_mii_writereg;
	sc->sc_mii.mii_statchg = sc->sc_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);
	mii_phy_probe(&sc->sc_dev, &sc->sc_mii, 0xffffffff);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else {
		sc->sc_flags |= TULIPF_HAS_MII;
		sc->sc_tick = tlp_mii_tick;
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}
}

/*
 * Lite-On PNIC media switch.  Must handle MII or internal NWAY.
 */
void	tlp_pnic_tmsw_init __P((struct tulip_softc *));
void	tlp_pnic_tmsw_get __P((struct tulip_softc *, struct ifmediareq *));
int	tlp_pnic_tmsw_set __P((struct tulip_softc *));

const struct tulip_mediasw tlp_pnic_mediasw = {
	tlp_pnic_tmsw_init, tlp_pnic_tmsw_get, tlp_pnic_tmsw_set
};

void	tlp_pnic_nway_statchg __P((struct device *));
void	tlp_pnic_nway_tick __P((void *));
int	tlp_pnic_nway_service __P((struct tulip_softc *, int));
void	tlp_pnic_nway_reset __P((struct tulip_softc *));
int	tlp_pnic_nway_auto __P((struct tulip_softc *, int));
void	tlp_pnic_nway_auto_timeout __P((void *));
void	tlp_pnic_nway_status __P((struct tulip_softc *));
void	tlp_pnic_nway_acomp __P((struct tulip_softc *));

void
tlp_pnic_tmsw_init(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	const char *sep = "";

#define	ADD(m, c)	ifmedia_add(&sc->sc_mii.mii_media, (m), (c), NULL)
#define	PRINT(s)	printf("%s%s", sep, s); sep = ", "

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = tlp_pnic_mii_readreg;
	sc->sc_mii.mii_writereg = tlp_pnic_mii_writereg;
	sc->sc_mii.mii_statchg = sc->sc_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, tlp_mediachange,
	    tlp_mediastatus);
	mii_phy_probe(&sc->sc_dev, &sc->sc_mii, 0xffffffff);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		/* XXX What about AUI/BNC support? */
		printf("%s: ", sc->sc_dev.dv_xname);

		tlp_pnic_nway_reset(sc);

		ADD(IFM_MAKEWORD(IFM_ETHER, IFM_10_T, 0, 0),
		    PNIC_NWAY_TW|PNIC_NWAY_CAP10T);
		PRINT("10baseT");

		ADD(IFM_MAKEWORD(IFM_ETHER, IFM_10_T, IFM_FDX, 0),
		    PNIC_NWAY_TW|PNIC_NWAY_FD|PNIC_NWAY_CAP10TFDX);
		PRINT("10baseT-FDX");

		ADD(IFM_MAKEWORD(IFM_ETHER, IFM_100_TX, 0, 0),
		    PNIC_NWAY_TW|PNIC_NWAY_100|PNIC_NWAY_CAP100TX);
		PRINT("100baseTX");

		ADD(IFM_MAKEWORD(IFM_ETHER, IFM_100_TX, IFM_FDX, 0),
		    PNIC_NWAY_TW|PNIC_NWAY_100|PNIC_NWAY_FD|
		    PNIC_NWAY_CAP100TXFDX);
		PRINT("100baseTX-FDX");

		ADD(IFM_MAKEWORD(IFM_ETHER, IFM_AUTO, 0, 0),
		    PNIC_NWAY_TW|PNIC_NWAY_RN|PNIC_NWAY_NW|
		    PNIC_NWAY_CAP10T|PNIC_NWAY_CAP10TFDX|
		    PNIC_NWAY_CAP100TXFDX|PNIC_NWAY_CAP100TX);
		PRINT("auto");

		printf("\n");

		sc->sc_statchg = tlp_pnic_nway_statchg;
		sc->sc_tick = tlp_pnic_nway_tick;
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	} else {
		sc->sc_flags |= TULIPF_HAS_MII;
		sc->sc_tick = tlp_mii_tick;
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}

#undef ADD
#undef PRINT
}

void
tlp_pnic_tmsw_get(sc, ifmr)
	struct tulip_softc *sc;
	struct ifmediareq *ifmr;
{
	struct mii_data *mii = &sc->sc_mii;

	if (sc->sc_flags & TULIPF_HAS_MII)
		tlp_mii_getmedia(sc, ifmr);
	else {
		mii->mii_media_status = 0;
		mii->mii_media_active = IFM_NONE;
		tlp_pnic_nway_service(sc, MII_POLLSTAT);
		ifmr->ifm_status = sc->sc_mii.mii_media_status;
		ifmr->ifm_active = sc->sc_mii.mii_media_active; 
	}
}

int
tlp_pnic_tmsw_set(sc)
	struct tulip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data *mii = &sc->sc_mii;

	if (sc->sc_flags & TULIPF_HAS_MII) {
		/*
		 * Make sure the built-in Tx jabber timer is disabled.
		 */
		TULIP_WRITE(sc, CSR_PNIC_ENDEC, PNIC_ENDEC_JDIS);

		return (tlp_mii_setmedia(sc));
	}

	if (ifp->if_flags & IFF_UP) {
		mii->mii_media_status = 0;
		mii->mii_media_active = IFM_NONE;
		return (tlp_pnic_nway_service(sc, MII_MEDIACHG));
	}

	return (0);
}

void
tlp_pnic_nway_statchg(self)
	struct device *self;
{
	struct tulip_softc *sc = (struct tulip_softc *)self;

	/* Idle the transmit and receive processes. */
	tlp_idle(sc, OPMODE_ST|OPMODE_SR);

	/*
	 * XXX What about Heartbeat Disable?  Is it magically frobbed
	 * XXX by the PHY?  I hope so...
	 */

	sc->sc_opmode &= ~(OPMODE_TTM|OPMODE_FD|OPMODE_PS|OPMODE_PCS|
	    OPMODE_SCR);

	if (IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_10_T) {
		sc->sc_opmode |= OPMODE_TTM;
		TULIP_WRITE(sc, CSR_GPP,
		    GPP_PNIC_OUT(GPP_PNIC_PIN_SPEED_RLY, 0) |
		    GPP_PNIC_OUT(GPP_PNIC_PIN_100M_LPKB, 1));
	} else {
		sc->sc_opmode |= OPMODE_PS|OPMODE_PCS|OPMODE_SCR;
		TULIP_WRITE(sc, CSR_GPP,
		    GPP_PNIC_OUT(GPP_PNIC_PIN_SPEED_RLY, 1) |
		    GPP_PNIC_OUT(GPP_PNIC_PIN_100M_LPKB, 1));
	}

	if (sc->sc_mii.mii_media_active & IFM_FDX)
		sc->sc_opmode |= OPMODE_FD;

	/*
	 * Write new OPMODE bits.  This also restarts the transmit
	 * and receive processes.
	 */
	TULIP_WRITE(sc, CSR_OPMODE, sc->sc_opmode);

	/* XXX Update ifp->if_baudrate */
}

void
tlp_pnic_nway_tick(arg)
	void *arg;
{
	struct tulip_softc *sc = arg;
	int s;

	s = splnet();
	tlp_pnic_nway_service(sc, MII_TICK);
	splx(s);

	timeout(tlp_pnic_nway_tick, sc, hz);
}

/*
 * Support for the Lite-On PNIC internal NWay block.  This is constructed
 * somewhat like a PHY driver for simplicity.
 */

int
tlp_pnic_nway_service(sc, cmd)
	struct tulip_softc *sc;
	int cmd;
{
	struct mii_data *mii = &sc->sc_mii;
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;

	if ((mii->mii_ifp->if_flags & IFF_UP) == 0)
		return (0);

	switch (cmd) {
	case MII_POLLSTAT:
		/* Nothing special to do here. */
		break;

	case MII_MEDIACHG:
		switch (IFM_SUBTYPE(ife->ifm_media)) {
		case IFM_AUTO:
			(void) tlp_pnic_nway_auto(sc, 1);
			break;
		case IFM_100_T4:
			/*
			 * XXX Not supported as a manual setting right now.
			 */
			return (EINVAL);
		default:
			/*
			 * NWAY register data is stored in the ifmedia entry.
			 */
			TULIP_WRITE(sc, CSR_PNIC_NWAY, ife->ifm_data);
		}
		break;

	case MII_TICK:
		/*
		 * Only used for autonegotiation.
		 */
		if (IFM_SUBTYPE(ife->ifm_media) != IFM_AUTO)
			return (0);

		/*
		 * Check to see if we have link.  If we do, we don't
		 * need to restart the autonegotiation process.
		 */
		if (sc->sc_flags & TULIPF_LINK_UP)
			return (0);

		/*
		 * Only retry autonegotiation every 5 seconds.
		 */
		if (++sc->sc_nway_ticks != 5)
			return (0);

		sc->sc_nway_ticks = 0;
		tlp_pnic_nway_reset(sc);
		if (tlp_pnic_nway_auto(sc, 0) == EJUSTRETURN)
			return (0);
		break;
	}

	/* Update the media status. */
	tlp_pnic_nway_status(sc);

	/* Callback if something changed. */
	if (sc->sc_nway_active != mii->mii_media_active ||
	    cmd == MII_MEDIACHG) {
		(*sc->sc_statchg)(&sc->sc_dev);
		sc->sc_nway_active = mii->mii_media_active;
	}
	return (0);
}

void
tlp_pnic_nway_reset(sc)
	struct tulip_softc *sc;
{

	TULIP_WRITE(sc, CSR_PNIC_NWAY, PNIC_NWAY_RS);
	delay(100);
	TULIP_WRITE(sc, CSR_PNIC_NWAY, 0);
}

int
tlp_pnic_nway_auto(sc, waitfor)
	struct tulip_softc *sc;
	int waitfor;
{
	struct mii_data *mii = &sc->sc_mii;
	struct ifmedia_entry *ife = mii->mii_media.ifm_cur;
	u_int32_t reg;
	int i;

	if ((sc->sc_flags & TULIPF_DOINGAUTO) == 0)
		TULIP_WRITE(sc, CSR_PNIC_NWAY, ife->ifm_data);

	if (waitfor) {
		/* Wait 500ms for it to complete. */
		for (i = 0; i < 500; i++) {
			reg = TULIP_READ(sc, CSR_PNIC_NWAY);
			if (reg & PNIC_NWAY_LPAR_MASK) {
				tlp_pnic_nway_acomp(sc);
				return (0);
			}
			delay(1000);
		}
#if 0
		if ((reg & PNIC_NWAY_LPAR_MASK) == 0)
			printf("%s: autonegotiation failed to complete\n",
			    sc->sc_dev.dv_xname);
#endif

		/*
		 * Don't need to worry about clearing DOINGAUTO.
		 * If that's set, a timeout is pending, and it will
		 * clear the flag.
		 */
		return (EIO);
	}

	/*
	 * Just let it finish asynchronously.  This is for the benefit of
	 * the tick handler driving autonegotiation.  Don't want 500ms
	 * delays all the time while the system is running!
	 */
	if ((sc->sc_flags & TULIPF_DOINGAUTO) == 0) {
		sc->sc_flags |= TULIPF_DOINGAUTO;
		timeout(tlp_pnic_nway_auto_timeout, sc, hz >> 1);
	}
	return (EJUSTRETURN);
}

void
tlp_pnic_nway_auto_timeout(arg)
	void *arg;
{
	struct tulip_softc *sc = arg;
	u_int32_t reg;
	int s;

	s = splnet();
	sc->sc_flags &= ~TULIPF_DOINGAUTO;
	reg = TULIP_READ(sc, CSR_PNIC_NWAY);
#if 0
	if ((reg & PNIC_NWAY_LPAR_MASK) == 0)
		printf("%s: autonegotiation failed to complete\n",
		    sc->sc_dev.dv_xname);
#endif

	tlp_pnic_nway_acomp(sc);

	/* Update the media status. */
	(void) tlp_pnic_nway_service(sc, MII_POLLSTAT);
	splx(s);
}

void
tlp_pnic_nway_status(sc)
	struct tulip_softc *sc;
{
	struct mii_data *mii = &sc->sc_mii;
	u_int32_t reg;

	mii->mii_media_status = IFM_AVALID;
	mii->mii_media_active = IFM_ETHER;

	reg = TULIP_READ(sc, CSR_PNIC_NWAY);

	if (sc->sc_flags & TULIPF_LINK_UP)
		mii->mii_media_status |= IFM_ACTIVE;

	if (reg & PNIC_NWAY_NW) {
		if ((reg & PNIC_NWAY_LPAR_MASK) == 0) {
			/* Erg, still trying, I guess... */
			mii->mii_media_active |= IFM_NONE;
			return;
		}

#if 0
		if (reg & PNIC_NWAY_LPAR100T4)
			mii->mii_media_active |= IFM_100_T4;
		else
#endif
		if (reg & PNIC_NWAY_LPAR100TXFDX)
			mii->mii_media_active |= IFM_100_TX|IFM_FDX;
		else if (reg & PNIC_NWAY_LPAR100TX)
			mii->mii_media_active |= IFM_100_TX;
		else if (reg & PNIC_NWAY_LPAR10TFDX)
			mii->mii_media_active |= IFM_10_T|IFM_FDX;
		else if (reg & PNIC_NWAY_LPAR10T)
			mii->mii_media_active |= IFM_10_T;
		else
			mii->mii_media_active |= IFM_NONE;
	} else {
		if (reg & PNIC_NWAY_100)
			mii->mii_media_active |= IFM_100_TX;
		else
			mii->mii_media_active |= IFM_10_T;
		if (reg & PNIC_NWAY_FD)
			mii->mii_media_active |= IFM_FDX;
	}
}

void
tlp_pnic_nway_acomp(sc)
	struct tulip_softc *sc;
{
	u_int32_t reg;

	reg = TULIP_READ(sc, CSR_PNIC_NWAY);
	reg &= ~(PNIC_NWAY_FD|PNIC_NWAY_100|PNIC_NWAY_RN);

	if (reg & (PNIC_NWAY_LPAR100TXFDX|PNIC_NWAY_LPAR100TX))
		reg |= PNIC_NWAY_100;
	if (reg & (PNIC_NWAY_LPAR10TFDX|PNIC_NWAY_LPAR100TXFDX))
		reg |= PNIC_NWAY_FD;

	TULIP_WRITE(sc, CSR_PNIC_NWAY, reg);
}
