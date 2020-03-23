/*	$NetBSD: if_scx.c,v 1.5 2020/03/23 05:49:57 nisimura Exp $	*/

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

#define NOT_MP_SAFE	0

/*
 * Socionext SC2A11 SynQuacer NetSec GbE driver
 *
 *   (possibly incorrect notes to be removed eventually)
 * - 32 byte descriptor for 64 bit paddr design.
 * - multiple rings seems available. There are special descriptor fields
 *   to designify ring number from which to arrive or to which go.
 * - memory mapped EEPROM to hold MAC address. The rest of the area is
 *   occupied by a set of ucode for two DMA engines and one packet engine.
 * - The size of frame address filter is unknown. Might be 32
 * - The first slot is my own station address. Always enabled to perform
 *   to identify oneself.
 * - 1~31 are for supplimental MAC addresses. Independently enabled
 *   for use. Good to catch multicast. Byte-wise selective match available.
 *   Use to catch { 0x01, 0x00, 0x00 } and/or { 0x33, 0x33 }.
 * - The size of multicast hash filter store is unknown. Might be 256 bit.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_scx.c,v 1.5 2020/03/23 05:49:57 nisimura Exp $");

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
#include <dev/acpi/acpireg.h>
#include <dev/acpi/acpivar.h>
#include <dev/acpi/acpi_intr.h>

#define SWRESET		0x104
#define COMINIT		0x120
#define INTRST		0x200
#define  IRQ_RX		(1U<<1)
#define  IRQ_TX		(1U<<0)
#define INTREN		0x204
#define INTR_SET	0x234
#define INTR_CLR	0x238
#define TXINTST		0x400
#define TXINTEN		0x404
#define TXINT_SET	0x428
#define TXINT_CLR	0x42c
#define  TXI_NTOWNR	(1U<<17)
#define  TXI_TR_ERR	(1U<<16)
#define  TXI_TXDONE	(1U<<15)
#define  TXI_TMREXP	(1U<<14)
#define RXINTST		0x440
#define RXINTEN		0x444
#define RXINT_SET	0x468
#define RXINT_CLR	0x46c
#define  RXI_RC_ERR	(1U<<16)
#define  RXI_PKTCNT	(1U<<15)
#define  RXI_TMREXP	(1U<<14)
#define TXTIMER		0x41c
#define RXTIMER		0x45c
#define TXCOUNT		0x410
#define RXCOUNT		0x454
#define H2MENG		0x210		/* DMAC host2media ucode port */
#define M2HENG		0x21c		/* DMAC media2host ucode port */
#define PKTENG		0x0d0		/* packet engine ucode port */
#define HWVER0		0x22c
#define HWVER1		0x230

#define MACSTAT		0x1024		/* gmac status */
#define MACDATA		0x11c0		/* gmac rd/wr data */
#define MACCMD		0x11c4		/* gmac operation */
#define  CMD_IOWR	(1U<<28)	/* write op */
#define  CMD_BUSY	(1U<<31)	/* busy bit */
#define DESCENG_INIT	0x11fc
#define DESCENG_SRST	0x1204

#define GMACMCR		0x0000		/* MAC configuration */
#define  MCR_IBN	(1U<<30)	/* */
#define  MCR_CST	(1U<<25)	/* strip CRC */
#define  MCR_TC		(1U<<24)	/* keep RGMII PHY notified */
#define  MCR_JE		(1U<<20)	/* ignore oversized >9018 condition */
#define  MCR_USEMII	(1U<<15)	/* 1: RMII/MII, 0: RGMII */
#define  MCR_SPD100	(1U<<14)	/* force speed 100 */
#define  MCR_USEFDX	(1U<<11)	/* force full duplex */
#define  MCR_IPCKEN	(1U<<10)	/* handle checksum */
#define  MCR_ACS	(1U<<7)		/* auto pad strip CRC */
#define  MCR_TXE	(1U<<3)		/* start Tx DMA engine */
#define  MCR_RXE	(1U<<2)		/* start Rx DMA engine */
#define  _MCR_FDX	0x0000280c	/* XXX TBD */
#define  _MCR_HDX	0x0001a00c	/* XXX TBD */
#define GMACAFR		0x0004		/* frame DA/SA address filter */
#define  AFR_RA		(1U<<31)	/* receive block all on */
#define  AFR_HPF	(1U<<10)	/* activate hash or perfect filter */
#define  AFR_SAF	(1U<<9)		/* source address filter */
#define  AFR_SAIF	(1U<<8)		/* SA inverse filtering */
#define  AFR_PCF	(3U<<6)		/* */
#define  AFR_RB		(1U<<5)		/* reject broadcast frame */
#define  AFR_AM		(1U<<4)		/* accept all multicast frame */
#define  AFR_DAIF	(1U<<3)		/* DA inverse filtering */
#define  AFR_MHTE	(1U<<2)		/* use multicast hash table */
#define  AFR_UHTE	(1U<<1)		/* use additional MAC addresses */
#define  AFR_PM		(1U<<0)		/* run promisc mode */
#define  _AFR		0x80000001	/* XXX TBD */
#define GMACMHTH	0x0008		/* XXX multicast hash table 63:32 */
#define GMACMHTL	0x000c		/* XXX multicast hash table 31:0 */
#define GMACGAR		0x0010		/* MDIO operation */
#define  GAR_PHY	(11)		/* mii phy 15:11 */
#define  GAR_REG	(6)		/* mii reg 10:6 */
#define  GAR_CTL	(2)		/* control 5:2 */
#define  GAR_IOWR	(1U<<1)		/* MDIO write op */
#define  GAR_BUSY	(1U)		/* busy bit */
#define GMACGDR		0x0014		/* MDIO rd/wr data */
#define GMACFCR		0x0018		/* 802.3x flowcontrol */
#define  FCR_RFE	(1U<<2)		/* accept PAUSE to throttle Tx */
#define  FCR_TFE	(1U<<1)		/* generate PAUSE to moderate Rx lvl */
#define GMACIMPL	0x0020		/* implementation number XXXX.YYYY */
#define GMACVTAG	0x001c		/* VLAN tag control */
#define GMACMAH0	0x0040		/* MAC address 0 47:32 */
#define GMACMAL0	0x0044		/* MAC address 0 31:0 */
#define GMACMAH(i) 	((i)*8+0x40)	/* supplimental MAC addr 1 - 15 */
#define GMACMAL(i) 	((i)*8+0x44)
#define GMACMHT0	0x0500		/* multicast hash table 0 - 8*/

