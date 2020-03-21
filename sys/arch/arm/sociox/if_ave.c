/*	$NetBSD: if_ave.c,v 1.8 2020/03/21 11:46:36 nisimura Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
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
 * Socionext UniPhier AVE GbE driver
 *
 * There are two groups for 64bit paddr model and 32bit paddr.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ave.c,v 1.8 2020/03/21 11:46:36 nisimura Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/intr.h>
#include <sys/device.h>
#include <sys/callout.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/rndsource.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_media.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <net/bpf.h>

#include <dev/fdt/fdtvar.h>

#define FDT_INTR_FLAGS	(0)	/* !MP_SAFE */

#define AVEID		0x000		/* hardware ID */
#define AVEHWVER	0x004		/* hardware version */
#define AVEGR		0x008		/* chip global control */
#define  GR_RXRST	(1U<<5)		/* RxFIFO reset */
#define  GR_PHYRST	(1U<<4)		/* external PHY reset */
#define  GR_GRST	(1U<<0)		/* full chip reset */
#define AVECFG		0x00c		/* hw configuration */
#define  CFG_FLE	(1U<<31)	/* filter function enable */
#define  CFG_CKE	(1U<<30)	/* checksum enable */
#define  CFG_MII	(1U<<27)	/* 1: MII/RMII, 0: RGMII */
#define  CFG_IPFCKE	(1U<<24)	/* IP framgment csum enable */
#define AVEGIMR		0x100		/* global interrupt mask */
#define AVEGISR		0x104		/* global interrupt status */
#define  GISR_PHY	(1U<<24)	/* PHY status change detected */
#define  GISR_TXCI	(1U<<16)	/* transmission completed */
#define  GISR_RXF2L	(1U<<8)		/* Rx frame length beyond limit */
#define  GISR_RXOVF	(1U<<7)		/* RxFIFO oveflow detected */
#define  GISR_RXDROP	(1U<<6)		/* PAUSE frame has been dropped */
#define  GISR_RXIT	(1U<<5)		/* receive itimer notify */
#define AVETXC		0x200		/* transmit control */
#define  TXC_FCE	(1U<<18)	/* enable Tx flow control */
#define  TXC_SPD1000	(1U<<17)	/* use 1000Mbps */
#define  TXC_SPD100	(1U<<16)	/* use 100Mbps */
#define AVERXC		0x204		/* receive control */
#define  RXC_EN		(1U<<30)	/* enable receive circuit */
#define  RXC_USEFDX	(1U<<22)	/* use full-duplex */
#define  RXC_FCE	(1U<<21)	/* enable Rx flow control */
#define  RXC_AFE	(1U<<19)	/* use address filter (!promisc) */
#define  RXC_DRPEN	(1U<<18)	/* drop receiving PAUSE frames */
/* RXC 15:0 max frame length to accept */
#define AVEMACL		0x208		/* MAC address lower */
#define AVEMACH		0x20c		/* MAC address upper */
#define AVEMDIOC	0x214		/* MDIO control */
#define  MDIOC_RD	(1U<<3)		/* read op */
#define  MDIOC_WR	(1U<<2)		/* write op */
#define AVEMDADR	0x218		/* MDIO address -- 13:8 phy id */
#define AVEMDWRD	0x21c		/* MDIO write data - 15:0 */
#define AVEMDIOS	0x220		/* MDIO status */
#define  MDIOS_BUSY	(1U<<0)		/* MDIO in progress */
#define AVEMDRDD	0x224		/* MDIO read data */
#define AVEDESCC	0x300		/* descriptor control */
#define  DESCC_RD0	(1U<<3)		/* activate Rx0 descriptor to run */
#define DESCC_RSTP	(1U<<2)		/* pause Rx descriptor */
#define  DESCC_TD	(1U<<0)		/* activate Tx descriptor to run */
					/* 31:16 status report to read */
#define AVETXDES	0x304		/* Tx descriptor control */
					/* 27:16 Tx descriptor byte count
					 * 11:0 start address offset */
#define AVERXDES0	0x308		/* Rx0 descriptor control */
					/* 30:16 Rx descriptor byte count
					 * 14:0 start address offset */
#define AVEITIRQC	0x34c		/* interval IRQ control */
#define  ITIRQC_R0E	(1U<<27)	/* enable Rx0 interval timer */
#define  INTMVAL	(20<<16)	/* INTM value */
					/* 15:0 interval timer count */

#define AVEAFB		0x0800		/* address filter base */
#define AVEAFMSKB	0x0d00		/* byte mask base */
#define  MSKBYTE0	0xfffffff3f	/* zeros in 7:6 */
#define  MSKBYTE1	0x003ffffff	/* ones in 25:0 */
#define  genmask0(x)	(MSKBYTE0 & (~0U << (x)))
#define AVEAFMSKV	0x0e00		/* bit mask base */
#define AVEAFRING	0x0f00		/* entry ring number selector */
#define AVEAFEN		0x0ffc		/* entry enable bit vector */

