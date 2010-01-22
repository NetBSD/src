/* $NetBSD: if_aumac.c,v 1.27 2010/01/22 08:56:05 martin Exp $ */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Device driver for Alchemy Semiconductor Au1x00 Ethernet Media
 * Access Controller.
 *
 * TODO:
 *
 *	Better Rx buffer management; we want to get new Rx buffers
 *	to the chip more quickly than we currently do.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_aumac.c,v 1.27 2010/01/22 08:56:05 martin Exp $");

#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h> 
#include <sys/device.h>
#include <sys/queue.h>

#include <uvm/uvm_extern.h>		/* for PAGE_SIZE */

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>
#include <mips/alchemy/include/aubusvar.h>
#include <mips/alchemy/dev/if_aumacreg.h>

/*
 * The Au1X00 MAC has 4 transmit and receive descriptors.  Each buffer
 * must consist of a single DMA segment, and must be aligned to a 2K
 * boundary.  Therefore, this driver does not perform DMA directly
 * to/from mbufs.  Instead, we copy the data to/from buffers allocated
 * at device attach time.
 *
 * We also skip the bus_dma dance.  The MAC is built in to the CPU, so
 * there's little point in not making assumptions based on the CPU type.
 * We also program the Au1X00 cache to be DMA coherent, so the buffers
 * are accessed via KSEG0 addresses.
 */
#define	AUMAC_NTXDESC		4
#define	AUMAC_NTXDESC_MASK	(AUMAC_NTXDESC - 1)

#define	AUMAC_NRXDESC		4
#define	AUMAC_NRXDESC_MASK	(AUMAC_NRXDESC - 1)

#define	AUMAC_NEXTTX(x)		(((x) + 1) & AUMAC_NTXDESC_MASK)
#define	AUMAC_NEXTRX(x)		(((x) + 1) & AUMAC_NRXDESC_MASK)

#define	AUMAC_TXBUF_OFFSET	0
#define	AUMAC_RXBUF_OFFSET	(MAC_BUFLEN * AUMAC_NTXDESC)
#define	AUMAC_BUFSIZE		(MAC_BUFLEN * (AUMAC_NTXDESC + AUMAC_NRXDESC))

struct aumac_buf {
	vaddr_t buf_vaddr;		/* virtual address of buffer */
	bus_addr_t buf_paddr;		/* DMA address of buffer */
};

/*
 * Software state per device.
 */
struct aumac_softc {
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_mac_sh;	/* MAC space handle */
	bus_space_handle_t sc_macen_sh;	/* MAC enable space handle */
	bus_space_handle_t sc_dma_sh;	/* DMA space handle */
	struct ethercom sc_ethercom;	/* Ethernet common data */
	void *sc_sdhook;		/* shutdown hook */

	void *sc_ih;			/* interrupt cookie */

	struct mii_data sc_mii;		/* MII/media information */

	struct callout sc_tick_ch;	/* tick callout */

	/* Transmit and receive buffers */
	struct aumac_buf sc_txbufs[AUMAC_NTXDESC];
	struct aumac_buf sc_rxbufs[AUMAC_NRXDESC];
	void *sc_bufaddr;

	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next Tx descriptor to use */
	int sc_txdirty;			/* first dirty Tx descriptor */

	int sc_rxptr;			/* next ready Rx descriptor */

#if NRND > 0
	rndsource_element_t rnd_source;
#endif

#ifdef AUMAC_EVENT_COUNTERS
	struct evcnt sc_ev_txstall;	/* Tx stalled */
	struct evcnt sc_ev_rxstall;	/* Rx stalled */
	struct evcnt sc_ev_txintr;	/* Tx interrupts */
	struct evcnt sc_ev_rxintr;	/* Rx interrupts */
#endif

	uint32_t sc_control;		/* MAC_CONTROL contents */
	uint32_t sc_flowctrl;		/* MAC_FLOWCTRL contents */
};

