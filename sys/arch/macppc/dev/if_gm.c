/*	$NetBSD: if_gm.c,v 1.24.6.4 2008/01/21 09:37:27 yamt Exp $	*/

/*-
 * Copyright (c) 2000 Tsubai Masanari.  All rights reserved.
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
__KERNEL_RCSID(0, "$NetBSD: if_gm.c,v 1.24.6.4 2008/01/21 09:37:27 yamt Exp $");

#include "opt_inet.h"
#include "rnd.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/callout.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <dev/mii/mii.h>
#include <dev/mii/miivar.h>

#include <dev/pci/pcivar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcidevs.h>

#include <dev/ofw/openfirm.h>
#include <macppc/dev/if_gmreg.h>
#include <machine/pio.h>

#define NTXBUF 4
#define NRXBUF 32

struct gmac_softc {
	struct device sc_dev;
	struct ethercom sc_ethercom;
	vaddr_t sc_reg;
	struct gmac_dma *sc_txlist;
	struct gmac_dma *sc_rxlist;
	int sc_txnext;
	int sc_rxlast;
	void *sc_txbuf[NTXBUF];
	void *sc_rxbuf[NRXBUF];
	struct mii_data sc_mii;
	struct callout sc_tick_ch;
	char sc_laddr[6];

#if NRND > 0
	rndsource_element_t sc_rnd_source; /* random source */
#endif
};

#define sc_if sc_ethercom.ec_if

int gmac_match(struct device *, struct cfdata *, void *);
void gmac_attach(struct device *, struct device *, void *);

static inline u_int gmac_read_reg(struct gmac_softc *, int);
static inline void gmac_write_reg(struct gmac_softc *, int, u_int);

static inline void gmac_start_txdma(struct gmac_softc *);
static inline void gmac_start_rxdma(struct gmac_softc *);
static inline void gmac_stop_txdma(struct gmac_softc *);
static inline void gmac_stop_rxdma(struct gmac_softc *);

int gmac_intr(void *);
void gmac_tint(struct gmac_softc *);
void gmac_rint(struct gmac_softc *);
struct mbuf * gmac_get(struct gmac_softc *, void *, int);
void gmac_start(struct ifnet *);
int gmac_put(struct gmac_softc *, void *, struct mbuf *);

void gmac_stop(struct gmac_softc *);
void gmac_reset(struct gmac_softc *);
void gmac_init(struct gmac_softc *);
void gmac_init_mac(struct gmac_softc *);
void gmac_setladrf(struct gmac_softc *);

int gmac_ioctl(struct ifnet *, u_long, void *);
void gmac_watchdog(struct ifnet *);

int gmac_mii_readreg(struct device *, int, int);
void gmac_mii_writereg(struct device *, int, int, int);
void gmac_mii_statchg(struct device *);
void gmac_mii_tick(void *);

CFATTACH_DECL(gm, sizeof(struct gmac_softc),
    gmac_match, gmac_attach, NULL, NULL);

int
gmac_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pci_attach_args *pa = aux;

	if (PCI_VENDOR(pa->pa_id) == PCI_VENDOR_APPLE &&
	    (PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC2 ||
	     PCI_PRODUCT(pa->pa_id) == PCI_PRODUCT_APPLE_GMAC3))
		return 1;

	return 0;
}

