/* $NetBSD: sun4i_emac.c,v 1.3.2.1 2018/07/28 04:37:29 pgoyette Exp $ */

/*-
 * Copyright (c) 2013-2017 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Jared McNeill.
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

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: sun4i_emac.c,v 1.3.2.1 2018/07/28 04:37:29 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_sramc.h>

#define	EMAC_IFNAME	"emac%d"

#define	EMAC_CTL_REG		0x00
#define	 EMAC_CTL_RX_EN			__BIT(2)
#define	 EMAC_CTL_TX_EN			__BIT(1)
#define	 EMAC_CTL_RST			__BIT(0)
#define	EMAC_TX_MODE_REG	0x04
#define	 EMAC_TX_MODE_DMA		__BIT(1)
#define	 EMAC_TX_MODE_ABF_ENA		__BIT(0)
#define	EMAC_TX_FLOW_REG	0x08
#define	EMAC_TX_CTL0_REG	0x0c
#define	EMAC_TX_CTL1_REG	0x10
#define	EMAC_TX_CTL_REG(n)	(EMAC_TX_CTL0_REG+4*(n))
#define	 EMAC_TX_CTL_START		__BIT(0)
#define	EMAC_TX_INS_REG		0x14
#define	EMAC_TX_PL0_REG		0x18
#define	EMAC_TX_PL1_REG		0x1c
#define	EMAC_TX_PL_REG(n)	(EMAC_TX_PL0_REG+4*(n))
#define	EMAC_TX_STA_REG		0x20
#define	EMAC_TX_IO_DATA0_REG	0x24
#define	EMAC_TX_IO_DATA1_REG	0x28
#define	EMAC_TX_IO_DATA_REG(n)	(EMAC_TX_IO_DATA0_REG+4*(n))
#define	EMAC_TX_TSVL0_REG	0x2c
#define	EMAC_TX_TSVH0_REG	0x30
#define	EMAC_TX_TSVL1_REG	0x34
#define	EMAC_TX_TSVH1_REG	0x38
#define	EMAC_RX_CTL_REG		0x3c
#define	 EMAC_RX_CTL_SA_IF		__BIT(25)
#define	 EMAC_RX_CTL_SA			__BIT(24)
#define	 EMAC_RX_CTL_BC0		__BIT(22)
#define	 EMAC_RX_CTL_MHF		__BIT(21)
#define	 EMAC_RX_CTL_MC0		__BIT(20)
#define	 EMAC_RX_CTL_DAF		__BIT(17)
#define	 EMAC_RX_CTL_UCAD		__BIT(16)
#define	 EMAC_RX_CTL_POR		__BIT(8)
#define	 EMAC_RX_CTL_PLE		__BIT(7)
#define	 EMAC_RX_CTL_PCRCE		__BIT(6)
#define	 EMAC_RX_CTL_PCF		__BIT(5)
#define	 EMAC_RX_CTL_PROMISC		__BIT(4)
#define	 EMAC_RX_CTL_FIFO_RESET		__BIT(3)
#define	 EMAC_RX_CTL_DMA		__BIT(2)
#define	 EMAC_RX_CTL_DRQ_MODE		__BIT(1)
#define	 EMAC_RX_CTL_START		__BIT(0)
#define	EMAC_RX_HASH0_REG	0x40
#define	EMAC_RX_HASH1_REG	0x44
#define	EMAC_RX_STA_REG		0x48
#define	 EMAC_RX_STA_PKTOK		__BIT(7)
#define	 EMAC_RX_STA_ALNERR		__BIT(6)
#define	 EMAC_RX_STA_LENERR		__BIT(5)
#define	 EMAC_RX_STA_CRCERR		__BIT(4)
#define	EMAC_RX_IO_DATA_REG	0x4c
#define	EMAC_RX_FBC_REG		0x50
#define	EMAC_INT_CTL_REG	0x54
#define	EMAC_INT_STA_REG	0x58
#define	 EMAC_INT_RX			__BIT(8)
#define	 EMAC_INT_TX1			__BIT(1)
#define	 EMAC_INT_TX0			__BIT(0)
#define	 EMAC_INT_ENABLE		\
		(EMAC_INT_RX|EMAC_INT_TX1|EMAC_INT_TX0)
#define	EMAC_MAC_CTL0_REG	0x5c
#define	 EMAC_MAC_CTL0_SOFT_RESET	__BIT(15)
#define	 EMAC_MAC_CTL0_TFC		__BIT(3)
#define	 EMAC_MAC_CTL0_RFC		__BIT(2)
#define	EMAC_MAC_CTL1_REG	0x60
#define	 EMAC_MAC_CTL1_ED		__BIT(15)
#define	 EMAC_MAC_CTL1_NB		__BIT(13)
#define	 EMAC_MAC_CTL1_BNB		__BIT(12)
#define	 EMAC_MAC_CTL1_LPE		__BIT(9)
#define	 EMAC_MAC_CTL1_PRE		__BIT(8)
#define	 EMAC_MAC_CTL1_ADP		__BIT(7)
#define	 EMAC_MAC_CTL1_VC		__BIT(6)
#define	 EMAC_MAC_CTL1_PC		__BIT(5)
#define	 EMAC_MAC_CTL1_CRC		__BIT(4)
#define	 EMAC_MAC_CTL1_DCRC		__BIT(3)
#define	 EMAC_MAC_CTL1_HF		__BIT(2)
#define	 EMAC_MAC_CTL1_FLC		__BIT(1)
#define	 EMAC_MAC_CTL1_FD		__BIT(0)
#define	EMAC_MAC_IPGT_REG	0x64
#define	 EMAC_MAC_IPGT_FD		0x15
#define	EMAC_MAC_IPGR_REG	0x68
#define	 EMAC_MAC_IPGR_IPG1		__BITS(15,8)
#define	 EMAC_MAC_IPGR_IPG2		__BITS(7,0)
#define	EMAC_MAC_CLRT_REG	0x6c
#define	 EMAC_MAC_CLRT_CW		__BITS(15,8)
#define	 EMAC_MAC_CLRT_RM		__BITS(7,0)
#define	EMAC_MAC_MAXF_REG	0x70
#define	EMAC_MAC_SUPP_REG	0x74
#define	 EMAC_MAC_SUPP_100M		__BIT(8)
#define	EMAC_MAC_TEST_REG	0x78
#define	EMAC_MAC_MCFG_REG	0x7c
#define	 EMAC_MAC_MCFG_CLK		__BITS(5,2)
#define	EMAC_MAC_MCMD_REG	0x80
#define	EMAC_MAC_MADR_REG	0x84
#define	EMAC_MAC_MWTD_REG	0x88
#define	EMAC_MAC_MRDD_REG	0x8c
#define	EMAC_MAC_MIND_REG	0x90
#define	EMAC_MAC_SSRR_REG	0x94
#define	EMAC_MAC_A0_REG		0x98
#define	EMAC_MAC_A1_REG		0x9c
#define	EMAC_MAC_A2_REG		0xa0

#define	EMAC_RXHDR_STS			__BITS(31,16)
#define	EMAC_RXHDR_LEN			__BITS(15,0)

#define	EMAC_RX_MAGIC		0x0143414d	/* M A C \001 */

