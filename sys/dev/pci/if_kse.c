/*	$NetBSD: if_kse.c,v 1.58 2021/12/31 14:25:23 riastradh Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * Micrel 8841/8842 10/100 PCI ethernet driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_kse.c,v 1.58 2021/12/31 14:25:23 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/rndsource.h>
#include <sys/errno.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <net/bpf.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#define KSE_LINKDEBUG 0

#define CSR_READ_4(sc, off) \
	    bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (off))
#define CSR_WRITE_4(sc, off, val) \
	    bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (off), (val))
#define CSR_READ_2(sc, off) \
	    bus_space_read_2((sc)->sc_st, (sc)->sc_sh, (off))
#define CSR_WRITE_2(sc, off, val) \
	    bus_space_write_2((sc)->sc_st, (sc)->sc_sh, (off), (val))

#define MDTXC		0x000		/* DMA transmit control */
#define MDRXC		0x004		/* DMA receive control */
#define MDTSC		0x008		/* trigger DMA transmit (SC) */
#define MDRSC		0x00c		/* trigger DMA receive (SC) */
#define TDLB		0x010		/* transmit descriptor list base */
#define RDLB		0x014		/* receive descriptor list base */
#define MTR0		0x020		/* multicast table 31:0 */
#define MTR1		0x024		/* multicast table 63:32 */
#define INTEN		0x028		/* interrupt enable */
#define INTST		0x02c		/* interrupt status */
#define MAAL0		0x080		/* additional MAC address 0 low */
#define MAAH0		0x084		/* additional MAC address 0 high */
#define MARL		0x200		/* MAC address low */
#define MARM		0x202		/* MAC address middle */
#define MARH		0x204		/* MAC address high */
#define GRR		0x216		/* global reset */
#define SIDER		0x400		/* switch ID and function enable */
#define SGCR3		0x406		/* switch function control 3 */
#define  CR3_USEHDX	(1U<<6)		/* use half-duplex 8842 host port */
#define  CR3_USEFC	(1U<<5) 	/* use flowcontrol 8842 host port */
#define IACR		0x4a0		/* indirect access control */
#define IADR1		0x4a2		/* indirect access data 66:63 */
#define IADR2		0x4a4		/* indirect access data 47:32 */
#define IADR3		0x4a6		/* indirect access data 63:48 */
#define IADR4		0x4a8		/* indirect access data 15:0 */
#define IADR5		0x4aa		/* indirect access data 31:16 */
#define  IADR_LATCH	(1U<<30)	/* latch completed indication */
#define  IADR_OVF	(1U<<31)	/* overflow detected */
#define P1CR4		0x512		/* port 1 control 4 */
#define P1SR		0x514		/* port 1 status */
#define P2CR4		0x532		/* port 2 control 4 */
#define P2SR		0x534		/* port 2 status */
#define  PxCR_STARTNEG	(1U<<9)		/* restart auto negotiation */
#define  PxCR_AUTOEN	(1U<<7)		/* auto negotiation enable */
#define  PxCR_SPD100	(1U<<6)		/* force speed 100 */
#define  PxCR_USEFDX	(1U<<5)		/* force full duplex */
#define  PxCR_USEFC	(1U<<4)		/* advertise pause flow control */
#define  PxSR_ACOMP	(1U<<6)		/* auto negotiation completed */
#define  PxSR_SPD100	(1U<<10)	/* speed is 100Mbps */
#define  PxSR_FDX	(1U<<9)		/* full duplex */
#define  PxSR_LINKUP	(1U<<5)		/* link is good */
#define  PxSR_RXFLOW	(1U<<12)	/* receive flow control active */
#define  PxSR_TXFLOW	(1U<<11)	/* transmit flow control active */
#define P1VIDCR		0x504		/* port 1 vtag */
#define P2VIDCR		0x524		/* port 2 vtag */
#define P3VIDCR		0x544		/* 8842 host vtag */
#define EVCNTBR		0x1c00		/* 3 sets of 34 event counters */

#define TXC_BS_MSK	0x3f000000	/* burst size */
#define TXC_BS_SFT	(24)		/* 1,2,4,8,16,32 or 0 for unlimited */
#define TXC_UCG		(1U<<18)	/* generate UDP checksum */
#define TXC_TCG		(1U<<17)	/* generate TCP checksum */
#define TXC_ICG		(1U<<16)	/* generate IP checksum */
#define TXC_FCE		(1U<<9)		/* generate PAUSE to moderate Rx lvl */
#define TXC_EP		(1U<<2)		/* enable automatic padding */
#define TXC_AC		(1U<<1)		/* add CRC to frame */
#define TXC_TEN		(1)		/* enable DMA to run */

#define RXC_BS_MSK	0x3f000000	/* burst size */
#define RXC_BS_SFT	(24)		/* 1,2,4,8,16,32 or 0 for unlimited */
#define RXC_IHAE	(1U<<19)	/* IP header alignment enable */
#define RXC_UCC		(1U<<18)	/* run UDP checksum */
#define RXC_TCC		(1U<<17)	/* run TDP checksum */
#define RXC_ICC		(1U<<16)	/* run IP checksum */
#define RXC_FCE		(1U<<9)		/* accept PAUSE to throttle Tx */
#define RXC_RB		(1U<<6)		/* receive broadcast frame */
#define RXC_RM		(1U<<5)		/* receive all multicast (inc. RB) */
#define RXC_RU		(1U<<4)		/* receive 16 additional unicasts */
#define RXC_RE		(1U<<3)		/* accept error frame */
#define RXC_RA		(1U<<2)		/* receive all frame */
#define RXC_MHTE	(1U<<1)		/* use multicast hash table */
#define RXC_REN		(1)		/* enable DMA to run */

#define INT_DMLCS	(1U<<31)	/* link status change */
#define INT_DMTS	(1U<<30)	/* sending desc. has posted Tx done */
#define INT_DMRS	(1U<<29)	/* frame was received */
#define INT_DMRBUS	(1U<<27)	/* Rx descriptor pool is full */
#define INT_DMxPSS	(3U<<25)	/* 26:25 DMA Tx/Rx have stopped */

struct tdes {
	uint32_t t0, t1, t2, t3;
};

struct rdes {
	uint32_t r0, r1, r2, r3;
};

#define T0_OWN		(1U<<31)	/* desc is ready to Tx */

