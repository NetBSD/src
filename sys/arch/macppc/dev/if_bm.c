/*	$NetBSD: if_bm.c,v 1.46.16.3 2016/10/05 20:55:31 skrll Exp $	*/

/*-
 * Copyright (C) 1998, 1999, 2000 Tsubai Masanari.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_bm.c,v 1.46.16.3 2016/10/05 20:55:31 skrll Exp $");

#include "opt_inet.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/callout.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/bpf.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif


#include <dev/ofw/openfirm.h>

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>
#include <dev/mii/mii_bitbang.h>

#include <powerpc/spr.h>
#include <powerpc/oea/spr.h>

#include <machine/autoconf.h>
#include <machine/pio.h>

#include <macppc/dev/dbdma.h>
#include <macppc/dev/if_bmreg.h>
#include <macppc/dev/obiovar.h>

#define BMAC_TXBUFS 2
#define BMAC_RXBUFS 16
#define BMAC_BUFLEN 2048

struct bmac_softc {
	device_t sc_dev;
	struct ethercom sc_ethercom;
#define sc_if sc_ethercom.ec_if
	struct callout sc_tick_ch;
	bus_space_tag_t sc_iot;
	bus_space_handle_t sc_ioh;
	dbdma_regmap_t *sc_txdma;
	dbdma_regmap_t *sc_rxdma;
	dbdma_command_t *sc_txcmd;
	dbdma_command_t *sc_rxcmd;
	void *sc_txbuf;
	void *sc_rxbuf;
	int sc_rxlast;
	int sc_flags;
	struct mii_data sc_mii;
	u_char sc_enaddr[6];
};

#define BMAC_BMACPLUS	0x01
#define BMAC_DEBUGFLAG	0x02

int bmac_match(device_t, cfdata_t, void *);
void bmac_attach(device_t, device_t, void *);
void bmac_reset_chip(struct bmac_softc *);
void bmac_init(struct bmac_softc *);
void bmac_init_dma(struct bmac_softc *);
int bmac_intr(void *);
int bmac_rint(void *);
void bmac_reset(struct bmac_softc *);
void bmac_stop(struct bmac_softc *);
void bmac_start(struct ifnet *);
void bmac_transmit_packet(struct bmac_softc *, void *, int);
int bmac_put(struct bmac_softc *, void *, struct mbuf *);
struct mbuf *bmac_get(struct bmac_softc *, void *, int);
void bmac_watchdog(struct ifnet *);
int bmac_ioctl(struct ifnet *, u_long, void *);
void bmac_setladrf(struct bmac_softc *);

int bmac_mii_readreg(device_t, int, int);
void bmac_mii_writereg(device_t, int, int, int);
void bmac_mii_statchg(struct ifnet *);
void bmac_mii_tick(void *);
u_int32_t bmac_mbo_read(device_t);
void bmac_mbo_write(device_t, u_int32_t);

CFATTACH_DECL_NEW(bm, sizeof(struct bmac_softc),
    bmac_match, bmac_attach, NULL, NULL);

const struct mii_bitbang_ops bmac_mbo = {
	bmac_mbo_read, bmac_mbo_write,
	{ MIFDO, MIFDI, MIFDC, MIFDIR, 0 }
};

static inline uint16_t
bmac_read_reg(struct bmac_softc *sc, bus_size_t off)
{
	return bus_space_read_2(sc->sc_iot, sc->sc_ioh, off);
}

static inline void
bmac_write_reg(struct bmac_softc *sc, bus_size_t off, uint16_t val)
{
	bus_space_write_2(sc->sc_iot, sc->sc_ioh, off, val);
}

static inline void
bmac_set_bits(struct bmac_softc *sc, bus_size_t off, uint16_t val)
{
	val |= bmac_read_reg(sc, off);
	bmac_write_reg(sc, off, val);
}

static inline void
bmac_reset_bits(struct bmac_softc *sc, bus_size_t off, uint16_t val)
{
	bmac_write_reg(sc, off, bmac_read_reg(sc, off) & ~val);
}

int
bmac_match(device_t parent, cfdata_t cf, void *aux)
{
	struct confargs *ca = aux;

	if (ca->ca_nreg < 24 || ca->ca_nintr < 12)
		return 0;

	if (strcmp(ca->ca_name, "bmac") == 0)		/* bmac */
		return 1;
	if (strcmp(ca->ca_name, "ethernet") == 0)	/* bmac+ */
		return 1;

	return 0;
}