#define GMACBMR		0x1000		/* DMA bus mode control
					 * 24    4PBL
					 * 22:17 RPBL
					 * 16    fix burst
					 * 15:14 priority between Rx and Tx
					 *  3    rxtx41
					 *  2    rxtx31
					 *  1    rxtx21
					 *  0    rxtx11
					 * 13:8  PBL possible DMA burst len
					 * 0     reset op. self clear
					 */
#define  _BMR		0x00412080	/* XXX TBD */
#define  _BMR0		0x00020181	/* XXX TBD */
#define  BMR_RST	(1U<<0)		/* reset op. self clear when done */
#define GMACRDLAR	0x100c		/* */
#define  _RDLAR		0x18000		/* XXX TBD */
#define GMACTDLAR	0x1010		/* */
#define  _TDLAR		0x1c000		/* XXX TBD */
#define GMACOMR		0x1018		/* DMA operation */
#define  OMR_TXE	(1U<<13)	/* start Tx DMA engine, 0 to stop */
#define  OMR_RXE	(1U<<1)		/* start Rx DMA engine, 0 to stop */

static int get_garclk(uint32_t);

/* descriptor format definition */
struct tdes {
	uint32_t t0, t1, t2, t3;
};

struct rdes {
	uint32_t r0, r1, r2, r3;
};

#define T0_OWN		(1U<<31)	/* desc is ready to Tx */
#define T0_EOD		(1U<<30)	/* end of descriptor array */
#define T0_DRID		(24)		/* 29:24 DRID */
#define T0_PT		(1U<<21)	/* 23:21 PT */
#define T0_TRID		(16)		/* 20:16 TRID */
#define T0_FS		(1U<<9)		/* first segment of frame */
#define T0_LS		(1U<<8)		/* last segment of frame */
#define T0_CSUM		(1U<<7)		/* enable check sum offload */
#define T0_SGOL		(1U<<6)		/* enable TCP segment offload */
#define T0_TRS		(1U<<4)		/* 5:4 TRS */
#define T0_IOC		(0)		/* XXX TBD interrupt when completed */
/* T1 segment address 63:32 */
/* T2 segment address 31:0 */
/* T3 31:16 TCP segment length, 15:0 segment length to transmit */
#define R0_OWN		(1U<<31)	/* desc is empty */
#define R0_EOD		(1U<<30)	/* end of descriptor array */
#define R0_SRID		(24)		/* 29:24 SRID */
#define R0_FR		(1U<<23)	/* FR */
#define R0_ER		(1U<<21)	/* Rx error indication */
#define R0_ERR		(3U<<16)	/* 18:16 receive error code */
#define R0_TDRID	(14)		/* 15:14 TDRID */
#define R0_FS		(1U<<9)		/* first segment of frame */
#define R0_LS		(1U<<8)		/* last segment of frame */
#define R0_CSUM		(3U<<6)		/* 7:6 checksum status */
#define R0_CERR		(2U<<6)		/* 0 (undone), 1 (found ok), 2 (bad) */
/* R1 frame address 63:32 */
/* R2 frame address 31:0 */
/* R3 31:16 received frame length, 15:0 buffer length to receive */

#define SCX_NTXSEGS		16
#define SCX_TXQUEUELEN		16
#define SCX_TXQUEUELEN_MASK	(SCX_TXQUEUELEN - 1)
#define SCX_TXQUEUE_GC		(SCX_TXQUEUELEN / 4)
#define SCX_NTXDESC		(SCX_TXQUEUELEN * SCX_NTXSEGS)
#define SCX_NTXDESC_MASK	(SCX_NTXDESC - 1)
#define SCX_NEXTTX(x)		(((x) + 1) & SCX_NTXDESC_MASK)
#define SCX_NEXTTXS(x)		(((x) + 1) & SCX_TXQUEUELEN_MASK)

#define SCX_NRXDESC		64
#define SCX_NRXDESC_MASK	(SCX_NRXDESC - 1)
#define SCX_NEXTRX(x)		(((x) + 1) & SCX_NRXDESC_MASK)

#define SCX_INIT_RXDESC(sc, x)						\
do {									\
	struct scx_rxsoft *__rxs = &(sc)->sc_rxsoft[(x)];		\
	struct rdes *__rxd = &(sc)->sc_rxdescs[(x)];			\
	struct mbuf *__m = __rxs->rxs_mbuf;				\
	bus_addr_t __paddr =__rxs->rxs_dmamap->dm_segs[0].ds_addr;	\
	__m->m_data = __m->m_ext.ext_buf;				\
	__rxd->r3 = __rxs->rxs_dmamap->dm_segs[0].ds_len;		\
	__rxd->r2 = htole32(BUS_ADDR_LO32(__paddr));			\
	__rxd->r1 = htole32(BUS_ADDR_HI32(__paddr));			\
	__rxd->r0 = R0_OWN | R0_FS | R0_LS;				\
	if ((x) == SCX_NRXDESC - 1) __rxd->r0 |= R0_EOD;		\
} while (/*CONSTCOND*/0)