#define AVETDB		0x1000	/* 64bit Tx descriptor store, upto 256 */
#define AVERDB		0x1c00	/* 64bit Rx descriptor store, upto 2048 */
#define AVE32TDB	0x1000	/* 32bit Tx store base, upto 256 */
#define AVE32RDB	0x1800	/* 32bit Rx store base, upto 2048 */

#define AVERMIIC	0x8028		/* RMII control */
#define  RMIIC_RST	(1U<<16)	/* reset operation */
#define AVELINKSEL	0x8034		/* RMII speed selection */
#define  LINKSEL_SPD100	(1U<<0)		/* use 100Mbps */

/*
 * descriptor size is 12 bytes when 64bit paddr design, 8 bytes otherwise.
 */
struct tdes {
	uint32_t t0, t1, t2;
};

struct rdes {
	uint32_t r0, r1, r2;
};

struct tdes32 { uint32_t t0, t1; };
struct rdes32 { uint32_t r0, r1; };

#define T0_OWN		(1U<<31)	/* desc is ready to Tx */
#define T0_IOC		(1U<<29)	/* post interrupt on Tx completes */
#define T0_NOCSUM	(1U<<28)	/* inhibit checksum operation */
#define T0_DONEOK	(1U<<27)	/* status - Tx completed ok */
#define T0_FS		(1U<<26)	/* first segment of frame */
#define T0_LS		(1U<<25)	/* last segment of frame */
#define T0_OWC		(1U<<21)	/* status - out of win. late coll. */
#define T0_ECOL		(1U<<20)	/* status - excess collision */
#define T0_TBS_MASK	0xffff		/* T0 segment length 15:0 */
/* T1 segment address 31:0 */
/* T2 segment address 63:32 */
#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_CSUM		(1U<<21)	/* receive checksum done */
#define R0_CERR		(1U<<20)	/* csum found negative */
#define R0_FL_MASK	0x07ff		/* R0 frame length 10:0 */
/* R1 frame address 31:0 */
/* R2 frame address 63:32 */

#define AVE_NTXSEGS		16
#define AVE_TXQUEUELEN		(AVE_NTXDESC / AVE_NTXSEGS)
#define AVE_TXQUEUELEN_MASK	(AVE_TXQUEUELEN - 1)
#define AVE_TXQUEUE_GC		(AVE_TXQUEUELEN / 4)
#define AVE_NTXDESC		256			/* HW limit */
#define AVE_NTXDESC_MASK	(AVE_NTXDESC - 1)
#define AVE_NEXTTX(x)		(((x) + 1) & AVE_NTXDESC_MASK)
#define AVE_NEXTTXS(x)		(((x) + 1) & AVE_TXQUEUELEN_MASK)

#define AVE_NRXDESC		256
#define AVE_NRXDESC_MASK	(AVE_NRXDESC - 1)
#define AVE_NEXTRX(x)		(((x) + 1) & AVE_NRXDESC_MASK)

#define AVE_INIT_RXDESC(sc, x)						\
do {									\
	struct ave_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct rdes *__rxd = &(sc)->sc_rxdescs[(x)];			\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
	bus_addr_t __paddr =__rxs->rxs_dmamap->dm_segs[0].ds_addr;	\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->r2 = htole32(BUS_ADDR_HI32(__paddr));			\
	__rxd->r1 = htole32(BUS_ADDR_LO32(__paddr));			\
	__rxd->r0 = R0_OWN | R0_FL_MASK;				\
} while (/*CONSTCOND*/0)

#define AVE32_INIT_RXDESC(sc, x)					\
do {									\
	struct ave_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct rdes32 *__rxd = &(sc)->sc_rxd32[(x)];			\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
	bus_addr_t __paddr =__rxs->rxs_dmamap->dm_segs[0].ds_addr;	\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->r1 = htole32(__paddr);					\
	__rxd->r0 = R0_OWN | R0_FL_MASK;				\
} while (/*CONSTCOND*/0)

struct ave_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndesc;			/* # of descriptors used */
};

struct ave_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

struct ave_softc {
	device_t sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_size_t sc_mapsize;		/* csr map size */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* Ethernet common data */
	struct mii_data sc_mii;		/* MII */
	callout_t sc_tick_ch;		/* PHY monitor callout */
	int sc_flowflags;		/* 802.3x PAUSE flow control */
	void *sc_ih;			/* interrupt cookie */
	int sc_phy_id;			/* PHY address */
	uint32_t sc_phymode;		/* 1<<27: MII/RMII, 0: RGMII */
	uint32_t sc_rxc;		/* software copy of AVERXC */
	int sc_model;			/* 64 paddr model or otherwise 32 */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct tdes *sc_txdescs;	/* PTR to tdes [NTXDESC] store */
	struct rdes *sc_rxdescs;	/* PTR to rdes [NRXDESC] store */
	struct tdes32 *sc_txd32;
	struct rdes32 *sc_rxd32;

