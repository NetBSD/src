/*	$NetBSD: if_enet.c,v 1.35 2022/01/24 09:14:37 andvar Exp $	*/

/*
 * Copyright (c) 2014 Ryo Shimizu <ryo@nerv.org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * i.MX6,7 10/100/1000-Mbps ethernet MAC (ENET)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_enet.c,v 1.35 2022/01/24 09:14:37 andvar Exp $");

#include "vlan.h"

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/mbuf.h>
#include <sys/device.h>
#include <sys/sockio.h>
#include <sys/kernel.h>
#include <sys/rndsource.h>

#include <lib/libkern/libkern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_ether.h>
#include <net/bpf.h>
#include <net/if_vlanvar.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <arm/imx/if_enetreg.h>
#include <arm/imx/if_enetvar.h>

#undef DEBUG_ENET
#undef ENET_EVENT_COUNTER

#define ENET_TICK	hz

#ifdef DEBUG_ENET
int enet_debug = 0;
# define DEVICE_DPRINTF(args...)	\
	do { if (enet_debug) device_printf(sc->sc_dev, args); } while (0)
#else
# define DEVICE_DPRINTF(args...)
#endif


#define RXDESC_MAXBUFSIZE	0x07f0
				/* ENET does not work greather than 0x0800... */

#undef ENET_SUPPORT_JUMBO	/* JUMBO FRAME SUPPORT is unstable */
#ifdef ENET_SUPPORT_JUMBO
# define ENET_MAX_PKT_LEN	4034	/* MAX FIFO LEN */
#else
# define ENET_MAX_PKT_LEN	1522
#endif
#define ENET_DEFAULT_PKT_LEN	1522	/* including VLAN tag */
#define MTU2FRAMESIZE(n)	\
	((n) + ETHER_HDR_LEN + ETHER_CRC_LEN + ETHER_VLAN_ENCAP_LEN)


#define ENET_MAX_PKT_NSEGS	64

#define ENET_TX_NEXTIDX(idx)	\
	(((idx) >= (ENET_TX_RING_CNT - 1)) ? 0 : ((idx) + 1))
#define ENET_RX_NEXTIDX(idx)	\
	(((idx) >= (ENET_RX_RING_CNT - 1)) ? 0 : ((idx) + 1))

#define TXDESC_WRITEOUT(idx)					\
	bus_dmamap_sync(sc->sc_dmat, sc->sc_txdesc_dmamap,	\
	    sizeof(struct enet_txdesc) * (idx),			\
	    sizeof(struct enet_txdesc),				\
	    BUS_DMASYNC_PREWRITE)

#define TXDESC_READIN(idx)					\
	bus_dmamap_sync(sc->sc_dmat, sc->sc_txdesc_dmamap,	\
	    sizeof(struct enet_txdesc) * (idx),			\
	    sizeof(struct enet_txdesc),				\
	    BUS_DMASYNC_PREREAD)

#define RXDESC_WRITEOUT(idx)					\
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxdesc_dmamap,	\
	    sizeof(struct enet_rxdesc) * (idx),			\
	    sizeof(struct enet_rxdesc),				\
	    BUS_DMASYNC_PREWRITE)

#define RXDESC_READIN(idx)					\
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxdesc_dmamap,	\
	    sizeof(struct enet_rxdesc) * (idx),			\
	    sizeof(struct enet_rxdesc),				\
	    BUS_DMASYNC_PREREAD)

#define ENET_REG_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_iot, (sc)->sc_ioh, reg)

#define ENET_REG_WRITE(sc, reg, value)				\
	bus_space_write_4((sc)->sc_iot, (sc)->sc_ioh, reg, value)

#ifdef ENET_EVENT_COUNTER
static void enet_attach_evcnt(struct enet_softc *);
static void enet_update_evcnt(struct enet_softc *);
#endif

static void enet_tick(void *);
static int enet_tx_intr(void *);
static int enet_rx_intr(void *);
static void enet_rx_csum(struct enet_softc *, struct ifnet *, struct mbuf *,
			 int);

static void enet_start(struct ifnet *);
static int enet_ifflags_cb(struct ethercom *);
static int enet_ioctl(struct ifnet *, u_long, void *);
static int enet_init(struct ifnet *);
static void enet_stop(struct ifnet *, int);
static void enet_watchdog(struct ifnet *);
static void enet_mediastatus(struct ifnet *, struct ifmediareq *);

static int enet_miibus_readreg(device_t, int, int, uint16_t *);
static int enet_miibus_writereg(device_t, int, int, uint16_t);
static void enet_miibus_statchg(struct ifnet *);

static void enet_gethwaddr(struct enet_softc *, uint8_t *);
static void enet_sethwaddr(struct enet_softc *, uint8_t *);
static void enet_setmulti(struct enet_softc *);
static int enet_encap_mbufalign(struct mbuf **);
static int enet_encap_txring(struct enet_softc *, struct mbuf **);
static int enet_init_regs(struct enet_softc *, int);
static int enet_alloc_ring(struct enet_softc *);
static void enet_init_txring(struct enet_softc *);
static int enet_init_rxring(struct enet_softc *);
static void enet_reset_rxdesc(struct enet_softc *, int);
static int enet_alloc_rxbuf(struct enet_softc *, int);
static void enet_drain_txbuf(struct enet_softc *);
static void enet_drain_rxbuf(struct enet_softc *);
static int enet_alloc_dma(struct enet_softc *, size_t, void **,
			  bus_dmamap_t *);

int
enet_attach_common(device_t self)
{
	struct enet_softc *sc = device_private(self);
	struct ifnet *ifp;
	struct mii_data * const mii = &sc->sc_mii;

	/* allocate dma buffer */
	if (enet_alloc_ring(sc))
		return -1;

#define IS_ENADDR_ZERO(enaddr)				\
	((enaddr[0] | enaddr[1] | enaddr[2] |		\
	 enaddr[3] | enaddr[4] | enaddr[5]) == 0)

	if (IS_ENADDR_ZERO(sc->sc_enaddr)) {
		/* by any chance, mac-address is already set by bootloader? */
		enet_gethwaddr(sc, sc->sc_enaddr);
		if (IS_ENADDR_ZERO(sc->sc_enaddr)) {
			/* give up. set randomly */
			uint32_t eaddr = random();
			/* not multicast */
			sc->sc_enaddr[0] = (eaddr >> 24) & 0xfc;
			sc->sc_enaddr[1] = eaddr >> 16;
			sc->sc_enaddr[2] = eaddr >> 8;
			sc->sc_enaddr[3] = eaddr;
			eaddr = random();
			sc->sc_enaddr[4] = eaddr >> 8;
			sc->sc_enaddr[5] = eaddr;

			aprint_error_dev(self,
			    "cannot get mac address. set randomly\n");
		}
	}
	enet_sethwaddr(sc, sc->sc_enaddr);

	aprint_normal_dev(self, "Ethernet address %s\n",
	    ether_sprintf(sc->sc_enaddr));

	enet_init_regs(sc, 1);

	/* callout will be scheduled from enet_init() */
	callout_init(&sc->sc_tick_ch, 0);
	callout_setfunc(&sc->sc_tick_ch, enet_tick, sc);

	/* setup ifp */
	ifp = &sc->sc_ethercom.ec_if;
	strlcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_mtu = ETHERMTU;
	ifp->if_baudrate = IF_Gbps(1);
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_ioctl = enet_ioctl;
	ifp->if_start = enet_start;
	ifp->if_init = enet_init;
	ifp->if_stop = enet_stop;
	ifp->if_watchdog = enet_watchdog;

	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;
#ifdef ENET_SUPPORT_JUMBO
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_JUMBO_MTU;
#endif

	ifp->if_capabilities = IFCAP_CSUM_IPv4_Tx | IFCAP_CSUM_IPv4_Rx |
	    IFCAP_CSUM_TCPv4_Tx | IFCAP_CSUM_UDPv4_Tx |
	    IFCAP_CSUM_TCPv4_Rx | IFCAP_CSUM_UDPv4_Rx |
	    IFCAP_CSUM_TCPv6_Tx | IFCAP_CSUM_UDPv6_Tx |
	    IFCAP_CSUM_TCPv6_Rx | IFCAP_CSUM_UDPv6_Rx;

	IFQ_SET_MAXLEN(&ifp->if_snd, uimax(ENET_TX_RING_CNT, IFQ_MAXLEN));
	IFQ_SET_READY(&ifp->if_snd);

	/* setup MII */
	sc->sc_ethercom.ec_mii = mii;
	mii->mii_ifp = ifp;
	mii->mii_readreg = enet_miibus_readreg;
	mii->mii_writereg = enet_miibus_writereg;
	mii->mii_statchg = enet_miibus_statchg;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, enet_mediastatus);

	/* try to attach PHY */
	mii_attach(self, mii, 0xffffffff, sc->sc_phyid, MII_OFFSET_ANY, 0);
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER | IFM_MANUAL, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_MANUAL);
	} else {
		ifmedia_set(&mii->mii_media, IFM_ETHER | IFM_AUTO);
	}

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);
	ether_set_ifflags_cb(&sc->sc_ethercom, enet_ifflags_cb);

	rnd_attach_source(&sc->sc_rnd_source, device_xname(sc->sc_dev),
	    RND_TYPE_NET, RND_FLAG_DEFAULT);

