/*	$NetBSD: if_sip.c,v 1.24.2.7 2002/04/01 07:46:22 nathanw Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * Device driver for the Silicon Integrated Systems SiS 900,
 * SiS 7016 10/100, National Semiconductor DP83815 10/100, and
 * National Semiconductor DP83820 10/100/1000 PCI Ethernet
 * controllers.
 *    
 * Originally written to support the SiS 900 by Jason R. Thorpe for
 * Network Computer, Inc.
 *
 * TODO:
 *
 *	- Support the 10-bit interface on the DP83820 (for fiber).
 *
 *	- Reduce the interrupt load.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_sip.c,v 1.24.2.7 2002/04/01 07:46:22 nathanw Exp $");

#include "bpfilter.h"

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

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/endian.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#ifdef DP83820
#include <dev/mii/mii_bitbang.h>
#endif /* DP83820 */

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>
#include <dev/pci/pcidevs.h>

#include <dev/pci/if_sipreg.h>

#ifdef DP83820		/* DP83820 Gigabit Ethernet */
#define	SIP_DECL(x)	__CONCAT(gsip_,x)
#else			/* SiS900 and DP83815 */
#define	SIP_DECL(x)	__CONCAT(sip_,x)
#endif

#define	SIP_STR(x)	__STRING(SIP_DECL(x))

/*
 * Transmit descriptor list size.  This is arbitrary, but allocate
 * enough descriptors for 128 pending transmissions, and 8 segments
 * per packet.  This MUST work out to a power of 2.
 */
#define	SIP_NTXSEGS		8

#define	SIP_TXQUEUELEN		256
#define	SIP_NTXDESC		(SIP_TXQUEUELEN * SIP_NTXSEGS)
#define	SIP_NTXDESC_MASK	(SIP_NTXDESC - 1)
#define	SIP_NEXTTX(x)		(((x) + 1) & SIP_NTXDESC_MASK)

#if defined(DP83020)
#define	TX_DMAMAP_SIZE		ETHER_MAX_LEN_JUMBO
#else
#define	TX_DMAMAP_SIZE		MCLBYTES
#endif

/*
 * Receive descriptor list size.  We have one Rx buffer per incoming
 * packet, so this logic is a little simpler.
 *
 * Actually, on the DP83820, we allow the packet to consume more than
 * one buffer, in order to support jumbo Ethernet frames.  In that
 * case, a packet may consume up to 5 buffers (assuming a 2048 byte
 * mbuf cluster).  256 receive buffers is only 51 maximum size packets,
 * so we'd better be quick about handling receive interrupts.
 */
#if defined(DP83820)
#define	SIP_NRXDESC		256
#else
#define	SIP_NRXDESC		128
#endif /* DP83820 */
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

	const struct sip_product *sc_model; /* which model are we? */
	int sc_rev;			/* chip revision */

	void *sc_ih;			/* interrupt cookie */

	struct mii_data sc_mii;		/* MII/media information */

	struct callout sc_tick_ch;	/* tick callout */

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

#ifdef SIP_EVENT_COUNTERS
	/*
	 * Event counters.
	 */
	struct evcnt sc_ev_txsstall;	/* Tx stalled due to no txs */
	struct evcnt sc_ev_txdstall;	/* Tx stalled due to no txd */
	struct evcnt sc_ev_txintr;	/* Tx interrupts */
	struct evcnt sc_ev_rxintr;	/* Rx interrupts */
#ifdef DP83820
	struct evcnt sc_ev_rxipsum;	/* IP checksums checked in-bound */
	struct evcnt sc_ev_rxtcpsum;	/* TCP checksums checked in-bound */
	struct evcnt sc_ev_rxudpsum;	/* UDP checksums checked in-boudn */
	struct evcnt sc_ev_txipsum;	/* IP checksums comp. out-bound */
	struct evcnt sc_ev_txtcpsum;	/* TCP checksums comp. out-bound */
	struct evcnt sc_ev_txudpsum;	/* UDP checksums comp. out-bound */
#endif /* DP83820 */
#endif /* SIP_EVENT_COUNTERS */

	u_int32_t sc_txcfg;		/* prototype TXCFG register */
	u_int32_t sc_rxcfg;		/* prototype RXCFG register */
	u_int32_t sc_imr;		/* prototype IMR register */
	u_int32_t sc_rfcr;		/* prototype RFCR register */

	u_int32_t sc_cfg;		/* prototype CFG register */

#ifdef DP83820
	u_int32_t sc_gpior;		/* prototype GPIOR register */
#endif /* DP83820 */

	u_int32_t sc_tx_fill_thresh;	/* transmit fill threshold */
	u_int32_t sc_tx_drain_thresh;	/* transmit drain threshold */

	u_int32_t sc_rx_drain_thresh;	/* receive drain threshold */

	int	sc_flags;		/* misc. flags; see below */

	int	sc_txfree;		/* number of free Tx descriptors */
	int	sc_txnext;		/* next ready Tx descriptor */

	struct sip_txsq sc_txfreeq;	/* free Tx descsofts */
	struct sip_txsq sc_txdirtyq;	/* dirty Tx descsofts */

	int	sc_rxptr;		/* next ready Rx descriptor/descsoft */
#if defined(DP83820)
	int	sc_rxdiscard;
	int	sc_rxlen;
	struct mbuf *sc_rxhead;
	struct mbuf *sc_rxtail;
	struct mbuf **sc_rxtailp;
#endif /* DP83820 */
};

/* sc_flags */
#define	SIPF_PAUSED	0x00000001	/* paused (802.3x flow control) */

#ifdef DP83820
#define	SIP_RXCHAIN_RESET(sc)						\
do {									\
	(sc)->sc_rxtailp = &(sc)->sc_rxhead;				\
	*(sc)->sc_rxtailp = NULL;					\
	(sc)->sc_rxlen = 0;						\
} while (/*CONSTCOND*/0)

#define	SIP_RXCHAIN_LINK(sc, m)						\
do {									\
	*(sc)->sc_rxtailp = (sc)->sc_rxtail = (m);			\
	(sc)->sc_rxtailp = &(m)->m_next;				\
} while (/*CONSTCOND*/0)
#endif /* DP83820 */

#ifdef SIP_EVENT_COUNTERS
#define	SIP_EVCNT_INCR(ev)	(ev)->ev_count++
#else
#define	SIP_EVCNT_INCR(ev)	/* nothing */
#endif

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

