/*	$NetBSD: ralink_eth.c,v 1.13.12.1 2018/07/28 04:37:37 pgoyette Exp $	*/
/*-
 * Copyright (c) 2011 CradlePoint Technology, Inc.
 * All rights reserved.
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY CRADLEPOINT TECHNOLOGY, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* ralink_eth.c -- Ralink Ethernet Driver */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ralink_eth.c,v 1.13.12.1 2018/07/28 04:37:37 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/callout.h>
#include <sys/device.h>
#include <sys/endian.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/intr.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/if_vlanvar.h>

#include <net/bpf.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <mips/ralink/ralink_var.h>
#include <mips/ralink/ralink_reg.h>
#if 0
#define CPDEBUG				/* XXX TMP DEBUG FIXME */
#define RALINK_ETH_DEBUG		/* XXX TMP DEBUG FIXME */
#define ENABLE_RALINK_DEBUG_ERROR 1
#define ENABLE_RALINK_DEBUG_MISC  1
#define ENABLE_RALINK_DEBUG_INFO  1
#define ENABLE_RALINK_DEBUG_FORCE 1
#define ENABLE_RALINK_DEBUG_REG   1
#endif
#include <mips/ralink/ralink_debug.h>


/* PDMA RX Descriptor Format */
struct ralink_rx_desc {
	uint32_t data_ptr;
	uint32_t rxd_info1;
#define RXD_LEN1(x)	(((x) >> 0) & 0x3fff)
#define RXD_LAST1	(1 << 14)
#define RXD_LEN0(x)	(((x) >> 16) & 0x3fff)
#define RXD_LAST0	(1 << 30)
#define RXD_DDONE	(1 << 31)
	uint32_t unused;
	uint32_t rxd_info2;
#define RXD_FOE(x)	(((x) >> 0) & 0x3fff)
#define RXD_FVLD	(1 << 14)
#define RXD_INFO(x)	(((x) >> 16) & 0xff)
#define RXD_PORT(x)	(((x) >> 24) & 0x7)
#define RXD_INFO_CPU	(1 << 27)
#define RXD_L4_FAIL	(1 << 28)
#define RXD_IP_FAIL	(1 << 29)
#define RXD_L4_VLD	(1 << 30)
#define RXD_IP_VLD	(1 << 31)
};

/* PDMA TX Descriptor Format */
struct ralink_tx_desc {
	uint32_t data_ptr0;
	uint32_t txd_info1;
#define TXD_LEN1(x)	(((x) & 0x3fff) << 0)
#define TXD_LAST1	(1 << 14)
#define TXD_BURST	(1 << 15)
#define TXD_LEN0(x)	(((x) & 0x3fff) << 16)
#define TXD_LAST0	(1 << 30)
#define TXD_DDONE	(1 << 31)
	uint32_t data_ptr1;
	uint32_t txd_info2;
#define TXD_VIDX(x)	(((x) & 0xf) << 0)
#define TXD_VPRI(x)	(((x) & 0x7) << 4)
#define TXD_VEN		(1 << 7)
#define TXD_SIDX(x)	(((x) & 0xf) << 8)
#define TXD_SEN(x)	(1 << 13)
#define TXD_QN(x)	(((x) & 0x7) << 16)
#define TXD_PN(x)	(((x) & 0x7) << 24)
#define  TXD_PN_CPU	0
#define  TXD_PN_GDMA1	1
#define  TXD_PN_GDMA2	2
#define TXD_TCP_EN	(1 << 29)
#define TXD_UDP_EN	(1 << 30)
#define TXD_IP_EN	(1 << 31)
};

/* TODO:
 * try to scale number of descriptors swith size of memory
 * these numbers may have a significant impact on performance/memory/mbuf usage
 */
#if RTMEMSIZE >= 64
#define RALINK_ETH_NUM_RX_DESC 256
#define RALINK_ETH_NUM_TX_DESC 256
#else
#define RALINK_ETH_NUM_RX_DESC 64
#define RALINK_ETH_NUM_TX_DESC 64
#endif
/* maximum segments per packet */
#define RALINK_ETH_MAX_TX_SEGS 1

/* define a struct for ease of dma memory allocation */
struct ralink_descs {
	struct ralink_rx_desc rxdesc[RALINK_ETH_NUM_RX_DESC];
	struct ralink_tx_desc txdesc[RALINK_ETH_NUM_TX_DESC];
};

/* Software state for transmit jobs. */
struct ralink_eth_txstate {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_idx;			/* the index in txdesc ring that */
					/*  this state is tracking */
	SIMPLEQ_ENTRY(ralink_eth_txstate) txs_q;
};

SIMPLEQ_HEAD(ralink_eth_txsq, ralink_eth_txstate);

/*
 * Software state for receive jobs.
 */
struct ralink_eth_rxstate {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

typedef struct ralink_eth_softc {
	device_t sc_dev;		/* generic device information */
	bus_space_tag_t sc_memt;	/* bus space tag */
	bus_space_handle_t sc_sy_memh;	/* handle at SYSCTL_BASE */
	bus_space_handle_t sc_fe_memh;	/* handle at FRAME_ENGINE_BASE */
	bus_space_handle_t sc_sw_memh;	/* handle at ETH_SW_BASE */
	int sc_sy_size;			/* size of Sysctl regs space */
	int sc_fe_size;			/* size of Frame Engine regs space */
	int sc_sw_size;			/* size of Ether Switch regs space */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	void *sc_ih;			/* interrupt handle */

	/* tx/rx dma mapping */
	bus_dma_segment_t sc_dseg;
	int sc_ndseg;
	bus_dmamap_t sc_pdmamap;	/* PDMA DMA map */
#define sc_pdma sc_pdmamap->dm_segs[0].ds_addr

	struct ralink_descs *sc_descs;
#define sc_rxdesc sc_descs->rxdesc
#define sc_txdesc sc_descs->txdesc

#define RALINK_MIN_BUF 64
	char ralink_zero_buf[RALINK_MIN_BUF];

	struct ralink_eth_txstate sc_txstate[RALINK_ETH_NUM_TX_DESC];
	struct ralink_eth_rxstate sc_rxstate[RALINK_ETH_NUM_RX_DESC];

	struct ralink_eth_txsq sc_txfreeq;	/* free Tx descsofts */
	struct ralink_eth_txsq sc_txdirtyq;	/* dirty Tx descsofts */

	struct ethercom sc_ethercom;		/* ethernet common data */
	u_int sc_pending_tx;

	/* mii */
	struct mii_data sc_mii;
	struct callout sc_tick_callout;

	struct evcnt sc_evcnt_spurious_intr;
	struct evcnt sc_evcnt_rxintr;
	struct evcnt sc_evcnt_rxintr_skip_len;
	struct evcnt sc_evcnt_rxintr_skip_tag_none;
	struct evcnt sc_evcnt_rxintr_skip_tag_inval;
	struct evcnt sc_evcnt_rxintr_skip_inact;
	struct evcnt sc_evcnt_txintr;
	struct evcnt sc_evcnt_input;
	struct evcnt sc_evcnt_output;
	struct evcnt sc_evcnt_watchdog;
	struct evcnt sc_evcnt_wd_reactivate;
	struct evcnt sc_evcnt_wd_tx;
	struct evcnt sc_evcnt_wd_spurious;
	struct evcnt sc_evcnt_add_rxbuf_hdr_fail;
	struct evcnt sc_evcnt_add_rxbuf_mcl_fail;
} ralink_eth_softc_t;

/* alignment so the IP header is aligned */
#define RALINK_ETHER_ALIGN 2

/* device functions */
static int  ralink_eth_match(device_t, cfdata_t, void *);
static void ralink_eth_attach(device_t, device_t, void *);
static int  ralink_eth_detach(device_t, int);
static int  ralink_eth_activate(device_t, enum devact);

/* local driver functions */
static void ralink_eth_hw_init(ralink_eth_softc_t *);
static int  ralink_eth_intr(void *);
static void ralink_eth_reset(ralink_eth_softc_t *);
static void ralink_eth_rxintr(ralink_eth_softc_t *);
static void ralink_eth_txintr(ralink_eth_softc_t *);

/* partition functions */
static int  ralink_eth_enable(ralink_eth_softc_t *);
static void ralink_eth_disable(ralink_eth_softc_t *);

/* ifnet functions */
static int  ralink_eth_init(struct ifnet *);
static void ralink_eth_rxdrain(ralink_eth_softc_t *);
static void ralink_eth_stop(struct ifnet *, int);
static int  ralink_eth_add_rxbuf(ralink_eth_softc_t *, int);
static void ralink_eth_start(struct ifnet *);
static void ralink_eth_watchdog(struct ifnet *);
static int  ralink_eth_ioctl(struct ifnet *, u_long, void *);

/* mii functions */
#if defined(RT3050) || defined(RT3052)
static void ralink_eth_mdio_enable(ralink_eth_softc_t *, bool);
#endif
static void ralink_eth_mii_statchg(struct ifnet *);
static void ralink_eth_mii_tick(void *);
static int  ralink_eth_mii_read(device_t, int, int);
static void ralink_eth_mii_write(device_t, int, int, int);

CFATTACH_DECL_NEW(reth, sizeof(struct ralink_eth_softc),
    ralink_eth_match, ralink_eth_attach, ralink_eth_detach,
    ralink_eth_activate);

static inline uint32_t
sy_read(const ralink_eth_softc_t *sc, const bus_size_t off)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_sy_memh, off);
}

