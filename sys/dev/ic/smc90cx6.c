/*	$NetBSD: smc90cx6.c,v 1.3 1995/03/02 09:12:27 chopps Exp $ */

/*
 * Copyright (c) 1994, 1995 Ignatios Souvatzis
 * Copyright (c) 1994 Timo Rossi
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
 *      This product includes software developed by  Timo Rossi
 *      This product includes software developed by  Ignatios Souvatzis
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Driver for the Commodore Busines Machines arcnet card
 * written by Ignatios Souvatzis <is@beverly.rhein.de>,
 * somewhat based on Amiga if_ed.c 
 */

#define BAHASMCOPY /**/
#define BAHSOFTCOPY /**/
/* #define BAHTIMINGS /**/
/* #define BAH_DEBUG 3 /**/

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/errno.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/netisr.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_ether.h>
#include <netinet/if_arc.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <sys/kernel.h>
#include <machine/cpu.h>
#include <machine/mtpr.h>

#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/if_bahreg.h>

/* these should be elsewhere */

#define ARC_MIN_LEN 1
#define ARC_MIN_FORBID_LEN 254
#define ARC_MAX_FORBID_LEN 256
#define ARC_MAX_LEN 508
#define ARC_ADDR_LEN 1

#define MIN(a,b) ((b)<0?(a):min((a),(b)))

/*
 * This currently uses 2 bufs for tx, 2 for rx
 *
 * New rx protocol:
 *
 * rx has a fillcount variable. If fillcount > (NRXBUF-1), 
 * rx can be switched off from rx hard int. 
 * Else rx is restarted on the other receiver.
 * rx soft int counts down. if it is == (NRXBUF-1), it restarts
 * the receiver.
 * To ensure packet ordering (we need that for 1201 later), we have a counter
 * which is incremented modulo 256 on each receive and a per buffer
 * variable, which is set to the counter on filling. The soft int can
 * compare both values to determine the older packet.
 *
 * Transmit direction:
 * 
 * bah_start checks tx_fillcount
 * case 2: return
 *
 * else fill tx_act ^ 1 && inc tx_fillcount
 *
 * check tx_fillcount again.
 * case 2: set IFF_OACTIVE to stop arc_output from filling us.
 * case 1: start tx
 *
 * tint clears IFF_OCATIVE, decrements and checks tx_fillcount
 * case 1: start tx on tx_act ^ 1, softcall bah_start
 * case 0: softcall bah_start
 *
 * #define fill(i) get mbuf && copy mbuf to chip(i)
 */

/*
 * Arcnet software status per interface
 *
 */
struct bah_softc {
	struct	device	sc_dev;
	struct	arccom	sc_arccom;	/* Common arcnet structures */
	struct	isr	sc_isr;
	struct	a2060	*sc_base;
	u_long	sc_recontime;		/* seconds only, I'm lazy */
	u_long	sc_reconcount;		/* for the above */
	u_long	sc_reconcount_excessive; /* for the above */
#define ARC_EXCESSIVE_RECONS 20
#define ARC_EXCESSIVE_RECONS_REWARN 400
	u_char	sc_bufstat[4];		/* use for packet no for rx */
	u_char	sc_intmask;
	u_char	sc_rx_packetno;
	u_char	sc_rx_act;		/* 2..3 */
	u_char	sc_tx_act;		/* 0..1 */
	u_char	sc_rx_fillcount;
	u_char	sc_tx_fillcount;
	u_char	sc_broadcast[2];	/* is it a broadcast packet? */
	u_char	sc_retransmits[2];	/* unused at the moment */
#ifdef BAHTIMINGS
	struct	{
		int mincopyin;
		int maxcopyin;	/* divided by byte count */
		int mincopyout;
		int maxcopyout;
		int minsend;
		int maxsend;
		int lasttxstart_mics;
		struct timeval lasttxstart_tv;
		
	} sc_stats;
#endif
#if NBPFILTER > 0
	caddr_t sc_bpf;
#endif
/* Add other fields as needed... -- IS */
};

/* prototypes */

int bahmatch __P((struct device *, struct cfdata *, void *));
void bahattach __P((struct device *, struct device *, void *));
void bah_ini __P((struct bah_softc *));
void bah_reset __P((struct bah_softc *));
void bah_stop __P((struct bah_softc *));
int bah_start __P((struct ifnet *));
int bahintr __P((struct bah_softc *sc));
int bah_ioctl __P((struct ifnet *, unsigned long, caddr_t));
void movepout __P((u_char *from, u_char volatile *to, int len));
void movepin __P((u_char volatile *from, u_char *to, int len));
void bah_srint __P((struct bah_softc *sc, void *dummy));
void callstart __P((struct bah_softc *sc, void *dummy));

