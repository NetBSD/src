/*
 * Copyright (c) 1982, 1990 The Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)if_le.c	7.6 (Berkeley) 5/8/91
 *	$Id: if_le.c,v 1.6 1994/06/21 04:02:15 chopps Exp $
 */

#include "le.h"
#if NLE > 0

#include "bpfilter.h"

/*
 * AMD 7990 LANCE
 *
 * This driver will generate and accept tailer encapsulated packets even
 * though it buys us nothing.  The motivation was to avoid incompatibilities
 * with VAXen, SUNs, and others that handle and benefit from them.
 * This reasoning is dubious.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/cpu.h>
#include <machine/mtpr.h>
#include <amiga/amiga/device.h>
#include <amiga/dev/ztwobusvar.h>
#include <amiga/dev/if_lereg.h>

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * le_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	le_softc {
	struct	arpcom sc_ac;	/* common Ethernet structures */
#define	sc_if	sc_ac.ac_if	/* network-visible interface */
#define	sc_addr	sc_ac.ac_enaddr	/* hardware Ethernet address */
	void	*sc_base;	/* base address of board */
	struct	lereg1 *sc_r1;	/* LANCE registers */
	struct	lereg2 *sc_r2;	/* dual-port RAM */
	int	sc_rmd;		/* predicted next rmd to process */
	int	sc_runt;
	int	sc_jab;
	int	sc_merr;
	int	sc_babl;
	int	sc_cerr;
	int	sc_miss;
	int	sc_xint;
	int	sc_xown;
	int	sc_uflo;
	int	sc_rxlen;
	int	sc_rxoff;
	int	sc_txoff;
	int	sc_busy;
	short	sc_iflags;
#if NBPFILTER > 0
	caddr_t sc_bpf;
#endif
} le_softc[NLE];

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

/* offsets for:	   ID,   REGS,    MEM */
int	lestd[] = { 0, 0x4000, 0x8000 };

/* console error messages */
int	ledebug = 0;

int	leintr(), lestart(), leioctl(), ether_output();
void	leinit();

struct	mbuf *leget();
extern	struct ifnet loif;

void leattach __P((struct device *, struct device *, void *));
int lematch __P((struct device *, struct cfdata *, void *args));

struct cfdriver lecd = {
	NULL, "le", lematch, leattach, DV_IFNET,
	sizeof(struct le_softc), NULL, 0};

