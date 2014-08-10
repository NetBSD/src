/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry.
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

#include "locators.h"

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: awin_eth.c,v 1.5 2014/08/10 16:44:33 tls Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/ioctl.h>
#include <sys/mutex.h>
#include <sys/rnd.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <dev/mii/miivar.h>

#include <arm/allwinner/awin_reg.h>
#include <arm/allwinner/awin_var.h>

static int awin_eth_match(device_t, cfdata_t, void *);
static void awin_eth_attach(device_t, device_t, void *);

static int awin_eth_intr(void *);

static int awin_eth_miibus_read_reg(device_t, int, int);
static void awin_eth_miibus_write_reg(device_t, int, int, int);
static void awin_eth_miibus_statchg(struct ifnet *);

static void awin_eth_ifstart(struct ifnet *);
static int awin_eth_ifioctl(struct ifnet *, u_long, void *);
static int awin_eth_ifinit(struct ifnet *);
static void awin_eth_ifstop(struct ifnet *, int);
static void awin_eth_ifwatchdog(struct ifnet *);
static void awin_eth_ifdrain(struct ifnet *);

struct awin_eth_softc;
static void awin_eth_rx_hash(struct awin_eth_softc *);

struct awin_eth_regs {
	uint32_t reg_ctl;
	uint32_t reg_ts_mode;
	uint32_t reg_ts_flow;
	uint32_t reg_ts_ctl[2];
	uint32_t reg_ts_ins;
	uint32_t reg_ts_pl[2];
	uint32_t reg_ts_sta;
	uint32_t reg_rx_ctl;
	uint32_t reg_rx_hash[2];
	uint32_t reg_rx_sta;
	uint32_t reg_rx_fbc;
	uint32_t reg_int_ctl;
	uint32_t reg_mac_ctl0;
	uint32_t reg_mac_ctl1;
	uint32_t reg_mac_ipgt;
	uint32_t reg_mac_ipgr;
	uint32_t reg_mac_clrt;
	uint32_t reg_mac_maxf;
	uint32_t reg_mac_supp;
	uint32_t reg_mac_a0;
	uint32_t reg_mac_a1;
	uint32_t reg_mac_a2;
};

struct awin_eth_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	bus_dma_tag_t sc_dmat;
	struct ethercom sc_ec;
	struct mii_data sc_mii;
	krndsource_t sc_rnd_source;	/* random source */
	kmutex_t sc_intr_lock;
	kmutex_t sc_mdio_lock;
	uint8_t sc_tx_active;
	struct awin_eth_regs sc_reg;
	void *sc_ih;
};

CFATTACH_DECL_NEW(awin_eth, sizeof(struct awin_eth_softc),
	awin_eth_match, awin_eth_attach, NULL, NULL);

static const struct awin_gpio_pinset awin_eth_pinsets[2] = {
        [0] = { 'A', AWIN_PIO_PA_EMAC_FUNC, AWIN_PIO_PA_EMAC_PINS },
        [1] = { 'H', AWIN_PIO_PH_EMAC_FUNC, AWIN_PIO_PH_EMAC_PINS },
};

static inline uint32_t
awin_eth_read(struct awin_eth_softc *sc, bus_size_t o)
{
	return bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
}

static inline void
awin_eth_write(struct awin_eth_softc *sc, bus_size_t o, uint32_t v)
{
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, v);
}

static inline void
awin_eth_clear_set(struct awin_eth_softc *sc, bus_size_t o, uint32_t c,
    uint32_t s)
{
	uint32_t v = bus_space_read_4(sc->sc_bst, sc->sc_bsh, o);
	return bus_space_write_4(sc->sc_bst, sc->sc_bsh, o, (v & ~c) | s);
}

static int
awin_eth_match(device_t parent, cfdata_t cf, void *aux)
{
	struct awinio_attach_args * const aio = aux;
#ifdef DIAGNOSTIC
	const struct awin_locators * const loc = &aio->aio_loc;
#endif
	const struct awin_gpio_pinset * const pinset =
	    &awin_eth_pinsets[cf->cf_flags & 1];

	KASSERT(!strcmp(cf->cf_name, loc->loc_name));
	KASSERT(cf->cf_loc[AWINIOCF_PORT] == AWINIOCF_PORT_DEFAULT
	    || cf->cf_loc[AWINIOCF_PORT] == loc->loc_port);

	if (!awin_gpio_pinset_available(pinset))
		return 0;

	return 1;
}