static inline void
sy_write(const ralink_eth_softc_t *sc, const bus_size_t off, const uint32_t val)
{
	bus_space_write_4(sc->sc_memt, sc->sc_sy_memh, off, val);
}

static inline uint32_t
fe_read(const ralink_eth_softc_t *sc, const bus_size_t off)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_fe_memh, off);
}

static inline void
fe_write(const ralink_eth_softc_t *sc, const bus_size_t off, const uint32_t val)
{
	bus_space_write_4(sc->sc_memt, sc->sc_fe_memh, off, val);
}

static inline uint32_t
sw_read(const ralink_eth_softc_t *sc, const bus_size_t off)
{
	return bus_space_read_4(sc->sc_memt, sc->sc_sw_memh, off);
}

static inline void
sw_write(const ralink_eth_softc_t *sc, const bus_size_t off, const uint32_t val)
{
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, off, val);
}

/*
 * ralink_eth_match
 */
int
ralink_eth_match(device_t parent, cfdata_t cf, void *aux)
{
	return 1;
}

/*
 * ralink_eth_attach
 */
void
ralink_eth_attach(device_t parent, device_t self, void *aux)
{
	ralink_eth_softc_t * const sc = device_private(self);
	const struct mainbus_attach_args *ma = aux;
	int error;
	int i;

	aprint_naive(": Ralink Ethernet\n");
	aprint_normal(": Ralink Ethernet\n");

	evcnt_attach_dynamic(&sc->sc_evcnt_spurious_intr, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "spurious intr");
	evcnt_attach_dynamic(&sc->sc_evcnt_rxintr, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "rxintr");
	evcnt_attach_dynamic(&sc->sc_evcnt_rxintr_skip_len,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_rxintr,
	    device_xname(self), "rxintr skip: no room for VLAN header");
	evcnt_attach_dynamic(&sc->sc_evcnt_rxintr_skip_tag_none,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_rxintr,
	    device_xname(self), "rxintr skip: no VLAN tag");
	evcnt_attach_dynamic(&sc->sc_evcnt_rxintr_skip_tag_inval,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_rxintr,
	    device_xname(self), "rxintr skip: invalid VLAN tag");
	evcnt_attach_dynamic(&sc->sc_evcnt_rxintr_skip_inact,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_rxintr,
	    device_xname(self), "rxintr skip: partition inactive");
	evcnt_attach_dynamic(&sc->sc_evcnt_txintr, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "txintr");
	evcnt_attach_dynamic(&sc->sc_evcnt_input, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "input");
	evcnt_attach_dynamic(&sc->sc_evcnt_output, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "output");
	evcnt_attach_dynamic(&sc->sc_evcnt_watchdog, EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "watchdog");
	evcnt_attach_dynamic(&sc->sc_evcnt_wd_tx,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_watchdog,
	    device_xname(self), "watchdog TX timeout");
	evcnt_attach_dynamic(&sc->sc_evcnt_wd_spurious,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_watchdog,
	    device_xname(self), "watchdog spurious");
	evcnt_attach_dynamic(&sc->sc_evcnt_wd_reactivate,
	    EVCNT_TYPE_INTR, &sc->sc_evcnt_watchdog,
	    device_xname(self), "watchdog reactivate");
	evcnt_attach_dynamic(&sc->sc_evcnt_add_rxbuf_hdr_fail,
	    EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "add rxbuf hdr fail");
	evcnt_attach_dynamic(&sc->sc_evcnt_add_rxbuf_mcl_fail,
	    EVCNT_TYPE_INTR, NULL,
	    device_xname(self), "add rxbuf mcl fail");

	/*
	 * In order to obtain unique initial Ethernet address on a host,
	 * do some randomisation using the current uptime.  It's not meant
	 * for anything but avoiding hard-coding an address.
	 */
#ifdef RALINK_ETH_MACADDR
	uint8_t enaddr[ETHER_ADDR_LEN];
	ether_aton_r(enaddr, sizeof(enaddr), ___STRING(RALINK_ETH_MACADDR));
#else
	uint8_t enaddr[ETHER_ADDR_LEN] = { 0x00, 0x30, 0x44, 0x00, 0x00, 0x00 };
#endif

	sc->sc_dev = self;
	sc->sc_dmat = ma->ma_dmat;
	sc->sc_memt = ma->ma_memt;
	sc->sc_sy_size = 0x10000;
	sc->sc_fe_size = 0x10000;
	sc->sc_sw_size = 0x08000;

	/*
	 * map the registers
	 *
	 * we map the Sysctl, Frame Engine and Ether Switch registers
	 * seperately so we can use the defined register offsets sanely
	 */
	if ((error = bus_space_map(sc->sc_memt, RA_SYSCTL_BASE,
	    sc->sc_sy_size, 0, &sc->sc_sy_memh)) != 0) {
		aprint_error_dev(self, "unable to map Sysctl registers, "
		    "error=%d\n", error);
		goto fail_0a;
	}
	if ((error = bus_space_map(sc->sc_memt, RA_FRAME_ENGINE_BASE,
	    sc->sc_fe_size, 0, &sc->sc_fe_memh)) != 0) {
		aprint_error_dev(self, "unable to map Frame Engine registers, "
		    "error=%d\n", error);
		goto fail_0b;
	}
	if ((error = bus_space_map(sc->sc_memt, RA_ETH_SW_BASE,
	    sc->sc_sw_size, 0, &sc->sc_sw_memh)) != 0) {
		aprint_error_dev(self, "unable to map Ether Switch registers, "
		    "error=%d\n", error);
		goto fail_0c;
	}

	/* Allocate desc structures, and create & load the DMA map for them */
	if ((error = bus_dmamem_alloc(sc->sc_dmat, sizeof(struct ralink_descs),
	    PAGE_SIZE, 0, &sc->sc_dseg, 1, &sc->sc_ndseg, 0)) != 0) {
		aprint_error_dev(self, "unable to allocate transmit descs, "
		    "error=%d\n", error);
		goto fail_1;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, &sc->sc_dseg, sc->sc_ndseg,
	    sizeof(struct ralink_descs), (void **)&sc->sc_descs,
	    BUS_DMA_COHERENT)) != 0) {
		aprint_error_dev(self, "unable to map control data, "
		    "error=%d\n", error);
		goto fail_2;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat, sizeof(struct ralink_descs),
	    1, sizeof(struct ralink_descs), 0, 0, &sc->sc_pdmamap)) != 0) {
		aprint_error_dev(self, "unable to create control data DMA map, "
		    "error=%d\n", error);
		goto fail_3;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_pdmamap, sc->sc_descs,
	    sizeof(struct ralink_descs), NULL, 0)) != 0) {
		aprint_error_dev(self, "unable to load control data DMA map, "
		    "error=%d\n", error);
		goto fail_4;
	}

	/* Create the transmit buffer DMA maps.  */
	for (i = 0; i < RALINK_ETH_NUM_TX_DESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    RALINK_ETH_MAX_TX_SEGS, MCLBYTES, 0, 0,
		    &sc->sc_txstate[i].txs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create tx DMA map %d, error=%d\n",
			    i, error);
			goto fail_5;
		}
	}

	/* Create the receive buffer DMA maps.  */
	for (i = 0; i < RALINK_ETH_NUM_RX_DESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES, 1,
		    MCLBYTES, 0, 0, &sc->sc_rxstate[i].rxs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create rx DMA map %d, error=%d\n",
			    i, error);
			goto fail_6;
		}
		sc->sc_rxstate[i].rxs_mbuf = NULL;
	}

	/* this is a zero buffer used for zero'ing out short packets */
	memset(sc->ralink_zero_buf, 0, RALINK_MIN_BUF);

	/* setup some address in hardware */
	fe_write(sc, RA_FE_GDMA1_MAC_LSB,
	    (enaddr[5] | (enaddr[4] << 8) |
	    (enaddr[3] << 16) | (enaddr[2] << 24)));
	fe_write(sc, RA_FE_GDMA1_MAC_MSB,
	    (enaddr[1] | (enaddr[0] << 8)));

	/*
	 * iterate through ports
	 *  slickrock must use specific non-linear sequence
	 *  others are linear
	 */
	struct ifnet * const ifp = &sc->sc_ethercom.ec_if;

	strlcpy(ifp->if_xname, device_xname(self), IFNAMSIZ);

	/*
	 * Initialize our media structures.
	 * This may probe the PHY, if present.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = ralink_eth_mii_read;
	sc->sc_mii.mii_writereg = ralink_eth_mii_write;
	sc->sc_mii.mii_statchg = ralink_eth_mii_statchg;
	sc->sc_ethercom.ec_mii = &sc->sc_mii;
	ifmedia_init(&sc->sc_mii.mii_media, 0, ether_mediachange,
	    ether_mediastatus);
	mii_attach(sc->sc_dev, &sc->sc_mii, ~0, MII_PHY_ANY, MII_OFFSET_ANY,
	    MIIF_FORCEANEG|MIIF_DOPAUSE|MIIF_NOISOLATE);

	if (LIST_EMPTY(&sc->sc_mii.mii_phys)) {
#if 1
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_1000_T|
		    IFM_FDX|IFM_ETH_RXPAUSE|IFM_ETH_TXPAUSE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_1000_T|
		    IFM_FDX|IFM_ETH_RXPAUSE|IFM_ETH_TXPAUSE);
#else
		ifmedia_add(&sc->sc_mii.mii_media,
		    IFM_ETHER|IFM_MANUAL, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_MANUAL);
#endif
	} else {
		/* Ensure we mask ok for the switch multiple phy's */
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_init = ralink_eth_init;
	ifp->if_start = ralink_eth_start;
	ifp->if_ioctl = ralink_eth_ioctl;
	ifp->if_stop = ralink_eth_stop;
	ifp->if_watchdog = ralink_eth_watchdog;
	IFQ_SET_READY(&ifp->if_snd);

	/* We can support 802.1Q VLAN-sized frames. */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

	/* We support IPV4 CRC Offload */
	ifp->if_capabilities |=
	    (IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx);

	/* Attach the interface. */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr);

	/* init our mii ticker */
	callout_init(&sc->sc_tick_callout, 0);
	callout_reset(&sc->sc_tick_callout, hz, ralink_eth_mii_tick, sc);

	return;

	/*
	 * Free any resources we've allocated during the failed attach
	 * attempt.  Do this in reverse order and fall through.
	 */
 fail_6:
	for (i = 0; i < RALINK_ETH_NUM_RX_DESC; i++) {
		if (sc->sc_rxstate[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxstate[i].rxs_dmamap);
	}
 fail_5:
	for (i = 0; i < RALINK_ETH_NUM_TX_DESC; i++) {
		if (sc->sc_txstate[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txstate[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_pdmamap);
 fail_4:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_pdmamap);
 fail_3:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_descs,
	    sizeof(struct ralink_descs));
 fail_2:
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dseg, sc->sc_ndseg);
 fail_1:
	bus_space_unmap(sc->sc_memt, sc->sc_sw_memh, sc->sc_sw_size);
 fail_0c:
	bus_space_unmap(sc->sc_memt, sc->sc_fe_memh, sc->sc_fe_size);
 fail_0b:
	bus_space_unmap(sc->sc_memt, sc->sc_sy_memh, sc->sc_fe_size);
 fail_0a:
	return;
}