int
lematch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{

	struct ztwobus_args *zap;

	zap = (struct ztwobus_args *)auxp;

	/* Commodore ethernet card */
	if ( zap->manid == 514 && zap->prodid == 112)
		return(1);

	/* Ameristar ethernet card */
	if ( zap->manid == 1053 && zap->prodid == 1)
		return(1);

	return (0);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
leattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	register struct lereg0 *ler0;
	register struct lereg2 *ler2;
	struct ztwobus_args *zap;
	struct lereg2 *lemem = (struct lereg2 *) 0x8000;
	struct le_softc *le = &le_softc[dp->dv_unit];
	struct ifnet *ifp = &le->sc_if;
	char *cp;
	int i;
	unsigned long ser;
	int s = splhigh ();

	zap =(struct ztwobus_args *)auxp;

	/*
	 * Make config msgs look nicer.
	 */
	printf("\n");

	ler0 = le->sc_base = zap->va;
	le->sc_r1 = (struct lereg1 *)(lestd[1] + (int)zap->va);
	ler2 = le->sc_r2 = (struct lereg2 *)(lestd[2] + (int)zap->va);

	/*
	 * Manufacturer decides the 3 first bytes, i.e. ethernet vendor ID.
	 */
	if ( zap->manid == 514 && zap->prodid == 112) {
	    /* Commodore 2065 */
	    le->sc_addr[0] = 0x00;
	    le->sc_addr[1] = 0x80;
	    le->sc_addr[2] = 0x10;
	}
	if ( zap->manid == 1053 && zap->prodid == 1) {
	    le->sc_addr[0] = 0x00;
	    le->sc_addr[1] = 0x00;
	    le->sc_addr[2] = 0x9f;
	}

	/*
	 * Serial number for board is used as host ID.
	 */
	ser = (unsigned long) zap->serno;

	le->sc_addr[3] = (ser >> 16) & 0xff;
	le->sc_addr[4] = (ser >>  8) & 0xff;
	le->sc_addr[5] = (ser      ) & 0xff;

	printf("le%d: hardware address %s\n", dp->dv_unit,
		ether_sprintf(le->sc_addr));

	/*
	 * Setup for transmit/receive
	 */
	ler2->ler2_mode = LE_MODE;
	ler2->ler2_padr[0] = le->sc_addr[1];
	ler2->ler2_padr[1] = le->sc_addr[0];
	ler2->ler2_padr[2] = le->sc_addr[3];
	ler2->ler2_padr[3] = le->sc_addr[2];
	ler2->ler2_padr[4] = le->sc_addr[5];
	ler2->ler2_padr[5] = le->sc_addr[4];
	ler2->ler2_ladrf0 = 0;
	ler2->ler2_ladrf1 = 0;
	ler2->ler2_rlen = LE_RLEN;
	ler2->ler2_rdra = (int)lemem->ler2_rmd;
	ler2->ler2_tlen = LE_TLEN;
	ler2->ler2_tdra = (int)lemem->ler2_tmd;

	splx (s);

	ifp->if_unit = dp->dv_unit;
	ifp->if_name = "le";
	ifp->if_mtu = ETHERMTU;
	ifp->if_ioctl = leioctl;
	ifp->if_output = ether_output;
	ifp->if_start = lestart;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX;

#if NBPFILTER > 0
	bpfattach(&le->sc_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	if_attach(ifp);
	ether_ifattach(ifp);

	return;
}

ledrinit(ler2)
	register struct lereg2 *ler2;
{
	register struct lereg2 *lemem = (struct lereg2 *) 0x8000;
	register int i;

	for (i = 0; i < LERBUF; i++) {
		ler2->ler2_rmd[i].rmd0 = (int)lemem->ler2_rbuf[i];
		ler2->ler2_rmd[i].rmd1 = LE_OWN;
		ler2->ler2_rmd[i].rmd2 = -LEMTU;
		ler2->ler2_rmd[i].rmd3 = 0;
	}

	for (i = 0; i < LETBUF; i++) {
		ler2->ler2_tmd[i].tmd0 = (int)lemem->ler2_tbuf[i];
		ler2->ler2_tmd[i].tmd1 = 0;
		ler2->ler2_tmd[i].tmd2 = 0;
		ler2->ler2_tmd[i].tmd3 = 0;
	}
}

void
lereset(unit)
	register int unit;
{
	register struct le_softc *le = &le_softc[unit];
	register struct lereg1 *ler1 = le->sc_r1;
	/*
	 * This structure is referenced from the CARDS/LANCE point of
	 * view, thus the 0x8000 address which is the buffer RAM area of
	 * the Commodore and Ameristar cards. This pointer is manipulated
	 * with the LANCE's view of memory and NOT the Amiga's. FYI.
	 */
	register struct lereg2 *lemem = (struct lereg2 *) 0x8000;

	register int timo = 100000;
	register int stat;

#ifdef lint
	stat = unit;
#endif
#if NBPFILTER > 0
	if (le->sc_if.if_flags & IFF_PROMISC)
		/* set the promiscuous bit */
		le->sc_r2->ler2_mode = LE_MODE|0x8000;
	else
		le->sc_r2->ler2_mode = LE_MODE;
#endif
	 ler1->ler1_rap =  LE_CSR0;
	 ler1->ler1_rdp =  LE_STOP;

	 ledrinit(le->sc_r2);

	 le->sc_rmd = 0;
	 ler1->ler1_rap =  LE_CSR1;
	 ler1->ler1_rdp =  (int)&lemem->ler2_mode;
	 ler1->ler1_rap =  LE_CSR2;
	 ler1->ler1_rdp =  0;
	 ler1->ler1_rap =  LE_CSR0;
	 ler1->ler1_rdp =  LE_INIT;

	do {
		if (--timo == 0) {
			printf("le%d: init timeout, stat = 0x%x\n",
			       unit, stat);
			break;
		}
		 stat =  ler1->ler1_rdp;
	} while ((stat & LE_IDON) == 0);

	 ler1->ler1_rdp =  LE_STOP;
	 ler1->ler1_rap =  LE_CSR3;
	 ler1->ler1_rdp =  LE_BSWP;
	 ler1->ler1_rap =  LE_CSR0;
	 ler1->ler1_rdp =  LE_STRT | LE_INEA;
	 le->sc_if.if_flags &= ~IFF_OACTIVE;

	return;
}

/*
 * Initialization of interface
 */
void
leinit(unit)
	int unit;
{
	struct le_softc *le = &le_softc[unit];
	register struct ifnet *ifp = &le->sc_if;
	int s;

	/* not yet, if address still unknown */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		s = splimp();
		ifp->if_flags |= IFF_RUNNING;
		lereset(unit);
	        (void) lestart(ifp);
		splx(s);
	}

	return;
}

