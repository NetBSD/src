/*	$NetBSD: if_kse.c,v 1.4 2007/07/09 21:00:53 ad Exp $	*/

/*
 * Copyright (c) 2006 Tohru Nishimura
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
 *	This product includes software developed by Tohru Nishimura.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_kse.c,v 1.4 2007/07/09 21:00:53 ad Exp $");

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>
#include <sys/queue.h>

#include <machine/endian.h>
#include <machine/bus.h>
#include <machine/intr.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#define CSR_READ_4(sc, off) \
	    bus_space_read_4(sc->sc_st, sc->sc_sh, off)
#define CSR_WRITE_4(sc, off, val) \
	    bus_space_write_4(sc->sc_st, sc->sc_sh, off, val)
#define CSR_READ_2(sc, off) \
	    bus_space_read_2(sc->sc_st, sc->sc_sh, off)
#define CSR_WRITE_2(sc, off, val) \
	    bus_space_write_2(sc->sc_st, sc->sc_sh, off, val)

#define MDTXC	0x000	/* DMA transmit control */
#define MDRXC	0x004	/* DMA receive control */
#define MDTSC	0x008	/* DMA transmit start */
#define MDRSC	0x00c	/* DMA receive start */
#define TDLB	0x010	/* transmit descriptor list base */
#define RDLB	0x014	/* receive descriptor list base */
#define INTEN	0x028	/* interrupt enable */
#define INTST	0x02c	/* interrupt status */
#define MARL	0x200	/* MAC address low */
#define MARM	0x202	/* MAC address middle */
#define MARH	0x204	/* MAC address high */
#define GRR	0x216	/* global reset */
#define CIDR	0x400	/* chip ID and enable */
#define CGCR	0x40a	/* chip global control */
#define P1CR4	0x512	/* port 1 control 4 */
#define P1SR	0x514	/* port 1 status */

#define TXC_BS_MSK	0x3f000000	/* burst size */
#define TXC_BS_SFT	(24)		/* 1,2,4,8,16,32 or 0 for unlimited */
#define TXC_UCG		(1U<<18)	/* generate UDP checksum */
#define TXC_TCG		(1U<<17)	/* generate TCP checksum */
#define TXC_ICG		(1U<<16)	/* generate IP checksum */
#define TXC_FCE		(1U<<9)		/* enable flowcontrol */
#define TXC_EP		(1U<<2)		/* enable automatic padding */
#define TXC_AC		(1U<<1)		/* add CRC to frame */
#define TXC_TEN		(1)		/* enable DMA to run */

#define RXC_BS_MSK	0x3f000000	/* burst size */
#define RXC_BS_SFT	(24)		/* 1,2,4,8,16,32 or 0 for unlimited */
#define RXC_UCG		(1U<<18)	/* run UDP checksum */
#define RXC_TCG		(1U<<17)	/* run TDP checksum */
#define RXC_ICG		(1U<<16)	/* run IP checksum */
#define RXC_FCE		(1U<<9)		/* enable flowcontrol */
#define RXC_RB		(1U<<6)		/* receive broadcast frame */
#define RXC_RM		(1U<<5)		/* receive multicast frame */
#define RXC_RU		(1U<<4)		/* receive unicast frame */
#define RXC_RE		(1U<<3)		/* accept error frame */
#define RXC_RA		(1U<<2)		/* receive all frame */
#define RXC_MA		(1U<<1)		/* receive through hash filter */
#define RXC_REN		(1)		/* enable DMA to run */

#define INT_DMLCS	(1U<<31)	/* link status change */
#define INT_DMTS	(1U<<30)	/* sending desc. has posted Tx done */
#define INT_DMRS	(1U<<29)	/* frame was received */
#define INT_DMRBUS	(1U<<27)	/* Rx descriptor pool is full */

#define T0_OWN		(1U<<31)	/* desc is ready to Tx */

#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_FS		(1U<<30)	/* first segment of frame */
#define R0_LS		(1U<<29)	/* last segment of frame */
#define R0_IPE		(1U<<28)	/* IP checksum error */
#define R0_TCPE		(1U<<27)	/* TCP checksum error */
#define R0_UDPE		(1U<<26)	/* UDP checksum error */
#define R0_ES		(1U<<25)	/* error summary */
#define R0_MF		(1U<<24)	/* multicast frame */
#define R0_RE		(1U<<19)	/* framing error */
#define R0_TL		(1U<<18)	/* too long frame */
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
#define T1_TBS_MASK	0x7ff		/* segment size 10:0 */