	struct ave_txsoft sc_txsoft[AVE_TXQUEUELEN];
	struct ave_rxsoft sc_rxsoft[AVE_NRXDESC];
	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next ready Tx descriptor */
	int sc_txsfree;			/* number of free Tx jobs */
	int sc_txsnext;			/* next ready Tx job */
	int sc_txsdirty;		/* dirty Tx jobs */
	int sc_rxptr;			/* next ready Rx descriptor/descsoft */
	uint32_t sc_t0csum;		/* t0 field checksum designation */

	krndsource_t rnd_source;	/* random source */
};

static int ave_fdt_match(device_t, cfdata_t, void *);
static void ave_fdt_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(ave_fdt, sizeof(struct ave_softc),
    ave_fdt_match, ave_fdt_attach, NULL, NULL);

static void ave_reset(struct ave_softc *);
static int ave_init(struct ifnet *);
static void ave_start(struct ifnet *);
static void ave_stop(struct ifnet *, int);
static void ave_watchdog(struct ifnet *);
static int ave_ioctl(struct ifnet *, u_long, void *);
static void ave_set_rcvfilt(struct ave_softc *);
static void ave_write_filt(struct ave_softc *, int, const uint8_t *);
static int ave_ifmedia_upd(struct ifnet *);
static void ave_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static void mii_statchg(struct ifnet *);
static void lnkchg(struct ave_softc *);
static void phy_tick(void *);
static int mii_readreg(device_t, int, int, uint16_t *);
static int mii_writereg(device_t, int, int, uint16_t);
static int ave_intr(void *);
static void txreap(struct ave_softc *);
static void rxintr(struct ave_softc *);
static int add_rxbuf(struct ave_softc *, int);

#define CSR_READ(sc, off) \
	    bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (off))
#define CSR_WRITE(sc, off, val) \
	    bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (off), (val))

static const struct of_compat_data compat_data[] = {
	{ "socionext,unifier-ld20-ave4", 64 },
	{ "socionext,unifier-pro4-ave4", 32 },
	{ "socionext,unifier-pxs2-ave4", 32 },
	{ "socionext,unifier-ld11-ave4", 32 },
	{ "socionext,unifier-pxs3-ave4", 32 },
	{ NULL }
};