#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_FS		(1U<<30)	/* first segment of frame */
#define R0_LS		(1U<<29)	/* last segment of frame */
#define R0_IPE		(1U<<28)	/* IP checksum error */
#define R0_TCPE		(1U<<27)	/* TCP checksum error */
#define R0_UDPE		(1U<<26)	/* UDP checksum error */
#define R0_ES		(1U<<25)	/* error summary */
#define R0_MF		(1U<<24)	/* multicast frame */
#define R0_SPN		0x00300000	/* 21:20 switch port 1/2 */
#define R0_ALIGN	0x00300000	/* 21:20 (KSZ8692P) Rx align amount */
#define R0_RE		(1U<<19)	/* MII reported error */
#define R0_TL		(1U<<18)	/* frame too long, beyond 1518 */
#define R0_RF		(1U<<17)	/* damaged runt frame */
#define R0_CE		(1U<<16)	/* CRC error */
#define R0_FT		(1U<<15)	/* frame type */
#define R0_FL_MASK	0x7ff		/* frame length 10:0 */

#define T1_IC		(1U<<31)	/* post interrupt on complete */
#define T1_FS		(1U<<30)	/* first segment of frame */
#define T1_LS		(1U<<29)	/* last segment of frame */
#define T1_IPCKG	(1U<<28)	/* generate IP checksum */
#define T1_TCPCKG	(1U<<27)	/* generate TCP checksum */
#define T1_UDPCKG	(1U<<26)	/* generate UDP checksum */
#define T1_TER		(1U<<25)	/* end of ring */
#define T1_SPN		0x00300000	/* 21:20 switch port 1/2 */
#define T1_TBS_MASK	0x7ff		/* segment size 10:0 */

#define R1_RER		(1U<<25)	/* end of ring */
#define R1_RBS_MASK	0x7fc		/* segment size 10:0 */

#define KSE_NTXSEGS		16
#define KSE_TXQUEUELEN		64
#define KSE_TXQUEUELEN_MASK	(KSE_TXQUEUELEN - 1)
#define KSE_TXQUEUE_GC		(KSE_TXQUEUELEN / 4)
#define KSE_NTXDESC		256
#define KSE_NTXDESC_MASK	(KSE_NTXDESC - 1)
#define KSE_NEXTTX(x)		(((x) + 1) & KSE_NTXDESC_MASK)
#define KSE_NEXTTXS(x)		(((x) + 1) & KSE_TXQUEUELEN_MASK)

#define KSE_NRXDESC		64
#define KSE_NRXDESC_MASK	(KSE_NRXDESC - 1)
#define KSE_NEXTRX(x)		(((x) + 1) & KSE_NRXDESC_MASK)

struct kse_control_data {
	struct tdes kcd_txdescs[KSE_NTXDESC];
	struct rdes kcd_rxdescs[KSE_NRXDESC];
};
#define KSE_CDOFF(x)		offsetof(struct kse_control_data, x)
#define KSE_CDTXOFF(x)		KSE_CDOFF(kcd_txdescs[(x)])
#define KSE_CDRXOFF(x)		KSE_CDOFF(kcd_rxdescs[(x)])

struct kse_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndesc;			/* # of descriptors used */
};

struct kse_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

struct kse_softc {
	device_t sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_size_t sc_memsize;		/* csr map size */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	pci_chipset_tag_t sc_pc;	/* PCI chipset tag */
	struct ethercom sc_ethercom;	/* Ethernet common data */
	void *sc_ih;			/* interrupt cookie */

	struct mii_data sc_mii;		/* mii 8841 */
	struct ifmedia sc_media;	/* ifmedia 8842 */
	int sc_flowflags;		/* 802.3x PAUSE flow control */

	callout_t  sc_tick_ch;		/* MII tick callout */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct kse_control_data *sc_control_data;
#define sc_txdescs	sc_control_data->kcd_txdescs
#define sc_rxdescs	sc_control_data->kcd_rxdescs

	struct kse_txsoft sc_txsoft[KSE_TXQUEUELEN];
	struct kse_rxsoft sc_rxsoft[KSE_NRXDESC];
	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next ready Tx descriptor */
	int sc_txsfree;			/* number of free Tx jobs */
	int sc_txsnext;			/* next ready Tx job */
	int sc_txsdirty;		/* dirty Tx jobs */
	int sc_rxptr;			/* next ready Rx descriptor/descsoft */

	uint32_t sc_txc, sc_rxc;
	uint32_t sc_t1csum;
	int sc_mcsum;
	uint32_t sc_inten;
	uint32_t sc_chip;

	krndsource_t rnd_source;	/* random source */

#ifdef KSE_EVENT_COUNTERS
	struct ksext {
		char evcntname[3][8];
		struct evcnt pev[3][34];
	} sc_ext;			/* switch statistics */
#endif
};

#define KSE_CDTXADDR(sc, x)	((sc)->sc_cddma + KSE_CDTXOFF((x)))
#define KSE_CDRXADDR(sc, x)	((sc)->sc_cddma + KSE_CDRXOFF((x)))

#define KSE_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > KSE_NTXDESC) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    KSE_CDTXOFF(__x), sizeof(struct tdes) *		\
		    (KSE_NTXDESC - __x), (ops));			\
		__n -= (KSE_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    KSE_CDTXOFF(__x), sizeof(struct tdes) * __n, (ops));	\
} while (/*CONSTCOND*/0)

#define KSE_CDRXSYNC(sc, x, ops)					\
do {									\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    KSE_CDRXOFF((x)), sizeof(struct rdes), (ops));		\
} while (/*CONSTCOND*/0)

#define KSE_INIT_RXDESC(sc, x)						\
do {									\
	struct kse_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct rdes *__rxd = &(sc)->sc_rxdescs[(x)];			\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
									\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->r2 = __rxs->rxs_dmamap->dm_segs[0].ds_addr;		\
	__rxd->r1 = R1_RBS_MASK /* __m->m_ext.ext_size */;		\
	__rxd->r0 = R0_OWN;						\
	KSE_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE); \
} while (/*CONSTCOND*/0)

u_int kse_burstsize = 8;	/* DMA burst length tuning knob */

#ifdef KSEDIAGNOSTIC
u_int kse_monitor_rxintr;	/* fragmented UDP csum HW bug hook */
#endif