#define	EMAC_TXBUF_SIZE		4096

static int sun4i_emac_match(device_t, cfdata_t, void *);
static void sun4i_emac_attach(device_t, device_t, void *);

static int sun4i_emac_intr(void *);
static void sun4i_emac_tick(void *);

static int sun4i_emac_miibus_read_reg(device_t, int, int);
static void sun4i_emac_miibus_write_reg(device_t, int, int, int);
static void sun4i_emac_miibus_statchg(struct ifnet *);

static void sun4i_emac_ifstart(struct ifnet *);
static int sun4i_emac_ifioctl(struct ifnet *, u_long, void *);
static int sun4i_emac_ifinit(struct ifnet *);
static void sun4i_emac_ifstop(struct ifnet *, int);
static void sun4i_emac_ifwatchdog(struct ifnet *);

struct sun4i_emac_softc;
static void sun4i_emac_rx_hash(struct sun4i_emac_softc *);

struct sun4i_emac_softc {
	device_t sc_dev;
	int sc_phandle;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	struct ethercom sc_ec;
	struct mii_data sc_mii;
	krndsource_t sc_rnd_source;	/* random source */
	kmutex_t sc_intr_lock;
	uint8_t sc_tx_active;
	callout_t sc_stat_ch;
	void *sc_ih;
	uint32_t sc_txbuf[EMAC_TXBUF_SIZE/4];
};