#ifdef BAHTIMINGS
int clkread();
#endif

struct cfdriver bahcd = {
	NULL, "bah", (cfmatch_t)bahmatch, bahattach, DV_IFNET, 
	sizeof(struct bah_softc),
};

int
bahmatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{
	struct zbus_args *zap;

	zap = auxp;

	if ((zap->manid == 514 || zap->manid == 1053) && zap->prodid == 9)
		return (1);
	return (0);
}

void
bahattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct zbus_args *zap;
	struct bah_softc *sc;
	struct ifnet *ifp;
	int i, s, linkaddress;

	zap = auxp;
	sc = (struct bah_softc *)dp;
	ifp = &sc->sc_arccom.ac_if;

#if (defined(BAH_DEBUG) && (BAH_DEBUG > 2))
	printf("\nbah%ld: attach(0x%x, 0x%x, 0x%x)\n",
	    sc->sc_dev.dv_unit, pdp, dp, auxp);
#endif
	s = splhigh();
	sc->sc_base = zap->va;

	/*
	 * read the arcnet address from the board
	 */

	sc->sc_base->kick1 = 0x0;
	sc->sc_base->kick2 = 0x0;
	DELAY(120);

	sc->sc_base->kick1 = 0xFF;
	sc->sc_base->kick2 = 0xFF;
	do {
		DELAY(120);
	} while (!(sc->sc_base->status & ARC_POR)); 

	linkaddress = sc->sc_base->dipswitches;

#ifdef BAHTIMINGS
	printf(": link addr 0x%02x(%ld), with timer\n",
	    linkaddress, linkaddress);
#else
	printf(": link addr 0x%02x(%ld)\n", linkaddress, linkaddress);
#endif

	sc->sc_arccom.ac_anaddr = sc->sc_base->dipswitches;

	/* clear the int mask... */

	sc->sc_base->status = sc->sc_intmask = 0;

	sc->sc_base->command = ARC_CONF(CONF_LONG);
	sc->sc_base->command = ARC_CLR(CLR_POR|CLR_RECONFIG);
	sc->sc_recontime = sc->sc_reconcount = 0;

	/* and reenable kernel int level */
	splx(s);

	/*
	 * set interface to stopped condition (reset)
	 * 
	 */
	bah_stop(sc); 

	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = bahcd.cd_name;
	ifp->if_output = arc_output;
	ifp->if_start = bah_start;
	ifp->if_ioctl = bah_ioctl;
	/* might need later: ifp->if_watchdog  = bah_watchdog */

	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX |
	    IFF_NOTRAILERS | IFF_NOARP;

	ifp->if_mtu = ARCMTU;

#if NBPFILTER > 0
	bpfattach(&sc->sc_bpf, ifp, DLT_ARCNET, ARC_HDRLEN);
#endif

	if_attach(ifp);
	arc_ifattach(ifp);

	sc->sc_isr.isr_intr = bahintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr(&sc->sc_isr);
}

/*
 * Initialize device
 *
 */
void
bah_init(sc)
	struct bah_softc *sc;
{
	struct ifnet *ifp;
	int s;

	ifp = &sc->sc_arccom.ac_if;

	/* Address not known. */
	if (ifp->if_addrlist == 0)
		return;

	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		s = splimp();
		ifp->if_flags |= IFF_RUNNING;
		bah_reset(sc);
		(void)bah_start(ifp);
		splx(s);
	}
}

/*
 * Reset the interface...
 *
 * this assumes that it is called inside a critical section...
 *
 */
void
bah_reset(sc)
	struct bah_softc *sc;
{
	struct ifnet *ifp;
	int i, s;

	ifp = &sc->sc_arccom.ac_if;

#ifdef BAH_DEBUG
	printf("bah%ld: reset\n", ifp->if_unit);
#endif
	/* stop hardware in case it still runs */

	sc->sc_base->kick1 = 0;
	sc->sc_base->kick2 = 0;
	DELAY(120);

	/* and restart it */
	sc->sc_base->kick1 = 0xFF;
	sc->sc_base->kick2 = 0xFF;

