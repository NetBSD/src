/*	$NetBSD: if_sip.c,v 1.2 1999/08/03 17:25:52 thorpej Exp $	*/

/*-
 * Copyright (c) 1999 Network Computer, Inc.
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
 * 3. Neither the name of Network Computer, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NETWORK COMPUTER, INC. AND CONTRIBUTORS
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
 * Device driver for the Silicon Integrated Systems SiS900 10/100 PCI
 * Ethernet controller.
 *    
 * Written by Jason R. Thorpe for Network Computer, Inc.
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
#include <sys/queue.h>

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

#include <dev/mii/miivar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_sipreg.h>

/*
 * Devices supported by this driver.
 */
const struct sip_product {
	pci_vendor_id_t		sip_vendor;
	pci_product_id_t	sip_product;
	const char		*sip_name;
} sip_products[] = {
	{ PCI_VENDOR_SIS,	PCI_PRODUCT_SIS_900,
	  "SiS 900 10/100 Ethernet" },

	{ 0,			0,
	  NULL },
};

/*
 * Transmit descriptor list size.  This is arbitrary, but allocate
 * enough descriptors for 64 pending transmissions, and 16 segments
 * per packet.  This MUST work out to a power of 2.
 */
#define	SIP_NTXSEGS		16

#define	SIP_TXQUEUELEN		64
#define	SIP_NTXDESC		(SIP_TXQUEUELEN * SIP_NTXSEGS)
#define	SIP_NTXDESC_MASK	(SIP_NTXDESC - 1)
#define	SIP_NEXTTX(x)		(((x) + 1) & SIP_NTXDESC_MASK)

/*
 * Receive descriptor list size.  We have one Rx buffer per incoming
 * packet, so this logic is a little simpler.
 */
#define	SIP_NRXDESC		64
#define	SIP_NRXDESC_MASK	(SIP_NRXDESC - 1)
#define	SIP_NEXTRX(x)		(((x) + 1) & SIP_NRXDESC_MASK)

/*
 * Control structures are DMA'd to the SiS900 chip.  We allocate them in
 * a single clump that maps to a single DMA segment to make several things
 * easier.
 */
struct sip_control_data {
	/*
	 * The transmit descriptors.
	 */
	struct sip_desc scd_txdescs[SIP_NTXDESC];

	/*
	 * The receive descriptors.
	 */
	struct sip_desc scd_rxdescs[SIP_NRXDESC];
};

#define	SIP_CDOFF(x)	offsetof(struct sip_control_data, x)
#define	SIP_CDTXOFF(x)	SIP_CDOFF(scd_txdescs[(x)])
#define	SIP_CDRXOFF(x)	SIP_CDOFF(scd_rxdescs[(x)])

/*
 * Software state for transmit jobs.
 */
struct sip_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	SIMPLEQ_ENTRY(sip_txsoft) txs_q;
};

SIMPLEQ_HEAD(sip_txsq, sip_txsoft);

/*
 * Software state for receive jobs.
 */
struct sip_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

/*
 * Software state per device.
 */
struct sip_softc {
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* ethernet common data */
	void *sc_sdhook;		/* shutdown hook */

	void *sc_ih;			/* interrupt cookie */

	struct mii_data sc_mii;		/* MII/media information */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define	sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	/*
	 * Software state for transmit and receive descriptors.
	 */
	struct sip_txsoft sc_txsoft[SIP_TXQUEUELEN];
	struct sip_rxsoft sc_rxsoft[SIP_NRXDESC];

	/*
	 * Control data structures.
	 */
	struct sip_control_data *sc_control_data;
#define	sc_txdescs	sc_control_data->scd_txdescs
#define	sc_rxdescs	sc_control_data->scd_rxdescs

	u_int32_t sc_txcfg;		/* prototype TXCFG register */
	u_int32_t sc_rxcfg;		/* prototype RXCFG register */
	u_int32_t sc_imr;		/* prototype IMR register */
	u_int32_t sc_rfcr;		/* prototype RFCR register */

	u_int32_t sc_tx_fill_thresh;	/* transmit fill threshold */
	u_int32_t sc_tx_drain_thresh;	/* transmit drain threshold */

	u_int32_t sc_rx_drain_thresh;	/* receive drain threshold */

	int	sc_flags;		/* misc. flags; see below */

	int	sc_txfree;		/* number of free Tx descriptors */
	int	sc_txnext;		/* next ready Tx descriptor */

	struct sip_txsq sc_txfreeq;	/* free Tx descsofts */
	struct sip_txsq sc_txdirtyq;	/* dirty Tx descsofts */

	int	sc_rxptr;		/* next ready Rx descriptor/descsoft */
};

/* sc_flags */
#define	SIPF_PAUSED	0x00000001	/* paused (802.3x flow control) */

#define	SIP_CDTXADDR(sc, x)	((sc)->sc_cddma + SIP_CDTXOFF((x)))
#define	SIP_CDRXADDR(sc, x)	((sc)->sc_cddma + SIP_CDRXOFF((x)))

#define	SIP_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > SIP_NTXDESC) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    SIP_CDTXOFF(__x), sizeof(struct sip_desc) *		\
		    (SIP_NTXDESC - __x), (ops));			\
		__n -= (SIP_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    SIP_CDTXOFF(__x), sizeof(struct sip_desc) * __n, (ops));	\
} while (0)

#define	SIP_CDRXSYNC(sc, x, ops)					\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    SIP_CDRXOFF((x)), sizeof(struct sip_desc), (ops))