#ifdef DP83820
#define	SIP_INIT_RXDESC_EXTSTS	__sipd->sipd_extsts = 0;
#define	SIP_RXBUF_LEN		(MCLBYTES - 4)
#else
#define	SIP_INIT_RXDESC_EXTSTS	/* nothing */
#define	SIP_RXBUF_LEN		(MCLBYTES - 1)	/* field width */
#endif
#define	SIP_INIT_RXDESC(sc, x)						\
do {									\
	struct sip_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct sip_desc *__sipd = &(sc)->sc_rxdescs[(x)];		\
									\
	__sipd->sipd_link =						\
	    htole32(SIP_CDRXADDR((sc), SIP_NEXTRX((x))));		\
	__sipd->sipd_bufptr =						\
	    htole32(__rxs->rxs_dmamap->dm_segs[0].ds_addr);		\
	__sipd->sipd_cmdsts = htole32(CMDSTS_INTR |			\
	    (SIP_RXBUF_LEN & CMDSTS_SIZE_MASK));			\
	SIP_INIT_RXDESC_EXTSTS						\
	SIP_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (0)

#define	SIP_CHIP_VERS(sc, v, p, r)					\
	((sc)->sc_model->sip_vendor == (v) &&				\
	 (sc)->sc_model->sip_product == (p) &&				\
	 (sc)->sc_rev == (r))

#define	SIP_CHIP_MODEL(sc, v, p)					\
	((sc)->sc_model->sip_vendor == (v) &&				\
	 (sc)->sc_model->sip_product == (p))

#if !defined(DP83820)
#define	SIP_SIS900_REV(sc, rev)						\
	SIP_CHIP_VERS((sc), PCI_VENDOR_SIS, PCI_PRODUCT_SIS_900, (rev))
#endif

#define SIP_TIMEOUT 1000

void	SIP_DECL(start)(struct ifnet *);
void	SIP_DECL(watchdog)(struct ifnet *);
int	SIP_DECL(ioctl)(struct ifnet *, u_long, caddr_t);
int	SIP_DECL(init)(struct ifnet *);
void	SIP_DECL(stop)(struct ifnet *, int);

void	SIP_DECL(shutdown)(void *);

void	SIP_DECL(reset)(struct sip_softc *);
void	SIP_DECL(rxdrain)(struct sip_softc *);
int	SIP_DECL(add_rxbuf)(struct sip_softc *, int);
void	SIP_DECL(read_eeprom)(struct sip_softc *, int, int, u_int16_t *);
void	SIP_DECL(tick)(void *);

#if !defined(DP83820)
void	SIP_DECL(sis900_set_filter)(struct sip_softc *);
#endif /* ! DP83820 */
void	SIP_DECL(dp83815_set_filter)(struct sip_softc *);

#if defined(DP83820)
void	SIP_DECL(dp83820_read_macaddr)(struct sip_softc *,
	    const struct pci_attach_args *, u_int8_t *);
#else
void	SIP_DECL(sis900_read_macaddr)(struct sip_softc *,
	    const struct pci_attach_args *, u_int8_t *);
void	SIP_DECL(dp83815_read_macaddr)(struct sip_softc *,
	    const struct pci_attach_args *, u_int8_t *);
#endif /* DP83820 */

int	SIP_DECL(intr)(void *);
void	SIP_DECL(txintr)(struct sip_softc *);
void	SIP_DECL(rxintr)(struct sip_softc *);

#if defined(DP83820)
int	SIP_DECL(dp83820_mii_readreg)(struct device *, int, int);
void	SIP_DECL(dp83820_mii_writereg)(struct device *, int, int, int);
void	SIP_DECL(dp83820_mii_statchg)(struct device *);
#else
int	SIP_DECL(sis900_mii_readreg)(struct device *, int, int);
void	SIP_DECL(sis900_mii_writereg)(struct device *, int, int, int);
void	SIP_DECL(sis900_mii_statchg)(struct device *);

int	SIP_DECL(dp83815_mii_readreg)(struct device *, int, int);
void	SIP_DECL(dp83815_mii_writereg)(struct device *, int, int, int);
void	SIP_DECL(dp83815_mii_statchg)(struct device *);
#endif /* DP83820 */

int	SIP_DECL(mediachange)(struct ifnet *);
void	SIP_DECL(mediastatus)(struct ifnet *, struct ifmediareq *);

int	SIP_DECL(match)(struct device *, struct cfdata *, void *);
void	SIP_DECL(attach)(struct device *, struct device *, void *);

int	SIP_DECL(copy_small) = 0;

struct cfattach SIP_DECL(ca) = {
	sizeof(struct sip_softc), SIP_DECL(match), SIP_DECL(attach),
};

/*
 * Descriptions of the variants of the SiS900.
 */
struct sip_variant {
	int	(*sipv_mii_readreg)(struct device *, int, int);
	void	(*sipv_mii_writereg)(struct device *, int, int, int);
	void	(*sipv_mii_statchg)(struct device *);
	void	(*sipv_set_filter)(struct sip_softc *);
	void	(*sipv_read_macaddr)(struct sip_softc *, 
		    const struct pci_attach_args *, u_int8_t *);
};

#if defined(DP83820)
u_int32_t SIP_DECL(dp83820_mii_bitbang_read)(struct device *);
void	SIP_DECL(dp83820_mii_bitbang_write)(struct device *, u_int32_t);

const struct mii_bitbang_ops SIP_DECL(dp83820_mii_bitbang_ops) = {
	SIP_DECL(dp83820_mii_bitbang_read),
	SIP_DECL(dp83820_mii_bitbang_write),
	{
		EROMAR_MDIO,		/* MII_BIT_MDO */
		EROMAR_MDIO,		/* MII_BIT_MDI */
		EROMAR_MDC,		/* MII_BIT_MDC */
		EROMAR_MDDIR,		/* MII_BIT_DIR_HOST_PHY */
		0,			/* MII_BIT_DIR_PHY_HOST */
	}
};
#endif /* DP83820 */

#if defined(DP83820)
const struct sip_variant SIP_DECL(variant_dp83820) = {
	SIP_DECL(dp83820_mii_readreg),
	SIP_DECL(dp83820_mii_writereg),
	SIP_DECL(dp83820_mii_statchg),
	SIP_DECL(dp83815_set_filter),
	SIP_DECL(dp83820_read_macaddr),
};
#else
const struct sip_variant SIP_DECL(variant_sis900) = {
	SIP_DECL(sis900_mii_readreg),
	SIP_DECL(sis900_mii_writereg),
	SIP_DECL(sis900_mii_statchg),
	SIP_DECL(sis900_set_filter),
	SIP_DECL(sis900_read_macaddr),
};

const struct sip_variant SIP_DECL(variant_dp83815) = {
	SIP_DECL(dp83815_mii_readreg),
	SIP_DECL(dp83815_mii_writereg),
	SIP_DECL(dp83815_mii_statchg),
	SIP_DECL(dp83815_set_filter),
	SIP_DECL(dp83815_read_macaddr),
};
#endif /* DP83820 */

/*
 * Devices supported by this driver.
 */
const struct sip_product {
	pci_vendor_id_t		sip_vendor;
	pci_product_id_t	sip_product;
	const char		*sip_name;
	const struct sip_variant *sip_variant;
} SIP_DECL(products)[] = {
#if defined(DP83820)
	{ PCI_VENDOR_NS,	PCI_PRODUCT_NS_DP83820,
	  "NatSemi DP83820 Gigabit Ethernet",
	  &SIP_DECL(variant_dp83820) },
#else
	{ PCI_VENDOR_SIS,	PCI_PRODUCT_SIS_900,
	  "SiS 900 10/100 Ethernet",
	  &SIP_DECL(variant_sis900) },
	{ PCI_VENDOR_SIS,	PCI_PRODUCT_SIS_7016,
	  "SiS 7016 10/100 Ethernet",
	  &SIP_DECL(variant_sis900) },

	{ PCI_VENDOR_NS,	PCI_PRODUCT_NS_DP83815,
	  "NatSemi DP83815 10/100 Ethernet",
	  &SIP_DECL(variant_dp83815) },
#endif /* DP83820 */

	{ 0,			0,
	  NULL,
	  NULL },
};

static const struct sip_product *
SIP_DECL(lookup)(const struct pci_attach_args *pa)
{
	const struct sip_product *sip;

	for (sip = SIP_DECL(products); sip->sip_name != NULL; sip++) {
		if (PCI_VENDOR(pa->pa_id) == sip->sip_vendor &&
		    PCI_PRODUCT(pa->pa_id) == sip->sip_product)
			return (sip);
	}
	return (NULL);
}

int
SIP_DECL(match)(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pci_attach_args *pa = aux;

	if (SIP_DECL(lookup)(pa) != NULL)
		return (1);

	return (0);
}

void
SIP_DECL(attach)(struct device *parent, struct device *self, void *aux)
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
	u_int8_t enaddr[ETHER_ADDR_LEN];
	int pmreg;
#ifdef DP83820
	pcireg_t memtype;
	u_int32_t reg;
#endif /* DP83820 */

	callout_init(&sc->sc_tick_ch);

	sip = SIP_DECL(lookup)(pa);
	if (sip == NULL) {
		printf("\n");
		panic(SIP_STR(attach) ": impossible");
	}
	sc->sc_rev = PCI_REVISION(pa->pa_class);

	printf(": %s, rev %#02x\n", sip->sip_name, sc->sc_rev);

	sc->sc_model = sip;

	/*
	 * XXX Work-around broken PXE firmware on some boards.
	 *
	 * The DP83815 shares an address decoder with the MEM BAR
	 * and the ROM BAR.  Make sure the ROM BAR is disabled,
	 * so that memory mapped access works.
	 */
	pci_conf_write(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM,
	    pci_conf_read(pa->pa_pc, pa->pa_tag, PCI_MAPREG_ROM) &
	    ~PCI_MAPREG_ROM_ENABLE);

	/*
	 * Map the device.
	 */
	ioh_valid = (pci_mapreg_map(pa, SIP_PCI_CFGIOA,
	    PCI_MAPREG_TYPE_IO, 0,
	    &iot, &ioh, NULL, NULL) == 0);
#ifdef DP83820
	memtype = pci_mapreg_type(pa->pa_pc, pa->pa_tag, SIP_PCI_CFGMA);
	switch (memtype) {
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT:
	case PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_64BIT:
		memh_valid = (pci_mapreg_map(pa, SIP_PCI_CFGMA,
		    memtype, 0, &memt, &memh, NULL, NULL) == 0);
		break;
	default:
		memh_valid = 0;
	}
#else
	memh_valid = (pci_mapreg_map(pa, SIP_PCI_CFGMA,
	    PCI_MAPREG_TYPE_MEM|PCI_MAPREG_MEM_TYPE_32BIT, 0,
	    &memt, &memh, NULL, NULL) == 0);
#endif /* DP83820 */

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

	/*
	 * Make sure bus mastering is enabled.  Also make sure
	 * Write/Invalidate is enabled if we're allowed to use it.
	 */
	pmreg = pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG);
	if (pa->pa_flags & PCI_FLAGS_MWI_OKAY)
		pmreg |= PCI_COMMAND_INVALIDATE_ENABLE;
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pmreg | PCI_COMMAND_MASTER_ENABLE);

	/* Get it out of power save mode if needed. */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, &pmreg, 0)) {
		pmode = pci_conf_read(pc, pa->pa_tag, pmreg + 4) & 0x3;
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
			pci_conf_write(pc, pa->pa_tag, pmreg + 4, 0);
		}
	}

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, SIP_DECL(intr), sc);
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
		if ((error = bus_dmamap_create(sc->sc_dmat, TX_DMAMAP_SIZE,
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
	SIP_DECL(reset)(sc);

	/*
	 * Read the Ethernet address from the EEPROM.  This might
	 * also fetch other stuff from the EEPROM and stash it
	 * in the softc.
	 */
	sc->sc_cfg = 0;
#if !defined(DP83820)
	if (SIP_SIS900_REV(sc,SIS_REV_635) ||
	    SIP_SIS900_REV(sc,SIS_REV_900B))
		sc->sc_cfg |= (CFG_PESEL | CFG_RNDCNT);
#endif

	(*sip->sip_variant->sipv_read_macaddr)(sc, pa, enaddr);

	printf("%s: Ethernet address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(enaddr));

	/*
	 * Initialize the configuration register: aggressive PCI
	 * bus request algorithm, default backoff, default OW timer,
	 * default parity error detection.
	 *
	 * NOTE: "Big endian mode" is useless on the SiS900 and
	 * friends -- it affects packet data, not descriptors.
	 */
#ifdef DP83820
	reg = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_CFG);
	if (reg & CFG_PCI64_DET) {
		printf("%s: 64-bit PCI slot detected\n", sc->sc_dev.dv_xname);
		/*
		 * XXX Need some PCI flags indicating support for
		 * XXX 64-bit addressing (SAC or DAC) and 64-bit
		 * XXX data path.
		 */
	}
	if (sc->sc_cfg & (CFG_TBI_EN|CFG_EXT_125)) {
		const char *sep = "";
		printf("%s: using ", sc->sc_dev.dv_xname);
		if (sc->sc_cfg & CFG_EXT_125) {
			printf("%s125MHz clock", sep);
			sep = ", ";
		}
		if (sc->sc_cfg & CFG_TBI_EN) {
			printf("%sten-bit interface", sep);
			sep = ", ";
		}
		printf("\n");
	}
	if ((pa->pa_flags & PCI_FLAGS_MRM_OKAY) == 0)
		sc->sc_cfg |= CFG_MRM_DIS;
	if ((pa->pa_flags & PCI_FLAGS_MWI_OKAY) == 0)
		sc->sc_cfg |= CFG_MWI_DIS;

	/*
	 * Use the extended descriptor format on the DP83820.  This
	 * gives us an interface to VLAN tagging and IPv4/TCP/UDP
	 * checksumming.
	 */
	sc->sc_cfg |= CFG_EXTSTS_EN;
#endif /* DP83820 */

	/*
	 * Initialize our media structures and probe the MII.
	 */
	sc->sc_mii.mii_ifp = ifp;
	sc->sc_mii.mii_readreg = sip->sip_variant->sipv_mii_readreg;
	sc->sc_mii.mii_writereg = sip->sip_variant->sipv_mii_writereg;
	sc->sc_mii.mii_statchg = sip->sip_variant->sipv_mii_statchg;
	ifmedia_init(&sc->sc_mii.mii_media, 0, SIP_DECL(mediachange),
	    SIP_DECL(mediastatus));
#ifdef DP83820
	if (sc->sc_cfg & CFG_TBI_EN) {
		/* Using ten-bit interface. */
		printf("%s: TBI -- FIXME\n", sc->sc_dev.dv_xname);
	} else {
		mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
		    MII_OFFSET_ANY, 0);
		if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
			ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE,
			    0, NULL);
			ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
		} else
			ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
	}
