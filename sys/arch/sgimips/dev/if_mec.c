/* $NetBSD: if_mec.c,v 1.13 2004/01/11 14:01:46 sekiya Exp $	 */

/*
 * Copyright (c) 2003 Christopher SEKIYA
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
 *          NetBSD Project.  See http://www.NetBSD.org/ for
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_mec.c,v 1.13 2004/01/11 14:01:46 sekiya Exp $");

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
#include <machine/machtype.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <sgimips/dev/macevar.h>

#include <sgimips/dev/if_mecreg.h>
#include <dev/arcbios/arcbios.h>
#include <dev/arcbios/arcbiosvar.h>

struct mec_tx_dma_desc {
	u_int64_t       command;
	u_int64_t       concat[3];
	u_int8_t        buffer[120];
};

#define MEC_NTXDESC             64
#define MEC_NRXDESC             16

struct mec_control {
	struct mec_tx_dma_desc tx_desc[MEC_NRXDESC];
};

struct mec_softc {
	struct device   sc_dev;

	bus_space_tag_t sc_st;
	bus_space_handle_t sc_sh;
	bus_dma_tag_t   sc_dmat;

	struct ethercom sc_ethercom;

	unsigned char   sc_enaddr[ETHER_ADDR_LEN];

	void           *sc_sdhook;

	struct mii_data sc_mii;
	int             phy;
	struct callout  sc_callout;

	struct mec_control *sc_control;

	/* DMA structures for control data (DMA RX/TX descriptors) */
	int             sc_ncdseg;
	bus_dma_segment_t sc_cdseg;
	bus_dmamap_t    sc_cdmap;
	int             sc_nextrx;

	/* DMA structures for TX packet data */
	bus_dma_segment_t sc_txseg[MEC_NTXDESC];
	bus_dmamap_t    sc_txmap[MEC_NTXDESC];
	struct mbuf    *sc_txmbuf[MEC_NTXDESC];

	int             sc_nexttx;
	int             sc_prevtx;
	int             sc_nfreetx;


	bus_dma_segment_t rx_seg[16];
	bus_dmamap_t    rx_map[16];
	unsigned char  *rx_buf[16];
	struct mbuf    *rx_mbuf[16];
	int             rx_nseg;


	caddr_t        *rx_buffer[16];
	caddr_t        *tx_buffer;
	u_int64_t       rx_read_ptr;
	u_int64_t       rx_write_ptr;
	u_int64_t       rx_read_length;
	u_int64_t       rx_boffset;
	u_int64_t       tx_read_ptr;
	u_int64_t       tx_write_ptr;
	u_int64_t       tx_available;
	u_int64_t       tx_read_length;
	u_int64_t       tx_boffset;
	u_int64_t       me_rxdelay;

#if NRND > 0
	rndsource_element_t rnd_source;	/* random source */
#endif
};

#define TX_RING_SIZE	16

static int      mec_match(struct device *, struct cfdata *, void *);
static void     mec_attach(struct device *, struct device *, void *);
static void     mec_start(struct ifnet *);
static void     mec_watchdog(struct ifnet *);
static int      mec_ioctl(struct ifnet *, u_long, caddr_t);
static int      mec_mii_readreg(struct device *, int, int);
static void     mec_mii_writereg(struct device *, int, int, int);
static int      mec_mii_wait(struct mec_softc *);
static void     mec_statchg(struct device *);
int             mec_mediachange(struct ifnet *);
void            mec_mediastatus(struct ifnet *, struct ifmediareq *);
void            enaddr_aton(const char *, u_int8_t *);
void            mec_reset(struct mec_softc *);
int             mec_init(struct ifnet * ifp);
int             mec_intr(void *arg);
void            mec_stop(struct ifnet * ifp, int disable);

static void     mec_rxintr(struct mec_softc * sc, u_int64_t status);

CFATTACH_DECL(mec, sizeof(struct mec_softc),
	      mec_match, mec_attach, NULL, NULL);

static int
mec_match(struct device * parent, struct cfdata * match, void *aux)
{
	if (mach_type != MACH_SGI_IP32)
		return 0;
	return 1;
}