/*
 * Note we rely on MCLBYTES being a power of two below.
 */
#define	SIP_INIT_RXDESC(sc, x)						\
do {									\
	struct sip_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct sip_desc *__sipd = &(sc)->sc_rxdescs[(x)];		\
									\
	__sipd->sipd_link = SIP_CDRXADDR((sc), SIP_NEXTRX((x)));	\
	__sipd->sipd_bufptr = __rxs->rxs_dmamap->dm_segs[0].ds_addr;	\
	__sipd->sipd_cmdsts = CMDSTS_INTR |				\
	    ((MCLBYTES - 1) & CMDSTS_SIZE_MASK);			\
	SIP_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (0)

void	sip_start __P((struct ifnet *));
void	sip_watchdog __P((struct ifnet *));
int	sip_ioctl __P((struct ifnet *, u_long, caddr_t));

void	sip_shutdown __P((void *));

void	sip_reset __P((struct sip_softc *));
int	sip_init __P((struct sip_softc *));
void	sip_stop __P((struct sip_softc *, int));
void	sip_rxdrain __P((struct sip_softc *));
int	sip_add_rxbuf __P((struct sip_softc *, int));
void	sip_read_eeprom __P((struct sip_softc *, int, int, u_int16_t *));
void	sip_set_filter __P((struct sip_softc *));
void	sip_tick __P((void *));

int	sip_intr __P((void *));
void	sip_txintr __P((struct sip_softc *));
void	sip_rxintr __P((struct sip_softc *));

int	sip_mii_readreg __P((struct device *, int, int));
void	sip_mii_writereg __P((struct device *, int, int, int));
void	sip_mii_statchg __P((struct device *));

int	sip_mediachange __P((struct ifnet *));
void	sip_mediastatus __P((struct ifnet *, struct ifmediareq *));

int	sip_match __P((struct device *, struct cfdata *, void *));
void	sip_attach __P((struct device *, struct device *, void *));

int	sip_copy_small = 0;

struct cfattach sip_ca = {
	sizeof(struct sip_softc), sip_match, sip_attach,
};

const struct sip_product *sip_lookup __P((const struct pci_attach_args *));

const struct sip_product *
sip_lookup(pa)
	const struct pci_attach_args *pa;
{
	const struct sip_product *sip;

	for (sip = sip_products; sip->sip_name != NULL; sip++) {
		if (PCI_VENDOR(pa->pa_id) == sip->sip_vendor &&
		    PCI_PRODUCT(pa->pa_id) == sip->sip_product)
			return (sip);
	}
	return (NULL);
}

int
sip_match(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (sip_lookup(pa) != NULL)
		return (1);

	return (0);
}

void
sip_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct sip_softc *sc = (struct sip_softc *) self;
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	bus_space_tag_t iot, memt;
	bus_space_handle_t ioh, memh;
	bus_dma_segment_t seg;
	int ioh_valid, memh_valid;
	int i, rseg, error;
	const struct sip_product *sip;
	pcireg_t pmode;
	u_int16_t enaddr[ETHER_ADDR_LEN / 2];

	sip = sip_lookup(pa);
	if (sip == NULL) {
		printf("\n");
		panic("sip_attach: impossible");
	}

	printf(": %s\n", sip->sip_name);

	/*
	 * Map the device.
	 */
	ioh_valid = (pci_mapreg_map(pa, SIP_PCI_CFGIOA,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
	memh_valid = (pci_mapreg_map(pa, SIP_PCI_CFGMA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);
	
	if (memh_valid) {
		sc->sc_st = memt;
		sc->sc_sh = memh;
	} else if (ioh_valid) {
		sc->sc_st = iot;
		sc->sc_sh = ioh;
	} else {
		printf("%s: unable to map device registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	/* Enable bus mastering. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Get it out of power save mode if needed. */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, 0, 0)) {
		pmode = pci_conf_read(pc, pa->pa_tag, SIP_PCI_CFGPMCSR) & 0x3;
		if (pmode == 3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake up from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		if (pmode != 0) {
			printf("%s: waking up from power state D%d\n",
			    sc->sc_dev.dv_xname, pmode);
			pci_conf_write(pc, pa->pa_tag, SIP_PCI_CFGPMCSR, 0);
		}
	}

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pc, pa->pa_intrtag, pa->pa_intrpin,
	    pa->pa_intrline, &ih)) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, sip_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct sip_control_data), PAGE_SIZE, 0, &seg, 1, &rseg,
	    0)) != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, rseg,
	    sizeof(struct sip_control_data), (caddr_t *)&sc->sc_control_data,
	    BUS_DMA_COHERENT)) != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct sip_control_data), 1,
	    sizeof(struct sip_control_data), 0, 0, &sc->sc_cddmamap)) != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct sip_control_data), NULL,
	    0)) != 0) {
		printf("%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}

	/*
	 * Create the transmit buffer DMA maps.
	 */
	for (i = 0; i < SIP_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    SIP_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			printf("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
	}

	/*
	 * Create the receive buffer DMA maps.
	 */
	for (i = 0; i < SIP_NRXDESC; i++) {
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
	sip_reset(sc);

	/*
	 * Read the Ethernet address from the EEPROM.
	 */
	sip_read_eeprom(sc, SIP_EEPROM_ETHERNET_ID0 >> 1,
	    sizeof(enaddr) / sizeof(enaddr[0]), enaddr);

	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf((u_int8_t *)enaddr));

	/*
	 * Initialize our media structures and probe the MII.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = sip_mii_readreg;
	sc->sc_mii.mii_writereg = sip_mii_writereg;
	sc->sc_mii.mii_statchg = sip_mii_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, sip_mediachange,
	    sip_mediastatus);
	mii_phy_probe(&sc->sc_dev, &sc->sc_mii, 0xffffffff);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = sip_ioctl;
	ifp->if_start = sip_start;
	ifp->if_watchdog = sip_watchdog;

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, (u_int8_t *)enaddr);
#if NBPFILTER > 0
	bpfattach(&sc->sc_ethercom.ec_if.if_bpf, ifp, DLT_EN10MB,
	    sizeof(struct ether_header));
#endif

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(sip_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		printf("%s: WARNING: unable to establish shutdown hook\n",
		    sc->sc_dev.dv_xname);
	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
 fail_5:
	for (i = 0; i < SIP_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_4:
	for (i = 0; i < SIP_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (caddr_t)sc->sc_control_data,
	    sizeof(struct sip_control_data));
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, rseg);
 fail_0:
	return;
}

/*
 * sip_shutdown:
 *
 *	Make sure the interface is stopped at reboot time.
 */
void
sip_shutdown(arg)
	void *arg;
{
	struct sip_softc *sc = arg;

	sip_stop(sc, 1);
}

/*
 * sip_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
void
sip_start(ifp)
	struct ifnet *ifp;
{
	struct sip_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct sip_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, firsttx, nexttx, lasttx, ofree, seg;

	/*
	 * If we've been told to pause, don't transmit any more packets.
	 */
	if (sc->sc_flags & SIPF_PAUSED)
		ifp->if_flags |= IFF_OACTIVE;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors and
	 * the first descriptor we'll use.
	 */
	ofree = sc->sc_txfree;
	firsttx = sc->sc_txnext;

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
		 * didn't fit in the alloted number of segments, or we
		 * were short on resources.  In this case, we'll copy
		 * and try again.
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
			 * packet.  We haven't committed anything yet,
			 * so just unload the DMA map, put the packet
			 * back on the queue, and punt.  Notify the upper
			 * layer that there are not more slots left.
			 *
			 * XXX We could allocate an mbuf and copy, but
			 * XXX is it worth it?
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
		     seg++, nexttx = SIP_NEXTTX(nexttx)) {
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.  That could cause a race condition.
			 * We'll do it below.
			 */
			sc->sc_txdescs[nexttx].sipd_bufptr =
			    dmamap->dm_segs[seg].ds_addr;
			sc->sc_txdescs[nexttx].sipd_cmdsts =
			    (nexttx == firsttx ? 0 : CMDSTS_OWN) |
			    CMDSTS_MORE | dmamap->dm_segs[seg].ds_len;
			lasttx = nexttx;
		}

		/* Clear the MORE bit on the last segment. */
		sc->sc_txdescs[lasttx].sipd_cmdsts &= ~CMDSTS_MORE;

		/* Sync the descriptors we're using. */
		SIP_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
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
		/*
		 * Cause a descriptor interrupt to happen on the
		 * last packet we enqueued.
		 */
		sc->sc_txdescs[lasttx].sipd_cmdsts |= CMDSTS_INTR;
		SIP_CDTXSYNC(sc, lasttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * The entire packet chain is set up.  Give the
		 * first descrptor to the chip now.
		 */
		sc->sc_txdescs[firsttx].sipd_cmdsts |= CMDSTS_OWN;
		SIP_CDTXSYNC(sc, firsttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* Start the transmit process. */
		if ((bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_CR) &
		     CR_TXE) == 0) {
			bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_TXDP,
			    SIP_CDTXADDR(sc, firsttx));
			bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_CR, CR_TXE);
		}

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

/*
 * sip_watchdog:	[ifnet interface function]
 *
 *	Watchdog timer handler.
 */
void
sip_watchdog(ifp)
	struct ifnet *ifp;
{
	struct sip_softc *sc = ifp->if_softc;

	/*
	 * The chip seems to ignore the CMDSTS_INTR bit sometimes!
	 * If we get a timeout, try and sweep up transmit descriptors.
	 * If we manage to sweep them all up, ignore the lack of
	 * interrupt.
	 */
	sip_txintr(sc);

	if (sc->sc_txfree != SIP_NTXDESC) {
		printf("%s: device timeout\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;

		/* Reset the interface. */
		(void) sip_init(sc);
	} else if (ifp->if_flags & IFF_DEBUG)
		printf("%s: recovered from device timeout\n",
		    sc->sc_dev.dv_xname);

	/* Try to get more packets going. */
	sip_start(ifp);
}

/*
 * sip_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
int
sip_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct sip_softc *sc = ifp->if_softc;
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
			if ((error = sip_init(sc)) != 0)
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
				memcpy(LLADDR(ifp->if_sadl),
				    ina->x_host.c_host, ifp->if_addrlen);
			error = sip_init(sc);
			break;
		    }
#endif /* NS */
		default:
			error = sip_init(sc);
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
			sip_stop(sc, 1);
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interfase it marked up and it is stopped, then
			 * start it.
			 */
			error = sip_init(sc);
		} else if ((ifp->if_flags & IFF_UP) != 0) {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect the hardware state.
			 */
			error = sip_init(sc);
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
			sip_set_filter(sc);
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
	sip_start(ifp);

	splx(s);
	return (error);
}

/*
 * sip_intr:
 *
 *	Interrupt service routine.
 */
int
sip_intr(arg)
	void *arg;
{
	struct sip_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int32_t isr;
	int handled = 0;

	for (;;) {
		/* Reading clears interrupt. */
		isr = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_ISR);
		if ((isr & sc->sc_imr) == 0)
			break;

		handled = 1;

		if (isr & (ISR_RXORN|ISR_RXIDLE|ISR_RXDESC)) {
			/* Grab any new packets. */
			sip_rxintr(sc);

			if (isr & ISR_RXORN) {
				printf("%s: receive FIFO overrun\n",
				    sc->sc_dev.dv_xname);

				/* XXX adjust rx_drain_thresh? */
			}

			if (isr & ISR_RXIDLE) {
				printf("%s: receive ring overrun\n",
				    sc->sc_dev.dv_xname);

				/* Get the receive process going again. */
				bus_space_write_4(sc->sc_st, sc->sc_sh,
				    SIP_RXDP, SIP_CDRXADDR(sc, sc->sc_rxptr));
				bus_space_write_4(sc->sc_st, sc->sc_sh,
				    SIP_CR, CR_RXE);
			}
		}

		if (isr & (ISR_TXURN|ISR_TXDESC)) {
			/* Sweep up transmit descriptors. */
			sip_txintr(sc);

			if (isr & ISR_TXURN) {
				u_int32_t thresh;

				printf("%s: transmit FIFO underrun",
				    sc->sc_dev.dv_xname);

				thresh = sc->sc_tx_drain_thresh + 1;
				if (thresh <= TXCFG_DRTH &&
				    (thresh * 32) <= (SIP_TXFIFO_SIZE -
				     (sc->sc_tx_fill_thresh * 32))) {
					printf("; increasing Tx drain "
					    "threshold to %u bytes\n",
					    thresh * 32);
					sc->sc_tx_drain_thresh = thresh;
					(void) sip_init(sc);
				} else {
					(void) sip_init(sc);
					printf("\n");
				}
			}
		}

		if (sc->sc_imr & (ISR_PAUSE_END|ISR_PAUSE_ST)) {
			if (isr & ISR_PAUSE_ST) {
				sc->sc_flags |= SIPF_PAUSED;
				ifp->if_flags |= IFF_OACTIVE;
			}
			if (isr & ISR_PAUSE_END) {
				sc->sc_flags &= ~SIPF_PAUSED;
				ifp->if_flags &= ~IFF_OACTIVE;
			}
		}

		if (isr & ISR_HIBERR) {
#define	PRINTERR(bit, str)						\
			if (isr & (bit))				\
				printf("%s: %s\n", sc->sc_dev.dv_xname, str)
			PRINTERR(ISR_DPERR, "parity error");
			PRINTERR(ISR_SSERR, "system error");
			PRINTERR(ISR_RMABT, "master abort");
			PRINTERR(ISR_RTABT, "target abort");
			PRINTERR(ISR_RXSOVR, "receive status FIFO overrun");
			(void) sip_init(sc);
#undef PRINTERR
		}
	}

	/* Try to get more packets going. */
	sip_start(ifp);

	return (handled);
}