/*
 * ralink_eth_activate:
 *
 *	Handle device activation/deactivation requests.
 */
int
ralink_eth_activate(device_t self, enum devact act)
{
	ralink_eth_softc_t * const sc = device_private(self);
	int error = 0;
	int s;

	s = splnet();
	switch (act) {
	case DVACT_DEACTIVATE:
		if_deactivate(&sc->sc_ethercom.ec_if);
		break;
	}
	splx(s);

	return error;
}

/*
 * ralink_eth_partition_enable
 */
static int
ralink_eth_enable(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();

	if (sc->sc_ih != NULL) {
		RALINK_DEBUG(RALINK_DEBUG_MISC, "%s() already active",
			__func__);
		return EALREADY;
	}

	sc->sc_pending_tx = 0;

	int s = splnet();
	ralink_eth_hw_init(sc);
	sc->sc_ih = ra_intr_establish(RA_IRQ_FENGINE,
	    ralink_eth_intr, sc, 1);
	splx(s);
	if (sc->sc_ih == NULL) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR,
		    "%s: unable to establish interrupt\n",
		    device_xname(sc->sc_dev));
		return EIO;
	}

	return 0;
}

/*
 * ralink_eth_partition_disable
 */
static void
ralink_eth_disable(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();

	int s = splnet();
	ralink_eth_rxdrain(sc);
	ra_intr_disestablish(sc->sc_ih);
	sc->sc_ih = NULL;

	/* stop the mii ticker */
	callout_stop(&sc->sc_tick_callout);

	/* quiesce the block */
	ralink_eth_reset(sc);
	splx(s);
}

/*
 * ralink_eth_detach
 */
static int
ralink_eth_detach(device_t self, int flags)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ralink_eth_softc_t * const sc = device_private(self);
	struct ifnet * const ifp = &sc->sc_ethercom.ec_if;
	struct ralink_eth_rxstate *rxs;
	struct ralink_eth_txstate *txs;
	int i;

	ralink_eth_disable(sc);
	mii_detach(&sc->sc_mii, MII_PHY_ANY, MII_OFFSET_ANY);
	ifmedia_delete_instance(&sc->sc_mii.mii_media, IFM_INST_ANY);
	ether_ifdetach(ifp);
	if_detach(ifp);

	for (i = 0; i < RALINK_ETH_NUM_RX_DESC; i++) {
		rxs = &sc->sc_rxstate[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
		bus_dmamap_destroy(sc->sc_dmat, rxs->rxs_dmamap);
	}

	for (i = 0; i < RALINK_ETH_NUM_TX_DESC; i++) {
		txs = &sc->sc_txstate[i];
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
		bus_dmamap_destroy(sc->sc_dmat, txs->txs_dmamap);
	}

	bus_dmamap_unload(sc->sc_dmat, sc->sc_pdmamap);
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_pdmamap);
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_descs,
	    sizeof(struct ralink_descs));
	bus_dmamem_free(sc->sc_dmat, &sc->sc_dseg, sc->sc_ndseg);

	bus_space_unmap(sc->sc_memt, sc->sc_sw_memh, sc->sc_sw_size);
	bus_space_unmap(sc->sc_memt, sc->sc_fe_memh, sc->sc_fe_size);

	return 0;
}

/*
 * ralink_eth_reset
 */
static void
ralink_eth_reset(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();
	uint32_t r;

	/* Reset the frame engine */
	r = sy_read(sc, RA_SYSCTL_RST);
	r |= RST_FE;
	sy_write(sc, RA_SYSCTL_RST, r);
	r ^= RST_FE;
	sy_write(sc, RA_SYSCTL_RST, r);

	/* Wait until the PDMA is quiescent */
	for (;;) {
		r = fe_read(sc, RA_FE_PDMA_GLOBAL_CFG);
		if (r & FE_PDMA_GLOBAL_CFG_RX_DMA_BUSY) {
			aprint_normal_dev(sc->sc_dev, "RX DMA BUSY\n");
			continue;
		}
		if (r & FE_PDMA_GLOBAL_CFG_TX_DMA_BUSY) {
			aprint_normal_dev(sc->sc_dev, "TX DMA BUSY\n");
			continue;
		}
		break;
	}
}