#define R1_RER		(1U<<25)	/* end of ring */
#define R1_RBS_MASK	0x7ff		/* segment size 10:0 */

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

struct tdes {
	uint32_t t0, t1, t2, t3;
};

struct rdes {
	uint32_t r0, r1, r2, r3;
};

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
	struct device sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* Ethernet common data */
	void *sc_ih;			/* interrupt cookie */

	struct ifmedia sc_media;	/* ifmedia information */
	int sc_media_status;		/* PHY */
	int sc_media_active;		/* PHY */
	callout_t sc_callout;		/* tick callout */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct kse_control_data *sc_control_data;
#define sc_txdescs      sc_control_data->kcd_txdescs
#define sc_rxdescs      sc_control_data->kcd_rxdescs

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
	uint32_t sc_chip;
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
	/*								\
	 * Note: may be able to scoot the packet forward 2 bytes for	\
	 * the alignment. Unclear KS8842 Rx DMA really mandates to have	\
	 * 32-bit buffer boundary. Tx DMA has no alignment limitation.	\
	 */								\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->r2 = __rxs->rxs_dmamap->dm_segs[0].ds_addr;		\
	__rxd->r1 = R1_RBS_MASK /* __m->m_ext.ext_size */;		\
	__rxd->r0 = R0_OWN;						\
	KSE_CDRXSYNC((sc), (x), BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE); \
} while (/*CONSTCOND*/0)

u_int kse_burstsize = 16;	/* DMA burst length tuning knob */

#ifdef KSEDIAGNOSTIC
u_int kse_monitor_rxintr;	/* fragmented UDP csum HW bug hook */
#endif

static int kse_match(struct device *, struct cfdata *, void *);
static void kse_attach(struct device *, struct device *, void *);

CFATTACH_DECL(kse, sizeof(struct kse_softc),
    kse_match, kse_attach, NULL, NULL);

static int kse_ioctl(struct ifnet *, u_long, void *);
static void kse_start(struct ifnet *);
static void kse_watchdog(struct ifnet *);
static int kse_init(struct ifnet *);
static void kse_stop(struct ifnet *, int);
static void kse_reset(struct kse_softc *);
static void kse_set_filter(struct kse_softc *);
static int add_rxbuf(struct kse_softc *, int);
static void rxdrain(struct kse_softc *);
static int kse_intr(void *);
static void rxintr(struct kse_softc *);
static void txreap(struct kse_softc *);
static void lnkchg(struct kse_softc *);
static int ifmedia_upd(struct ifnet *);
static void ifmedia_sts(struct ifnet *, struct ifmediareq *);
static void phy_tick(void *);

static int
kse_match(struct device *parent, struct cfdata *match, void *aux)
{
	struct pci_attach_args *pa = (struct pci_attach_args *)aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_MICREL &&
	     (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_MICREL_KSZ8842 ||
	      PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_MICREL_KSZ8841) &&
	    PCI_CLASS(pa->pa_class) == PCI_CLASS_NETWORK)
		return 1;

	return 0;
}