#ifdef ENET_EVENT_COUNTER
	enet_attach_evcnt(sc);
#endif

	sc->sc_stopping = false;

	return 0;
}

#ifdef ENET_EVENT_COUNTER
static void
enet_attach_evcnt(struct enet_softc *sc)
{
	const char *xname;

	xname = device_xname(sc->sc_dev);

#define ENET_EVCNT_ATTACH(name)	\
	evcnt_attach_dynamic(&sc->sc_ev_ ## name, EVCNT_TYPE_MISC,	\
	    NULL, xname, #name);

	ENET_EVCNT_ATTACH(t_drop);
	ENET_EVCNT_ATTACH(t_packets);
	ENET_EVCNT_ATTACH(t_bc_pkt);
	ENET_EVCNT_ATTACH(t_mc_pkt);
	ENET_EVCNT_ATTACH(t_crc_align);
	ENET_EVCNT_ATTACH(t_undersize);
	ENET_EVCNT_ATTACH(t_oversize);
	ENET_EVCNT_ATTACH(t_frag);
	ENET_EVCNT_ATTACH(t_jab);
	ENET_EVCNT_ATTACH(t_col);
	ENET_EVCNT_ATTACH(t_p64);
	ENET_EVCNT_ATTACH(t_p65to127n);
	ENET_EVCNT_ATTACH(t_p128to255n);
	ENET_EVCNT_ATTACH(t_p256to511);
	ENET_EVCNT_ATTACH(t_p512to1023);
	ENET_EVCNT_ATTACH(t_p1024to2047);
	ENET_EVCNT_ATTACH(t_p_gte2048);
	ENET_EVCNT_ATTACH(t_octets);
	ENET_EVCNT_ATTACH(r_packets);
	ENET_EVCNT_ATTACH(r_bc_pkt);
	ENET_EVCNT_ATTACH(r_mc_pkt);
	ENET_EVCNT_ATTACH(r_crc_align);
	ENET_EVCNT_ATTACH(r_undersize);
	ENET_EVCNT_ATTACH(r_oversize);
	ENET_EVCNT_ATTACH(r_frag);
	ENET_EVCNT_ATTACH(r_jab);
	ENET_EVCNT_ATTACH(r_p64);
	ENET_EVCNT_ATTACH(r_p65to127);
	ENET_EVCNT_ATTACH(r_p128to255);
	ENET_EVCNT_ATTACH(r_p256to511);
	ENET_EVCNT_ATTACH(r_p512to1023);
	ENET_EVCNT_ATTACH(r_p1024to2047);
	ENET_EVCNT_ATTACH(r_p_gte2048);
	ENET_EVCNT_ATTACH(r_octets);
}

static void
enet_update_evcnt(struct enet_softc *sc)
{
	sc->sc_ev_t_drop.ev_count += ENET_REG_READ(sc, ENET_RMON_T_DROP);
	sc->sc_ev_t_packets.ev_count += ENET_REG_READ(sc, ENET_RMON_T_PACKETS);
	sc->sc_ev_t_bc_pkt.ev_count += ENET_REG_READ(sc, ENET_RMON_T_BC_PKT);
	sc->sc_ev_t_mc_pkt.ev_count += ENET_REG_READ(sc, ENET_RMON_T_MC_PKT);
	sc->sc_ev_t_crc_align.ev_count += ENET_REG_READ(sc, ENET_RMON_T_CRC_ALIGN);
	sc->sc_ev_t_undersize.ev_count += ENET_REG_READ(sc, ENET_RMON_T_UNDERSIZE);
	sc->sc_ev_t_oversize.ev_count += ENET_REG_READ(sc, ENET_RMON_T_OVERSIZE);
	sc->sc_ev_t_frag.ev_count += ENET_REG_READ(sc, ENET_RMON_T_FRAG);
	sc->sc_ev_t_jab.ev_count += ENET_REG_READ(sc, ENET_RMON_T_JAB);
	sc->sc_ev_t_col.ev_count += ENET_REG_READ(sc, ENET_RMON_T_COL);
	sc->sc_ev_t_p64.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P64);
	sc->sc_ev_t_p65to127n.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P65TO127N);
	sc->sc_ev_t_p128to255n.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P128TO255N);
	sc->sc_ev_t_p256to511.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P256TO511);
	sc->sc_ev_t_p512to1023.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P512TO1023);
	sc->sc_ev_t_p1024to2047.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P1024TO2047);
	sc->sc_ev_t_p_gte2048.ev_count += ENET_REG_READ(sc, ENET_RMON_T_P_GTE2048);
	sc->sc_ev_t_octets.ev_count += ENET_REG_READ(sc, ENET_RMON_T_OCTETS);
	sc->sc_ev_r_packets.ev_count += ENET_REG_READ(sc, ENET_RMON_R_PACKETS);
	sc->sc_ev_r_bc_pkt.ev_count += ENET_REG_READ(sc, ENET_RMON_R_BC_PKT);
	sc->sc_ev_r_mc_pkt.ev_count += ENET_REG_READ(sc, ENET_RMON_R_MC_PKT);
	sc->sc_ev_r_crc_align.ev_count += ENET_REG_READ(sc, ENET_RMON_R_CRC_ALIGN);
	sc->sc_ev_r_undersize.ev_count += ENET_REG_READ(sc, ENET_RMON_R_UNDERSIZE);
	sc->sc_ev_r_oversize.ev_count += ENET_REG_READ(sc, ENET_RMON_R_OVERSIZE);
	sc->sc_ev_r_frag.ev_count += ENET_REG_READ(sc, ENET_RMON_R_FRAG);
	sc->sc_ev_r_jab.ev_count += ENET_REG_READ(sc, ENET_RMON_R_JAB);
	sc->sc_ev_r_p64.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P64);
	sc->sc_ev_r_p65to127.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P65TO127);
	sc->sc_ev_r_p128to255.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P128TO255);
	sc->sc_ev_r_p256to511.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P256TO511);
	sc->sc_ev_r_p512to1023.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P512TO1023);
	sc->sc_ev_r_p1024to2047.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P1024TO2047);
	sc->sc_ev_r_p_gte2048.ev_count += ENET_REG_READ(sc, ENET_RMON_R_P_GTE2048);
	sc->sc_ev_r_octets.ev_count += ENET_REG_READ(sc, ENET_RMON_R_OCTETS);
}
#endif /* ENET_EVENT_COUNTER */

static void
enet_tick(void *arg)
{
	struct enet_softc *sc;
	struct mii_data *mii;
	struct ifnet *ifp;
	int s;

	sc = arg;
	mii = &sc->sc_mii;
	ifp = &sc->sc_ethercom.ec_if;

	s = splnet();

	if (sc->sc_stopping)
		goto out;

#ifdef ENET_EVENT_COUNTER
	enet_update_evcnt(sc);
#endif

	/* update counters */
	if_statadd(ifp, if_ierrors,
	    (uint64_t)ENET_REG_READ(sc, ENET_RMON_R_UNDERSIZE) +
	    (uint64_t)ENET_REG_READ(sc, ENET_RMON_R_FRAG) +
	    (uint64_t)ENET_REG_READ(sc, ENET_RMON_R_JAB));

	/* clear counters */
	ENET_REG_WRITE(sc, ENET_MIBC, ENET_MIBC_MIB_CLEAR);
	ENET_REG_WRITE(sc, ENET_MIBC, 0);

	mii_tick(mii);
 out:

	if (!sc->sc_stopping)
		callout_schedule(&sc->sc_tick_ch, ENET_TICK);

	splx(s);
}