static int
ave_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
ave_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct ave_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct ifnet * const ifp = &sc->sc_ethercom.ec_if;
	struct mii_data * const mii = &sc->sc_mii;
	struct ifmedia * const ifm = &mii->mii_media;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_addr_t addr;
	bus_size_t size;
	char intrstr[128];
	const char *phy_mode;
	uint32_t hwimp, hwver, csr;
	uint8_t enaddr[ETHER_ADDR_LEN];
	int i, error = 0;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0
	    || bus_space_map(faa->faa_bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": unable to map device\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET, 0,
	    ave_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		goto fail;
	}

	sc->sc_dev = self;
	sc->sc_st = bst;
	sc->sc_sh = bsh;
	sc->sc_mapsize = size;
	sc->sc_dmat = faa->faa_dmat;

	hwimp = CSR_READ(sc, AVEID);
	hwver = CSR_READ(sc, AVEHWVER);
	sc->sc_model = of_search_compatible(phandle, compat_data)->data;

	phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL) {
		aprint_error(": missing 'phy-mode' property\n");
		phy_mode = "rgmii";
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");
	aprint_normal_dev(self, "UniPhier %c%c%c%c AVE %d GbE (%d.%d) %s\n",
	    hwimp >> 24, hwimp >> 16, hwimp >> 8, hwimp,
	    sc->sc_model, hwver >> 8, hwver & 0xff, phy_mode);
	aprint_normal_dev(self, "interrupt on %s\n", intrstr);

	if (strcmp(phy_mode, "rgmii") == 0)
		sc->sc_phymode = 0;		/* RGMII */
	else
		sc->sc_phymode = CFG_MII;	/* MII|RMII */

	CSR_WRITE(sc, AVEGR, GR_GRST | GR_PHYRST);
	DELAY(20);
	CSR_WRITE(sc, AVEGR, GR_GRST);
	DELAY(40);
	CSR_WRITE(sc, AVEGR, 0);
	DELAY(40);
	CSR_WRITE(sc, AVEGIMR, 0);

	/* Read the Ethernet MAC address from the EEPROM. */
	csr = CSR_READ(sc, AVEMACL);
	enaddr[0] = csr;
	enaddr[1] = csr >> 8;
	enaddr[2] = csr >> 16;
	enaddr[3] = csr >> 24;
	csr = CSR_READ(sc, AVEMACH);
	enaddr[4] = csr;
	enaddr[5] = csr >> 8;
	aprint_normal_dev(self,
	    "Ethernet address %s\n", ether_sprintf(enaddr));

	mii->mii_ifp = ifp;
	mii->mii_readreg = mii_readreg;
	mii->mii_writereg = mii_writereg;
	mii->mii_statchg = mii_statchg;
	sc->sc_phy_id = MII_PHY_ANY;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(ifm, 0, ave_ifmedia_upd, ave_ifmedia_sts);
	mii_attach(sc->sc_dev, mii, 0xffffffff, sc->sc_phy_id,
	    MII_OFFSET_ANY, MIIF_DOPAUSE);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(ifm, IFM_ETHER | IFM_NONE, 0, NULL);
		ifmedia_set(ifm, IFM_ETHER | IFM_NONE);
	} else
		ifmedia_set(ifm, IFM_ETHER | IFM_AUTO);
	ifm->ifm_media = ifm->ifm_cur->ifm_media; /* as if user has requested */

	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = ave_ioctl;
	ifp->if_start = ave_start;
	ifp->if_watchdog = ave_watchdog;
	ifp->if_init = ave_init;
	ifp->if_stop = ave_stop;
	IFQ_SET_READY(&ifp->if_snd);

	sc->sc_flowflags = 0;
	sc->sc_rxc = 0;

	sc->sc_ethercom.ec_capabilities = ETHERCAP_VLAN_MTU;
	ifp->if_capabilities = IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx;

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr);

	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, phy_tick, sc);

	/*
	 * HW has a dedicated store to hold Tx/Rx descriptor arrays.
	 * so no need to build Tx/Rx descriptor control_data.
	 * go straight to make dmamap to hold Tx segments and Rx frames.
	 */
	for (i = 0; i < AVE_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    AVE_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}
	for (i = 0; i < AVE_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error_dev(self,
			    "unable to create rx DMA map %d, error = %d\n",
			    i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}

	if (pmf_device_register(sc->sc_dev, NULL, NULL))
		pmf_class_network_register(self, ifp);
	else
		aprint_error_dev(self,
			"couldn't establish power handler\n");

	rnd_attach_source(&sc->rnd_source, device_xname(self),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	return;

  fail_5:
	for (i = 0; i < AVE_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
  fail_4:
	for (i = 0; i < AVE_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
  /* no fail_3|2|1 */
  fail:
	fdtbus_intr_disestablish(phandle, sc->sc_ih);
	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_mapsize);
	return;
}

static void
ave_reset(struct ave_softc *sc)
{
	uint32_t csr;

	CSR_WRITE(sc, AVERXC, 0);	/* stop Rx first */
	CSR_WRITE(sc, AVEDESCC, 0);	/* stop Tx/Rx descriptor engine */
	if (sc->sc_phymode & CFG_MII) {
		csr = CSR_READ(sc, AVERMIIC);
		CSR_WRITE(sc, AVERMIIC, csr &~ RMIIC_RST);
		DELAY(10);
		CSR_WRITE(sc , AVERMIIC, csr);
	}
	CSR_WRITE(sc, AVEGR, GR_RXRST); /* assert RxFIFO reset operation */
	DELAY(50);
	CSR_WRITE(sc, AVEGR, 0);
	CSR_WRITE(sc, AVEGISR, GISR_RXOVF); /* clear OVF condition */
}

static int
ave_init(struct ifnet *ifp)
{
	struct ave_softc *sc = ifp->if_softc;
	extern const uint8_t etherbroadcastaddr[];
	const uint8_t promisc[ETHER_ADDR_LEN] = { 0, 0, 0, 0, 0, 0 };
	uint32_t csr;
	int i;

	CSR_WRITE(sc, AVEGIMR, 0);

	/* cancel pending I/O */
	ave_stop(ifp, 0);

	/* make sure Rx circuit sane & stable state */
	ave_reset(sc);

	CSR_WRITE(sc, AVECFG, CFG_FLE | sc->sc_phymode);

	/* set Tx/Rx descriptor ring base addr offset and total size */
	CSR_WRITE(sc, AVETXDES,	 0U|(sizeof(struct tdes)*AVE_NTXDESC) << 16);
	CSR_WRITE(sc, AVERXDES0, 0U|(sizeof(struct rdes)*AVE_NRXDESC) << 16);

	/* set ptr to Tx/Rx descriptor store */
	sc->sc_txdescs = (void *)((uintptr_t)sc->sc_sh + AVETDB);
	sc->sc_rxdescs = (void *)((uintptr_t)sc->sc_sh + AVERDB);
	sc->sc_txd32 =   (void *)((uintptr_t)sc->sc_sh + AVE32TDB);
	sc->sc_rxd32 =   (void *)((uintptr_t)sc->sc_sh + AVE32RDB);

	/* build sane and loaded Tx/Rx descriptors */
	memset(sc->sc_txdescs, 0, sizeof(struct tdes)*AVE_NTXDESC);
	for (i = 0; i < AVE_NRXDESC; i++)
		(void)add_rxbuf(sc, i);

	/*
	 * address filter usage
	 * 0 - promisc.
	 * 1 - my own MAC station address
	 * 2 - broadcast address
	 */
	CSR_WRITE(sc, AVEAFEN, 0); /* clear all 17 entries first */
	ave_write_filt(sc, 0, promisc);
	ave_write_filt(sc, 1, CLLADDR(ifp->if_sadl));
	ave_write_filt(sc, 2, etherbroadcastaddr);

	/* accept multicast frame or run promisc mode */
	ave_set_rcvfilt(sc);

	csr = CSR_READ(sc, AVECFG);
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Tx) {
		sc->sc_t0csum = 0;
		csr |= (CFG_CKE | CFG_IPFCKE);
	} else
		sc->sc_t0csum = T0_NOCSUM;
	if (ifp->if_capenable & IFCAP_CSUM_IPv4_Rx)
		csr |= (CFG_CKE | CFG_IPFCKE);
	CSR_WRITE(sc, AVECFG, csr);

	sc->sc_rxc = 1518 | RXC_AFE | RXC_DRPEN;
	CSR_WRITE(sc, AVERXC, sc->sc_rxc | RXC_EN);

	/* activate Tx/Rx descriptor engine */
	CSR_WRITE(sc, AVEDESCC, DESCC_TD | DESCC_RD0);

	/* enable Rx ring0 timer */
	csr = CSR_READ(sc, AVEITIRQC) & 0xffff;
	CSR_WRITE(sc, AVEITIRQC, csr | ITIRQC_R0E | INTMVAL);

	CSR_WRITE(sc, AVEGIMR, /* PHY interrupt is not maskable */
	    GISR_TXCI | GISR_RXIT | GISR_RXDROP | GISR_RXOVF | GISR_RXF2L);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* start one second timer */
	callout_schedule(&sc->sc_tick_ch, hz);

	return 0;
}

static void
ave_stop(struct ifnet *ifp, int disable)
{
	struct ave_softc *sc = ifp->if_softc;

	/* Stop the one second clock. */
	callout_stop(&sc->sc_tick_ch);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	/* Mark the interface down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

static int
ave_ifmedia_upd(struct ifnet *ifp)
{
	struct ave_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_mii.mii_media;
	uint32_t txcr, rxcr;

	txcr = CSR_READ(sc, AVETXC);
	rxcr = CSR_READ(sc, AVERXC);
	CSR_WRITE(sc, AVERXC, rxcr &~ RXC_EN); /* stop Rx first */

	if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_AUTO) {
		; /* restart AN */
		; /* enable AN */
		; /* advertise flow control pause */
		; /* adv. 1000FDX,100FDX,100HDX,10FDX,10HDX */
	} else {
		txcr &= ~(TXC_SPD1000 | TXC_SPD100);
		rxcr &= ~RXC_USEFDX;
		if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_1000_T)
			txcr |= TXC_SPD1000;
		else if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_100_TX)
			txcr |= TXC_SPD100;
		if (ifm->ifm_media & IFM_FDX)
			rxcr |= RXC_USEFDX;
	}
	sc->sc_rxc = rxcr;
	CSR_WRITE(sc, AVETXC, txcr);
	CSR_WRITE(sc, AVERXC, rxcr | RXC_EN);
	return 0;
}