#else
	mii_attach(&sc->sc_dev, &sc->sc_mii, 0xffffffff, MII_PHY_ANY,
	    MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&sc->sc_mii.mii_phys) == NULL) {
		ifmedia_add(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&sc->sc_mii.mii_media, IFM_ETHER|IFM_AUTO);
#endif /* DP83820 */

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = SIP_DECL(ioctl);
	ifp->if_start = SIP_DECL(start);
	ifp->if_watchdog = SIP_DECL(watchdog);
	ifp->if_init = SIP_DECL(init);
	ifp->if_stop = SIP_DECL(stop);
	IFQ_SET_READY(&ifp->if_snd);

	/*
	 * We can support 802.1Q VLAN-sized frames.
	 */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;

#ifdef DP83820
	/*
	 * And the DP83820 can do VLAN tagging in hardware, and
	 * support the jumbo Ethernet MTU.
	 */
	sc->sc_ethercom.ec_capabilities |=
	    ETHERCAP_VLAN_HWTAGGING | ETHERCAP_JUMBO_MTU;

	/*
	 * The DP83820 can do IPv4, TCPv4, and UDPv4 checksums
	 * in hardware.
	 */
	ifp->if_capabilities |= IFCAP_CSUM_IPv4 | IFCAP_CSUM_TCPv4 |
	    IFCAP_CSUM_UDPv4;
#endif /* DP83820 */

	/*
	 * Attach the interface.
	 */
	if_attach(ifp);
	ether_ifattach(ifp, enaddr);

	/*
	 * The number of bytes that must be available in
	 * the Tx FIFO before the bus master can DMA more
	 * data into the FIFO.
	 */
	sc->sc_tx_fill_thresh = 64 / 32;

	/*
	 * Start at a drain threshold of 512 bytes.  We will
	 * increase it if a DMA underrun occurs.
	 *
	 * XXX The minimum value of this variable should be
	 * tuned.  We may be able to improve performance
	 * by starting with a lower value.  That, however,
	 * may trash the first few outgoing packets if the
	 * PCI bus is saturated.
	 */
	sc->sc_tx_drain_thresh = 512 / 32;

	/*
	 * Initialize the Rx FIFO drain threshold.
	 *
	 * This is in units of 8 bytes.
	 *
	 * We should never set this value lower than 2; 14 bytes are
	 * required to filter the packet.
	 */
	sc->sc_rx_drain_thresh = 128 / 8;

#ifdef SIP_EVENT_COUNTERS
	/*
	 * Attach event counters.
	 */
	evcnt_attach_dynamic(&sc->sc_ev_txsstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txsstall");
	evcnt_attach_dynamic(&sc->sc_ev_txdstall, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txdstall");
	evcnt_attach_dynamic(&sc->sc_ev_txintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "txintr");
	evcnt_attach_dynamic(&sc->sc_ev_rxintr, EVCNT_TYPE_INTR,
	    NULL, sc->sc_dev.dv_xname, "rxintr");
#ifdef DP83820
	evcnt_attach_dynamic(&sc->sc_ev_rxipsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxipsum");
	evcnt_attach_dynamic(&sc->sc_ev_rxtcpsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxtcpsum");
	evcnt_attach_dynamic(&sc->sc_ev_rxudpsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "rxudpsum");
	evcnt_attach_dynamic(&sc->sc_ev_txipsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txipsum");
	evcnt_attach_dynamic(&sc->sc_ev_txtcpsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txtcpsum");
	evcnt_attach_dynamic(&sc->sc_ev_txudpsum, EVCNT_TYPE_MISC,
	    NULL, sc->sc_dev.dv_xname, "txudpsum");
#endif /* DP83820 */
#endif /* SIP_EVENT_COUNTERS */

	/*
	 * Make sure the interface is shutdown during reboot.
	 */
	sc->sc_sdhook = shutdownhook_establish(SIP_DECL(shutdown), sc);
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
SIP_DECL(shutdown)(void *arg)
{
	struct sip_softc *sc = arg;

	SIP_DECL(stop)(&sc->sc_ethercom.ec_if, 1);
}

/*
 * sip_start:		[ifnet interface function]
 *
 *	Start packet transmission on the interface.
 */
void
SIP_DECL(start)(struct ifnet *ifp)
{
	struct sip_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct sip_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, firsttx, nexttx, lasttx, ofree, seg;
#ifdef DP83820
	u_int32_t extsts;
#endif

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
	for (;;) {
		/* Get a work queue entry. */
		if ((txs = SIMPLEQ_FIRST(&sc->sc_txfreeq)) == NULL) {
			SIP_EVCNT_INCR(&sc->sc_ev_txsstall);
			break;
		}

		/*
		 * Grab a packet off the queue.
		 */
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;
#ifndef DP83820
		m = NULL;
#endif

		dmamap = txs->txs_dmamap;

#ifdef DP83820
		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the allotted number of segments, or we
		 * were short on resources.  For the too-many-segments
		 * case, we simply report an error and drop the packet,
		 * since we can't sanely copy a jumbo packet to a single
		 * buffer.
		 */
		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT);
		if (error) {
			if (error == EFBIG) {
				printf("%s: Tx packet consumes too many "
				    "DMA segments, dropping...\n",
				    sc->sc_dev.dv_xname);
				IFQ_DEQUEUE(&ifp->if_snd, m0);
				m_freem(m0); 
				continue;
			}
			/*
			 * Short on resources, just stop for now.
			 */
			break;
		}
#else /* DP83820 */
		/*
		 * Load the DMA map.  If this fails, the packet either
		 * didn't fit in the alloted number of segments, or we
		 * were short on resources.  In this case, we'll copy
		 * and try again.
		 */
		if (bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE|BUS_DMA_NOWAIT) != 0) {
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
			    m, BUS_DMA_WRITE|BUS_DMA_NOWAIT);
			if (error) {
				printf("%s: unable to load Tx buffer, "
				    "error = %d\n", sc->sc_dev.dv_xname, error);
				break;
			}
		}
#endif /* DP83820 */

		/*
		 * Ensure we have enough descriptors free to describe
		 * the packet.  Note, we always reserve one descriptor
		 * at the end of the ring as a termination point, to
		 * prevent wrap-around.
		 */
		if (dmamap->dm_nsegs > (sc->sc_txfree - 1)) {
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
#ifndef DP83820
			if (m != NULL)
				m_freem(m);
#endif
			SIP_EVCNT_INCR(&sc->sc_ev_txdstall);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);
#ifndef DP83820
		if (m != NULL) {
			m_freem(m0);
			m0 = m;
		}
#endif

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
			    htole32(dmamap->dm_segs[seg].ds_addr);
			sc->sc_txdescs[nexttx].sipd_cmdsts =
			    htole32((nexttx == firsttx ? 0 : CMDSTS_OWN) |
			    CMDSTS_MORE | dmamap->dm_segs[seg].ds_len);
#ifdef DP83820
			sc->sc_txdescs[nexttx].sipd_extsts = 0;
#endif /* DP83820 */
			lasttx = nexttx;
		}

		/* Clear the MORE bit on the last segment. */
		sc->sc_txdescs[lasttx].sipd_cmdsts &= htole32(~CMDSTS_MORE);

#ifdef DP83820
		/*
		 * If VLANs are enabled and the packet has a VLAN tag, set
		 * up the descriptor to encapsulate the packet for us.
		 *
		 * This apparently has to be on the last descriptor of
		 * the packet.
		 */
		if (sc->sc_ethercom.ec_nvlans != 0 &&
		    (m = m_aux_find(m0, AF_LINK, ETHERTYPE_VLAN)) != NULL) {
			sc->sc_txdescs[lasttx].sipd_extsts |=
			    htole32(EXTSTS_VPKT |
				    htons(*mtod(m, int *) & EXTSTS_VTCI));
		}

		/*
		 * If the upper-layer has requested IPv4/TCPv4/UDPv4
		 * checksumming, set up the descriptor to do this work
		 * for us.
		 *
		 * This apparently has to be on the first descriptor of
		 * the packet.
		 *
		 * Byte-swap constants so the compiler can optimize.
		 */
		extsts = 0;
		if (m0->m_pkthdr.csum_flags & M_CSUM_IPv4) {
			KDASSERT(ifp->if_capenable & IFCAP_CSUM_IPv4);
			SIP_EVCNT_INCR(&sc->sc_ev_txipsum);
			extsts |= htole32(EXTSTS_IPPKT);
		}
		if (m0->m_pkthdr.csum_flags & M_CSUM_TCPv4) {
			KDASSERT(ifp->if_capenable & IFCAP_CSUM_TCPv4);
			SIP_EVCNT_INCR(&sc->sc_ev_txtcpsum);
			extsts |= htole32(EXTSTS_TCPPKT);
		} else if (m0->m_pkthdr.csum_flags & M_CSUM_UDPv4) {
			KDASSERT(ifp->if_capenable & IFCAP_CSUM_UDPv4);
			SIP_EVCNT_INCR(&sc->sc_ev_txudpsum);
			extsts |= htole32(EXTSTS_UDPPKT);
		}
		sc->sc_txdescs[sc->sc_txnext].sipd_extsts |= extsts;
#endif /* DP83820 */

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
		sc->sc_txdescs[lasttx].sipd_cmdsts |= htole32(CMDSTS_INTR);
		SIP_CDTXSYNC(sc, lasttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * The entire packet chain is set up.  Give the
		 * first descrptor to the chip now.
		 */
		sc->sc_txdescs[firsttx].sipd_cmdsts |= htole32(CMDSTS_OWN);
		SIP_CDTXSYNC(sc, firsttx, 1,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/*
		 * Start the transmit process.  Note, the manual says
		 * that if there are no pending transmissions in the
		 * chip's internal queue (indicated by TXE being clear),
		 * then the driver software must set the TXDP to the
		 * first descriptor to be transmitted.  However, if we
		 * do this, it causes serious performance degredation on
		 * the DP83820 under load, not setting TXDP doesn't seem
		 * to adversely affect the SiS 900 or DP83815.
		 *
		 * Well, I guess it wouldn't be the first time a manual
		 * has lied -- and they could be speaking of the NULL-
		 * terminated descriptor list case, rather than OWN-
		 * terminated rings.
		 */
#if 0
		if ((bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_CR) &
		     CR_TXE) == 0) {
			bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_TXDP,
			    SIP_CDTXADDR(sc, firsttx));
			bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_CR, CR_TXE);
		}
#else
		bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_CR, CR_TXE);
#endif

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
SIP_DECL(watchdog)(struct ifnet *ifp)
{
	struct sip_softc *sc = ifp->if_softc;

	/*
	 * The chip seems to ignore the CMDSTS_INTR bit sometimes!
	 * If we get a timeout, try and sweep up transmit descriptors.
	 * If we manage to sweep them all up, ignore the lack of
	 * interrupt.
	 */
	SIP_DECL(txintr)(sc);

	if (sc->sc_txfree != SIP_NTXDESC) {
		printf("%s: device timeout\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;

		/* Reset the interface. */
		(void) SIP_DECL(init)(ifp);
	} else if (ifp->if_flags & IFF_DEBUG)
		printf("%s: recovered from device timeout\n",
		    sc->sc_dev.dv_xname);

	/* Try to get more packets going. */
	SIP_DECL(start)(ifp);
}

/*
 * sip_ioctl:		[ifnet interface function]
 *
 *	Handle control requests from the operator.
 */
int
SIP_DECL(ioctl)(struct ifnet *ifp, u_long cmd, caddr_t data)
{
	struct sip_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

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
			(*sc->sc_model->sip_variant->sipv_set_filter)(sc);
			error = 0;
		}
		break;
	}

	/* Try to get more packets going. */
	SIP_DECL(start)(ifp);

	splx(s);
	return (error);
}

/*
 * sip_intr:
 *
 *	Interrupt service routine.
 */
int
SIP_DECL(intr)(void *arg)
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
			SIP_EVCNT_INCR(&sc->sc_ev_rxintr);

			/* Grab any new packets. */
			SIP_DECL(rxintr)(sc);

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
			SIP_EVCNT_INCR(&sc->sc_ev_txintr);

			/* Sweep up transmit descriptors. */
			SIP_DECL(txintr)(sc);

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
					(void) SIP_DECL(init)(ifp);
				} else {
					(void) SIP_DECL(init)(ifp);
					printf("\n");
				}
			}
		}