static int kse_match(device_t, cfdata_t, void *);
static void kse_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(kse, sizeof(struct kse_softc),
    kse_match, kse_attach, NULL, NULL);

static int kse_ioctl(struct ifnet *, u_long, void *);
static void kse_start(struct ifnet *);
static void kse_watchdog(struct ifnet *);
static int kse_init(struct ifnet *);
static void kse_stop(struct ifnet *, int);
static void kse_reset(struct kse_softc *);
static void kse_set_rcvfilt(struct kse_softc *);
static int add_rxbuf(struct kse_softc *, int);
static void rxdrain(struct kse_softc *);
static int kse_intr(void *);
static void rxintr(struct kse_softc *);
static void txreap(struct kse_softc *);
static void lnkchg(struct kse_softc *);
static int kse_ifmedia_upd(struct ifnet *);
static void kse_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static void nopifmedia_sts(struct ifnet *, struct ifmediareq *);
static void phy_tick(void *);
int kse_mii_readreg(device_t, int, int, uint16_t *);
int kse_mii_writereg(device_t, int, int, uint16_t);
void kse_mii_statchg(struct ifnet *);
#ifdef KSE_EVENT_COUNTERS
static void stat_tick(void *);
static void zerostats(struct kse_softc *);
#endif

static const struct device_compatible_entry compat_data[] = {
	{ .id = PCI_ID_CODE(PCI_VENDOR_MICREL,
		PCI_PRODUCT_MICREL_KSZ8842) },
	{ .id = PCI_ID_CODE(PCI_VENDOR_MICREL,
		PCI_PRODUCT_MICREL_KSZ8841) },

	PCI_COMPAT_EOL
};

static int
kse_match(device_t parent, cfdata_t match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	return PCI_CLASS(pa->pa_class) == PCI_CLASS_NETWORK &&
	       pci_compatible_match(pa, compat_data);
}

static void
kse_attach(device_t parent, device_t self, void *aux)
{
	struct kse_softc *sc = device_private(self);
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mii_data * const mii = &sc->sc_mii;
	struct ifmedia *ifm;
	uint8_t enaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t seg;
	int i, error, nseg;
	char intrbuf[PCI_INTRSTR_LEN];

	aprint_normal(": Micrel KSZ%04x Ethernet (rev. 0x%02x)\n",
	    PCI_PRODUCT(pa->pa_id), PCI_REVISION(pa->pa_class));

	if (pci_mapreg_map(pa, 0x10,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT,
	    0, &sc->sc_st, &sc->sc_sh, NULL, &sc->sc_memsize) != 0) {
		aprint_error_dev(self, "unable to map device registers\n");
		return;
	}

	/* Make sure bus mastering is enabled. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Power up chip if necessary. */
	if ((error = pci_activate(pc, pa->pa_tag, self, NULL))
	    && error != EOPNOTSUPP) {
		aprint_error_dev(self, "cannot activate %d\n", error);
		return;
	}

	/* Map and establish our interrupt. */
	if (pci_intr_map(pa, &ih)) {
		aprint_error_dev(self, "unable to map interrupt\n");
		goto fail;
	}
	intrstr = pci_intr_string(pc, ih, intrbuf, sizeof(intrbuf));
	sc->sc_ih = pci_intr_establish_xname(pc, ih, IPL_NET, kse_intr, sc,
	    device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "unable to establish interrupt");
		if (intrstr != NULL)
			aprint_error(" at %s", intrstr);
		aprint_error("\n");
		goto fail;
	}
	aprint_normal_dev(self, "interrupting at %s\n", intrstr);

	sc->sc_dev = self;
	sc->sc_dmat = pa->pa_dmat;
	sc->sc_pc = pa->pa_pc;
	sc->sc_chip = PCI_PRODUCT(pa->pa_id);

	/*
	 * Read the Ethernet address from the EEPROM.
	 */
	i = CSR_READ_2(sc, MARL);
	enaddr[5] = i;
	enaddr[4] = i >> 8;
	i = CSR_READ_2(sc, MARM);
	enaddr[3] = i;
	enaddr[2] = i >> 8;
	i = CSR_READ_2(sc, MARH);
	enaddr[1] = i;
	enaddr[0] = i >> 8;
	aprint_normal_dev(self,
	    "Ethernet address %s\n", ether_sprintf(enaddr));

	/*
	 * Enable chip function.
	 */
	CSR_WRITE_2(sc, SIDER, 1);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct kse_control_data), PAGE_SIZE, 0, &seg, 1, &nseg, 0);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to allocate control data, error = %d\n", error);
		goto fail_0;
	}
	error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct kse_control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_COHERENT);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to map control data, error = %d\n", error);
		goto fail_1;
	}
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct kse_control_data), 1,
	    sizeof(struct kse_control_data), 0, 0, &sc->sc_cddmamap);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to create control data DMA map, "
		    "error = %d\n", error);
		goto fail_2;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct kse_control_data), NULL, 0);
	if (error != 0) {
		aprint_error_dev(self,
		    "unable to load control data DMA map, error = %d\n",
		    error);
		goto fail_3;
	}
	for (i = 0; i < KSE_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    KSE_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}
	for (i = 0; i < KSE_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create rx DMA map %d, error = %d\n",
			    i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	mii->mii_ifp = ifp;
	mii->mii_readreg = kse_mii_readreg;
	mii->mii_writereg = kse_mii_writereg;
	mii->mii_statchg = kse_mii_statchg;

	/* Initialize ifmedia structures. */
	if (sc->sc_chip == 0x8841) {
		/* use port 1 builtin PHY as index 1 device */
		sc->sc_ethercom.ec_mii = mii;
		ifm = &mii->mii_media;
		ifmedia_init(ifm, 0, kse_ifmedia_upd, kse_ifmedia_sts);
		mii_attach(sc->sc_dev, mii, 0xffffffff, 1 /* PHY1 */,
		    MII_OFFSET_ANY, MIIF_DOPAUSE);
		if (LIST_FIRST(&mii->mii_phys) == NULL) {
			ifmedia_add(ifm, IFM_ETHER | IFM_NONE, 0, NULL);
			ifmedia_set(ifm, IFM_ETHER | IFM_NONE);
		} else
			ifmedia_set(ifm, IFM_ETHER | IFM_AUTO);
	} else {
		/*
		 * pretend 100FDX w/ no alternative media selection.
		 * 8842 MAC is tied with a builtin 3 port switch. It can do
		 * 4 degree priotised rate control over either of tx/rx
		 * direction for any of ports, respectively. Tough, this
		 * driver leaves the rate unlimited intending 100Mbps maximum.
		 * 2 external ports behave in AN mode and this driver provides
		 * no mean to manipulate and see their operational details.
		 */
		sc->sc_ethercom.ec_ifmedia = ifm = &sc->sc_media;
		ifmedia_init(ifm, 0, NULL, nopifmedia_sts);
		ifmedia_add(ifm, IFM_ETHER | IFM_100_TX | IFM_FDX, 0, NULL);
		ifmedia_set(ifm, IFM_ETHER | IFM_100_TX | IFM_FDX);

		aprint_normal_dev(self,
		    "10baseT, 10baseT-FDX, 100baseTX, 100baseTX-FDX, auto\n");
	}
	ifm->ifm_media = ifm->ifm_cur->ifm_media; /* as if user has requested */

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = kse_ioctl;
	ifp->if_start = kse_start;
	ifp->if_watchdog = kse_watchdog;
	ifp->if_init = kse_init;
	ifp->if_stop = kse_stop;
	IFQ_SET_READY(&ifp->if_snd);

	/*
	 * capable of 802.1Q VLAN-sized frames and hw assisted tagging.
	 * can do IPv4, TCPv4, and UDPv4 checksums in hardware.
	 */
	sc->sc_ethercom.ec_capabilities = ETHERCAP_VLAN_MTU;
	ifp->if_capabilities =
	    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;

	sc->sc_flowflags = 0;

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr);

	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, phy_tick, sc);

	rnd_attach_source(&sc->rnd_source, device_xname(self),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

#ifdef KSE_EVENT_COUNTERS
	const char *events[34] = {
		"RxLoPriotyByte",
		"RxHiPriotyByte",
		"RxUndersizePkt",
		"RxFragments",
		"RxOversize",
		"RxJabbers",
		"RxSymbolError",
		"RxCRCError",
		"RxAlignmentError",
		"RxControl8808Pkts",
		"RxPausePkts",
		"RxBroadcast",
		"RxMulticast",
		"RxUnicast",
		"Rx64Octets",
		"Rx65To127Octets",
		"Rx128To255Octets",
		"Rx255To511Octets",
		"Rx512To1023Octets",
		"Rx1024To1522Octets",
		"TxLoPriotyByte",
		"TxHiPriotyByte",
		"TxLateCollision",
		"TxPausePkts",
		"TxBroadcastPkts",
		"TxMulticastPkts",
		"TxUnicastPkts",
		"TxDeferred",
		"TxTotalCollision",
		"TxExcessiveCollision",
		"TxSingleCollision",
		"TxMultipleCollision",
		"TxDropPkts",
		"RxDropPkts",
	};
	struct ksext *ee = &sc->sc_ext;
	int p = (sc->sc_chip == 0x8842) ? 3 : 1;
	for (i = 0; i < p; i++) {
		snprintf(ee->evcntname[i], sizeof(ee->evcntname[i]),
		    "%s.%d", device_xname(sc->sc_dev), i+1);
		for (int ev = 0; ev < 34; ev++) {
			evcnt_attach_dynamic(&ee->pev[i][ev], EVCNT_TYPE_MISC,
			    NULL, ee->evcntname[i], events[ev]);
		}
	}
#endif
	return;

 fail_5:
	for (i = 0; i < KSE_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_4:
	for (i = 0; i < KSE_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
 fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
 fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct kse_control_data));
 fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, nseg);
 fail_0:
	pci_intr_disestablish(pc, sc->sc_ih);
 fail:
	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_memsize);
	return;
}