void
bmac_attach(device_t parent, device_t self, void *aux)
{
	struct confargs *ca = aux;
	struct bmac_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_if;
	struct mii_data *mii = &sc->sc_mii;
	u_char laddr[6];

	callout_init(&sc->sc_tick_ch, 0);

	sc->sc_dev = self;
	sc->sc_flags = 0;
	if (strcmp(ca->ca_name, "ethernet") == 0) {
		char name[64];

		memset(name, 0, 64);
		OF_package_to_path(ca->ca_node, name, sizeof(name));
		OF_open(name);
		sc->sc_flags |= BMAC_BMACPLUS;
	}

	ca->ca_reg[0] += ca->ca_baseaddr;
	ca->ca_reg[2] += ca->ca_baseaddr;
	ca->ca_reg[4] += ca->ca_baseaddr;

	sc->sc_iot = ca->ca_tag;
	if (bus_space_map(sc->sc_iot, ca->ca_reg[0], ca->ca_reg[1], 0,
	    &sc->sc_ioh) != 0) {
		aprint_error(": couldn't map %#x", ca->ca_reg[0]);
		return;
	}

	bmac_write_reg(sc, INTDISABLE, NoEventsMask);

	if (OF_getprop(ca->ca_node, "local-mac-address", laddr, 6) == -1 &&
	    OF_getprop(ca->ca_node, "mac-address", laddr, 6) == -1) {
		aprint_error(": cannot get mac-address\n");
		return;
	}
	memcpy(sc->sc_enaddr, laddr, 6);

	sc->sc_txdma = mapiodev(ca->ca_reg[2], PAGE_SIZE, false);
	sc->sc_rxdma = mapiodev(ca->ca_reg[4], PAGE_SIZE, false);
	sc->sc_txcmd = dbdma_alloc(BMAC_TXBUFS * sizeof(dbdma_command_t), NULL);
	sc->sc_rxcmd = dbdma_alloc((BMAC_RXBUFS + 1) * sizeof(dbdma_command_t),
	    NULL);
	sc->sc_txbuf = malloc(BMAC_BUFLEN * BMAC_TXBUFS, M_DEVBUF, M_NOWAIT);
	sc->sc_rxbuf = malloc(BMAC_BUFLEN * BMAC_RXBUFS, M_DEVBUF, M_NOWAIT);
	if (sc->sc_txbuf == NULL || sc->sc_rxbuf == NULL ||
	    sc->sc_txcmd == NULL || sc->sc_rxcmd == NULL) {
		aprint_error("cannot allocate memory\n");
		return;
	}

	aprint_normal(" irq %d,%d: address %s\n",
	    ca->ca_intr[0], ca->ca_intr[2],
	    ether_sprintf(laddr));

	intr_establish(ca->ca_intr[0], IST_EDGE, IPL_NET, bmac_intr, sc);
	intr_establish(ca->ca_intr[2], IST_EDGE, IPL_NET, bmac_rint, sc);

	memcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_ioctl = bmac_ioctl;
	ifp->if_start = bmac_start;
	ifp->if_flags =
		IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	ifp->if_watchdog = bmac_watchdog;
	IFQ_SET_READY(&ifp->if_snd);

	mii->mii_ifp = ifp;
	mii->mii_readreg = bmac_mii_readreg;
	mii->mii_writereg = bmac_mii_writereg;
	mii->mii_statchg = bmac_mii_statchg;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii_attach(sc->sc_dev, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0);

	/* Choose a default media. */
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_10_T, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_10_T);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);

	bmac_reset_chip(sc);

	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);
}

/*
 * Reset and enable bmac by heathrow FCR.
 */
