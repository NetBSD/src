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
 *	$Id: if_le.c,v 1.8 1994/02/06 00:46:02 mycroft Exp $
 */

#include "le.h"
#if NLE > 0

#include "bpfilter.h"

/*
 * AMD 7990 LANCE
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/route.h>
#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

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
#include <hp300/hp300/isr.h>
#include <machine/mtpr.h>

#include <hp300/dev/device.h>
#include <hp300/dev/if_lereg.h>

/* offsets for:	   ID,   REGS,    MEM,  NVRAM */
int	lestd[] = { 0, 0x4000, 0x8000, 0xC008 };

struct	isr le_isr[NLE];
int	ledebug = 0;		/* console error messages */

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
	struct	lereg0 *sc_r0;	/* DIO registers */
	struct	lereg1 *sc_r1;	/* LANCE registers */
	struct	lereg2 *sc_r2;	/* dual-port RAM */
	int	sc_rmd;		/* predicted next rmd to process */
	int	sc_tmd;		/* next available tmd */
	int	sc_txcnt;	/* # of transmit buffers in use */
	/* stats */
	int	sc_runt;
	int	sc_jab;
	int	sc_merr;
	int	sc_babl;
	int	sc_cerr;
	int	sc_miss;
	int	sc_rown;
	int	sc_xint;
	int	sc_xown;
	int	sc_xown2;
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

/* access LANCE registers */
#define	LERDWR(cntl, src, dst) \
	do { \
		(dst) = (src); \
	} while (((cntl)->ler0_status & LE_ACK) == 0);

int leattach __P((struct hp_device *));
void lesetladrf __P((struct le_softc *));
void ledrinit __P((struct lereg2 *));
void lereset __P((struct le_softc *));
void leinit __P((int));
int lestart __P((struct ifnet *));
int leintr __P((int));
void lexint __P((struct le_softc *));
void lerint __P((struct le_softc *));
void leread __P((struct le_softc *, char *, int));
int leput __P((char *, struct mbuf *));
struct mbuf *leget __P((char *, int, int, struct ifnet *));
int leioctl __P((struct ifnet *, int, caddr_t));
void leerror __P((struct le_softc *, int));
void lererror __P((struct le_softc *, char *));
void lexerror __P((struct le_softc *));
int ether_output();

struct	driver ledriver = {
	leattach, "le",
};

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
int
leattach(hd)
	struct hp_device *hd;
{
	register struct lereg0 *ler0;
	register struct lereg2 *ler2;
	struct lereg2 *lemem = 0;
	struct le_softc *sc = &le_softc[hd->hp_unit];
	struct ifnet *ifp = &sc->sc_if;
	char *cp;
	int i;

	ler0 = sc->sc_r0 = (struct lereg0 *)(lestd[0] + (int)hd->hp_addr);
	sc->sc_r1 = (struct lereg1 *)(lestd[1] + (int)hd->hp_addr);
	ler2 = sc->sc_r2 = (struct lereg2 *)(lestd[2] + (int)hd->hp_addr);
	if (ler0->ler0_id != LEID)
		return(0);
	le_isr[hd->hp_unit].isr_intr = leintr;
	hd->hp_ipl = le_isr[hd->hp_unit].isr_ipl = LE_IPL(ler0->ler0_status);
	le_isr[hd->hp_unit].isr_arg = hd->hp_unit;
	ler0->ler0_id = 0xFF;
	DELAY(100);

	/*
	 * Read the ethernet address off the board, one nibble at a time.
	 */
	cp = (char *)(lestd[3] + (int)hd->hp_addr);
	for (i = 0; i < sizeof(sc->sc_addr); i++) {
		sc->sc_addr[i] = (*++cp & 0xF) << 4;
		cp++;
		sc->sc_addr[i] |= *++cp & 0xF;
		cp++;
	}
	printf("le%d: hardware address %s\n", hd->hp_unit,
		ether_sprintf(sc->sc_addr));