static const char * compatible[] = {
	"allwinner,sun4i-a10-emac",
	NULL
};

CFATTACH_DECL_NEW(sun4i_emac, sizeof(struct sun4i_emac_softc),
	sun4i_emac_match, sun4i_emac_attach, NULL, NULL);

static inline uint32_t
sun4i_emac_read(struct sun4i_emac_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
sun4i_emac_write(struct sun4i_emac_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static inline void
sun4i_emac_clear_set(struct sun4i_emac_softc *sc, bus_size_t o, uint32_t c,
    uint32_t s)
{
	uint32_t v = bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, (v & ~c) | s);
}

static int
sun4i_emac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sun4i_emac_attach(device_t parent, device_t self, void *aux)
{
	struct sun4i_emac_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct mii_data * const mii = &sc->sc_mii;
	const int phandle = faa->faa_phandle;
	char enaddr[ETHER_ADDR_LEN];
	const uint8_t *local_addr;
	char intrstr[128];
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	int len;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": cannot get registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": cannot decode interrupt\n");
		return;
	}

	clk = fdtbus_clock_get_index(phandle, 0);
	if (clk == NULL) {
		aprint_error(": cannot acquire clock\n");
		return;
	}
	if (clk_enable(clk) != 0) {
		aprint_error(": cannot enable clock\n");
		return;
	}

	if (sunxi_sramc_claim(phandle) != 0) {
		aprint_error(": cannot map SRAM to EMAC\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_ec.ec_mii = mii;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": cannot map registers\n");
		return;
	}
	sc->sc_dmat = faa->faa_dmat;

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NET);
	callout_init(&sc->sc_stat_ch, 0);
	callout_setfunc(&sc->sc_stat_ch, sun4i_emac_tick, sc);

	aprint_naive("\n");
	aprint_normal(": 10/100 Ethernet Controller\n");

	/*
	 * Disable and then clear all interrupts
	 */
	sun4i_emac_write(sc, EMAC_INT_CTL_REG, 0);
	sun4i_emac_write(sc, EMAC_INT_STA_REG,
	    sun4i_emac_read(sc, EMAC_INT_STA_REG));

	sc->sc_ih = fdtbus_intr_establish(phandle, 0, IPL_NET, 0,
	    sun4i_emac_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	local_addr = fdtbus_get_prop(phandle, "local-mac-address", &len);
	if (local_addr && len == ETHER_ADDR_LEN) {
		memcpy(enaddr, local_addr, ETHER_ADDR_LEN);

		uint32_t a1 = ((uint32_t)enaddr[0] << 16) |
			      ((uint32_t)enaddr[1] << 8) |
			       (uint32_t)enaddr[2];
		uint32_t a0 = ((uint32_t)enaddr[3] << 16) |
			      ((uint32_t)enaddr[4] << 8) |
			       (uint32_t)enaddr[5];

		sun4i_emac_write(sc, EMAC_MAC_A1_REG, a1);
		sun4i_emac_write(sc, EMAC_MAC_A0_REG, a0);
	}

	uint32_t a1 = sun4i_emac_read(sc, EMAC_MAC_A1_REG);
	uint32_t a0 = sun4i_emac_read(sc, EMAC_MAC_A0_REG);
	if (a0 != 0 || a1 != 0) {
		enaddr[0] = a1 >> 16;
		enaddr[1] = a1 >>  8;
		enaddr[2] = a1 >>  0;
		enaddr[3] = a0 >> 16;
		enaddr[4] = a0 >>  8;
		enaddr[5] = a0 >>  0;
	}
	aprint_normal_dev(self, "Ethernet address %s\n", ether_sprintf(enaddr));

	snprintf(ifp->if_xname, IFNAMSIZ, EMAC_IFNAME, device_unit(self));
	ifp->if_softc = sc;
	ifp->if_capabilities = 0;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = sun4i_emac_ifstart;
	ifp->if_ioctl = sun4i_emac_ifioctl;
	ifp->if_init = sun4i_emac_ifinit;
	ifp->if_stop = sun4i_emac_ifstop;
	ifp->if_watchdog = sun4i_emac_ifwatchdog;
	IFQ_SET_READY(&ifp->if_snd);

	/* 802.1Q VLAN-sized frames are supported */
	sc->sc_ec.ec_capabilities |= ETHERCAP_VLAN_MTU;

	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);

        mii->mii_ifp = ifp;
        mii->mii_readreg = sun4i_emac_miibus_read_reg;
        mii->mii_writereg = sun4i_emac_miibus_write_reg;
        mii->mii_statchg = sun4i_emac_miibus_statchg;

        mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0);

        if (LIST_EMPTY(&mii->mii_phys)) { 
                aprint_error_dev(self, "no PHY found!\n");
                ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
                ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_MANUAL);
        } else {
                ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);
        }

	/*      
	 * Attach the interface.
	 */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, enaddr); 
	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);
}