struct control_data {
	struct tdes cd_txdescs[SCX_NTXDESC];
	struct rdes cd_rxdescs[SCX_NRXDESC];
};
#define SCX_CDOFF(x)		offsetof(struct control_data, x)
#define SCX_CDTXOFF(x)		SCX_CDOFF(cd_txdescs[(x)])
#define SCX_CDRXOFF(x)		SCX_CDOFF(cd_rxdescs[(x)])

struct scx_txsoft {
	struct mbuf *txs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t txs_dmamap;	/* our DMA map */
	int txs_firstdesc;		/* first descriptor in packet */
	int txs_lastdesc;		/* last descriptor in packet */
	int txs_ndesc;			/* # of descriptors used */
};

struct scx_rxsoft {
	struct mbuf *rxs_mbuf;		/* head of our mbuf chain */
	bus_dmamap_t rxs_dmamap;	/* our DMA map */
};

struct scx_softc {
	device_t sc_dev;		/* generic device information */
	bus_space_tag_t sc_st;		/* bus space tag */
	bus_space_handle_t sc_sh;	/* bus space handle */
	bus_size_t sc_sz;		/* csr map size */
	bus_space_handle_t sc_eesh;	/* eeprom section handle */
	bus_size_t sc_eesz;		/* eeprom map size */
	bus_dma_tag_t sc_dmat;		/* bus DMA tag */
	struct ethercom sc_ethercom;	/* Ethernet common data */
	struct mii_data sc_mii;		/* MII */
	callout_t sc_tick_ch;		/* PHY monitor callout */
	bus_dma_segment_t sc_seg;	/* descriptor store seg */
	int sc_nseg;			/* descriptor store nseg */
	void *sc_ih;			/* interrupt cookie */
	int sc_phy_id;			/* PHY address */
	int sc_flowflags;		/* 802.3x PAUSE flow control */
	uint32_t sc_gar;		/* GAR 5:2 clock selection */
	uint32_t sc_t0coso;		/* T0_CSUM | T0_SGOL to run */
	int sc_ucodeloaded;		/* ucode for H2M/M2H/PKT */
	int sc_100mii;			/* 1<<15 RMII/MII, 0 for RGMII */
	int sc_phandle;			/* fdt phandle */

	bus_dmamap_t sc_cddmamap;	/* control data DMA map */
#define sc_cddma	sc_cddmamap->dm_segs[0].ds_addr

	struct control_data *sc_control_data;
#define sc_txdescs	sc_control_data->cd_txdescs
#define sc_rxdescs	sc_control_data->cd_rxdescs

	struct scx_txsoft sc_txsoft[SCX_TXQUEUELEN];
	struct scx_rxsoft sc_rxsoft[SCX_NRXDESC];
	int sc_txfree;			/* number of free Tx descriptors */
	int sc_txnext;			/* next ready Tx descriptor */
	int sc_txsfree;			/* number of free Tx jobs */
	int sc_txsnext;			/* next ready Tx job */
	int sc_txsdirty;		/* dirty Tx jobs */
	int sc_rxptr;			/* next ready Rx descriptor/descsoft */

	krndsource_t rnd_source;	/* random source */
};

#define SCX_CDTXADDR(sc, x)	((sc)->sc_cddma + SCX_CDTXOFF((x)))
#define SCX_CDRXADDR(sc, x)	((sc)->sc_cddma + SCX_CDRXOFF((x)))

#define SCX_CDTXSYNC(sc, x, n, ops)					\
do {									\
	int __x, __n;							\
									\
	__x = (x);							\
	__n = (n);							\
									\
	/* If it will wrap around, sync to the end of the ring. */	\
	if ((__x + __n) > SCX_NTXDESC) {				\
		bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,	\
		    SCX_CDTXOFF(__x), sizeof(struct tdes) *		\
		    (SCX_NTXDESC - __x), (ops));			\
		__n -= (SCX_NTXDESC - __x);				\
		__x = 0;						\
	}								\
									\
	/* Now sync whatever is left. */				\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    SCX_CDTXOFF(__x), sizeof(struct tdes) * __n, (ops));	\
} while (/*CONSTCOND*/0)

#define SCX_CDRXSYNC(sc, x, ops)					\
do {									\
	bus_dmamap_sync((sc)->sc_dmat, (sc)->sc_cddmamap,		\
	    SCX_CDRXOFF((x)), sizeof(struct rdes), (ops));		\
} while (/*CONSTCOND*/0)

static int scx_fdt_match(device_t, cfdata_t, void *);
static void scx_fdt_attach(device_t, device_t, void *);
static int scx_acpi_match(device_t, cfdata_t, void *);
static void scx_acpi_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(scx_fdt, sizeof(struct scx_softc),
    scx_fdt_match, scx_fdt_attach, NULL, NULL);

CFATTACH_DECL_NEW(scx_acpi, sizeof(struct scx_softc),
    scx_acpi_match, scx_acpi_attach, NULL, NULL);

static void scx_attach_i(struct scx_softc *);
static void scx_reset(struct scx_softc *);
static int scx_init(struct ifnet *);
static void scx_start(struct ifnet *);
static void scx_stop(struct ifnet *, int);
static void scx_watchdog(struct ifnet *);
static int scx_ioctl(struct ifnet *, u_long, void *);
static void scx_set_rcvfilt(struct scx_softc *);
static int scx_ifmedia_upd(struct ifnet *);
static void scx_ifmedia_sts(struct ifnet *, struct ifmediareq *);
static void mii_statchg(struct ifnet *);
static void phy_tick(void *);
static int mii_readreg(device_t, int, int, uint16_t *);
static int mii_writereg(device_t, int, int, uint16_t);
static int scx_intr(void *);
static void txreap(struct scx_softc *);
static void rxintr(struct scx_softc *);
static int add_rxbuf(struct scx_softc *, int);
static int spin_waitfor(struct scx_softc *, int, int);
static int mac_read(struct scx_softc *, int);
static void mac_write(struct scx_softc *, int, int);
static void loaducode(struct scx_softc *);
static void injectucode(struct scx_softc *, int, bus_addr_t, bus_size_t);

