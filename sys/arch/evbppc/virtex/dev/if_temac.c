/* 	$NetBSD: if_temac.c,v 1.19 2022/02/27 11:49:28 riastradh Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 * Driver for Xilinx LocalLink TEMAC as wired on the GSRD platform.
 *
 * TODO:
 * 	- Optimize
 * 	- Checksum offload
 * 	- Address filters
 * 	- Support jumbo frames
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_temac.c,v 1.19 2022/02/27 11:49:28 riastradh Exp $");


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/cpu.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>

#include <net/bpf.h>

#include <powerpc/ibm4xx/cpu.h>

#include <evbppc/virtex/idcr.h>
#include <evbppc/virtex/dev/xcvbusvar.h>
#include <evbppc/virtex/dev/cdmacreg.h>
#include <evbppc/virtex/dev/temacreg.h>
#include <evbppc/virtex/dev/temacvar.h>

#include <dev/mii/miivar.h>


/* This is outside of TEMAC's DCR window, we have to hardcode it... */
#define DCR_ETH_BASE 		0x0030

#define	TEMAC_REGDEBUG 		0
#define	TEMAC_RXDEBUG 		0
#define	TEMAC_TXDEBUG 		0

#if TEMAC_RXDEBUG > 0 || TEMAC_TXDEBUG > 0
#define	TEMAC_DEBUG 		1
#else
#define	TEMAC_DEBUG 		0
#endif

#if TEMAC_REGDEBUG > 0
#define	TRACEREG(arg) 		printf arg
#else
#define	TRACEREG(arg) 		/* nop */
#endif

/* DMA control chains take up one (16KB) page. */
#define TEMAC_NTXDESC 		256
#define TEMAC_NRXDESC 		256

#define TEMAC_TXQLEN 		64 	/* Software Tx queue length */
#define TEMAC_NTXSEG 		16 	/* Maximum Tx segments per packet */

#define TEMAC_NRXSEG 		1 	/* Maximum Rx segments per packet */
#define TEMAC_RXPERIOD 		1 	/* Interrupt every N descriptors. */
#define TEMAC_RXTIMO_HZ 	100 	/* Rx reaper frequency */

/* Next Tx descriptor and descriptor's offset WRT sc_cdaddr. */
#define TEMAC_TXSINC(n, i) 	(((n) + TEMAC_TXQLEN + (i)) % TEMAC_TXQLEN)
#define TEMAC_TXINC(n, i) 	(((n) + TEMAC_NTXDESC + (i)) % TEMAC_NTXDESC)

#define TEMAC_TXSNEXT(n) 	TEMAC_TXSINC((n), 1)
#define TEMAC_TXNEXT(n) 	TEMAC_TXINC((n), 1)
#define TEMAC_TXDOFF(n) 	(offsetof(struct temac_control, cd_txdesc) + \
				 (n) * sizeof(struct cdmac_descr))

/* Next Rx descriptor and descriptor's offset WRT sc_cdaddr. */
#define TEMAC_RXINC(n, i) 	(((n) + TEMAC_NRXDESC + (i)) % TEMAC_NRXDESC)
#define TEMAC_RXNEXT(n) 	TEMAC_RXINC((n), 1)
#define TEMAC_RXDOFF(n) 	(offsetof(struct temac_control, cd_rxdesc) + \
				 (n) * sizeof(struct cdmac_descr))
#define TEMAC_ISINTR(i) 	(((i) % TEMAC_RXPERIOD) == 0)
#define TEMAC_ISLAST(i) 	((i) == (TEMAC_NRXDESC - 1))


struct temac_control {
	struct cdmac_descr 	cd_txdesc[TEMAC_NTXDESC];
	struct cdmac_descr 	cd_rxdesc[TEMAC_NRXDESC];
};

struct temac_txsoft {
	bus_dmamap_t 		txs_dmap;
	struct mbuf 		*txs_mbuf;
	int 			txs_last;
};

struct temac_rxsoft {
	bus_dmamap_t 		rxs_dmap;
	struct mbuf 		*rxs_mbuf;
};

struct temac_softc {
	device_t 		sc_dev;
	struct ethercom 	sc_ec;
#define sc_if 			sc_ec.ec_if

	/* Peripheral registers */
	bus_space_tag_t 	sc_iot;
	bus_space_handle_t 	sc_ioh;

	/* CDMAC channel registers */
	bus_space_tag_t 	sc_dma_rxt;
	bus_space_handle_t 	sc_dma_rxh; 	/* Rx channel */
	bus_space_handle_t 	sc_dma_rsh; 	/* Rx status */

	bus_space_tag_t 	sc_dma_txt;
	bus_space_handle_t 	sc_dma_txh; 	/* Tx channel */
	bus_space_handle_t 	sc_dma_tsh; 	/* Tx status */

	struct temac_txsoft 	sc_txsoft[TEMAC_TXQLEN];
	struct temac_rxsoft 	sc_rxsoft[TEMAC_NRXDESC];

	struct callout 		sc_rx_timo;
	struct callout 		sc_mii_tick;
	struct mii_data 	sc_mii;

	bus_dmamap_t 		sc_control_dmap;
#define sc_cdaddr 		sc_control_dmap->dm_segs[0].ds_addr

	struct temac_control 	*sc_control_data;
#define sc_rxdescs 		sc_control_data->cd_rxdesc
#define sc_txdescs 		sc_control_data->cd_txdesc

	int 			sc_txbusy;