static void
mec_attach(struct device * parent, struct device * self, void *aux)
{
	struct mec_softc *sc = (void *) self;
	struct mace_attach_args *maa = aux;
	struct ifnet   *ifp = &sc->sc_ethercom.ec_if;
	u_int64_t       command;
	u_int64_t	address = 0;
	char           *macaddr;
	struct mii_softc *child;
	int             i, err;

	sc->sc_st = maa->maa_st;
	if (bus_space_subregion(sc->sc_st, maa->maa_sh,
				maa->maa_offset, 0,
				&sc->sc_sh) != 0) {
		printf(": can't map i/o space\n");
		return;
	}

	/* Reset device */
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, MEC_MAC_CORE_RESET);
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, 0);

	printf(": MAC-110 Ethernet, ");
	command = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL);
	printf("rev %lld\n",
	       (command & MEC_MAC_REVISION) >> MEC_MAC_REVISION_SHIFT);

	/* set up DMA structures */

	sc->sc_dmat = maa->maa_dmat;

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((err = bus_dmamem_alloc(sc->sc_dmat, sizeof(struct mec_control),
				    PAGE_SIZE, PAGE_SIZE, &sc->sc_cdseg,
				 1, &sc->sc_ncdseg, BUS_DMA_NOWAIT)) != 0) {
		printf(": unable to allocate control data, error = %d\n", err);
		goto fail_0;
	}
	if ((err = bus_dmamem_map(sc->sc_dmat, &sc->sc_cdseg, sc->sc_ncdseg,
				  sizeof(struct mec_control),
				  (caddr_t *) & sc->sc_control,
				  BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		printf(": unable to map control data, error = %d\n", err);
		goto fail_1;
	}
	memset(sc->sc_control, 0, sizeof(struct mec_control));

	if ((err = bus_dmamap_create(sc->sc_dmat, sizeof(struct mec_control),
				   1, sizeof(struct mec_control), PAGE_SIZE,
				     BUS_DMA_NOWAIT, &sc->sc_cdmap)) != 0) {
		printf(": unable to create DMA map for control data, error "
		       "= %d\n", err);
		goto fail_2;
	}
	if ((err = bus_dmamap_load(sc->sc_dmat, sc->sc_cdmap, sc->sc_control,
				   sizeof(struct mec_control),
				   NULL, BUS_DMA_NOWAIT)) != 0) {
		printf(": unable to load DMA map for control data, error "
		       "= %d\n", err);
		goto fail_3;
	}
	/* Create transmit buffer DMA maps */
	for (i = 0; i < MEC_NTXDESC; i++) {
		if ((err = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1, MCLBYTES,
					     0, BUS_DMA_NOWAIT,
					     &sc->sc_txmap[i])) != 0) {
			printf(": unable to create tx DMA map %d, error = %d\n",
			       i, err);
			goto fail_4;
		}
	}

	/* Create the receive buffer DMA maps */
	for (i = 0; i < 16; i++) {
		if ((err = bus_dmamap_create(sc->sc_dmat, 4096,
					     1, 4096, PAGE_SIZE,
				    BUS_DMA_NOWAIT, &sc->rx_map[i])) != 0) {
			printf("%s: unable to create rx DMA map %d",
			       sc->sc_dev.dv_xname, err);
			goto fail_5;
		}
	}

	if ((macaddr = ARCBIOS->GetEnvironmentVariable("eaddr")) == NULL) {
		panic(": unable to get MAC address!");
	}
	enaddr_aton(macaddr, sc->sc_enaddr);

	for (i = 0; i < 6; i++) {
		address = address << 8;
		address += sc->sc_enaddr[i];
	}

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_STATION, (address));

	printf("%s: station address %s\n", sc->sc_dev.dv_xname,
	       ether_sprintf(sc->sc_enaddr));

	/* Done, now attach everything */

	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = mec_mii_readreg;
	sc->sc_mii.mii_writereg = mec_mii_writereg;
	sc->sc_mii.mii_statchg = mec_statchg;

	/* Set up PHY properties */
	ifmedia_init(&sc->sc_mii.mii_media, 0, mec_mediachange,
		     mec_mediastatus);
	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
		   MII_OFFSET_ANY, 0);

	child = LIST_FIRST(&sc->sc_mii.mii_phys);
	if (child == NULL) {
		/* No PHY attached */
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER | IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_MANUAL);
	} else {
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER | IFM_AUTO);
		sc->phy = child->mii_phy;
	}

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = mec_ioctl;
	ifp->if_start = mec_start;
	ifp->if_watchdog = mec_watchdog;
	ifp->if_init = mec_init;
	ifp->if_stop = mec_stop;
	ifp->if_mtu = ETHERMTU;
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_ethercom.ec_if.if_capabilities |= IFCAP_CSUM_IPv4 |
		IFCAP_CSUM_TCPv4 | IFCAP_CSUM_UDPv4;

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);
#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_NET, 0);
#endif
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall though.
	 */