void
bmac_reset_chip(struct bmac_softc *sc)
{
	u_int v;

	dbdma_reset(sc->sc_txdma);
	dbdma_reset(sc->sc_rxdma);

	v = obio_read_4(HEATHROW_FCR);

	v |= EnetEnable;
	obio_write_4(HEATHROW_FCR, v);
	delay(50000);

	v |= ResetEnetCell;
	obio_write_4(HEATHROW_FCR, v);
	delay(50000);

	v &= ~ResetEnetCell;
	obio_write_4(HEATHROW_FCR, v);
	delay(50000);

	obio_write_4(HEATHROW_FCR, v);
}

void
bmac_init(struct bmac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	struct ether_header *eh;
	void *data;
	int i, tb, bmcr;
	u_short *p;

	bmac_reset_chip(sc);

	/* XXX */
	bmcr = bmac_mii_readreg(sc->sc_dev, 0, MII_BMCR);
	bmcr &= ~BMCR_ISO;
	bmac_mii_writereg(sc->sc_dev, 0, MII_BMCR, bmcr);

	bmac_write_reg(sc, RXRST, RxResetValue);
	bmac_write_reg(sc, TXRST, TxResetBit);

	/* Wait for reset completion. */
	for (i = 1000; i > 0; i -= 10) {
		if ((bmac_read_reg(sc, TXRST) & TxResetBit) == 0)
			break;
		delay(10);
	}
	if (i <= 0)
		printf("%s: reset timeout\n", ifp->if_xname);

	if (! (sc->sc_flags & BMAC_BMACPLUS))
		bmac_set_bits(sc, XCVRIF, ClkBit|SerialMode|COLActiveLow);

	if ((mfpvr() >> 16) == MPC601)
		tb = mfrtcl();
	else
		tb = mftbl();
	bmac_write_reg(sc, RSEED, tb);
	bmac_set_bits(sc, XIFC, TxOutputEnable);
	bmac_read_reg(sc, PAREG);

	/* Reset various counters. */
	bmac_write_reg(sc, NCCNT, 0);
	bmac_write_reg(sc, NTCNT, 0);
	bmac_write_reg(sc, EXCNT, 0);
	bmac_write_reg(sc, LTCNT, 0);
	bmac_write_reg(sc, FRCNT, 0);
	bmac_write_reg(sc, LECNT, 0);
	bmac_write_reg(sc, AECNT, 0);
	bmac_write_reg(sc, FECNT, 0);
	bmac_write_reg(sc, RXCV, 0);

	/* Set tx fifo information. */
	bmac_write_reg(sc, TXTH, 4);	/* 4 octets before tx starts */

	bmac_write_reg(sc, TXFIFOCSR, 0);
	bmac_write_reg(sc, TXFIFOCSR, TxFIFOEnable);

	/* Set rx fifo information. */
	bmac_write_reg(sc, RXFIFOCSR, 0);
	bmac_write_reg(sc, RXFIFOCSR, RxFIFOEnable);

	/* Clear status register. */
	bmac_read_reg(sc, STATUS);

	bmac_write_reg(sc, HASH3, 0);
	bmac_write_reg(sc, HASH2, 0);
	bmac_write_reg(sc, HASH1, 0);
	bmac_write_reg(sc, HASH0, 0);

	/* Set MAC address. */
	p = (u_short *)sc->sc_enaddr;
	bmac_write_reg(sc, MADD0, *p++);
	bmac_write_reg(sc, MADD1, *p++);
	bmac_write_reg(sc, MADD2, *p);

	bmac_write_reg(sc, RXCFG,
		RxCRCEnable | RxHashFilterEnable | RxRejectOwnPackets);

	if (ifp->if_flags & IFF_PROMISC)
		bmac_set_bits(sc, RXCFG, RxPromiscEnable);

	bmac_init_dma(sc);

	/* Enable TX/RX */
	bmac_set_bits(sc, RXCFG, RxMACEnable);
	bmac_set_bits(sc, TXCFG, TxMACEnable);

	bmac_write_reg(sc, INTDISABLE, NormalIntEvents);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;

	data = sc->sc_txbuf;
	eh = (struct ether_header *)data;

	memset(data, 0, sizeof(eh) + ETHERMIN);
	memcpy(eh->ether_dhost, sc->sc_enaddr, ETHER_ADDR_LEN);
	memcpy(eh->ether_shost, sc->sc_enaddr, ETHER_ADDR_LEN);
	bmac_transmit_packet(sc, data, sizeof(eh) + ETHERMIN);

	bmac_start(ifp);

	callout_reset(&sc->sc_tick_ch, hz, bmac_mii_tick, sc);
}