	int 			sc_txfree;
	int 			sc_txcur;
	int 			sc_txreap;

	int 			sc_rxreap;

	int 			sc_txsfree;
	int 			sc_txscur;
	int 			sc_txsreap;

	int 			sc_dead; 	/* Rx/Tx DMA error (fatal) */
	int 			sc_rx_drained;

	int 			sc_rx_chan;
	int 			sc_tx_chan;

	void 			*sc_sdhook;
	void 			*sc_rx_ih;
	void 			*sc_tx_ih;

	bus_dma_tag_t 		sc_dmat;
};

/* Device interface. */
static void 	temac_attach(device_t, device_t, void *);

/* Ifnet interface. */
static int 	temac_init(struct ifnet *);
static int 	temac_ioctl(struct ifnet *, u_long, void *);
static void 	temac_start(struct ifnet *);
static void 	temac_stop(struct ifnet *, int);

/* Media management. */
static int	temac_mii_readreg(device_t, int, int, uint16_t *);
static void	temac_mii_statchg(struct ifnet *);
static void	temac_mii_tick(void *);
static int	temac_mii_writereg(device_t, int, int, uint16_t);

/* Indirect hooks. */
static void 	temac_shutdown(void *);
static void 	temac_rx_intr(void *);
static void 	temac_tx_intr(void *);

/* Tools. */
static inline void 	temac_rxcdsync(struct temac_softc *, int, int, int);
static inline void 	temac_txcdsync(struct temac_softc *, int, int, int);
static void 		temac_txreap(struct temac_softc *);
static void 		temac_rxreap(struct temac_softc *);
static int 		temac_rxalloc(struct temac_softc *, int, int);
static void 		temac_rxtimo(void *);
static void 		temac_rxdrain(struct temac_softc *);
static void 		temac_reset(struct temac_softc *);
static void 		temac_txkick(struct temac_softc *);

/* Register access. */
static inline void 	gmi_write_8(uint32_t, uint32_t, uint32_t);
static inline void 	gmi_write_4(uint32_t, uint32_t);
static inline void 	gmi_read_8(uint32_t, uint32_t *, uint32_t *);
static inline uint32_t 	gmi_read_4(uint32_t);
static inline int 	hif_wait_stat(uint32_t);

#define cdmac_rx_stat(sc) \
    bus_space_read_4((sc)->sc_dma_rxt, (sc)->sc_dma_rsh, 0 /* XXX hack */)

#define cdmac_rx_reset(sc)						      \
    bus_space_write_4((sc)->sc_dma_rxt, (sc)->sc_dma_rsh, 0, CDMAC_STAT_RESET)

#define cdmac_rx_start(sc, val)						      \
    bus_space_write_4((sc)->sc_dma_rxt, (sc)->sc_dma_rxh, CDMAC_CURDESC, (val))

#define cdmac_tx_stat(sc) \
    bus_space_read_4((sc)->sc_dma_txt, (sc)->sc_dma_tsh, 0 /* XXX hack */)

#define cdmac_tx_reset(sc) \
    bus_space_write_4((sc)->sc_dma_txt, (sc)->sc_dma_tsh, 0, CDMAC_STAT_RESET)

#define cdmac_tx_start(sc, val)						      \
    bus_space_write_4((sc)->sc_dma_txt, (sc)->sc_dma_txh, CDMAC_CURDESC, (val))


CFATTACH_DECL_NEW(temac, sizeof(struct temac_softc),
    xcvbus_child_match, temac_attach, NULL, NULL);


/*
 * Private bus utilities.
 */
static inline int
hif_wait_stat(uint32_t mask)
{
	int 			i = 0;
	int			rv = 0;

	while (mask != (mfidcr(IDCR_HIF_STAT) & mask)) {
		if (i++ > 100) {
			printf("%s: timeout waiting for 0x%08x\n",
			    __func__, mask);
			rv = ETIMEDOUT;
			break;
		}
		delay(5);
	}

	TRACEREG(("%s: stat %#08x loops %d\n", __func__, mask, i));
	return rv;
}

static inline void
gmi_write_4(uint32_t addr, uint32_t lo)
{
	mtidcr(IDCR_HIF_ARG0, lo);
	mtidcr(IDCR_HIF_CTRL, (addr & HIF_CTRL_GMIADDR) | HIF_CTRL_WRITE);
	hif_wait_stat(HIF_STAT_GMIWR);

	TRACEREG(("%s: %#08x <- %#08x\n", __func__, addr, lo));
}

static inline void __unused
gmi_write_8(uint32_t addr, uint32_t lo, uint32_t hi)
{
	mtidcr(IDCR_HIF_ARG1, hi);
	gmi_write_4(addr, lo);
}

static inline void __unused
gmi_read_8(uint32_t addr, uint32_t *lo, uint32_t *hi)
{
	*lo = gmi_read_4(addr);
	*hi = mfidcr(IDCR_HIF_ARG1);
}

static inline uint32_t
gmi_read_4(uint32_t addr)
{
	uint32_t 		res;

	mtidcr(IDCR_HIF_CTRL, addr & HIF_CTRL_GMIADDR);
	hif_wait_stat(HIF_STAT_GMIRR);

	res = mfidcr(IDCR_HIF_ARG0);
	TRACEREG(("%s:  %#08x -> %#08x\n", __func__, addr, res));
	return (res);
}

/*
 * Generic device.
 */