/*
 * ralink_eth_hw_init
 */
static void
ralink_eth_hw_init(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();
	struct ralink_eth_txstate *txs;
	uint32_t r;
	int i;

	/* reset to a known good state */
	ralink_eth_reset(sc);

#if defined(RT3050) || defined(RT3052) || defined(MT7628)
	/* Bring the switch to a sane default state (from linux driver) */
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_SGC2,
	    0x00000000);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_PFC1,
	    0x00405555);	/* check VLAN tag on port forward */
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_VLANI0,
	    0x00002001);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_PVIDC0,
	    0x00001002);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_PVIDC1,
	    0x00001001);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_PVIDC2,
	    0x00001001);
#if defined(MT7628)
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_VMSC0,
	    0xffffffff);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_POC0,
	    0x10007f7f);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_POC2,
	    0x00007f7f);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_FTC2,
	    0x0002500c);
#else
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_VMSC0,
	    0xffff417e);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_POC0,
	    0x00007f7f);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_POC2,
	    0x00007f3f);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_FTC2,
	    0x00d6500c);
#endif
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_SWGC,
	    0x0008a301);	/* hashing algorithm=XOR48 */
				/*  aging interval=300sec  */
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_SOCPC,
	    0x02404040);
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_FPORT,
	    0x3f502b28);	/* Change polling Ext PHY Addr=0x0 */
	bus_space_write_4(sc->sc_memt, sc->sc_sw_memh, RA_ETH_SW_FPA,
	    0x00000000);

	/* do some mii magic  TODO: define these registers/bits */
	/* lower down PHY 10Mbps mode power */
	/* select local register */
	ralink_eth_mii_write(sc->sc_dev, 0, 31, 0x8000);

	for (i=0; i < 5; i++) {
		/* set TX10 waveform coefficient */
		ralink_eth_mii_write(sc->sc_dev, i, 26, 0x1601);

		/* set TX100/TX10 AD/DA current bias */
		ralink_eth_mii_write(sc->sc_dev, i, 29, 0x7058);

		/* set TX100 slew rate control */
		ralink_eth_mii_write(sc->sc_dev, i, 30, 0x0018);
	}

	/* PHY IOT */

	/* select global register */
	ralink_eth_mii_write(sc->sc_dev, 0, 31, 0x0);

	/* tune TP_IDL tail and head waveform */
	ralink_eth_mii_write(sc->sc_dev, 0, 22, 0x052f);

	/* set TX10 signal amplitude threshold to minimum */
	ralink_eth_mii_write(sc->sc_dev, 0, 17, 0x0fe0);

	/* set squelch amplitude to higher threshold */
	ralink_eth_mii_write(sc->sc_dev, 0, 18, 0x40ba);

	/* longer TP_IDL tail length */
	ralink_eth_mii_write(sc->sc_dev, 0, 14, 0x65);

	/* select local register */
	ralink_eth_mii_write(sc->sc_dev, 0, 31, 0x8000);
#else
	/* GE1 + GigSW */
	fe_write(sc, RA_FE_MDIO_CFG1,
	    MDIO_CFG_PHY_ADDR(0x1f) |
	    MDIO_CFG_BP_EN |
	    MDIO_CFG_FORCE_CFG |
	    MDIO_CFG_SPEED(MDIO_CFG_SPEED_1000M) |
	    MDIO_CFG_FULL_DUPLEX |
	    MDIO_CFG_FC_TX |
	    MDIO_CFG_FC_RX |
	    MDIO_CFG_TX_CLK_MODE(MDIO_CFG_TX_CLK_MODE_3COM));