	do {
		DELAY(120);
	} while (!(sc->sc_base->status & ARC_POR)); 

#if defined(BAH_DEBUG) && (BAH_DEBUG > 2)
	printf("bah%ld: reset: card reset, status=0x%02x\n",
	    ifp->if_unit,
	    sc->sc_base->status);
#endif
	/* POR is NMI, but we need it below: */
	sc->sc_intmask = ARC_RECON|ARC_POR;
	sc->sc_base->status	= sc->sc_intmask;
	sc->sc_base->command = ARC_CONF(CONF_LONG);
	
#ifdef BAH_DEBUG
	printf("bah%ld: reset: chip configured, status=0x%02x\n",
	    ifp->if_unit,
	    sc->sc_base->status);
#endif

	sc->sc_base->command = ARC_CLR(CLR_POR|CLR_RECONFIG);

#ifdef BAH_DEBUG
	printf("bah%ld: reset: bits cleared, status=0x%02x\n",
	    ifp->if_unit,
	    sc->sc_base->status);
#endif

	sc->sc_reconcount_excessive = ARC_EXCESSIVE_RECONS;

	/* start receiver */

	sc->sc_intmask  |= ARC_RI;

	sc->sc_bufstat[2] =
	    sc->sc_bufstat[3] =
	    sc->sc_rx_packetno = 
	    sc->sc_rx_fillcount = 0;

	sc->sc_rx_act = 2;

	sc->sc_base->command = ARC_RXBC(2);
	sc->sc_base->status	= sc->sc_intmask;

#ifdef BAH_DEBUG
	printf("bah%ld: reset: started receiver, status=0x%02x\n",
	    ifp->if_unit,
	    sc->sc_base->status);
#endif

	/* and init transmitter status */
	sc->sc_tx_act = 0;
	sc->sc_tx_fillcount = 0;

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

#ifdef BAHTIMINGS
	bzero((caddr_t)&(sc->sc_stats),sizeof(sc->sc_stats));
	sc->sc_stats.mincopyin =
	    sc->sc_stats.mincopyout =
	    sc->sc_stats.minsend = 999999L;

#endif

	bah_start(ifp);
}

/*
 * Take interface offline
 */
void
bah_stop(sc)
	struct bah_softc *sc;
{
	int s;
	
	/* Stop the interrupts */
	sc->sc_base->status = 0;

	/* Stop the interface */
	sc->sc_base->kick1 = 0;
	sc->sc_base->kick2 = 0;

#ifdef BAHTIMINGS
	log(LOG_DEBUG,"%s%ld\
  To board: %6ld .. %6ld ns/byte\nFrom board: %6ld .. %6ld ns/byte\n\
Send time:  %6ld .. %6ld mics\n",
	    sc->sc_arccom.ac_if.if_name,
	    sc->sc_arccom.ac_if.if_unit,
	    sc->sc_stats.mincopyout,sc->sc_stats.maxcopyout,
	    sc->sc_stats.mincopyin, sc->sc_stats.maxcopyin,
	    sc->sc_stats.minsend,   sc->sc_stats.maxsend);

	sc->sc_stats.minsend = 
	    sc->sc_stats.mincopyout = 
	    sc->sc_stats.mincopyin = 999999L;
	sc->sc_stats.maxsend = 
	    sc->sc_stats.maxcopyout = 
	    sc->sc_stats.maxcopyin = 0L;
#endif

}