static void
temac_attach(device_t parent, device_t self, void *aux)
{
	struct xcvbus_attach_args *vaa = aux;
	struct ll_dmac 		*rx = vaa->vaa_rx_dmac;
	struct ll_dmac 		*tx = vaa->vaa_tx_dmac;
	struct temac_softc 	*sc = device_private(self);
	struct ifnet 		*ifp = &sc->sc_if;
	struct mii_data 	*mii = &sc->sc_mii;
	uint8_t 		enaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t 	seg;
	int 			error, nseg, i;
	const char * const xname = device_xname(self);

	aprint_normal(": TEMAC\n"); 	/* XXX will be LL_TEMAC, PLB_TEMAC */

	KASSERT(rx);
	KASSERT(tx);

	sc->sc_dev = self;
	sc->sc_dmat = vaa->vaa_dmat;
	sc->sc_dead = 0;
	sc->sc_rx_drained = 1;
	sc->sc_txbusy = 0;
	sc->sc_iot = vaa->vaa_iot;
	sc->sc_dma_rxt = rx->dmac_iot;
	sc->sc_dma_txt = tx->dmac_iot;

	/*
	 * Map HIF and receive/transmit dmac registers.
	 */
	if ((error = bus_space_map(vaa->vaa_iot, vaa->vaa_addr, TEMAC_SIZE, 0,
	    &sc->sc_ioh)) != 0) {
		aprint_error_dev(self, "could not map registers\n");
		goto fail_0;
	}

	if ((error = bus_space_map(sc->sc_dma_rxt, rx->dmac_ctrl_addr,
	    CDMAC_CTRL_SIZE, 0, &sc->sc_dma_rxh)) != 0) {
		aprint_error_dev(self, "could not map Rx control registers\n");
		goto fail_0;
	}
	if ((error = bus_space_map(sc->sc_dma_rxt, rx->dmac_stat_addr,
	    CDMAC_STAT_SIZE, 0, &sc->sc_dma_rsh)) != 0) {
		aprint_error_dev(self, "could not map Rx status register\n");
		goto fail_0;
	}

	if ((error = bus_space_map(sc->sc_dma_txt, tx->dmac_ctrl_addr,
	    CDMAC_CTRL_SIZE, 0, &sc->sc_dma_txh)) != 0) {
		aprint_error_dev(self, "could not map Tx control registers\n");
		goto fail_0;
	}
	if ((error = bus_space_map(sc->sc_dma_txt, tx->dmac_stat_addr,
	    CDMAC_STAT_SIZE, 0, &sc->sc_dma_tsh)) != 0) {
		aprint_error_dev(self, "could not map Tx status register\n");
		goto fail_0;
	}

	/*
	 * Allocate and initialize DMA control chains.
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct temac_control), 8, 0, &seg, 1, &nseg, 0)) != 0) {
	    	aprint_error_dev(self, "could not allocate control data\n");
		goto fail_0;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct temac_control),
	    (void **)&sc->sc_control_data, BUS_DMA_COHERENT)) != 0) {
	    	aprint_error_dev(self, "could not map control data\n");
		goto fail_1;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct temac_control), 1,
	    sizeof(struct temac_control), 0, 0, &sc->sc_control_dmap)) != 0) {
	    	aprint_error_dev(self,
		    "could not create control data DMA map\n");
		goto fail_2;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_control_dmap,
	    sc->sc_control_data, sizeof(struct temac_control), NULL, 0)) != 0) {
	    	aprint_error_dev(self, "could not load control data DMA map\n");
		goto fail_3;
	}

	/*
	 * Link descriptor chains.
	 */
	memset(sc->sc_control_data, 0, sizeof(struct temac_control));

	for (i = 0; i < TEMAC_NTXDESC; i++) {
		sc->sc_txdescs[i].desc_next = sc->sc_cdaddr +
		    TEMAC_TXDOFF(TEMAC_TXNEXT(i));
		sc->sc_txdescs[i].desc_stat = CDMAC_STAT_DONE;
	}
	for (i = 0; i < TEMAC_NRXDESC; i++) {
		sc->sc_rxdescs[i].desc_next = sc->sc_cdaddr +
		    TEMAC_RXDOFF(TEMAC_RXNEXT(i));
		sc->sc_txdescs[i].desc_stat = CDMAC_STAT_DONE;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_control_dmap, 0,
	    sizeof(struct temac_control),
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	/*
	 * Initialize software state for transmit/receive jobs.
	 */
	for (i = 0; i < TEMAC_TXQLEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat,
		    ETHER_MAX_LEN_JUMBO, TEMAC_NTXSEG, ETHER_MAX_LEN_JUMBO,
		    0, 0, &sc->sc_txsoft[i].txs_dmap)) != 0) {
		    	aprint_error_dev(self,
			    "could not create Tx DMA map %d\n",
		    	    i);
			goto fail_4;
		}
		sc->sc_txsoft[i].txs_mbuf = NULL;
		sc->sc_txsoft[i].txs_last = 0;
	}

	for (i = 0; i < TEMAC_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat,
		    MCLBYTES, TEMAC_NRXSEG, MCLBYTES, 0, 0,
		    &sc->sc_rxsoft[i].rxs_dmap)) != 0) {
		    	aprint_error_dev(self,
			    "could not create Rx DMA map %d\n", i);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	/*
	 * Setup transfer interrupt handlers.
	 */
	error = ENOMEM;

	sc->sc_rx_ih = ll_dmac_intr_establish(rx->dmac_chan,
	    temac_rx_intr, sc);
	if (sc->sc_rx_ih == NULL) {
		aprint_error_dev(self, "could not establish Rx interrupt\n");
		goto fail_5;
	}

	sc->sc_tx_ih = ll_dmac_intr_establish(tx->dmac_chan,
	    temac_tx_intr, sc);
	if (sc->sc_tx_ih == NULL) {
		aprint_error_dev(self, "could not establish Tx interrupt\n");
		goto fail_6;
	}

	/* XXXFreza: faked, should read unicast address filter. */
	enaddr[0] = 0x00;
	enaddr[1] = 0x11;
	enaddr[2] = 0x17;
	enaddr[3] = 0xff;
	enaddr[4] = 0xff;
	enaddr[5] = 0x01;

	/*
	 * Initialize the TEMAC.
	 */
	temac_reset(sc);

	/* Configure MDIO link. */
	gmi_write_4(TEMAC_GMI_MGMTCF, GMI_MGMT_CLKDIV_100MHz | GMI_MGMT_MDIO);

	/* Initialize PHY. */
	mii->mii_ifp = ifp;
	mii->mii_readreg = temac_mii_readreg;
	mii->mii_writereg = temac_mii_writereg;
	mii->mii_statchg = temac_mii_statchg;
	sc->sc_ec.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);

	mii_attach(sc->sc_dev, mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_NONE);
	} else {
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);
	}

	/* Hold PHY in reset. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TEMAC_RESET, TEMAC_RESET_PHY);

	/* Reset EMAC. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TEMAC_RESET,
	    TEMAC_RESET_EMAC);
	delay(10000);

	/* Reset peripheral, awakes PHY and EMAC. */
	bus_space_write_4(sc->sc_iot, sc->sc_ioh, TEMAC_RESET,
	    TEMAC_RESET_PERIPH);
	delay(40000);

	/* (Re-)Configure MDIO link. */
	gmi_write_4(TEMAC_GMI_MGMTCF, GMI_MGMT_CLKDIV_100MHz | GMI_MGMT_MDIO);

	/*
	 * Hook up with network stack.
	 */
	strcpy(ifp->if_xname, xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = temac_ioctl;
	ifp->if_start = temac_start;
	ifp->if_init = temac_init;
	ifp->if_stop = temac_stop;
	ifp->if_watchdog = NULL;
	IFQ_SET_READY(&ifp->if_snd);
	IFQ_SET_MAXLEN(&ifp->if_snd, TEMAC_TXQLEN);

	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	sc->sc_sdhook = shutdownhook_establish(temac_shutdown, sc);
	if (sc->sc_sdhook == NULL)
		aprint_error_dev(self,
		    "WARNING: unable to establish shutdown hook\n");

	callout_setfunc(&sc->sc_mii_tick, temac_mii_tick, sc);
	callout_setfunc(&sc->sc_rx_timo, temac_rxtimo, sc);

	return ;

 fail_6:
	ll_dmac_intr_disestablish(rx->dmac_chan, sc->sc_rx_ih);
	i = TEMAC_NRXDESC;
 fail_5:
 	for (--i; i >= 0; i--)
 		bus_dmamap_destroy(sc->sc_dmat, sc->sc_rxsoft[i].rxs_dmap);
	i = TEMAC_TXQLEN;
 fail_4:
 	for (--i; i >= 0; i--)
 		bus_dmamap_destroy(sc->sc_dmat, sc->sc_txsoft[i].txs_dmap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_control_dmap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct temac_control));
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, nseg);
 fail_0:
 	aprint_error_dev(self, "error = %d\n", error);
}