/*
 * sip_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
void
sip_txintr(sc)
	struct sip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct sip_txsoft *txs;
	u_int32_t cmdsts;

	if ((sc->sc_flags & SIPF_PAUSED) == 0)
		ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * Go through our Tx list and free mbufs for those
	 * frames which have been transmitted.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		SIP_CDTXSYNC(sc, txs->txs_firstdesc, txs->txs_dmamap->dm_nsegs,
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		cmdsts = sc->sc_txdescs[txs->txs_lastdesc].sipd_cmdsts;
		if (cmdsts & CMDSTS_OWN)
			break;

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs, txs_q);

		sc->sc_txfree += txs->txs_dmamap->dm_nsegs;

		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;

		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);

		/*
		 * Check for errors and collisions.
		 */
		if (cmdsts &
		    (CMDSTS_Tx_TXA|CMDSTS_Tx_TFU|CMDSTS_Tx_ED|CMDSTS_Tx_EC)) {
			if (ifp->if_flags & IFF_DEBUG) {
				if (CMDSTS_Tx_ED)
					printf("%s: excessive deferral\n",
					    sc->sc_dev.dv_xname);
				if (CMDSTS_Tx_EC) {
					printf("%s: excessive collisions\n",
					    sc->sc_dev.dv_xname);
					ifp->if_collisions += 16;
				}
			}
		} else {
			/* Packet was transmitted successfully. */
			ifp->if_opackets++;
			ifp->if_collisions += CMDSTS_COLLISIONS(cmdsts);
		}
	}

	/*
	 * If there are no more pending transmissions, cancel the watchdog
	 * timer.
	 */
	if (txs == NULL)
		ifp->if_timer = 0;
}