static inline void
sun4i_emac_int_enable(struct sun4i_emac_softc *sc)
{
	sun4i_emac_clear_set(sc, EMAC_INT_CTL_REG, 0,
	    EMAC_INT_ENABLE);
	sun4i_emac_write(sc, EMAC_INT_STA_REG,
	    sun4i_emac_read(sc, EMAC_INT_STA_REG));
}

int
sun4i_emac_miibus_read_reg(device_t self, int phy, int reg)
{
	struct sun4i_emac_softc * const sc = device_private(self);
	int retry = 100;

	sun4i_emac_write(sc, EMAC_MAC_MADR_REG, (phy << 8) | reg);
	sun4i_emac_write(sc, EMAC_MAC_MCMD_REG, 1);

	while (--retry > 0 && (sun4i_emac_read(sc, EMAC_MAC_MIND_REG) & 1) != 0)
		delay(1000);
	if (retry == 0)
		device_printf(self, "PHY read timeout\n");

	sun4i_emac_write(sc, EMAC_MAC_MCMD_REG, 0);
	const uint32_t rv = sun4i_emac_read(sc, EMAC_MAC_MRDD_REG);

	return rv;
}

void
sun4i_emac_miibus_write_reg(device_t self, int phy, int reg, int val)
{
	struct sun4i_emac_softc * const sc = device_private(self);
	int retry = 100;

	sun4i_emac_write(sc, EMAC_MAC_MADR_REG, (phy << 8) | reg);
	sun4i_emac_write(sc, EMAC_MAC_MCMD_REG, 1);

	while (--retry > 0 && (sun4i_emac_read(sc, EMAC_MAC_MIND_REG) & 1) != 0)
		delay(1000);
	if (retry == 0)
		device_printf(self, "PHY write timeout\n");

	sun4i_emac_write(sc, EMAC_MAC_MCMD_REG, 0);
	sun4i_emac_write(sc, EMAC_MAC_MWTD_REG, val);
}