#if !defined(DP83820)
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
#endif /* ! DP83820 */

		if (isr & ISR_HIBERR) {
#define	PRINTERR(bit, str)						\
			if (isr & (bit))				\
				printf("%s: %s\n", sc->sc_dev.dv_xname, str)
			PRINTERR(ISR_DPERR, "parity error");
			PRINTERR(ISR_SSERR, "system error");
			PRINTERR(ISR_RMABT, "master abort");
			PRINTERR(ISR_RTABT, "target abort");
			PRINTERR(ISR_RXSOVR, "receive status FIFO overrun");
			(void) SIP_DECL(init)(ifp);
#undef PRINTERR
		}
	}

	/* Try to get more packets going. */
	SIP_DECL(start)(ifp);

	return (handled);
}

/*
 * sip_txintr:
 *
 *	Helper; handle transmit interrupts.
 */
void
SIP_DECL(txintr)(struct sip_softc *sc)
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

		cmdsts = le32toh(sc->sc_txdescs[txs->txs_lastdesc].sipd_cmdsts);
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
			ifp->if_oerrors++;
			if (cmdsts & CMDSTS_Tx_EC)
				ifp->if_collisions += 16;
			if (ifp->if_flags & IFF_DEBUG) {
				if (cmdsts & CMDSTS_Tx_ED)
					printf("%s: excessive deferral\n",
					    sc->sc_dev.dv_xname);
				if (cmdsts & CMDSTS_Tx_EC)
					printf("%s: excessive collisions\n",
					    sc->sc_dev.dv_xname);
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

#if defined(DP83820)
/*
 * sip_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
void
SIP_DECL(rxintr)(struct sip_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct sip_rxsoft *rxs;
	struct mbuf *m, *tailm;
	u_int32_t cmdsts, extsts;
	int i, len;

	for (i = sc->sc_rxptr;; i = SIP_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		SIP_CDRXSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		cmdsts = le32toh(sc->sc_rxdescs[i].sipd_cmdsts);
		extsts = le32toh(sc->sc_rxdescs[i].sipd_extsts);

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

		if (__predict_false(sc->sc_rxdiscard)) {
			SIP_INIT_RXDESC(sc, i);
			if ((cmdsts & CMDSTS_MORE) == 0) {
				/* Reset our state. */
				sc->sc_rxdiscard = 0;
			}
			continue;
		}

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		m = rxs->rxs_mbuf;

		/*
		 * Add a new receive buffer to the ring.
		 */
		if (SIP_DECL(add_rxbuf)(sc, i) != 0) {
			/*
			 * Failed, throw away what we've done so
			 * far, and discard the rest of the packet.
			 */
			ifp->if_ierrors++;
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
			SIP_INIT_RXDESC(sc, i);
			if (cmdsts & CMDSTS_MORE)
				sc->sc_rxdiscard = 1;
			if (sc->sc_rxhead != NULL)
				m_freem(sc->sc_rxhead);
			SIP_RXCHAIN_RESET(sc);
			continue;
		}

		SIP_RXCHAIN_LINK(sc, m);

		/*
		 * If this is not the end of the packet, keep
		 * looking.
		 */
		if (cmdsts & CMDSTS_MORE) {
			sc->sc_rxlen += m->m_len;
			continue;
		}

		/*
		 * Okay, we have the entire packet now...
		 */
		*sc->sc_rxtailp = NULL;
		m = sc->sc_rxhead;
		tailm = sc->sc_rxtail;

		SIP_RXCHAIN_RESET(sc);

		/*
		 * If an error occurred, update stats and drop the packet.
		 */
		if (cmdsts & (CMDSTS_Rx_RXA|CMDSTS_Rx_RUNT|
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
			PRINTERR(CMDSTS_Rx_RUNT, "runt packet");
			PRINTERR(CMDSTS_Rx_ISE, "invalid symbol error");
			PRINTERR(CMDSTS_Rx_CRCE, "CRC error");
			PRINTERR(CMDSTS_Rx_FAE, "frame alignment error");
#undef PRINTERR
			m_freem(m);
			continue;
		}

		/*
		 * No errors.
		 *
		 * Note, the DP83820 includes the CRC with
		 * every packet.
		 */
		len = CMDSTS_SIZE(cmdsts);
		tailm->m_len = len - sc->sc_rxlen;

		/*
		 * If the packet is small enough to fit in a
		 * single header mbuf, allocate one and copy
		 * the data into it.  This greatly reduces
		 * memory consumption when we receive lots
		 * of small packets.
		 */
		if (SIP_DECL(copy_small) != 0 && len <= (MHLEN - 2)) {
			struct mbuf *nm;
			MGETHDR(nm, M_DONTWAIT, MT_DATA);
			if (nm == NULL) {
				ifp->if_ierrors++;
				m_freem(m);
				continue;
			}
			nm->m_data += 2;
			nm->m_pkthdr.len = nm->m_len = len;
			m_copydata(m, 0, len, mtod(nm, caddr_t));
			m_freem(m);
			m = nm;
		}
#ifndef __NO_STRICT_ALIGNMENT
		else {
			/*
			 * The DP83820's receive buffers must be 4-byte
			 * aligned.  But this means that the data after
			 * the Ethernet header is misaligned.  To compensate,
			 * we have artificially shortened the buffer size
			 * in the descriptor, and we do an overlapping copy
			 * of the data two bytes further in (in the first
			 * buffer of the chain only).
			 */
			memmove(mtod(m, caddr_t) + 2, mtod(m, caddr_t),
			    m->m_len);
			m->m_data += 2;
		}
#endif /* ! __NO_STRICT_ALIGNMENT */

		/*
		 * If VLANs are enabled, VLAN packets have been unwrapped
		 * for us.  Associate the tag with the packet.
		 */
		if (sc->sc_ethercom.ec_nvlans != 0 &&
		    (extsts & EXTSTS_VPKT) != 0) {
			struct mbuf *vtag;

			vtag = m_aux_add(m, AF_LINK, ETHERTYPE_VLAN);
			if (vtag == NULL) {
				ifp->if_ierrors++;
				printf("%s: unable to allocate VLAN tag\n",
				    sc->sc_dev.dv_xname);
				m_freem(m);
				continue;
			}

			*mtod(vtag, int *) = ntohs(extsts & EXTSTS_VTCI);
			vtag->m_len = sizeof(int);
		}

		/*
		 * Set the incoming checksum information for the
		 * packet.
		 */
		if ((extsts & EXTSTS_IPPKT) != 0) {
			SIP_EVCNT_INCR(&sc->sc_ev_rxipsum);
			m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			if (extsts & EXTSTS_Rx_IPERR)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
			if (extsts & EXTSTS_TCPPKT) {
				SIP_EVCNT_INCR(&sc->sc_ev_rxtcpsum);
				m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;
				if (extsts & EXTSTS_Rx_TCPERR)
					m->m_pkthdr.csum_flags |=
					    M_CSUM_TCP_UDP_BAD;
			} else if (extsts & EXTSTS_UDPPKT) {
				SIP_EVCNT_INCR(&sc->sc_ev_rxudpsum);
				m->m_pkthdr.csum_flags |= M_CSUM_UDPv4;
				if (extsts & EXTSTS_Rx_UDPERR)
					m->m_pkthdr.csum_flags |=
					    M_CSUM_TCP_UDP_BAD;
			}
		}

		ifp->if_ipackets++;
		m->m_flags |= M_HASFCS;
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = len;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass if up the stack if it's for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NBPFILTER > 0 */

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;
}
#else /* ! DP83820 */
/*
 * sip_rxintr:
 *
 *	Helper; handle receive interrupts.
 */