/*
 * sip_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
void
sip_rxintr(sc)
	struct sip_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_header *eh;
	struct sip_rxsoft *rxs;
	struct mbuf *m;
	u_int32_t cmdsts;
	int i, len;

	for (i = sc->sc_rxptr;; i = SIP_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		SIP_CDRXSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		cmdsts = sc->sc_rxdescs[i].sipd_cmdsts;

		/*
		 * NOTE: OWN is set if owned by _consumer_.  We're the
		 * consumer of the receive ring, so if the bit is clear,
		 * we have processed all of the packets.
		 */
		if ((cmdsts & CMDSTS_OWN) == 0) {
			/*
			 * We have processed all of the receive buffers.
			 */
			break;
		}

		/*
		 * If any collisions were seen on the wire, count one.
		 */
		if (cmdsts & CMDSTS_Rx_COL)
			ifp->if_collisions++;

		/*
		 * If an error occurred, update stats, clear the status
		 * word, and leave the packet buffer in place.  It will
		 * simply be reused the next time the ring comes around.
		 */
		if (cmdsts & (CMDSTS_Rx_RXA|CMDSTS_Rx_LONG|CMDSTS_Rx_RUNT|
		    CMDSTS_Rx_ISE|CMDSTS_Rx_CRCE|CMDSTS_Rx_FAE)) {
			ifp->if_ierrors++;
			if ((cmdsts & CMDSTS_Rx_RXA) != 0 &&
			    (cmdsts & CMDSTS_Rx_RXO) == 0) {
				/* Receive overrun handled elsewhere. */
				printf("%s: receive descriptor error\n",
				    sc->sc_dev.dv_xname);
			}
#define	PRINTERR(bit, str)						\
			if (cmdsts & (bit))				\
				printf("%s: %s\n", sc->sc_dev.dv_xname, str)
			PRINTERR(CMDSTS_Rx_LONG, "packet too long");
			PRINTERR(CMDSTS_Rx_RUNT, "runt packet");
			PRINTERR(CMDSTS_Rx_ISE, "invalid symbol error");
			PRINTERR(CMDSTS_Rx_CRCE, "CRC error");
			PRINTERR(CMDSTS_Rx_FAE, "frame alignment error");
#undef PRINTERR
			SIP_INIT_RXDESC(sc, i);
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		/*
		 * No errors; receive the packet.  Note, the SiS 900
		 * includes the CRC with every packet; trim it.
		 */
		len = CMDSTS_SIZE(cmdsts) - ETHER_CRC_LEN;

#ifdef __NO_STRICT_ALIGNMENT
		/*
		 * If the packet is small enough to fit in a
		 * single header mbuf, allocate one and copy
		 * the data into it.  This greatly reduces
		 * memory consumption when we receive lots
		 * of small packets.
		 *
		 * Otherwise, we add a new buffer to the receive
		 * chain.  If this fails, we drop the packet and
		 * recycle the old buffer.
		 */
		if (sip_copy_small != 0 && len <= MHLEN) {
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL)
				goto dropit;
			memcpy(mtod(m, caddr_t),
			    mtod(rxs->rxs_mbuf, caddr_t), len);
			SIP_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize,
			    BUS_DMASYNC_PREREAD);
		} else {
			m = rxs->rxs_mbuf;
			if (sip_add_rxbuf(sc, i) != 0) {
 dropit:
				ifp->if_ierrors++;
				SIP_INIT_RXDESC(sc, i);
				bus_dmamap_sync(sc->sc_dmat,
				    rxs->rxs_dmamap, 0,
				    rxs->rxs_dmamap->dm_mapsize,
				    BUS_DMASYNC_PREREAD);
				continue;
			}
		}