static int
kse_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct kse_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifmedia *ifm;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
		/* Flow control requires full-duplex mode. */
		if (IFM_SUBTYPE(ifr->ifr_media) == IFM_AUTO ||
		    (ifr->ifr_media & IFM_FDX) == 0)
			ifr->ifr_media &= ~IFM_ETH_FMASK;
		if (IFM_SUBTYPE(ifr->ifr_media) != IFM_AUTO) {
			if ((ifr->ifr_media & IFM_ETH_FMASK) == IFM_FLOW) {
				/* We can do both TXPAUSE and RXPAUSE. */
				ifr->ifr_media |=
				    IFM_ETH_TXPAUSE | IFM_ETH_RXPAUSE;
			}
			sc->sc_flowflags = ifr->ifr_media & IFM_ETH_FMASK;
		}
		ifm = (sc->sc_chip == 0x8841)
		    ? &sc->sc_mii.mii_media : &sc->sc_media;
		error = ifmedia_ioctl(ifp, ifr, ifm, cmd);
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		if (error != ENETRESET)
			break;
		error = 0;
		if (cmd == SIOCSIFCAP)
			error = if_init(ifp);
		if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			kse_set_rcvfilt(sc);
		}
		break;
	}

	splx(s);

	return error;
}

static int
kse_init(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;
	uint32_t paddr;
	int i, error = 0;

	/* cancel pending I/O */
	kse_stop(ifp, 0);

	/* reset all registers but PCI configuration */
	kse_reset(sc);

	/* craft Tx descriptor ring */
	memset(sc->sc_txdescs, 0, sizeof(sc->sc_txdescs));
	for (i = 0, paddr = KSE_CDTXADDR(sc, 1); i < KSE_NTXDESC - 1; i++) {
		sc->sc_txdescs[i].t3 = paddr;
		paddr += sizeof(struct tdes);
	}
	sc->sc_txdescs[KSE_NTXDESC - 1].t3 = KSE_CDTXADDR(sc, 0);
	KSE_CDTXSYNC(sc, 0, KSE_NTXDESC,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);
	sc->sc_txfree = KSE_NTXDESC;
	sc->sc_txnext = 0;

	for (i = 0; i < KSE_TXQUEUELEN; i++)
		sc->sc_txsoft[i].txs_mbuf = NULL;
	sc->sc_txsfree = KSE_TXQUEUELEN;
	sc->sc_txsnext = 0;
	sc->sc_txsdirty = 0;

	/* craft Rx descriptor ring */
	memset(sc->sc_rxdescs, 0, sizeof(sc->sc_rxdescs));
	for (i = 0, paddr = KSE_CDRXADDR(sc, 1); i < KSE_NRXDESC - 1; i++) {
		sc->sc_rxdescs[i].r3 = paddr;
		paddr += sizeof(struct rdes);
	}
	sc->sc_rxdescs[KSE_NRXDESC - 1].r3 = KSE_CDRXADDR(sc, 0);
	for (i = 0; i < KSE_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_mbuf == NULL) {
			if ((error = add_rxbuf(sc, i)) != 0) {
				aprint_error_dev(sc->sc_dev,
				    "unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				    i, error);
				rxdrain(sc);
				goto out;
			}
		}
		else
			KSE_INIT_RXDESC(sc, i);
	}
	sc->sc_rxptr = 0;

	/* hand Tx/Rx rings to HW */
	CSR_WRITE_4(sc, TDLB, KSE_CDTXADDR(sc, 0));
	CSR_WRITE_4(sc, RDLB, KSE_CDRXADDR(sc, 0));

	sc->sc_txc = TXC_TEN | TXC_EP | TXC_AC;
	sc->sc_rxc = RXC_REN | RXC_RU | RXC_RB;
	sc->sc_t1csum = sc->sc_mcsum = 0;
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) {
		sc->sc_rxc |= RXC_ICC;
		sc->sc_mcsum |= M_CSUM_IPv4;
	}
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Tx) {
		sc->sc_txc |= TXC_ICG;
		sc->sc_t1csum |= T1_IPCKG;
	}
	if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Rx) {
		sc->sc_rxc |= RXC_TCC;
		sc->sc_mcsum |= M_CSUM_TCPv4;
	}
	if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Tx) {
		sc->sc_txc |= TXC_TCG;
		sc->sc_t1csum |= T1_TCPCKG;
	}
	if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Rx) {
		sc->sc_rxc |= RXC_UCC;
		sc->sc_mcsum |= M_CSUM_UDPv4;
	}
	if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Tx) {
		sc->sc_txc |= TXC_UCG;
		sc->sc_t1csum |= T1_UDPCKG;
	}
	sc->sc_txc |= (kse_burstsize << TXC_BS_SFT);
	sc->sc_rxc |= (kse_burstsize << RXC_BS_SFT);

	if (sc->sc_chip == 0x8842) {
		/* make PAUSE flow control to run */
		sc->sc_txc |= TXC_FCE;
		sc->sc_rxc |= RXC_FCE;
		i = CSR_READ_2(sc, SGCR3);
		CSR_WRITE_2(sc, SGCR3, i | CR3_USEFC);
	}

	/* accept multicast frame or run promisc mode */
	kse_set_rcvfilt(sc);

	/* set current media */
	if (sc->sc_chip == 0x8841)
		(void)kse_ifmedia_upd(ifp);

	/* enable transmitter and receiver */
	CSR_WRITE_4(sc, MDTXC, sc->sc_txc);
	CSR_WRITE_4(sc, MDRXC, sc->sc_rxc);
	CSR_WRITE_4(sc, MDRSC, 1);

	/* enable interrupts */
	sc->sc_inten = INT_DMTS | INT_DMRS | INT_DMRBUS;
	if (sc->sc_chip == 0x8841)
		sc->sc_inten |= INT_DMLCS;
	CSR_WRITE_4(sc, INTST, ~0);
	CSR_WRITE_4(sc, INTEN, sc->sc_inten);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* start one second timer */
	callout_schedule(&sc->sc_tick_ch, hz);