int
enet_intr(void *arg)
{
	struct enet_softc *sc;
	struct ifnet *ifp;
	uint32_t status;

	sc = arg;
	status = ENET_REG_READ(sc, ENET_EIR);

	if (sc->sc_imxtype == 7) {
		if (status & (ENET_EIR_TXF | ENET_EIR_TXF1 | ENET_EIR_TXF2))
			enet_tx_intr(arg);
		if (status & (ENET_EIR_RXF | ENET_EIR_RXF1 | ENET_EIR_RXF2))
			enet_rx_intr(arg);
	} else {
		if (status & ENET_EIR_TXF)
			enet_tx_intr(arg);
		if (status & ENET_EIR_RXF)
			enet_rx_intr(arg);
	}

	if (status & ENET_EIR_EBERR) {
		device_printf(sc->sc_dev, "Ethernet Bus Error\n");
		ifp = &sc->sc_ethercom.ec_if;
		enet_stop(ifp, 1);
		enet_init(ifp);
	} else {
		ENET_REG_WRITE(sc, ENET_EIR, status);
	}

	rnd_add_uint32(&sc->sc_rnd_source, status);

	return 1;
}

static int
enet_tx_intr(void *arg)
{
	struct enet_softc *sc;
	struct ifnet *ifp;
	struct enet_txsoft *txs;
	int idx;

	sc = (struct enet_softc *)arg;
	ifp = &sc->sc_ethercom.ec_if;

	for (idx = sc->sc_tx_considx; idx != sc->sc_tx_prodidx;
	    idx = ENET_TX_NEXTIDX(idx)) {

		txs = &sc->sc_txsoft[idx];

		TXDESC_READIN(idx);
		if (sc->sc_txdesc_ring[idx].tx_flags1_len & TXFLAGS1_R) {
			/* This TX Descriptor has not been transmitted yet */
			break;
		}

		/* txsoft is available on first segment (TXFLAGS1_T1) */
		if (sc->sc_txdesc_ring[idx].tx_flags1_len & TXFLAGS1_T1) {
			bus_dmamap_unload(sc->sc_dmat,
			    txs->txs_dmamap);
			m_freem(txs->txs_mbuf);
			if_statinc(ifp, if_opackets);
		}

		/* checking error */
		if (sc->sc_txdesc_ring[idx].tx_flags1_len & TXFLAGS1_L) {
			uint32_t flags2;

			flags2 = sc->sc_txdesc_ring[idx].tx_flags2;

			if (flags2 & (TXFLAGS2_TXE |
			    TXFLAGS2_UE | TXFLAGS2_EE | TXFLAGS2_FE |
			    TXFLAGS2_LCE | TXFLAGS2_OE | TXFLAGS2_TSE)) {
#ifdef DEBUG_ENET
				if (enet_debug) {
					char flagsbuf[128];

					snprintb(flagsbuf, sizeof(flagsbuf),
					    "\20" "\20TRANSMIT" "\16UNDERFLOW"
					    "\15COLLISION" "\14FRAME"
					    "\13LATECOLLISION" "\12OVERFLOW",
					    flags2);

					device_printf(sc->sc_dev,
					    "txdesc[%d]: transmit error: "
					    "flags2=%s\n", idx, flagsbuf);
				}
#endif /* DEBUG_ENET */
				if_statinc(ifp, if_oerrors);
			}
		}

		sc->sc_tx_free++;
	}
	sc->sc_tx_considx = idx;

	if (sc->sc_tx_free > 0)
		ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * No more pending TX descriptor,
	 * cancel the watchdog timer.
	 */
	if (sc->sc_tx_free == ENET_TX_RING_CNT)
		ifp->if_timer = 0;

	return 1;
}

static int
enet_rx_intr(void *arg)
{
	struct enet_softc *sc;
	struct ifnet *ifp;
	struct enet_rxsoft *rxs;
	int idx, len, amount;
	uint32_t flags1, flags2;
	struct mbuf *m, *m0, *mprev;

	sc = arg;
	ifp = &sc->sc_ethercom.ec_if;

	m0 = mprev = NULL;
	amount = 0;
	for (idx = sc->sc_rx_readidx; ; idx = ENET_RX_NEXTIDX(idx)) {

		rxs = &sc->sc_rxsoft[idx];

		RXDESC_READIN(idx);
		if (sc->sc_rxdesc_ring[idx].rx_flags1_len & RXFLAGS1_E) {
			/* This RX Descriptor has not been received yet */
			break;
		}

		/*
		 * build mbuf from RX Descriptor if needed
		 */
		m = rxs->rxs_mbuf;
		rxs->rxs_mbuf = NULL;

		flags1 = sc->sc_rxdesc_ring[idx].rx_flags1_len;
		len = RXFLAGS1_LEN(flags1);

#define RACC_SHIFT16	2
		if (m0 == NULL) {
			m0 = m;
			m_adj(m0, RACC_SHIFT16);
			len -= RACC_SHIFT16;
			m->m_len = len;
			amount = len;
		} else {
			if (flags1 & RXFLAGS1_L)
				len = len - amount - RACC_SHIFT16;

			m->m_len = len;
			amount += len;
			if (m->m_flags & M_PKTHDR)
				m_remove_pkthdr(m);
			mprev->m_next = m;
		}
		mprev = m;

		flags2 = sc->sc_rxdesc_ring[idx].rx_flags2;

		if (flags1 & RXFLAGS1_L) {
			/* last buffer */
			if ((amount < ETHER_HDR_LEN) ||
			    ((flags1 & (RXFLAGS1_LG | RXFLAGS1_NO |
			    RXFLAGS1_CR | RXFLAGS1_OV | RXFLAGS1_TR)) ||
			    (flags2 & (RXFLAGS2_ME | RXFLAGS2_PE |
			    RXFLAGS2_CE)))) {

#ifdef DEBUG_ENET
				if (enet_debug) {
					char flags1buf[128], flags2buf[128];
					snprintb(flags1buf, sizeof(flags1buf),
					    "\20" "\31MISS" "\26LENGTHVIOLATION"
					    "\25NONOCTET" "\23CRC" "\22OVERRUN"
					    "\21TRUNCATED", flags1);
					snprintb(flags2buf, sizeof(flags2buf),
					    "\20" "\40MAC" "\33PHY"
					    "\32COLLISION", flags2);

					DEVICE_DPRINTF(
					    "rxdesc[%d]: receive error: "
					    "flags1=%s,flags2=%s,len=%d\n",
					    idx, flags1buf, flags2buf, amount);
				}
#endif /* DEBUG_ENET */
				if_statinc(ifp, if_ierrors);
				m_freem(m0);

			} else {
				/* packet receive ok */
				m_set_rcvif(m0, ifp);
				m0->m_pkthdr.len = amount;

				bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
				    rxs->rxs_dmamap->dm_mapsize,
				    BUS_DMASYNC_PREREAD);

				if (ifp->if_csum_flags_rx & (M_CSUM_IPv4 |
				    M_CSUM_TCPv4 | M_CSUM_UDPv4 |
				    M_CSUM_TCPv6 | M_CSUM_UDPv6))
					enet_rx_csum(sc, ifp, m0, idx);

				if_percpuq_enqueue(ifp->if_percpuq, m0);
			}

			m0 = NULL;
			mprev = NULL;
			amount = 0;

		} else {
			/* continued from previous buffer */
			bus_dmamap_sync(sc->sc_dmat, rxs->rxs_dmamap, 0,
			    rxs->rxs_dmamap->dm_mapsize, BUS_DMASYNC_PREREAD);
		}

		bus_dmamap_unload(sc->sc_dmat, rxs->rxs_dmamap);
		if (enet_alloc_rxbuf(sc, idx) != 0) {
			panic("enet_alloc_rxbuf NULL\n");
		}
	}
	sc->sc_rx_readidx = idx;

	/* re-enable RX DMA to make sure */
	ENET_REG_WRITE(sc, ENET_RDAR, ENET_RDAR_ACTIVE);

	return 1;
}