void
SIP_DECL(rxintr)(struct sip_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct sip_rxsoft *rxs;
	struct mbuf *m;
	u_int32_t cmdsts;
	int i, len;

	for (i = sc->sc_rxptr;; i = SIP_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		SIP_CDRXSYNC(sc, i, BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		cmdsts = le32toh(sc->sc_rxdescs[i].sipd_cmdsts);

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
		if (cmdsts & (CMDSTS_Rx_RXA|CMDSTS_Rx_RUNT|
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
		 * includes the CRC with every packet.
		 */
		len = CMDSTS_SIZE(cmdsts);

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
		if (SIP_DECL(copy_small) != 0 && len <= MHLEN) {
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
			if (SIP_DECL(add_rxbuf)(sc, i) != 0) {
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
		m->m_flags |= M_HASFCS;
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

#if NBPFILTER > 0
		/*
		 * Pass this up to any BPF listeners, but only
		 * pass if up the stack if it's for us.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NBPFILTER > 0 */

		/* Pass it on. */
		(*ifp->if_input)(ifp, m);
	}

	/* Update the receive pointer. */
	sc->sc_rxptr = i;
}
#endif /* DP83820 */

/*
 * sip_tick:
 *
 *	One second timer, used to tick the MII.
 */
void
SIP_DECL(tick)(void *arg)
{
	struct sip_softc *sc = arg;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, SIP_DECL(tick), sc);
}

/*
 * sip_reset:
 *
 *	Perform a soft reset on the SiS 900.
 */
void
SIP_DECL(reset)(struct sip_softc *sc)
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	int i;

	bus_space_write_4(st, sh, SIP_IER, 0);
	bus_space_write_4(st, sh, SIP_IMR, 0);
	bus_space_write_4(st, sh, SIP_RFCR, 0);
	bus_space_write_4(st, sh, SIP_CR, CR_RST);

	for (i = 0; i < SIP_TIMEOUT; i++) {
		if ((bus_space_read_4(st, sh, SIP_CR) & CR_RST) == 0)
			break;
		delay(2);
	}

	if (i == SIP_TIMEOUT)
		printf("%s: reset failed to complete\n", sc->sc_dev.dv_xname);

	delay(1000);

#ifdef DP83820
	/*
	 * Set the general purpose I/O bits.  Do it here in case we
	 * need to have GPIO set up to talk to the media interface.
	 */
	bus_space_write_4(st, sh, SIP_GPIOR, sc->sc_gpior);
	delay(1000);
#endif /* DP83820 */
}

/*
 * sip_init:		[ ifnet interface function ]
 *
 *	Initialize the interface.  Must be called at splnet().
 */
int
SIP_DECL(init)(struct ifnet *ifp)
{
	struct sip_softc *sc = ifp->if_softc;
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct sip_txsoft *txs;
	struct sip_rxsoft *rxs;
	struct sip_desc *sipd;
	u_int32_t reg;
	int i, error = 0;

	/*
	 * Cancel any pending I/O.
	 */
	SIP_DECL(stop)(ifp, 0);

	/*
	 * Reset the chip to a known state.
	 */
	SIP_DECL(reset)(sc);

#if !defined(DP83820)
	if (SIP_CHIP_MODEL(sc, PCI_VENDOR_NS, PCI_PRODUCT_NS_DP83815)) {
		/*
		 * DP83815 manual, page 78:
		 *    4.4 Recommended Registers Configuration
		 *    For optimum performance of the DP83815, version noted
		 *    as DP83815CVNG (SRR = 203h), the listed register
		 *    modifications must be followed in sequence...
		 *
		 * It's not clear if this should be 302h or 203h because that
		 * chip name is listed as SRR 302h in the description of the
		 * SRR register.  However, my revision 302h DP83815 on the
		 * Netgear FA311 purchased in 02/2001 needs these settings
		 * to avoid tons of errors in AcceptPerfectMatch (non-
		 * IFF_PROMISC) mode.  I do not know if other revisions need
		 * this set or not.  [briggs -- 09 March 2001]
		 *
		 * Note that only the low-order 12 bits of 0xe4 are documented
		 * and that this sets reserved bits in that register.
		 */
		reg = bus_space_read_4(st, sh, SIP_NS_SRR);
		if (reg == 0x302) {
			bus_space_write_4(st, sh, 0x00cc, 0x0001);
			bus_space_write_4(st, sh, 0x00e4, 0x189C);
			bus_space_write_4(st, sh, 0x00fc, 0x0000);
			bus_space_write_4(st, sh, 0x00f4, 0x5040);
			bus_space_write_4(st, sh, 0x00f8, 0x008c);
		}
	}
#endif /* ! DP83820 */

	/*
	 * Initialize the transmit descriptor ring.
	 */
	for (i = 0; i < SIP_NTXDESC; i++) {
		sipd = &sc->sc_txdescs[i];
		memset(sipd, 0, sizeof(struct sip_desc));
		sipd->sipd_link = htole32(SIP_CDTXADDR(sc, SIP_NEXTTX(i)));
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
			if ((error = SIP_DECL(add_rxbuf)(sc, i)) != 0) {
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    sc->sc_dev.dv_xname, i, error);
				/*
				 * XXX Should attempt to run with fewer receive
				 * XXX buffers instead of just failing.
				 */
				SIP_DECL(rxdrain)(sc);
				goto out;
			}
		} else
			SIP_INIT_RXDESC(sc, i);
	}
	sc->sc_rxptr = 0;
#ifdef DP83820
	sc->sc_rxdiscard = 0;
	SIP_RXCHAIN_RESET(sc);
#endif /* DP83820 */

	/*
	 * Set the configuration register; it's already initialized
	 * in sip_attach().
	 */
	bus_space_write_4(st, sh, SIP_CFG, sc->sc_cfg);

	/*
	 * Initialize the prototype TXCFG register.
	 */
#if defined(DP83820)
	sc->sc_txcfg = TXCFG_MXDMA_512;
	sc->sc_rxcfg = RXCFG_MXDMA_512;
#else
	if ((SIP_SIS900_REV(sc, SIS_REV_635) ||
	     SIP_SIS900_REV(sc, SIS_REV_900B)) &&
	    (bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_CFG) & CFG_EDBMASTEN)) {
		sc->sc_txcfg = TXCFG_MXDMA_64;
		sc->sc_rxcfg = RXCFG_MXDMA_64;
	} else {
		sc->sc_txcfg = TXCFG_MXDMA_512;
		sc->sc_rxcfg = RXCFG_MXDMA_512;
	}
#endif /* DP83820 */

	sc->sc_txcfg |= TXCFG_ATP |
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
	sc->sc_rxcfg |= (sc->sc_rx_drain_thresh << RXCFG_DRTH_SHIFT);
	bus_space_write_4(st, sh, SIP_RXCFG, sc->sc_rxcfg);

#ifdef DP83820
	/*
	 * Initialize the VLAN/IP receive control register.
	 * We enable checksum computation on all incoming
	 * packets, and do not reject packets w/ bad checksums.
	 */
	reg = 0;
	if (ifp->if_capenable &
	    (IFCAP_CSUM_IPv4|IFCAP_CSUM_TCPv4|IFCAP_CSUM_UDPv4))
		reg |= VRCR_IPEN;
	if (sc->sc_ethercom.ec_nvlans != 0)
		reg |= VRCR_VTDEN|VRCR_VTREN;
	bus_space_write_4(st, sh, SIP_VRCR, reg);

	/*
	 * Initialize the VLAN/IP transmit control register.
	 * We enable outgoing checksum computation on a
	 * per-packet basis.
	 */
	reg = 0;
	if (ifp->if_capenable &
	    (IFCAP_CSUM_IPv4|IFCAP_CSUM_TCPv4|IFCAP_CSUM_UDPv4))
		reg |= VTCR_PPCHK;
	if (sc->sc_ethercom.ec_nvlans != 0)
		reg |= VTCR_VPPTI;
	bus_space_write_4(st, sh, SIP_VTCR, reg);

	/*
	 * If we're using VLANs, initialize the VLAN data register.
	 * To understand why we bswap the VLAN Ethertype, see section
	 * 4.2.36 of the DP83820 manual.
	 */
	if (sc->sc_ethercom.ec_nvlans != 0)
		bus_space_write_4(st, sh, SIP_VDR, bswap16(ETHERTYPE_VLAN));
#endif /* DP83820 */

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

	/* Set up the receive filter. */
	(*sc->sc_model->sip_variant->sipv_set_filter)(sc);

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
	callout_reset(&sc->sc_tick_ch, hz, SIP_DECL(tick), sc);

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
SIP_DECL(rxdrain)(struct sip_softc *sc)
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
 * sip_stop:		[ ifnet interface function ]
 *
 *	Stop transmission on the interface.
 */
void
SIP_DECL(stop)(struct ifnet *ifp, int disable)
{
	struct sip_softc *sc = ifp->if_softc;
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct sip_txsoft *txs;
	u_int32_t cmdsts = 0;		/* DEBUG */

	/*
	 * Stop the one second clock.
	 */
	callout_stop(&sc->sc_tick_ch);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

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
		    (le32toh(sc->sc_txdescs[txs->txs_lastdesc].sipd_cmdsts) &
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
		    le32toh(sc->sc_txdescs[txs->txs_lastdesc].sipd_cmdsts);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
		SIMPLEQ_INSERT_TAIL(&sc->sc_txfreeq, txs, txs_q);
	}

	if (disable)
		SIP_DECL(rxdrain)(sc);

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
SIP_DECL(read_eeprom)(struct sip_softc *sc, int word, int wordcnt,
    u_int16_t *data)
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
			delay(4);
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
SIP_DECL(add_rxbuf)(struct sip_softc *sc, int idx)
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

#if defined(DP83820)
	m->m_len = SIP_RXBUF_LEN;
#endif /* DP83820 */

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL,
	    BUS_DMA_READ|BUS_DMA_NOWAIT);
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

#if !defined(DP83820)
/*
 * sip_sis900_set_filter:
 *
 *	Set up the receive filter.
 */
void
SIP_DECL(sis900_set_filter)(struct sip_softc *sc)
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ether_multi *enm;
	u_int8_t *cp;
	struct ether_multistep step;
	u_int32_t crc, mchash[16];

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
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
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

		crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);

		if (SIP_SIS900_REV(sc, SIS_REV_635) ||
		    SIP_SIS900_REV(sc, SIS_REV_900B)) {
			/* Just want the 8 most significant bits. */
			crc >>= 24;
		} else {
			/* Just want the 7 most significant bits. */
			crc >>= 25;
		}

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
	delay(1);							\
	bus_space_write_4(st, sh, SIP_RFDR, (data));			\
	delay(1)

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
		if (SIP_SIS900_REV(sc, SIS_REV_635) ||
		    SIP_SIS900_REV(sc, SIS_REV_900B)) {
			FILTER_EMIT(RFCR_RFADDR_MC8, mchash[8]);
			FILTER_EMIT(RFCR_RFADDR_MC9, mchash[9]);
			FILTER_EMIT(RFCR_RFADDR_MC10, mchash[10]);
			FILTER_EMIT(RFCR_RFADDR_MC11, mchash[11]);
			FILTER_EMIT(RFCR_RFADDR_MC12, mchash[12]);
			FILTER_EMIT(RFCR_RFADDR_MC13, mchash[13]);
			FILTER_EMIT(RFCR_RFADDR_MC14, mchash[14]);
			FILTER_EMIT(RFCR_RFADDR_MC15, mchash[15]);
		}
	}