void
bmac_init_dma(struct bmac_softc *sc)
{
	dbdma_command_t *cmd = sc->sc_rxcmd;
	int i;

	dbdma_reset(sc->sc_txdma);
	dbdma_reset(sc->sc_rxdma);

	memset(sc->sc_txcmd, 0, BMAC_TXBUFS * sizeof(dbdma_command_t));
	memset(sc->sc_rxcmd, 0, (BMAC_RXBUFS + 1) * sizeof(dbdma_command_t));

	for (i = 0; i < BMAC_RXBUFS; i++) {
		DBDMA_BUILD(cmd, DBDMA_CMD_IN_LAST, 0, BMAC_BUFLEN,
			vtophys((vaddr_t)sc->sc_rxbuf + BMAC_BUFLEN * i),
			DBDMA_INT_ALWAYS, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
		cmd++;
	}
	DBDMA_BUILD(cmd, DBDMA_CMD_NOP, 0, 0, 0,
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_ALWAYS);
	out32rb(&cmd->d_cmddep, vtophys((vaddr_t)sc->sc_rxcmd));

	sc->sc_rxlast = 0;

	dbdma_start(sc->sc_rxdma, sc->sc_rxcmd);
}

int
bmac_intr(void *v)
{
	struct bmac_softc *sc = v;
	int stat;

	stat = bmac_read_reg(sc, STATUS);
	if (stat == 0)
		return 0;

#ifdef BMAC_DEBUG
	printf("bmac_intr status = 0x%x\n", stat);
#endif

	if (stat & IntFrameSent) {
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
		sc->sc_if.if_timer = 0;
		sc->sc_if.if_opackets++;
		bmac_start(&sc->sc_if);
	}

	/* XXX should do more! */

	return 1;
}

int
bmac_rint(void *v)
{
	struct bmac_softc *sc = v;
	struct ifnet *ifp = &sc->sc_if;
	struct mbuf *m;
	dbdma_command_t *cmd;
	int status, resid, count, datalen;
	int i, n;
	void *data;

	i = sc->sc_rxlast;
	for (n = 0; n < BMAC_RXBUFS; n++, i++) {
		if (i == BMAC_RXBUFS)
			i = 0;
		cmd = &sc->sc_rxcmd[i];
		status = in16rb(&cmd->d_status);
		resid = in16rb(&cmd->d_resid);

#ifdef BMAC_DEBUG
		if (status != 0 && status != 0x8440 && status != 0x9440)
			printf("bmac_rint status = 0x%x\n", status);
#endif

		if ((status & DBDMA_CNTRL_ACTIVE) == 0)	/* 0x9440 | 0x8440 */
			continue;
		count = in16rb(&cmd->d_count);
		datalen = count - resid - 2;		/* 2 == framelen */
		if (datalen < sizeof(struct ether_header)) {
			printf("%s: short packet len = %d\n",
				ifp->if_xname, datalen);
			goto next;
		}
		DBDMA_BUILD_CMD(cmd, DBDMA_CMD_STOP, 0, 0, 0, 0);
		data = (char *)sc->sc_rxbuf + BMAC_BUFLEN * i;

		/* XXX Sometimes bmac reads one extra byte. */
		if (datalen == ETHER_MAX_LEN + 1)
			datalen--;

		/* Trim the CRC. */
		datalen -= ETHER_CRC_LEN;

		m = bmac_get(sc, data, datalen);
		if (m == NULL) {
			ifp->if_ierrors++;
			goto next;
		}

		/*
		 * Check if there's a BPF listener on this interface.
		 * If so, hand off the raw packet to BPF.
		 */
		bpf_mtap(ifp, m);
		if_percpuq_enqueue(ifp->if_percpuq, m);
		ifp->if_ipackets++;

next:
		DBDMA_BUILD_CMD(cmd, DBDMA_CMD_IN_LAST, 0, DBDMA_INT_ALWAYS,
			DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

		cmd->d_status = 0;
		cmd->d_resid = 0;
		sc->sc_rxlast = i + 1;
	}
	ether_mediachange(ifp);

	dbdma_continue(sc->sc_rxdma);

	return 1;
}

void
bmac_reset(struct bmac_softc *sc)
{
	int s;

	s = splnet();
	bmac_init(sc);
	splx(s);
}

void
bmac_stop(struct bmac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	int s;

	s = splnet();

	callout_stop(&sc->sc_tick_ch);
	mii_down(&sc->sc_mii);

	/* Disable TX/RX. */
	bmac_reset_bits(sc, TXCFG, TxMACEnable);
	bmac_reset_bits(sc, RXCFG, RxMACEnable);

	/* Disable all interrupts. */
	bmac_write_reg(sc, INTDISABLE, NoEventsMask);

	dbdma_stop(sc->sc_txdma);
	dbdma_stop(sc->sc_rxdma);

	ifp->if_flags &= ~(IFF_UP | IFF_RUNNING);
	ifp->if_timer = 0;

	splx(s);
}

void
bmac_start(struct ifnet *ifp)
{
	struct bmac_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int tlen;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	while (1) {
		if (ifp->if_flags & IFF_OACTIVE)
			return;

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			break;
		/*
		 * If BPF is listening on this interface, let it see the
		 * packet before we commit it to the wire.
		 */
		bpf_mtap(ifp, m);

		ifp->if_flags |= IFF_OACTIVE;
		tlen = bmac_put(sc, sc->sc_txbuf, m);

		/* 5 seconds to watch for failing to transmit */
		ifp->if_timer = 5;
		ifp->if_opackets++;		/* # of pkts */

		bmac_transmit_packet(sc, sc->sc_txbuf, tlen);
	}
}

void
bmac_transmit_packet(struct bmac_softc *sc, void *buff, int len)
{
	dbdma_command_t *cmd = sc->sc_txcmd;
	vaddr_t va = (vaddr_t)buff;

#ifdef BMAC_DEBUG
	if (vtophys(va) + len - 1 != vtophys(va + len - 1))
		panic("bmac_transmit_packet");
#endif

	DBDMA_BUILD(cmd, DBDMA_CMD_OUT_LAST, 0, len, vtophys(va),
		DBDMA_INT_NEVER, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);
	cmd++;
	DBDMA_BUILD(cmd, DBDMA_CMD_STOP, 0, 0, 0,
		DBDMA_INT_ALWAYS, DBDMA_WAIT_NEVER, DBDMA_BRANCH_NEVER);

	dbdma_start(sc->sc_txdma, sc->sc_txcmd);
}

int
bmac_put(struct bmac_softc *sc, void *buff, struct mbuf *m)
{
	struct mbuf *n;
	int len, tlen = 0;

	for (; m; m = n) {
		len = m->m_len;
		if (len == 0) {
			n = m_free(m);
			continue;
		}
		memcpy(buff, mtod(m, void *), len);
		buff = (char *)buff + len;
		tlen += len;
		n = m_free(m);
	}
	if (tlen > PAGE_SIZE)
		panic("%s: putpacket packet overflow",
		    device_xname(sc->sc_dev));

	return tlen;
}

struct mbuf *
bmac_get(struct bmac_softc *sc, void *pkt, int totlen)
{
	struct mbuf *m;
	struct mbuf *top, **mp;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m_set_rcvif(m, &sc->sc_if);
	m->m_pkthdr.len = totlen;
	len = MHLEN;
	top = 0;
	mp = &top;

	while (totlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return 0;
			}
			len = MLEN;
		}
		if (totlen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_free(m);
				m_freem(top);
				return 0;
			}
			len = MCLBYTES;
		}
		m->m_len = len = min(totlen, len);
		memcpy(mtod(m, void *), pkt, len);
		pkt = (char *)pkt + len;
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return top;
}