static void
awin_eth_attach(device_t parent, device_t self, void *aux)
{
	cfdata_t cf = device_cfdata(self);
	struct awin_eth_softc * const sc = device_private(self);
	struct awinio_attach_args * const aio = aux;
	const struct awin_locators * const loc = &aio->aio_loc;
	const struct awin_gpio_pinset * const pinset =
	    &awin_eth_pinsets[cf->cf_flags & 1];
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct mii_data * const mii = &sc->sc_mii;
	char enaddr[ETHER_ADDR_LEN];

	sc->sc_dev = self;

	awin_gpio_pinset_acquire(pinset);
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_ccm_bsh,
	    AWIN_AHB_GATING0_REG, AWIN_AHB_GATING0_EMAC, 0);
	/*
	 * Give 13KB of SRAM to EMAC
	 */
	awin_reg_set_clear(aio->aio_core_bst, aio->aio_core_bsh,
	    AWIN_SRAM_OFFSET + AWIN_SRAM_CTL1_REG,
	    __SHIFTIN(AWIN_SRAM_CTL1_A3_A4_EMAC, AWIN_SRAM_CTL1_A3_A4),
	    AWIN_SRAM_CTL1_A3_A4);

	mutex_init(&sc->sc_intr_lock, MUTEX_DEFAULT, IPL_NET);
	mutex_init(&sc->sc_mdio_lock, MUTEX_DEFAULT, IPL_NET);

	sc->sc_bst = aio->aio_core_bst;
	sc->sc_dmat = aio->aio_dmat;
	bus_space_subregion(sc->sc_bst, aio->aio_core_bsh,
	    loc->loc_offset, loc->loc_size, &sc->sc_bsh);

	aprint_naive("\n");
	aprint_normal(": 10/100 Ethernet Controller\n");

	/*
	 * Diable and then clear all interrupts
	 */
	awin_eth_write(sc, AWIN_EMAC_INT_CTL_REG, 0);
	awin_eth_write(sc, AWIN_EMAC_INT_STA_REG,
	    awin_eth_read(sc, AWIN_EMAC_INT_STA_REG));

	sc->sc_ih = intr_establish(loc->loc_intr, IPL_NET, IST_LEVEL,
	    awin_eth_intr, sc);
	if (sc->sc_ih == NULL) {
		aprint_error_dev(self, "failed to establish interrupt %d\n",
		    loc->loc_intr);
		return;
	}
	aprint_normal_dev(self, "interrupting on irq %d\n", loc->loc_intr);

	uint32_t a1 = awin_eth_read(sc, AWIN_EMAC_MAC_A1_REG);
	uint32_t a0 = awin_eth_read(sc, AWIN_EMAC_MAC_A0_REG);
	if (a0 != 0 || a1 != 0) {
		enaddr[0] = a1 >> 16;
		enaddr[1] = a1 >>  8;
		enaddr[2] = a1 >>  0;
		enaddr[3] = a0 >> 16;
		enaddr[4] = a0 >>  8;
		enaddr[5] = a0 >>  0;
	}

	ifp->if_softc = sc;
	ifp->if_start = awin_eth_ifstart;
	ifp->if_ioctl = awin_eth_ifioctl;
	ifp->if_init = awin_eth_ifinit;
	ifp->if_stop = awin_eth_ifstop;
	ifp->if_watchdog = awin_eth_ifwatchdog;
	ifp->if_drain = awin_eth_ifdrain;

	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);

        mii->mii_ifp = ifp;
        mii->mii_readreg = awin_eth_miibus_read_reg;
        mii->mii_writereg = awin_eth_miibus_write_reg;
        mii->mii_statchg = awin_eth_miibus_statchg;

        mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY,
	    MIIF_DOPAUSE);
                
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
	ether_ifattach(ifp, enaddr); 
	rnd_attach_source(&sc->sc_rnd_source, device_xname(self),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);
}

