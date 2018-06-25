/*	$NetBSD: if_le_ebus.c,v 1.13.14.1 2018/06/25 07:25:40 pgoyette Exp $	*/

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was written by Alessandro Forin and Neil Pittman
 * at Microsoft Research and contributed to The NetBSD Foundation
 * by Microsoft Corporation.
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
__KERNEL_RCSID(0, "$NetBSD: if_le_ebus.c,v 1.13.14.1 2018/06/25 07:25:40 pgoyette Exp $");

#include "opt_inet.h"



#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/bpf.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <sys/rndsource.h>

#include <emips/ebus/ebusvar.h>
#include <emips/emips/machdep.h>
#include <machine/emipsreg.h>

extern paddr_t kvtophys(vaddr_t);

struct bufmap {
	struct mbuf *mbuf;
	paddr_t phys;
};

struct enic_softc {
	device_t sc_dev;		/* base device glue */
	struct	ethercom sc_ethercom;	/* Ethernet common part */
	struct	ifmedia sc_media;	/* our supported media */

	struct _Enic *sc_regs;		/* hw registers */

	int	sc_havecarrier;		/* carrier status */
	void	*sc_sh;			/* shutdownhook cookie */
	int inited;

	int sc_no_rd;
	int sc_n_recv;
	int sc_recv_h;
	/* BUGBUG really should be malloc-ed */
#define SC_MAX_N_RECV 64
	struct bufmap sc_recv[SC_MAX_N_RECV];

	int sc_no_td;
	int sc_n_xmit;
	int sc_xmit_h;
	/* BUGBUG really should be malloc-ed */
#define SC_MAX_N_XMIT 16
	struct bufmap sc_xmit[SC_MAX_N_XMIT];

#if DEBUG
	int xhit;
	int xmiss;
	int tfull;
	int tfull2;
	int brh;
	int rf;
	int bxh;

	int it;
#endif

	uint8_t sc_enaddr[ETHER_ADDR_LEN];
	uint8_t sc_pad[2];
	krndsource_t	rnd_source;
};

void enic_reset(struct ifnet *);
int enic_init(struct ifnet *);
void enic_stop(struct ifnet *, int);
void enic_start(struct ifnet *);
void enic_shutdown(void *);
void enic_watchdog(struct ifnet *);
int enic_mediachange(struct ifnet *);
void enic_mediastatus(struct ifnet *, struct ifmediareq *);
int enic_ioctl(struct ifnet *, u_long, void *);
int enic_intr(void *, void *);
void enic_rint(struct enic_softc *, uint32_t, paddr_t);
void enic_tint(struct enic_softc *, uint32_t, paddr_t);
void enic_kill_xmit(struct enic_softc *);
void enic_post_recv(struct enic_softc *, struct mbuf *);
void enic_refill(struct enic_softc *);
static int enic_gethwinfo(struct enic_softc *);
int enic_put(struct enic_softc *, struct mbuf **);

static int enic_match(device_t, cfdata_t, void *);
static void enic_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(enic_emips, sizeof(struct enic_softc),
    enic_match, enic_attach, NULL, NULL);

int
enic_match(device_t parent, cfdata_t cf, void *aux)
{
	struct ebus_attach_args *d = aux;
	/* donno yet */
	struct _Enic *et = (struct _Enic *)d->ia_vaddr;

	if (strcmp("enic", d->ia_name) != 0)
		return 0;
	if ((et == NULL) || (et->Tag != PMTTAG_ETHERNET))
		return 0;
	return 1;
}