#define CSR_READ(sc,off) \
	    bus_space_read_4((sc)->sc_st, (sc)->sc_sh, (off))
#define CSR_WRITE(sc,off,val) \
	    bus_space_write_4((sc)->sc_st, (sc)->sc_sh, (off), (val))
#define EE_READ(sc,off) \
	    bus_space_read_4((sc)->sc_st, (sc)->sc_eesh, (off))

static int
scx_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	static const char * compatible[] = {
		"socionext,synquacer-netsec",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
scx_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct scx_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	bus_space_handle_t eebsh;
	bus_addr_t addr[2];
	bus_size_t size[2];
	char intrstr[128];
	const char *phy_mode;

	if (fdtbus_get_reg(phandle, 0, addr+0, size+0) != 0
	    || bus_space_map(faa->faa_bst, addr[0], size[0], 0, &bsh) != 0) {
		aprint_error(": unable to map device csr\n");
		return;
	}
	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		goto fail;
	}
	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET,
		NOT_MP_SAFE, scx_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}
	if (fdtbus_get_reg(phandle, 1, addr+1, size+1) != 0
	    || bus_space_map(faa->faa_bst, addr[0], size[1], 0, &eebsh) != 0) {
		aprint_error(": unable to map device eeprom\n");
		goto fail;
	}

	phy_mode = fdtbus_get_string(phandle, "phy-mode");
	if (phy_mode == NULL) {
		aprint_error(": missing 'phy-mode' property\n");
		phy_mode = "rgmii";
	}

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");
	aprint_normal_dev(self, "interrupt on %s\n", intrstr);

	sc->sc_dev = self;
	sc->sc_st = bst;
	sc->sc_sh = bsh;
	sc->sc_sz = size[0];
	sc->sc_eesh = eebsh;
	sc->sc_eesz = size[1];
	sc->sc_dmat = faa->faa_dmat;
	sc->sc_phandle = phandle;
	sc->sc_100mii = (strcmp(phy_mode, "rgmii") != 0) ? MCR_USEMII : 0;

	scx_attach_i(sc);
	return;
 fail:
	if (sc->sc_eesz)
		bus_space_unmap(sc->sc_st, sc->sc_eesh, sc->sc_eesz);
	if (sc->sc_sz)
		bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
	return;
}

static int
scx_acpi_match(device_t parent, cfdata_t cf, void *aux)
{
	static const char * compatible[] = {
		"SCX0001",
		NULL
	};
	struct acpi_attach_args *aa = aux;

	if (aa->aa_node->ad_type != ACPI_TYPE_DEVICE)
		return 0;
	return acpi_match_hid(aa->aa_node->ad_devinfo, compatible);
}