int
awin_eth_miibus_read_reg(device_t self, int phy, int reg)
{
	struct awin_eth_softc * const sc = device_private(self);

	mutex_enter(&sc->sc_mdio_lock);

	awin_eth_write(sc, AWIN_EMAC_MAC_MADR_REG, (phy << 8) | reg);
	awin_eth_write(sc, AWIN_EMAC_MAC_MCMD_REG, 1);

	delay(100);

	awin_eth_write(sc, AWIN_EMAC_MAC_MCMD_REG, 0);
	const uint32_t rv = awin_eth_read(sc, AWIN_EMAC_MAC_MRDD_REG);

	mutex_exit(&sc->sc_mdio_lock);

	return rv;
}

void
awin_eth_miibus_write_reg(device_t self, int phy, int reg, int val)
{
	struct awin_eth_softc * const sc = device_private(self);

	mutex_enter(&sc->sc_mdio_lock);

	awin_eth_write(sc, AWIN_EMAC_MAC_MADR_REG, (phy << 8) | reg);
	awin_eth_write(sc, AWIN_EMAC_MAC_MCMD_REG, 1);

	delay(100);

	awin_eth_write(sc, AWIN_EMAC_MAC_MCMD_REG, 0);
	awin_eth_write(sc, AWIN_EMAC_MAC_MWTD_REG, val);

	mutex_exit(&sc->sc_mdio_lock);
}

void
awin_eth_miibus_statchg(struct ifnet *ifp)
{
	struct awin_eth_softc * const sc = ifp->if_softc;
	struct mii_data * const mii = &sc->sc_mii;
	const u_int media = mii->mii_media_active;

	/*
	 * Set MII interface based on the speed
	 * negotiated by the PHY.                                           
	 */                                                                 
	switch (IFM_SUBTYPE(media)) {
	case IFM_10_T:
		sc->sc_reg.reg_mac_supp &= ~AWIN_EMAC_MAC_SUPP_100M;
		break;
	case IFM_100_TX:
		sc->sc_reg.reg_mac_supp |= AWIN_EMAC_MAC_SUPP_100M;
		break;
	}

	sc->sc_reg.reg_mac_ctl0 &=
	    ~(AWIN_EMAC_MAC_CTL0_TFC|AWIN_EMAC_MAC_CTL0_RFC);
	if (media & IFM_FLOW) {
		if (media & IFM_ETH_TXPAUSE) {
			sc->sc_reg.reg_mac_ctl0 |= AWIN_EMAC_MAC_CTL0_TFC;
		}
		if (media & IFM_ETH_RXPAUSE) {
			sc->sc_reg.reg_mac_ctl0 |= AWIN_EMAC_MAC_CTL0_RFC;
		}
	}

	sc->sc_reg.reg_mac_ctl1 &= ~AWIN_EMAC_MAC_CTL1_FD;
	if (media & IFM_FDX) {
		sc->sc_reg.reg_mac_ctl1 |= AWIN_EMAC_MAC_CTL1_FD;
	}
}

static inline void
awin_eth_int_enable(struct awin_eth_softc *sc)
{
	awin_eth_clear_set(sc, AWIN_EMAC_INT_CTL_REG, 0,
	    AWIN_EMAC_INT_RX|AWIN_EMAC_INT_TX0|AWIN_EMAC_INT_TX1);
}

static inline void
awin_eth_rxfifo_flush(struct awin_eth_softc *sc)
{
	uint32_t rxctl_reg = awin_eth_read(sc, AWIN_EMAC_RX_CTL_REG);

	if (rxctl_reg & AWIN_EMAC_RX_CTL_DMA) {
		awin_eth_write(sc, AWIN_EMAC_RX_CTL_REG,
		    rxctl_reg & ~AWIN_EMAC_RX_CTL_DMA);
	}
	awin_eth_write(sc, AWIN_EMAC_RX_CTL_REG,
	    (rxctl_reg & ~AWIN_EMAC_RX_CTL_DMA)
		| AWIN_EMAC_RX_CTL_FIFO_RESET);

	for (;;) {
		uint32_t v0 = awin_eth_read(sc, AWIN_EMAC_RX_CTL_REG);
		if ((v0 & AWIN_EMAC_RX_CTL_FIFO_RESET) == 0)
			break;
	}
	awin_eth_write(sc, AWIN_EMAC_RX_CTL_REG, rxctl_reg);
}