#ifdef AUMAC_EVENT_COUNTERS
#define	AUMAC_EVCNT_INCR(ev)	(ev)->ev_count++
#else
#define	AUMAC_EVCNT_INCR(ev)	/* nothing */
#endif

#define	AUMAC_INIT_RXDESC(sc, x)					\
do {									\
	bus_space_write_4((sc)->sc_st, (sc)->sc_dma_sh,			\
	    MACDMA_RX_STAT((x)), 0);					\
	bus_space_write_4((sc)->sc_st, (sc)->sc_dma_sh,			\
	    MACDMA_RX_ADDR((x)),					\
	    (sc)->sc_rxbufs[(x)].buf_paddr | RX_ADDR_EN);		\
} while (/*CONSTCOND*/0)

static void	aumac_start(struct ifnet *);
static void	aumac_watchdog(struct ifnet *);
static int	aumac_ioctl(struct ifnet *, u_long, void *);
static int	aumac_init(struct ifnet *);
static void	aumac_stop(struct ifnet *, int);

static void	aumac_shutdown(void *);

static void	aumac_tick(void *);

static void	aumac_set_filter(struct aumac_softc *);

static void	aumac_powerup(struct aumac_softc *);
static void	aumac_powerdown(struct aumac_softc *);

static int	aumac_intr(void *);
static int	aumac_txintr(struct aumac_softc *);
static int	aumac_rxintr(struct aumac_softc *);

static int	aumac_mii_readreg(struct device *, int, int);
static void	aumac_mii_writereg(struct device *, int, int, int);
static void	aumac_mii_statchg(struct device *);
static int	aumac_mii_wait(struct aumac_softc *, const char *);

static int	aumac_match(struct device *, struct cfdata *, void *);
static void	aumac_attach(struct device *, struct device *, void *);

int	aumac_copy_small = 0;

CFATTACH_DECL(aumac, sizeof(struct aumac_softc),
    aumac_match, aumac_attach, NULL, NULL);