void
gmac_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct gmac_softc *sc = (void *)self;
	struct pci_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_if;
	struct mii_data *mii = &sc->sc_mii;
	pci_intr_handle_t ih;
	const char *intrstr = NULL;
	int node, i;
	char *p;
	struct gmac_dma *dp;
	u_int32_t reg[10];
	u_char laddr[6];

	node = pcidev_to_ofdev(pa->pa_pc, pa->pa_tag);
	if (node == 0) {
		printf(": cannot find gmac node\n");
		return;
	}

	OF_getprop(node, "local-mac-address", laddr, sizeof laddr);
	OF_getprop(node, "assigned-addresses", reg, sizeof reg);

	memcpy(sc->sc_laddr, laddr, sizeof laddr);
	sc->sc_reg = reg[2];

	if (pci_intr_map(pa, &ih)) {
		printf(": unable to map interrupt\n");
		return;
	}
	intrstr = pci_intr_string(pa->pa_pc, ih);

	if (pci_intr_establish(pa->pa_pc, ih, IPL_NET, gmac_intr, sc) == NULL) {
		printf(": unable to establish interrupt");
		if (intrstr)
			printf(" at %s", intrstr);
		printf("\n");
		return;
	}

	/* Setup packet buffers and DMA descriptors. */
	p = malloc((NRXBUF + NTXBUF) * 2048 + 3 * 0x800, M_DEVBUF, M_NOWAIT);
	if (p == NULL) {
		printf(": cannot malloc buffers\n");
		return;
	}
	p = (void *)roundup((vaddr_t)p, 0x800);
	memset(p, 0, 2048 * (NRXBUF + NTXBUF) + 2 * 0x800);

	sc->sc_rxlist = (void *)p;
	p += 0x800;
	sc->sc_txlist = (void *)p;
	p += 0x800;

	dp = sc->sc_rxlist;
	for (i = 0; i < NRXBUF; i++) {
		sc->sc_rxbuf[i] = p;
		dp->address = htole32(vtophys((vaddr_t)p));
		dp->cmd = htole32(GMAC_OWN);
		dp++;
		p += 2048;
	}

	dp = sc->sc_txlist;
	for (i = 0; i < NTXBUF; i++) {
		sc->sc_txbuf[i] = p;
		dp->address = htole32(vtophys((vaddr_t)p));
		dp++;
		p += 2048;
	}

	printf(": Ethernet address %s\n", ether_sprintf(laddr));
	printf("%s: interrupting at %s\n", sc->sc_dev.dv_xname, intrstr);

	callout_init(&sc->sc_tick_ch, 0);

	gmac_reset(sc);
	gmac_init_mac(sc);

	memcpy(ifp->if_xname, sc->sc_dev.dv_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_ioctl = gmac_ioctl;
	ifp->if_start = gmac_start;
	ifp->if_watchdog = gmac_watchdog;
	ifp->if_flags =
		IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	IFQ_SET_READY(&ifp->if_snd);

	mii->mii_ifp = ifp;
	mii->mii_readreg = gmac_mii_readreg;
	mii->mii_writereg = gmac_mii_writereg;
	mii->mii_statchg = gmac_mii_statchg;

	sc->sc_ethercom.ec_mii = mii;
	ifmedia_init(&mii->mii_media, 0, ether_mediachange, ether_mediastatus);
	mii_attach(self, mii, 0xffffffff, MII_PHY_ANY, MII_OFFSET_ANY, 0);

	/* Choose a default media. */
	if (LIST_FIRST(&mii->mii_phys) == NULL) {
		ifmedia_add(&mii->mii_media, IFM_ETHER|IFM_NONE, 0, NULL);
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_NONE);
	} else
		ifmedia_set(&mii->mii_media, IFM_ETHER|IFM_AUTO);

	if_attach(ifp);
	ether_ifattach(ifp, laddr);
#if NRND > 0 
	rnd_attach_source(&sc->sc_rnd_source, sc->sc_dev.dv_xname,
	    RND_TYPE_NET, 0); 
#endif
}

u_int
gmac_read_reg(sc, reg)
	struct gmac_softc *sc;
	int reg;
{
	return in32rb(sc->sc_reg + reg);
}

void
gmac_write_reg(sc, reg, val)
	struct gmac_softc *sc;
	int reg;
	u_int val;
{
	out32rb(sc->sc_reg + reg, val);
}

void
gmac_start_txdma(sc)
	struct gmac_softc *sc;
{
	u_int x;

	x = gmac_read_reg(sc, GMAC_TXDMACONFIG);
	x |= 1;
	gmac_write_reg(sc, GMAC_TXDMACONFIG, x);
	x = gmac_read_reg(sc, GMAC_TXMACCONFIG);
	x |= 1;
	gmac_write_reg(sc, GMAC_TXMACCONFIG, x);
}

void
gmac_start_rxdma(sc)
	struct gmac_softc *sc;
{
	u_int x;

	x = gmac_read_reg(sc, GMAC_RXDMACONFIG);
	x |= 1;
	gmac_write_reg(sc, GMAC_RXDMACONFIG, x);
	x = gmac_read_reg(sc, GMAC_RXMACCONFIG);
	x |= 1;
	gmac_write_reg(sc, GMAC_RXMACCONFIG, x);
}