void
enic_attach(device_t parent, device_t self, void *aux)
{
	struct enic_softc *sc = device_private(self);
	struct ebus_attach_args *ia = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	sc->sc_regs = (struct _Enic *)(ia->ia_vaddr);
#if DEBUG
	printf(" virt=%p ", (void *)sc->sc_regs);
#endif

	/* Get the MAC and the depth of the FIFOs */
	if (!enic_gethwinfo(sc)) {
		printf(" <cannot get hw info> DISABLED.\n");
		/*
		 * NB: caveat maintainer: make sure what we
		 * did NOT do below does not hurt the system
		 */
		return;
	}

	sc->sc_dev = self;
	sc->sc_no_td = 0;
	sc->sc_havecarrier = 1; /* BUGBUG */
	sc->sc_recv_h = 0;
	sc->sc_xmit_h = 0;
	/* uhmm do I need to do this? */
	memset(sc->sc_recv, 0, sizeof sc->sc_recv);
	memset(sc->sc_xmit, 0, sizeof sc->sc_xmit);

	/* Initialize ifnet structure. */
	strcpy(ifp->if_xname, device_xname(sc->sc_dev));
	ifp->if_softc = sc;
	ifp->if_start = enic_start;
	ifp->if_ioctl = enic_ioctl;
	ifp->if_watchdog = enic_watchdog;
	ifp->if_init = enic_init;
	ifp->if_stop = enic_stop;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	IFQ_SET_READY(&ifp->if_snd);

	/* Initialize ifmedia structures. */
	ifmedia_init(&sc->sc_media, 0, enic_mediachange, enic_mediastatus);
	ifmedia_add(&sc->sc_media, IFM_ETHER|IFM_MANUAL, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER|IFM_MANUAL);

	/* Make sure the chip is stopped. */
	enic_stop(ifp, 0);
	sc->inited = 0;

	/* Get the mac address and print it */
	printf(": eNIC [%d %d], address %s\n",
	    sc->sc_n_recv, sc->sc_n_xmit, ether_sprintf(sc->sc_enaddr));

	/* claim 802.1q capability */
#if 0
	sc->sc_ethercom.ec_capabilities |= ETHERCAP_VLAN_MTU;
#endif

	/* Attach the interface. */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, sc->sc_enaddr);

	sc->sc_sh = shutdownhook_establish(enic_shutdown, ifp);
	if (sc->sc_sh == NULL)
		panic("enic_attach: cannot establish shutdown hook");

	rnd_attach_source(&sc->rnd_source, device_xname(sc->sc_dev),
			  RND_TYPE_NET, RND_FLAG_DEFAULT);

	ebus_intr_establish(parent, (void *)ia->ia_cookie, IPL_NET,
	    enic_intr, sc);
}

/*
 * Beware: does not work while the nic is running
 */
static int enic_gethwinfo(struct enic_softc *sc)
{
	uint8_t buffer[8];	/* 64bits max */
	PENIC_INFO hw = (PENIC_INFO)buffer;
	paddr_t phys = kvtophys((vaddr_t)&buffer[0]), phys2;
	int i;

	/*
	 * First thing first, get the MAC address
	 */
	memset(buffer,0,sizeof buffer);
	buffer[0] = ENIC_CMD_GET_ADDRESS;
	buffer[3] = ENIC_CMD_GET_ADDRESS;	/* bswap bug */
	sc->sc_regs->SizeAndFlags = (sizeof buffer) | ES_F_CMD;
	sc->sc_regs->BufferAddressHi32 = 0;
	sc->sc_regs->BufferAddressLo32 = phys; /* go! */

	for (i = 0; i < 100; i++) {
		DELAY(100);
		if ((sc->sc_regs->Control & EC_OF_EMPTY) == 0)
			break;
	}
	if (i == 100)
		return 0;

	phys2 = sc->sc_regs->BufferAddressLo32;
	if (phys2 != phys) {
		printf("enic uhu? %llx != %llx?\n",
		    (long long)phys, (long long)phys2);
		return 0;
	}
	memcpy(sc->sc_enaddr, buffer, ETHER_ADDR_LEN);

	/*
	 * Next get the HW parameters
	 */
	memset(buffer,0,sizeof buffer);
	buffer[0] = ENIC_CMD_GET_INFO;
	buffer[3] = ENIC_CMD_GET_INFO;	/* bswap bug */
	sc->sc_regs->SizeAndFlags = (sizeof buffer) | ES_F_CMD;
	sc->sc_regs->BufferAddressHi32 = 0;
	sc->sc_regs->BufferAddressLo32 = phys; /* go! */

	for (i = 0; i < 100; i++) {
		DELAY(100);
		if ((sc->sc_regs->Control & EC_OF_EMPTY) == 0)
			break;
	}
	if (i == 100)
		return 0;

	phys2 = sc->sc_regs->BufferAddressLo32;
	if (phys2 != phys) {
		printf("enic uhu2? %llx != %llx?\n",
		    (long long)phys, (long long)phys2);
		return 0;
	}
#if 0
	printf("enic: hwinfo: %x %x %x %x %x %x \n",
	    hw->InputFifoSize, hw->OutputFifoSize, hw->CompletionFifoSize,
	    hw->ErrorCount, hw->FramesDropped, hw->Reserved);
#endif

	/*
	 * Get FIFO depths and cap them
	 */
	sc->sc_n_recv = hw->InputFifoSize;
	if (sc->sc_n_recv > SC_MAX_N_RECV)
		sc->sc_n_recv = SC_MAX_N_RECV;
	if (sc->sc_n_recv == 0) { /* sanity and compat with old hw/simulator */
		sc->sc_n_recv = 8;
		sc->sc_n_xmit = 4;
	} else {
		sc->sc_n_xmit = hw->OutputFifoSize;
		if (sc->sc_n_xmit > SC_MAX_N_XMIT)
			sc->sc_n_xmit = SC_MAX_N_XMIT;
	}

	return 1;
}