#endif

	/*
	 * TODO: QOS - RT3052 has 4 TX queues for QOS,
	 * forgoing for 1 for simplicity
	 */

	/*
	 * Allocate DMA accessible memory for TX/RX descriptor rings
	 */

	/* Initialize the TX queues. */
	SIMPLEQ_INIT(&sc->sc_txfreeq);
	SIMPLEQ_INIT(&sc->sc_txdirtyq);

	/* Initialize the TX descriptor ring. */
	memset(sc->sc_txdesc, 0, sizeof(sc->sc_txdesc));
	for (i = 0; i < RALINK_ETH_NUM_TX_DESC; i++) {

		sc->sc_txdesc[i].txd_info1 = TXD_LAST0 | TXD_DDONE;

		/* setup the freeq as well */
		txs = &sc->sc_txstate[i];
		txs->txs_mbuf = NULL;
		txs->txs_idx = i;
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	/*
	 * Flush the TX descriptors
	 *  - TODO: can we just access descriptors via KSEG1
	 *    to avoid the flush?
	 */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_pdmamap,
	    (int)&sc->sc_txdesc - (int)sc->sc_descs, sizeof(sc->sc_txdesc),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* Initialize the RX descriptor ring */
	memset(sc->sc_rxdesc, 0, sizeof(sc->sc_rxdesc));
	for (i = 0; i < RALINK_ETH_NUM_RX_DESC; i++) {
		if (ralink_eth_add_rxbuf(sc, i)) {
			panic("Can't allocate rx mbuf\n");
		}
	}

	/*
	 * Flush the RX descriptors
	 * - TODO: can we just access descriptors via KSEG1
	 *   to avoid the flush?
	 */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_pdmamap,
	    (int)&sc->sc_rxdesc - (int)sc->sc_descs, sizeof(sc->sc_rxdesc),
	    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

	/* Clear the PDMA state */
	r = fe_read(sc, RA_FE_PDMA_GLOBAL_CFG);
	r &= 0xff;
	fe_write(sc, RA_FE_PDMA_GLOBAL_CFG, r);
	(void) fe_read(sc, RA_FE_PDMA_GLOBAL_CFG);

#if !defined(MT7628)
	/* Setup the PDMA VLAN ID's */
	fe_write(sc, RA_FE_VLAN_ID_0001, 0x00010000);
	fe_write(sc, RA_FE_VLAN_ID_0203, 0x00030002);
	fe_write(sc, RA_FE_VLAN_ID_0405, 0x00050004);
	fe_write(sc, RA_FE_VLAN_ID_0607, 0x00070006);
	fe_write(sc, RA_FE_VLAN_ID_0809, 0x00090008);
	fe_write(sc, RA_FE_VLAN_ID_1011, 0x000b000a);
	fe_write(sc, RA_FE_VLAN_ID_1213, 0x000d000c);
	fe_write(sc, RA_FE_VLAN_ID_1415, 0x000f000e);
#endif

	/* Give the TX and TX rings to the chip. */
	fe_write(sc, RA_FE_PDMA_TX0_PTR,
	    htole32(MIPS_KSEG0_TO_PHYS(&sc->sc_txdesc)));
	fe_write(sc, RA_FE_PDMA_TX0_COUNT, htole32(RALINK_ETH_NUM_TX_DESC));
	fe_write(sc, RA_FE_PDMA_TX0_CPU_IDX, 0);
#if !defined(MT7628)
	fe_write(sc, RA_FE_PDMA_RESET_IDX, PDMA_RST_TX0);
#endif

	fe_write(sc, RA_FE_PDMA_RX0_PTR,
	    htole32(MIPS_KSEG0_TO_PHYS(&sc->sc_rxdesc)));
	fe_write(sc, RA_FE_PDMA_RX0_COUNT, htole32(RALINK_ETH_NUM_RX_DESC));
	fe_write(sc, RA_FE_PDMA_RX0_CPU_IDX,
	    htole32(RALINK_ETH_NUM_RX_DESC - 1));
#if !defined(MT7628)
	fe_write(sc, RA_FE_PDMA_RESET_IDX, PDMA_RST_RX0);
#endif
	fe_write(sc, RA_FE_PDMA_RX0_CPU_IDX,
	    htole32(RALINK_ETH_NUM_RX_DESC - 1));

	/* Start PDMA */
	fe_write(sc, RA_FE_PDMA_GLOBAL_CFG,
	    FE_PDMA_GLOBAL_CFG_TX_WB_DDONE |
	    FE_PDMA_GLOBAL_CFG_RX_DMA_EN |
	    FE_PDMA_GLOBAL_CFG_TX_DMA_EN |
	    FE_PDMA_GLOBAL_CFG_BURST_SZ_4);

	/* Setup the clock for the Frame Engine */
#if defined(MT7628)
	fe_write(sc, RA_FE_SDM_CON, 0x8100);
#else
	fe_write(sc, RA_FE_GLOBAL_CFG,
	    FE_GLOBAL_CFG_EXT_VLAN(0x8100) |
	    FE_GLOBAL_CFG_US_CLK(RA_BUS_FREQ / 1000000) |
	    FE_GLOBAL_CFG_L2_SPACE(0x8));
#endif

	/* Turn on all interrupts */
#if defined(MT7628)
	fe_write(sc, RA_FE_INT_MASK,
	    RA_FE_INT_RX_DONE_INT1 |
	    RA_FE_INT_RX_DONE_INT0 |
	    RA_FE_INT_TX_DONE_INT3 |
	    RA_FE_INT_TX_DONE_INT2 |
	    RA_FE_INT_TX_DONE_INT1 |
	    RA_FE_INT_TX_DONE_INT0);
#else
	fe_write(sc, RA_FE_INT_ENABLE,
	    FE_INT_RX | FE_INT_TX3 | FE_INT_TX2 | FE_INT_TX1 | FE_INT_TX0);
#endif

	/*
	 * Configure GDMA forwarding
	 * - default all packets to CPU
	 * - Turn on auto-CRC
	 */
#if 0
	fe_write(sc, RA_FE_GDMA1_FWD_CFG,
	    (FE_GDMA_FWD_CFG_DIS_TX_CRC | FE_GDMA_FWD_CFG_DIS_TX_PAD));
#endif

#if !defined(MT7628)
	fe_write(sc, RA_FE_GDMA1_FWD_CFG,
	    FE_GDMA_FWD_CFG_JUMBO_LEN(MCLBYTES/1024) |
	    FE_GDMA_FWD_CFG_STRIP_RX_CRC |
	    FE_GDMA_FWD_CFG_IP4_CRC_EN |
	    FE_GDMA_FWD_CFG_TCP_CRC_EN |
	    FE_GDMA_FWD_CFG_UDP_CRC_EN);
#endif

	/* CDMA also needs CRCs turned on */
#if !defined(MT7628)
	r = fe_read(sc, RA_FE_CDMA_CSG_CFG);
	r |= (FE_CDMA_CSG_CFG_IP4_CRC_EN | FE_CDMA_CSG_CFG_UDP_CRC_EN |
	    FE_CDMA_CSG_CFG_TCP_CRC_EN);
	fe_write(sc, RA_FE_CDMA_CSG_CFG, r);
#endif

	/* Configure Flow Control Thresholds */
#if defined(MT7628)
	sw_write(sc, RA_ETH_SW_FCT0,
	    RA_ETH_SW_FCT0_FC_RLS_TH(0xc8) |
	    RA_ETH_SW_FCT0_FC_SET_TH(0xa0) |
	    RA_ETH_SW_FCT0_DROP_RLS_TH(0x78) |
	    RA_ETH_SW_FCT0_DROP_SET_TH(0x50));
	sw_write(sc, RA_ETH_SW_FCT1,
	    RA_ETH_SW_FCT1_PORT_TH(0x14));
#elif defined(RT3883)
	fe_write(sc, RA_FE_PSE_FQ_CFG,
	    FE_PSE_FQ_MAX_COUNT(0xff) |
	    FE_PSE_FQ_FC_RELEASE(0x90) |
	    FE_PSE_FQ_FC_ASSERT(0x80));
#else
	fe_write(sc, RA_FE_PSE_FQ_CFG,
	    FE_PSE_FQ_MAX_COUNT(0x80) |
	    FE_PSE_FQ_FC_RELEASE(0x50) |
	    FE_PSE_FQ_FC_ASSERT(0x40));
#endif

#ifdef RALINK_ETH_DEBUG
#ifdef RA_FE_MDIO_CFG1
	printf("FE_MDIO_CFG1: 0x%08x\n", fe_read(sc, RA_FE_MDIO_CFG1));
#endif
#ifdef RA_FE_MDIO_CFG2
	printf("FE_MDIO_CFG2: 0x%08x\n", fe_read(sc, RA_FE_MDIO_CFG2));
#endif
	printf("FE_PDMA_TX0_PTR: %08x\n", fe_read(sc, RA_FE_PDMA_TX0_PTR));
	printf("FE_PDMA_TX0_COUNT: %08x\n",
	    fe_read(sc, RA_FE_PDMA_TX0_COUNT));
	printf("FE_PDMA_TX0_CPU_IDX: %08x\n",
	    fe_read(sc, RA_FE_PDMA_TX0_CPU_IDX));
	printf("FE_PDMA_TX0_DMA_IDX: %08x\n",
	    fe_read(sc, RA_FE_PDMA_TX0_DMA_IDX));
	printf("FE_PDMA_RX0_PTR: %08x\n", fe_read(sc, RA_FE_PDMA_RX0_PTR));
	printf("FE_PDMA_RX0_COUNT: %08x\n",
	    fe_read(sc, RA_FE_PDMA_RX0_COUNT));
	printf("FE_PDMA_RX0_CPU_IDX: %08x\n",
	    fe_read(sc, RA_FE_PDMA_RX0_CPU_IDX));
	printf("FE_PDMA_RX0_DMA_IDX: %08x\n",
	    fe_read(sc, RA_FE_PDMA_RX0_DMA_IDX));
	printf("FE_PDMA_GLOBAL_CFG: %08x\n",
	    fe_read(sc, RA_FE_PDMA_GLOBAL_CFG));
#ifdef RA_FE_GLOBAL_CFG
	printf("FE_GLOBAL_CFG: %08x\n", fe_read(sc, RA_FE_GLOBAL_CFG));
#endif
#ifdef RA_FE_GDMA1_FWD_CFG
	printf("FE_GDMA1_FWD_CFG: %08x\n",
	    fe_read(sc, RA_FE_GDMA1_FWD_CFG));
#endif
#ifdef RA_FE_CDMA_CSG_CFG
	printf("FE_CDMA_CSG_CFG: %08x\n", fe_read(sc, RA_FE_CDMA_CSG_CFG));
#endif
#ifdef RA_FE_PSE_FQ_CFG
	printf("FE_PSE_FQ_CFG: %08x\n", fe_read(sc, RA_FE_PSE_FQ_CFG));
#endif
#endif

	/* Force PSE Reset to get everything finalized */
#if defined(MT7628)
#else
	fe_write(sc, RA_FE_GLOBAL_RESET, FE_GLOBAL_RESET_PSE);
	fe_write(sc, RA_FE_GLOBAL_RESET, 0);
#endif
}

/*
 * ralink_eth_init
 */
static int
ralink_eth_init(struct ifnet *ifp)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ralink_eth_softc_t * const sc = ifp->if_softc;
	int error;

	error = ralink_eth_enable(sc);
	if (!error) {
		/* Note that the interface is now running. */
		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
	}

	return error;
}

/*
 * ralink_eth_rxdrain
 *
 *  Drain the receive queue.
 */
static void
ralink_eth_rxdrain(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();

	for (int i = 0; i < RALINK_ETH_NUM_RX_DESC; i++) {
		struct ralink_eth_rxstate *rxs = &sc->sc_rxstate[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

/*
 * ralink_eth_stop
 */
static void
ralink_eth_stop(struct ifnet *ifp, int disable)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ralink_eth_softc_t * const sc = ifp->if_softc;

	ralink_eth_disable(sc);

	/* Mark the interface down and cancel the watchdog timer.  */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

/*
 * ralink_eth_add_rxbuf
 */
static int
ralink_eth_add_rxbuf(ralink_eth_softc_t *sc, int idx)
{
	RALINK_DEBUG_FUNC_ENTRY();
	struct ralink_eth_rxstate * const rxs = &sc->sc_rxstate[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		printf("MGETHDR failed\n");
		sc->sc_evcnt_add_rxbuf_hdr_fail.ev_count++;
		return ENOBUFS;
	}

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		printf("MCLGET failed\n");
		sc->sc_evcnt_add_rxbuf_mcl_fail.ev_count++;
		return ENOBUFS;
	}

	m->m_data = m->m_ext.ext_buf;
	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap, m->m_ext.ext_buf,
	    m->m_ext.ext_size, NULL, BUS_DMA_READ|BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev, "can't load rx DMA map %d, "
		    "error=%d\n", idx, error);
		panic(__func__);  /* XXX */
	}

	sc->sc_rxdesc[idx].data_ptr = MIPS_KSEG0_TO_PHYS(
	    rxs->rxs_dmamap->dm_segs[0].ds_addr + RALINK_ETHER_ALIGN);
	sc->sc_rxdesc[idx].rxd_info1 = RXD_LAST0;

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	return 0;
}