static void
awin_eth_rxfifo_consume(struct awin_eth_softc *sc, size_t len)
{
	for (len = (len + 3) >> 2; len > 0; len--) {
		(void) awin_eth_read(sc, AWIN_EMAC_RX_IO_DATA_REG);
	}
}

static void
awin_eth_rxfifo_transfer(struct awin_eth_softc *sc, struct mbuf *m)
{
	uint32_t *dp32 = (uint32_t *)(m->m_data);

	bus_space_read_multi_4(sc->sc_bst, sc->sc_bsh, 
	    AWIN_EMAC_RX_IO_DATA_REG, dp32, m->m_len >> 2);

	/*
	 * Now we have to copy any remaining bytes that if it wasn't
	 * a multiple of 4 bytes.
	 */
	if (m->m_len & 3) {
		const uint32_t v = awin_eth_read(sc, AWIN_EMAC_RX_IO_DATA_REG);
		memcpy(m->m_data, &v, m->m_len & 3);
	}

	/*
	 * Now all we have is the CRC to read and discard.  We may have read
	 * to three bytes of it but there is at least one more byte to read
	 * discard.
	 */
	(void) awin_eth_read(sc, AWIN_EMAC_RX_IO_DATA_REG);
}

static struct mbuf *
awin_eth_mgethdr(struct awin_eth_softc *sc, size_t rxlen)
{
	struct mbuf *m = m_gethdr(M_DONTWAIT, MT_DATA);

	if (rxlen + 2 > MHLEN) {
		MCLGET(m, M_DONTWAIT);
		if ((m->m_flags & M_EXT) == 0) {
			m_free(m);
			return NULL;
		}
	}

	m->m_data += 2;
	m->m_len = rxlen;
	m->m_pkthdr.len = rxlen;
	m->m_pkthdr.rcvif = &sc->sc_ec.ec_if;

	return m;
}

static void
awin_eth_if_input(struct awin_eth_softc *sc, struct mbuf *m)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;

	(*ifp->if_input)(ifp, m);
}

static void
awin_eth_rx_intr(struct awin_eth_softc *sc)
{
	for (;;) {
		uint32_t rx_count = awin_eth_read(sc, AWIN_EMAC_RX_FBC_REG);
		struct mbuf *m;

		if (rx_count == 0) {
			awin_eth_int_enable(sc);
			rx_count = awin_eth_read(sc, AWIN_EMAC_RX_FBC_REG);
			if (rx_count == 0)
				return;
		}

		uint32_t v = awin_eth_read(sc, AWIN_EMAC_RX_IO_DATA_REG);
		if (v != AWIN_EMAC_RX_MAGIC) {
			awin_eth_rxfifo_flush(sc);
			awin_eth_int_enable(sc);
			return;
		}

		uint32_t rxhdr = awin_eth_read(sc, AWIN_EMAC_RX_IO_DATA_REG);
		uint32_t rxlen = __SHIFTOUT(rxhdr, AWIN_EMAC_RXHDR_LEN);
		uint32_t rxsts = __SHIFTOUT(rxhdr, AWIN_EMAC_RXHDR_STS);
		bool drop = false;

		if (rxlen < ETHER_MIN_LEN) {
			drop = true;
		} else if (rxsts & (AWIN_EMAC_RX_STA_CRCERR | AWIN_EMAC_RX_STA_LENERR | AWIN_EMAC_RX_STA_ALNERR)) {
			drop = true;
			sc->sc_ec.ec_if.if_ierrors++;
		}

		if (!drop) {
			m = awin_eth_mgethdr(sc, rxlen - 4);
			drop = (m == NULL);
		}

		if (drop) {
			awin_eth_rxfifo_consume(sc, rxlen);
			awin_eth_int_enable(sc);
			return;
		}

		awin_eth_rxfifo_transfer(sc, m);
		awin_eth_if_input(sc, m);
	}
}