void
enic_reset(struct ifnet *ifp)
{
	int s;

	s = splnet();
	enic_stop(ifp,0);
	enic_init(ifp);
	splx(s);
}

void
enic_stop(struct ifnet *ifp, int suspend)
{
	struct enic_softc *sc = ifp->if_softc;

#if 0
	printf("enic_stop %x\n", sc->sc_regs->Control);
#endif

	/*
	 * NB: only "ifconfig down" says suspend=1 (then "up" calls init)
	 * Could simply set RXDIS in this case
	 */
	sc->inited = 2;
	sc->sc_regs->Control = EC_RESET;
	sc->sc_no_rd = 0; /* they are gone */
	sc->sc_no_td = 0; /* they are gone */
}

void
enic_shutdown(void *arg)
{
	struct ifnet *ifp = arg;

	enic_stop(ifp, 0);
}

void
enic_kill_xmit(struct enic_softc *sc)
{
	int i;
	struct mbuf *m;

	for (i = 0; i < sc->sc_n_xmit; i++) {
		if ((m = sc->sc_xmit[i].mbuf) != NULL) {
			sc->sc_xmit[i].mbuf = NULL;
			sc->sc_xmit[i].phys = ~0;
			m_freem(m);
		}
	}
	sc->sc_no_td = 0;
	sc->sc_xmit_h = 0;
}