inline void 
movepout(from,to,len)
	u_char *from;
	volatile u_char *to;
	int len;
{
#ifdef BAHASMCOPY
	u_short shortd;
	u_long longd,longd1,longd2,longd3,longd4;

	if ((len>3) && ((int)from)&3) {
		switch (((int)from) & 3) {
		case 3:
			*to = *from++;
			to+=2;--len;
			break;
		case 1:
			*to = *from++;
			to += 2; --len;
		case 2:
			shortd = *((u_short *)from)++;
			asm("movepw %0,%1@(0)" : : "d"(shortd), "a"(to));
			to += 4; len -= 2;
			break;
		default:
		}

		while (len>=32) {
			longd1 = *((u_long *)from)++;
			longd2 = *((u_long *)from)++;
			longd3 = *((u_long *)from)++;
			longd4 = *((u_long *)from)++;
			asm("movepl %0,%1@(0)"  : : "d"(longd1), "a"(to));
			asm("movepl %0,%1@(8)"  : : "d"(longd2), "a"(to));
			asm("movepl %0,%1@(16)" : : "d"(longd3), "a"(to));
			asm("movepl %0,%1@(24)" : : "d"(longd4), "a"(to));

			longd1 = *((u_long *)from)++;
			longd2 = *((u_long *)from)++;
			longd3 = *((u_long *)from)++;
			longd4 = *((u_long *)from)++;
			asm("movepl %0,%1@(32)" : : "d"(longd1), "a"(to));
			asm("movepl %0,%1@(40)" : : "d"(longd2), "a"(to));
			asm("movepl %0,%1@(48)" : : "d"(longd3), "a"(to));
			asm("movepl %0,%1@(56)" : : "d"(longd4), "a"(to));

			to += 64; len -= 32;
		}
		while (len>0) {
			longd = *((u_long *)from)++;
			asm("movepl %0,%1@(0)" : : "d"(longd), "a"(to));
			to += 8; len -= 4;
		}
	}
#endif
	while (len>0) {
		*to = *from++;
		to+=2;
		--len;
	}
}

/*
 * Start output on interface. Get another datagram to send
 * off the interface queue, and copy it to the
 * interface becore starting the output
 *
 * this assumes that it is called inside a critical section...
 * XXX hm... does it still?
 *
 */
int
bah_start(ifp)
	struct ifnet *ifp;
{
	struct bah_softc *sc;
	struct mbuf *m,*mp;
	volatile u_char *bah_ram_ptr;
	int i, len, tlen, offset, s, buffer;
	
#ifdef BAHTIMINGS
	int copystart,lencopy,perbyte;
#endif

	sc = bahcd.cd_devs[ifp->if_unit];

#if defined(BAH_DEBUG) && (BAH_DEBUG > 3)
	printf("bah%ld: start(0x%x)\n",
	    ifp->if_unit, ifp);
#endif

	if((ifp->if_flags & IFF_RUNNING) == 0)
		return (0);

	s = splimp();

	if (sc->sc_tx_fillcount >= 2) {
		splx(s);
		return (0);
	}

	IF_DEQUEUE(&ifp->if_snd, m);
	buffer = sc->sc_tx_act ^ 1;

	splx(s);

	if (m == 0)
		return (0);

#if NBPFILTER > 0
	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire
	 *
	 * (can't give the copy in A2060 card RAM to bpf, because
	 * that RAM is just accessed as on every other byte)
	 */

	if (sc->sc_bpf)
		bpf_mtap(sc->sc_bpf, m);
#endif
	/* we need the data length beforehand */

	for (mp = m,tlen=0;mp;mp=mp->m_next)
		tlen += mp->m_len;

#ifdef BAH_DEBUG
	m = m_pullup(m,3);	/* gcc does structure padding */
	printf("bah%ld: start: filling %ld from %ld to %ld type %ld\n",
	    ifp->if_unit,
	    buffer, 
	    mtod(m, u_char *)[0],
	    mtod(m, u_char *)[1],
	    mtod(m, u_char *)[2]);
#else
	m = m_pullup(m,2);
#endif
	bah_ram_ptr = sc->sc_base->buffers + buffer*512*2;

	/* write the addresses to RAM and throw them away */

	/*
	 * Hardware does this: Yet Another Microsecond Saved.
	 * (btw, timing code says usually 2 microseconds)
	 * bah_ram_ptr[0*2] = mtod(m, u_char *)[0];
	 */

	bah_ram_ptr[1*2] = mtod(m, u_char *)[1];
	m_adj(m,2);
		
	/* correct total length for that */
	tlen -= 2;
	if (tlen < ARC_MIN_FORBID_LEN) {
		offset = 256-tlen;
		bah_ram_ptr[2*2] = offset;
	} else {
		if (tlen <= ARC_MAX_FORBID_LEN) 
			offset = 512-3-tlen;
		else {
			if (tlen > ARC_MAX_LEN)
				tlen = ARC_MAX_LEN;
			offset = 512-tlen;
		}

		bah_ram_ptr[2*2] = 0;
		bah_ram_ptr[3*2] = offset;
	}
	bah_ram_ptr += offset*2;

	/* lets loop again through the mbuf chain */