static int
aumac_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct aubus_attach_args *aa = aux;

	if (strcmp(aa->aa_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static void
aumac_attach(struct device *parent, struct device *self, void *aux)
{
	const uint8_t *enaddr;
	prop_data_t ea;
	struct aumac_softc *sc = (void *) self;
	struct aubus_attach_args *aa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct pglist pglist;
	paddr_t bufaddr;
	vaddr_t vbufaddr;
	int i;

	callout_init(&sc->sc_tick_ch, 0);

	printf(": Au1X00 10/100 Ethernet\n");

	sc->sc_st = aa->aa_st;

	/* Get the MAC address. */
	ea = prop_dictionary_get(device_properties(&sc->sc_dev), "mac-address");
	if (ea == NULL) {
		printf("%s: unable to get mac-addr property\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	KASSERT(prop_object_type(ea) == PROP_TYPE_DATA);
	KASSERT(prop_data_size(ea) == ETHER_ADDR_LEN);
	enaddr = prop_data_data_nocopy(ea);

	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	/* Map the device. */
	if (bus_space_map(sc->sc_st, aa->aa_addrs[AA_MAC_BASE],
	    MACx_SIZE, 0, &sc->sc_mac_sh) != 0) {
		printf("%s: unable to map MAC registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_st, aa->aa_addrs[AA_MAC_ENABLE],
	    MACENx_SIZE, 0, &sc->sc_macen_sh) != 0) {
		printf("%s: unable to map MACEN registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}
	if (bus_space_map(sc->sc_st, aa->aa_addrs[AA_MAC_DMA_BASE],
	    MACx_DMA_SIZE, 0, &sc->sc_dma_sh) != 0) {
		printf("%s: unable to map MACDMA registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/* Make sure the MAC is powered off. */
	aumac_powerdown(sc);

	/* Hook up the interrupt handler. */
	sc->sc_ih = au_intr_establish(aa->aa_irq[0], 1, IPL_NET, IST_LEVEL,
	    aumac_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to register interrupt handler\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	/*
	 * Allocate space for the transmit and receive buffers.
	 */
	if (uvm_pglistalloc(AUMAC_BUFSIZE, 0, ctob(physmem), PAGE_SIZE, 0,
	    &pglist, 1, 0))
		return;

	bufaddr = VM_PAGE_TO_PHYS(TAILQ_FIRST(&pglist));
	vbufaddr = MIPS_PHYS_TO_KSEG0(bufaddr);

	for (i = 0; i < AUMAC_NTXDESC; i++) {
		int offset = AUMAC_TXBUF_OFFSET + (i * MAC_BUFLEN);

		sc->sc_txbufs[i].buf_vaddr = vbufaddr + offset;
		sc->sc_txbufs[i].buf_paddr = bufaddr + offset;
	}

	for (i = 0; i < AUMAC_NRXDESC; i++) {
		int offset = AUMAC_RXBUF_OFFSET + (i * MAC_BUFLEN);

		sc->sc_rxbufs[i].buf_vaddr = vbufaddr + offset;
		sc->sc_rxbufs[i].buf_paddr = bufaddr + offset;
	}

	/*
	 * Power up the MAC before accessing any MAC registers (including
	 * MII configuration.
	 */
	aumac_powerup(sc);

	/*
	 * Initialize the media structures and probe the MII.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = aumac_mii_readreg;
	sc->sc_mii.mii_writereg = aumac_mii_writereg;
	sc->sc_mii.mii_statchg = aumac_mii_statchg;
	sc->sc_ethercom.ec_mii = &sc->sc_mii;
	ifmedia_init(&sc->sc_mii.mii_media, 0, ether_mediachange,
	    ether_mediastatus);

	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);

	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = aumac_ioctl;
	ifp->if_start = aumac_start;
	ifp->if_watchdog = aumac_watchdog;
	ifp->if_init = aumac_init;
	ifp->if_stop = aumac_stop;
	IFQ_SET_READY(&ifp->if_snd);

	/* Attach the interface. */
	if_attach(ifp); 
	ether_ifattach(ifp, enaddr);

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0);
#endif

#ifdef AUMAC_EVENT_COUNTERS
	evcnt_attach_dynamic(&sc->sc_ev_txstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txstall");
	evcnt_attach_dynamic(&sc->sc_ev_rxstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxstall");
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxintr");
#endif

	/* Make sure the interface is shutdown during reboot. */
	sc->sc_sdhook = shutdownhook_establish(aumac_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	return;
}

/*
 * aumac_shutdown:
 *
 *	Make sure the interface is stopped at reboot time.
 */
static void
aumac_shutdown(void *arg)
{
	struct aumac_softc *sc = arg;

	aumac_stop(&sc->sc_ethercom.ec_if, 1);

	/*
	 * XXX aumac_stop leaves device powered up at the moment
	 * XXX but this still isn't enough to keep yamon happy... :-(
	 */
	bus_space_write_4(sc->sc_st, sc->sc_macen_sh, 0, 0);
}

/*
 * aumac_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
static void
aumac_start(struct ifnet *ifp)
{
	struct aumac_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int nexttx;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * unitl we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		/* Grab a packet off the queue. */
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			return;

		/* Get a spare descriptor. */
		if (sc->sc_txfree == 0) {
			/* No more slots left; notify upper layer. */
			ifp->if_flags |= IFF_OACTIVE;
			AUMAC_EVCNT_INCR(&sc->sc_ev_txstall);
			return;
		}
		nexttx = sc->sc_txnext;

		IFQ_DEQUEUE(&ifp->if_snd, m);

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		m_copydata(m, 0, m->m_pkthdr.len,
		    (void *)sc->sc_txbufs[nexttx].buf_vaddr);

		/* Zero out the remainder of any short packets. */
		if (m->m_pkthdr.len < (ETHER_MIN_LEN - ETHER_CRC_LEN))
			memset((char *)sc->sc_txbufs[nexttx].buf_vaddr +
			    m->m_pkthdr.len, 0,
			    ETHER_MIN_LEN - ETHER_CRC_LEN - m->m_pkthdr.len);

		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_STAT(nexttx), 0);
		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_LEN(nexttx),
		    m->m_pkthdr.len < (ETHER_MIN_LEN - ETHER_CRC_LEN) ?
		    ETHER_MIN_LEN - ETHER_CRC_LEN : m->m_pkthdr.len);
		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_ADDR(nexttx),
		    sc->sc_txbufs[nexttx].buf_paddr | TX_ADDR_EN);
		/* XXX - needed??  we should be coherent */
		bus_space_barrier(sc->sc_st, sc->sc_dma_sh, 0 /* XXX */,
		    0 /* XXX */, BUS_SPACE_BARRIER_WRITE);

		/* Advance the Tx pointer. */
		sc->sc_txfree--;
		sc->sc_txnext = AUMAC_NEXTTX(nexttx);

		/* Pass the packet to any BPF listeners. */
		if (ifp->if_bpf)
			bpf_ops->bpf_mtap(ifp->if_bpf, m);

		m_freem(m);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
	/* NOTREACHED */
}

/*
 * aumac_watchdog:	[ifnet interface function]
 *
 *	Watchdog timer handler.
 */
static void
aumac_watchdog(struct ifnet *ifp)
{
	struct aumac_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", sc->sc_dev.dv_xname);
	(void) aumac_init(ifp);

	/* Try to get more packets going. */
	aumac_start(ifp);
}

/*
 * aumac_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
static int
aumac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct aumac_softc *sc = ifp->if_softc;
	int s, error;

	s = splnet();

	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET) {
		/*
		 * Multicast list has changed; set the hardware filter
		 * accordingly.
		 */
		if (ifp->if_flags & IFF_RUNNING)
			aumac_set_filter(sc);
	}

	/* Try to get more packets going. */
	aumac_start(ifp);

	splx(s);
	return (error);
}

/*
 * aumac_intr:
 *
 *	Interrupt service routine.
 */
static int
aumac_intr(void *arg)
{
	struct aumac_softc *sc = arg;
	int status;

	/*
	 * There aren't really any interrupt status bits on the
	 * Au1X00 MAC, and each MAC has a dedicated interrupt
	 * in the CPU's built-in interrupt controller.  Just
	 * check for new incoming packets, and then Tx completions
	 * (for status updating).
	 */
	if ((sc->sc_ethercom.ec_if.if_flags & IFF_RUNNING) == 0)
		return (0);

	status = aumac_rxintr(sc);
	status += aumac_txintr(sc);

#if NRND > 0
	if (RND_ENABLED(&sc->rnd_source))
		rnd_add_uint32(&sc->rnd_source, status);
#endif

	return status;
}

/*
 * aumac_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
static int
aumac_txintr(struct aumac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t stat;
	int i;
	int pkts = 0;

	for (i = sc->sc_txdirty; sc->sc_txfree != AUMAC_NTXDESC;
	     i = AUMAC_NEXTTX(i)) {
		if ((bus_space_read_4(sc->sc_st, sc->sc_dma_sh,
		     MACDMA_TX_ADDR(i)) & TX_ADDR_DN) == 0)
			break;
		pkts++;

		/* ACK interrupt. */
		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_ADDR(i), 0);

		stat = bus_space_read_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_STAT(i));

		if (stat & TX_STAT_FA) {
			/* XXX STATS */
			ifp->if_oerrors++;
		} else
			ifp->if_opackets++;

		if (stat & TX_STAT_EC)
			ifp->if_collisions += 16;
		else
			ifp->if_collisions += TX_STAT_CC(stat);

		sc->sc_txfree++;
		ifp->if_flags &= ~IFF_OACTIVE;

		/* Try to queue more packets. */
		aumac_start(ifp);
	}

	if (pkts)
		AUMAC_EVCNT_INCR(&sc->sc_ev_txintr);

	/* Update the dirty descriptor pointer. */
	sc->sc_txdirty = i;

	/*
	 * If there are no more pending transmissions, cancel the watchdog
	 * timer.
	 */
	if (sc->sc_txfree == AUMAC_NTXDESC)
		ifp->if_timer = 0;

	return pkts;
}

/*
 * aumac_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
static int
aumac_rxintr(struct aumac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	uint32_t stat;
	int i, len;
	int pkts = 0;

	for (i = sc->sc_rxptr;; i = AUMAC_NEXTRX(i)) {
		if ((bus_space_read_4(sc->sc_st, sc->sc_dma_sh,
		     MACDMA_RX_ADDR(i)) & RX_ADDR_DN) == 0)
			break;
		pkts++;

		stat = bus_space_read_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_RX_STAT(i));

#define PRINTERR(str)							\
	do {								\
		error++;						\
		printf("%s: %s\n", sc->sc_dev.dv_xname, str);		\
	} while (0)

		if (stat & RX_STAT_ERRS) {
			int error = 0;

#if 0	/*
	 * Missed frames are a semi-frequent occurence with this hardware,
	 * and reporting of them just makes everything run slower and fills
	 * the system log.  Be silent.
	 * 
	 * Additionally, this missed bit indicates an error with the previous
	 * packet, and not with this one!  So PRINTERR is definitely wrong
	 * here.
	 *
	 * These should probably all be converted to evcnt counters anyway.
	 */
			if (stat & RX_STAT_MI)
				PRINTERR("missed frame");
#endif
			if (stat & RX_STAT_UC)
				PRINTERR("unknown control frame");
			if (stat & RX_STAT_LE)
				PRINTERR("short frame");
			if (stat & RX_STAT_CR)
				PRINTERR("CRC error");
			if (stat & RX_STAT_ME)
				PRINTERR("medium error");
			if (stat & RX_STAT_CS)
				PRINTERR("late collision");
			if (stat & RX_STAT_FL)
				PRINTERR("frame too big");
			if (stat & RX_STAT_RF)
				PRINTERR("runt frame (collision)");
			if (stat & RX_STAT_WT)
				PRINTERR("watch dog");
			if (stat & RX_STAT_DB) {
				if (stat & (RX_STAT_CS | RX_STAT_RF |
				    RX_STAT_CR)) {
					if (!error)
						goto pktok;
				} else
					PRINTERR("dribbling bit");
			}
#undef PRINTERR
			ifp->if_ierrors++;

 dropit:
			/* reuse the current descriptor */
			AUMAC_INIT_RXDESC(sc, i);
			continue;
		}
 pktok:
		len = RX_STAT_L(stat);

		/*
		 * The Au1X00 MAC includes the CRC with every packet;
		 * trim it off here.
		 */
		len -= ETHER_CRC_LEN;

		/*
		 * Truncate the packet if it's too big to fit in
		 * a single mbuf cluster.
		 */
		if (len > MCLBYTES - 2)
			len = MCLBYTES - 2;

		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
			printf("%s: unable to allocate Rx mbuf\n",
			    sc->sc_dev.dv_xname);
			goto dropit;
		}
		if (len > MHLEN - 2) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				printf("%s: unable to allocate Rx cluster\n",
				    sc->sc_dev.dv_xname);
				m_freem(m);
				goto dropit;
			}
		}

		m->m_data += 2;		/* align payload */
		memcpy(mtod(m, void *),
		    (void *)sc->sc_rxbufs[i].buf_vaddr, len);
		AUMAC_INIT_RXDESC(sc, i);

		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

		/* Pass this up to any BPF listeners. */
		if (ifp->if_bpf)
			bpf_ops->bpf_mtap(ifp->if_bpf, m);

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
		ifp->if_ipackets++;
	}
	if (pkts)
		AUMAC_EVCNT_INCR(&sc->sc_ev_rxintr);
	if (pkts == AUMAC_NRXDESC)
		AUMAC_EVCNT_INCR(&sc->sc_ev_rxstall);

	/* Update the receive pointer. */
	sc->sc_rxptr = i;

	return pkts;
}