void
sun4i_emac_miibus_statchg(struct ifnet *ifp)
{
	struct sun4i_emac_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;
	const u_int media = mii->mii_media_active;

	/*
	 * Set MII interface based on the speed
	 * negotiated by the PHY.                                           
	 */                                                                 
	switch (IFM_SUBTYPE(media)) {
	case IFM_10_T:
		sun4i_emac_clear_set(sc, EMAC_MAC_SUPP_REG,
		    EMAC_MAC_SUPP_100M, 0);
		break;
	case IFM_100_TX:
		sun4i_emac_clear_set(sc, EMAC_MAC_SUPP_REG,
		    0, EMAC_MAC_SUPP_100M);
		break;
	}

	const bool link = (IFM_SUBTYPE(media) & (IFM_10_T|IFM_100_TX)) != 0;
	if (link) {
		if (media & IFM_FDX) {
			sun4i_emac_clear_set(sc, EMAC_MAC_CTL1_REG,
			    0, EMAC_MAC_CTL1_FD);
		} else {
			sun4i_emac_clear_set(sc, EMAC_MAC_CTL1_REG,
			    EMAC_MAC_CTL1_FD, 0);
		}
	}
}

static void
sun4i_emac_tick(void *softc)
{
	struct sun4i_emac_softc * const sc = softc;
	struct mii_data * const mii = &sc->sc_mii;
	int s;

	s = splnet();
	mii_tick(mii);
	callout_schedule(&sc->sc_stat_ch, hz);
	splx(s);
}

static inline void
sun4i_emac_rxfifo_flush(struct sun4i_emac_softc *sc)
{
	sun4i_emac_clear_set(sc, EMAC_CTL_REG, EMAC_CTL_RX_EN, 0);

	sun4i_emac_clear_set(sc, EMAC_RX_CTL_REG, 0, EMAC_RX_CTL_FIFO_RESET);

	for (;;) {
		uint32_t v0 = sun4i_emac_read(sc, EMAC_RX_CTL_REG);
		if ((v0 & EMAC_RX_CTL_FIFO_RESET) == 0)
			break;
	}

	sun4i_emac_clear_set(sc, EMAC_CTL_REG, 0, EMAC_CTL_RX_EN);
}

static void
sun4i_emac_rxfifo_consume(struct sun4i_emac_softc *sc, size_t len)
{
	for (len = (len + 3) >> 2; len > 0; len--) {
		(void) sun4i_emac_read(sc, EMAC_RX_IO_DATA_REG);
	}
}

static void
sun4i_emac_rxfifo_transfer(struct sun4i_emac_softc *sc, struct mbuf *m)
{
	uint32_t *dp32 = mtod(m, uint32_t *);
	const int len = roundup2(m->m_len, 4);

	bus_space_read_multi_4(sc->sc_bst, sc->sc_bsh, 
	    EMAC_RX_IO_DATA_REG, dp32, len / 4);
}

static struct mbuf *
sun4i_emac_mgethdr(struct sun4i_emac_softc *sc, size_t rxlen)
{
	struct mbuf *m = m_gethdr(M_DONTWAIT, MT_DATA);

	if (m == NULL) {
		return NULL;
	}
	if (rxlen + 2 > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return NULL;
		}
	}

	m_adj(m, 2);
	m->m_len = rxlen;
	m->m_pkthdr.len = rxlen;
	m_set_rcvif(m, &sc->sc_ec.ec_if);
	m->m_flags |= M_HASFCS;

	return m;
}

static void
sun4i_emac_if_input(struct sun4i_emac_softc *sc, struct mbuf *m)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;

	if_percpuq_enqueue(ifp->if_percpuq, m);
}