static void
enet_rx_csum(struct enet_softc *sc, struct ifnet *ifp, struct mbuf *m, int idx)
{
	uint32_t flags2;
	uint8_t proto;

	flags2 = sc->sc_rxdesc_ring[idx].rx_flags2;

	if (flags2 & RXFLAGS2_IPV6) {
		proto = sc->sc_rxdesc_ring[idx].rx_proto;

		/* RXFLAGS2_PCR is valid when IPv6 and TCP/UDP */
		if ((proto == IPPROTO_TCP) &&
		    (ifp->if_csum_flags_rx & M_CSUM_TCPv6))
			m->m_pkthdr.csum_flags |= M_CSUM_TCPv6;
		else if ((proto == IPPROTO_UDP) &&
		    (ifp->if_csum_flags_rx & M_CSUM_UDPv6))
			m->m_pkthdr.csum_flags |= M_CSUM_UDPv6;
		else
			return;

		/* IPv6 protocol checksum error */
		if (flags2 & RXFLAGS2_PCR)
			m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;

	} else {
		struct ether_header *eh;
		uint8_t *ip;

		eh = mtod(m, struct ether_header *);

		/* XXX: is an IPv4? */
		if (ntohs(eh->ether_type) != ETHERTYPE_IP)
			return;
		ip = (uint8_t *)(eh + 1);
		if ((ip[0] & 0xf0) == 0x40)
			return;

		proto = sc->sc_rxdesc_ring[idx].rx_proto;
		if (flags2 & RXFLAGS2_ICE) {
			if (ifp->if_csum_flags_rx & M_CSUM_IPv4) {
				m->m_pkthdr.csum_flags |=
				    M_CSUM_IPv4 | M_CSUM_IPv4_BAD;
			}
		} else {
			if (ifp->if_csum_flags_rx & M_CSUM_IPv4) {
				m->m_pkthdr.csum_flags |= M_CSUM_IPv4;
			}

			/*
			 * PCR is valid when
			 * ICE == 0 and FRAG == 0
			 */
			if (flags2 & RXFLAGS2_FRAG)
				return;

			/*
			 * PCR is valid when proto is TCP or UDP
			 */
			if ((proto == IPPROTO_TCP) &&
			    (ifp->if_csum_flags_rx & M_CSUM_TCPv4))
				m->m_pkthdr.csum_flags |= M_CSUM_TCPv4;
			else if ((proto == IPPROTO_UDP) &&
			    (ifp->if_csum_flags_rx & M_CSUM_UDPv4))
				m->m_pkthdr.csum_flags |= M_CSUM_UDPv4;
			else
				return;

			/* IPv4 protocol cksum error */
			if (flags2 & RXFLAGS2_PCR)
				m->m_pkthdr.csum_flags |= M_CSUM_TCP_UDP_BAD;
		}
	}
}

static void
enet_setmulti(struct enet_softc *sc)
{
	struct ethercom *ec = &sc->sc_ethercom;
	struct ifnet *ifp = &ec->ec_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	uint32_t crc, hashidx;
	uint32_t gaddr[2];

	if (ifp->if_flags & IFF_PROMISC) {
		/* receive all unicast packet */
		ENET_REG_WRITE(sc, ENET_IAUR, 0xffffffff);
		ENET_REG_WRITE(sc, ENET_IALR, 0xffffffff);
		/* receive all multicast packet */
		gaddr[0] = gaddr[1] = 0xffffffff;
	} else {
		gaddr[0] = gaddr[1] = 0;

		ETHER_LOCK(ec);
		ETHER_FIRST_MULTI(step, ec, enm);
		while (enm != NULL) {
			if (memcmp(enm->enm_addrlo, enm->enm_addrhi,
			    ETHER_ADDR_LEN)) {
				/*
				 * if specified by range, give up setting hash,
				 * and fallback to allmulti.
				 */
				gaddr[0] = gaddr[1] = 0xffffffff;
				break;
			}

			crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);
			hashidx = __SHIFTOUT(crc, __BITS(30,26));
			gaddr[__SHIFTOUT(crc, __BIT(31))] |= __BIT(hashidx);

			ETHER_NEXT_MULTI(step, enm);
		}
		ETHER_UNLOCK(ec);

		/* dont't receive any unicast packet (except own address) */
		ENET_REG_WRITE(sc, ENET_IAUR, 0);
		ENET_REG_WRITE(sc, ENET_IALR, 0);
	}

	if (gaddr[0] == 0xffffffff && gaddr[1] == 0xffffffff)
		ifp->if_flags |= IFF_ALLMULTI;
	else
		ifp->if_flags &= ~IFF_ALLMULTI;

	/* receive multicast packets according to multicast filter */
	ENET_REG_WRITE(sc, ENET_GAUR, gaddr[1]);
	ENET_REG_WRITE(sc, ENET_GALR, gaddr[0]);

}

static void
enet_gethwaddr(struct enet_softc *sc, uint8_t *hwaddr)
{
	uint32_t paddr;

	paddr = ENET_REG_READ(sc, ENET_PALR);
	hwaddr[0] = paddr >> 24;
	hwaddr[1] = paddr >> 16;
	hwaddr[2] = paddr >> 8;
	hwaddr[3] = paddr;

	paddr = ENET_REG_READ(sc, ENET_PAUR);
	hwaddr[4] = paddr >> 24;
	hwaddr[5] = paddr >> 16;
}

static void
enet_sethwaddr(struct enet_softc *sc, uint8_t *hwaddr)
{
	uint32_t paddr;

	paddr = (hwaddr[0] << 24) | (hwaddr[1] << 16) | (hwaddr[2] << 8) |
	    hwaddr[3];
	ENET_REG_WRITE(sc, ENET_PALR, paddr);
	paddr = (hwaddr[4] << 24) | (hwaddr[5] << 16);
	ENET_REG_WRITE(sc, ENET_PAUR, paddr);
}

/*
 * ifnet interfaces
 */
static int
enet_init(struct ifnet *ifp)
{
	struct enet_softc *sc;
	int s, error;

	sc = ifp->if_softc;

	s = splnet();

	enet_init_regs(sc, 0);
	enet_init_txring(sc);
	error = enet_init_rxring(sc);
	if (error != 0) {
		enet_drain_rxbuf(sc);
		device_printf(sc->sc_dev, "Cannot allocate mbuf cluster\n");
		goto init_failure;
	}

	/* reload mac address */
	memcpy(sc->sc_enaddr, CLLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
	enet_sethwaddr(sc, sc->sc_enaddr);

	/* program multicast address */
	enet_setmulti(sc);

	/* update if_flags */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* update local copy of if_flags */
	sc->sc_if_flags = ifp->if_flags;

	/* mii */
	mii_mediachg(&sc->sc_mii);

	/* enable RX DMA */
	ENET_REG_WRITE(sc, ENET_RDAR, ENET_RDAR_ACTIVE);

	sc->sc_stopping = false;
	callout_schedule(&sc->sc_tick_ch, ENET_TICK);

 init_failure:
	splx(s);

	return error;
}

static void
enet_start(struct ifnet *ifp)
{
	struct enet_softc *sc;
	struct mbuf *m;
	int npkt;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	sc = ifp->if_softc;
	for (npkt = 0; ; npkt++) {
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		if (sc->sc_tx_free <= 0) {
			/* no tx descriptor now... */
			ifp->if_flags |= IFF_OACTIVE;
			DEVICE_DPRINTF("TX descriptor is full\n");
			break;
		}

		IFQ_DEQUEUE(&ifp->if_snd, m);

		if (enet_encap_txring(sc, &m) != 0) {
			/* too many mbuf chains? */
			ifp->if_flags |= IFF_OACTIVE;
			DEVICE_DPRINTF(
			    "TX descriptor is full. dropping packet\n");
			m_freem(m);
			if_statinc(ifp, if_oerrors);
			break;
		}

		/* Pass the packet to any BPF listeners */
		bpf_mtap(ifp, m, BPF_D_OUT);
	}

	if (npkt) {
		/* enable TX DMA */
		ENET_REG_WRITE(sc, ENET_TDAR, ENET_TDAR_ACTIVE);

		ifp->if_timer = 5;
	}
}

static void
enet_stop(struct ifnet *ifp, int disable)
{
	struct enet_softc *sc;
	int s;
	uint32_t v;

	sc = ifp->if_softc;

	s = splnet();

	sc->sc_stopping = true;
	callout_stop(&sc->sc_tick_ch);

	/* clear ENET_ECR[ETHEREN] to abort receive and transmit */
	v = ENET_REG_READ(sc, ENET_ECR);
	ENET_REG_WRITE(sc, ENET_ECR, v & ~ENET_ECR_ETHEREN);

	/* Mark the interface as down and cancel the watchdog timer. */
	ifp->if_flags &= ~(IFF_RUNNING | IFF_OACTIVE);
	ifp->if_timer = 0;

	if (disable) {
		enet_drain_txbuf(sc);
		enet_drain_rxbuf(sc);
	}

	splx(s);
}

static void
enet_watchdog(struct ifnet *ifp)
{
	struct enet_softc *sc;
	int s;

	sc = ifp->if_softc;
	s = splnet();

	device_printf(sc->sc_dev, "watchdog timeout\n");
	if_statinc(ifp, if_oerrors);

	/* salvage packets left in descriptors */
	enet_tx_intr(sc);
	enet_rx_intr(sc);

	/* reset */
	enet_stop(ifp, 1);
	enet_init(ifp);

	splx(s);
}

static void
enet_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct enet_softc *sc = ifp->if_softc;

	ether_mediastatus(ifp, ifmr);
	ifmr->ifm_active = (ifmr->ifm_active & ~IFM_ETH_FMASK)
	    | sc->sc_flowflags;
}