/*
 * aumac_tick:
 *
 *	One second timer, used to tick the MII.
 */
static void
aumac_tick(void *arg)
{
	struct aumac_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, aumac_tick, sc);
}

/*
 * aumac_init:		[ifnet interface function]
 *
 *	Initialize the interface.  Must be called at splnet().
 */
static int
aumac_init(struct ifnet *ifp)
{
	struct aumac_softc *sc = ifp->if_softc;
	int i, error = 0;

	/* Cancel any pending I/O, reset MAC. */
	aumac_stop(ifp, 0);

	/* Set up the transmit ring. */
	for (i = 0; i < AUMAC_NTXDESC; i++) {
		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_STAT(i), 0);
		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_LEN(i), 0);
		bus_space_write_4(sc->sc_st, sc->sc_dma_sh,
		    MACDMA_TX_ADDR(i), sc->sc_txbufs[i].buf_paddr);
	}
	sc->sc_txfree = AUMAC_NTXDESC;
	sc->sc_txnext = TX_ADDR_CB(bus_space_read_4(sc->sc_st, sc->sc_dma_sh,
	    MACDMA_TX_ADDR(0)));
	sc->sc_txdirty = sc->sc_txnext;

	/* Set up the receive ring. */
	for (i = 0; i < AUMAC_NRXDESC; i++)
			AUMAC_INIT_RXDESC(sc, i);
	sc->sc_rxptr = RX_ADDR_CB(bus_space_read_4(sc->sc_st, sc->sc_dma_sh,
	    MACDMA_RX_ADDR(0)));

	/*
	 * Power up the MAC.
	 */
	aumac_powerup(sc);

	sc->sc_control |= CONTROL_DO | CONTROL_TE | CONTROL_RE;