static void
sun4i_emac_rx_intr(struct sun4i_emac_softc *sc)
{
	for (;;) {
		uint32_t rx_count = sun4i_emac_read(sc, EMAC_RX_FBC_REG);
		struct mbuf *m;

		if (rx_count == 0) {
			rx_count = sun4i_emac_read(sc, EMAC_RX_FBC_REG);
			if (rx_count == 0)
				return;
		}

		uint32_t v = sun4i_emac_read(sc, EMAC_RX_IO_DATA_REG);
		if (v != EMAC_RX_MAGIC) {
			sun4i_emac_rxfifo_flush(sc);
			return;
		}

		uint32_t rxhdr = sun4i_emac_read(sc, EMAC_RX_IO_DATA_REG);
		uint32_t rxlen = __SHIFTOUT(rxhdr, EMAC_RXHDR_LEN);
		uint32_t rxsts = __SHIFTOUT(rxhdr, EMAC_RXHDR_STS);

		if (rxlen < ETHER_MIN_LEN || (rxsts & EMAC_RX_STA_PKTOK) == 0) {
			sc->sc_ec.ec_if.if_ierrors++;
			continue;
		}

		m = sun4i_emac_mgethdr(sc, rxlen);
		if (m == NULL) {
			sc->sc_ec.ec_if.if_ierrors++;
			sun4i_emac_rxfifo_consume(sc, rxlen);
			return;
		}

		sun4i_emac_rxfifo_transfer(sc, m);
		sun4i_emac_if_input(sc, m);
	}
}

static int
sun4i_emac_txfifo_transfer(struct sun4i_emac_softc *sc, struct mbuf *m, u_int slot)
{
	bus_size_t const io_data_reg = EMAC_TX_IO_DATA_REG(0);
	const int len = m->m_pkthdr.len;
	uint32_t *pktdata;

	KASSERT(len > 0 && len <= sizeof(sc->sc_txbuf));

	if (m->m_next != NULL) {
		m_copydata(m, 0, len, sc->sc_txbuf);
		pktdata = sc->sc_txbuf;
	} else {
		pktdata = mtod(m, uint32_t *);
	}

	bus_space_write_multi_4(sc->sc_bst, sc->sc_bsh, io_data_reg,
	    pktdata, roundup2(len, 4) / 4);

	return len;
}

static void
sun4i_emac_tx_enqueue(struct sun4i_emac_softc *sc, struct mbuf *m, u_int slot)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;

	sun4i_emac_write(sc, EMAC_TX_INS_REG, slot);

	const int len = sun4i_emac_txfifo_transfer(sc, m, slot);

	bus_size_t const pl_reg = EMAC_TX_PL_REG(slot);
	bus_size_t const ctl_reg = EMAC_TX_CTL_REG(slot);

	sun4i_emac_write(sc, pl_reg, len);
	sun4i_emac_clear_set(sc, ctl_reg, 0, EMAC_TX_CTL_START);

	bpf_mtap(ifp, m, BPF_D_OUT);

	m_freem(m);
}

static void
sun4i_emac_tx_intr(struct sun4i_emac_softc *sc, u_int slot)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;

	sc->sc_tx_active &= ~__BIT(slot);
	ifp->if_flags &= ~IFF_OACTIVE;
}

int
sun4i_emac_intr(void *arg)
{
	struct sun4i_emac_softc * const sc = arg;
	struct ifnet * const ifp = &sc->sc_ec.ec_if;

	mutex_enter(&sc->sc_intr_lock);

	uint32_t sts = sun4i_emac_read(sc, EMAC_INT_STA_REG);
	sun4i_emac_write(sc, EMAC_INT_STA_REG, sts);
	rnd_add_uint32(&sc->sc_rnd_source, sts);

	if (sts & EMAC_INT_RX) {
		sun4i_emac_rx_intr(sc);
	}
	if (sts & EMAC_INT_TX0) {
		sun4i_emac_tx_intr(sc, 0);
	}
	if (sts & EMAC_INT_TX1) {
		sun4i_emac_tx_intr(sc, 1);
	}
	if (sts & (EMAC_INT_TX0|EMAC_INT_TX1)) {
		if (sc->sc_tx_active == 0)
			ifp->if_timer = 0;
		if_schedule_deferred_start(ifp);
	}

	mutex_exit(&sc->sc_intr_lock);

	return 1;
}