/*
 * Start output on interface.  Get another datagram to send
 * off of the interface queue, and copy it to the interface
 * before starting the output.
 */
lestart(ifp)
	struct ifnet *ifp;
{
	register struct le_softc *le = &le_softc[ifp->if_unit];
	register struct letmd *tmd;
	register struct mbuf *m;
	int len;

	if ((le->sc_if.if_flags & IFF_RUNNING) == 0)
		return (0);

	IF_DEQUEUE(&le->sc_if.if_snd, m);
	if (m == 0)
		return (0);

	len = leput(le->sc_r2->ler2_tbuf[0], m);

#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire.
	 */
	if (le->sc_bpf)
                bpf_tap(le->sc_bpf, le->sc_r2->ler2_tbuf[0], len);
#endif

	tmd = le->sc_r2->ler2_tmd;
	tmd->tmd3 = 0;
	tmd->tmd2 = -len;
	tmd->tmd1 = LE_OWN | LE_STP | LE_ENP;
	le->sc_if.if_flags |= IFF_OACTIVE;

	return (0);
}

leintr(unit)
	register int unit;
{
	register struct le_softc *le = &le_softc[unit];
	register struct lereg1 *ler1;
	register int stat;

	/* if not even initialized, don't do anything further.. */
	if (! le->sc_base)
	  return 0;

	ler1 = le->sc_r1;
	stat =  ler1->ler1_rdp;

	if (! (stat & LE_INTR))
	  return 0;

	if (stat & LE_SERR) {
		leerror(unit, stat);
		if (stat & LE_MERR) {
			le->sc_merr++;
			lereset(unit);
			return(1);
		}
		if (stat & LE_BABL)
			le->sc_babl++;
		if (stat & LE_CERR)
			le->sc_cerr++;
		if (stat & LE_MISS)
			le->sc_miss++;
		 ler1->ler1_rdp =  LE_BABL|LE_CERR|LE_MISS|LE_INEA;
	}
	if ((stat & LE_RXON) == 0) {
		le->sc_rxoff++;
		lereset(unit);
		return(1);
	}
	if ((stat & LE_TXON) == 0) {
		le->sc_txoff++;
		lereset(unit);
		return(1);
	}
	if (stat & LE_RINT) {
		/* interrupt is cleared in lerint */
		lerint(unit);
	}
	if (stat & LE_TINT) {
		 ler1->ler1_rdp =  LE_TINT|LE_INEA;
		lexint(unit);
	}
	return(1);
}