/*
 * ralink_eth_start
 */
static void
ralink_eth_start(struct ifnet *ifp)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ralink_eth_softc_t * const sc = ifp->if_softc;
	struct mbuf *m0, *m = NULL;
	struct ralink_eth_txstate *txs;
	bus_dmamap_t dmamap;
	int tx_cpu_idx;
	int error;
	int s;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	s = splnet();

	tx_cpu_idx = fe_read(sc, RA_FE_PDMA_TX0_CPU_IDX);

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available
	 * transmit descriptors.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txfreeq)) != NULL) {
		/* Grab a packet off the queue.  */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		dmamap = txs->txs_dmamap;

		if (m0->m_pkthdr.len < RALINK_MIN_BUF) {
			int padlen = 64 - m0->m_pkthdr.len;
			m_copyback(m0, m0->m_pkthdr.len, padlen,
			    sc->ralink_zero_buf);
			/* TODO : need some checking here */
		}

		/*
		 * Do we need to align the buffer
		 * or does the DMA map load fail?
		 */
		if (bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT) != 0) {

			/* Allocate a new mbuf for re-alignment */
			MGETHDR(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				aprint_error_dev(sc->sc_dev,
				    "unable to allocate aligned Tx mbuf\n");
				break;
			}
			MCLAIM(m, &sc->sc_ethercom.ec_tx_mowner);
			if (m0->m_pkthdr.len > MHLEN) {
				MCLGET(m, M_DONTWAIT);
				if ((m->m_flags & M_EXT) == 0) {
					aprint_error_dev(sc->sc_dev,
					    "unable to allocate Tx cluster\n");
					m_freem(m);
					break;
				}
			}
			m_copydata(m0, 0, m0->m_pkthdr.len, mtod(m, void *));
			m->m_pkthdr.len = m->m_len = m0->m_pkthdr.len;
			error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m,
			    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
			if (error) {
				aprint_error_dev(sc->sc_dev,
				    "unable to load Tx buffer error=%d\n",
				    error);
				m_freem(m);
				break;
			}
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
		/* did we copy the buffer out already? */
		if (m != NULL) {
			m_freem(m0);
			m0 = m;
		}

		/* Sync the DMA map. */
		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		/* Initialize the transmit descriptor */
		sc->sc_txdesc[tx_cpu_idx].data_ptr0 =
		    MIPS_KSEG0_TO_PHYS(dmamap->dm_segs[0].ds_addr);
		sc->sc_txdesc[tx_cpu_idx].txd_info1 =
		    TXD_LEN0(dmamap->dm_segs[0].ds_len) | TXD_LAST0;
		sc->sc_txdesc[tx_cpu_idx].txd_info2 =
		    TXD_QN(3) | TXD_PN(TXD_PN_GDMA1);
		sc->sc_txdesc[tx_cpu_idx].txd_info2 = TXD_QN(3) |
		    TXD_PN(TXD_PN_GDMA1) | TXD_VEN |
		    // TXD_VIDX(pt->vlan_id) |
		    TXD_TCP_EN | TXD_UDP_EN | TXD_IP_EN;

		RALINK_DEBUG(RALINK_DEBUG_REG,"+tx(%d) 0x%08x: 0x%08x\n",
		    tx_cpu_idx, (int)&sc->sc_txdesc[tx_cpu_idx].data_ptr0,
		    sc->sc_txdesc[tx_cpu_idx].data_ptr0);
		RALINK_DEBUG(RALINK_DEBUG_REG,"+tx(%d) 0x%08x: 0x%08x\n",
		    tx_cpu_idx, (int)&sc->sc_txdesc[tx_cpu_idx].txd_info1,
		    sc->sc_txdesc[tx_cpu_idx].txd_info1);
		RALINK_DEBUG(RALINK_DEBUG_REG,"+tx(%d) 0x%08x: 0x%08x\n",
		    tx_cpu_idx, (int)&sc->sc_txdesc[tx_cpu_idx].data_ptr1,
		    sc->sc_txdesc[tx_cpu_idx].data_ptr1);
		RALINK_DEBUG(RALINK_DEBUG_REG,"+tx(%d) 0x%08x: 0x%08x\n",
		    tx_cpu_idx, (int)&sc->sc_txdesc[tx_cpu_idx].txd_info2,
		    sc->sc_txdesc[tx_cpu_idx].txd_info2);

		/* sync the descriptor we're using. */
		bus_dmamap_sync(sc->sc_dmat, sc->sc_pdmamap,
		    (int)&sc->sc_txdesc[tx_cpu_idx] - (int)sc->sc_descs,
		    sizeof(struct ralink_tx_desc),
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * Store a pointer to the packet so we can free it later,
		 * and remember what txdirty will be once the packet is
		 * done.
		 */
		txs->txs_mbuf = m0;
		sc->sc_pending_tx++;
		if (txs->txs_idx != tx_cpu_idx) {
			panic("txs_idx doesn't match %d != %d\n",
			    txs->txs_idx, tx_cpu_idx);
		}

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txfreeq, txs_q);
		SIMPLEQ_INSERT_TAIL(&sc->sc_txdirtyq, txs, txs_q);

		/* Pass the packet to any BPF listeners. */
		bpf_mtap(ifp, m0, BPF_D_OUT);

		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;

		tx_cpu_idx = (tx_cpu_idx + 1) % RALINK_ETH_NUM_TX_DESC;

		/* Write back the tx_cpu_idx */
		fe_write(sc, RA_FE_PDMA_TX0_CPU_IDX, tx_cpu_idx);
	}

	if (txs == NULL) {
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}

	splx(s);
}

/*
 * ralink_eth_watchdog
 *
 *	Watchdog timer handler.
 */
static void
ralink_eth_watchdog(struct ifnet *ifp)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ralink_eth_softc_t * const sc = ifp->if_softc;
	bool doing_transmit;

	sc->sc_evcnt_watchdog.ev_count++;
	doing_transmit = !SIMPLEQ_EMPTY(&sc->sc_txdirtyq);

	if (doing_transmit) {
		RALINK_DEBUG(RALINK_DEBUG_ERROR, "%s: transmit timeout\n",
		    ifp->if_xname);
		ifp->if_oerrors++;
		sc->sc_evcnt_wd_tx.ev_count++;
	} else {
		RALINK_DEBUG(RALINK_DEBUG_ERROR,
		    "%s: spurious watchog timeout\n", ifp->if_xname);
		sc->sc_evcnt_wd_spurious.ev_count++;
		return;
	}

	sc->sc_evcnt_wd_reactivate.ev_count++;
	const int s = splnet();
	/* deactive the active partitions, retaining the active information */
	ralink_eth_disable(sc);
	ralink_eth_enable(sc);
	splx(s);

	/* Try to get more packets going. */
	ralink_eth_start(ifp);
}

/*
 * ralink_eth_ioctl
 *
 *	Handle control requests from the operator.
 */
static int
ralink_eth_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	RALINK_DEBUG_FUNC_ENTRY();
	struct ifdrv * const ifd = (struct ifdrv *) data;
	ralink_eth_softc_t * const sc = ifp->if_softc;
	int s, error = 0;

	RALINK_DEBUG(RALINK_DEBUG_INFO, "ifp: %p  cmd: %lu  data: %p\n",
		ifp, cmd, data);

	s = splnet();

	switch (cmd) {
	case SIOCSDRVSPEC:
		switch (ifd->ifd_cmd) {
#if 0
		case ETH_SWITCH_CMD_PORT_MODE:
			/* len parameter is the mode */
			pt->mode = (int) ifd->ifd_len;
			ralink_eth_configure_switch(pt->sc_reth);
			break;
#endif
		default:
			error = EINVAL;
		}
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error == ENETRESET) {
			if (ifp->if_flags & IFF_RUNNING) {
				/*
				 * Multicast list has changed.  Set the
				 * hardware filter accordingly.
				 */
				RALINK_DEBUG(RALINK_DEBUG_INFO, "TODO!!!");
#if 0
				ralink_eth_filter_setup(sc);
#endif
			}
			error = 0;
		}
		break;
	}

	splx(s);

	/* Try to get more packets going. */
	if (sc->sc_ih != NULL)
		ralink_eth_start(ifp);

	return error;
}