static void
ave_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct ave_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;

	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = sc->sc_flowflags |
	    (mii->mii_media_active & ~IFM_ETH_FMASK);
}

void
mii_statchg(struct ifnet *ifp)
{
	struct ave_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;
	struct ifmedia * const ifm = &mii->mii_media;
	uint32_t txcr, rxcr, csr;

	/* get flow control negotiation result */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->sc_flowflags)
		sc->sc_flowflags = mii->mii_media_active & IFM_ETH_FMASK;

	txcr = CSR_READ(sc, AVETXC);
	rxcr = CSR_READ(sc, AVERXC);
	CSR_WRITE(sc, AVERXC, rxcr &~ RXC_EN); /* stop Rx first */

	/* adjust 802.3x PAUSE flow control */
	if ((mii->mii_media_active & IFM_FDX)
	    && (sc->sc_flowflags & IFM_ETH_TXPAUSE))
		txcr |= TXC_FCE;
	else
		txcr &= ~TXC_FCE;
	if ((mii->mii_media_active & IFM_FDX)
	    && (sc->sc_flowflags & IFM_ETH_RXPAUSE))
		rxcr |= RXC_FCE;
	else
		rxcr &= ~RXC_FCE;

	/* HW does not handle automatic speed adjustment */
	txcr &= ~(TXC_SPD1000 | TXC_SPD100);
	if ((sc->sc_phymode & CFG_MII) == 0 /* RGMII model */
	     && IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_1000_T)
		txcr |= TXC_SPD1000;
	else if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_100_TX)
		txcr |= TXC_SPD100;

	/* adjust LINKSEL when MII/RMII too */
	if (sc->sc_phymode & CFG_MII) {
		csr = CSR_READ(sc, AVELINKSEL) &~ LINKSEL_SPD100;;
		if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_100_TX)
			csr |= LINKSEL_SPD100;
		CSR_WRITE(sc, AVELINKSEL, csr);
	}

	sc->sc_rxc = rxcr;
	CSR_WRITE(sc, AVETXC, txcr);
	CSR_WRITE(sc, AVERXC, rxcr | RXC_EN);