	/*
	 * Setup for transmit/receive
	 */
	ler2->ler2_mode = LE_MODE;
	ler2->ler2_rlen = LE_RLEN;
	ler2->ler2_rdra = (int)lemem->ler2_rmd;
	ler2->ler2_tlen = LE_TLEN;
	ler2->ler2_tdra = (int)lemem->ler2_tmd;
	isrlink(&le_isr[hd->hp_unit]);
	ler0->ler0_status = LE_IE;

	ifp->if_unit = hd->hp_unit;
	ifp->if_name = "le";
	ifp->if_mtu = ETHERMTU;
	ifp->if_ioctl = leioctl;
	ifp->if_output = ether_output;
	ifp->if_start = lestart;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST |
			IFF_NOTRAILERS;
#if NBPFILTER > 0
	bpfattach(&sc->sc_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	if_attach(ifp);
	return (1);
}

/*
 * Set up the logical address filter
 */
void
lesetladrf(sc)
	struct le_softc *sc;
{
	struct lereg2 *ler2 = sc->sc_r2;
	struct ifnet *ifp = &sc->sc_if;
	struct ether_multi *enm;
	register u_char *cp, c;
	register u_long crc;
	register int i, len;
	struct ether_multistep step;

	/*
	 * Set up multicast address filter by passing all multicast
	 * addresses through a crc generator, and then using the high
	 * order 6 bits as a index into the 64 bit logical address
	 * filter. The high order two bits select the word, while the
	 * rest of the bits select the bit within the word.
	 */