/*
 * ralink_eth_intr
 *
 */
static int
ralink_eth_intr(void *arg)
{
	RALINK_DEBUG_FUNC_ENTRY();
	ralink_eth_softc_t * const sc = arg;

	for (u_int n = 0;; n = 1) {
		u_int32_t status = fe_read(sc, RA_FE_INT_STATUS);
		fe_write(sc, RA_FE_INT_STATUS, ~0);
		RALINK_DEBUG(RALINK_DEBUG_REG,"%s() status: 0x%08x\n",
		    __func__, status);
#if defined(MT7628)
		if ((status & (RA_FE_INT_RX_DONE_INT1 | RA_FE_INT_RX_DONE_INT0 |
		    RA_FE_INT_TX_DONE_INT3 | RA_FE_INT_TX_DONE_INT2 |
		    RA_FE_INT_TX_DONE_INT1 | RA_FE_INT_TX_DONE_INT0)) == 0) {
			if (n == 0)
				sc->sc_evcnt_spurious_intr.ev_count++;
			return (n != 0);
		}

		if (status & (RA_FE_INT_RX_DONE_INT1|RA_FE_INT_RX_DONE_INT0))
			ralink_eth_rxintr(sc);

		if (status & (RA_FE_INT_TX_DONE_INT3 | RA_FE_INT_TX_DONE_INT2 |
		    RA_FE_INT_TX_DONE_INT1 | RA_FE_INT_TX_DONE_INT0))
			ralink_eth_txintr(sc);
#else
		if ((status & (FE_INT_RX | FE_INT_TX0)) == 0) {
			if (n == 0)
				sc->sc_evcnt_spurious_intr.ev_count++;
			return (n != 0);
		}

		if (status & FE_INT_RX)
			ralink_eth_rxintr(sc);

		if (status & FE_INT_TX0)
			ralink_eth_txintr(sc);
#endif
	}

	/* Try to get more packets going. */
	if_schedule_deferred_start(&sc->sc_ethercom.ec_if);

	return 1;
}

/*
 * ralink_eth_rxintr
 */
static void
ralink_eth_rxintr(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();
	struct ifnet * const ifp = &sc->sc_ethercom.ec_if;
	struct ralink_eth_rxstate *rxs;
	struct mbuf *m;
	int len;
	int rx_cpu_idx;

	KASSERT(curcpu()->ci_cpl >= IPL_NET);
	sc->sc_evcnt_rxintr.ev_count++;
	rx_cpu_idx = fe_read(sc, RA_FE_PDMA_RX0_CPU_IDX);

	for (;;)  {
		rx_cpu_idx = (rx_cpu_idx + 1) % RALINK_ETH_NUM_RX_DESC;

		rxs = &sc->sc_rxstate[rx_cpu_idx];

		bus_dmamap_sync(sc->sc_dmat, sc->sc_pdmamap,
		    (int)&sc->sc_rxdesc[rx_cpu_idx] - (int)sc->sc_descs,
		    sizeof(struct ralink_rx_desc),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		RALINK_DEBUG(RALINK_DEBUG_REG,"rx(%d) 0x%08x: 0x%08x\n",
		    rx_cpu_idx, (int)&sc->sc_rxdesc[rx_cpu_idx].data_ptr,
		    sc->sc_rxdesc[rx_cpu_idx].data_ptr);
		RALINK_DEBUG(RALINK_DEBUG_REG,"rx(%d) 0x%08x: 0x%08x\n",
		    rx_cpu_idx, (int)&sc->sc_rxdesc[rx_cpu_idx].rxd_info1,
		    sc->sc_rxdesc[rx_cpu_idx].rxd_info1);
		RALINK_DEBUG(RALINK_DEBUG_REG,"rx(%d) 0x%08x: 0x%08x\n",
		    rx_cpu_idx, (int)&sc->sc_rxdesc[rx_cpu_idx].unused,
		    sc->sc_rxdesc[rx_cpu_idx].unused);
		RALINK_DEBUG(RALINK_DEBUG_REG,"rx(%d) 0x%08x: 0x%08x\n",
		    rx_cpu_idx, (int)&sc->sc_rxdesc[rx_cpu_idx].rxd_info2,
		    sc->sc_rxdesc[rx_cpu_idx].rxd_info2);

		if (!(sc->sc_rxdesc[rx_cpu_idx].rxd_info1 & RXD_DDONE))
			break;

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		/*
		 * No errors; receive the packet.
		 * Note the chip includes the CRC with every packet.
		 */
		len = RXD_LEN0(sc->sc_rxdesc[rx_cpu_idx].rxd_info1);

		RALINK_DEBUG(RALINK_DEBUG_REG,"rx(%d) packet rx %d bytes\n",
		    rx_cpu_idx, len);

		/*
		 * Allocate a new mbuf cluster.  If that fails, we are
		 * out of memory, and must drop the packet and recycle
		 * the buffer that's already attached to this descriptor.
		 */
		m = rxs->rxs_mbuf;
		if (ralink_eth_add_rxbuf(sc, rx_cpu_idx) != 0)
			break;
		m->m_data += RALINK_ETHER_ALIGN;
		m->m_pkthdr.len = m->m_len = len;

#ifdef RALINK_ETH_DEBUG
 {
		struct ether_header *eh = mtod(m, struct ether_header *);
		printf("rx: eth_dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("rx: eth_src: %s type: 0x%04x \n",
		    ether_sprintf(eh->ether_shost), ntohs(eh->ether_type));
		printf("0x14: %08x\n", *(volatile unsigned int *)(0xb0110014));
		printf("0x98: %08x\n", *(volatile unsigned int *)(0xb0110098));

		unsigned char * s = mtod(m, unsigned char *);
		for (int j = 0; j < 32; j++)
			printf("%02x%c", *(s + j),
				(j == 15 || j == 31) ? '\n' : ' ');
 }
#endif

		/*
		 * claim the buffer here since we can't do it at
		 * allocation time due to the SW partitions
		 */
		MCLAIM(m, &sc->sc_ethercom.ec_rx_mowner);

		/* push it up the inteface */
		m_set_rcvif(m, ifp);

#ifdef RALINK_ETH_DEBUG
 {
		struct ether_header *eh = mtod(m, struct ether_header *);
		printf("rx: eth_dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("rx: eth_src: %s type: 0x%04x\n",
		    ether_sprintf(eh->ether_shost), ntohs(eh->ether_type));
		printf("0x14: %08x\n", *(volatile unsigned int *)(0xb0110014));
		printf("0x98: %08x\n", *(volatile unsigned int *)(0xb0110098));

		unsigned char * s = mtod(m, unsigned char *);
		for (int j = 0; j < 32; j++)
			printf("%02x%c", *(s + j),
			    (j == 15 || j == 31) ? '\n' : ' ');
 }
#endif

		/*
		 * XXX: M_CSUM_TCPv4 and M_CSUM_UDPv4 do not currently work when
		 * using PF's ROUTETO option for load balancing.
		 */
		m->m_pkthdr.csum_flags |= M_CSUM_IPv4;

		/* Pass it on. */
		sc->sc_evcnt_input.ev_count++;
		if_percpuq_enqueue(ifp->if_percpuq, m);

		fe_write(sc, RA_FE_PDMA_RX0_CPU_IDX, rx_cpu_idx);
	}
}

/*
 * ralink_eth_txintr
 */
static void
ralink_eth_txintr(ralink_eth_softc_t *sc)
{
	RALINK_DEBUG_FUNC_ENTRY();
	struct ralink_eth_txstate *txs;

	KASSERT(curcpu()->ci_cpl >= IPL_NET);
	sc->sc_evcnt_txintr.ev_count++;

	/*
	 * Go through our Tx list and free mbufs for those
	 * frames that have been transmitted.
	 */
	while ((txs = SIMPLEQ_FIRST(&sc->sc_txdirtyq)) != NULL) {
		bus_dmamap_sync(sc->sc_dmat, sc->sc_pdmamap,
		    (int)&sc->sc_txdesc[txs->txs_idx] - (int)sc->sc_descs,
		    sizeof(struct ralink_tx_desc),
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		RALINK_DEBUG(RALINK_DEBUG_REG,"-tx(%d) 0x%08x: 0x%08x\n",
		    txs->txs_idx, (int)&sc->sc_txdesc[txs->txs_idx].data_ptr0,
		    sc->sc_txdesc[txs->txs_idx].data_ptr0);
		RALINK_DEBUG(RALINK_DEBUG_REG,"-tx(%d) 0x%08x: 0x%08x\n",
		    txs->txs_idx, (int)&sc->sc_txdesc[txs->txs_idx].txd_info1,
		    sc->sc_txdesc[txs->txs_idx].txd_info1);
		RALINK_DEBUG(RALINK_DEBUG_REG,"-tx(%d) 0x%08x: 0x%08x\n",
		    txs->txs_idx, (int)&sc->sc_txdesc[txs->txs_idx].data_ptr1,
		    sc->sc_txdesc[txs->txs_idx].data_ptr1);
		RALINK_DEBUG(RALINK_DEBUG_REG,"-tx(%d) 0x%08x: 0x%08x\n",
		    txs->txs_idx, (int)&sc->sc_txdesc[txs->txs_idx].txd_info2,
		    sc->sc_txdesc[txs->txs_idx].txd_info2);

		/* we're finished if the current tx isn't done */
		if (!(sc->sc_txdesc[txs->txs_idx].txd_info1 & TXD_DDONE))
			break;

		RALINK_DEBUG(RALINK_DEBUG_REG,"-tx(%d) transmitted\n",
		   txs->txs_idx);

		SIMPLEQ_REMOVE_HEAD(&sc->sc_txdirtyq, txs_q);

		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap, 0,
		    txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;

		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);

		struct ifnet *ifp = &sc->sc_ethercom.ec_if;
		ifp->if_flags &= ~IFF_OACTIVE;
		ifp->if_opackets++;
		sc->sc_evcnt_output.ev_count++;

		if (--sc->sc_pending_tx == 0)
			ifp->if_timer = 0;
	}
}

/*
 * ralink_eth_mdio_enable
 */
#if defined(RT3050) || defined(RT3052)
static void
ralink_eth_mdio_enable(ralink_eth_softc_t *sc, bool enable)
{
	uint32_t data = sy_read(sc, RA_SYSCTL_GPIOMODE);

	if (enable)
		data &= ~GPIOMODE_MDIO;
	else
		data |= GPIOMODE_MDIO;

	sy_write(sc, RA_SYSCTL_GPIOMODE, data);
}
#else
#define ralink_eth_mdio_enable(sc, enable)
#endif

/*
 * ralink_eth_mii_statchg
 */
static void
ralink_eth_mii_statchg(struct ifnet *ifp)
{
#if 0
	ralink_eth_softc_t * const sc = ifp->if_softc;

#endif
}

/*
 * ralink_eth_mii_tick
 *
 *	One second timer, used to tick the MIIs.
 */
static void
ralink_eth_mii_tick(void *arg)
{
	ralink_eth_softc_t * const sc = arg;

	const int s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_callout, hz, ralink_eth_mii_tick, sc);
}