void
enic_post_recv(struct enic_softc *sc, struct mbuf *m)
{
	int i, waitmode = M_DONTWAIT;
	paddr_t phys;

#define rpostone(_p_) \
    sc->sc_regs->SizeAndFlags = ES_F_RECV | MCLBYTES; \
    sc->sc_regs->BufferAddressHi32 = 0; \
    sc->sc_regs->BufferAddressLo32 = _p_;
#define tpostone(_p_,_s_) \
    sc->sc_regs->SizeAndFlags = ES_F_XMIT | (_s_); \
    sc->sc_regs->BufferAddressHi32 = 0; \
    sc->sc_regs->BufferAddressLo32 = _p_;

	/* Operational reload? */
	if (m != NULL) {
		/* But is the hw ready for it */
		if (sc->sc_regs->Control & EC_IF_FULL)
			goto no_room;
		/* Yes, find a spot. Include empty q case. */
		for (i = sc->sc_recv_h; i < sc->sc_n_recv; i++)
			if (sc->sc_recv[i].mbuf == NULL)
				goto found;
		for (i = 0; i < sc->sc_recv_h; i++)
			if (sc->sc_recv[i].mbuf == NULL)
				goto found;
		/* no spot, drop it (sigh) */
 no_room:
#if DEBUG
		sc->rf++;
#endif
		m_freem(m);
		return;
 found:
		phys = kvtophys((vaddr_t)m->m_data);
		sc->sc_recv[i].mbuf = m;
		sc->sc_recv[i].phys = phys;
		rpostone(phys);
		sc->sc_no_rd++;
		return;
	}

	/* Repost after reset? */
	if (sc->inited) {
		/* order doesnt matter, might as well keep it clean */
		int j = 0;
		sc->sc_recv_h = 0;
		for (i = 0; i < sc->sc_n_recv; i++)
			if ((m = sc->sc_recv[i].mbuf) != NULL) {
				phys = sc->sc_recv[i].phys;
				sc->sc_recv[i].mbuf = NULL;
				sc->sc_recv[i].phys = ~0;
				sc->sc_recv[j].mbuf = m;
				sc->sc_recv[j].phys = phys;
#if DEBUG
				if (sc->sc_regs->Control & EC_IF_FULL)
					printf("?uhu? postrecv full? %d\n",
					    sc->sc_no_rd);
#endif
				sc->sc_no_rd++;
				rpostone(phys);
				j++;
			}
		/* Any holes left? */
		sc->inited = 1;
		if (j >= sc->sc_n_recv)
			return;	/* no, we are done */
		/* continue on with the loop below */
		i = j; m = NULL;
		goto fillem;
	}

	/* Initial refill, we can wait */
	waitmode = M_WAIT;
	sc->sc_recv_h = 0;
	memset(sc->sc_recv, 0, sizeof(sc->sc_recv[0]) * sc->sc_n_recv);
	i = 0;
 fillem:
	for (; i < sc->sc_n_recv; i++) {
		MGETHDR(m, waitmode, MT_DATA);
		if (m == 0)
			break;
		m_set_rcvif(m, &sc->sc_ethercom.ec_if);
		m->m_pkthdr.len = 0;

		MCLGET(m, waitmode);
		if ((m->m_flags & M_EXT) == 0)
			break;

		/*
		 * This offset aligns IP/TCP headers and helps performance
		 */
#if 1
#define ADJUST_MBUF_OFFSET(_m_) { \
	(_m_)->m_data += 2; \
	(_m_)->m_len -= 2; \
}
#else
#define ADJUST_MBUF_OFFSET(_m_)
#endif

		m->m_len = MCLBYTES;

		ADJUST_MBUF_OFFSET(m);
		phys = kvtophys((vaddr_t)m->m_data);
		sc->sc_recv[i].mbuf = m;
		sc->sc_recv[i].phys = phys;
#if DEBUG
		if (sc->sc_regs->Control & EC_IF_FULL)
			printf("?uhu? postrecv2 full? %d\n", sc->sc_no_rd);
#endif
		sc->sc_no_rd++;
		rpostone(phys);
		m = NULL;
	}

	if (m)
		m_freem(m);
	sc->inited = 1;
}

void enic_refill(struct enic_softc *sc)
{
	struct mbuf *m;
	int waitmode = M_DONTWAIT;

	MGETHDR(m, waitmode, MT_DATA);
	if (m == NULL)
		return;
	m_set_rcvif(m, &sc->sc_ethercom.ec_if);
	m->m_pkthdr.len = 0;

	MCLGET(m, waitmode);
	if ((m->m_flags & M_EXT) == 0) {
		m_freem(m);
		return;
	}

	m->m_len = MCLBYTES;
	ADJUST_MBUF_OFFSET(m);

	enic_post_recv(sc,m);
}

int
enic_init(struct ifnet *ifp)
{
	struct enic_softc *sc = ifp->if_softc;
	uint32_t ctl;

	/* no need to init many times unless we are in reset */
	if (sc->inited != 1) {

		/* Cancel all xmit buffers */
		enic_kill_xmit(sc);

		/* Re-post all recv buffers */
		enic_post_recv(sc,NULL);
	}

	/* Start the eNIC */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	ifp->if_timer = 0;
	ctl = sc->sc_regs->Control | EC_INTEN;
	ctl &= ~EC_RXDIS;
	sc->sc_regs->Control = ctl;
#if 0
	printf("enic_init <- %x\n",ctl);
#endif

	if_schedule_deferred_start(ifp);

	return 0;
}