static int
enet_ifflags_cb(struct ethercom *ec)
{
	struct ifnet *ifp = &ec->ec_if;
	struct enet_softc *sc = ifp->if_softc;
	u_short change = ifp->if_flags ^ sc->sc_if_flags;

	if ((change & ~(IFF_CANTCHANGE | IFF_DEBUG)) != 0)
		return ENETRESET;
	else if ((change & (IFF_PROMISC | IFF_ALLMULTI)) == 0)
		return 0;

	enet_setmulti(sc);

	sc->sc_if_flags = ifp->if_flags;
	return 0;
}

static int
enet_ioctl(struct ifnet *ifp, u_long command, void *data)
{
	struct enet_softc *sc;
	struct ifreq *ifr;
	int s, error;
	uint32_t v;

	sc = ifp->if_softc;
	ifr = data;

	error = 0;

	s = splnet();

	switch (command) {
	case SIOCSIFMTU:
		if (MTU2FRAMESIZE(ifr->ifr_mtu) > ENET_MAX_PKT_LEN) {
			error = EINVAL;
		} else {
			ifp->if_mtu = ifr->ifr_mtu;

			/* set maximum frame length */
			v = MTU2FRAMESIZE(ifr->ifr_mtu);
			ENET_REG_WRITE(sc, ENET_FTRL, v);
			v = ENET_REG_READ(sc, ENET_RCR);
			v &= ~ENET_RCR_MAX_FL(0x3fff);
			v |= ENET_RCR_MAX_FL(ifp->if_mtu + ETHER_HDR_LEN +
			    ETHER_CRC_LEN + ETHER_VLAN_ENCAP_LEN);
			ENET_REG_WRITE(sc, ENET_RCR, v);
		}
		break;
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
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_mii.mii_media, command);
		break;
	default:
		error = ether_ioctl(ifp, command, data);
		if (error != ENETRESET)
			break;

		/* post-process */
		error = 0;
		switch (command) {
		case SIOCSIFCAP:
			error = if_init(ifp);
			break;
		case SIOCADDMULTI:
		case SIOCDELMULTI:
			if (ifp->if_flags & IFF_RUNNING)
				enet_setmulti(sc);
			break;
		}
		break;
	}

	splx(s);

	return error;
}

/*
 * for MII
 */
static int
enet_miibus_readreg(device_t dev, int phy, int reg, uint16_t *val)
{
	struct enet_softc *sc;
	int timeout;
	uint32_t status;

	sc = device_private(dev);

	/* clear MII update */
	ENET_REG_WRITE(sc, ENET_EIR, ENET_EIR_MII);

	/* read command */
	ENET_REG_WRITE(sc, ENET_MMFR,
	    ENET_MMFR_ST | ENET_MMFR_OP_READ | ENET_MMFR_TA |
	    ENET_MMFR_PHY_REG(reg) | ENET_MMFR_PHY_ADDR(phy));

	/* check MII update */
	for (timeout = 5000; timeout > 0; --timeout) {
		status = ENET_REG_READ(sc, ENET_EIR);
		if (status & ENET_EIR_MII)
			break;
	}
	if (timeout <= 0) {
		DEVICE_DPRINTF("MII read timeout: reg=0x%02x\n",
		    reg);
		return ETIMEDOUT;
	} else
		*val = ENET_REG_READ(sc, ENET_MMFR) & ENET_MMFR_DATAMASK;

	return 0;
}

static int
enet_miibus_writereg(device_t dev, int phy, int reg, uint16_t val)
{
	struct enet_softc *sc;
	int timeout;

	sc = device_private(dev);

	/* clear MII update */
	ENET_REG_WRITE(sc, ENET_EIR, ENET_EIR_MII);

	/* write command */
	ENET_REG_WRITE(sc, ENET_MMFR,
	    ENET_MMFR_ST | ENET_MMFR_OP_WRITE | ENET_MMFR_TA |
	    ENET_MMFR_PHY_REG(reg) | ENET_MMFR_PHY_ADDR(phy) |
	    (ENET_MMFR_DATAMASK & val));

	/* check MII update */
	for (timeout = 5000; timeout > 0; --timeout) {
		if (ENET_REG_READ(sc, ENET_EIR) & ENET_EIR_MII)
			break;
	}
	if (timeout <= 0) {
		DEVICE_DPRINTF("MII write timeout: reg=0x%02x\n", reg);
		return ETIMEDOUT;
	}

	return 0;
}

static void
enet_miibus_statchg(struct ifnet *ifp)
{
	struct enet_softc *sc;
	struct mii_data *mii;
	struct ifmedia_entry *ife;
	uint32_t ecr, ecr0;
	uint32_t rcr, rcr0;
	uint32_t tcr, tcr0;

	sc = ifp->if_softc;
	mii = &sc->sc_mii;
	ife = mii->mii_media.ifm_cur;

	/* get current status */
	ecr0 = ecr = ENET_REG_READ(sc, ENET_ECR) & ~ENET_ECR_RESET;
	rcr0 = rcr = ENET_REG_READ(sc, ENET_RCR);
	tcr0 = tcr = ENET_REG_READ(sc, ENET_TCR);

	if (IFM_SUBTYPE(mii->mii_media.ifm_cur->ifm_media) == IFM_AUTO &&
	    (mii->mii_media_active & IFM_ETH_FMASK) != sc->sc_flowflags) {
		sc->sc_flowflags = mii->mii_media_active & IFM_ETH_FMASK;
		mii->mii_media_active &= ~IFM_ETH_FMASK;
	}

	if ((ife->ifm_media & IFM_FDX) != 0) {
		tcr |= ENET_TCR_FDEN;	/* full duplex */
		rcr &= ~ENET_RCR_DRT;	/* enable receive on transmit */
	} else {
		tcr &= ~ENET_TCR_FDEN;	/* half duplex */
		rcr |= ENET_RCR_DRT;	/* disable receive on transmit */
	}

	if ((tcr ^ tcr0) & ENET_TCR_FDEN) {
		/*
		 * need to reset because
		 * FDEN can change when ECR[ETHEREN] is 0
		 */
		enet_init_regs(sc, 0);
		return;
	}

	switch (IFM_SUBTYPE(ife->ifm_media)) {
	case IFM_AUTO:
	case IFM_1000_T:
		ecr |= ENET_ECR_SPEED;		/* 1000Mbps mode */
		rcr &= ~ENET_RCR_RMII_10T;
		break;
	case IFM_100_TX:
		ecr &= ~ENET_ECR_SPEED;		/* 100Mbps mode */
		rcr &= ~ENET_RCR_RMII_10T;	/* 100Mbps mode */
		break;
	case IFM_10_T:
		ecr &= ~ENET_ECR_SPEED;		/* 10Mbps mode */
		rcr |= ENET_RCR_RMII_10T;	/* 10Mbps mode */
		break;
	default:
		ecr = ecr0;
		rcr = rcr0;
		tcr = tcr0;
		break;
	}

	if (sc->sc_rgmii == 0)
		ecr &= ~ENET_ECR_SPEED;

	if (sc->sc_flowflags & IFM_FLOW)
		rcr |= ENET_RCR_FCE;
	else
		rcr &= ~ENET_RCR_FCE;

	/* update registers if need change */
	if (ecr != ecr0)
		ENET_REG_WRITE(sc, ENET_ECR, ecr);
	if (rcr != rcr0)
		ENET_REG_WRITE(sc, ENET_RCR, rcr);
	if (tcr != tcr0)
		ENET_REG_WRITE(sc, ENET_TCR, tcr);
}

/*
 * handling descriptors
 */
static void
enet_init_txring(struct enet_softc *sc)
{
	int i;

	/* build TX ring */
	for (i = 0; i < ENET_TX_RING_CNT; i++) {
		sc->sc_txdesc_ring[i].tx_flags1_len =
		    ((i == (ENET_TX_RING_CNT - 1)) ? TXFLAGS1_W : 0);
		sc->sc_txdesc_ring[i].tx_databuf = 0;
		sc->sc_txdesc_ring[i].tx_flags2 = TXFLAGS2_INT;
		sc->sc_txdesc_ring[i].tx__reserved1 = 0;
		sc->sc_txdesc_ring[i].tx_flags3 = 0;
		sc->sc_txdesc_ring[i].tx_1588timestamp = 0;
		sc->sc_txdesc_ring[i].tx__reserved2 = 0;
		sc->sc_txdesc_ring[i].tx__reserved3 = 0;

		TXDESC_WRITEOUT(i);
	}

	sc->sc_tx_free = ENET_TX_RING_CNT;
	sc->sc_tx_considx = 0;
	sc->sc_tx_prodidx = 0;
}