void
sun4i_emac_ifstart(struct ifnet *ifp)
{
	struct sun4i_emac_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->sc_intr_lock);

	if ((sc->sc_tx_active & 1) == 0) {
		struct mbuf *m;
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL) {
			mutex_exit(&sc->sc_intr_lock);
			return;
		}
		sun4i_emac_tx_enqueue(sc, m, 0);
		sc->sc_tx_active |= 1;
	}

	if ((sc->sc_tx_active & 2) == 0) {
		struct mbuf *m;
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL) {
			mutex_exit(&sc->sc_intr_lock);
			return;
		}
		sun4i_emac_tx_enqueue(sc, m, 1);
		sc->sc_tx_active |= 2;
	}

	if (sc->sc_tx_active == 3)
		ifp->if_flags |= IFF_OACTIVE;

	ifp->if_timer = 5;

	mutex_exit(&sc->sc_intr_lock);
}


static int
sun4i_emac_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct sun4i_emac_softc * const sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error;

	switch (cmd) {
	case SIOCGIFMEDIA: 
	case SIOCSIFMEDIA:     
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, cmd);
		break;
	default:
		if ((error = ether_ioctl(ifp, cmd, data)) != ENETRESET)
			break;
		error = 0;
		if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			break;
		if (ifp->if_flags & IFF_RUNNING) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			mutex_enter(&sc->sc_intr_lock);
			sun4i_emac_ifstop(ifp, 0);
			error = sun4i_emac_ifinit(ifp);
			mutex_exit(&sc->sc_intr_lock);
		}
		break;
	}

	return error;
}

static void
sun4i_emac_ifstop(struct ifnet *ifp, int discard)
{
	struct sun4i_emac_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;

	KASSERT(mutex_owned(&sc->sc_intr_lock));

	callout_stop(&sc->sc_stat_ch);
	mii_down(mii);

	sun4i_emac_write(sc, EMAC_INT_CTL_REG, 0);
	sun4i_emac_write(sc, EMAC_INT_STA_REG,
	    sun4i_emac_read(sc, EMAC_INT_STA_REG));

	sun4i_emac_clear_set(sc, EMAC_CTL_REG,
	    EMAC_CTL_RST | EMAC_CTL_TX_EN | EMAC_CTL_RX_EN, 0);

	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;
}