#undef FILTER_EMIT

	/*
	 * Re-enable the receiver filter.
	 */
	bus_space_write_4(st, sh, SIP_RFCR, sc->sc_rfcr);
}
#endif /* ! DP83820 */

/*
 * sip_dp83815_set_filter:
 *
 *	Set up the receive filter.
 */
void
SIP_DECL(dp83815_set_filter)(struct sip_softc *sc)
{
	bus_space_tag_t st = sc->sc_st;
	bus_space_handle_t sh = sc->sc_sh;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if; 
	struct ether_multi *enm;
	u_int8_t *cp;    
	struct ether_multistep step; 
	u_int32_t crc, hash, slot, bit;
#ifdef DP83820
#define	MCHASH_NWORDS	128
#else
#define	MCHASH_NWORDS	32
#endif /* DP83820 */
	u_int16_t mchash[MCHASH_NWORDS];
	int i;

	/*
	 * Initialize the prototype RFCR.
	 * Enable the receive filter, and accept on
	 *    Perfect (destination address) Match
	 * If IFF_BROADCAST, also accept all broadcast packets.
	 * If IFF_PROMISC, accept all unicast packets (and later, set
	 *    IFF_ALLMULTI and accept all multicast, too).
	 */
	sc->sc_rfcr = RFCR_RFEN | RFCR_APM;
	if (ifp->if_flags & IFF_BROADCAST)
		sc->sc_rfcr |= RFCR_AAB;
	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_rfcr |= RFCR_AAP;
		goto allmulti;
	}