	for (mp=m;mp;mp=mp->m_next) {
		if (len = mp->m_len) {		/* YAMS */
#ifdef BAHTIMINGS
			lencopy = len;
			copystart = clkread();
#endif
			movepout(mtod(mp, caddr_t),bah_ram_ptr,len);

#ifdef BAHTIMINGS
			perbyte = 1000*(clkread() - copystart) / lencopy;
			sc->sc_stats.mincopyout = 
			    MIN(sc->sc_stats.mincopyout,perbyte);
			sc->sc_stats.maxcopyout =
			    max(sc->sc_stats.maxcopyout,perbyte);
#endif
			bah_ram_ptr += len*2;
		}
	}

	sc->sc_broadcast[buffer] = (m->m_flags & M_BCAST) != 0;
	sc->sc_retransmits[buffer] = 
		m->m_flags & M_BCAST ? 1 : 5;

	/* actually transmit the packet */
	s = splimp();

	if (++sc->sc_tx_fillcount > 1) { 
		/*
		 * We are filled up to the rim. No more bufs for the moment,
		 * please.
		 */
		ifp->if_flags |= IFF_OACTIVE;
	} else {
#ifdef BAH_DEBUG
		printf("bah%ld: start: starting transmitter on buffer %d\n", 
		    ifp->if_unit, buffer);
#endif
		/* Transmitter was off, start it */
		sc->sc_tx_act = buffer;

		/*
		 * We still can accept another buf, so don't:
		 * ifp->if_flags |= IFF_OACTIVE;
		 */
		sc->sc_intmask |= ARC_TA;
		sc->sc_base->command = ARC_TX(buffer);
		sc->sc_base->status  = sc->sc_intmask;

#ifdef BAHTIMINGS
		bcopy((caddr_t)&time,
		    (caddr_t)&(sc->sc_stats.lasttxstart_tv),
		    sizeof(struct timeval));

		sc->sc_stats.lasttxstart_mics = clkread();
#endif
	}
	splx(s);
	m_freem(m);

	/*
	 * We dont really need a transmit timeout timer, do we?
	 * XXX (sighing deeply) yes, after 10 times reading the docs,
	 * I realized that in the case the receiver NAKs the buffer request,
	 * the hardware retries till shutdown.
	 * TODO: Insert some reasonable transmit timeout timer.
	 */
	return (1);
}

void 
callstart(sc, dummy)
	struct bah_softc *sc;
	void *dummy;
{
	(void)bah_start(&sc->sc_arccom.ac_if);
}

inline void
movepin(from,to,len)
	volatile u_char *from;
	u_char *to;
	int len;
{
#ifdef BAHASMCOPY
	unsigned long	longd, longd1, longd2, longd3, longd4;
	ushort		shortd;

	if ((len>3) && (((int)to) & 3)) {
		switch(((int)to) & 3) {
		case 3: *to++ = *from;
			from+=2; --len;
			break;
		case 1: *to++ = *from;
			from+=2; --len;
		case 2:	asm ("movepw %1@(0),%0": "=d" (shortd) : "a" (from));
			*((ushort *)to)++ = shortd;
			from += 4; len -= 2;
			break;
		default:
		}

		while (len>=32) {
			asm("movepl %1@(0),%0"  : "=d"(longd1) : "a" (from));
			asm("movepl %1@(8),%0"  : "=d"(longd2) : "a" (from));
			asm("movepl %1@(16),%0" : "=d"(longd3) : "a" (from));
			asm("movepl %1@(24),%0" : "=d"(longd4) : "a" (from));
			*((unsigned long *)to)++ = longd1;
			*((unsigned long *)to)++ = longd2;
			*((unsigned long *)to)++ = longd3;
			*((unsigned long *)to)++ = longd4;

			asm("movepl %1@(32),%0" : "=d"(longd1) : "a" (from));
			asm("movepl %1@(40),%0" : "=d"(longd2) : "a" (from));
			asm("movepl %1@(48),%0" : "=d"(longd3) : "a" (from));
			asm("movepl %1@(56),%0" : "=d"(longd4) : "a" (from));
			*((unsigned long *)to)++ = longd1;
			*((unsigned long *)to)++ = longd2;
			*((unsigned long *)to)++ = longd3;
			*((unsigned long *)to)++ = longd4;

			from += 64; len -= 32;
		}
		while (len>0) {
			asm("movepl %1@(0),%0" : "=d"(longd) : "a" (from));
			*((unsigned long *)to)++ = longd;
			from += 8; len -= 4;
		}

	}
#endif /* BAHASMCOPY */
	while (len>0) {
		*to++ = *from;
		from+=2;
		--len;
	}

}