static void
kse_attach(struct device *parent, struct device *self, void *aux)
{
	struct kse_softc *sc = (struct kse_softc *)self;
	struct pci_attach_args *pa = aux;
	pci_chipset_tag_t pc = pa->pa_pc;
	pci_intr_handle_t ih;
	const char *intrstr;
	struct ifnet *ifp;
	uint8_t enaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t seg;
	int error, i, nseg;
	pcireg_t pmode;
	int pmreg;

	if (pci_mapreg_map(pa, 0x10,
	    PCI_MAPREG_TYPE_MEM | PCI_MAPREG_MEM_TYPE_32BIT,
	    0, &sc->sc_st, &sc->sc_sh, NULL, NULL) != 0) {
		printf(": unable to map device registers\n");
		return;
	}

	sc->sc_dmat = pa->pa_dmat;

	/* Make sure bus mastering is enabled. */
	pci_conf_write(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG,
	    pci_conf_read(pc, pa->pa_tag, PCI_COMMAND_STATUS_REG) |
	    PCI_COMMAND_MASTER_ENABLE);

	/* Get it out of power save mode, if needed. */
	if (pci_get_capability(pc, pa->pa_tag, PCI_CAP_PWRMGMT, &pmreg, 0)) {
		pmode = pci_conf_read(pc, pa->pa_tag, pmreg + PCI_PMCSR) &
		    PCI_PMCSR_STATE_MASK;
		if (pmode == PCI_PMCSR_STATE_D3) {
			/*
			 * The card has lost all configuration data in
			 * this state, so punt.
			 */
			printf("%s: unable to wake from power state D3\n",
			    sc->sc_dev.dv_xname);
			return;
		}
		if (pmode != PCI_PMCSR_STATE_D0) {
			printf("%s: waking up from power date D%d\n",
			    sc->sc_dev.dv_xname, pmode);
			pci_conf_write(pc, pa->pa_tag, pmreg + PCI_PMCSR,
			    PCI_PMCSR_STATE_D0);
		}
	}

	sc->sc_chip = PCI_PRODUCT(pa->pa_id);
	printf(": Micrel KSZ%04x Ethernet (rev. 0x%02x)\n",
	    sc->sc_chip, PCI_REVISION(pa->pa_class));

	/*
	 * Read the Ethernet address from the EEPROM.
	 */
	i = CSR_READ_2(sc, MARL);
	enaddr[5] = i; enaddr[4] = i >> 8;
	i = CSR_READ_2(sc, MARM);
	enaddr[3] = i; enaddr[2] = i >> 8;
	i = CSR_READ_2(sc, MARH);
	enaddr[1] = i; enaddr[0] = i >> 8;
	printf("%s: Ethernet address: %s\n",
		sc->sc_dev.dv_xname, ether_sprintf(enaddr));

	/*
	 * Enable chip function.
	 */
	CSR_WRITE_2(sc, CIDR, 1);

	/*
	 * Map and establish our interrupt.
	 */
	if (pci_intr_map(pa, &ih)) {
		printf("%s: unable to map interrupt\n", sc->sc_dev.dv_xname);
		return;
	}
	intrstr = pci_intr_string(pc, ih);
	sc->sc_ih = pci_intr_establish(pc, ih, IPL_NET, kse_intr, sc);
	if (sc->sc_ih == NULL) {
		printf("%s: unable to establish interrupt",
		    sc->sc_dev.dv_xname);
		if (intrstr != NULL)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct kse_control_data), PAGE_SIZE, 0, &seg, 1, &nseg, 0);
	if (error != 0) {
		printf("%s: unable to allocate control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_0;
	}
	error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct kse_control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_COHERENT);
	if (error != 0) {
		printf("%s: unable to map control data, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_1;
	}
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct kse_control_data), 1,
	    sizeof(struct kse_control_data), 0, 0, &sc->sc_cddmamap);
	if (error != 0) {
		printf("%s: unable to create control data DMA map, "
		    "error = %d\n", sc->sc_dev.dv_xname, error);
		goto fail_2;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct kse_control_data), NULL, 0);
	if (error != 0) {
		printf("%s: unable to load control data DMA map, error = %d\n",
		    sc->sc_dev.dv_xname, error);
		goto fail_3;
	}
	for (i = 0; i < KSE_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    KSE_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			printf("%s: unable to create tx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_4;
		}
	}
	for (i = 0; i < KSE_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			printf("%s: unable to create rx DMA map %d, "
			    "error = %d\n", sc->sc_dev.dv_xname, i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	callout_init(&sc->sc_callout, 0);

	ifmedia_init(&sc->sc_media, 0, ifmedia_upd, ifmedia_sts);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_10_T, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_10_T|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_100_TX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_100_TX|IFM_FDX, 0, NULL);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_AUTO);

	printf("%s: 10baseT, 10baseT-FDX, 100baseTX, 100baseTX-FDX, auto\n",
	    sc->sc_dev.dv_xname);

	ifp = &sc->sc_ethercom.ec_if;
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = kse_ioctl;
	ifp->if_start = kse_start;
	ifp->if_watchdog = kse_watchdog;
	ifp->if_init = kse_init;
	ifp->if_stop = kse_stop;
	IFQ_SET_READY(&ifp->if_snd);

	/*
	 * KSZ8842 can handle 802.1Q VLAN-sized frames,
	 * can do IPv4, TCPv4, and UDPv4 checksums in hardware.
	 */
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;
	ifp->if_capabilities |=
	    IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_TCPv4_Rx |
	    IFCAP_CSUM_UDPv4_Tx | IFCAP_CSUM_UDPv4_Rx;

	if_attach(ifp);
	ether_ifattach(ifp, enaddr);
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
	return;
}