static int
enet_init_rxring(struct enet_softc *sc)
{
	int i, error;

	/* build RX ring */
	for (i = 0; i < ENET_RX_RING_CNT; i++) {
		error = enet_alloc_rxbuf(sc, i);
		if (error != 0)
			return error;
	}

	sc->sc_rx_readidx = 0;

	return 0;
}

static int
enet_alloc_rxbuf(struct enet_softc *sc, int idx)
{
	struct mbuf *m;
	int error;

	KASSERT((idx >= 0) && (idx < ENET_RX_RING_CNT));

	/* free mbuf if already allocated */
	if (sc->sc_rxsoft[idx].rxs_mbuf != NULL) {
		bus_dmamap_unload(sc->sc_dmat, sc->sc_rxsoft[idx].rxs_dmamap);
		m_freem(sc->sc_rxsoft[idx].rxs_mbuf);
		sc->sc_rxsoft[idx].rxs_mbuf = NULL;
	}

	/* allocate new mbuf cluster */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return ENOBUFS;
	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		m_freem(m);
		return ENOBUFS;
	}
	m->m_len = MCLBYTES;
	m->m_next = NULL;

	error = bus_dmamap_load(sc->sc_dmat, sc->sc_rxsoft[idx].rxs_dmamap,
	    m->m_ext.ext_buf, m->m_ext.ext_size, NULL,
	    BUS_DMA_READ | BUS_DMA_NOWAIT);
	if (error) {
		m_freem(m);
		return error;
	}

	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxsoft[idx].rxs_dmamap, 0,
	    sc->sc_rxsoft[idx].rxs_dmamap->dm_mapsize,
	    BUS_DMASYNC_PREREAD | BUS_DMASYNC_PREWRITE);

	sc->sc_rxsoft[idx].rxs_mbuf = m;
	enet_reset_rxdesc(sc, idx);
	return 0;
}

static void
enet_reset_rxdesc(struct enet_softc *sc, int idx)
{
	uint32_t paddr;

	paddr = sc->sc_rxsoft[idx].rxs_dmamap->dm_segs[0].ds_addr;

	sc->sc_rxdesc_ring[idx].rx_flags1_len =
	    RXFLAGS1_E |
	    ((idx == (ENET_RX_RING_CNT - 1)) ? RXFLAGS1_W : 0);
	sc->sc_rxdesc_ring[idx].rx_databuf = paddr;
	sc->sc_rxdesc_ring[idx].rx_flags2 =
	    RXFLAGS2_INT;
	sc->sc_rxdesc_ring[idx].rx_hl = 0;
	sc->sc_rxdesc_ring[idx].rx_proto = 0;
	sc->sc_rxdesc_ring[idx].rx_cksum = 0;
	sc->sc_rxdesc_ring[idx].rx_flags3 = 0;
	sc->sc_rxdesc_ring[idx].rx_1588timestamp = 0;
	sc->sc_rxdesc_ring[idx].rx__reserved2 = 0;
	sc->sc_rxdesc_ring[idx].rx__reserved3 = 0;

	RXDESC_WRITEOUT(idx);
}

static void
enet_drain_txbuf(struct enet_softc *sc)
{
	int idx;
	struct enet_txsoft *txs;
	struct ifnet *ifp;

	ifp = &sc->sc_ethercom.ec_if;

	for (idx = sc->sc_tx_considx; idx != sc->sc_tx_prodidx;
	    idx = ENET_TX_NEXTIDX(idx)) {

		/* txsoft[] is used only first segment */
		txs = &sc->sc_txsoft[idx];
		TXDESC_READIN(idx);
		if (sc->sc_txdesc_ring[idx].tx_flags1_len & TXFLAGS1_T1) {
			sc->sc_txdesc_ring[idx].tx_flags1_len = 0;
			bus_dmamap_unload(sc->sc_dmat,
			    txs->txs_dmamap);
			m_freem(txs->txs_mbuf);

			if_statinc(ifp, if_oerrors);
		}
		sc->sc_tx_free++;
	}
}

static void
enet_drain_rxbuf(struct enet_softc *sc)
{
	int i;

	for (i = 0; i < ENET_RX_RING_CNT; i++) {
		if (sc->sc_rxsoft[i].rxs_mbuf != NULL) {
			sc->sc_rxdesc_ring[i].rx_flags1_len = 0;
			bus_dmamap_unload(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
			m_freem(sc->sc_rxsoft[i].rxs_mbuf);
			sc->sc_rxsoft[i].rxs_mbuf = NULL;
		}
	}
}

static int
enet_alloc_ring(struct enet_softc *sc)
{
	int i, error;

	/*
	 * build DMA maps for TX.
	 * TX descriptor must be able to contain mbuf chains,
	 * so, make up ENET_MAX_PKT_NSEGS dmamap.
	 */
	for (i = 0; i < ENET_TX_RING_CNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, ENET_MAX_PKT_LEN,
		    ENET_MAX_PKT_NSEGS, ENET_MAX_PKT_LEN, 0, BUS_DMA_NOWAIT,
		    &sc->sc_txsoft[i].txs_dmamap);

		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "can't create DMA map for TX descs\n");
			goto fail_1;
		}
	}

	/*
	 * build DMA maps for RX.
	 * RX descripter contains An mbuf cluster,
	 * and make up a dmamap.
	 */
	for (i = 0; i < ENET_RX_RING_CNT; i++) {
		error = bus_dmamap_create(sc->sc_dmat, MCLBYTES,
		    1, MCLBYTES, 0, BUS_DMA_NOWAIT,
		    &sc->sc_rxsoft[i].rxs_dmamap);
		if (error) {
			aprint_error_dev(sc->sc_dev,
			    "can't create DMA map for RX descs\n");
			goto fail_2;
		}
	}

	if (enet_alloc_dma(sc, sizeof(struct enet_txdesc) * ENET_TX_RING_CNT,
	    (void **)&(sc->sc_txdesc_ring), &(sc->sc_txdesc_dmamap)) != 0)
		return -1;
	memset(sc->sc_txdesc_ring, 0,
	    sizeof(struct enet_txdesc) * ENET_TX_RING_CNT);

	if (enet_alloc_dma(sc, sizeof(struct enet_rxdesc) * ENET_RX_RING_CNT,
	    (void **)&(sc->sc_rxdesc_ring), &(sc->sc_rxdesc_dmamap)) != 0)
		return -1;
	memset(sc->sc_rxdesc_ring, 0,
	    sizeof(struct enet_rxdesc) * ENET_RX_RING_CNT);

	return 0;

 fail_2:
	for (i = 0; i < ENET_RX_RING_CNT; i++) {
		if (sc->sc_rxsoft[i].rxs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_rxsoft[i].rxs_dmamap);
	}
 fail_1:
	for (i = 0; i < ENET_TX_RING_CNT; i++) {
		if (sc->sc_txsoft[i].txs_dmamap != NULL)
			bus_dmamap_destroy(sc->sc_dmat,
			    sc->sc_txsoft[i].txs_dmamap);
	}
	return error;
}