void
enic_watchdog(struct ifnet *ifp)
{
	struct enic_softc *sc = ifp->if_softc;

#if 0
	printf("enic_watch ctl=%x\n", sc->sc_regs->Control);
#endif
	log(LOG_ERR, "%s: device timeout\n", device_xname(sc->sc_dev));
	++ifp->if_oerrors;

	enic_reset(ifp);
}

int
enic_mediachange(struct ifnet *ifp)
{
#if 0
	struct enic_softc *sc = ifp->if_softc;
#endif
	/* more code here.. */

	return 0;
}

void
enic_mediastatus(struct ifnet *ifp, struct ifmediareq *ifmr)
{
	struct enic_softc *sc = ifp->if_softc;

	if ((ifp->if_flags & IFF_UP) == 0)
		return;

	ifmr->ifm_status = IFM_AVALID;
	if (sc->sc_havecarrier)
		ifmr->ifm_status |= IFM_ACTIVE;

	/* more code here someday.. */
}

/*
 * Process an ioctl request.
 */
int
enic_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct enic_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {
	case SIOCGIFMEDIA:
	case SIOCSIFMEDIA:
#if 0 /*DEBUG*/
	    {
		extern int ei_drops[];
		static int flip = 0;
		if (flip++ == 2) {
			int i;
			flip = 0;
			printf("enic_ioctl(%x) %qd/%qd %qd/%qd %d/%d %d:%d "
			    "%d+%d %d/%d/%d\n", ifp->if_flags,
			    ifp->if_ierrors, ifp->if_oerrors,
			    ifp->if_ipackets, ifp->if_opackets,
			    sc->sc_no_rd, sc->sc_no_td,
			    sc->xhit, sc->xmiss,
			    sc->tfull, sc->tfull2,
			    sc->brh, sc->rf, sc->bxh);
			printf(" Ctl %x lt %x tim %d\n",
			    sc->sc_regs->Control, sc->it, ifp->if_timer);

			for (i = 0; i < 64; i++)
				if (ei_drops[i])
					printf(" %d.%d",i,ei_drops[i]);
			printf("\n");
		}
	    }
#endif
		error = ifmedia_ioctl(ifp, ifr, &sc->sc_media, cmd);
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
		if (cmd == SIOCSIFADDR) {
			/*
			 * hackattack: NFS does not turn us back
			 * on after a stop. So.
			 */
#if 0
			printf("enic_ioctl(%lx)\n",cmd);
#endif
			enic_init(ifp);
		}
		if (error != ENETRESET)
			break;
		error = 0;
		if (cmd != SIOCADDMULTI && cmd != SIOCDELMULTI)
			break;
		if (ifp->if_flags & IFF_RUNNING) {
			enic_reset(ifp);
		}
		break;
	}
	splx(s);

	return error;
}

int
enic_intr(void *cookie, void *f)
{
	struct enic_softc *sc = cookie;
	uint32_t isr, saf, hi, lo, fl;

	isr = sc->sc_regs->Control;

	/* Make sure there is one and that we should take it */
	if ((isr & (EC_INTEN|EC_DONE)) != (EC_INTEN|EC_DONE))
		return 0;

	if (isr & EC_ERROR) {
		printf("%s: internal error\n", device_xname(sc->sc_dev));
		enic_reset(&sc->sc_ethercom.ec_if);
		return 1;
	}

	/*
	 * pull out all completed buffers
	 */
	while ((isr & EC_OF_EMPTY) == 0) {

		/* beware, order matters */
		saf = sc->sc_regs->SizeAndFlags;
		hi  = sc->sc_regs->BufferAddressHi32; /* BUGBUG 64bit */
		lo  = sc->sc_regs->BufferAddressLo32; /* this pops the fifo */
		__USE(hi);

		fl = saf & (ES_F_MASK &~ ES_F_DONE);
		if (fl == ES_F_RECV)
			enic_rint(sc,saf,lo);
		else if (fl == ES_F_XMIT)
			enic_tint(sc,saf,lo);
		else {
			/*
			 * we do not currently expect or care for
			 * command completions?
			 */
			if (fl != ES_F_CMD)
				printf("%s: invalid saf=x%x (lo=%x)\n",
				    device_xname(sc->sc_dev), saf, lo);
		}

		isr = sc->sc_regs->Control;
	}

	rnd_add_uint32(&sc->rnd_source, isr);

	return 1;
}