	ler2->ler2_ladrf[0] = 0;
	ler2->ler2_ladrf[1] = 0;
	ifp->if_flags &= ~IFF_ALLMULTI;
	ETHER_FIRST_MULTI(step, &sc->sc_ac, enm);
	while (enm != NULL) {
		if (bcmp((caddr_t)&enm->enm_addrlo,
		    (caddr_t)&enm->enm_addrhi, sizeof(enm->enm_addrlo)) != 0) {
			/*
			 * We must listen to a range of multicast
			 * addresses. For now, just accept all
			 * multicasts, rather than trying to set only
			 * those filter bits needed to match the range.
			 * (At this time, the only use of address
			 * ranges is for IP multicast routing, for
			 * which the range is big enough to require all
			 * bits set.)
			 */
			ler2->ler2_ladrf[0] = 0xffffffff;
			ler2->ler2_ladrf[1] = 0xffffffff;
			ifp->if_flags |= IFF_ALLMULTI;
			return;
		}

		/*
		 * One would think, given the AM7990 document's polynomial
		 * of 0x04c11db6, that this should be 0x6db88320 (the bit
		 * reversal of the AMD value), but that is not right.  See
		 * the BASIC listing: bit 0 (our bit 31) must then be set.
		 */
		cp = (unsigned char *)&enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = 6; --len >= 0;) {
			c = *cp++;
			for (i = 8; --i >= 0;) {
				if ((c & 0x01) ^ (crc & 0x01)) {
					crc >>= 1;
					crc = crc ^ 0xedb88320;
				} else
					crc >>= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc = crc >> 26;

		/* Turn on the corresponding bit in the filter. */
		ler2->ler2_ladrf[crc >> 5] |= 1 << (crc & 0x1f);

		ETHER_NEXT_MULTI(step, enm);
	}
}

void
ledrinit(ler2)
	register struct lereg2 *ler2;
{
	register struct lereg2 *lemem = 0;
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
lereset(sc)
	register struct le_softc *sc;
{
	register struct lereg0 *ler0 = sc->sc_r0;
	register struct lereg1 *ler1 = sc->sc_r1;
	register struct lereg2 *ler2 = sc->sc_r2;
	struct lereg2 *lemem = 0;
	register int timo, stat;

#if NBPFILTER > 0
	if (sc->sc_if.if_flags & IFF_PROMISC)
		/* set the promiscuous bit */
		ler2->ler2_mode = LE_MODE|0x8000;
	else
#endif
		ler2->ler2_mode = LE_MODE;
	LERDWR(ler0, LE_CSR0, ler1->ler1_rap);
	LERDWR(ler0, LE_STOP, ler1->ler1_rdp);

	ler2->ler2_padr[0] = sc->sc_addr[1];
	ler2->ler2_padr[1] = sc->sc_addr[0];
	ler2->ler2_padr[2] = sc->sc_addr[3];
	ler2->ler2_padr[3] = sc->sc_addr[2];
	ler2->ler2_padr[4] = sc->sc_addr[5];
	ler2->ler2_padr[5] = sc->sc_addr[4];
	lesetladrf(sc);
	ledrinit(ler2);
	sc->sc_rmd = sc->sc_tmd = sc->sc_txcnt = 0;

	LERDWR(ler0, LE_CSR1, ler1->ler1_rap);
	LERDWR(ler0, (int)&lemem->ler2_mode, ler1->ler1_rdp);
	LERDWR(ler0, LE_CSR2, ler1->ler1_rap);
	LERDWR(ler0, 0, ler1->ler1_rdp);
	LERDWR(ler0, LE_CSR3, ler1->ler1_rap);
	LERDWR(ler0, LE_BSWP, ler1->ler1_rdp);
	LERDWR(ler0, LE_CSR0, ler1->ler1_rap);
	LERDWR(ler0, LE_INIT, ler1->ler1_rdp);
	timo = 100000;
	do {
		if (--timo == 0) {
			printf("le%d: init timeout, stat=0x%x\n",
			    sc->sc_if.if_unit, stat);
			break;
		}
		LERDWR(ler0, ler1->ler1_rdp, stat);
	} while ((stat & (LE_IDON | LE_ERR)) == 0);
	if (stat & LE_ERR)
		printf("le%d: init failed, stat=0x%x\n",
		    sc->sc_if.if_unit, stat);
	else
		LERDWR(ler0, LE_IDON, ler1->ler1_rdp);
	sc->sc_if.if_flags &= ~IFF_OACTIVE;
	LERDWR(ler0, LE_STRT | LE_INEA, ler1->ler1_rdp);
}

/*
 * Initialization of interface
 */
void
leinit(unit)
	int unit;
{
	struct le_softc *sc = &le_softc[unit];
	register struct ifnet *ifp = &sc->sc_if;
	int s;

	/* not yet, if address still unknown */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return;
	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		s = splimp();
		ifp->if_flags |= IFF_RUNNING;
		lereset(sc);
		(void) lestart(ifp);
		splx(s);
	}
}

#define	LENEXTTMP \
	if (++bix == LETBUF) bix = 0, tmd = sc->sc_r2->ler2_tmd; else ++tmd

/*
 * Start output on interface.  Get another datagram to send
 * off of the interface queue, and copy it to the interface
 * before starting the output.
 */
int
lestart(ifp)
	struct ifnet *ifp;
{
	register struct le_softc *sc = &le_softc[ifp->if_unit];
	register int bix;
	register struct letmd *tmd;
	register struct mbuf *m;
	int len, gotone = 0;