/*
 * Network device.
 */
static int
temac_init(struct ifnet *ifp)
{
	struct temac_softc 	*sc = (struct temac_softc *)ifp->if_softc;
	uint32_t 		rcr, tcr;
	int 			i, error;

	/* Reset DMA channels. */
	cdmac_tx_reset(sc);
	cdmac_rx_reset(sc);

	/* Set current media. */
	if ((error = ether_mediachange(ifp)) != 0)
		return error;

	callout_schedule(&sc->sc_mii_tick, hz);

	/* Enable EMAC engine. */
	rcr = (gmi_read_4(TEMAC_GMI_RXCF1) | GMI_RX_ENABLE) &
	    ~(GMI_RX_JUMBO | GMI_RX_FCS);
	gmi_write_4(TEMAC_GMI_RXCF1, rcr);

	tcr = (gmi_read_4(TEMAC_GMI_TXCF) | GMI_TX_ENABLE) &
	    ~(GMI_TX_JUMBO | GMI_TX_FCS);
	gmi_write_4(TEMAC_GMI_TXCF, tcr);

	/* XXXFreza: Force promiscuous mode, for now. */
	gmi_write_4(TEMAC_GMI_AFM, GMI_AFM_PROMISC);
	ifp->if_flags |= IFF_PROMISC;

	/* Rx/Tx queues are drained -- either from attach() or stop(). */
	sc->sc_txsfree = TEMAC_TXQLEN;
	sc->sc_txsreap = 0;
	sc->sc_txscur = 0;

	sc->sc_txfree = TEMAC_NTXDESC;
	sc->sc_txreap = 0;
	sc->sc_txcur = 0;

	sc->sc_rxreap = 0;

	/* Allocate and map receive buffers. */
	if (sc->sc_rx_drained) {
		for (i = 0; i < TEMAC_NRXDESC; i++) {
			if ((error = temac_rxalloc(sc, i, 1)) != 0) {
				aprint_error_dev(sc->sc_dev,
				    "failed to allocate Rx descriptor %d\n",
				    i);
				temac_rxdrain(sc);
				return (error);
			}
		}
		sc->sc_rx_drained = 0;

		temac_rxcdsync(sc, 0, TEMAC_NRXDESC,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
		cdmac_rx_start(sc, sc->sc_cdaddr + TEMAC_RXDOFF(0));
	}

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	return (0);
}

static int
temac_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct temac_softc 	*sc = (struct temac_softc *)ifp->if_softc;
	int 			s, ret;

	s = splnet();
	if (sc->sc_dead)
		ret = EIO;
	else
		ret = ether_ioctl(ifp, cmd, data);
	splx(s);
	return (ret);
}