static void
scx_acpi_attach(device_t parent, device_t self, void *aux)
{
	struct scx_softc * const sc = device_private(self);
	struct acpi_attach_args * const aa = aux;
	ACPI_HANDLE handle = aa->aa_node->ad_handle;
	bus_space_tag_t bst = aa->aa_memt;
	bus_space_handle_t bsh, eebsh;
	struct acpi_resources res;
	struct acpi_mem *mem;
	struct acpi_irq *irq;
	ACPI_STATUS rv;

	rv = acpi_resource_parse(self, handle, "_CRS",
	    &res, &acpi_resource_parse_ops_default);
	if (ACPI_FAILURE(rv))
		return;
	mem = acpi_res_mem(&res, 0);
	irq = acpi_res_irq(&res, 0);
	if (mem == NULL || irq == NULL || mem->ar_length == 0) {
		aprint_error(": incomplete csr resources\n");
		return;
	}
	if (bus_space_map(bst, mem->ar_base, mem->ar_length, 0, &bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	sc->sc_sz = mem->ar_length;
	sc->sc_ih = acpi_intr_establish(self, (uint64_t)handle, IPL_NET,
	    NOT_MP_SAFE, scx_intr, sc, device_xname(self));
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt\n");
		goto fail;
	}
	mem = acpi_res_mem(&res, 1); /* EEPROM for MAC address and ucode */
	if (mem == NULL || mem->ar_length == 0) {
		aprint_error(": incomplete eeprom resources\n");
		goto fail;
	}
	if (bus_space_map(bst, mem->ar_base, mem->ar_length, 0, &eebsh) != 0) {
		aprint_error(": couldn't map registers\n");
		goto fail;
	}
	sc->sc_eesz = mem->ar_length;

	aprint_naive("\n");
	aprint_normal(": Gigabit Ethernet Controller\n");

	sc->sc_dev = self;
	sc->sc_st = bst;
	sc->sc_sh = bsh;
	sc->sc_eesh = eebsh;
	sc->sc_dmat = aa->aa_dmat64;

	scx_attach_i(sc);

	acpi_resource_cleanup(&res);
	return;
 fail:
	if (sc->sc_eesz > 0)
		bus_space_unmap(sc->sc_st, sc->sc_eesh, sc->sc_eesz);
	if (sc->sc_sz > 0)
		bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
	acpi_resource_cleanup(&res);
	return;
}

static void
scx_attach_i(struct scx_softc *sc)
{
	struct ifnet * const ifp = &sc->sc_ethercom.ec_if;
	struct mii_data * const mii = &sc->sc_mii;
	struct ifmedia * const ifm = &mii->mii_media;
	uint32_t hwver, phyfreq;
	uint8_t enaddr[ETHER_ADDR_LEN];
	bus_dma_segment_t seg;
	uint32_t csr;
	int i, nseg, error = 0;

	hwver = CSR_READ(sc, HWVER1);
	csr = bus_space_read_4(sc->sc_st, sc->sc_eesh, 0);
	enaddr[0] = csr >> 24;
	enaddr[1] = csr >> 16;
	enaddr[2] = csr >> 8;
	enaddr[3] = csr;
	csr = bus_space_read_4(sc->sc_st, sc->sc_eesh, 4);
	enaddr[4] = csr >> 24;
	enaddr[5] = csr >> 16;
	csr = CSR_READ(sc, GMACIMPL);

	aprint_normal_dev(sc->sc_dev, "NetSec GbE (%d.%d) impl (%x.%x)\n",
	    hwver >> 16, hwver & 0xffff, csr >> 16, csr & 0xffff);
	aprint_normal_dev(sc->sc_dev,
	    "Ethernet address %s\n", ether_sprintf(enaddr));

	phyfreq = 0;
	sc->sc_phy_id = MII_PHY_ANY;
	sc->sc_gar = get_garclk(phyfreq) << GAR_CTL; /* 5:2 gar control */

	sc->sc_flowflags = 0;

	if (sc->sc_ucodeloaded == 0)
		loaducode(sc);

	mii->mii_ifp = ifp;
	mii->mii_readreg = mii_readreg;
	mii->mii_writereg = mii_writereg;
	mii->mii_statchg = mii_statchg;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(ifm, 0, scx_ifmedia_upd, scx_ifmedia_sts);
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
	ifp->if_ioctl = scx_ioctl;
	ifp->if_start = scx_start;
	ifp->if_watchdog = scx_watchdog;
	ifp->if_init = scx_init;
	ifp->if_stop = scx_stop;
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr);

	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, phy_tick, sc);

	/*
	 * Allocate the control data structures, and create and load the
	 * DMA map for it.
	 */
	error = bus_dmamem_alloc(sc->sc_dmat,
	    sizeof(struct control_data), PAGE_SIZE, 0, &seg, 1, &nseg, 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to allocate control data, error = %d\n", error);
		goto fail_0;
	}
	error = bus_dmamem_map(sc->sc_dmat, &seg, nseg,
	    sizeof(struct control_data), (void **)&sc->sc_control_data,
	    BUS_DMA_COHERENT);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to map control data, error = %d\n", error);
		goto fail_1;
	}
	error = bus_dmamap_create(sc->sc_dmat,
	    sizeof(struct control_data), 1,
	    sizeof(struct control_data), 0, 0, &sc->sc_cddmamap);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to create control data DMA map, "
		    "error = %d\n", error);
		goto fail_2;
	}
	error = bus_dmamap_load(sc->sc_dmat, sc->sc_cddmamap,
	    sc->sc_control_data, sizeof(struct control_data), NULL, 0);
	if (error != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to load control data DMA map, error = %d\n",
		    error);
		goto fail_3;
	}
	for (i = 0; i < SCX_TXQUEUELEN; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    SCX_NTXSEGS, MCLBYTES, 0, 0,
		    &sc->sc_txsoft[i].txs_dmamap)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create tx DMA map %d, error = %d\n",
			    i, error);
			goto fail_4;
		}
	}
	for (i = 0; i < SCX_NRXDESC; i++) {
		if ((error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, 0, &sc->sc_rxsoft[i].rxs_dmamap)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "unable to create rx DMA map %d, error = %d\n",
			    i, error);
			goto fail_5;
		}
		sc->sc_rxsoft[i].rxs_mbuf = NULL;
	}
	sc->sc_seg = seg;
	sc->sc_nseg = nseg;