/*
 * Ethernet interface transmitter interrupt.
 * Start another output if more data to send.
 */
lexint(unit)
	register int unit;
{
	register struct le_softc *le = &le_softc[unit];
	register struct letmd *tmd = le->sc_r2->ler2_tmd;

	if ((le->sc_if.if_flags & IFF_OACTIVE) == 0) {
		le->sc_xint++;
		return;
	}
	if (tmd->tmd1 & LE_OWN) {
		le->sc_xown++;
		return;
	}
	if (tmd->tmd1 & LE_ERR) {
err:
		lexerror(unit);
		le->sc_if.if_oerrors++;
		if (tmd->tmd3 & (LE_TBUFF|LE_UFLO)) {
			le->sc_uflo++;
			lereset(unit);
		}
		else if (tmd->tmd3 & LE_LCOL)
			le->sc_if.if_collisions++;
		else if (tmd->tmd3 & LE_RTRY)
			le->sc_if.if_collisions += 16;
	}
	else if (tmd->tmd3 & LE_TBUFF)
		/* XXX documentation says BUFF not included in ERR */
		goto err;
	else if (tmd->tmd1 & LE_ONE)
		le->sc_if.if_collisions++;
	else if (tmd->tmd1 & LE_MORE)
		/* what is the real number? */
		le->sc_if.if_collisions += 2;
	else
		le->sc_if.if_opackets++;

	le->sc_if.if_flags &= ~IFF_OACTIVE;

	(void) lestart(&le->sc_if);
}

#define	LENEXTRMP \
	if (++bix == LERBUF) bix = 0, rmd = le->sc_r2->ler2_rmd; else ++rmd

/*
 * Ethernet interface receiver interrupt.
 * If input error just drop packet.
 * Decapsulate packet based on type and pass to type specific
 * higher-level input routine.
 */
lerint(unit)
	int unit;
{
	register struct le_softc *le = &le_softc[unit];
	register int bix = le->sc_rmd;
	register struct lermd *rmd = &le->sc_r2->ler2_rmd[bix];

	/*
	 * Out of sync with hardware, should never happen?
	 */
	if (rmd->rmd1 & LE_OWN) {
		 le->sc_r1->ler1_rdp =  LE_RINT|LE_INEA;
		return;
	}

	/*
	 * Process all buffers with valid data
	 */
	while ((rmd->rmd1 & LE_OWN) == 0) {
		int len = rmd->rmd3;

		/* Clear interrupt to avoid race condition */
		 le->sc_r1->ler1_rdp =  LE_RINT|LE_INEA;

		if (rmd->rmd1 & LE_ERR) {
			le->sc_rmd = bix;
			lererror(unit, "bad packet");
			le->sc_if.if_ierrors++;
		} else if ((rmd->rmd1 & (LE_STP|LE_ENP)) != (LE_STP|LE_ENP)) {
			/*
			 * Find the end of the packet so we can see how long
			 * it was.  We still throw it away.
			 */
			do {
			        le->sc_r1->ler1_rdp = LE_RINT|LE_INEA;
				rmd->rmd3 = 0;
				rmd->rmd1 = LE_OWN;
				LENEXTRMP;
			} while (!(rmd->rmd1 & (LE_OWN|LE_ERR|LE_STP|LE_ENP)));

			le->sc_rmd = bix;
			lererror(unit, "chained buffer");
			le->sc_rxlen++;

			/*
			 * If search terminated without successful completion
			 * we reset the hardware (conservative).
			 */
			if ((rmd->rmd1 & (LE_OWN|LE_ERR|LE_STP|LE_ENP)) != LE_ENP) {
				lereset(unit);
				return;
			}
		} else
			leread(unit, le->sc_r2->ler2_rbuf[bix], len);

		rmd->rmd3 = 0;
		rmd->rmd1 = LE_OWN;
		LENEXTRMP;

	}

	le->sc_rmd = bix;
}