static void
temac_start(struct ifnet *ifp)
{
	struct temac_softc 	*sc = (struct temac_softc *)ifp->if_softc;
	struct temac_txsoft 	*txs;
	struct mbuf 		*m;
	bus_dmamap_t 		dmap;
	int 			error, head, nsegs, i;

	nsegs = 0;
	head = sc->sc_txcur;
	txs = NULL; 		/* gcc */

	if (sc->sc_dead)
		return;

	KASSERT(sc->sc_txfree >= 0);
	KASSERT(sc->sc_txsfree >= 0);

	/*
	 * Push mbufs into descriptor chain until we drain the interface
	 * queue or run out of descriptors. We'll mark the first segment
	 * as "done" in hope that we might put CDMAC interrupt above IPL_NET
	 * and have it start jobs & mark packets for GC preemtively for
	 * us -- creativity due to limitations in CDMAC transfer engine
	 * (it really consumes lists, not circular queues, AFAICS).
	 *
	 * We schedule one interrupt per Tx batch.
	 */
	while (1) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (sc->sc_txsfree == 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		txs = &sc->sc_txsoft[sc->sc_txscur];
		dmap = txs->txs_dmap;

		if (txs->txs_mbuf != NULL)
			printf("FOO\n");
		if (txs->txs_last)
			printf("BAR\n");

		if ((error = bus_dmamap_load_mbuf(sc->sc_dmat, dmap, m,
		    BUS_DMA_WRITE | BUS_DMA_NOWAIT)) != 0) {
		    	if (error == EFBIG) {
		    		aprint_error_dev(sc->sc_dev,
				    "Tx consumes too many segments, dropped\n");
				IFQ_DEQUEUE(&ifp->if_snd, m);
				m_freem(m);
				continue;
		    	} else {
		    		aprint_debug_dev(sc->sc_dev,
				    "Tx stall due to resource shortage\n");
		    		break;
			}
		}

		/*
		 * If we're short on DMA descriptors, notify upper layers
		 * and leave this packet for later.
		 */
		if (dmap->dm_nsegs > sc->sc_txfree) {
			bus_dmamap_unload(sc->sc_dmat, dmap);
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);

		bus_dmamap_sync(sc->sc_dmat, dmap, 0, dmap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);
		txs->txs_mbuf = m;

		/*
		 * Map the packet into descriptor chain. XXX We'll want
		 * to fill checksum offload commands here.
		 *
		 * We would be in a race if we weren't blocking CDMAC intr
		 * at this point -- we need to be locked against txreap()
		 * because of dmasync ops.
		 */

		temac_txcdsync(sc, sc->sc_txcur, dmap->dm_nsegs,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		for (i = 0; i < dmap->dm_nsegs; i++) {
			sc->sc_txdescs[sc->sc_txcur].desc_addr =
			    dmap->dm_segs[i].ds_addr;
			sc->sc_txdescs[sc->sc_txcur].desc_size =
			    dmap->dm_segs[i].ds_len;
			sc->sc_txdescs[sc->sc_txcur].desc_stat =
			    (i == 0 			? CDMAC_STAT_SOP : 0) |
			    (i == (dmap->dm_nsegs - 1) 	? CDMAC_STAT_EOP : 0);

			sc->sc_txcur = TEMAC_TXNEXT(sc->sc_txcur);
		}

		sc->sc_txfree -= dmap->dm_nsegs;
		nsegs += dmap->dm_nsegs;

		sc->sc_txscur = TEMAC_TXSNEXT(sc->sc_txscur);
		sc->sc_txsfree--;
	}

	/* Get data running if we queued any. */
	if (nsegs > 0) {
		int 		tail = TEMAC_TXINC(sc->sc_txcur, -1);

		/* Mark the last packet in this job. */
		txs->txs_last = 1;

		/* Mark the last descriptor in this job. */
		sc->sc_txdescs[tail].desc_stat |= CDMAC_STAT_STOP |
		    CDMAC_STAT_INTR;
		temac_txcdsync(sc, head, nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		temac_txkick(sc);
#if TEMAC_TXDEBUG > 0
		aprint_debug_dev(sc->sc_dev,
		    "start:  txcur  %03d -> %03d, nseg %03d\n",
		    head, sc->sc_txcur, nsegs);
#endif
	}
}

static void
temac_stop(struct ifnet *ifp, int disable)
{
	struct temac_softc 	*sc = (struct temac_softc *)ifp->if_softc;
	struct temac_txsoft 	*txs;
	int 			i;

#if TEMAC_DEBUG > 0
	aprint_debug_dev(sc->sc_dev, "stop\n");
#endif

	/* Down the MII. */
	callout_stop(&sc->sc_mii_tick);
	mii_down(&sc->sc_mii);

	/* Stop the engine. */
	temac_reset(sc);

	/* Drain buffers queues (unconditionally). */
	temac_rxdrain(sc);

	for (i = 0; i < TEMAC_TXQLEN; i++) {
		txs = &sc->sc_txsoft[i];

		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
			txs->txs_last = 0;
		}
	}
	sc->sc_txbusy = 0;

	/* Acknowledge we're down. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
}

static int
temac_mii_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	int rv;

	mtidcr(IDCR_HIF_ARG0, (phy << 5) | reg);
	mtidcr(IDCR_HIF_CTRL, TEMAC_GMI_MII_ADDR);

	if ((rv = hif_wait_stat(HIF_STAT_MIIRR)) != 0)
		return rv;

	*val = mfidcr(IDCR_HIF_ARG0) & 0xffff;
	return 0;
}

static int
temac_mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	mtidcr(IDCR_HIF_ARG0, val);
	mtidcr(IDCR_HIF_CTRL, TEMAC_GMI_MII_WRVAL | HIF_CTRL_WRITE);
	mtidcr(IDCR_HIF_ARG0, (phy << 5) | reg);
	mtidcr(IDCR_HIF_CTRL, TEMAC_GMI_MII_ADDR | HIF_CTRL_WRITE);
	return hif_wait_stat(HIF_STAT_MIIWR);
}

static void
temac_mii_statchg(struct ifnet *ifp)
{
	struct temac_softc 	*sc = ifp->if_softc;
	uint32_t 		rcf, tcf, mmc;

	/* Full/half duplex link. */
	rcf = gmi_read_4(TEMAC_GMI_RXCF1);
	tcf = gmi_read_4(TEMAC_GMI_TXCF);

	if (sc->sc_mii.mii_media_active & IFM_FDX) {
		gmi_write_4(TEMAC_GMI_RXCF1, rcf & ~GMI_RX_HDX);
		gmi_write_4(TEMAC_GMI_TXCF, tcf & ~GMI_TX_HDX);
	} else {
		gmi_write_4(TEMAC_GMI_RXCF1, rcf | GMI_RX_HDX);
		gmi_write_4(TEMAC_GMI_TXCF, tcf | GMI_TX_HDX);
	}

	/* Link speed. */
	mmc = gmi_read_4(TEMAC_GMI_MMC) & ~GMI_MMC_SPEED_MASK;

	switch (IFM_SUBTYPE(sc->sc_mii.mii_media_active)) {
	case IFM_10_T:
		/*
		 * XXXFreza: the GMAC is not happy with 10Mbit ethernet,
		 * although the documentation claims it's supported. Maybe
		 * it's just my equipment...
		 */
		mmc |= GMI_MMC_SPEED_10;
		break;
	case IFM_100_TX:
		mmc |= GMI_MMC_SPEED_100;
		break;
	case IFM_1000_T:
		mmc |= GMI_MMC_SPEED_1000;
		break;
	}

	gmi_write_4(TEMAC_GMI_MMC, mmc);
}

static void
temac_mii_tick(void *arg)
{
	struct temac_softc 	*sc = (struct temac_softc *)arg;
	int 			s;

	if (!device_is_active(sc->sc_dev))
		return;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_schedule(&sc->sc_mii_tick, hz);
}

/*
 * External hooks.
 */
static void
temac_shutdown(void *arg)
{
	struct temac_softc 	*sc = (struct temac_softc *)arg;

	temac_reset(sc);
}

static void
temac_tx_intr(void *arg)
{
	struct temac_softc 	*sc = (struct temac_softc *)arg;
	uint32_t 		stat;

	/* XXX: We may need to splnet() here if cdmac(4) changes. */

	if ((stat = cdmac_tx_stat(sc)) & CDMAC_STAT_ERROR) {
		aprint_error_dev(sc->sc_dev,
		    "transmit DMA is toast (%#08x), halted!\n",
		    stat);

		/* XXXFreza: how to signal this upstream? */
		temac_stop(&sc->sc_if, 1);
		sc->sc_dead = 1;
	}

#if TEMAC_DEBUG > 0
	aprint_debug_dev(sc->sc_dev, "tx intr 0x%08x\n", stat);
#endif
	temac_txreap(sc);
}

static void
temac_rx_intr(void *arg)
{
	struct temac_softc 	*sc = (struct temac_softc *)arg;
	uint32_t 		stat;

	/* XXX: We may need to splnet() here if cdmac(4) changes. */

	if ((stat = cdmac_rx_stat(sc)) & CDMAC_STAT_ERROR) {
		aprint_error_dev(sc->sc_dev,
		    "receive DMA is toast (%#08x), halted!\n",
		    stat);

		/* XXXFreza: how to signal this upstream? */
		temac_stop(&sc->sc_if, 1);
		sc->sc_dead = 1;
	}

#if TEMAC_DEBUG > 0
	aprint_debug_dev(sc->sc_dev, "rx intr 0x%08x\n", stat);
#endif
	temac_rxreap(sc);
}

/*
 * Utils.
 */
static inline void
temac_txcdsync(struct temac_softc *sc, int first, int cnt, int flag)
{
	if ((first + cnt) > TEMAC_NTXDESC) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_control_dmap,
		    TEMAC_TXDOFF(first),
		    sizeof(struct cdmac_descr) * (TEMAC_NTXDESC - first),
		    flag);
		cnt = (first + cnt) % TEMAC_NTXDESC;
		first = 0;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_control_dmap,
	    TEMAC_TXDOFF(first),
	    sizeof(struct cdmac_descr) * cnt,
	    flag);
}