void
gmac_stop_txdma(sc)
	struct gmac_softc *sc;
{
	u_int x;

	x = gmac_read_reg(sc, GMAC_TXDMACONFIG);
	x &= ~1;
	gmac_write_reg(sc, GMAC_TXDMACONFIG, x);
	x = gmac_read_reg(sc, GMAC_TXMACCONFIG);
	x &= ~1;
	gmac_write_reg(sc, GMAC_TXMACCONFIG, x);
}

void
gmac_stop_rxdma(sc)
	struct gmac_softc *sc;
{
	u_int x;

	x = gmac_read_reg(sc, GMAC_RXDMACONFIG);
	x &= ~1;
	gmac_write_reg(sc, GMAC_RXDMACONFIG, x);
	x = gmac_read_reg(sc, GMAC_RXMACCONFIG);
	x &= ~1;
	gmac_write_reg(sc, GMAC_RXMACCONFIG, x);
}

int
gmac_intr(v)
	void *v;
{
	struct gmac_softc *sc = v;
	u_int status;

	status = gmac_read_reg(sc, GMAC_STATUS) & 0xff;
	if (status == 0)
		return 0;

	if (status & GMAC_INT_RXDONE)
		gmac_rint(sc);

	if (status & GMAC_INT_TXEMPTY)
		gmac_tint(sc);

#if NRND > 0 
	rnd_add_uint32(&sc->sc_rnd_source, status);
#endif  
	return 1;
}

void
gmac_tint(sc)
	struct gmac_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;

	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;
	gmac_start(ifp);
}

void
gmac_rint(sc)
	struct gmac_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;
	volatile struct gmac_dma *dp;
	struct mbuf *m;
	int i, j, len;
	u_int cmd;

	for (i = sc->sc_rxlast;; i++) {
		if (i == NRXBUF)
			i = 0;

		dp = &sc->sc_rxlist[i];
		cmd = le32toh(dp->cmd);
		if (cmd & GMAC_OWN)
			break;
		len = (cmd >> 16) & GMAC_LEN_MASK;
		len -= 4;	/* CRC */

		if (le32toh(dp->cmd_hi) & 0x40000000) {
			ifp->if_ierrors++;
			goto next;
		}

		m = gmac_get(sc, sc->sc_rxbuf[i], len);
		if (m == NULL) {
			ifp->if_ierrors++;
			goto next;
		}

#if NBPFILTER > 0
		/*
		 * Check if there's a BPF listener on this interface.
		 * If so, hand off the raw packet to BPF.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		(*ifp->if_input)(ifp, m);
		ifp->if_ipackets++;

next:
		dp->cmd_hi = 0;
		__asm volatile ("sync");
		dp->cmd = htole32(GMAC_OWN);
	}
	sc->sc_rxlast = i;

	/* XXX Make sure free buffers have GMAC_OWN. */
	i++;
	for (j = 1; j < NRXBUF; j++) {
		if (i == NRXBUF)
			i = 0;
		dp = &sc->sc_rxlist[i++];
		dp->cmd = htole32(GMAC_OWN);
	}
}

struct mbuf *
gmac_get(sc, pkt, totlen)
	struct gmac_softc *sc;
	void *pkt;
	int totlen;
{
	struct mbuf *m;
	struct mbuf *top, **mp;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m->m_pkthdr.rcvif = &sc->sc_if;
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
		pkt += len;
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return top;
}

void
gmac_start(ifp)
	struct ifnet *ifp;
{
	struct gmac_softc *sc = ifp->if_softc;
	struct mbuf *m;
	void *buff;
	int i, tlen;
	volatile struct gmac_dma *dp;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		if (ifp->if_flags & IFF_OACTIVE)
			break;

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			break;

		/* 5 seconds to watch for failing to transmit */
		ifp->if_timer = 5;
		ifp->if_opackets++;		/* # of pkts */

		i = sc->sc_txnext;
		buff = sc->sc_txbuf[i];
		tlen = gmac_put(sc, buff, m);

		dp = &sc->sc_txlist[i];
		dp->cmd_hi = 0;
		dp->address_hi = 0;
		dp->cmd = htole32(tlen | GMAC_OWN | GMAC_SOP);

		i++;
		if (i == NTXBUF)
			i = 0;
		__asm volatile ("sync");