printf("%ctxfe, %crxfe\n",
	(txcr & TXC_FCE) ? '+' : '-', (rxcr & RXC_FCE) ? '+' : '-');
}

static void
lnkchg(struct ave_softc *sc)
{
	struct ifmediareq ifmr;

	ave_ifmedia_sts(&sc->sc_ethercom.ec_if, &ifmr);
}

static void
phy_tick(void *arg)
{
	struct ave_softc *sc = arg;
	struct mii_data *mii = &sc->sc_mii;
	int s;

	s = splnet();
	mii_tick(mii);
	splx(s);

	callout_schedule(&sc->sc_tick_ch, hz);
}

static int
mii_readreg(device_t self, int phy, int reg, uint16_t *val)
{
	struct ave_softc *sc = device_private(self);
	uint32_t ctrl, stat;

	CSR_WRITE(sc, AVEMDADR, reg | (sc->sc_phy_id << 8));
	ctrl = CSR_READ(sc, AVEMDIOC) & ~MDIOC_WR;
	CSR_WRITE(sc, AVEMDIOC, ctrl | MDIOC_RD);
	stat = CSR_READ(sc, AVEMDIOS);
	while (stat & MDIOS_BUSY) {
		DELAY(10);
		stat = CSR_READ(sc, AVEMDIOS);
	}
	*val = CSR_READ(sc, AVEMDRDD);
	return 0;
}

static int
mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct ave_softc *sc = device_private(self);
	uint32_t ctrl, stat;

	CSR_WRITE(sc, AVEMDADR, reg | (sc->sc_phy_id << 8));
	CSR_WRITE(sc, AVEMDWRD, val);
	ctrl = CSR_READ(sc, AVEMDIOC) & ~MDIOC_RD;
	CSR_WRITE(sc, AVEMDIOC, ctrl | MDIOC_WR);
	stat = CSR_READ(sc, AVEMDIOS);
	while (stat & MDIOS_BUSY) {
		DELAY(10);
		stat = CSR_READ(sc, AVEMDIOS);
	}
	return 0;
}

static int
ave_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ave_softc *sc = ifp->if_softc;
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
		ifm = &sc->sc_mii.mii_media;
		error = ifmedia_ioctl(ifp, ifr, ifm, cmd);
		break;
	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;

		error = 0;

		if (cmd == SIOCSIFCAP)
			error = (*ifp->if_init)(ifp);
		if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			;
		else if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			ave_set_rcvfilt(sc);
		}
		break;
	}

	splx(s);
	return error;
}

static void
ave_write_filt(struct ave_softc *sc, int i, const uint8_t *en)
{
	uint32_t macl, mach, n, mskbyte0;

	/* pick v4mcast or v6mcast length */
	n = (en[0] == 0x01) ? 3 : (en[0] == 0x33) ? 2 : ETHER_ADDR_LEN;
	/* entry 0 is reserved for promisc mode */
	mskbyte0 = (i > 0) ? genmask0(n) : MSKBYTE0;

	/* set frame address first */
	macl = mach = 0;
	macl |= (en[3]<<24) | (en[2]<<16)| (en[1]<<8) | en[0];
	mach |= (en[5]<<8)  | en[4];
	CSR_WRITE(sc, AVEAFB + (i * 0x40) + 0, macl);
	CSR_WRITE(sc, AVEAFB + (i * 0x40) + 4, mach);
	/* set byte mask according to mask length, any of 6, 3, or 2 */
	CSR_WRITE(sc, AVEAFMSKB + (i * 8) + 0, mskbyte0);
	CSR_WRITE(sc, AVEAFMSKB + (i * 8) + 4, MSKBYTE1);
	/* set bit vector mask */
	CSR_WRITE(sc, AVEAFMSKV + (i * 4), 0xffff);
	/* use Rx ring 0 anyway */
	CSR_WRITE(sc, AVEAFRING + (i * 4), 0);
	/* filter entry enable bit vector */
	CSR_WRITE(sc, AVEAFEN, CSR_READ(sc, AVEAFEN) | 1U << i);
}