static inline void
temac_rxcdsync(struct temac_softc *sc, int first, int cnt, int flag)
{
	if ((first + cnt) > TEMAC_NRXDESC) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_control_dmap,
		    TEMAC_RXDOFF(first),
		    sizeof(struct cdmac_descr) * (TEMAC_NRXDESC - first),
		    flag);
		cnt = (first + cnt) % TEMAC_NRXDESC;
		first = 0;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_control_dmap,
	    TEMAC_RXDOFF(first),
	    sizeof(struct cdmac_descr) * cnt,
	    flag);
}

static void
temac_txreap(struct temac_softc *sc)
{
	struct temac_txsoft 	*txs;
	bus_dmamap_t 		dmap;
	int 			sent = 0;

	/*
	 * Transmit interrupts happen on the last descriptor of Tx jobs.
	 * Hence, every time we're called (and we assume txintr is our
	 * only caller!), we reap packets upto and including the one
	 * marked as last-in-batch.
	 *
	 * XXX we rely on that we make EXACTLY one batch per intr, no more
	 */
	while (sc->sc_txsfree != TEMAC_TXQLEN) {
		txs = &sc->sc_txsoft[sc->sc_txsreap];
		dmap = txs->txs_dmap;

		sc->sc_txreap = TEMAC_TXINC(sc->sc_txreap, dmap->dm_nsegs);
		sc->sc_txfree += dmap->dm_nsegs;

		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;

		if_statinc(&sc->sc_if, if_opackets);
		sent = 1;

		sc->sc_txsreap = TEMAC_TXSNEXT(sc->sc_txsreap);
		sc->sc_txsfree++;

		if (txs->txs_last) {
			txs->txs_last = 0;
			sc->sc_txbusy = 0; 	/* channel stopped now */

			temac_txkick(sc);
			break;
		}
	}

	if (sent && (sc->sc_if.if_flags & IFF_OACTIVE))
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
}