fail_5:
	for (i = 0; i < 16; i++) {
		if (sc->rx_map[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->rx_map[i]);
	}
fail_4:
	for (i = 0; i < MEC_NTXDESC; i++) {
		if (sc->sc_txmap[i] != NULL)
			bus_dmamap_destroy(sc->sc_dmat, sc->sc_txmap[i]);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cdmap);
fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cdmap);
fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t) sc->sc_control,
			 sizeof(struct mec_control));
fail_1:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_cdseg, sc->sc_ncdseg);
fail_0:
	return;
}

void
mec_stop(struct ifnet * ifp, int disable)
{
	printf("mec_stop\n");
}

static int
mec_mii_readreg(struct device * self, int phy, int reg)
{
	struct mec_softc *sc = (struct mec_softc *) self;
	u_int64_t       val;
	int             i = 0;

        if (mec_mii_wait(sc) != 0)
                return 0;

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_ADDRESS,
			  (phy << MEC_PHY_ADDR_DEVSHIFT) | (reg & 0x1f));
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_READ_INITIATE, 1);
	delay(25);

	for (i = 0; i < 20; i++) {
		delay(30);

		val = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_PHY_DATA);

		if ((val & MEC_PHY_DATA_BUSY) == 0)
			return (int) val & MEC_PHY_DATA_VALUE;
	}
	return 0;

}

void
mec_mii_writereg(struct device * self, int phy, int reg, int val)
{
	struct mec_softc *sc = (struct mec_softc *) self;

	if (mec_mii_wait(sc) != 0)
	{
		printf("timed out writing %x: %x\n", reg, val);
		return;
	}

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_ADDRESS,
			  (phy << MEC_PHY_ADDR_DEVSHIFT) | (reg & 0x1f));

	delay(60);

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_PHY_DATA,
			  val & MEC_PHY_DATA_VALUE);

	delay(60);

	(void) mec_mii_wait(sc);

	return;
}

static int
mec_mii_wait(struct mec_softc * sc)
{
	int             i;
	int s;

	for (i = 0; i < 100; i++) {
		u_int64_t       busy;

		delay(30);

		s = splhigh();

		/* i have absolutely no idea why this must be _4.  if it is
		   _8, writes will busy out. */
		busy = bus_space_read_4(sc->sc_st, sc->sc_sh, MEC_PHY_DATA);
		splx(s);

		if ((busy & MEC_PHY_DATA_BUSY) == 0)
			return 0;
		if (busy == 0xffff)
			return 0;
	}

	printf("%s: MII timed out\n", sc->sc_dev.dv_xname);
	return 1;
}

void
mec_statchg(struct device * self)
{
	struct mec_softc *sc = (void *) self;
	u_int32_t       control;

	printf("mec_statchg\n");
	control = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL);
	control &= ~(0x1ffff00 | MEC_MAC_FULL_DUPLEX | MEC_MAC_SPEED_SELECT);

	/* must also set IPG here for duplex stuff ... */
	if ((sc->sc_mii.mii_media_active & IFM_FDX) != 0) {
		control |= MEC_MAC_FULL_DUPLEX;
	} else {
		/* set IPG */
	}

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, control);

	/* turn on TX DMA via dma_control here */

	return;
}

void
mec_mediastatus(struct ifnet * ifp, struct ifmediareq * ifmr)
{
	struct mec_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

int
mec_mediachange(struct ifnet * ifp)
{
	struct mec_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return 0;

	return (mii_mediachg(&sc->sc_mii));
}

void
enaddr_aton(const char *str, u_int8_t * eaddr)
{
	int             i;
	char            c;

	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		if (*str == ':')
			str++;

		c = *str++;
		if (isdigit(c)) {
			eaddr[i] = (c - '0');
		} else if (isxdigit(c)) {
			eaddr[i] = (toupper(c) + 10 - 'A');
		}
		c = *str++;
		if (isdigit(c)) {
			eaddr[i] = (eaddr[i] << 4) | (c - '0');
		} else if (isxdigit(c)) {
			eaddr[i] = (eaddr[i] << 4) | (toupper(c) + 10 - 'A');
		}
	}
}