		gmac_write_reg(sc, GMAC_TXDMAKICK, i);
		sc->sc_txnext = i;

#if NBPFILTER > 0
		/*
		 * If BPF is listening on this interface, let it see the
		 * packet before we commit it to the wire.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif
		m_freem(m);

		i++;
		if (i == NTXBUF)
			i = 0;
		if (i == gmac_read_reg(sc, GMAC_TXDMACOMPLETE)) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}
	}
}

int
gmac_put(sc, buff, m)
	struct gmac_softc *sc;
	void *buff;
	struct mbuf *m;
{
	int len, tlen = 0;

	for (; m; m = m->m_next) {
		len = m->m_len;
		if (len == 0)
			continue;
		memcpy(buff, mtod(m, void *), len);
		buff += len;
		tlen += len;
	}
	if (tlen > 2048)
		panic("%s: gmac_put packet overflow", sc->sc_dev.dv_xname);

	return tlen;
}

void
gmac_reset(sc)
	struct gmac_softc *sc;
{
	int i, s;

	s = splnet();

	gmac_stop_txdma(sc);
	gmac_stop_rxdma(sc);

	gmac_write_reg(sc, GMAC_SOFTWARERESET, 3);
	for (i = 10; i > 0; i--) {
		delay(300000);				/* XXX long delay */
		if ((gmac_read_reg(sc, GMAC_SOFTWARERESET) & 3) == 0)
			break;
	}
	if (i == 0)
		printf("%s: reset timeout\n", sc->sc_dev.dv_xname);

	sc->sc_txnext = 0;
	sc->sc_rxlast = 0;
	for (i = 0; i < NRXBUF; i++)
		sc->sc_rxlist[i].cmd = htole32(GMAC_OWN);
	__asm volatile ("sync");

	gmac_write_reg(sc, GMAC_TXDMADESCBASEHI, 0);
	gmac_write_reg(sc, GMAC_TXDMADESCBASELO,
		       vtophys((vaddr_t)sc->sc_txlist));
	gmac_write_reg(sc, GMAC_RXDMADESCBASEHI, 0);
	gmac_write_reg(sc, GMAC_RXDMADESCBASELO,
		       vtophys((vaddr_t)sc->sc_rxlist));
	gmac_write_reg(sc, GMAC_RXDMAKICK, NRXBUF);

	splx(s);
}

void
gmac_stop(sc)
	struct gmac_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;
	int s;

	s = splnet();

	callout_stop(&sc->sc_tick_ch);
	mii_down(&sc->sc_mii);

	gmac_stop_txdma(sc);
	gmac_stop_rxdma(sc);

	gmac_write_reg(sc, GMAC_INTMASK, 0xffffffff);

	ifp->if_flags &= ~(IFF_UP | IFF_RUNNING);
	ifp->if_timer = 0;

	splx(s);
}

void
gmac_init_mac(sc)
	struct gmac_softc *sc;
{
	int i, tb;
	char *laddr = sc->sc_laddr;

	if ((mfpvr() >> 16) == MPC601)
		tb = mfrtcl();
	else
		tb = mftbl();
	gmac_write_reg(sc, GMAC_RANDOMSEED, tb);

	/* init-mii */
	gmac_write_reg(sc, GMAC_DATAPATHMODE, 4);
	gmac_mii_writereg(&sc->sc_dev, 0, 0, 0x1000);

	gmac_write_reg(sc, GMAC_TXDMACONFIG, 0xffc00);
	gmac_write_reg(sc, GMAC_RXDMACONFIG, 0);
	gmac_write_reg(sc, GMAC_MACPAUSE, 0x1bf0);
	gmac_write_reg(sc, GMAC_INTERPACKETGAP0, 0);
	gmac_write_reg(sc, GMAC_INTERPACKETGAP1, 8);
	gmac_write_reg(sc, GMAC_INTERPACKETGAP2, 4);
	gmac_write_reg(sc, GMAC_MINFRAMESIZE, ETHER_MIN_LEN);
	gmac_write_reg(sc, GMAC_MAXFRAMESIZE, ETHER_MAX_LEN);
	gmac_write_reg(sc, GMAC_PASIZE, 7);
	gmac_write_reg(sc, GMAC_JAMSIZE, 4);
	gmac_write_reg(sc, GMAC_ATTEMPTLIMIT,0x10);
	gmac_write_reg(sc, GMAC_MACCNTLTYPE, 0x8808);