static int
temac_rxalloc(struct temac_softc *sc, int which, int verbose)
{
	struct temac_rxsoft 	*rxs;
	struct mbuf 		*m;
	uint32_t 		stat;
	int 			error;

	rxs = &sc->sc_rxsoft[which];

	/* The mbuf itself is not our problem, just clear DMA related stuff. */
	if (rxs->rxs_mbuf != NULL) {
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmap);
		rxs->rxs_mbuf = NULL;
	}

	/*
	 * We would like to store mbuf and dmap in application specific
	 * fields of the descriptor, but that doesn't work for Rx. Shame
	 * on Xilinx for this (and for the useless timer architecture).
	 *
	 * Hence each descriptor needs its own soft state. We may want
	 * to merge multiple rxs's into a monster mbuf when we support
	 * jumbo frames though. Also, we use single set of indexing
	 * variables for both sc_rxdescs[] and sc_rxsoft[].
	 */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		if (verbose)
			aprint_debug_dev(sc->sc_dev,
			    "out of Rx header mbufs\n");
		return (ENOBUFS);
	}
	MCLAIM(m, &sc->sc_ec.ec_rx_mowner);

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		if (verbose)
			aprint_debug_dev(sc->sc_dev,
			    "out of Rx cluster mbufs\n");
		m_freem(m);
		return (ENOBUFS);
	}

	rxs->rxs_mbuf = m;
	m->m_pkthdr.len = m->m_len = MCLBYTES;

	/* Make sure the payload after ethernet header is 4-aligned. */
	m_adj(m, 2);

	error = bus_dmamap_load_mbuf(sc->sc_dmat, rxs->rxs_dmap, m,
	    BUS_DMA_NOWAIT);
	if (error) {
		if (verbose)
			aprint_debug_dev(sc->sc_dev,
			    "could not map Rx descriptor %d, error = %d\n",
			    which, error);

		rxs->rxs_mbuf = NULL;
		m_freem(m);

		return (error);
	}

	stat =
	    (TEMAC_ISINTR(which) ? CDMAC_STAT_INTR : 0) |
	    (TEMAC_ISLAST(which) ? CDMAC_STAT_STOP : 0);

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmap, 0,
	    rxs->rxs_dmap->dm_mapsize, BUS_DMASYNC_PREREAD);

	/* Descriptor post-sync, if needed, left to the caller. */

	sc->sc_rxdescs[which].desc_addr = rxs->rxs_dmap->dm_segs[0].ds_addr;
	sc->sc_rxdescs[which].desc_size  = rxs->rxs_dmap->dm_segs[0].ds_len;
	sc->sc_rxdescs[which].desc_stat = stat;

	/* Descriptor pre-sync, if needed, left to the caller. */

	return (0);
}