static void
mec_start(struct ifnet * ifp)
{
#if 0
	struct device  *self = ifp->if_softc;
	struct mec_softc *sc = ifp->if_softc;
	int             i;
	for (i = 4; i < 7; i++)
		printf("mec_statchg: phy reg %x %x\n", i, mec_mii_readreg(self,
							       sc->phy, i));

	struct mbuf    *m0, *m;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		/*
                 * Grab a packet off the queue.
                 */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
		m = NULL;

		if (sc->sc_txpending == fXP_NTXCB) {
			FXP_EVCNT_INCR(&sc->sc_ev_txstall);
			break;
		}
		/*
                 * Get the next available transmit descriptor.
                 */
		nexttx = FXP_NEXTTX(sc->sc_txlast);
		txd = FXP_CDTX(sc, nexttx);
		txs = FXP_DSTX(sc, nexttx);
		dmamap = txs->txs_dmamap;

		/*
                 * Load the DMA map.  If this fails, the packet either
                 * didn't fit in the allotted number of frags, or we were
                 * short on resources.  In this case, we'll copy and try
                 * again.
                 */
		if (bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
				     BUS_DMA_WRITE | BUS_DMA_NOWAIT) != 0) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				printf("%s: unable to allocate Tx mbuf\n",
				       sc->sc_dev.dv_xname);
				break;
			}
			if (m0->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					printf("%s: unable to allocate Tx "
					  "cluster\n", sc->sc_dev.dv_xname);
					m_freem(m);
					break;
				}
			}
			m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, caddr_t));
			m->m_pkthdr.len = m->m_len = m0->m_pkthdr.len;
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap,
					 m, BUS_DMA_WRITE | BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				"error = %d\n", sc->sc_dev.dv_xname, error);
				break;
			}
		}
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m != NULL) {
			m_freem(m0);
			m0 = m;
		}
		/* Initialize the fraglist. */
		for (seg = 0; seg < dmamap->dm_nsegs; seg++) {
			txd->txd_tbd[seg].tb_addr =
				htole32(dmamap->dm_segs[seg].ds_addr);
			txd->txd_tbd[seg].tb_size =
		}

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
				BUS_DMASYNC_PREWRITE);

		/*
                 * Store a pointer to the packet so we can free it later.
                 */
		txs->txs_mbuf = m0;

		/*
                 * Initialize the transmit descriptor.
                 */
		/* BIG_ENDIAN: no need to swap to store 0 */
		txd->txd_txcb.cb_status = 0;
		txd->txd_txcb.cb_command =
			htole16(FXP_CB_COMMAND_XMIT | FXP_CB_COMMAND_SF);
		txd->txd_txcb.tx_threshold = tx_threshold;
		txd->txd_txcb.tbd_number = dmamap->dm_nsegs;

		FXP_CDTXSYNC(sc, nexttx,
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		/* Advance the tx pointer. */
		sc->sc_txpending++;
		sc->sc_txlast = nexttx;

#if NBPFILTER > 0
		/*
                 * Pass packet to bpf if there is a listener.
                 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif
	}

	if (sc->sc_txpending == FXP_NTXCB) {
		/* No more slots; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}
	if (sc->sc_txpending != opending) {
		/*
                 * We enqueued packets.  If the transmitter was idle,
                 * reset the txdirty pointer.
                 */
		if (opending == 0)
			sc->sc_txdirty = FXP_NEXTTX(lasttx);

		/*
                 * Cause the chip to interrupt and suspend command
                 * processing once the last packet we've enqueued
                 * has been transmitted.
                 */
		FXP_CDTX(sc, sc->sc_txlast)->txd_txcb.cb_command |=
			htole16(FXP_CB_COMMAND_I | FXP_CB_COMMAND_S);
		FXP_CDTXSYNC(sc, sc->sc_txlast,
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/*
                 * The entire packet chain is set up.  Clear the suspend bit
                 * on the command prior to the first packet we set up.
                 */
		FXP_CDTXSYNC(sc, lasttx,
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		FXP_CDTX(sc, lasttx)->txd_txcb.cb_command &=
			htole16(~FXP_CB_COMMAND_S);
		FXP_CDTXSYNC(sc, lasttx,
			     BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		FXP_CDTX(sc, lasttx)->txd_txcb.cb_command &=
			htole16(~FXP_CB_COMMAND_S);
		FXP_CDTXSYNC(sc, lasttx,
			     BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
#endif
}

static int
mec_ioctl(struct ifnet * ifp, u_long cmd, caddr_t data)
{
	struct mec_softc *sc = ifp->if_softc;
	struct ifreq   *ifr = (struct ifreq *) data;
	int             s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			/*
                         * Multicast list has changed; set the hardware filter
                         * accordingly.
                         */
			error = mec_init(ifp);
		}
		break;
	}

	/* Try to get more packets going. */
	mec_start(ifp);

	splx(s);
	return (error);
}

int
mec_init(struct ifnet * ifp)
{
	struct mec_softc *sc = ifp->if_softc;
	u_int64_t       address = 0;
	u_int64_t	control = 0;
	int             i;

	/* stop things via mec_stop(sc) */

	/* Reset device */
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, MEC_MAC_CORE_RESET);
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, 0);

	for (i = 0; i < 6; i++) {
		address = address << 8;
		address += sc->sc_enaddr[i];
	}

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_STATION, (address));

	/* Default to 100/half and let autonegotiation work its magic */
	control = MEC_MAC_SPEED_SELECT;

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, control);


	/* Initialize TX/RX pointers, start DMA */

	/* set the TX ring pointer to the base address */
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_TX_RING_BASE,
			  MIPS_KSEG1_TO_PHYS(sc->sc_control));

	/* Set the RX pointers to the 4k boundaries */

	for (i = 0; i < 16; i++)
		bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MCL_RX_FIFO,
				  MIPS_KSEG1_TO_PHYS(sc->rx_buf[i]));

	sc->rx_read_ptr = 0;
	sc->rx_read_length = 16;