#ifdef DP83820
	/*
	 * Set up the DP83820 multicast address filter by passing all multicast
	 * addresses through a CRC generator, and then using the high-order
	 * 11 bits as an index into the 2048 bit multicast hash table.  The
	 * high-order 7 bits select the slot, while the low-order 4 bits
	 * select the bit within the slot.  Note that only the low 16-bits
	 * of each filter word are used, and there are 128 filter words.
	 */
#else
	/*
	 * Set up the DP83815 multicast address filter by passing all multicast
	 * addresses through a CRC generator, and then using the high-order
	 * 9 bits as an index into the 512 bit multicast hash table.  The
	 * high-order 5 bits select the slot, while the low-order 4 bits
	 * select the bit within the slot.  Note that only the low 16-bits
	 * of each filter word are used, and there are 32 filter words.
	 */
#endif /* DP83820 */

	memset(mchash, 0, sizeof(mchash));

	ifp->if_flags &= ~IFF_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	if (enm == NULL)
		goto setit;
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, ETHER_ADDR_LEN)) {
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

		crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);

#ifdef DP83820
		/* Just want the 11 most significant bits. */
		hash = crc >> 21;
#else
		/* Just want the 9 most significant bits. */
		hash = crc >> 23;
#endif /* DP83820 */

		slot = hash >> 4;
		bit = hash & 0xf;

		/* Set the corresponding bit in the hash table. */
		mchash[slot] |= 1 << bit;

		ETHER_NEXT_MULTI(step, enm);
	}
	sc->sc_rfcr |= RFCR_MHEN;
	goto setit;

 allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	sc->sc_rfcr |= RFCR_AAM;

 setit:
#define	FILTER_EMIT(addr, data)						\
	bus_space_write_4(st, sh, SIP_RFCR, (addr));			\
	delay(1);							\
	bus_space_write_4(st, sh, SIP_RFDR, (data));			\
	delay(1)

	/*
	 * Disable receive filter, and program the node address.
	 */
	cp = LLADDR(ifp->if_sadl);
	FILTER_EMIT(RFCR_NS_RFADDR_PMATCH0, (cp[1] << 8) | cp[0]);
	FILTER_EMIT(RFCR_NS_RFADDR_PMATCH2, (cp[3] << 8) | cp[2]);
	FILTER_EMIT(RFCR_NS_RFADDR_PMATCH4, (cp[5] << 8) | cp[4]);

	if ((ifp->if_flags & IFF_ALLMULTI) == 0) {
		/*
		 * Program the multicast hash table.
		 */
		for (i = 0; i < MCHASH_NWORDS; i++) {
			FILTER_EMIT(RFCR_NS_RFADDR_FILTMEM + (i * 2),
			    mchash[i]);
		}
	}
#undef FILTER_EMIT
#undef MCHASH_NWORDS

	/*
	 * Re-enable the receiver filter.
	 */
	bus_space_write_4(st, sh, SIP_RFCR, sc->sc_rfcr);
} 

#if defined(DP83820)
/*
 * sip_dp83820_mii_readreg:	[mii interface function]
 *
 *	Read a PHY register on the MII of the DP83820.
 */
int
SIP_DECL(dp83820_mii_readreg)(struct device *self, int phy, int reg)
{

	return (mii_bitbang_readreg(self, &SIP_DECL(dp83820_mii_bitbang_ops),
	    phy, reg));
}

/*
 * sip_dp83820_mii_writereg:	[mii interface function]
 *
 *	Write a PHY register on the MII of the DP83820.
 */
void
SIP_DECL(dp83820_mii_writereg)(struct device *self, int phy, int reg, int val)
{

	mii_bitbang_writereg(self, &SIP_DECL(dp83820_mii_bitbang_ops),
	    phy, reg, val);
}

/*
 * sip_dp83815_mii_statchg:	[mii interface function]
 *
 *	Callback from MII layer when media changes.
 */
void
SIP_DECL(dp83820_mii_statchg)(struct device *self)
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t cfg;

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
	 * Update CFG for MII/GMII.
	 */
	if (sc->sc_ethercom.ec_if.if_baudrate == IF_Mbps(1000))
		cfg = sc->sc_cfg | CFG_MODE_1000;
	else
		cfg = sc->sc_cfg;

	/*
	 * XXX 802.3x flow control.
	 */

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_CFG, cfg);
	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_TXCFG, sc->sc_txcfg);
	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_RXCFG, sc->sc_rxcfg);
}

/*
 * sip_dp83820_mii_bitbang_read: [mii bit-bang interface function]
 *
 *	Read the MII serial port for the MII bit-bang module.
 */
u_int32_t
SIP_DECL(dp83820_mii_bitbang_read)(struct device *self)
{
	struct sip_softc *sc = (void *) self;

	return (bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_EROMAR));
}

/*
 * sip_dp83820_mii_bitbang_write: [mii big-bang interface function]
 *
 *	Write the MII serial port for the MII bit-bang module.
 */
void
SIP_DECL(dp83820_mii_bitbang_write)(struct device *self, u_int32_t val)
{
	struct sip_softc *sc = (void *) self;

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_EROMAR, val);
}
#else /* ! DP83820 */
/*
 * sip_sis900_mii_readreg:	[mii interface function]
 *
 *	Read a PHY register on the MII.
 */
int
SIP_DECL(sis900_mii_readreg)(struct device *self, int phy, int reg)
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t enphy;

	/*
	 * The SiS 900 has only an internal PHY on the MII.  Only allow
	 * MII address 0.
	 */
	if (sc->sc_model->sip_product == PCI_PRODUCT_SIS_900 &&
	    sc->sc_rev < SIS_REV_635 && phy != 0)
		return (0);

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_ENPHY,
	    (phy << ENPHY_PHYADDR_SHIFT) | (reg << ENPHY_REGADDR_SHIFT) |
	    ENPHY_RWCMD | ENPHY_ACCESS);
	do {
		enphy = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_ENPHY);
	} while (enphy & ENPHY_ACCESS);
	return ((enphy & ENPHY_PHYDATA) >> ENPHY_DATA_SHIFT);
}

/*
 * sip_sis900_mii_writereg:	[mii interface function]
 *
 *	Write a PHY register on the MII.
 */
void
SIP_DECL(sis900_mii_writereg)(struct device *self, int phy, int reg, int val)
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t enphy;

	/*
	 * The SiS 900 has only an internal PHY on the MII.  Only allow
	 * MII address 0.
	 */
	if (sc->sc_model->sip_product == PCI_PRODUCT_SIS_900 &&
	    sc->sc_rev < SIS_REV_635 && phy != 0)
		return;

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_ENPHY,
	    (val << ENPHY_DATA_SHIFT) | (phy << ENPHY_PHYADDR_SHIFT) |
	    (reg << ENPHY_REGADDR_SHIFT) | ENPHY_ACCESS);
	do {
		enphy = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_ENPHY);
	} while (enphy & ENPHY_ACCESS);
}

/*
 * sip_sis900_mii_statchg:	[mii interface function]
 *
 *	Callback from MII layer when media changes.
 */
void
SIP_DECL(sis900_mii_statchg)(struct device *self)
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
}