static void
temac_rxreap(struct temac_softc *sc)
{
	struct ifnet 		*ifp = &sc->sc_if;
	uint32_t 		stat, rxstat, rxsize;
	struct mbuf 		*m;
	int 			nseg, head, tail;

	head = sc->sc_rxreap;
	tail = 0; 		/* gcc */
	nseg = 0;

	/*
	 * Collect finished entries on the Rx list, kick DMA if we hit
	 * the end. DMA will always stop on the last descriptor in chain,
	 * so it will never hit a reap-in-progress descriptor.
	 */
	while (1) {
		/* Maybe we previously failed to refresh this one? */
		if (sc->sc_rxsoft[sc->sc_rxreap].rxs_mbuf == NULL) {
			if (temac_rxalloc(sc, sc->sc_rxreap, 0) != 0)
				break;

			sc->sc_rxreap = TEMAC_RXNEXT(sc->sc_rxreap);
			continue;
		}
		temac_rxcdsync(sc, sc->sc_rxreap, 1,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		stat = sc->sc_rxdescs[sc->sc_rxreap].desc_stat;
		m = NULL;

		if ((stat & CDMAC_STAT_DONE) == 0)
			break;

		/*
		 * Count any descriptor we've collected, regardless of status.
		 */
		nseg ++;

		/* XXXFreza: This won't work for jumbo frames. */

		if ((stat & (CDMAC_STAT_EOP | CDMAC_STAT_SOP)) !=
		    (CDMAC_STAT_EOP | CDMAC_STAT_SOP)) {
		    	aprint_error_dev(sc->sc_dev,
			    "Rx packet doesn't fit in one descriptor, "
			    "stat = %#08x\n", stat);
			goto badframe;
		}

		/* Dissect TEMAC footer if this is end of packet. */
		rxstat = sc->sc_rxdescs[sc->sc_rxreap].desc_rxstat;
		rxsize = sc->sc_rxdescs[sc->sc_rxreap].desc_rxsize &
		    RXSIZE_MASK;

		if ((rxstat & RXSTAT_GOOD) == 0 ||
		    (rxstat & RXSTAT_SICK) != 0) {
		    	aprint_error_dev(sc->sc_dev,
			    "corrupt Rx packet, rxstat = %#08x\n",
		    	    rxstat);
			goto badframe;
		}

		/* We are now bound to succeed. */
		bus_dmamap_sync(sc->sc_dmat,
		    sc->sc_rxsoft[sc->sc_rxreap].rxs_dmap, 0,
		    sc->sc_rxsoft[sc->sc_rxreap].rxs_dmap->dm_mapsize,
		    BUS_DMASYNC_POSTREAD);

		m = sc->sc_rxsoft[sc->sc_rxreap].rxs_mbuf;
		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = rxsize;

 badframe:
 		/* Get ready for more work. */
		tail = sc->sc_rxreap;
		sc->sc_rxreap = TEMAC_RXNEXT(sc->sc_rxreap);

 		/* On failures we reuse the descriptor and go ahead. */
 		if (m == NULL) {
			sc->sc_rxdescs[tail].desc_stat =
			    (TEMAC_ISINTR(tail) ? CDMAC_STAT_INTR : 0) |
			    (TEMAC_ISLAST(tail) ? CDMAC_STAT_STOP : 0);

			if_statinc(ifp, if_ierrors);
			continue;
 		}

		if_percpuq_enqueue(ifp->if_percpuq, m);

		/* Refresh descriptor, bail out if we're out of buffers. */
		if (temac_rxalloc(sc, tail, 1) != 0) {
 			sc->sc_rxreap = TEMAC_RXINC(sc->sc_rxreap, -1);
 			aprint_error_dev(sc->sc_dev, "Rx give up for now\n");
			break;
		}
	}

	/* We may now have a contiguous ready-to-go chunk of descriptors. */
	if (nseg > 0) {
#if TEMAC_RXDEBUG > 0
		aprint_debug_dev(sc->sc_dev,
		    "rxreap: rxreap %03d -> %03d, nseg %03d\n",
		    head, sc->sc_rxreap, nseg);
#endif
		temac_rxcdsync(sc, head, nseg,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		if (TEMAC_ISLAST(tail))
			cdmac_rx_start(sc, sc->sc_cdaddr + TEMAC_RXDOFF(0));
	}

	/* Ensure maximum Rx latency is kept under control. */
	callout_schedule(&sc->sc_rx_timo, hz / TEMAC_RXTIMO_HZ);
}

static void
temac_rxtimo(void *arg)
{
	struct temac_softc 	*sc = (struct temac_softc *)arg;
	int 			s;

	/* We run TEMAC_RXTIMO_HZ times/sec to ensure Rx doesn't stall. */
	s = splnet();
	temac_rxreap(sc);
	splx(s);
}

static void
temac_reset(struct temac_softc *sc)
{
	uint32_t 		rcr, tcr;

	/* Kill CDMAC channels. */
	cdmac_tx_reset(sc);
	cdmac_rx_reset(sc);

	/* Disable receiver. */
	rcr = gmi_read_4(TEMAC_GMI_RXCF1) & ~GMI_RX_ENABLE;
	gmi_write_4(TEMAC_GMI_RXCF1, rcr);

	/* Disable transmitter. */
	tcr = gmi_read_4(TEMAC_GMI_TXCF) & ~GMI_TX_ENABLE;
	gmi_write_4(TEMAC_GMI_TXCF, tcr);
}

static void
temac_rxdrain(struct temac_softc *sc)
{
	struct temac_rxsoft 	*rxs;
	int 			i;

	for (i = 0; i < TEMAC_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];

		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}

	sc->sc_rx_drained = 1;
}

static void
temac_txkick(struct temac_softc *sc)
{
	if (sc->sc_txsoft[sc->sc_txsreap].txs_mbuf != NULL &&
	    sc->sc_txbusy == 0) {
		cdmac_tx_start(sc, sc->sc_cdaddr + TEMAC_TXDOFF(sc->sc_txreap));
		sc->sc_txbusy = 1;
	}
}