printf("bus_dmaseg ds_addr %08lx, ds_len %08lx, nseg %d\n", seg.ds_addr, seg.ds_len, nseg);

	if (pmf_device_register(sc->sc_dev, NULL, NULL))
		pmf_class_network_register(sc->sc_dev, ifp);
	else
		aprint_error_dev(sc->sc_dev,
			"couldn't establish power handler\n");

	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

	return;

  fail_5:
	for (i = 0; i < SCX_NRXDESC; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
  fail_4:
	for (i = 0; i < SCX_TXQUEUELEN; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	bus_dmamap_unload(sc->sc_dmat, sc->sc_cddmamap);
  fail_3:
	bus_dmamap_destroy(sc->sc_dmat, sc->sc_cddmamap);
  fail_2:
	bus_dmamem_unmap(sc->sc_dmat, (void *)sc->sc_control_data,
	    sizeof(struct control_data));
  fail_1:
	bus_dmamem_free(sc->sc_dmat, &seg, nseg);
  fail_0:
	if (sc->sc_phandle)
		fdtbus_intr_disestablish(sc->sc_phandle, sc->sc_ih);
	else
		acpi_intr_disestablish(sc->sc_ih);
	bus_space_unmap(sc->sc_st, sc->sc_sh, sc->sc_sz);
	return;
}

static void
scx_reset(struct scx_softc *sc)
{

	mac_write(sc, GMACBMR, BMR_RST); /* may take for a while */
	(void)spin_waitfor(sc, GMACBMR, BMR_RST);

	CSR_WRITE(sc, DESCENG_SRST, 1);
	CSR_WRITE(sc, DESCENG_INIT, 1);
	mac_write(sc, GMACBMR, _BMR);
	mac_write(sc, GMACRDLAR, _RDLAR);
	mac_write(sc, GMACTDLAR, _TDLAR);
	mac_write(sc, GMACAFR, _AFR);
}

static int
scx_init(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	const uint8_t *ea = CLLADDR(ifp->if_sadl);
	uint32_t csr;
	int i;

	/* Cancel pending I/O. */
	scx_stop(ifp, 0);

	/* Reset the chip to a known state. */
	scx_reset(sc);

	/* build sane Tx and load Rx descriptors with mbuf */
	for (i = 0; i < SCX_NTXDESC; i++)
		sc->sc_txdescs[i].t0 = T0_OWN;
	sc->sc_txdescs[SCX_NTXDESC - 1].t0 |= T0_EOD; /* tie off the ring */
	for (i = 0; i < SCX_NRXDESC; i++)
		(void)add_rxbuf(sc, i);

	/* set my address in perfect match slot 0 */
	csr = (ea[3] << 24) | (ea[2] << 16) | (ea[1] << 8) |  ea[0];
	CSR_WRITE(sc, GMACMAL0, csr);
	csr = (ea[5] << 8) | ea[4];
	CSR_WRITE(sc, GMACMAH0, csr | 1U<<31); /* always valid? */

	/* accept multicast frame or run promisc mode */
	scx_set_rcvfilt(sc);

	(void)scx_ifmedia_upd(ifp);

	/* kick to start GMAC engine */
	csr = mac_read(sc, GMACOMR);
	CSR_WRITE(sc, RXINT_CLR, ~0);
	CSR_WRITE(sc, TXINT_CLR, ~0);
	mac_write(sc, GMACOMR, csr | OMR_RXE | OMR_TXE);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* start one second timer */
	callout_schedule(&sc->sc_tick_ch, hz);

	return 0;
}

static void
scx_stop(struct ifnet *ifp, int disable)
{
	struct scx_softc *sc = ifp->if_softc;

	/* Stop the one second clock. */
	callout_stop(&sc->sc_tick_ch);

	/* Down the MII. */
	mii_down(&sc->sc_mii);

	/* Mark the interface down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

static void
scx_watchdog(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;

	/*
	 * Since we're not interrupting every packet, sweep
	 * up before we report an error.
	 */
	txreap(sc);

	if (sc->sc_txfree != SCX_NTXDESC) {
		aprint_error_dev(sc->sc_dev,
		    "device timeout (txfree %d txsfree %d txnext %d)\n",
		    sc->sc_txfree, sc->sc_txsfree, sc->sc_txnext);
		if_statinc(ifp, if_oerrors);

		/* Reset the interface. */
		scx_init(ifp);
	}

	scx_start(ifp);
}

static int
scx_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct scx_softc *sc = ifp->if_softc;
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
			scx_set_rcvfilt(sc);
		}
		break;
	}

	splx(s);
	return error;
}

static void
scx_set_rcvfilt(struct scx_softc *sc)
{
	struct ethercom * const ec = &sc->sc_ethercom;
	struct ifnet * const ifp = &ec->ec_if;
	struct ether_multistep step;
	struct ether_multi *enm;
	uint32_t mchash[8]; 	/* 8x 32 = 256 bit */
	uint32_t csr, crc;
	int i;

	csr = CSR_READ(sc, GMACAFR);
	csr &= ~(AFR_PM | AFR_AM | AFR_MHTE);
	CSR_WRITE(sc, GMACAFR, csr);

	ETHER_LOCK(ec);
	if (ifp->if_flags & IFF_PROMISC) {
		ec->ec_flags |= ETHER_F_ALLMULTI;
		ETHER_UNLOCK(ec);
		goto update;
	}
	ec->ec_flags &= ~ETHER_F_ALLMULTI;

	/* clear 15 entry supplimental perfect match filter */
	for (i = 1; i < 16; i++)
		 CSR_WRITE(sc, GMACMAH(i), 0);
	/* build 256 bit multicast hash filter */
	memset(mchash, 0, sizeof(mchash));
	crc = 0;

	ETHER_FIRST_MULTI(step, ec, enm);
	i = 1; /* slot 0 is occupied */
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
printf("[%d] %s\n", i, ether_sprintf(enm->enm_addrlo));
		if (i < 16) {
			/* use 31 entry perfect match filter */
			uint32_t addr;
			uint8_t *ep = enm->enm_addrlo;
			addr = (ep[3] << 24) | (ep[2] << 16)
			     | (ep[1] <<  8) |  ep[0];
			CSR_WRITE(sc, GMACMAL(i), addr);
			addr = (ep[5] << 8) | ep[4];
			CSR_WRITE(sc, GMACMAH(i), addr | 1U<<31);
		} else {
			/* use hash table when too many */
			/* bit_reserve_32(~crc) !? */
			crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
			/* 3(31:29) 5(28:24) bit sampling */
			mchash[crc >> 29] |= 1 << ((crc >> 24) & 0x1f);
		}
		ETHER_NEXT_MULTI(step, enm);
		i++;
	}
	ETHER_UNLOCK(ec);

	if (crc)
		csr |= AFR_MHTE;
	for (i = 0; i < 8; i++)
		CSR_WRITE(sc, GMACMHT0 + i * 4, mchash[i]);
	CSR_WRITE(sc, GMACAFR, csr);
	return;

 update:
	/* With PM or AM, MHTE/MHTL/MHTH are never consulted. really? */
	if (ifp->if_flags & IFF_PROMISC)
		csr |= AFR_PM;	/* run promisc. mode */
	else
		csr |= AFR_AM;	/* accept all multicast */
	CSR_WRITE(sc, GMACAFR, csr);
	return;
}

static int
scx_ifmedia_upd(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	struct ifmedia *ifm = &sc->sc_mii.mii_media;

	if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_AUTO) {
		; /* restart AN */
		; /* enable AN */
		; /* advertise flow control pause */
		; /* adv. 100FDX,100HDX,10FDX,10HDX */
	} else {
#if 1 /* XXX not sure to belong here XXX */
		uint32_t mcr = mac_read(sc, GMACMCR);
		if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_1000_T)
			mcr &= ~MCR_USEMII; /* RGMII+SPD1000 */
		else {
			if (IFM_SUBTYPE(ifm->ifm_cur->ifm_media) == IFM_100_TX
			    && sc->sc_100mii)
				mcr |= MCR_SPD100;
			mcr |= MCR_USEMII;
		}
		if (ifm->ifm_cur->ifm_media & IFM_FDX)
			mcr |= MCR_USEFDX;
		mcr |= MCR_CST | MCR_JE;
		if (sc->sc_100mii == 0)
			mcr |= MCR_IBN;
		mac_write(sc, GMACMCR, mcr);