#ifdef KSE_EVENT_COUNTERS
	zerostats(sc);
#endif

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		aprint_error_dev(sc->sc_dev, "interface not running\n");
	}
	return error;
}

static void
kse_stop(struct ifnet *ifp, int disable)
{
	struct kse_softc *sc = ifp->if_softc;
	struct kse_txsoft *txs;
	int i;

	callout_stop(&sc->sc_tick_ch);

	sc->sc_txc &= ~TXC_TEN;
	sc->sc_rxc &= ~RXC_REN;
	CSR_WRITE_4(sc, MDTXC, sc->sc_txc);
	CSR_WRITE_4(sc, MDRXC, sc->sc_rxc);

	for (i = 0; i < KSE_TXQUEUELEN; i++) {
		txs = &sc->sc_txsoft[i];
		if (txs->txs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			txs->txs_mbuf = NULL;
		}
	}

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	if (disable)
		rxdrain(sc);
}

static void
kse_reset(struct kse_softc *sc)
{

	/* software reset */
	CSR_WRITE_2(sc, GRR, 1);
	delay(1000); /* PDF does not mention the delay amount */
	CSR_WRITE_2(sc, GRR, 0);

	/* enable switch function */
	CSR_WRITE_2(sc, SIDER, 1);
}

static void
kse_watchdog(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;

	/*
	 * Since we're not interrupting every packet, sweep
	 * up before we report an error.
	 */
	txreap(sc);

	if (sc->sc_txfree != KSE_NTXDESC) {
		aprint_error_dev(sc->sc_dev,
		    "device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_txfree, sc->sc_txsfree, sc->sc_txnext);
		if_statinc(ifp, if_oerrors);

		/* Reset the interface. */
		kse_init(ifp);
	}
	else if (ifp->if_flags & IFF_DEBUG)
		aprint_error_dev(sc->sc_dev, "recovered from device timeout\n");

	/* Try to get more packets going. */
	kse_start(ifp);
}

static void
kse_start(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct kse_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, nexttx, lasttx, ofree, seg;
	uint32_t tdes0;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/* Remember the previous number of free descriptors. */
	ofree = sc->sc_txfree;

	/*
	 * Loop through the send queue, setting up transmit descriptors
	 * until we drain the queue, or use up all available transmit
	 * descriptors.
	 */
	for (;;) {
		IFQ_POLL(&ifp->if_snd, m0);
		if (m0 == NULL)
			break;

		if (sc->sc_txsfree < KSE_TXQUEUE_GC) {
			txreap(sc);
			if (sc->sc_txsfree == 0)
				break;
		}
		txs = &sc->sc_txsoft[sc->sc_txsnext];
		dmamap = txs->txs_dmamap;

		error = bus_dmamap_load_mbuf(sc->sc_dmat, dmamap, m0,
		    BUS_DMA_WRITE | BUS_DMA_NOWAIT);
		if (error) {
			if (error == EFBIG) {
				aprint_error_dev(sc->sc_dev,
				    "Tx packet consumes too many "
				    "DMA segments, dropping...\n");
				    IFQ_DEQUEUE(&ifp->if_snd, m0);
				    m_freem(m0);
				    continue;
			}
			/* Short on resources, just stop for now. */
			break;
		}

		if (dmamap->dm_nsegs > sc->sc_txfree) {
			/*
			 * Not enough free descriptors to transmit this
			 * packet.  We haven't committed anything yet,
			 * so just unload the DMA map, put the packet
			 * back on the queue, and punt.	 Notify the upper
			 * layer that there are not more slots left.
			 */
			ifp->if_flags |= IFF_OACTIVE;
			bus_dmamap_unload(sc->sc_dmat, dmamap);
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m0);

		/*
		 * WE ARE NOW COMMITTED TO TRANSMITTING THE PACKET.
		 */

		bus_dmamap_sync(sc->sc_dmat, dmamap, 0, dmamap->dm_mapsize,
		    BUS_DMASYNC_PREWRITE);

		tdes0 = 0; /* to postpone 1st segment T0_OWN write */
		lasttx = -1;
		for (nexttx = sc->sc_txnext, seg = 0;
		     seg < dmamap->dm_nsegs;
		     seg++, nexttx = KSE_NEXTTX(nexttx)) {
			struct tdes *tdes = &sc->sc_txdescs[nexttx];
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.	 That could cause a race condition.
			 * We'll do it below.
			 */
			tdes->t2 = dmamap->dm_segs[seg].ds_addr;
			tdes->t1 = sc->sc_t1csum
			     | (dmamap->dm_segs[seg].ds_len & T1_TBS_MASK);
			tdes->t0 = tdes0;
			tdes0 = T0_OWN; /* 2nd and other segments */
			lasttx = nexttx;
		}
		/*
		 * Outgoing NFS mbuf must be unloaded when Tx completed.
		 * Without T1_IC NFS mbuf is left unack'ed for excessive
		 * time and NFS stops to proceed until kse_watchdog()
		 * calls txreap() to reclaim the unack'ed mbuf.
		 * It's painful to traverse every mbuf chain to determine
		 * whether someone is waiting for Tx completion.
		 */
		m = m0;
		do {
			if ((m->m_flags & M_EXT) && m->m_ext.ext_free) {
				sc->sc_txdescs[lasttx].t1 |= T1_IC;
				break;
			}
		} while ((m = m->m_next) != NULL);

		/* Write deferred 1st segment T0_OWN at the final stage */
		sc->sc_txdescs[lasttx].t1 |= T1_LS;
		sc->sc_txdescs[sc->sc_txnext].t1 |= T1_FS;
		sc->sc_txdescs[sc->sc_txnext].t0 = T0_OWN;
		KSE_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Tell DMA start transmit */
		CSR_WRITE_4(sc, MDTSC, 1);

		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;
		txs->txs_ndesc = dmamap->dm_nsegs;

		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;
		sc->sc_txsfree--;
		sc->sc_txsnext = KSE_NEXTTXS(sc->sc_txsnext);
		/*
		 * Pass the packet to any BPF listeners.
		 */
		bpf_mtap(ifp, m0, BPF_D_OUT);
	}

	if (sc->sc_txsfree == 0 || sc->sc_txfree == 0) {
		/* No more slots left; notify upper layer. */
		ifp->if_flags |= IFF_OACTIVE;
	}
	if (sc->sc_txfree != ofree) {
		/* Set a watchdog timer in case the chip flakes out. */
		ifp->if_timer = 5;
	}
}

static void
kse_set_rcvfilt(struct kse_softc *sc)
{
	struct ether_multistep step;
	struct ether_multi *enm;
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	uint32_t crc, mchash[2];
	int i;

	sc->sc_rxc &= ~(RXC_MHTE | RXC_RM | RXC_RA);

	/* clear perfect match filter and prepare mcast hash table */
	for (i = 0; i < 16; i++)
		 CSR_WRITE_4(sc, MAAH0 + i*8, 0);
	crc = mchash[0] = mchash[1] = 0;

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		/* run promisc. mode */
		sc->sc_rxc |= RXC_RA;
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	i = 0;
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
			ec->ec_flags |= ETHER_F_ALLMULTI;
			ETHER_UNLOCK(ec);
			/* accept all multicast */
			sc->sc_rxc |= RXC_RM;
			goto update;
		}
#if KSE_MCASTDEBUG == 1
		printf("[%d] %s\n", i, ether_sprintf(enm->enm_addrlo));
#endif
		if (i < 16) {
			/* use 16 additional MAC addr to accept mcast */
			uint32_t addr;
			uint8_t *ep = enm->enm_addrlo;
			addr = (ep[3] << 24) | (ep[2] << 16)
			     | (ep[1] << 8)  |  ep[0];
			CSR_WRITE_4(sc, MAAL0 + i*8, addr);
			addr = (ep[5] << 8) | ep[4];
			CSR_WRITE_4(sc, MAAH0 + i*8, addr | (1U << 31));
		} else {
			/* use hash table when too many */
			crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
			mchash[crc >> 31] |= 1 << ((crc >> 26) & 0x1f);
		}
		ETHER_NEXT_MULTI(step, enm);
		i++;
	}
	ETHER_UNLOCK(ec);

	if (crc)
		sc->sc_rxc |= RXC_MHTE;
	CSR_WRITE_4(sc, MTR0, mchash[0]);
	CSR_WRITE_4(sc, MTR1, mchash[1]);
 update:
	/* With RA or RM, MHTE/MTR0/MTR1 are never consulted. */
	return;
}