static void
ave_set_rcvfilt(struct ave_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multistep step;
	struct ether_multi *enm;
	extern const uint8_t ether_ipmulticast_min[];
	extern const uint8_t ether_ip6multicast_min[];
	uint32_t csr;
	int i;

	sc->sc_rxc &= (RXC_AFE | RXC_EN);
	CSR_WRITE(sc, AVERXC, sc->sc_rxc); /* stop Rx first */

	/* turn off all 7 mcast filter entries */
	csr = CSR_READ(sc, AVEAFEN);
	CSR_WRITE(sc, AVEAFEN, csr & ~(0177U << 11));

	ETHER_LOCK(ec);
	if ((ifp->if_flags & IFF_PROMISC) || ec->ec_multicnt > 7) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;
	ETHER_FIRST_MULTI(step, ec, enm);
	i = 11; /* slot 11:17 to catch multicast frames */
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
			goto update;
		}
		KASSERT(i < 17);
printf("[%d] %s\n", i, ether_sprintf(enm->enm_addrlo));
		/* use additional MAC addr to accept up to 7 */
		ave_write_filt(sc, i, enm->enm_addrlo);
		ETHER_NEXT_MULTI(step, enm);
		i++;
	}
	ETHER_UNLOCK(ec);
	sc->sc_rxc |= RXC_AFE;

 update:
	if (ifp->if_flags & IFF_PROMISC)
		/* RXC_AFE has been cleared, nothing to do */;
	else if (ec->ec_flags & ETHER_F_ALLMULTI) {
		/* slot 11/12 for IPv4/v6 multicast */
		ave_write_filt(sc, 11, ether_ipmulticast_min);
		ave_write_filt(sc, 12, ether_ip6multicast_min); /* INET6 */
		/* clear slot 13-17 */
		csr = CSR_READ(sc, AVEAFEN);
		CSR_WRITE(sc, AVEAFEN, csr & ~(037U << 13));
		sc->sc_rxc |= RXC_AFE;
	}
	CSR_WRITE(sc, AVERXC, sc->sc_rxc | RXC_EN);
}