#endif
	}
	return 0;
}

static void
scx_ifmedia_sts(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct scx_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;

	mii_pollstat(mii);
	ifmr->ifm_status = mii->mii_media_status;
	ifmr->ifm_active = sc->sc_flowflags |
	    (mii->mii_media_active & ~IFM_ETH_FMASK);
}

void
mii_statchg(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	struct mii_data *mii = &sc->sc_mii;
	uint32_t fcr;

	/* Get flow control negotiation result. */
	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->sc_flowflags)
		sc->sc_flowflags = mii->mii_media_active & IFM_ETH_FMASK;

	/* Adjust PAUSE flow control. */
	fcr = mac_read(sc, GMACFCR) & ~(FCR_TFE | FCR_RFE);
	if (mii->mii_media_active & IFM_FDX) {
		if (sc->sc_flowflags & IFM_ETH_TXPAUSE)
			fcr |= FCR_TFE;
		if (sc->sc_flowflags & IFM_ETH_RXPAUSE)
			fcr |= FCR_RFE;
	}
	mac_write(sc, GMACFCR, fcr);

printf("%ctxfe, %crxfe\n",
     (fcr & FCR_TFE) ? '+' : '-', (fcr & FCR_RFE) ? '+' : '-');
}

static void
phy_tick(void *arg)
{
	struct scx_softc *sc = arg;
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
	struct scx_softc *sc = device_private(self);
	uint32_t gar;
	int error;

	gar = (phy << GAR_PHY) | (reg << GAR_REG) | sc->sc_gar;
	mac_write(sc, GMACGAR, gar | GAR_BUSY);
	error = spin_waitfor(sc, GMACGAR, GAR_BUSY);
	if (error)
		return error;
	*val = mac_read(sc, GMACGDR);
	return 0;
}

static int
mii_writereg(device_t self, int phy, int reg, uint16_t val)
{
	struct scx_softc *sc = device_private(self);
	uint32_t gar;
	uint16_t dummy;
	int error;

	gar = (phy << GAR_PHY) | (reg << GAR_REG) | sc->sc_gar;
	mac_write(sc, GMACGDR, val);
	mac_write(sc, GMACGAR, gar | GAR_IOWR | GAR_BUSY);
	error = spin_waitfor(sc, GMACGAR, GAR_BUSY);
	if (error)
		return error;
	mii_readreg(self, phy, MII_PHYIDR1, &dummy); /* dummy read cycle */
	return 0;
}