static int
kse_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct kse_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error;

	s = splnet();

	switch (cmd) {
	case SIOCSIFMEDIA:
	case SIOCGIFMEDIA:
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (cmd == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			kse_set_filter(sc);
			error = 0;
		}
		break;
	}

	kse_start(ifp);

	splx(s);
	return error;
}

#define KSE_INTRS (INT_DMLCS|INT_DMTS|INT_DMRS|INT_DMRBUS)

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
				printf("%s: unable to allocate or map rx "
				    "buffer %d, error = %d\n",
				     sc->sc_dev.dv_xname, i, error);
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

	sc->sc_txc = TXC_TEN | TXC_EP | TXC_AC | TXC_FCE;
	sc->sc_rxc = RXC_REN | RXC_RU | RXC_FCE;
	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_rxc |= RXC_RA;
	if (ifp->if_flags & IFF_BROADCAST)
		sc->sc_rxc |= RXC_RB;

	sc->sc_t1csum = sc->sc_mcsum = 0;
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx) {
		sc->sc_rxc |= RXC_ICG;
		sc->sc_mcsum |= M_CSUM_IPv4;
	}
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Tx) {
		sc->sc_txc |= TXC_ICG;
		sc->sc_t1csum |= T1_IPCKG;
	}
	if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Rx) {
		sc->sc_rxc |= RXC_TCG;
		sc->sc_mcsum |= M_CSUM_TCPv4;
	}
	if (ifp->if_capenable & IFCAP_CSUM_TCPv4_Tx) {
		sc->sc_txc |= TXC_TCG;
		sc->sc_t1csum |= T1_TCPCKG;
	}
	if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Rx) {
		sc->sc_rxc |= RXC_UCG;
		sc->sc_mcsum |= M_CSUM_UDPv4;
	}
	if (ifp->if_capenable & IFCAP_CSUM_UDPv4_Tx) {
		sc->sc_txc |= TXC_UCG;
		sc->sc_t1csum |= T1_UDPCKG;
	}
	sc->sc_txc |= (kse_burstsize << TXC_BS_SFT);
	sc->sc_rxc |= (kse_burstsize << RXC_BS_SFT);

	/* set current media */
	(void)ifmedia_upd(ifp);

	/* enable transmitter and receiver */
	CSR_WRITE_4(sc, MDTXC, sc->sc_txc);
	CSR_WRITE_4(sc, MDRXC, sc->sc_rxc);
	CSR_WRITE_4(sc, MDRSC, 1);

	/* enable interrupts */
	CSR_WRITE_4(sc, INTST, ~0);
	CSR_WRITE_4(sc, INTEN, KSE_INTRS);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* start one second timer */
	callout_reset(&sc->sc_callout, hz, phy_tick, sc);

 out:
	if (error) {
		ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
		ifp->if_timer = 0;
		printf("%s: interface not running\n", sc->sc_dev.dv_xname);
	}
	return error;
}

static void
kse_stop(struct ifnet *ifp, int disable)
{
	struct kse_softc *sc = ifp->if_softc;
	struct kse_txsoft *txs;
	int i;

	callout_stop(&sc->sc_callout);

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

	if (disable)
		rxdrain(sc);
	
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

static void
kse_reset(struct kse_softc *sc)
{

	CSR_WRITE_2(sc, GRR, 1);
	delay(1000); /* PDF does not mention the delay amount */
	CSR_WRITE_2(sc, GRR, 0);

	CSR_WRITE_2(sc, CIDR, 1);
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
		printf("%s: device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_dev.dv_xname, sc->sc_txfree, sc->sc_txsfree,
		    sc->sc_txnext);
		ifp->if_oerrors++;

		/* Reset the interface. */
		kse_init(ifp);
	}
	else if (ifp->if_flags & IFF_DEBUG)
		printf("%s: recovered from device timeout\n",
		    sc->sc_dev.dv_xname);

	/* Try to get more packets going. */
	kse_start(ifp);
}