leread(unit, buf, len)
	int unit;
	char *buf;
	int len;
{
	register struct le_softc *le = &le_softc[unit];
	register struct ether_header *et;
    	struct mbuf *m;
	int off, resid;

	le->sc_if.if_ipackets++;

	et = (struct ether_header *)buf;
	et->ether_type = ntohs((u_short)et->ether_type);

	/* adjust input length to account for header and CRC */
	len = len - sizeof(struct ether_header) - 4;

#define	ledataaddr(et, off, type)	((type)(((caddr_t)((et)+1)+(off))))
	if (et->ether_type >= ETHERTYPE_TRAIL &&
	    et->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (et->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;		/* sanity */
		et->ether_type = ntohs(*ledataaddr(et, off, u_short *));
		resid = ntohs(*(ledataaddr(et, off+2, u_short *)));
		if (off + resid > len)
			return;		/* sanity */
		len = off + resid;
	} else
		off = 0;

	if (len <= 0) {
		if (ledebug)
			log(LOG_WARNING,
			    "le%d: ierror(runt packet): from %s: len=%d\n",
			    unit, ether_sprintf(et->ether_shost), len);
		le->sc_runt++;
		le->sc_if.if_ierrors++;
		return;
	}
#if NBPFILTER > 0
	/*
	 * Check if there's a bpf filter listening on this interface.
	 * If so, hand off the raw packet to bpf, which must deal with
	 * trailers in its own way.
	 */
	if (le->sc_bpf) {
		bpf_tap(le->sc_bpf, buf, len + sizeof(struct ether_header));

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no bpf listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 *
		 * XXX This test does not support multicasts.
		 */
		if ((le->sc_if.if_flags & IFF_PROMISC)
		    && bcmp(et->ether_dhost, le->sc_addr, 
			    sizeof(et->ether_dhost)) != 0
		    && bcmp(et->ether_dhost, etherbroadcastaddr, 
			    sizeof(et->ether_dhost)) != 0)
			return;
	}
#endif
	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; leget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = leget(buf, len, off, &le->sc_if);
	if (m == 0)
		return;

	ether_input(&le->sc_if, et, m);
}

/*
 * Routine to copy from mbuf chain to transmit
 * buffer in board local memory.
 */
leput(lebuf, m)
	register char *lebuf;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register int len, tlen = 0;

	for (mp = m; mp; mp = mp->m_next) {
		len = mp->m_len;
		if (len == 0)
			continue;
		tlen += len;
		bcopy(mtod(mp, char *), lebuf, len);
		lebuf += len;
	}

	m_freem(m);

	if (tlen < LEMINSIZE) {
		bzero(lebuf, LEMINSIZE - tlen);
		tlen = LEMINSIZE;
	}

	return(tlen);
}

/*
 * Routine to copy from board local memory into mbufs.
 */
struct mbuf *
leget(lebuf, totlen, off0, ifp)
	char *lebuf;
	int totlen, off0;
	struct ifnet *ifp;
{
	register struct mbuf *m;
	struct mbuf *top = 0, **mp = &top;
	register int off = off0, len;
	register char *cp;
	char *epkt;

	lebuf += sizeof (struct ether_header);
	cp = lebuf;
	epkt = cp + totlen;
	if (off) {
		cp += off + 2 * sizeof(u_short);
		totlen -= 2 * sizeof(u_short);
	}

	MGETHDR(m, M_DONTWAIT, MT_DATA);

	if (m == 0)
		return (0);

	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	m->m_len = MHLEN;

	while (totlen > 0) {

		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return (0);
			}
			m->m_len = MLEN;
		}

		len = min(totlen, epkt - cp);
		if (len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				m->m_len = len = min(len, MCLBYTES);
			else
				len = m->m_len;
		} else {
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (len < m->m_len) {
				if (top == 0 && len + max_linkhdr <= m->m_len)
					m->m_data += max_linkhdr;
				m->m_len = len;
			} else
				len = m->m_len;
		}

		bcopy(cp, mtod(m, caddr_t), (unsigned)len);
		cp += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;

		if (cp == epkt)
			cp = lebuf;

	}

	return (top);
}