/*
 * ralink_eth_mii_read
 */
static int
ralink_eth_mii_read(device_t self, int phy_addr, int phy_reg)
{
	ralink_eth_softc_t *sc = device_private(self);
	KASSERT(sc != NULL);
#if 0
	printf("%s() phy_addr: %d  phy_reg: %d\n", __func__, phy_addr, phy_reg);
#endif
#if defined(RT3050) || defined(RT3052) || defined(MT7628)
	if (phy_addr > 5)
		return 0;
#endif

	/* We enable mdio gpio purpose register, and disable it when exit. */
	ralink_eth_mdio_enable(sc, true);

	/*
	 * make sure previous read operation is complete
	 * TODO: timeout (linux uses jiffies to measure 5 seconds)
	 */
	for (;;) {
		/* rd_rdy: read operation is complete */
#if defined(RT3050) || defined(RT3052) || defined(MT7628)
		if ((sw_read(sc, RA_ETH_SW_PCTL1) & PCTL1_RD_DONE) == 0)
			break;
#else
		if ((fe_read(sc, RA_FE_MDIO_ACCESS) & MDIO_ACCESS_TRG) == 0)
			break;
#endif
	}

#if defined(RT3050) || defined(RT3052) || defined(MT7628)
	sw_write(sc, RA_ETH_SW_PCTL0,
	    PCTL0_RD_CMD | PCTL0_REG(phy_reg) | PCTL0_ADDR(phy_addr));
#else
	fe_write(sc, RA_FE_MDIO_ACCESS,
	    MDIO_ACCESS_PHY_ADDR(phy_addr) | MDIO_ACCESS_REG(phy_reg));
	fe_write(sc, RA_FE_MDIO_ACCESS,
	    MDIO_ACCESS_PHY_ADDR(phy_addr) | MDIO_ACCESS_REG(phy_reg) |
	    MDIO_ACCESS_TRG);
#endif

	/*
	 * make sure read operation is complete
	 * TODO: timeout (linux uses jiffies to measure 5 seconds)
	 */
	for (;;) {
#if defined(RT3050) || defined(RT3052) || defined(MT7628)
		if ((sw_read(sc, RA_ETH_SW_PCTL1) & PCTL1_RD_DONE) != 0) {
			int data = PCTL1_RD_VAL(
			    sw_read(sc, RA_ETH_SW_PCTL1));
			ralink_eth_mdio_enable(sc, false);
			return data;
		}
#else
		if ((fe_read(sc, RA_FE_MDIO_ACCESS) & MDIO_ACCESS_TRG) == 0) {
			int data = MDIO_ACCESS_DATA(
			    fe_read(sc, RA_FE_MDIO_ACCESS));
			ralink_eth_mdio_enable(sc, false);
			return data;
		}
#endif
	}
}

/*
 * ralink_eth_mii_write
 */
static void
ralink_eth_mii_write(device_t self, int phy_addr, int phy_reg, int val)
{
	ralink_eth_softc_t *sc = device_private(self);
	KASSERT(sc != NULL);
#if 0
	printf("%s() phy_addr: %d  phy_reg: %d  val: 0x%04x\n",
	    __func__, phy_addr, phy_reg, val);
#endif
	ralink_eth_mdio_enable(sc, true);

	/*
	 * make sure previous write operation is complete
	 * TODO: timeout (linux uses jiffies to measure 5 seconds)
	 */
	for (;;) {
#if defined(RT3050) || defined(RT3052) || defined(MT7628)
		if ((sw_read(sc, RA_ETH_SW_PCTL1) & PCTL1_RD_DONE) == 0)
			break;
#else
		if ((fe_read(sc, RA_FE_MDIO_ACCESS) & MDIO_ACCESS_TRG) == 0)
			break;
#endif
	}

#if defined(RT3050) || defined(RT3052) || defined(MT7628)
	sw_write(sc, RA_ETH_SW_PCTL0,
	    PCTL0_WR_CMD | PCTL0_WR_VAL(val) | PCTL0_REG(phy_reg) |
	    PCTL0_ADDR(phy_addr));
#else
	fe_write(sc, RA_FE_MDIO_ACCESS,
	    MDIO_ACCESS_WR | MDIO_ACCESS_PHY_ADDR(phy_addr) |
	    MDIO_ACCESS_REG(phy_reg) | MDIO_ACCESS_DATA(val));
	fe_write(sc, RA_FE_MDIO_ACCESS,
	    MDIO_ACCESS_WR | MDIO_ACCESS_PHY_ADDR(phy_addr) |
	    MDIO_ACCESS_REG(phy_reg) | MDIO_ACCESS_DATA(val) |
	    MDIO_ACCESS_TRG);
#endif


	/* make sure write operation is complete */
	for (;;) {
#if defined(RT3050) || defined(RT3052) || defined(MT7628)
		if ((sw_read(sc, RA_ETH_SW_PCTL1) & PCTL1_WR_DONE) != 0) {
			ralink_eth_mdio_enable(sc, false);
			return;
		}
#else
		if ((fe_read(sc, RA_FE_MDIO_ACCESS) & MDIO_ACCESS_TRG) == 0){
			ralink_eth_mdio_enable(sc, false);
			return;
		}
#endif
	}
}