static void
scx_start(struct ifnet *ifp)
{
	struct scx_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	struct scx_txsoft *txs;
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

		if (sc->sc_txsfree < SCX_TXQUEUE_GC) {
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
		     seg++, nexttx = SCX_NEXTTX(nexttx)) {
			struct tdes *tdes = &sc->sc_txdescs[nexttx];
			bus_addr_t paddr = dmamap->dm_segs[seg].ds_addr;
			/*
			 * If this is the first descriptor we're
			 * enqueueing, don't set the OWN bit just
			 * yet.	 That could cause a race condition.
			 * We'll do it below.
			 */
			tdes->t3 = dmamap->dm_segs[seg].ds_len;
			tdes->t2 = htole32(BUS_ADDR_LO32(paddr));
			tdes->t1 = htole32(BUS_ADDR_HI32(paddr));
			tdes->t0 = tdes0 | (tdes->t0 & T0_EOD) |
					(15 << T0_TRID) | T0_PT |
					sc->sc_t0coso | T0_TRS;
			tdes0 = T0_OWN; /* 2nd and other segments */
			lasttx = nexttx;
		}
		/*
		 * Outgoing NFS mbuf must be unloaded when Tx completed.
		 * Without T1_IC NFS mbuf is left unack'ed for excessive
		 * time and NFS stops to proceed until scx_watchdog()
		 * calls txreap() to reclaim the unack'ed mbuf.
		 * It's painful to traverse every mbuf chain to determine
		 * whether someone is waiting for Tx completion.
		 */
		m = m0;
		do {
			if ((m->m_flags & M_EXT) && m->m_ext.ext_free) {
				sc->sc_txdescs[lasttx].t0 |= T0_IOC; /* !!! */
				break;
			}
		} while ((m = m->m_next) != NULL);

		/* Write deferred 1st segment T0_OWN at the final stage */
		sc->sc_txdescs[lasttx].t0 |= T0_LS;
		sc->sc_txdescs[sc->sc_txnext].t0 |= (T0_FS | T0_OWN);
		SCX_CDTXSYNC(sc, sc->sc_txnext, dmamap->dm_nsegs,
		    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

		/* Tell DMA start transmit */
		/* CSR_WRITE(sc, MDTSC, 1); */

		txs->txs_mbuf = m0;
		txs->txs_firstdesc = sc->sc_txnext;
		txs->txs_lastdesc = lasttx;
		txs->txs_ndesc = dmamap->dm_nsegs;

		sc->sc_txfree -= txs->txs_ndesc;
		sc->sc_txnext = nexttx;
		sc->sc_txsfree--;
		sc->sc_txsnext = SCX_NEXTTXS(sc->sc_txsnext);
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
scx_intr(void *arg)
{
	struct scx_softc *sc = arg;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	(void)ifp;
	rxintr(sc);
	txreap(sc);
	return 1;
}

static void
txreap(struct scx_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct scx_txsoft *txs;
	uint32_t txstat;
	int i;

	ifp->if_flags &= ~IFF_OACTIVE;

	for (i = sc->sc_txsdirty; sc->sc_txsfree != SCX_TXQUEUELEN;
	     i = SCX_NEXTTXS(i), sc->sc_txsfree++) {
		txs = &sc->sc_txsoft[i];

		SCX_CDTXSYNC(sc, txs->txs_firstdesc, txs->txs_ndesc,
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
	if (sc->sc_txsfree == SCX_TXQUEUELEN)
		ifp->if_timer = 0;
}

static void
rxintr(struct scx_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct scx_rxsoft *rxs;
	struct mbuf *m;
	uint32_t rxstat;
	int i, len;

	for (i = sc->sc_rxptr; /*CONSTCOND*/ 1; i = SCX_NEXTRX(i)) {
		rxs = &sc->sc_rxsoft[i];

		SCX_CDRXSYNC(sc, i,
		    BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);

		rxstat = sc->sc_rxdescs[i].r0;
		if (rxstat & R0_OWN) /* desc is left empty */
			break;

		/* R0_FS | R0_LS must have been marked for this desc */

		bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
		    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_POSTREAD);

		len = sc->sc_rxdescs[i].r3 >> 16; /* 31:16 received */
		len -= ETHER_CRC_LEN;	/* Trim CRC off */
		m = rxs->rxs_mbuf;

		if (add_rxbuf(sc, i) != 0) {
			if_statinc(ifp, if_ierrors);
			SCX_INIT_RXDESC(sc, i);
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
add_rxbuf(struct scx_softc *sc, int i)
{
	struct scx_rxsoft *rxs = &sc->sc_rxsoft[i];
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
	SCX_INIT_RXDESC(sc, i);

	return 0;
}

static int
spin_waitfor(struct scx_softc *sc, int reg, int exist)
{
	int val, loop;

	val = CSR_READ(sc, reg);
	if ((val & exist) == 0)
		return 0;
	loop = 3000;
	do {
		DELAY(10);
		val = CSR_READ(sc, reg);
	} while (--loop > 0 && (val & exist));
	return (loop > 0) ? 0 : ETIMEDOUT;
}

static int
mac_read(struct scx_softc *sc, int reg)
{

	CSR_WRITE(sc, MACCMD, reg);
	(void)spin_waitfor(sc, MACCMD, CMD_BUSY);
	return CSR_READ(sc, MACDATA);
}

static void
mac_write(struct scx_softc *sc, int reg, int val)
{

	CSR_WRITE(sc, MACDATA, val);
	CSR_WRITE(sc, MACCMD, reg | CMD_IOWR);
	(void)spin_waitfor(sc, MACCMD, CMD_BUSY);
}

static int
get_garclk(uint32_t freq)
{

	const struct {
		uint16_t freq, bit; /* GAR 5:2 MDIO frequency selection */
	} garclk[] = {
		{ 35,	2 },	/* 25-35 MHz */
		{ 60,	3 },	/* 35-60 MHz */
		{ 100,	0 },	/* 60-100 MHz */
		{ 150,	1 },	/* 100-150 MHz */
		{ 250,	4 },	/* 150-250 MHz */
		{ 300,	5 },	/* 250-300 MHz */
	};
	int i;

	/* convert MDIO clk to a divisor value */
	if (freq < garclk[0].freq)
		return garclk[0].bit;
	for (i = 1; i < __arraycount(garclk); i++) {
		if (freq < garclk[i].freq)
			return garclk[i-1].bit;
	}
	return garclk[__arraycount(garclk) - 1].bit;
}

static void
loaducode(struct scx_softc *sc)
{
	uint32_t up, lo, sz;
	uint64_t addr;

	sc->sc_ucodeloaded = 1;

	up = EE_READ(sc, 0x08); /* H->M ucode addr high */
	lo = EE_READ(sc, 0x0c); /* H->M ucode addr low */
	sz = EE_READ(sc, 0x10); /* H->M ucode size */
	sz *= 4;
	addr = ((uint64_t)up << 32) | lo;
	aprint_normal_dev(sc->sc_dev, "H2M ucode %u\n", sz);
	injectucode(sc, H2MENG, (bus_addr_t)addr, (bus_size_t)sz);

	up = EE_READ(sc, 0x14); /* M->H ucode addr high */
	lo = EE_READ(sc, 0x18); /* M->H ucode addr low */
	sz = EE_READ(sc, 0x1c); /* M->H ucode size */
	sz *= 4;
	addr = ((uint64_t)up << 32) | lo;
	injectucode(sc, M2HENG, (bus_addr_t)addr, (bus_size_t)sz);
	aprint_normal_dev(sc->sc_dev, "M2H ucode %u\n", sz);

	lo = EE_READ(sc, 0x20); /* PKT ucode addr */
	sz = EE_READ(sc, 0x24); /* PKT ucode size */
	sz *= 4;
	injectucode(sc, PKTENG, (bus_addr_t)lo, (bus_size_t)sz);
	aprint_normal_dev(sc->sc_dev, "PKT ucode %u\n", sz);
}

static void
injectucode(struct scx_softc *sc, int port,
	bus_addr_t addr, bus_size_t size)
{
	bus_space_handle_t bsh;
	bus_size_t off;
	uint32_t ucode;

	if (!bus_space_map(sc->sc_st, addr, size, 0, &bsh) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "eeprom map failure for ucode port 0x%x\n", port);
		return;
	}
	for (off = 0; off < size; off += 4) {
		ucode = bus_space_read_4(sc->sc_st, bsh, off);
		CSR_WRITE(sc, port, ucode);
	}
	bus_space_unmap(sc->sc_st, bsh, size);
}