#else
		/*
		 * The SiS 900's receive buffers must be 4-byte aligned.
		 * But this means that the data after the Ethernet header
		 * is misaligned.  We must allocate a new buffer and
		 * copy the data, shifted forward 2 bytes.
		 */
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == NULL) {
 dropit:
			ifp->if_ierrors++;
			SIP_INIT_RXDESC(sc, i);
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
		SIP_INIT_RXDESC(sc, i);
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
		 * pass if up the stack if it's for us.
		 */
		if (ifp->if_bpf) {
			bpf_mtap(ifp->if_bpf, m);
			if ((ifp->if_flags & IFF_PROMISC) != 0 &&
			    (cmdsts & CMDSTS_Rx_DEST) == CMDSTS_Rx_DEST_REJ) {
				m_freem(m);
				continue;
			}
		}
#endif /* NBPFILTER > 0 */

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;
}

/*
 * sip_tick:
 *
 *	One second timer, used to tick the MII.
 */
void
sip_tick(arg)
	void *arg;
{
	struct sip_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	timeout(sip_tick, sc, hz);
}

/*
 * sip_reset:
 *
 *	Perform a soft reset on the SiS 900.
 */
void
sip_reset(sc)
	struct sip_softc *sc;
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	int i;

	bus_space_write_4(st, sh, SIP_CR, CR_RST);

	for (i = 0; i < 1000; i++) {
		if ((bus_space_read_4(st, sh, SIP_ISR) &
		     (ISR_TXRCMP|ISR_RXRCMP)) == (ISR_TXRCMP|ISR_RXRCMP))
			return;
		delay(2);
	}

	printf("%s: reset failed to complete\n", sc->sc_dev.dv_xname);
}

/*
 * sip_init:
 *
 *	Initialize the interface.  Must be called at splnet().
 */
int
sip_init(sc)
	struct sip_softc *sc;
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct sip_txsoft *txs;
	struct sip_rxsoft *rxs;
	struct sip_desc *sipd;
	u_int32_t cfg;
	int i, error = 0;

	/*
	 * Cancel any pending I/O.
	 */
	sip_stop(sc, 0);

	/*
	 * Reset the chip to a known state.
	 */
	sip_reset(sc);

	/*
	 * Initialize the transmit descriptor ring.
	 */
	for (i = 0; i < SIP_NTXDESC; i++) {
		sipd = &sc->sc_txdescs[i];
		memset(sipd, 0, sizeof(struct sip_desc));
		sipd->sipd_link = SIP_CDTXADDR(sc, SIP_NEXTTX(i));
	}
	SIP_CDTXSYNC(sc, 0, SIP_NTXDESC,
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = SIP_NTXDESC;
	sc->sc_txnext = 0;

	/*
	 * Initialize the transmit job descriptors.
	 */
	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);
	for (i = 0; i < SIP_TXQUEUELEN; i++) {
		txs = &sc->sc_txsoft[i];
		txs->txs_mbuf = NULL;
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	/*
	 * Initialize the receive descriptor and receive job
	 * descriptor rings.
	 */
	for (i = 0; i < SIP_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf == NULL) {
			if ((error = sip_add_rxbuf(sc, i)) != 0) {
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				sip_rxdrain(sc);
				goto out;
			}
		}
	}
	sc->sc_rxptr = 0;

	/*
	 * Initialize the configuration register: aggressive PCI
	 * bus request algorithm, default backoff, default OW timer,
	 * default parity error detection.
	 */
	cfg = 0;