static int
add_rxbuf(struct kse_softc *sc, int idx)
{
	struct kse_rxsoft *rxs = &sc->sc_rxsoft[idx];
	struct mbuf *m;
	int error;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;

	MCLGET(m, M_DONTWAIT);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return ENOBUFS;
	}

	if (rxs->rxs_mbuf != NULL)
		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);

	rxs->rxs_mbuf = m;

	error = bus_dmamap_load(sc->sc_dmat, rxs->rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		aprint_error_dev(sc->sc_dev,
		    "can't load rx DMA map %d, error = %d\n", idx, error);
		panic("kse_add_rxbuf");
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);

	KSE_INIT_RXDESC(sc, idx);

	return 0;
}

static void
rxdrain(struct kse_softc *sc)
{
	struct kse_rxsoft *rxs;
	int i;

	for (i = 0; i < KSE_NRXDESC; i++) {
		rxs = &sc->sc_rxsoft[i];
		if (rxs->rxs_mbuf != NULL) {
			bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
			m_freem(rxs->rxs_mbuf);
			rxs->rxs_mbuf = NULL;
		}
	}
}

static int
kse_intr(void *arg)
{
	struct kse_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	uint32_t isr;

	if ((isr = CSR_READ_4(sc, INTST)) == 0)
		return 0;

	if (isr & INT_DMRS)
		rxintr(sc);
	if (isr & INT_DMTS)
		txreap(sc);
	if (isr & INT_DMLCS)
		lnkchg(sc);
	if (isr & INT_DMRBUS)
		aprint_error_dev(sc->sc_dev, "Rx descriptor full\n");

	CSR_WRITE_4(sc, INTST, isr);

	if (ifp->if_flags & IFF_RUNNING)
		if_schedule_deferred_start(ifp);

	return 1;
}