static void
ave_watchdog(struct ifnet *ifp)
{
	struct ave_softc *sc = ifp->if_softc;

	/*
	 * Since we're not interrupting every packet, sweep
	 * up before we report an error.
	 */
	txreap(sc);

	if (sc->sc_txfree != AVE_NTXDESC) {
		aprint_error_dev(sc->sc_dev,
		    "device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_txfree, sc->sc_txsfree, sc->sc_txnext);
		if_statinc(ifp, if_oerrors);

		/* Reset the interface. */
		ave_init(ifp);
	}

	ave_start(ifp);
}

static void
ave_start(struct ifnet *ifp)
{
	struct ave_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct ave_txsoft *txs;
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

		if (sc->sc_txsfree < AVE_TXQUEUE_GC) {
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
		     seg++, nexttx = AVE_NEXTTX(nexttx)) {
			struct tdes *tdes = &sc->sc_txdescs[nexttx];
			bus_addr_t paddr = dmamap->dm_segs[seg].ds_addr;
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.	 That could cause a race condition.
			 * We'll do it below.
			 */
			tdes->t2 = htole32(BUS_ADDR_HI32(paddr));
			tdes->t1 = htole32(BUS_ADDR_LO32(paddr));
			tdes->t0 = tdes0 | sc->sc_t0csum
			     | (dmamap->dm_segs[seg].ds_len & T0_TBS_MASK);
			tdes0 = T0_OWN; /* 2nd and other segments */
			lasttx = nexttx;
		}
		/*
		 * Outgoing NFS mbuf must be unloaded when Tx completed.
		 * Without T0_IOC NFS mbuf is left unack'ed for excessive
		 * time and NFS stops to proceed until ave_watchdog()
		 * calls txreap() to reclaim the unack'ed mbuf.
		 * It's painful to traverse every mbuf chain to determine
		 * whether someone is waiting for Tx completion.
		 */
		m = m0;
		do {
			if ((m->m_flags & M_EXT) && m->m_ext.ext_free) {
				sc->sc_txdescs[lasttx].t0 |= T0_IOC;
				break;
			}
		} while ((m = m->m_next) != NULL);

		/* Write deferred 1st segment T0_OWN at the final stage */
		sc->sc_txdescs[lasttx].t0 |= T0_LS;
		sc->sc_txdescs[sc->sc_txnext].t0 |= (T0_FS | T0_OWN);
		/* AVE_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE); */

		/* Tell DMA start transmit */
		/* CSR_WRITE(sc, AVEDESCC, DESCC_TD | DESCC_RD0); */

		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;
		txs->txs_ndesc = dmamap->dm_nsegs;

		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;
		sc->sc_txsfree--;
		sc->sc_txsnext = AVE_NEXTTXS(sc->sc_txsnext);
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

static int
ave_intr(void *arg)
{
	struct ave_softc *sc = arg;
	uint32_t gimr, stat;

	gimr = CSR_READ(sc, AVEGIMR);
	CSR_WRITE(sc, AVEGIMR, 0);
	stat = CSR_READ(sc, AVEGISR);
	if (stat == 0)
		goto done;
	if (stat & GISR_PHY) {
		lnkchg(sc);
		CSR_WRITE(sc, AVEGISR, GISR_PHY);
	}
	stat &= CSR_READ(sc, AVEGIMR);
	if (stat == 0)
		goto done;
	if (stat & GISR_RXDROP)
		CSR_WRITE(sc, AVEGISR, GISR_RXDROP);
	if (stat & GISR_RXOVF)
		CSR_WRITE(sc, AVEGISR, GISR_RXOVF);
	if (stat & GISR_RXF2L)
		CSR_WRITE(sc, AVEGISR, GISR_RXF2L);
	if (stat & GISR_RXIT) {
		rxintr(sc);
		CSR_WRITE(sc, AVEGISR, GISR_RXIT);
	}
	if (stat & GISR_TXCI) {
		txreap(sc);
		CSR_WRITE(sc, AVEGISR, GISR_TXCI);
	}
 done:
	CSR_WRITE(sc, AVEGIMR, gimr);
	return (stat != 0);
}

static void
txreap(struct ave_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ave_txsoft *txs;
	uint32_t txstat;
	int i;

	ifp->if_flags &= ~IFF_OACTIVE;

	for (i = sc->sc_txsdirty; sc->sc_txsfree != AVE_TXQUEUELEN;
	     i = AVE_NEXTTXS(i), sc->sc_txsfree++) {
		txs = &sc->sc_txsoft[i];

		/* AVE_CDTXSYNC(sc, txs->txs_firstdesc, txs->txs_ndesc,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE); */

		txstat = sc->sc_txdescs[txs->txs_lastdesc].t0;

		if (txstat & T0_OWN) /* desc is still in use */
			break;
		/*
		 * XXX able to count statistics XXX
		 * T0_DONEOK -- completed ok
		 * T0_OWC    -- out of window or collision
		 * T0_ECOL   -- dropped by excess collision
		 */
		if_statinc(ifp, if_opackets);

		sc->sc_txfree += txs->txs_ndesc;
		bus_dmamap_sync(sc->sc_dmat, txs->txs_dmamap,
		    0, txs->txs_dmamap->dm_mapsize, BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(sc->sc_dmat, txs->txs_dmamap);
		m_freem(txs->txs_mbuf);
		txs->txs_mbuf = NULL;
	}
	sc->sc_txsdirty = i;
	if (sc->sc_txsfree == AVE_TXQUEUELEN)
		ifp->if_timer = 0;
}

static void
rxintr(struct ave_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct ave_rxsoft *rxs;
	struct mbuf *m;
	uint32_t rxstat;
	int i, len;

	for (i = sc->sc_rxptr; /*CONSTCOND*/ 1; i = AVE_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		/* AVE_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE); */

		rxstat = sc->sc_rxdescs[i].r0;
		if (rxstat & R0_OWN) /* desc is left empty */
			break;

		/* R0_FS | R0_LS must have been marked for this desc */

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		len = rxstat & R0_FL_MASK;
		len -= ETHER_CRC_LEN;	/* Trim CRC off */
		m = rxs->rxs_mbuf;

		if (add_rxbuf(sc, i) != 0) {
			if_statinc(ifp, if_ierrors);
			AVE_INIT_RXDESC(sc, i);
			bus_dmamap_sync(sc->sc_dmat,
			    rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize,
			    BUS_DMASYNC_PREREAD);
			continue;
		}

		m_set_rcvif(m, ifp);
		m->m_pkthdr.len = m->m_len = len;

		if (rxstat & R0_CSUM) {
			uint32_t csum = M_CSUM_IPv4;
			if (rxstat & R0_CERR)
				csum |= M_CSUM_IPv4_BAD;
			m->m_pkthdr.csum_flags |= csum;
		}
		if_percpuq_enqueue(ifp->if_percpuq, m);
	}
	sc->sc_rxptr = i;
}

static int
add_rxbuf(struct ave_softc *sc, int i)
{
	struct ave_rxsoft *rxs = &sc->sc_rxsoft[i];
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
		    "can't load rx DMA map %d, error = %d\n", i, error);
		panic("add_rxbuf");
	}

	bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
	    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
	AVE_INIT_RXDESC(sc, i);

	return 0;
}