#if _BYTE_ORDER == _BIG_ENDIAN
	sc->sc_control |= CONTROL_EM;
#endif

	/* Set the media. */
	if ((error = ether_mediachange(ifp)) != 0)
		goto out;

	/*
	 * Set the receive filter.  This will actually start the transmit
	 * and receive processes.
	 */
	aumac_set_filter(sc);

	/* Start the one second clock. */
	callout_reset(&sc->sc_tick_ch, hz, aumac_tick, sc);

	/* ...all done! */
	ifp->if_flags |= IFF_RUNNING; 
	ifp->if_flags &= ~IFF_OACTIVE;

out:
	if (error)
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	return (error);
}

/*
 * aumac_stop:		[ifnet interface function]
 *
 *	Stop transmission on the interface.
 */
static void
aumac_stop(struct ifnet *ifp, int disable)
{
	struct aumac_softc *sc = ifp->if_softc;

	/* Stop the one-second clock. */
	callout_stop(&sc->sc_tick_ch);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	/* Stop the transmit and receive processes. */
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_CONTROL, 0);

	/* Power down/reset the MAC. */
	aumac_powerdown(sc);

	/* Mark the interface as down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/*
 * aumac_powerdown:
 *
 *	Power down the MAC.
 */
static void
aumac_powerdown(struct aumac_softc *sc)
{

	/* Disable the MAC clocks, and place the device in reset. */
	// bus_space_write_4(sc->sc_st, sc->sc_macen_sh, 0, MACEN_JP);

	// delay(10000);
}