static void
awin_eth_txfifo_transfer(struct awin_eth_softc *sc, struct mbuf *m, u_int slot)
{
	bus_size_t const io_data_reg = AWIN_EMAC_TX_IO_DATA_REG(0 /*slot*/);
	u_int leftover = 0;
	uint32_t v = 0;
	signed int pad = ETHER_MIN_LEN - ETHER_CRC_LEN - m->m_pkthdr.len;

	for (; m != NULL; m = m->m_next) {
		uint8_t *dp = m->m_data;
		size_t len = m->m_len;
		if (len == 0)
			continue;

		if (leftover > 0) {
			/*
			 * We do a memcpy instead of dereferencing an uint32_t 
			 * in case these are the last bytes of a page which is
			 * followed by an unmapped page.
			 */
			uint32_t uv = 0;
			memcpy(&uv, dp, 32 - leftover / 8);
			if (leftover + len * 8 < 32) {
				v |= ((uv & ~0L) << (len * 8)) << leftover;
				leftover += len * 8;
				continue;
			}
			dp += (32 - leftover) / 8;
			len -= (32 - leftover) / 8;
			awin_eth_write(sc, io_data_reg, v | (uv << leftover)); 
			v = 0;
			leftover = 0;
			if (len == 0)
				continue;
		}
		KASSERT(len > 0);
		KASSERT(leftover == 0);
		bus_space_write_multi_4(sc->sc_bst, sc->sc_bsh, io_data_reg,
		    (const uint32_t *)dp, len >> 2);

		dp += len & -4;
		len &= 3;

		if (len == 0)
			continue;

		/*
		 * We do a memcpy instead of dereferencing an uint32_t in case
		 * these are the last bytes of a page which is followed by an
		 * unmapped page.
		 */
		uint32_t uv = 0;
		memcpy(&uv, dp, len);
		const u_int bits = len * 8;
		if (bits + leftover < 32) {
			v |= ((uv & ~0U) << bits) << leftover;
			leftover += bits;
			continue;
		}
		awin_eth_write(sc, io_data_reg, v | (uv << leftover)); 
		if (bits + leftover == 32) {
			v = 0;
			leftover = 0;
		} else {
			v = uv >> (32 - leftover);
			leftover = (leftover + bits) & 31;
			v &= ~0U << leftover;
		}
	}

	/*
	 * If we have data yet to be written, write it now (padding by 0).
	 * Be sure to subtract the extra zeroes from the pad bytes.
	 */
	if (leftover) {
		awin_eth_write(sc, io_data_reg, v); 
		pad -= 32 - leftover / 8;
	}

	/*
	 * Pad the packet out to minimum packet size.
	 */
	for (; pad > 4; pad -= 4) {
		awin_eth_write(sc, io_data_reg, 0); 
	}
}

static void
awin_eth_tx_enqueue(struct awin_eth_softc *sc, struct mbuf *m, u_int slot)
{
	awin_eth_write(sc, AWIN_EMAC_TX_INS_REG, slot);

	awin_eth_txfifo_transfer(sc, m, slot);

	bus_size_t const pl_reg = AWIN_EMAC_TX_PL_REG(slot);
	bus_size_t const ctl_reg = AWIN_EMAC_TX_CTL_REG(slot);

	awin_eth_write(sc, pl_reg, m->m_pkthdr.len);
	awin_eth_clear_set(sc, ctl_reg, 0, AWIN_EMAC_TX_CTL_START);

	m_freem(m);
}

static void
awin_eth_tx_intr(struct awin_eth_softc *sc, u_int slot)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct mbuf *m;

	IF_DEQUEUE(&ifp->if_snd, m);

	if (m == NULL) {
		awin_eth_tx_enqueue(sc, m, slot);
	} else {
		sc->sc_tx_active &= ~__BIT(slot);
		ifp->if_flags &= ~IFF_OACTIVE;
	}
}