static int
enet_encap_mbufalign(struct mbuf **mp)
{
	struct mbuf *m, *m0, *mt, *p, *x;
	void *ap;
	uint32_t alignoff, chiplen;

	/*
	 * iMX6 SoC ethernet controller requires
	 * address of buffer must aligned 8, and
	 * length of buffer must be greater than 10 (first fragment only?)
	 */
#define ALIGNBYTE	8
#define MINBUFSIZE	10
#define ALIGN_PTR(p, align)	\
	(void *)(((uintptr_t)(p) + ((align) - 1)) & -(align))

	m0 = *mp;
	mt = p = NULL;
	for (m = m0; m != NULL; m = m->m_next) {
		alignoff = (uintptr_t)m->m_data & (ALIGNBYTE - 1);
		if (m->m_len < (ALIGNBYTE * 2)) {
			/*
			 * rearrange mbuf data aligned
			 *
			 *        align 8 *       *       *       *       *
			 *               +0123456789abcdef0123456789abcdef0
			 * FROM m->m_data[___________abcdefghijklmn_______]
			 *
			 *               +0123456789abcdef0123456789abcdef0
			 * TO   m->m_data[________abcdefghijklm___________] or
			 *      m->m_data[________________abcdefghijklmn__]
			 */
			if ((alignoff != 0) && (m->m_len != 0)) {
				chiplen = ALIGNBYTE - alignoff;
				if (M_LEADINGSPACE(m) >= alignoff) {
					ap = m->m_data - alignoff;
					memmove(ap, m->m_data, m->m_len);
					m->m_data = ap;
				} else if (M_TRAILINGSPACE(m) >= chiplen) {
					ap = m->m_data + chiplen;
					memmove(ap, m->m_data, m->m_len);
					m->m_data = ap;
				} else {
					/*
					 * no space to align data. (M_READONLY?)
					 * allocate new mbuf aligned,
					 * and copy to it.
					 */
					MGET(x, M_DONTWAIT, m->m_type);
					if (x == NULL) {
						m_freem(m);
						return ENOBUFS;
					}
					MCLAIM(x, m->m_owner);
					if (m->m_flags & M_PKTHDR)
						m_move_pkthdr(x, m);
					x->m_len = m->m_len;
					x->m_data = ALIGN_PTR(x->m_data,
					    ALIGNBYTE);
					memcpy(mtod(x, void *), mtod(m, void *),
					    m->m_len);
					p->m_next = x;
					x->m_next = m_free(m);
					m = x;
				}
			}

			/*
			 * fill 1st mbuf at least 10byte
			 *
			 *        align 8 *       *       *       *       *
			 *               +0123456789abcdef0123456789abcdef0
			 * FROM m->m_data[________abcde___________________]
			 *      m->m_data[__fg____________________________]
			 *      m->m_data[_________________hi_____________]
			 *      m->m_data[__________jk____________________]
			 *      m->m_data[____l___________________________]
			 *
			 *               +0123456789abcdef0123456789abcdef0
			 * TO   m->m_data[________abcdefghij______________]
			 *      m->m_data[________________________________]
			 *      m->m_data[________________________________]
			 *      m->m_data[___________k____________________]
			 *      m->m_data[____l___________________________]
			 */
			if (mt == NULL) {
				mt = m;
				while (mt->m_len == 0) {
					mt = mt->m_next;
					if (mt == NULL) {
						m_freem(m);
						return ENOBUFS;
					}
				}

				/* mt = 1st mbuf, x = 2nd mbuf */
				x = mt->m_next;
				while (mt->m_len < MINBUFSIZE) {
					if (x == NULL) {
						m_freem(m);
						return ENOBUFS;
					}

					alignoff = (uintptr_t)x->m_data &
					    (ALIGNBYTE - 1);
					chiplen = ALIGNBYTE - alignoff;
					if (chiplen > x->m_len) {
						chiplen = x->m_len;
					} else if ((mt->m_len + chiplen) <
					    MINBUFSIZE) {
						/*
						 * next mbuf should be greater
						 * than ALIGNBYTE?
						 */
						if (x->m_len >= (chiplen +
						    ALIGNBYTE * 2))
							chiplen += ALIGNBYTE;
						else
							chiplen = x->m_len;
					}

					if (chiplen &&
					    (M_TRAILINGSPACE(mt) < chiplen)) {
						/*
						 * move data to the beginning of
						 * m_dat[] (aligned) to en-
						 * large trailingspace
						 */
						ap = M_BUFADDR(mt);
						ap = ALIGN_PTR(ap, ALIGNBYTE);
						memcpy(ap, mt->m_data,
						    mt->m_len);
						mt->m_data = ap;
					}

					if (chiplen &&
					    (M_TRAILINGSPACE(mt) >= chiplen)) {
						memcpy(mt->m_data + mt->m_len,
						    x->m_data, chiplen);
						mt->m_len += chiplen;
						m_adj(x, chiplen);
					}

					x = x->m_next;
				}
			}

		} else {
			mt = m;

			/*
			 * allocate new mbuf x, and rearrange as below;
			 *
			 *        align 8 *       *       *       *       *
			 *               +0123456789abcdef0123456789abcdef0
			 * FROM m->m_data[____________abcdefghijklmnopq___]
			 *
			 *               +0123456789abcdef0123456789abcdef0
			 * TO   x->m_data[________abcdefghijkl____________]
			 *      m->m_data[________________________mnopq___]
			 *
			 */
			if (alignoff != 0) {
				/* at least ALIGNBYTE */
				chiplen = ALIGNBYTE - alignoff + ALIGNBYTE;

				MGET(x, M_DONTWAIT, m->m_type);
				if (x == NULL) {
					m_freem(m);
					return ENOBUFS;
				}
				MCLAIM(x, m->m_owner);
				if (m->m_flags & M_PKTHDR)
					m_move_pkthdr(x, m);
				x->m_data = ALIGN_PTR(x->m_data, ALIGNBYTE);
				memcpy(mtod(x, void *), mtod(m, void *),
				    chiplen);
				x->m_len = chiplen;
				x->m_next = m;
				m_adj(m, chiplen);

				if (p == NULL)
					m0 = x;
				else
					p->m_next = x;
			}
		}
		p = m;
	}
	*mp = m0;

	return 0;
}

static int
enet_encap_txring(struct enet_softc *sc, struct mbuf **mp)
{
	bus_dmamap_t map;
	struct mbuf *m;
	int csumflags, idx, i, error;
	uint32_t flags1, flags2;

	idx = sc->sc_tx_prodidx;
	map = sc->sc_txsoft[idx].txs_dmamap;

	/* align mbuf data for claim of ENET */
	error = enet_encap_mbufalign(mp);
	if (error != 0)
		return error;

	m = *mp;
	csumflags = m->m_pkthdr.csum_flags;

	error = bus_dmamap_load_mbuf(sc->sc_dmat, map, m,
	    BUS_DMA_NOWAIT);
	if (error != 0) {
		device_printf(sc->sc_dev,
		    "Error mapping mbuf into TX chain: error=%d\n", error);
		m_freem(m);
		return error;
	}

	if (map->dm_nsegs > sc->sc_tx_free) {
		bus_dmamap_unload(sc->sc_dmat, map);
		device_printf(sc->sc_dev,
		    "too many mbuf chain %d\n", map->dm_nsegs);
		m_freem(m);
		return ENOBUFS;
	}

	/* fill protocol cksum zero beforehand */
	if (csumflags & (M_CSUM_UDPv4 | M_CSUM_TCPv4 |
	    M_CSUM_UDPv6 | M_CSUM_TCPv6)) {
		int ehlen;
		uint16_t etype;

		m_copydata(m, ETHER_ADDR_LEN * 2, sizeof(etype), &etype);
		switch (ntohs(etype)) {
		case ETHERTYPE_IP:
		case ETHERTYPE_IPV6:
			ehlen = ETHER_HDR_LEN;
			break;
		case ETHERTYPE_VLAN:
			ehlen = ETHER_HDR_LEN + ETHER_VLAN_ENCAP_LEN;
			break;
		default:
			ehlen = 0;
			break;
		}

		if (ehlen) {
			const int off =
			    M_CSUM_DATA_IPv4_IPHL(m->m_pkthdr.csum_data) +
			    M_CSUM_DATA_IPv4_OFFSET(m->m_pkthdr.csum_data);
			if (m->m_pkthdr.len >= ehlen + off + sizeof(uint16_t)) {
				uint16_t zero = 0;
				m_copyback(m, ehlen + off, sizeof(zero), &zero);
			}
		}
	}

	bus_dmamap_sync(sc->sc_dmat, map, 0, map->dm_mapsize,
	    BUS_DMASYNC_PREWRITE);

	for (i = 0; i < map->dm_nsegs; i++) {
		flags1 = TXFLAGS1_R;
		flags2 = 0;

		if (i == 0) {
			flags1 |= TXFLAGS1_T1;	/* mark as first segment */
			sc->sc_txsoft[idx].txs_mbuf = m;
		}

		/* checksum offloading */
		if (csumflags & (M_CSUM_UDPv4 | M_CSUM_TCPv4 |
		    M_CSUM_UDPv6 | M_CSUM_TCPv6))
			flags2 |= TXFLAGS2_PINS;
		if (csumflags & (M_CSUM_IPv4))
			flags2 |= TXFLAGS2_IINS;

		if (i == map->dm_nsegs - 1) {
			/* mark last segment */
			flags1 |= TXFLAGS1_L | TXFLAGS1_TC;
			flags2 |= TXFLAGS2_INT;
		}
		if (idx == ENET_TX_RING_CNT - 1) {
			/* mark end of ring */
			flags1 |= TXFLAGS1_W;
		}

		sc->sc_txdesc_ring[idx].tx_databuf = map->dm_segs[i].ds_addr;
		sc->sc_txdesc_ring[idx].tx_flags2 = flags2;
		sc->sc_txdesc_ring[idx].tx_flags3 = 0;
		TXDESC_WRITEOUT(idx);

		sc->sc_txdesc_ring[idx].tx_flags1_len =
		    flags1 | TXFLAGS1_LEN(map->dm_segs[i].ds_len);
		TXDESC_WRITEOUT(idx);

		idx = ENET_TX_NEXTIDX(idx);
		sc->sc_tx_free--;
	}

	sc->sc_tx_prodidx = idx;

	return 0;
}