int
sun4i_emac_ifinit(struct ifnet *ifp)
{
	struct sun4i_emac_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;

	sun4i_emac_clear_set(sc, EMAC_RX_CTL_REG,
	    0, EMAC_RX_CTL_FIFO_RESET);

	delay(1);

	sun4i_emac_clear_set(sc, EMAC_MAC_CTL0_REG,
	    EMAC_MAC_CTL0_SOFT_RESET, 0);

	sun4i_emac_clear_set(sc, EMAC_MAC_MCFG_REG,
	    EMAC_MAC_MCFG_CLK, __SHIFTIN(0xd, EMAC_MAC_MCFG_CLK));

	sun4i_emac_write(sc, EMAC_RX_FBC_REG, 0);

	sun4i_emac_write(sc, EMAC_INT_CTL_REG, 0);
	sun4i_emac_write(sc, EMAC_INT_STA_REG,
	    sun4i_emac_read(sc, EMAC_INT_STA_REG));

	delay(1);

	sun4i_emac_clear_set(sc, EMAC_TX_MODE_REG,
	    EMAC_TX_MODE_DMA, EMAC_TX_MODE_ABF_ENA);

	sun4i_emac_clear_set(sc, EMAC_MAC_CTL0_REG,
	    0, EMAC_MAC_CTL0_TFC | EMAC_MAC_CTL0_RFC);

	sun4i_emac_clear_set(sc, EMAC_RX_CTL_REG,
	    EMAC_RX_CTL_DMA, 0);

	sun4i_emac_clear_set(sc, EMAC_MAC_CTL1_REG,
	    0,
	    EMAC_MAC_CTL1_FLC | EMAC_MAC_CTL1_CRC |
	    EMAC_MAC_CTL1_PC);

	sun4i_emac_write(sc, EMAC_MAC_IPGT_REG, EMAC_MAC_IPGT_FD);
	sun4i_emac_write(sc, EMAC_MAC_IPGR_REG,
	    __SHIFTIN(0x0c, EMAC_MAC_IPGR_IPG1) |
	    __SHIFTIN(0x12, EMAC_MAC_IPGR_IPG2));

	sun4i_emac_write(sc, EMAC_MAC_CLRT_REG,
	    __SHIFTIN(0x0f, EMAC_MAC_CLRT_RM) |
	    __SHIFTIN(0x37, EMAC_MAC_CLRT_CW));

	sun4i_emac_write(sc, EMAC_MAC_MAXF_REG, 0x600);

	sun4i_emac_rx_hash(sc);

	sun4i_emac_int_enable(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Enable RX/TX */
	sun4i_emac_clear_set(sc, EMAC_CTL_REG,
	    0, EMAC_CTL_RST | EMAC_CTL_TX_EN | EMAC_CTL_RX_EN);

	mii_mediachg(mii);
	callout_schedule(&sc->sc_stat_ch, hz);

	return 0;
}

static void
sun4i_emac_ifwatchdog(struct ifnet *ifp)
{
	struct sun4i_emac_softc * const sc = ifp->if_softc;

	device_printf(sc->sc_dev, "device timeout\n");

	ifp->if_oerrors++;
	sun4i_emac_ifinit(ifp);
	sun4i_emac_ifstart(ifp);
}

static void
sun4i_emac_rx_hash(struct sun4i_emac_softc *sc)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct ether_multistep step;
	struct ether_multi *enm;
	uint32_t hash[2];
	uint32_t rxctl;

	rxctl = sun4i_emac_read(sc, EMAC_RX_CTL_REG);
	rxctl &= ~EMAC_RX_CTL_MHF;
	rxctl |= EMAC_RX_CTL_UCAD;
	rxctl |= EMAC_RX_CTL_DAF;
	rxctl |= EMAC_RX_CTL_MC0;
	rxctl |= EMAC_RX_CTL_BC0;
	rxctl |= EMAC_RX_CTL_POR;

	hash[0] = hash[1] = ~0;
	if (ifp->if_flags & IFF_PROMISC) {
		ifp->if_flags |= IFF_ALLMULTI;
		rxctl |= EMAC_RX_CTL_PROMISC;
	} else {
		rxctl &= ~EMAC_RX_CTL_PROMISC;
	}

	if ((ifp->if_flags & IFF_PROMISC) == 0) {
		hash[0] = hash[1] = 0;

		ETHER_FIRST_MULTI(step, &sc->sc_ec, enm);
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
				hash[0] = hash[1] = ~0;
				ifp->if_flags |= IFF_ALLMULTI;
				goto done;
                	}

			u_int crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);

			/* Just want the 6 most significant bits. */
			crc >>= 26; 

			/* Set the corresponding bit in the filter. */
			hash[crc >> 5] |= __BIT(crc & 31);
                	ETHER_NEXT_MULTI(step, enm);
		}
		ifp->if_flags &= ~IFF_ALLMULTI;
		rxctl |= EMAC_RX_CTL_MHF;
	}

done:

	sun4i_emac_write(sc, EMAC_RX_HASH0_REG, hash[0]);
	sun4i_emac_write(sc, EMAC_RX_HASH1_REG, hash[1]);

	sun4i_emac_write(sc, EMAC_RX_CTL_REG, rxctl);
}