static void
kse_start(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;
	struct mbuf *m0;
	struct kse_txsoft *txs;
	bus_dmamap_t dmamap;
	int error, nexttx, lasttx, ofree, seg;

	lasttx = -1;

	if ((ifp->if_flags & (IFF_RUNNING|IFF_OACTIVE)) != IFF_RUNNING)
		return;

	/*
	 * Remember the previous number of free descriptors.
	 */
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
			if (nexttx != sc->sc_txnext)
				tdes->t0 = T0_OWN;
			lasttx = nexttx;
		}
#if 0
		/*
		 * T1_IC bit could schedule Tx frame done interrupt here,
		 * but this driver takes a "shoot away" Tx strategy.
		 */
#else
    {
		/*
		 * Outgoing NFS mbuf must be unloaded when Tx completed.
		 * Without T1_IC NFS mbuf is left unack'ed for excessive
		 * time and NFS stops to proceed until kse_watchdog()
		 * calls txreap() to reclaim the unack'ed mbuf.
		 * It's painful to tranverse every mbuf chain to determine
		 * whether someone is waiting for Tx completion.
		 */
		struct mbuf *m = m0;
		do {
			if ((m->m_flags & M_EXT) && m->m_ext.ext_free) {
				sc->sc_txdescs[lasttx].t1 |= T1_IC;
				break;
			}
		} while ((m = m->m_next) != NULL);
    }
#endif

		/* write last T0_OWN bit of the 1st segment */
		sc->sc_txdescs[lasttx].t1 |= T1_LS;
		sc->sc_txdescs[sc->sc_txnext].t1 |= T1_FS;
		sc->sc_txdescs[sc->sc_txnext].t0 = T0_OWN;
		KSE_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD|BUS_DMASYNC_PREWRITE);

		/* tell DMA start transmit */
		CSR_WRITE_4(sc, MDTSC, 1);

		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;
		txs->txs_ndesc = dmamap->dm_nsegs;

		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;
		sc->sc_txsfree--;
		sc->sc_txsnext = KSE_NEXTTXS(sc->sc_txsnext);
#if NBPFILTER > 0
		/*
		 * Pass the packet to any BPF listeners.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif /* NBPFILTER > 0 */
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
kse_set_filter(struct kse_softc *sc)
{
#if 0 /* later */
	struct ether_multistep step;
	struct ether_multi *enm;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int cnt = 0;

	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo,
		    enm->enm_addrhi, ETHER_ADDR_LEN) != 0) {
			;
		}
		ETHER_NEXT_MULTI(step, enm);
		cnt++;
	}
	return;
#endif
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
		printf("%s: can't load rx DMA map %d, error = %d\n",
		    sc->sc_dev.dv_xname, idx, error);
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
		printf("%s: Rx descriptor full\n", sc->sc_dev.dv_xname);

	CSR_WRITE_4(sc, INTST, isr);
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
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].r0;
		
		if (rxstat & R0_OWN) /* desc is left empty */
			break;

		/* R0_FS|R0_LS must have been marked for this desc */

		if (rxstat & R0_ES) {
			ifp->if_ierrors++;
#define PRINTERR(bit, str)						\
			if (rxstat & (bit))				\
				printf("%s: receive error: %s\n",	\
				    sc->sc_dev.dv_xname, str)
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
		len -= ETHER_CRC_LEN;	/* trim CRC off */
		m = rxs->rxs_mbuf;

		if (add_rxbuf(sc, i) != 0) {
			ifp->if_ierrors++;
			KSE_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat,
			    rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize,
			    BUS_DMASYNC_PREREAD);
			continue;
		}

		ifp->if_ipackets++;
		m->m_pkthdr.rcvif = ifp;
		m->m_pkthdr.len = m->m_len = len;

		if (sc->sc_mcsum) {
			m->m_pkthdr.csum_flags |= sc->sc_mcsum;
			if (rxstat & R0_IPE)
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4_BAD;
			if (rxstat & (R0_TCPE | R0_UDPE))
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}
#if NBPFILTER > 0
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif /* NBPFILTER > 0 */
		(*ifp->if_input)(ifp, m);