/*
 * device initialize
 */
static int
enet_init_regs(struct enet_softc *sc, int init)
{
	struct mii_data *mii;
	struct ifmedia_entry *ife;
	paddr_t paddr;
	uint32_t val;
	int miimode, fulldup, ecr_speed, rcr_speed, flowctrl;

	if (init) {
		fulldup = 1;
		ecr_speed = ENET_ECR_SPEED;
		rcr_speed = 0;
		flowctrl = 0;
	} else {
		mii = &sc->sc_mii;
		ife = mii->mii_media.ifm_cur;

		if ((ife->ifm_media & IFM_FDX) != 0)
			fulldup = 1;
		else
			fulldup = 0;

		switch (IFM_SUBTYPE(ife->ifm_media)) {
		case IFM_10_T:
			ecr_speed = 0;
			rcr_speed = ENET_RCR_RMII_10T;
			break;
		case IFM_100_TX:
			ecr_speed = 0;
			rcr_speed = 0;
			break;
		default:
			ecr_speed = ENET_ECR_SPEED;
			rcr_speed = 0;
			break;
		}

		flowctrl = sc->sc_flowflags & IFM_FLOW;
	}

	if (sc->sc_rgmii == 0)
		ecr_speed = 0;

	/* reset */
	ENET_REG_WRITE(sc, ENET_ECR, ecr_speed | ENET_ECR_RESET);

	/* mask and clear all interrupt */
	ENET_REG_WRITE(sc, ENET_EIMR, 0);
	ENET_REG_WRITE(sc, ENET_EIR, 0xffffffff);

	/* full duplex */
	ENET_REG_WRITE(sc, ENET_TCR, fulldup ? ENET_TCR_FDEN : 0);

	/* clear and enable MIB register */
	ENET_REG_WRITE(sc, ENET_MIBC, ENET_MIBC_MIB_CLEAR);
	ENET_REG_WRITE(sc, ENET_MIBC, 0);

	/* MII speed setup. MDCclk(=2.5MHz) = (internal module clock)/((val+1)*2) */
	val = (sc->sc_clock + (5000000 - 1)) / 5000000 - 1;
	ENET_REG_WRITE(sc, ENET_MSCR, __SHIFTIN(val, ENET_MSCR_MII_SPEED));

	/* Opcode/Pause Duration */
	ENET_REG_WRITE(sc, ENET_OPD, 0x00010020);

	/* Receive FIFO */
	ENET_REG_WRITE(sc, ENET_RSFL, 16);	/* RxFIFO Section Full */
	ENET_REG_WRITE(sc, ENET_RSEM, 0x84);	/* RxFIFO Section Empty */
	ENET_REG_WRITE(sc, ENET_RAEM, 8);	/* RxFIFO Almost Empty */
	ENET_REG_WRITE(sc, ENET_RAFL, 8);	/* RxFIFO Almost Full */

	/* Transmit FIFO */
	ENET_REG_WRITE(sc, ENET_TFWR, ENET_TFWR_STRFWD |
	    ENET_TFWR_FIFO(128));		/* TxFIFO Watermark */
	ENET_REG_WRITE(sc, ENET_TSEM, 0);	/* TxFIFO Section Empty */
	ENET_REG_WRITE(sc, ENET_TAEM, 256);	/* TxFIFO Almost Empty */
	ENET_REG_WRITE(sc, ENET_TAFL, 8);	/* TxFIFO Almost Full */
	ENET_REG_WRITE(sc, ENET_TIPG, 12);	/* Tx Inter-Packet Gap */

	/* hardware checksum is default off (override in TX descripter) */
	ENET_REG_WRITE(sc, ENET_TACC, 0);

	/*
	 * align ethernet payload on 32bit, discard frames with MAC layer error,
	 * and don't discard checksum error
	 */
	ENET_REG_WRITE(sc, ENET_RACC, ENET_RACC_SHIFT16 | ENET_RACC_LINEDIS);

	/* maximum frame size */
	val = ENET_DEFAULT_PKT_LEN;
	ENET_REG_WRITE(sc, ENET_FTRL, val);	/* Frame Truncation Length */

	if (sc->sc_rgmii == 0)
		miimode = ENET_RCR_RMII_MODE | ENET_RCR_MII_MODE;
	else
		miimode = ENET_RCR_RGMII_EN;
	ENET_REG_WRITE(sc, ENET_RCR,
	    ENET_RCR_PADEN |			/* RX frame padding remove */
	    miimode |
	    (flowctrl ? ENET_RCR_FCE : 0) |	/* flow control enable */
	    rcr_speed |
	    (fulldup ? 0 : ENET_RCR_DRT) |
	    ENET_RCR_MAX_FL(val));

	/* Maximum Receive BufSize per one descriptor */
	ENET_REG_WRITE(sc, ENET_MRBR, RXDESC_MAXBUFSIZE);


	/* TX/RX Descriptor Physical Address */
	paddr = sc->sc_txdesc_dmamap->dm_segs[0].ds_addr;
	ENET_REG_WRITE(sc, ENET_TDSR, paddr);
	paddr = sc->sc_rxdesc_dmamap->dm_segs[0].ds_addr;
	ENET_REG_WRITE(sc, ENET_RDSR, paddr);
	/* sync cache */
	bus_dmamap_sync(sc->sc_dmat, sc->sc_txdesc_dmamap, 0,
	    sc->sc_txdesc_dmamap->dm_mapsize, BUS_DMASYNC_PREWRITE);
	bus_dmamap_sync(sc->sc_dmat, sc->sc_rxdesc_dmamap, 0,
	    sc->sc_rxdesc_dmamap->dm_mapsize, BUS_DMASYNC_PREWRITE);

	/* enable interrupts */
	val = ENET_EIR_TXF | ENET_EIR_RXF | ENET_EIR_EBERR;
	if (sc->sc_imxtype == 7)
		val |= ENET_EIR_TXF2 | ENET_EIR_RXF2 | ENET_EIR_TXF1 |
		    ENET_EIR_RXF1;
	ENET_REG_WRITE(sc, ENET_EIMR, val);

	/* enable ether */
	ENET_REG_WRITE(sc, ENET_ECR,
#if _BYTE_ORDER == _LITTLE_ENDIAN
	    ENET_ECR_DBSWP |
#endif
	    ecr_speed |
	    ENET_ECR_EN1588 |	/* use enhanced TX/RX descriptor */
	    ENET_ECR_ETHEREN);	/* Ethernet Enable */

	return 0;
}

static int
enet_alloc_dma(struct enet_softc *sc, size_t size, void **addrp,
    bus_dmamap_t *mapp)
{
	bus_dma_segment_t seglist[1];
	int nsegs, error;

	if ((error = bus_dmamem_alloc(sc->sc_dmat, size, PAGE_SIZE, 0, seglist,
	    1, &nsegs, M_NOWAIT)) != 0) {
		device_printf(sc->sc_dev,
		    "unable to allocate DMA buffer, error=%d\n", error);
		goto fail_alloc;
	}

	if ((error = bus_dmamem_map(sc->sc_dmat, seglist, 1, size, addrp,
	    BUS_DMA_NOWAIT | BUS_DMA_COHERENT)) != 0) {
		device_printf(sc->sc_dev,
		    "unable to map DMA buffer, error=%d\n",
		    error);
		goto fail_map;
	}

	if ((error = bus_dmamap_create(sc->sc_dmat, size, 1, size, 0,
	    BUS_DMA_NOWAIT, mapp)) != 0) {
		device_printf(sc->sc_dev,
		    "unable to create DMA map, error=%d\n", error);
		goto fail_create;
	}

	if ((error = bus_dmamap_load(sc->sc_dmat, *mapp, *addrp, size, NULL,
	    BUS_DMA_NOWAIT)) != 0) {
		aprint_error_dev(sc->sc_dev,
		    "unable to load DMA map, error=%d\n", error);
		goto fail_load;
	}

	return 0;

 fail_load:
	bus_dmamap_destroy(sc->sc_dmat, *mapp);
 fail_create:
	bus_dmamem_unmap(sc->sc_dmat, *addrp, size);
 fail_map:
	bus_dmamem_free(sc->sc_dmat, seglist, 1);
 fail_alloc:
	return error;
}