void
enic_rint(struct enic_softc *sc, uint32_t saf, paddr_t phys)
{
	struct mbuf *m;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int len = saf & ES_S_MASK, i;

	/* Find what buffer it is. Should be the first. */
	for (i = sc->sc_recv_h; i < sc->sc_n_recv; i++)
		if (sc->sc_recv[i].phys == phys)
			goto found;
	for (i = 0; i < sc->sc_recv_h; i++)
		if (sc->sc_recv[i].phys == phys)
			goto found;

	/* uhu?? */
	printf("%s: bad recv phys %llx\n", device_xname(sc->sc_dev),
	    (long long)phys);
	ifp->if_ierrors++;
	return;

	/* got it, pop it */
 found:
	sc->sc_no_rd--;
	m = sc->sc_recv[i].mbuf;
	sc->sc_recv[i].mbuf = NULL;
	sc->sc_recv[i].phys = ~0;
	if (i == sc->sc_recv_h) { /* should be */
		sc->sc_recv_h = (++i == sc->sc_n_recv) ? 0 : i;
	}
#if DEBUG
	else
		sc->brh++;
#endif

	if (len <= sizeof(struct ether_header) ||
	    len > ((sc->sc_ethercom.ec_capenable & ETHERCAP_VLAN_MTU) ?
		ETHER_VLAN_ENCAP_LEN + ETHERMTU + sizeof(struct ether_header) :
		ETHERMTU + sizeof(struct ether_header))) {
		ifp->if_ierrors++;

		/* reuse it */
		enic_post_recv(sc,m);
		return;
	}

	/* Adjust size */
	m->m_pkthdr.len = len;
	m->m_len = len; /* recheck */

	/* Pass the packet up. */
	if_percpuq_enqueue(ifp->if_percpuq, m);

	/* Need to refill now */
	enic_refill(sc);
}