/*
 * sip_dp83815_mii_readreg:	[mii interface function]
 *
 *	Read a PHY register on the MII.
 */
int
SIP_DECL(dp83815_mii_readreg)(struct device *self, int phy, int reg)
{
	struct sip_softc *sc = (struct sip_softc *) self;
	u_int32_t val;

	/*
	 * The DP83815 only has an internal PHY.  Only allow
	 * MII address 0.
	 */
	if (phy != 0)
		return (0);

	/*
	 * Apparently, after a reset, the DP83815 can take a while
	 * to respond.  During this recovery period, the BMSR returns
	 * a value of 0.  Catch this -- it's not supposed to happen
	 * (the BMSR has some hardcoded-to-1 bits), and wait for the
	 * PHY to come back to life.
	 *
	 * This works out because the BMSR is the first register
	 * read during the PHY probe process.
	 */
	do {
		val = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_NS_PHY(reg));
	} while (reg == MII_BMSR && val == 0);

	return (val & 0xffff);
}

/*
 * sip_dp83815_mii_writereg:	[mii interface function]
 *
 *	Write a PHY register to the MII.
 */
void
SIP_DECL(dp83815_mii_writereg)(struct device *self, int phy, int reg, int val)
{
	struct sip_softc *sc = (struct sip_softc *) self;

	/*
	 * The DP83815 only has an internal PHY.  Only allow
	 * MII address 0.
	 */
	if (phy != 0)
		return;

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_NS_PHY(reg), val);
}

/*
 * sip_dp83815_mii_statchg:	[mii interface function]
 *
 *	Callback from MII layer when media changes.
 */
void
SIP_DECL(dp83815_mii_statchg)(struct device *self)
{
	struct sip_softc *sc = (struct sip_softc *) self;

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
	 * XXX 802.3x flow control.
	 */

	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_TXCFG, sc->sc_txcfg);
	bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_RXCFG, sc->sc_rxcfg);
}
#endif /* DP83820 */

#if defined(DP83820)
void
SIP_DECL(dp83820_read_macaddr)(struct sip_softc *sc,
    const struct pci_attach_args *pa, u_int8_t *enaddr)
{
	u_int16_t eeprom_data[SIP_DP83820_EEPROM_LENGTH / 2];
	u_int8_t cksum, *e, match;
	int i;

	/*
	 * EEPROM data format for the DP83820 can be found in
	 * the DP83820 manual, section 4.2.4.
	 */

	SIP_DECL(read_eeprom)(sc, 0,
	    sizeof(eeprom_data) / sizeof(eeprom_data[0]), eeprom_data);

	match = eeprom_data[SIP_DP83820_EEPROM_CHECKSUM / 2] >> 8;
	match = ~(match - 1);

	cksum = 0x55;
	e = (u_int8_t *) eeprom_data;
	for (i = 0; i < SIP_DP83820_EEPROM_CHECKSUM; i++)
		cksum += *e++;

	if (cksum != match)
		printf("%s: Checksum (%x) mismatch (%x)",
		    sc->sc_dev.dv_xname, cksum, match);

	enaddr[0] = eeprom_data[SIP_DP83820_EEPROM_PMATCH2 / 2] & 0xff;
	enaddr[1] = eeprom_data[SIP_DP83820_EEPROM_PMATCH2 / 2] >> 8;
	enaddr[2] = eeprom_data[SIP_DP83820_EEPROM_PMATCH1 / 2] & 0xff;
	enaddr[3] = eeprom_data[SIP_DP83820_EEPROM_PMATCH1 / 2] >> 8;
	enaddr[4] = eeprom_data[SIP_DP83820_EEPROM_PMATCH0 / 2] & 0xff;
	enaddr[5] = eeprom_data[SIP_DP83820_EEPROM_PMATCH0 / 2] >> 8;

	/* Get the GPIOR bits. */
	sc->sc_gpior = eeprom_data[0x04];

	/* Get various CFG related bits. */
	if ((eeprom_data[0x05] >> 0) & 1)
		sc->sc_cfg |= CFG_EXT_125; 
	if ((eeprom_data[0x05] >> 9) & 1)
		sc->sc_cfg |= CFG_TBI_EN;
}
#else /* ! DP83820 */
void
SIP_DECL(sis900_read_macaddr)(struct sip_softc *sc,
    const struct pci_attach_args *pa, u_int8_t *enaddr)
{
	u_int16_t myea[ETHER_ADDR_LEN / 2];

	switch (sc->sc_rev) {
	case SIS_REV_630S:
	case SIS_REV_630E:
	case SIS_REV_630EA1:
	case SIS_REV_630ET:
	case SIS_REV_635:
		/*
		 * The MAC address for the on-board Ethernet of
		 * the SiS 630 chipset is in the NVRAM.  Kick
		 * the chip into re-loading it from NVRAM, and
		 * read the MAC address out of the filter registers.
		 */
		bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_CR, CR_RLD);

		bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_RFCR,
		    RFCR_RFADDR_NODE0);
		myea[0] = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_RFDR) &
		    0xffff;

		bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_RFCR,
		    RFCR_RFADDR_NODE2);
		myea[1] = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_RFDR) &
		    0xffff;

		bus_space_write_4(sc->sc_st, sc->sc_sh, SIP_RFCR,
		    RFCR_RFADDR_NODE4);
		myea[2] = bus_space_read_4(sc->sc_st, sc->sc_sh, SIP_RFDR) &
		    0xffff;
		break;

	default:
		SIP_DECL(read_eeprom)(sc, SIP_EEPROM_ETHERNET_ID0 >> 1,
		    sizeof(myea) / sizeof(myea[0]), myea);
	}

	enaddr[0] = myea[0] & 0xff;
	enaddr[1] = myea[0] >> 8;
	enaddr[2] = myea[1] & 0xff;
	enaddr[3] = myea[1] >> 8;
	enaddr[4] = myea[2] & 0xff;
	enaddr[5] = myea[2] >> 8;
}

/* Table and macro to bit-reverse an octet. */
static const u_int8_t bbr4[] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
#define bbr(v)	((bbr4[(v)&0xf] << 4) | bbr4[((v)>>4) & 0xf])

void
SIP_DECL(dp83815_read_macaddr)(struct sip_softc *sc,
    const struct pci_attach_args *pa, u_int8_t *enaddr)
{
	u_int16_t eeprom_data[SIP_DP83815_EEPROM_LENGTH / 2], *ea;
	u_int8_t cksum, *e, match;
	int i;

	SIP_DECL(read_eeprom)(sc, 0, sizeof(eeprom_data) /
	    sizeof(eeprom_data[0]), eeprom_data);

	match = eeprom_data[SIP_DP83815_EEPROM_CHECKSUM/2] >> 8;
	match = ~(match - 1);

	cksum = 0x55;
	e = (u_int8_t *) eeprom_data;
	for (i=0 ; i<SIP_DP83815_EEPROM_CHECKSUM ; i++) {
		cksum += *e++;
	}
	if (cksum != match) {
		printf("%s: Checksum (%x) mismatch (%x)",
		    sc->sc_dev.dv_xname, cksum, match);
	}

	/*
	 * Unrolled because it makes slightly more sense this way.
	 * The DP83815 stores the MAC address in bit 0 of word 6
	 * through bit 15 of word 8.
	 */
	ea = &eeprom_data[6];
	enaddr[0] = ((*ea & 0x1) << 7);
	ea++;
	enaddr[0] |= ((*ea & 0xFE00) >> 9);
	enaddr[1] = ((*ea & 0x1FE) >> 1);
	enaddr[2] = ((*ea & 0x1) << 7);
	ea++;
	enaddr[2] |= ((*ea & 0xFE00) >> 9);
	enaddr[3] = ((*ea & 0x1FE) >> 1);
	enaddr[4] = ((*ea & 0x1) << 7);
	ea++;
	enaddr[4] |= ((*ea & 0xFE00) >> 9);
	enaddr[5] = ((*ea & 0x1FE) >> 1);

	/*
	 * In case that's not weird enough, we also need to reverse
	 * the bits in each byte.  This all actually makes more sense
	 * if you think about the EEPROM storage as an array of bits
	 * being shifted into bytes, but that's not how we're looking
	 * at it here...
	 */
	for (i = 0; i < 6 ;i++)
		enaddr[i] = bbr(enaddr[i]);
}
#endif /* DP83820 */

/*
 * sip_mediastatus:	[ifmedia interface function]
 *
 *	Get the current interface media status.
 */
void
SIP_DECL(mediastatus)(struct ifnet *ifp, struct ifmediareq *ifmr)
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
SIP_DECL(mediachange)(struct ifnet *ifp)
{
	struct sip_softc *sc = ifp->if_softc;

	if (ifp->if_flags & IFF_UP)
		mii_mediachg(&sc->sc_mii);
	return (0);
}