void
bmac_watchdog(struct ifnet *ifp)
{
	struct bmac_softc *sc = ifp->if_softc;

	bmac_reset_bits(sc, RXCFG, RxMACEnable);
	bmac_reset_bits(sc, TXCFG, TxMACEnable);

	printf("%s: device timeout\n", ifp->if_xname);
	ifp->if_oerrors++;

	bmac_reset(sc);
}

int
bmac_ioctl(struct ifnet *ifp, unsigned long cmd, void *data)
{
	struct bmac_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;

		bmac_init(sc);
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		/* XXX see the comment in ed_ioctl() about code re-use */
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			bmac_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			bmac_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			/*bmac_stop(sc);*/
			bmac_init(sc);
		}
#ifdef BMAC_DEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_flags |= BMAC_DEBUGFLAG;
#endif
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			if (ifp->if_flags & IFF_RUNNING) {
				bmac_init(sc);
				bmac_setladrf(sc);
			}
			error = 0;
		}
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
		break;
	}

	splx(s);
	return error;
}

/*
 * Set up the logical address filter.
 */
void
bmac_setladrf(struct bmac_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	u_int32_t crc;
	u_int16_t hash[4];
	int x;

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 6 bits as an
	 * index into the 64 bit logical address filter.  The high order bit
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	if (ifp->if_flags & IFF_PROMISC) {
		bmac_set_bits(sc, RXCFG, RxPromiscEnable);
		return;
	}

	if (ifp->if_flags & IFF_ALLMULTI) {
		hash[3] = hash[2] = hash[1] = hash[0] = 0xffff;
		goto chipit;
	}

	hash[3] = hash[2] = hash[1] = hash[0] = 0;

	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
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
			hash[3] = hash[2] = hash[1] = hash[0] = 0xffff;
			ifp->if_flags |= IFF_ALLMULTI;
			goto chipit;
		}

		crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);

		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Set the corresponding bit in the filter. */
		hash[crc >> 4] |= 1 << (crc & 0xf);

		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;