int
awin_eth_intr(void *arg)
{
	struct awin_eth_softc * const sc = arg;
	int rv = 0;

	mutex_enter(&sc->sc_intr_lock);

	for (;;) {
		uint32_t sts = awin_eth_read(sc, AWIN_EMAC_INT_STA_REG);
		awin_eth_write(sc, AWIN_EMAC_INT_STA_REG, sts);
                rnd_add_uint32(&sc->sc_rnd_source, sts);

		if ((sts & (AWIN_EMAC_INT_RX|AWIN_EMAC_INT_TX0|AWIN_EMAC_INT_TX1)) == 0)
			break;

		rv = 1;
		if (sts & AWIN_EMAC_INT_RX) {
			awin_eth_rx_intr(sc);
		}
		if (sts & AWIN_EMAC_INT_TX0) {
			awin_eth_tx_intr(sc, 0);
		}
		if (sts & AWIN_EMAC_INT_TX1) {
			awin_eth_tx_intr(sc, 1);
		}
	}
	mutex_exit(&sc->sc_intr_lock);

	return rv;
}

void
awin_eth_ifstart(struct ifnet *ifp)
{
	struct awin_eth_softc * const sc = ifp->if_softc;

	mutex_enter(&sc->sc_intr_lock);

	if ((sc->sc_tx_active & 1) == 0) {
		struct mbuf *m;
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL) {
			mutex_exit(&sc->sc_intr_lock);
			return;
		}
		awin_eth_tx_enqueue(sc, m, 0);
		sc->sc_tx_active |= 1;
	}

	if ((sc->sc_tx_active & 2) == 0) {
		struct mbuf *m;
		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL) {
			mutex_exit(&sc->sc_intr_lock);
			return;
		}
		awin_eth_tx_enqueue(sc, m, 1);
		sc->sc_tx_active |= 2;
	}

	if (sc->sc_tx_active == 3)
		ifp->if_flags |= IFF_OACTIVE;

	mutex_exit(&sc->sc_intr_lock);
}


static int
awin_eth_ifioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct awin_eth_softc * const sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int error;

	mutex_enter(&sc->sc_intr_lock);

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
			awin_eth_ifstop(ifp, 0);
			error = awin_eth_ifinit(ifp);
		}
		break;
	}

	mutex_exit(&sc->sc_intr_lock);

	return error;
}

static void
awin_eth_ifstop(struct ifnet *ifp, int discard)
{
	struct awin_eth_softc * const sc = ifp->if_softc;

	KASSERT(mutex_owned(&sc->sc_intr_lock));
	awin_eth_write(sc, AWIN_EMAC_CTL_REG, 0);
	delay(200);
	awin_eth_write(sc, AWIN_EMAC_CTL_REG, 1);
	delay(200);
}

int
awin_eth_ifinit(struct ifnet *ifp)
{
	struct awin_eth_softc * const sc = ifp->if_softc;

	awin_eth_rx_hash(sc);
	return 0;
}

void
awin_eth_ifwatchdog(struct ifnet *ifp)
{
}

void
awin_eth_ifdrain(struct ifnet *ifp)
{
}

static void
awin_eth_rx_hash(struct awin_eth_softc *sc)
{
	struct ifnet * const ifp = &sc->sc_ec.ec_if;
	struct ether_multistep step;
	struct ether_multi *enm;

	if (ifp->if_flags & IFF_PROMISC) {
		sc->sc_reg.reg_rx_hash[0] = sc->sc_reg.reg_rx_hash[1] = ~0;
		ifp->if_flags |= IFF_ALLMULTI;
		return;
	}

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
			sc->sc_reg.reg_rx_hash[0] = sc->sc_reg.reg_rx_hash[1] = ~0;
			ifp->if_flags |= IFF_ALLMULTI;
			return;
                }

#if 0
		u_int crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
#else
		u_int crc = ether_crc32_be(enm->enm_addrlo, ETHER_ADDR_LEN);
#endif
		 
		/* Just want the 6 most significant bits. */
		crc >>= 26; 
                
		/* Set the corresponding bit in the filter. */
		sc->sc_reg.reg_rx_hash[crc >> 5] |= __BIT(crc & 31);
                ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
}