#ifdef KSEDIAGNOSTIC
		if (kse_monitor_rxintr > 0) {
			printf("m stat %x data %p len %d\n",
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
		    BUS_DMASYNC_POSTREAD|BUS_DMASYNC_POSTWRITE);

		txstat = sc->sc_txdescs[txs->txs_lastdesc].t0;

		if (txstat & T0_OWN) /* desc is still in use */
			break;

		/* there is no way to tell transmission status per frame */

		ifp->if_opackets++;

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

#if 0 /* rambling link status */
	printf("%s: link %s\n", sc->sc_dev.dv_xname,
	    (CSR_READ_2(sc, P1SR) & (1U << 5)) ? "up" : "down");
#endif
	ifmedia_sts(&sc->sc_ethercom.ec_if, &ifmr);
}

static int
ifmedia_upd(struct ifnet *ifp)
{
	struct kse_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_media;
	uint16_t ctl;

	ctl = 0;
	if (IFM_SUBTYPE(ifm->ifm_media) == IFM_AUTO) {
		ctl |= (1U << 13); /* restart AN */
		ctl |= (1U << 7);  /* enable AN */
		ctl |= (1U << 4);  /* advertise flow control pause */
		ctl |= (1U << 3) | (1U << 2) | (1U << 1) | (1U << 0);
	}
	else {
		if (IFM_SUBTYPE(ifm->ifm_media) == IFM_100_TX)
			ctl |= (1U << 6);
		if (ifm->ifm_media & IFM_FDX)
			ctl |= (1U << 5);
	}
	CSR_WRITE_2(sc, P1CR4, ctl);

	sc->sc_media_active = IFM_NONE;
	sc->sc_media_status = IFM_AVALID;

	return 0;
}

static void
ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct kse_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_media;
	uint16_t ctl, sts, result;

	ifmr->ifm_status = IFM_AVALID;
	ifmr->ifm_active = IFM_ETHER;

	ctl = CSR_READ_2(sc, P1CR4);
	sts = CSR_READ_2(sc, P1SR);
	if ((sts & (1U << 5)) == 0) {
		ifmr->ifm_active |= IFM_NONE;
		goto out; /* link is down */
	}
	ifmr->ifm_status |= IFM_ACTIVE;
	if (IFM_SUBTYPE(ifm->ifm_media) == IFM_AUTO) {
		if ((sts & (1U << 6)) == 0) {
			ifmr->ifm_active |= IFM_NONE;
			goto out; /* negotiation in progress */
		}
		result = ctl & sts & 017;
		if (result & (1U << 3))
			ifmr->ifm_active |= IFM_100_TX|IFM_FDX;
		else if (result & (1U << 2))
			ifmr->ifm_active |= IFM_100_TX;
		else if (result & (1U << 1))
			ifmr->ifm_active |= IFM_10_T|IFM_FDX;
		else if (result & (1U << 0))
			ifmr->ifm_active |= IFM_10_T;
		else
			ifmr->ifm_active |= IFM_NONE;
		if (ctl & (1U << 4))
			ifmr->ifm_active |= IFM_FLOW | IFM_ETH_RXPAUSE;
		if (sts & (1U << 4))
			ifmr->ifm_active |= IFM_FLOW | IFM_ETH_TXPAUSE;
	}
	else {
		ifmr->ifm_active |= (sts & (1U << 10)) ? IFM_100_TX : IFM_10_T;
		if (sts & (1U << 9))
			ifmr->ifm_active |= IFM_FDX;
		if (sts & (1U << 12))
			ifmr->ifm_active |= IFM_FLOW | IFM_ETH_RXPAUSE;
		if (sts & (1U << 11))
			ifmr->ifm_active |= IFM_FLOW | IFM_ETH_TXPAUSE;
	}

  out:
	sc->sc_media_status = ifmr->ifm_status;
	sc->sc_media_active = ifmr->ifm_active;
}

static void
phy_tick(void *arg)
{
	struct kse_softc *sc = arg;
	struct ifmediareq ifmr;
	int s;

	s = splnet();
	ifmedia_sts(&sc->sc_ethercom.ec_if, &ifmr);
	splx(s);

	callout_reset(&sc->sc_callout, hz, phy_tick, sc);
}