	if ((sc->sc_if.if_flags & IFF_RUNNING) == 0)
		return (0);
	bix = sc->sc_tmd;
	tmd = &sc->sc_r2->ler2_tmd[bix];
	do {
		if (tmd->tmd1 & LE_OWN) {
			if (gotone)
				break;
			sc->sc_xown2++;
			return (0);
		}
		IF_DEQUEUE(&sc->sc_if.if_snd, m);
		if (m == 0) {
			if (gotone)
				break;
			return (0);
		}
		len = leput(sc->sc_r2->ler2_tbuf[bix], m);
#if NBPFILTER > 0
		/*
		 * If bpf is listening on this interface, let it
		 * see the packet before we commit it to the wire.
		 */
		if (sc->sc_bpf)
			bpf_tap(sc->sc_bpf, sc->sc_r2->ler2_tbuf[bix], len);
#endif
		tmd->tmd3 = 0;
		tmd->tmd2 = -len;
		tmd->tmd1 = LE_OWN | LE_STP | LE_ENP;
		LENEXTTMP;
		gotone++;
	} while (++sc->sc_txcnt < LETBUF);
	sc->sc_tmd = bix;
	sc->sc_if.if_flags |= IFF_OACTIVE;
	/* transmit as soon as possible */
	LERDWR(sc->sc_r0, LE_INEA|LE_TDMD, sc->sc_r1->ler1_rdp);
	return (0);
}

int
leintr(unit)
	register int unit;
{
	register struct le_softc *sc = &le_softc[unit];
	register struct lereg0 *ler0 = sc->sc_r0;
	register struct lereg1 *ler1;
	register int stat;

	if ((ler0->ler0_status & LE_IR) == 0)
		return(0);
	if (ler0->ler0_status & LE_JAB) {
		sc->sc_jab++;
		lereset(sc);
		return(1);
	}
	ler1 = sc->sc_r1;
	LERDWR(ler0, ler1->ler1_rdp, stat);
	if (stat & LE_SERR) {
		leerror(sc, stat);
		if (stat & LE_MERR) {
			sc->sc_merr++;
			lereset(sc);
			return(1);
		}
		if (stat & LE_BABL)
			sc->sc_babl++;
		if (stat & LE_CERR)
			sc->sc_cerr++;
		if (stat & LE_MISS)
			sc->sc_miss++;
		LERDWR(ler0, LE_BABL|LE_CERR|LE_MISS|LE_INEA, ler1->ler1_rdp);
	}
	if ((stat & LE_RXON) == 0) {
		sc->sc_rxoff++;
		lereset(sc);
		return(1);
	}
	if ((stat & LE_TXON) == 0) {
		sc->sc_txoff++;
		lereset(sc);
		return(1);
	}
	if (stat & LE_RINT)
		lerint(sc);
	if (stat & LE_TINT)
		lexint(sc);
	return(1);
}

/*
 * Ethernet interface transmitter interrupt.
 * Start another output if more data to send.
 */
void
lexint(sc)
	register struct le_softc *sc;
{
	register struct letmd *tmd;
	int bix, gotone = 0;

	if ((sc->sc_if.if_flags & IFF_OACTIVE) == 0) {
		sc->sc_xint++;
		return;
	}
	if ((bix = sc->sc_tmd - sc->sc_txcnt) < 0)
		bix += LETBUF;
	tmd = &sc->sc_r2->ler2_tmd[bix];
	do {
		if (tmd->tmd1 & LE_OWN) {
			if (gotone)
				break;
			sc->sc_xown++;
			return;
		}

		/* clear interrupt */
		LERDWR(sc->sc_r0, LE_TINT|LE_INEA, sc->sc_r1->ler1_rdp);

		/* XXX documentation says BUFF not included in ERR */
		if ((tmd->tmd1 & LE_ERR) || (tmd->tmd3 & LE_TBUFF)) {
			lexerror(sc);
			sc->sc_if.if_oerrors++;
			if (tmd->tmd3 & (LE_TBUFF|LE_UFLO)) {
				sc->sc_uflo++;
				lereset(sc);
			} else if (tmd->tmd3 & LE_LCOL)
				sc->sc_if.if_collisions++;
			else if (tmd->tmd3 & LE_RTRY)
				sc->sc_if.if_collisions += 16;
		}
		else if (tmd->tmd1 & LE_ONE)
			sc->sc_if.if_collisions++;
		else if (tmd->tmd1 & LE_MORE)
			/* what is the real number? */
			sc->sc_if.if_collisions += 2;
		else
			sc->sc_if.if_opackets++;
		LENEXTTMP;
		gotone++;
	} while (--sc->sc_txcnt > 0);
	sc->sc_if.if_flags &= ~IFF_OACTIVE;
	(void) lestart(&sc->sc_if);
}

#define	LENEXTRMP \
	if (++bix == LERBUF) bix = 0, rmd = sc->sc_r2->ler2_rmd; else ++rmd

/*
 * Ethernet interface receiver interrupt.
 * If input error just drop packet.
 * Decapsulate packet based on type and pass to type specific
 * higher-level input routine.
 */
void
lerint(sc)
	register struct le_softc *sc;
{
	register int bix = sc->sc_rmd;
	register struct lermd *rmd = &sc->sc_r2->ler2_rmd[bix];

	/*
	 * Out of sync with hardware, should never happen?
	 */
	if (rmd->rmd1 & LE_OWN) {
		sc->sc_rown++;
		do {
			LENEXTRMP;
		} while ((rmd->rmd1 & LE_OWN) && bix != sc->sc_rmd);
		if (bix == sc->sc_rmd) {
			printf("le%d: rint with no buffer\n",
			    sc->sc_if.if_unit);
			LERDWR(sc->sc_r0, LE_RINT|LE_INEA, sc->sc_r1->ler1_rdp);
			return;
		}
	}

	/*
	 * Process all buffers with valid data
	 */
	while ((rmd->rmd1 & LE_OWN) == 0) {
		int len = rmd->rmd3;

		/* Clear interrupt to avoid race condition */
		LERDWR(sc->sc_r0, LE_RINT|LE_INEA, sc->sc_r1->ler1_rdp);

		if (rmd->rmd1 & LE_ERR) {
			sc->sc_rmd = bix;
			lererror(sc, "bad packet");
			sc->sc_if.if_ierrors++;
		} else if ((rmd->rmd1 & (LE_STP|LE_ENP)) != (LE_STP|LE_ENP)) {
			/*
			 * Find the end of the packet so we can see how long
			 * it was.  We still throw it away.
			 */
			do {
				LERDWR(sc->sc_r0, LE_RINT|LE_INEA,
				       sc->sc_r1->ler1_rdp);
				rmd->rmd3 = 0;
				rmd->rmd1 = LE_OWN;
				LENEXTRMP;
			} while (!(rmd->rmd1 & (LE_OWN|LE_ERR|LE_STP|LE_ENP)));
			sc->sc_rmd = bix;
			lererror(sc, "chained buffer");
			sc->sc_rxlen++;
			/*
			 * If search terminated without successful completion
			 * we reset the hardware (conservative).
			 */
			if ((rmd->rmd1 & (LE_OWN|LE_ERR|LE_STP|LE_ENP)) !=
			    LE_ENP) {
				lereset(sc);
				return;
			}
		} else
			leread(sc, sc->sc_r2->ler2_rbuf[bix], len);
		rmd->rmd3 = 0;
		rmd->rmd1 = LE_OWN;
		LENEXTRMP;
	}
	sc->sc_rmd = bix;
}

void
leread(sc, buf, len)
	register struct le_softc *sc;
	char *buf;
	int len;
{
	register struct ether_header *et;
	register struct ifnet *ifp = &sc->sc_if;
    	struct mbuf *m;

	ifp->if_ipackets++;
	et = (struct ether_header *)buf;
	/* adjust input length to account for header and CRC */
	len -= sizeof(struct ether_header) + 4;

	if (len <= 0) {
		if (ledebug)
			log(LOG_WARNING,
			    "le%d: ierror(runt packet): from %s: len=%d\n",
			    sc->sc_if.if_unit, ether_sprintf(et->ether_shost),
			    len);
		sc->sc_runt++;
		ifp->if_ierrors++;
		return;
	}

#if NBPFILTER > 0
	/*
	 * Check if there's a bpf filter listening on this interface.
	 * If so, hand off the raw packet to bpf, then discard things
	 * not destined for us (but be sure to keep broadcast/multicast).
	 */
	if (sc->sc_bpf) {
		bpf_tap(sc->sc_bpf, buf, len + sizeof(struct ether_header));
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (et->ether_dhost[0] & 1) == 0 &&
		    bcmp(et->ether_dhost, sc->sc_addr,
			    sizeof(et->ether_dhost)) != 0 &&
		    bcmp(et->ether_dhost, etherbroadcastaddr,
			    sizeof(et->ether_dhost)) != 0)
			return;
	}
#endif