#if 0
	sc->rx_boffset = boffset * sizeof(u_int64_t);
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_TIMER, me_rxdelay);

	memset(sc->tx_buffer, 0, TX_RING_SIZE * sizeof(struct tx_fifo));
#endif

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_TX_RING_BASE,
			  MIPS_KSEG1_TO_PHYS(sc->tx_buffer));
	sc->tx_read_ptr = 0;
	sc->tx_write_ptr = 0;
	sc->tx_available = TX_RING_SIZE;


	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	mec_start(ifp);

	mii_mediachg(&sc->sc_mii);
	return 0;
}

void
mec_reset(struct mec_softc * sc)
{
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, MEC_MAC_CORE_RESET);
	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL, 0x00);
	/*
         * bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_MAC_CONTROL,
         * MEC_MAC_FULL_DUPLEX | );
         */
	delay(1000);

	printf("mec: control now %llx\n", bus_space_read_8(sc->sc_st, sc->sc_sh,
							   MEC_MAC_CONTROL));

}

void
mec_watchdog(struct ifnet * ifp)
{
#if 0
	/* struct mec_softc *sc = ifp->if_softc; */

	/*
         * Since we're not interrupting every packet, sweep
         * up before we report an error.
         */
	//pcn_txintr(sc);

	if (sc->sc_txfree != PCN_NTXDESC) {
		printf("%s: device timeout (txfree %d txsfree %d)\n",
		       sc->sc_dev.dv_xname, sc->sc_txfree, sc->sc_txsfree);
		ifp->if_oerrors++;

		/* Reset the interface. */
		(void) mec_init(ifp);
	} */
#endif

	/* Try to get more packets going. */
		mec_start(ifp);
}


int
mec_intr(void *arg)
{
	struct mec_softc *sc = arg;
	struct ifnet   *ifp = &sc->sc_ethercom.ec_if;
	u_int32_t       stat;

	if ((ifp->if_flags & IFF_RUNNING) == 0 ||
	    (sc->sc_dev.dv_flags & DVF_ACTIVE) == 0)
		return (0);

	stat = bus_space_read_8(sc->sc_st, sc->sc_sh, MEC_INT_STATUS);

	while (stat) {
		if (stat & MEC_INT_RX_THRESHOLD) {
			mec_rxintr(sc, stat);
			stat &= ~MEC_INT_RX_THRESHOLD;
		}
		if (stat & (MEC_INT_TX_EMPTY | MEC_INT_TX_PACKET_SENT)) {
			/* mec_txstat(sc); */
			stat &= ~(MEC_INT_TX_EMPTY | MEC_INT_TX_PACKET_SENT);
		}
	}

	bus_space_write_8(sc->sc_st, sc->sc_sh, MEC_INT_STATUS, 0xff);

	return 0;
}

static void
mec_rxintr(struct mec_softc * sc, u_int64_t status)
{
#if 0
	int             count = 0;
	struct mbuf    *m;
	int             i, framelen;
	u_int8_t        pktstat;
	u_int32_t       status;
	int             new_end, orig_end;
	unsigned int    rx_fifo_ptr;
	unsigned int    rx_sequence_no;
	u_int64_t       ptr;
	struct ifnet   *ifp = &sc->sc_ethercom.ec_if;

	rx_fifo_ptr = (status & MEC_INT_RX_MCL_FIFO_ALIAS) >> 8;
	rx_sequence_no = (status & MEC_INT_RX_SEQUENCE_NUMBER) >> 25;

	if (rx_fifo_ptr >= 32)
		panic("mec_rxintr: rx_fifo_ptr is %i\n", rx_fifo_ptr);

	ptr = sc->rptr;		/* XXX */

	while (rx_fifo_ptr != ptr) {
		m = sc->rx_fifo[RXRINGINDEX(ptr)];
	}
#endif
}