chipit:
	bmac_write_reg(sc, HASH0, hash[0]);
	bmac_write_reg(sc, HASH1, hash[1]);
	bmac_write_reg(sc, HASH2, hash[2]);
	bmac_write_reg(sc, HASH3, hash[3]);
	x = bmac_read_reg(sc, RXCFG);
	x &= ~RxPromiscEnable;
	x |= RxHashFilterEnable;
	bmac_write_reg(sc, RXCFG, x);
}

int
bmac_mii_readreg(device_t self, int phy, int reg)
{
	return mii_bitbang_readreg(self, &bmac_mbo, phy, reg);
}

void
bmac_mii_writereg(device_t self, int phy, int reg, int val)
{
	mii_bitbang_writereg(self, &bmac_mbo, phy, reg, val);
}

u_int32_t
bmac_mbo_read(device_t self)
{
	struct bmac_softc *sc = device_private(self);

	return bmac_read_reg(sc, MIFCSR);
}

void
bmac_mbo_write(device_t self, u_int32_t val)
{
	struct bmac_softc *sc = device_private(self);

	bmac_write_reg(sc, MIFCSR, val);
}

void
bmac_mii_statchg(struct ifnet *ifp)
{
	struct bmac_softc *sc = ifp->if_softc;
	int x;

	/* Update duplex mode in TX configuration */
	x = bmac_read_reg(sc, TXCFG);
	if ((IFM_OPTIONS(sc->sc_mii.mii_media_active) & IFM_FDX) != 0)
		x |= TxFullDuplex;
	else
		x &= ~TxFullDuplex;
	bmac_write_reg(sc, TXCFG, x);

#ifdef BMAC_DEBUG
	printf("bmac_mii_statchg 0x%x\n",
		IFM_OPTIONS(sc->sc_mii.mii_media_active));
#endif
}

void
bmac_mii_tick(void *v)
{
	struct bmac_softc *sc = v;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, bmac_mii_tick, sc);
}