	m = leget(buf, len, 0, ifp);
	if (m == 0)
		return;

	ether_input(ifp, et, m);
}

/*
 * Routine to copy from mbuf chain to transmit
 * buffer in board local memory.
 */
int
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
int
leioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	register struct ifaddr *ifa;
	struct le_softc *sc = &le_softc[ifp->if_unit];
	int s = splimp(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
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
				ina->x_host = *(union ns_host *)(sc->sc_addr);
			else {
				/* 
				 * The manual says we can't change the address 
				 * while the receiver is armed,
				 * so reset everything
				 */
				ifp->if_flags &= ~IFF_RUNNING; 
				LERDWR(sc->sc_r0, LE_STOP, sc->sc_r1->ler1_rdp);
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->sc_addr, sizeof(sc->sc_addr));
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
			ifp->if_flags &= ~IFF_RUNNING;
			LERDWR(sc->sc_r0, LE_STOP, sc->sc_r1->ler1_rdp);
		} else if (ifp->if_flags & IFF_UP &&
		    (ifp->if_flags & IFF_RUNNING) == 0)
			leinit(ifp->if_unit);
		/*
		 * If the state of the promiscuous bit changes, the interface
		 * must be reset to effect the change.
		 */
		if (((ifp->if_flags ^ sc->sc_iflags) & IFF_PROMISC) &&
		    (ifp->if_flags & IFF_RUNNING)) {
			sc->sc_iflags = ifp->if_flags;
			lereset(sc);
			lestart(ifp);
		}
		break;