/*
 * aumac_powerup:
 *
 *	Bring the device out of reset.
 */
static void
aumac_powerup(struct aumac_softc *sc)
{

	/* Enable clocks to the MAC. */
	bus_space_write_4(sc->sc_st, sc->sc_macen_sh, 0, MACEN_JP|MACEN_CE);

	/* Enable MAC, coherent transactions, pass only valid frames. */
	bus_space_write_4(sc->sc_st, sc->sc_macen_sh, 0,
	    MACEN_E2|MACEN_E1|MACEN_E0|MACEN_CE);

	delay(20000);
}

/*
 * aumac_set_filter:
 *
 *	Set up the receive filter.
 */
static void
aumac_set_filter(struct aumac_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	const uint8_t *enaddr = CLLADDR(ifp->if_sadl);
	uint32_t mchash[2], crc;

	sc->sc_control &= ~(CONTROL_PM | CONTROL_PR);

	/* Stop the receiver. */
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_CONTROL,
	    sc->sc_control & ~CONTROL_RE);

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_control |= CONTROL_PR;
		goto allmulti;
	}

	/* Set the station address. */
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_ADDRHIGH,
	    enaddr[4] | (enaddr[5] << 8));
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_ADDRLOW,
	    enaddr[0] | (enaddr[1] << 8) | (enaddr[2] << 16) |
	    (enaddr[3] << 24));

	sc->sc_control |= CONTROL_HP;

	mchash[0] = mchash[1] = 0;

	/*
	 * Set up the multicast address filter by passing all multicast
	 * addresses through a CRC generator, and then using the high
	 * order 6 bits as an index into the 64-bit multicast hash table.
	 * The high order bits select the word, while the rest of the bits
	 * select the bit within the word.
	 */
	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is large enough to require all bits set.)
			 */
			goto allmulti;
		}

		crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);

		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Set the corresponding bit in the filter. */
		mchash[crc >> 5] |= 1U << (crc & 0x1f);

		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;

	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_HASHHIGH,
	    mchash[1]);
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_HASHLOW,
	    mchash[0]);

	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_CONTROL,
	    sc->sc_control);
	return;

 allmulti:
	sc->sc_control |= CONTROL_PM;
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_CONTROL,
	    sc->sc_control);
}