static void
rxintr(struct kse_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct kse_rxsoft *rxs;
	struct mbuf *m;
	uint32_t rxstat;
	int i, len;

	for (i = sc->sc_rxptr; /*CONSTCOND*/ 1; i = KSE_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		KSE_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].r0;

		if (rxstat & R0_OWN) /* desc is left empty */
			break;

		/* R0_FS | R0_LS must have been marked for this desc */

		if (rxstat & R0_ES) {
			if_statinc(ifp, if_ierrors);
#define PRINTERR(bit, str)						\
			if (rxstat & (bit))				\
				aprint_error_dev(sc->sc_dev,		\
				    "%s\n", str)
			PRINTERR(R0_TL, "frame too long");
			PRINTERR(R0_RF, "runt frame");
			PRINTERR(R0_CE, "bad FCS");
#undef PRINTERR
			KSE_INIT_RXDESC(sc, i);
			continue;
		}

		/* HW errata; frame might be too small or too large */

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		len = rxstat & R0_FL_MASK;
		len -= ETHER_CRC_LEN;	/* Trim CRC off */
		m = rxs->rxs_mbuf;

		if (add_rxbuf(sc, i) != 0) {
			if_statinc(ifp, if_ierrors);
			KSE_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat,
			    rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize,
			    BUS_DMASYNC_PREREAD);
			continue;
		}

		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = len;

		if (sc->sc_mcsum) {
			m->m_pkthdr.csum_flags |= sc->sc_mcsum;
			if (rxstat & R0_IPE)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
			if (rxstat & (R0_TCPE | R0_UDPE))
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}
		if_percpuq_enqueue(ifp->if_percpuq, m);
#ifdef KSEDIAGNOSTIC
		if (kse_monitor_rxintr > 0) {
			aprint_error_dev(sc->sc_dev,
			    "m stat %x data %p len %d\n",
			    rxstat, m->m_data, m->m_len);
		}
#endif
	}
	sc->sc_rxptr = i;
}

static void
txreap(struct kse_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct kse_txsoft *txs;
	uint32_t txstat;
	int i;

	ifp->if_flags &= ~IFF_OACTIVE;

	for (i = sc->sc_txsdirty; sc->sc_txsfree != KSE_TXQUEUELEN;
	     i = KSE_NEXTTXS(i), sc->sc_txsfree++) {
		txs = &sc->sc_txsoft[i];

		KSE_CDTXSYNC(sc, txs->txs_firstdesc, txs->txs_ndesc,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		txstat = sc->sc_txdescs[txs->txs_lastdesc].t0;

		if (txstat & T0_OWN) /* desc is still in use */
			break;

		/* There is no way to tell transmission status per frame */

		if_statinc(ifp, if_opackets);

		sc->sc_txfree += txs->txs_ndesc;
		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
	}
	sc->sc_txsdirty = i;
	if (sc->sc_txsfree == KSE_TXQUEUELEN)
		ifp->if_timer = 0;
}

static void
lnkchg(struct kse_softc *sc)
{
	struct ifmediareq ifmr;

#if KSE_LINKDEBUG == 1
	uint16_t p1sr = CSR_READ_2(sc, P1SR);
printf("link %s detected\n", (p1sr & PxSR_LINKUP) ? "up" : "down");
#endif
	kse_ifmedia_sts(&sc->sc_ethercom.ec_if, &ifmr);
}

static int
kse_ifmedia_upd(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_mii.mii_media;
	uint16_t p1cr4;

	p1cr4 = 0;
	if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_AUTO) {
		p1cr4 |= PxCR_STARTNEG;	/* restart AN */
		p1cr4 |= PxCR_AUTOEN;	/* enable AN */
		p1cr4 |= PxCR_USEFC;	/* advertise flow control pause */
		p1cr4 |= 0xf;		/* adv. 100FDX,100HDX,10FDX,10HDX */
	} else {
		if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_100_TX)
			p1cr4 |= PxCR_SPD100;
		if (ifm->ifm_media & IFM_FDX)
			p1cr4 |= PxCR_USEFDX;
	}
	CSR_WRITE_2(sc, P1CR4, p1cr4);
#if KSE_LINKDEBUG == 1
printf("P1CR4: %04x\n", p1cr4);
#endif
	return 0;
}

static void
kse_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct kse_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;

	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = sc->sc_flowflags |
	    (mii->mii_media_active & ~IFM_ETH_FMASK);
}

static void
nopifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct kse_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_media;

#if KSE_LINKDEBUG == 2
printf("p1sr: %04x, p2sr: %04x\n", CSR_READ_2(sc, P1SR), CSR_READ_2(sc, P2SR));
#endif

	/* 8842 MAC pretends 100FDX all the time */
	ifmr->ifm_status = IFM_AVALID | IFM_ACTIVE;
	ifmr->ifm_active = ifm->ifm_cur->ifm_media |
	    IFM_FLOW | IFM_ETH_RXPAUSE | IFM_ETH_TXPAUSE;
}