	case SIOCADDMULTI:
		error = ether_addmulti((struct ifreq *)data, &sc->sc_ac);
		goto update_multicast;

	case SIOCDELMULTI:
		error = ether_delmulti((struct ifreq *)data, &sc->sc_ac);
	update_multicast:
		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware
			 * filter accordingly.
			 */
			lereset(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return (error);
}

void
leerror(sc, stat)
	register struct le_softc *sc;
	int stat;
{

	if (!ledebug)
		return;

	/*
	 * Not all transceivers implement heartbeat
	 * so we only log CERR once.
	 */
	if ((stat & LE_CERR) && sc->sc_cerr)
		return;
	log(LOG_WARNING,
	    "le%d: error: stat=%b\n", sc->sc_if.if_unit, stat,
	    "\20\20ERR\17BABL\16CERR\15MISS\14MERR\13RINT\12TINT\11IDON\10INTR\07INEA\06RXON\05TXON\04TDMD\03STOP\02STRT\01INIT");
}

void
lererror(sc, msg)
	register struct le_softc *sc;
	char *msg;
{
	register struct lermd *rmd;
	int len;

	if (!ledebug)
		return;

	rmd = &sc->sc_r2->ler2_rmd[sc->sc_rmd];
	len = rmd->rmd3;
	log(LOG_WARNING,
	    "le%d: ierror(%s): from %s: buf=%d, len=%d, rmd1=%b\n",
	    sc->sc_if.if_unit, msg,
	    len > 11 ? ether_sprintf(&sc->sc_r2->ler2_rbuf[sc->sc_rmd][6]) : "unknown",
	    sc->sc_rmd, len, rmd->rmd1,
	    "\20\20OWN\17ERR\16FRAM\15OFLO\14CRC\13RBUF\12STP\11ENP");
}

void
lexerror(sc)
	register struct le_softc *sc;
{
	register struct letmd *tmd;
	register int len;

	if (!ledebug)
		return;

	tmd = sc->sc_r2->ler2_tmd;
	len = -tmd->tmd2;
	log(LOG_WARNING,
	    "le%d: oerror: to %s: buf=%d, len=%d, tmd1=%b, tmd3=%b\n",
	    sc->sc_if.if_unit,
	    len > 5 ? ether_sprintf(&sc->sc_r2->ler2_tbuf[0][0]) : "unknown",
	    0, len, tmd->tmd1,
	    "\20\20OWN\17ERR\16RES\15MORE\14ONE\13DEF\12STP\11ENP",
	    tmd->tmd3,
	    "\20\20BUFF\17UFLO\16RES\15LCOL\14LCAR\13RTRY");
}
#endif