	gmac_write_reg(sc, GMAC_MACADDRESS0, (laddr[4] << 8) | laddr[5]);
	gmac_write_reg(sc, GMAC_MACADDRESS1, (laddr[2] << 8) | laddr[3]);
	gmac_write_reg(sc, GMAC_MACADDRESS2, (laddr[0] << 8) | laddr[1]);
	gmac_write_reg(sc, GMAC_MACADDRESS3, 0);
	gmac_write_reg(sc, GMAC_MACADDRESS4, 0);
	gmac_write_reg(sc, GMAC_MACADDRESS5, 0);
	gmac_write_reg(sc, GMAC_MACADDRESS6, 1);
	gmac_write_reg(sc, GMAC_MACADDRESS7, 0xc200);
	gmac_write_reg(sc, GMAC_MACADDRESS8, 0x0180);
	gmac_write_reg(sc, GMAC_MACADDRFILT0, 0);
	gmac_write_reg(sc, GMAC_MACADDRFILT1, 0);
	gmac_write_reg(sc, GMAC_MACADDRFILT2, 0);
	gmac_write_reg(sc, GMAC_MACADDRFILT2_1MASK, 0);
	gmac_write_reg(sc, GMAC_MACADDRFILT0MASK, 0);

	for (i = 0; i < 0x6c; i += 4)
		gmac_write_reg(sc, GMAC_HASHTABLE0 + i, 0);

	gmac_write_reg(sc, GMAC_SLOTTIME, 0x40);

	if (IFM_OPTIONS(sc->sc_mii.mii_media_active) & IFM_FDX) {
		gmac_write_reg(sc, GMAC_TXMACCONFIG, 6);
		gmac_write_reg(sc, GMAC_XIFCONFIG, 1);
	} else {
		gmac_write_reg(sc, GMAC_TXMACCONFIG, 0);
		gmac_write_reg(sc, GMAC_XIFCONFIG, 5);
	}

	if (0)	/* g-bit? */
		gmac_write_reg(sc, GMAC_MACCTRLCONFIG, 3);
	else
		gmac_write_reg(sc, GMAC_MACCTRLCONFIG, 0);
}

void
gmac_setladrf(sc)
	struct gmac_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;
	struct ether_multi *enm;
	struct ether_multistep step;
	struct ethercom *ec = &sc->sc_ethercom;
	u_int32_t crc;
	u_int32_t hash[16];
	u_int v;
	int i;

	/* Clear hash table */
	for (i = 0; i < 16; i++)
		hash[i] = 0;

	/* Get current RX configuration */
	v = gmac_read_reg(sc, GMAC_RXMACCONFIG);

	if ((ifp->if_flags & IFF_PROMISC) != 0) {
		/* Turn on promiscuous mode; turn off the hash filter */
		v |= GMAC_RXMAC_PR;
		v &= ~GMAC_RXMAC_HEN;
		ifp->if_flags |= IFF_ALLMULTI;
		goto chipit;
	}

	/* Turn off promiscuous mode; turn on the hash filter */
	v &= ~GMAC_RXMAC_PR;
	v |= GMAC_RXMAC_HEN;

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 8 bits as an
	 * index into the 256 bit logical address filter.  The high order bit
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	ETHER_FIRST_MULTI(step, ec, enm);
	while (enm != NULL) {
		if (memcmp(enm->enm_addrlo, enm->enm_addrhi, 6)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			for (i = 0; i < 16; i++)
				hash[i] = 0xffff;
			ifp->if_flags |= IFF_ALLMULTI;
			goto chipit;
		}

		crc = ether_crc32_le(enm->enm_addrlo, ETHER_ADDR_LEN);

		/* Just want the 8 most significant bits. */
		crc >>= 24;

		/* Set the corresponding bit in the filter. */
		hash[crc >> 4] |= 1 << (crc & 0xf);

		ETHER_NEXT_MULTI(step, enm);
	}

	ifp->if_flags &= ~IFF_ALLMULTI;