void enic_tint(struct enic_softc *sc, uint32_t saf, paddr_t phys)
{
	struct mbuf *m;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i;

#if DEBUG
	sc->it = 1;
#endif

	/* BUGBUG should there be a per-buffer error bit in SAF? */

	/* Find what buffer it is. Should be the first. */
	for (i = sc->sc_xmit_h; i < sc->sc_n_xmit; i++)
		if (sc->sc_xmit[i].phys == phys)
			goto found;
	for (i = 0; i < sc->sc_xmit_h; i++)
		if (sc->sc_xmit[i].phys == phys)
			goto found;

	/* uhu?? */
	printf("%s: bad xmit phys %llx\n", device_xname(sc->sc_dev),
	    (long long)phys);
	ifp->if_oerrors++;
	return;

	/* got it, pop it */
 found:
	m = sc->sc_xmit[i].mbuf;
	sc->sc_xmit[i].mbuf = NULL;
	sc->sc_xmit[i].phys = ~0;
	if (i == sc->sc_xmit_h) { /* should be */
		sc->sc_xmit_h = (++i == sc->sc_n_xmit) ? 0 : i;
	}
#if DEBUG
	else
		sc->bxh++;
#endif
	m_freem(m);
	ifp->if_opackets++;

	if (--sc->sc_no_td == 0)
		ifp->if_timer = 0;

	ifp->if_flags &= ~IFF_OACTIVE;
	if_schedule_deferred_start(ifp);
#if DEBUG
	sc->it = 1;
#endif
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue, and map it to the
 * interface before starting the output.
 * Called only at splnet or interrupt level.
 */
void
enic_start(struct ifnet *ifp)
{
	struct enic_softc *sc = ifp->if_softc;
	struct mbuf *m;
	int len, ix, s;
	paddr_t phys;

#if DEBUG
	sc->it = 0;
#endif

#if 0
	printf("enic_start(%x)\n", ifp->if_flags);
#endif

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	s = splnet();	/* I know, I dont trust people.. */

	ix = sc->sc_xmit_h;
	for (;;) {

		/* Anything to do? */
		IFQ_POLL(&ifp->if_snd, m);
		if (m == NULL)
			break;

		/* find a spot, if any */
		for (; ix < sc->sc_n_xmit; ix++)
			if (sc->sc_xmit[ix].mbuf == NULL)
				goto found;
		for (ix = 0; ix < sc->sc_xmit_h; ix++)
			if (sc->sc_xmit[ix].mbuf == NULL)
				goto found;
		/* oh well */
		ifp->if_flags |= IFF_OACTIVE;
#if DEBUG
		sc->tfull++;
#endif
		break;

 found:
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		/*
		 * If BPF is listening on this interface, let it see the packet
		 * before we commit it to the wire.
		 */
		bpf_mtap(ifp, m);

		/*
		 * Copy the mbuf chain into a contiguous transmit buffer.
		 */
		len = enic_put(sc, &m);
		if (len == 0)
			break;	/* sanity */
		if (len > (ETHERMTU + sizeof(struct ether_header))) {
			printf("enic? tlen %d > %d\n",
			    len, ETHERMTU + sizeof(struct ether_header));
			len = ETHERMTU + sizeof(struct ether_header);
		}

		ifp->if_timer = 5;

		/*
		 * Remember and post the buffer
		 */
		phys = kvtophys((vaddr_t)m->m_data);
		sc->sc_xmit[ix].mbuf = m;
		sc->sc_xmit[ix].phys = phys;

		sc->sc_no_td++;

		tpostone(phys,len);

		if (sc->sc_regs->Control & EC_IF_FULL) {
			ifp->if_flags |= IFF_OACTIVE;
#if DEBUG
			sc->tfull2++;
#endif
			break;
		}

		ix++;
	}

	splx(s);
}

int enic_put(struct enic_softc *sc, struct mbuf **pm)
{
	struct mbuf *n, *m = *pm, *mm;
	int len, tlen = 0, xlen = m->m_pkthdr.len;
	uint8_t *cp;

#if 0
	/* drop garbage */
	tlen = xlen;
	for (; m; m = n) {
		len = m->m_len;
		if (len == 0) {
			n = m_free(m);
			if (m == *pm)
				*pm = n;
			continue;
		}
		tlen -= len;
		KASSERT(m != m->m_next);
		n = m->m_next;
		if (tlen <= 0)
			break;
	}

	/*
	 * We might be done:
	 * (a) empty chain (b) only one segment (c) bad chain
	 */
	if (((m = *pm) == NULL) || (tlen > 0))
		xlen = 0;
#endif

	if ((xlen == 0) || (xlen <= m->m_len)) {
#if DEBUG
		sc->xhit++;
#endif
		return xlen;
	}

	/* Nope, true chain. Copy to contig :-(( */
	tlen = xlen;
	MGETHDR(n, M_NOWAIT, MT_DATA);
	if (n == NULL)
		goto Bad;
	m_set_rcvif(n, &sc->sc_ethercom.ec_if);
	n->m_pkthdr.len = tlen;

	MCLGET(n, M_NOWAIT);
	if ((n->m_flags & M_EXT) == 0) {
		m_freem(n);
		goto Bad;
	}

	n->m_len = tlen;
	cp = mtod(n, uint8_t *);
	for (; m && tlen; m = mm) {

		len = m->m_len;
		if (len > tlen)
			len = tlen;
		if (len)
			memcpy(cp, mtod(m, void *), len);

		cp += len;
		tlen -= len;
		mm = m_free(m);

	}

	*pm = n;
#if DEBUG
	sc->xmiss++;
#endif
	return (xlen);

 Bad:
	printf("enic_put: no mem?\n");
	m_freem(m);
	return 0;
}