#if BYTE_ORDER == BIG_ENDIAN
	/*
	 * ...descriptors in big-endian mode.
	 */
	cfg |= CFG_BEM;
#endif
	bus_space_write_4(st, sh, SIP_CFG, cfg);

	/*
	 * Initialize the transmit fill and drain thresholds if
	 * we have never done so.
	 */
	if (sc->sc_tx_fill_thresh == 0) {
		/*
		 * XXX This value should be tuned.  This is the
		 * minimum (32 bytes), and we may be able to
		 * improve performance by increasing it.
		 */
		sc->sc_tx_fill_thresh = 1;
	}
	if (sc->sc_tx_drain_thresh == 0) {
		/*
		 * Start at a drain threshold of 128 bytes.  We will
		 * increase it if a DMA underrun occurs.
		 *
		 * XXX The minimum value of this variable should be
		 * tuned.  We may be able to improve performance
		 * by starting with a lower value.  That, however,
		 * may trash the first few outgoing packets if the
		 * PCI bus is saturated.
		 */
		sc->sc_tx_drain_thresh = 4;
	}

	/*
	 * Initialize the prototype TXCFG register.
	 */
	sc->sc_txcfg = TXCFG_ATP | TXCFG_MXDMA_512 |
	    (sc->sc_tx_fill_thresh << TXCFG_FLTH_SHIFT) |
	    sc->sc_tx_drain_thresh;
	bus_space_write_4(st, sh, SIP_TXCFG, sc->sc_txcfg);

	/*
	 * Initialize the receive drain threshold if we have never
	 * done so.
	 */
	if (sc->sc_rx_drain_thresh == 0) {
		/*
		 * XXX This value should be tuned.  This is set to the
		 * maximum of 248 bytes, and we may be able to improve
		 * performance by decreasing it (although we should never
		 * set this value lower than 2; 14 bytes are required to
		 * filter the packet).
		 */
		sc->sc_rx_drain_thresh = RXCFG_DRTH >> RXCFG_DRTH_SHIFT;
	}

	/*
	 * Initialize the prototype RXCFG register.
	 */
	sc->sc_rxcfg = RXCFG_MXDMA_512 |
	    (sc->sc_rx_drain_thresh << RXCFG_DRTH_SHIFT);
	bus_space_write_4(st, sh, SIP_RXCFG, sc->sc_rxcfg);

	/* Set up the receive filter. */
	sip_set_filter(sc);

	/*
	 * Give the transmit and receive rings to the chip.
	 */
	bus_space_write_4(st, sh, SIP_TXDP, SIP_CDTXADDR(sc, sc->sc_txnext));
	bus_space_write_4(st, sh, SIP_RXDP, SIP_CDRXADDR(sc, sc->sc_rxptr));

	/*
	 * Initialize the interrupt mask.
	 */
	sc->sc_imr = ISR_DPERR|ISR_SSERR|ISR_RMABT|ISR_RTABT|ISR_RXSOVR|
	    ISR_TXURN|ISR_TXDESC|ISR_RXORN|ISR_RXIDLE|ISR_RXDESC;
	bus_space_write_4(st, sh, SIP_IMR, sc->sc_imr);

	/*
	 * Set the current media.  Do this after initializing the prototype
	 * IMR, since sip_mii_statchg() modifies the IMR for 802.3x flow
	 * control.
	 */
	mii_mediachg(&sc->sc_mii);

	/*
	 * Enable interrupts.
	 */
	bus_space_write_4(st, sh, SIP_IER, IER_IE);

	/*
	 * Start the transmit and receive processes.
	 */
	bus_space_write_4(st, sh, SIP_CR, CR_RXE | CR_TXE);

	/*
	 * Start the one second MII clock.
	 */
	timeout(sip_tick, sc, hz);

	/*
	 * ...all done!
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

 out:
	if (error)
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	return (error);
}

/*
 * sip_drain:
 *
 *	Drain the receive queue.
 */
void
sip_rxdrain(sc)
	struct sip_softc *sc;
{
	struct sip_rxsoft *rxs;
	int i;