/*
 * aumac_mii_wait:
 *
 *	Wait for the MII interface to not be busy.
 */
static int
aumac_mii_wait(struct aumac_softc *sc, const char *msg)
{
	int i;

	for (i = 0; i < 10000; i++) {
		if ((bus_space_read_4(sc->sc_st, sc->sc_mac_sh,
		     MAC_MIICTRL) & MIICTRL_MB) == 0)
			return (0);
		delay(10);
	}

	printf("%s: MII failed to %s\n", sc->sc_dev.dv_xname, msg);
	return (1);
}

/*
 * aumac_mii_readreg:	[mii interface function]
 *
 *	Read a PHY register on the MII.
 */
static int
aumac_mii_readreg(struct device *self, int phy, int reg)
{
	struct aumac_softc *sc = (void *) self;

	if (aumac_mii_wait(sc, "become ready"))
		return (0);

	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_MIICTRL,
	    MIICTRL_PHYADDR(phy) | MIICTRL_MIIREG(reg));

	if (aumac_mii_wait(sc, "complete"))
		return (0);

	return (bus_space_read_4(sc->sc_st, sc->sc_mac_sh, MAC_MIIDATA) &
	    MIIDATA_MASK);
}

/*
 * aumac_mii_writereg:	[mii interface function]
 *
 *	Write a PHY register on the MII.
 */
static void
aumac_mii_writereg(struct device *self, int phy, int reg, int val)
{
	struct aumac_softc *sc = (void *) self;

	if (aumac_mii_wait(sc, "become ready"))
		return;

	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_MIIDATA, val);
	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_MIICTRL,
	    MIICTRL_PHYADDR(phy) | MIICTRL_MIIREG(reg) | MIICTRL_MW);

	(void) aumac_mii_wait(sc, "complete");
}

/*
 * aumac_mii_statchg:	[mii interface function]
 *
 *	Callback from MII layer when media changes.
 */
static void
aumac_mii_statchg(struct device *self)
{
	struct aumac_softc *sc = (void *) self;

	if ((sc->sc_mii.mii_media_active & IFM_FDX) != 0)
		sc->sc_control |= CONTROL_F;
	else
		sc->sc_control &= ~CONTROL_F;

	bus_space_write_4(sc->sc_st, sc->sc_mac_sh, MAC_CONTROL,
	    sc->sc_control);
}