/*
 * Process an ioctl request.
 */
leioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct le_softc *le = &le_softc[ifp->if_unit];
	struct lereg1 *ler1 = le->sc_r1;
	int s = splimp(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			leinit(ifp->if_unit);	/* before arpwhohas */
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

			if (ns_nullhost(*ina))
				ina->x_host = *(union ns_host *)(le->sc_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)le->sc_addr, sizeof(le->sc_addr));
			}
			leinit(ifp->if_unit); /* does le_setaddr() */
			break;
		    }
#endif
		default:
			leinit(ifp->if_unit);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    ifp->if_flags & IFF_RUNNING) {
			 ler1->ler1_rdp =  LE_STOP;
			ifp->if_flags &= ~IFF_RUNNING;
		} else if (ifp->if_flags & IFF_UP && (ifp->if_flags & IFF_RUNNING) == 0)
			leinit(ifp->if_unit);

		/*
		 * If the state of the promiscuous bit changes, the interface
		 * must be reset to effect the change.
		 */
		if (((ifp->if_flags ^ le->sc_iflags) & IFF_PROMISC) && (ifp->if_flags & IFF_RUNNING)) {
			le->sc_iflags = ifp->if_flags;
			lereset(ifp->if_unit);
			(void)lestart(ifp);
		}
		break;

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

leerror(unit, stat)
	int unit;
	int stat;
{
	if (!ledebug)
		return;

	/*
	 * Not all transceivers implement heartbeat
	 * so we only log CERR once.
	 */
	if ((stat & LE_CERR) && le_softc[unit].sc_cerr)
		return;

	log(LOG_WARNING,
	    "le%d: error: stat=%b\n", unit,
	    stat,
	    "\20\20ERR\17BABL\16CERR\15MISS\14MERR\13RINT\12TINT\11IDON\10INTR\07INEA\06RXON\05TXON\04TDMD\03STOP\02STRT\01INIT");
}

lererror(unit, msg)
	int unit;
	char *msg;
{
	register struct le_softc *le = &le_softc[unit];
	register struct lermd *rmd;
	int len;

	if (!ledebug)
		return;

	rmd = &le->sc_r2->ler2_rmd[le->sc_rmd];
	len = rmd->rmd3;

	log(LOG_WARNING,
	    "le%d: ierror(%s): from %s: buf=%d, len=%d, rmd1=%b\n",
	    unit, msg,
	    len > 11 ? ether_sprintf(&le->sc_r2->ler2_rbuf[le->sc_rmd][6]) : "unknown",
	    le->sc_rmd, len,
	    rmd->rmd1,
	    "\20\20OWN\17ERR\16FRAM\15OFLO\14CRC\13RBUF\12STP\11ENP");
}

lexerror(unit)
	int unit;
{
	register struct le_softc *le = &le_softc[unit];
	register struct letmd *tmd;
	int len;

	if (!ledebug)
		return;

	tmd = le->sc_r2->ler2_tmd;
	len = -tmd->tmd2;

	log(LOG_WARNING,
	    "le%d: oerror: to %s: buf=%d, len=%d, tmd1=%b, tmd3=%b\n",
	    unit,
	    len > 5 ? ether_sprintf(&le->sc_r2->ler2_tbuf[0][0]) : "unknown",
	    0, len,
	    tmd->tmd1,
	    "\20\20OWN\17ERR\16RES\15MORE\14ONE\13DEF\12STP\11ENP",
	    tmd->tmd3,
	    "\20\20BUFF\17UFLO\16RES\15LCOL\14LCAR\13RTRY");
}

#endif