chipit:
	/* Now load the hash table into the chip */
	for (i = 0; i < 16; i++)
		gmac_write_reg(sc, GMAC_HASHTABLE0 + i * 4, hash[i]);

	gmac_write_reg(sc, GMAC_RXMACCONFIG, v);
}

void
gmac_init(sc)
	struct gmac_softc *sc;
{
	struct ifnet *ifp = &sc->sc_if;

	gmac_stop_txdma(sc);
	gmac_stop_rxdma(sc);

	gmac_init_mac(sc);
	gmac_setladrf(sc);

	gmac_start_txdma(sc);
	gmac_start_rxdma(sc);

	gmac_write_reg(sc, GMAC_INTMASK, ~(GMAC_INT_TXEMPTY | GMAC_INT_RXDONE));

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;

	callout_reset(&sc->sc_tick_ch, 1, gmac_mii_tick, sc);

	gmac_start(ifp);
}

int
gmac_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	void *data;
{
	struct gmac_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			gmac_init(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			gmac_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			gmac_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			gmac_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			gmac_reset(sc);
			gmac_init(sc);
		}
#ifdef GMAC_DEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_flags |= GMAC_DEBUGFLAG;
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
				gmac_init(sc);
				/* gmac_setladrf(sc); */
			}
			error = 0;
		}
		break;
	default:
		error = EINVAL;
	}

	splx(s);
	return error;
}

void
gmac_watchdog(ifp)
	struct ifnet *ifp;
{
	struct gmac_softc *sc = ifp->if_softc;

	printf("%s: device timeout\n", ifp->if_xname);
	ifp->if_oerrors++;

	gmac_reset(sc);
	gmac_init(sc);
}

int
gmac_mii_readreg(dev, phy, reg)
	struct device *dev;
	int phy, reg;
{
	struct gmac_softc *sc = (void *)dev;
	int i;

	gmac_write_reg(sc, GMAC_MIFFRAMEOUTPUT,
		0x60020000 | (phy << 23) | (reg << 18));

	for (i = 1000; i >= 0; i -= 10) {
		if (gmac_read_reg(sc, GMAC_MIFFRAMEOUTPUT) & 0x10000)
			break;
		delay(10);
	}
	if (i < 0) {
		printf("%s: gmac_mii_readreg: timeout\n", sc->sc_dev.dv_xname);
		return 0;
	}

	return gmac_read_reg(sc, GMAC_MIFFRAMEOUTPUT) & 0xffff;
}

void
gmac_mii_writereg(dev, phy, reg, val)
	struct device *dev;
	int phy, reg, val;
{
	struct gmac_softc *sc = (void *)dev;
	int i;

	gmac_write_reg(sc, GMAC_MIFFRAMEOUTPUT,
		0x50020000 | (phy << 23) | (reg << 18) | (val & 0xffff));

	for (i = 1000; i >= 0; i -= 10) {
		if (gmac_read_reg(sc, GMAC_MIFFRAMEOUTPUT) & 0x10000)
			break;
		delay(10);
	}
	if (i < 0)
		printf("%s: gmac_mii_writereg: timeout\n", sc->sc_dev.dv_xname);
}

void
gmac_mii_statchg(dev)
	struct device *dev;
{
	struct gmac_softc *sc = (void *)dev;

	gmac_stop_txdma(sc);
	gmac_stop_rxdma(sc);

	if (IFM_OPTIONS(sc->sc_mii.mii_media_active) & IFM_FDX) {
		gmac_write_reg(sc, GMAC_TXMACCONFIG, 6);
		gmac_write_reg(sc, GMAC_XIFCONFIG, 1);
	} else {
		gmac_write_reg(sc, GMAC_TXMACCONFIG, 0);
		gmac_write_reg(sc, GMAC_XIFCONFIG, 5);
	}

	if (0)	/* g-bit? */
		gmac_write_reg(sc, GMAC_MACCTRLCONFIG, 3);
	else
		gmac_write_reg(sc, GMAC_MACCTRLCONFIG, 0);

	gmac_start_txdma(sc);
	gmac_start_rxdma(sc);
}

void
gmac_mii_tick(v)
	void *v;
{
	struct gmac_softc *sc = v;
	int s;

	s = splnet();
	mii_tick(&sc->sc_mii);
	splx(s);

	callout_reset(&sc->sc_tick_ch, hz, gmac_mii_tick, sc);
}