	for (i = 0; i < SIP_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/*
 * sip_stop:
 *
 *	Stop transmission on the interface.
 */
void
sip_stop(sc, drain)
	struct sip_softc *sc;
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct sip_txsoft *txs;
	u_int32_t cmdsts = 0;		/* DEBUG */

	/*
	 * Stop the one second clock.
	 */
	untimeout(sip_tick, sc);

	/*
	 * Disable interrupts.
	 */
	bus_space_write_4(st, sh, SIP_IER, 0);

	/*
	 * Stop receiver and transmitter.
	 */
	bus_space_write_4(st, sh, SIP_CR, CR_RXD | CR_TXD);

	/*
	 * Release any queued transmit buffers.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		if ((ifp->if_flags & IFF_DEBUG) != 0 &&
		    SIMPLEQ_NEXT(txs, txs_q) == NULL &&
		    (sc->sc_txdescs[txs->txs_lastdesc].sipd_cmdsts &
		     CMDSTS_INTR) == 0)
			printf("%s: sip_stop: last descriptor does not "
			    "have INTR bit set\n", sc->sc_dev.dv_xname);
		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs, txs_q);
#ifdef DIAGNOSTIC
		if (txs->txs_mbuf == NULL) {
			printf("%s: dirty txsoft with no mbuf chain\n",
			    sc->sc_dev.dv_xname);
			panic("sip_stop");
		}
#endif
		cmdsts |=		/* DEBUG */
		    sc->sc_txdescs[txs->txs_lastdesc].sipd_cmdsts;
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	if (drain) {
		/*
		 * Release the receive buffers.
		 */
		sip_rxdrain(sc);
	}

	/*
	 * Mark the interface down and cancel the watchdog timer.
	 */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	if ((ifp->if_flags & IFF_DEBUG) != 0 &&
	    (cmdsts & CMDSTS_INTR) == 0 && sc->sc_txfree != SIP_NTXDESC)
		printf("%s: sip_stop: no INTR bits set in dirty tx "
		    "descriptors\n", sc->sc_dev.dv_xname);
}

/*
 * sip_read_eeprom:
 *
 *	Read data from the serial EEPROM.
 */
void
sip_read_eeprom(sc, word, wordcnt, data)
	struct sip_softc *sc;
	int word, wordcnt;
	u_int16_t *data;
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	u_int16_t reg;
	int i, x;

	for (i = 0; i < wordcnt; i++) {
		/* Send CHIP SELECT. */
		reg = EROMAR_EECS;
		bus_space_write_4(st, sh, SIP_EROMAR, reg);

		/* Shift in the READ opcode. */
		for (x = 3; x > 0; x--) {
			if (SIP_EEPROM_OPC_READ & (1 << (x - 1)))
				reg |= EROMAR_EEDI;
			else
				reg &= ~EROMAR_EEDI;
			bus_space_write_4(st, sh, SIP_EROMAR, reg);
			bus_space_write_4(st, sh, SIP_EROMAR,
			    reg | EROMAR_EESK);
			delay(4);
			bus_space_write_4(st, sh, SIP_EROMAR, reg);
			delay(4);
		}
		
		/* Shift in address. */
		for (x = 6; x > 0; x--) {
			if ((word + i) & (1 << (x - 1)))
				reg |= EROMAR_EEDI;
			else
				reg &= ~EROMAR_EEDI; 
			bus_space_write_4(st, sh, SIP_EROMAR, reg);
			bus_space_write_4(st, sh, SIP_EROMAR,
			    reg | EROMAR_EESK);
			delay(4);
			bus_space_write_4(st, sh, SIP_EROMAR, reg);
			delay(4);
		}

		/* Shift out data. */
		reg = EROMAR_EECS;
		data[i] = 0;
		for (x = 16; x > 0; x--) {
			bus_space_write_4(st, sh, SIP_EROMAR,
			    reg | EROMAR_EESK);
			delay(4);
			if (bus_space_read_4(st, sh, SIP_EROMAR) & EROMAR_EEDO)
				data[i] |= (1 << (x - 1));
			bus_space_write_4(st, sh, SIP_EROMAR, reg);
		}

		/* Clear CHIP SELECT. */
		bus_space_write_4(st, sh, SIP_EROMAR, 0);
		delay(4);
	}
}

/*
 * sip_add_rxbuf:
 *
 *	Add a receive buffer to the indicated descriptor.
 */
int
sip_add_rxbuf(sc, idx)
	struct sip_softc *sc;
	int idx;
{
	struct sip_rxsoft *rxs = &sc->sc_rxsoft[idx];
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
		panic("sip_add_rxbuf");		/* XXX */
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	SIP_INIT_RXDESC(sc, idx);

	return (0);
}

/*
 * sip_set_filter:
 *
 *	Set up the receive filter.
 */
void
sip_set_filter(sc)
	struct sip_softc *sc;
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int8_t *cp;
	u_int32_t crc, mchash[8];
	int len;
	static const u_int32_t crctab[] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
		0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
		0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
		0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
	};

	/*
	 * Initialize the prototype RFCR.
	 */
	sc->sc_rfcr = RFCR_RFEN;
	if (ifp->if_flags & IFF_BROADCAST)
		sc->sc_rfcr |= RFCR_AAB;
	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_rfcr |= RFCR_AAP;
		goto allmulti;
	}

	/*
	 * Set up the multicast address filter by passing all multicast
	 * addresses through a CRC generator, and then using the high-order
	 * 6 bits as an index into the 128 bit multicast hash table (only
	 * the lower 16 bits of each 32 bit multicast hash register are
	 * valid).  The high order bits select the register, while the
	 * rest of the bits select the bit within the register.
	 */

	memset(mchash, 0, sizeof(mchash));

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

		cp = enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = sizeof(enm->enm_addrlo); --len >= 0;) {
			crc ^= *cp++;
			crc = (crc >> 4) ^ crctab[crc & 0xf];
			crc = (crc >> 4) ^ crctab[crc & 0xf];
		}
		/* Just want the 7 most significant bits. */
		crc >>= 25;

		/* Set the corresponding bit in the hash table. */
		mchash[crc >> 4] |= 1 << (crc & 0xf);

		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;
	goto setit;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	sc->sc_rfcr |= RFCR_AAM;

 setit:
#define	FILTER_EMIT(addr, data)						\
	bus_space_write_4(st, sh, SIP_RFCR, (addr));			\
	bus_space_write_4(st, sh, SIP_RFDR, (data))

	/*
	 * Disable receive filter, and program the node address.
	 */
	cp = LLADDR(ifp->if_sadl);
	FILTER_EMIT(RFCR_RFADDR_NODE0, (cp[1] << 8) | cp[0]);
	FILTER_EMIT(RFCR_RFADDR_NODE2, (cp[3] << 8) | cp[2]);
	FILTER_EMIT(RFCR_RFADDR_NODE4, (cp[5] << 8) | cp[4]);

	if ((ifp->if_flags & IFF_ALLMULTI) == 0) {
		/*
		 * Program the multicast hash table.
		 */
		FILTER_EMIT(RFCR_RFADDR_MC0, mchash[0]);
		FILTER_EMIT(RFCR_RFADDR_MC1, mchash[1]);
		FILTER_EMIT(RFCR_RFADDR_MC2, mchash[2]);
		FILTER_EMIT(RFCR_RFADDR_MC3, mchash[3]);
		FILTER_EMIT(RFCR_RFADDR_MC4, mchash[4]);
		FILTER_EMIT(RFCR_RFADDR_MC5, mchash[5]);
		FILTER_EMIT(RFCR_RFADDR_MC6, mchash[6]);
		FILTER_EMIT(RFCR_RFADDR_MC7, mchash[7]);
	}
#undef FILTER_EMIT

	/*
	 * Re-enable the receiver filter.
	 */
	bus_space_write_4(st, sh, SIP_RFCR, sc->sc_rfcr);
}

/*
 * sip_mii_readreg:	[mii interface function]
 *
 *	Read a PHY register on the MII.
 */
int
sip_mii_readreg(self, phy, reg)
	struct device *self;
	int phy, reg;
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t enphy;

	/*
	 * The SiS 900 has only an internal PHY on the MII.  Only allow
	 * MII address 0.
	 */
	if (phy != 0)
		return (0);

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_ENPHY,
	    (reg << ENPHY_REGADDR_SHIFT) | ENPHY_RWCMD | ENPHY_ACCESS);
	do {
		enphy = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_ENPHY);
	} while (enphy & ENPHY_ACCESS);
	return ((enphy & ENPHY_PHYDATA) >> ENPHY_DATA_SHIFT);
}

/*
 * sip_mii_writereg:	[mii interface function]
 *
 *	Write a PHY register on the MII.
 */
void
sip_mii_writereg(self, phy, reg, val)
	struct device *self;
	int phy, reg, val;
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t enphy;

	/*
	 * The SiS 900 has only an internal PHY on the MII.  Only allow
	 * MII address 0.
	 */
	if (phy != 0)
		return;

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_ENPHY,
	    (val << ENPHY_DATA_SHIFT) | (reg << ENPHY_REGADDR_SHIFT) |
	    ENPHY_ACCESS);
	do {
		enphy = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_ENPHY);
	} while (enphy & ENPHY_ACCESS);
}

/*
 * sip_mii_statchg:	[mii interface function]
 *
 *	Callback from MII layer when media changes.
 */
void
sip_mii_statchg(self)
	struct device *self;
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t flowctl;

	/*
	 * Update TXCFG for full-duplex operation.
	 */
	if ((sc->sc_mii.mii_media_active & IFM_FDX) != 0)
		sc->sc_txcfg |= (TXCFG_CSI | TXCFG_HBI);
	else
		sc->sc_txcfg &= ~(TXCFG_CSI | TXCFG_HBI);

	/*
	 * Update RXCFG for full-duplex or loopback.
	 */
	if ((sc->sc_mii.mii_media_active & IFM_FDX) != 0 ||
	    IFM_SUBTYPE(sc->sc_mii.mii_media_active) == IFM_LOOP)
		sc->sc_rxcfg |= RXCFG_ATX;
	else
		sc->sc_rxcfg &= ~RXCFG_ATX;

	/*
	 * Update IMR for use of 802.3x flow control.
	 */
	if ((sc->sc_mii.mii_media_active & IFM_FLOW) != 0) {
		sc->sc_imr |= (ISR_PAUSE_END|ISR_PAUSE_ST);
		flowctl = FLOWCTL_FLOWEN;
	} else {
		sc->sc_imr &= ~(ISR_PAUSE_END|ISR_PAUSE_ST);
		flowctl = 0;
	}

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_TXCFG, sc->sc_txcfg);
	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_RXCFG, sc->sc_rxcfg);
	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_IMR, sc->sc_imr);
	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_FLOWCTL, flowctl);

	/* XXX Update ifp->if_baudrate */
}

/*
 * sip_mediastatus:	[ifmedia interface function]
 *
 *	Get the current interface media status.
 */
void
sip_mediastatus(ifp, ifmr)
	struct ifnet *ifp;
	struct ifmediareq *ifmr;
{
	struct sip_softc *sc = ifp->if_softc;

	mii_pollstat(&sc->sc_mii);
	ifmr->ifm_status = sc->sc_mii.mii_media_status;
	ifmr->ifm_active = sc->sc_mii.mii_media_active;
}

/*
 * sip_mediachange:	[ifmedia interface function]
 *
 *	Set hardware to newly-selected media.
 */
int
sip_mediachange(ifp)
	struct ifnet *ifp;
{
	struct sip_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return (0);
}