/*
 * Arcnet interface receiver soft interrupt:
 * get the stuff out of any filled buffer we find.
 */
void
bah_srint(sc,dummy)
	struct bah_softc *sc;
	void *dummy;
{
	int buffer, buffer1, len, len1, amount, offset, s, i;
	u_char volatile *bah_ram_ptr;
	struct mbuf *m, *dst, *head;
	struct arc_header *ah;

	head = 0;

#ifdef BAHTIMINGS
	int copystart,lencopy,perbyte;
#endif

	s = splimp();
	if (sc->sc_rx_fillcount <= 1)
		buffer = sc->sc_rx_act ^ 1;
	else {

		i = ((unsigned)(sc->sc_bufstat[2] - sc->sc_bufstat[3])) % 256;
		if (i < 64)
			buffer = 3;
		else if (i > 192)
			buffer = 2;
		else {
			log(LOG_WARNING,
			    "bah%ld: rx srint: which is older, %ld or %ld?\
(filled %ld)\n",
			    sc->sc_bufstat[2],sc->sc_bufstat[3],
			    sc->sc_rx_fillcount);
			splx(s);
			return;
		}
	}
	splx(s);

	/* Allocate header mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);

	if (m == 0) {
		/* 
	 	 * in case s.th. goes wrong with mem, drop it
	 	 * to make sure the receiver can be started again
		 * count it as input error (we dont have any other
		 * detectable)
	 	 */
		sc->sc_arccom.ac_if.if_ierrors++;
		goto cleanup;
	}
			
	m->m_pkthdr.rcvif = &sc->sc_arccom.ac_if;
	m->m_len = 0;

	/*
	 * Align so that IP packet will be longword aligned. Here we
	 * assume that m_data of new packet is longword aligned.
	 * When implementing RFC1201, we might have to change it to 2,
	 * (2*sizeof(ulong) - ARC_HDRLEN - sizeof(splitflag) - sizeof(pckid))
	 * possibly packet type dependent.
	 */

	m->m_data += 1;		/* sizeof(ulong) - ARC_HDRLEN */

	head = m;

	ah = mtod(head, struct arc_header *);
	bah_ram_ptr = sc->sc_base->buffers + buffer*512*2;
		
	ah->arc_shost = bah_ram_ptr[0*2];
	ah->arc_dhost = bah_ram_ptr[1*2];
	offset = bah_ram_ptr[2*2];
	if (offset)
		len = 256 - offset;
	else {
		offset = bah_ram_ptr[3*2];
		len = 512 - offset;
	}
	m->m_pkthdr.len = len+2;        /* whole packet length */
	m->m_len += 2; 		    	/* mbuf filled with ARCnet addresses */
	bah_ram_ptr += offset*2;	/* ram buffer continues there */

	while (len > 0) {
	
		len1 = len;
		amount = M_TRAILINGSPACE(m);

		if (amount == 0) {
			dst = m;
			MGET(m, M_DONTWAIT, MT_DATA);
		
			if (m == 0) {
				sc->sc_arccom.ac_if.if_ierrors++;
				goto cleanup;
			}
		
			if (len1 >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);
	
			m->m_len = 0;
			dst->m_next = m;
			amount = M_TRAILINGSPACE(m);
		}

		if (amount < len1)
			len1 = amount;

#ifdef BAHTIMINGS
		lencopy = len;
		copystart = clkread();
#endif

		movepin(bah_ram_ptr,mtod(m, u_char *) + m->m_len, len1);

#ifdef BAHTIMINGS
		perbyte = 1000*(clkread() - copystart) / lencopy;
		sc->sc_stats.mincopyin = MIN(sc->sc_stats.mincopyin,perbyte);
		sc->sc_stats.maxcopyin = max(sc->sc_stats.maxcopyin,perbyte);
#endif

		m->m_len += len1;
		bah_ram_ptr += len1*2;
		len -= len1;
	}

#if NBPFILTER > 0
	if (sc->sc_bpf) {
		bpf_mtap(sc->sc_bpf, head);
	}
#endif

	m_adj(head, 3); /* gcc does structure padding */
	arc_input(&sc->sc_arccom.ac_if, ah, head);

	/* arc_input has freed it, we dont need to... */

	head = NULL;
	sc->sc_arccom.ac_if.if_ipackets++;
	