static void
phy_tick(void *arg)
{
	struct kse_softc *sc = arg;
	struct mii_data *mii = &sc->sc_mii;
	int s;

	if (sc->sc_chip == 0x8841) {
		s = splnet();
		mii_tick(mii);
		splx(s);
	}
#ifdef KSE_EVENT_COUNTERS
	stat_tick(arg);
#endif
	callout_schedule(&sc->sc_tick_ch, hz);
}

static const uint16_t phy1csr[] = {
	/* 0 BMCR */	0x4d0,
	/* 1 BMSR */	0x4d2,
	/* 2 PHYID1 */	0x4d6,	/* 0x0022 - PHY1HR */
	/* 3 PHYID2 */	0x4d4,	/* 0x1430 - PHY1LR */
	/* 4 ANAR */	0x4d8,
	/* 5 ANLPAR */	0x4da,
};

int
kse_mii_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	struct kse_softc *sc = device_private(self);

	if (phy != 1 || reg >= __arraycount(phy1csr) || reg < 0)
		return EINVAL;
	*val = CSR_READ_2(sc, phy1csr[reg]);
	return 0;
}

int
kse_mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct kse_softc *sc = device_private(self);

	if (phy != 1 || reg >= __arraycount(phy1csr) || reg < 0)
		return EINVAL;
	CSR_WRITE_2(sc, phy1csr[reg], val);
	return 0;
}

void
kse_mii_statchg(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;

#if KSE_LINKDEBUG == 1
	/* decode P1SR register value */
	uint16_t p1sr = CSR_READ_2(sc, P1SR);
	printf("P1SR %04x, spd%d", p1sr, (p1sr & PxSR_SPD100) ? 100 : 10);
	if (p1sr & PxSR_FDX)
		printf(",full-duplex");
	if (p1sr & PxSR_RXFLOW)
		printf(",rxpause");
	if (p1sr & PxSR_TXFLOW)
		printf(",txpause");
	printf("\n");
	/* show resolved mii(4) parameters to compare against above */
	printf("MII spd%d",
	    (int)(sc->sc_ethercom.ec_if.if_baudrate / IF_Mbps(1)));
	if (mii->mii_media_active & IFM_FDX)
		printf(",full-duplex");
	if (mii->mii_media_active & IFM_FLOW) {
		printf(",flowcontrol");
		if (mii->mii_media_active & IFM_ETH_RXPAUSE)
			printf(",rxpause");
		if (mii->mii_media_active & IFM_ETH_TXPAUSE)
			printf(",txpause");
	}
	printf("\n");
#endif
	/* Get flow control negotiation result. */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->sc_flowflags)
		sc->sc_flowflags = mii->mii_media_active & IFM_ETH_FMASK;

	/* Adjust MAC PAUSE flow control. */
	if ((mii->mii_media_active & IFM_FDX)
	    && (sc->sc_flowflags & IFM_ETH_TXPAUSE))
		sc->sc_txc |= TXC_FCE;
	else
		sc->sc_txc &= ~TXC_FCE;
	if ((mii->mii_media_active & IFM_FDX)
	    && (sc->sc_flowflags & IFM_ETH_RXPAUSE))
		sc->sc_rxc |= RXC_FCE;
	else
		sc->sc_rxc &= ~RXC_FCE;
	CSR_WRITE_4(sc, MDTXC, sc->sc_txc);
	CSR_WRITE_4(sc, MDRXC, sc->sc_rxc);
#if KSE_LINKDEBUG == 1
	printf("%ctxfce, %crxfce\n",
	    (sc->sc_txc & TXC_FCE) ? '+' : '-',
	    (sc->sc_rxc & RXC_FCE) ? '+' : '-');
#endif
}

#ifdef KSE_EVENT_COUNTERS
static void
stat_tick(void *arg)
{
	struct kse_softc *sc = arg;
	struct ksext *ee = &sc->sc_ext;
	int nport, p, i, reg, val;

	nport = (sc->sc_chip == 0x8842) ? 3 : 1;
	for (p = 0; p < nport; p++) {
		/* read 34 ev counters by indirect read via IACR */
		for (i = 0; i < 32; i++) {
			reg = EVCNTBR + p * 0x20 + i;
			CSR_WRITE_2(sc, IACR, reg);
			/* 30-bit counter value are halved in IADR5 & IADR4 */
			do {
				val = CSR_READ_2(sc, IADR5) << 16;
			} while ((val & IADR_LATCH) == 0);
			if (val & IADR_OVF) {
				(void)CSR_READ_2(sc, IADR4);
				val = 0x3fffffff; /* has made overflow */
			}
			else {
				val &= 0x3fff0000;		/* 29:16 */
				val |= CSR_READ_2(sc, IADR4);	/* 15:0 */
			}
			ee->pev[p][i].ev_count += val; /* ev0 thru 31 */
		}
		/* ev32 and ev33 are 16-bit counter */
		CSR_WRITE_2(sc, IACR, EVCNTBR + 0x100 + p);
		ee->pev[p][32].ev_count += CSR_READ_2(sc, IADR4); /* ev32 */
		CSR_WRITE_2(sc, IACR, EVCNTBR + 0x100 + p * 3 + 1);
		ee->pev[p][33].ev_count += CSR_READ_2(sc, IADR4); /* ev33 */
	}
}

static void
zerostats(struct kse_softc *sc)
{
	struct ksext *ee = &sc->sc_ext;
	int nport, p, i, reg, val;

	/* Make sure all the HW counters get zero */
	nport = (sc->sc_chip == 0x8842) ? 3 : 1;
	for (p = 0; p < nport; p++) {
		for (i = 0; i < 32; i++) {
			reg = EVCNTBR + p * 0x20 + i;
			CSR_WRITE_2(sc, IACR, reg);
			do {
				val = CSR_READ_2(sc, IADR5) << 16;
			} while ((val & IADR_LATCH) == 0);
			(void)CSR_READ_2(sc, IADR4);
			ee->pev[p][i].ev_count = 0;
		}
		CSR_WRITE_2(sc, IACR, EVCNTBR + 0x100 + p);
		(void)CSR_READ_2(sc, IADR4);
		CSR_WRITE_2(sc, IACR, EVCNTBR + 0x100 + p * 3 + 1);
		(void)CSR_READ_2(sc, IADR4);
		ee->pev[p][32].ev_count = 0;
		ee->pev[p][33].ev_count = 0;
	}
}
#endif