cleanup:

	if(head == NULL)
		m_freem(head);

	s = splimp();

	if (--sc->sc_rx_fillcount == 1) {

		/* was off, restart it on buffer just emptied */
		sc->sc_rx_act = buffer;
		sc->sc_intmask |= ARC_RI;

		/* this also clears the RI flag interupt: */
		sc->sc_base->command = ARC_RXBC(buffer);
		sc->sc_base->status = sc->sc_intmask;

#ifdef BAH_DEBUG
		printf("bah%ld: srint: restarted rx on buf %ld\n",
		    sc->sc_arccom.ac_if.if_unit, buffer);
#endif
	}
	splx(s);
}

inline static void
bah_tint(sc)
	struct bah_softc *sc;
{
	int buffer;
	u_char volatile *bah_ram_ptr;
	int isr;
	int clknow;

	buffer = sc->sc_tx_act;
	isr = sc->sc_base->status;

	/* XXX insert retransmit code etc. here. For now just: */ 

	if (!(isr & ARC_TMA) && !(sc->sc_broadcast[buffer]))
		sc->sc_arccom.ac_if.if_oerrors++;
	else
		sc->sc_arccom.ac_if.if_opackets++;

#ifdef BAHTIMINGS
	clknow = clkread();

	sc->sc_stats.minsend = MIN(sc->sc_stats.minsend,
	    clknow - sc->sc_stats.lasttxstart_mics);

	sc->sc_stats.maxsend = max(sc->sc_stats.maxsend,
	    clknow - sc->sc_stats.lasttxstart_mics);
#endif

	/* We know we can accept another buffer at this point. */
	sc->sc_arccom.ac_if.if_flags &= ~IFF_OACTIVE;

	if (--sc->sc_tx_fillcount > 0) {

		/* 
		 * start tx on other buffer.
		 * This also clears the int flag
		 */
		buffer ^= 1;
		sc->sc_tx_act = buffer;

		/*
		 * already given:
		 * sc->sc_intmask |= ARC_TA; 
		 * sc->sc_base->status = sc->sc_intmask;
		 */
		sc->sc_base->command = ARC_TX(buffer);

#ifdef BAHTIMINGS
		bcopy((caddr_t)&time,
		    (caddr_t)&(sc->sc_stats.lasttxstart_tv),
		    sizeof(struct timeval));

		sc->sc_stats.lasttxstart_mics = clkread();
#endif
 
#if defined(BAH_DEBUG) && (BAH_DEBUG > 1)
		printf("bah%ld: tint: starting tx on buffer %d,\
status 0x%02x\n", 
		    sc->sc_arccom.ac_if.if_unit,
		    buffer,sc->sc_base->status);
#endif
	} else {
		/* have to disable TX interrupt */
		sc->sc_intmask &= ~ARC_TA;
		sc->sc_base->status = sc->sc_intmask;

#ifdef BAH_DEBUG
		printf("bah%ld: tint: no more buffers to send,\
status 0x%02x\n",
		    sc->sc_arccom.ac_if.if_unit,
		    sc->sc_base->status);
#endif
	}

#ifdef BAHSOFTCOPY
	/* schedule soft int to fill a new buffer for us */
	add_sicallback(callstart, sc, NULL);
#else
	/* call it directly */
	callstart(sc, NULL);
#endif
}

/*
 * Our interrupt routine
 */
int
bahintr(sc)
	struct bah_softc *sc;
{
	u_char isr;
	int buffer;
	int unit;
	u_long newsec;

	isr = sc->sc_base->status;
	if (!(isr & sc->sc_intmask)) 
		return (0);

#if defined(BAH_DEBUG) && (BAH_DEBUG>1)
	printf("bah%ld: intr: status 0x%02x, intmask 0x%02x\n",
	    sc->sc_arccom.ac_if.if_unit,
	    isr, sc->sc_intmask);
#endif

	if (isr & ARC_POR) {
		sc->sc_arccom.ac_anaddr = sc->sc_base->dipswitches;
		sc->sc_base->command = ARC_CLR(CLR_POR);
		log(LOG_WARNING,
		    "%s%ld: intr: got spurious power on reset int\n",
		    sc->sc_arccom.ac_if.if_name,
		    sc->sc_arccom.ac_if.if_unit);
	}

	if (isr & ARC_RECON) {
		/*
		 * we dont need to:
		 * sc->sc_base->command = ARC_CONF(CONF_LONG);
		 */
		sc->sc_base->command = ARC_CLR(CLR_RECONFIG);
		sc->sc_arccom.ac_if.if_collisions++;
/*
 * if more than 2 seconds per reconfig, reset time and counter.
 * else
 * if more than ARC_EXCESSIVE_RECONFIGS reconfigs since last burst, complain
 * and set treshold for warnings to ARC_EXCESSIVE_RECONS_REWARN.
 * This allows for, e.g., new stations on the cable, or cable switching as long 
 * as it is over after (normally) 16 seconds.
 * XXX Todo: check timeout bits in status word and double time if necessary.
 */

		newsec = time.tv_sec;
		if (newsec - sc->sc_recontime > 2*sc->sc_reconcount) {
			sc->sc_recontime = newsec;
			sc->sc_reconcount = 0;
			sc->sc_reconcount_excessive = ARC_EXCESSIVE_RECONS;
		} else if (++sc->sc_reconcount > sc->sc_reconcount_excessive) {
			sc->sc_reconcount_excessive = 
			    ARC_EXCESSIVE_RECONS_REWARN;
			log(LOG_WARNING,
			    "%s%d: excessive token losses, cable problem?\n",
			    sc->sc_arccom.ac_if.if_name,
			    sc->sc_arccom.ac_if.if_unit);
			sc->sc_recontime = newsec;
			sc->sc_reconcount = 0;
		}
	}

	if (isr & ARC_RI) {

#if defined(BAH_DEBUG) && (BAH_DEBUG > 1)
		printf("bah%ld: intr: hard rint, act %ld 2:%ld 3:%ld\n",
			sc->sc_arccom.ac_if.if_unit,
			sc->sc_rx_act,sc->sc_bufstat[2],sc->sc_bufstat[3]);
#endif
	
		buffer = sc->sc_rx_act;
		sc->sc_rx_packetno = (sc->sc_rx_packetno+1)%256;
		sc->sc_bufstat[buffer] = sc->sc_rx_packetno;

		if (++sc->sc_rx_fillcount > 1) {
			sc->sc_intmask &= ~ARC_RI;
			sc->sc_base->status = sc->sc_intmask;
		} else {

			buffer ^= 1;
			sc->sc_rx_act = buffer;

			/*
			 * Start receiver on other receive buffer.
			 * This also clears the RI interupt flag.
			 */
			sc->sc_base->command = ARC_RXBC(buffer);
			/* we are in the RX intr, so mask is ok for RX */

#ifdef BAH_DEBUG
			printf("bah%ld:  started rx for buffer %ld,\
status 0x%02x\n",
			    sc->sc_arccom.ac_if.if_unit,sc->sc_rx_act,
			    sc->sc_base->status);
#endif
		}

#ifdef BAHSOFTCOPY
		/* this one starts a soft int to copy out of the hw */
		add_sicallback(bah_srint,sc,NULL);
#else
		/* this one does the copy here */
		bah_srint(sc,NULL);
#endif
	}

	if (isr & sc->sc_intmask & ARC_TA) 
		bah_tint(sc);

	return (1);
}

/*
 * Process an ioctl request. 
 * This code needs some work - it looks pretty ugly.
 */
int
bah_ioctl(ifp, command, data)
	register struct ifnet *ifp;
	unsigned long command;
	caddr_t data;
{
	struct bah_softc *sc;
	register struct ifaddr *ifa;
	int s, error;

	error = 0;
	sc  = bahcd.cd_devs[ifp->if_unit];
	ifa = (struct ifaddr *)data;
	s = splimp();

#if defined(BAH_DEBUG) && (BAH_DEBUG > 2) 
	printf("bah%ld: ioctl() called, cmd = 0x%x\n",
	    sc->sc_arccom.ac_if.if_unit, command);
#endif

	switch(command) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch(ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			bah_init(sc);	 /* before arpwhohas */
			((struct arccom *)ifp)->ac_ipaddr =
			    IA_SIN(ifa)->sin_addr;
			/* arpwhohas((struct arccom *)ifp, 
			    &IA_SIN(ifa)->sin_addr);*/
			break;
#endif
		default:
			bah_init(sc);
			break;
		}

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {

			/*
			 * If interface is marked down and it is running, 
			 * then stop it.
			 */

			bah_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;

		} else if((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {

			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */

			bah_init(sc);
		} 
		break;

		/* Multicast not supported */
	default:
		error = EINVAL;
	}

	(void)splx(s);
	return (error);
}
