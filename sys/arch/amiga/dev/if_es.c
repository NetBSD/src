/*	$NetBSD: if_es.c,v 1.1 1995/02/13 00:27:08 chopps Exp $	*/

/*
 * Copyright (c) 1995 Michael L. Hitch
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
 *      This product includes software developed by Michael L. Hitch.
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
 * SMC 91C90 Single-Chip Ethernet Controller
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
#include <amiga/amiga/isr.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/if_esreg.h>

#define SWAP(x) (((x & 0xff) << 8) | ((x >> 8) & 0xff))

#define ESDEBUG
#define	USEPKTBUF

#define	ETHER_MIN_LEN	64
#define	ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	es_softc {
	struct	device sc_dev;
	struct	isr sc_isr;
	struct	arpcom sc_ac;	/* common Ethernet structures */
#define	sc_if	sc_ac.ac_if	/* network-visible interface */
#define	sc_addr	sc_ac.ac_enaddr	/* hardware Ethernet address */
	void	*sc_base;	/* base address of board */
	short	sc_iflags;
#if NBPFILTER > 0
	caddr_t sc_bpf;
#endif
	unsigned short sc_intctl;
#ifdef ESDEBUG
	int	sc_debug;
#endif
};

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#ifdef ESDEBUG
/* console error messages */
int	esdebug = 0;
#endif

int esintr __P((struct es_softc *));
int esstart __P((struct ifnet *));
int eswatchdog __P((int));
int esioctl __P((struct ifnet *, u_long, caddr_t));
void esrint __P((struct es_softc *));
void estint __P((struct es_softc *));
void esinit __P((struct es_softc *));
void esreset __P((struct es_softc *));

extern	struct ifnet loif;

void esattach __P((struct device *, struct device *, void *));
int esmatch __P((struct device *, struct cfdata *, void *args));

struct cfdriver escd = {
	NULL, "es", (cfmatch_t)esmatch, esattach, DV_IFNET,
	sizeof(struct es_softc), NULL, 0};

int
esmatch(pdp, cfp, auxp)
	struct device *pdp;
	struct cfdata *cfp;
	void *auxp;
{

	struct zbus_args *zap;

	zap = (struct zbus_args *)auxp;

	/* Ameristar A4066 ethernet card */
	if ( zap->manid == 1053 && zap->prodid == 10)
		return(1);

	return (0);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
esattach(pdp, dp, auxp)
	struct device *pdp, *dp;
	void *auxp;
{
	struct zbus_args *zap;
	struct es_softc *sc = (struct es_softc *) dp;
	struct ifnet *ifp = &sc->sc_if;
	char *cp;
	int i;
	unsigned long ser;

	zap =(struct zbus_args *)auxp;

	/*
	 * Make config msgs look nicer.
	 */
/*	printf("\n");*/

	sc->sc_base = zap->va;

	/*
	 * Manufacturer decides the 3 first bytes, i.e. ethernet vendor ID.
	 * (Currently only Ameristar.)
	 */
	sc->sc_addr[0] = 0x00;
	sc->sc_addr[1] = 0x00;
	sc->sc_addr[2] = 0x9f;

	/*
	 * Serial number for board contains last 3 bytes.
	 */
	ser = (unsigned long) zap->serno;

	sc->sc_addr[3] = (ser >> 16) & 0xff;
	sc->sc_addr[4] = (ser >>  8) & 0xff;
	sc->sc_addr[5] = (ser      ) & 0xff;

#if 0
	printf("es%d: hardware address %s\n",
	    dp->dv_unit, ether_sprintf(sc->sc_addr));
#else
	printf(": address %s\n", ether_sprintf(sc->sc_addr));
#endif

	ifp->if_unit = dp->dv_unit;
	ifp->if_name = escd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_ioctl = esioctl;
	ifp->if_start = esstart;
	ifp->if_watchdog = eswatchdog;
	/* XXX IFF_MULTICAST */
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;

	if_attach(ifp);
	ether_ifattach(ifp);
#if NBPFILTER > 0
	bpfattach(&sc->sc_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->sc_isr.isr_intr = esintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr (&sc->sc_isr);
	return;
}

void
esinit(sc)
	struct es_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ac.ac_if;
	union smcregs *smc = sc->sc_base;
	int s;

	if (!ifp->if_addrlist)
		return;

	s = splimp();

	smc->b0.bsr = 0x0000;		/* Select bank 0 */
	smc->b0.rcr = RCR_EPH_RST;
	smc->b0.rcr = 0;
	smc->b3.bsr = 0x0300;		/* Select bank 3 */
	smc->b3.mt[0] = 0;		/* clear Multicast table */
	smc->b3.mt[1] = 0;
	smc->b3.mt[2] = 0;
	smc->b3.mt[3] = 0;
	/* XXX set Multicast table from Multicast list */
	smc->b1.bsr = 0x0100;		/* Select bank 1 */
	smc->b1.cr = CR_RAM32K | CR_NO_WAIT_ST | CR_SET_SQLCH;
	smc->b1.ctr = CTR_AUTO_RLSE;
	smc->b1.iar[0] = *((unsigned short *) &sc->sc_addr[0]);
	smc->b1.iar[1] = *((unsigned short *) &sc->sc_addr[2]);
	smc->b1.iar[2] = *((unsigned short *) &sc->sc_addr[4]);
	smc->b2.bsr = 0x0200;		/* Select bank 2 */
	smc->b2.mmucr = MMUCR_RESET;
	smc->b0.bsr = 0x0000;		/* Select bank 0 */
	smc->b0.mcr = SWAP (0x0020);	/* reserve 8K for transmit buffers */
	smc->b0.tcr = TCR_PAD_EN | TCR_TXENA + TCR_MON_CSN;
	smc->b0.rcr = RCR_FILT_CAR | RCR_STRIP_CRC | RCR_RXEN;
	/* XXX add multicast/promiscuous flags */
	smc->b2.bsr = 0x0200;		/* Select bank 2 */
	smc->b2.msk = sc->sc_intctl = MSK_RX_OVRN | MSK_RX;

	/* Interface is now 'running', with no output active. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Attempt to start output, if any. */
	esstart(ifp);

	splx(s);
}

int
esintr(sc)
	struct es_softc *sc;
{
	int i;
	int done = 0;
	union smcregs *smc;

	smc = sc->sc_base;
	if ((smc->b2.msk & smc->b2.ist) == 0)
		return (0);
#ifdef ESDEBUG
	if (esdebug)
		printf ("%s: esintr ist %02x msk %02x",
		    sc->sc_dev.dv_xname, smc->b2.ist, smc->b2.msk);
#endif
	smc->b2.msk = 0;
#ifdef ESDEBUG
	if (esdebug)
		printf ("=>%02x%02x pnr %02x arr %02x fifo %04x\n",
		    smc->b2.ist, smc->b2.ist, smc->b2.pnr, smc->b2.arr,
		    smc->b2.fifo);
#endif
	if (smc->b2.ist & IST_ALLOC && sc->sc_intctl & MSK_ALLOC) {
		sc->sc_intctl &= ~MSK_ALLOC;
#ifdef ESDEBUG
		if (esdebug || 1)
			printf ("%s: ist %02x\n", sc->sc_dev.dv_xname,
			    smc->b2.ist);
#endif
		if ((smc->b2.arr & ARR_FAILED) == 0) {
#ifdef ESDEBUG
			if (esdebug || 1)
				printf ("%s: arr %02x\n", sc->sc_dev.dv_xname,
				    smc->b2.arr);
#endif
			smc->b2.pnr = smc->b2.arr;
			smc->b2.mmucr = MMUCR_RLSPKT;
			while (smc->b2.mmucr & MMUCR_BUSY)
				;
			sc->sc_ac.ac_if.if_flags &= ~IFF_OACTIVE;
		}
	}
	while ((smc->b2.fifo & FIFO_REMPTY) == 0) {
		esrint(sc);
	}
	if (smc->b2.ist & IST_RX_OVRN) {
		printf ("%s: Overrun ist %02x", sc->sc_dev.dv_xname,
		    smc->b2.ist);
		smc->b2.ist = ACK_RX_OVRN;
		printf ("->%02x\n", smc->b2.ist);
		sc->sc_if.if_ierrors++;
	}
	if (smc->b2.ist & IST_TX) {
#ifdef ESDEBUG
		if (esdebug)
			printf ("%s: TX INT ist %02x",
			    sc->sc_dev.dv_xname, smc->b2.ist);
#endif
		smc->b2.ist = ACK_TX;
#ifdef ESDEBUG
		if (esdebug)
			printf ("->%02x\n", smc->b2.ist);
#endif
		smc->b0.bsr = 0x0000;
		printf ("%s: EPH status %04x\n", sc->sc_dev.dv_xname,
		   smc->b0.ephsr);
		if ((smc->b0.ephsr & EPHSR_TX_SUC) == 0) {
			smc->b0.tcr |= TCR_TXENA;
			sc->sc_if.if_oerrors++;
		}
		smc->b2.bsr = 0x0200;
	}
	if (smc->b2.ist & IST_TX_EMPTY) {
		u_short ecr;
#ifdef ESDEBUG
		if (esdebug)
			printf ("%s: TX EMPTY %02x",
			    sc->sc_dev.dv_xname, smc->b2.ist);
#endif
		smc->b2.ist = ACK_TX_EMPTY;
#ifdef ESDEBUG
		if (esdebug)
			printf ("->%02x intcl %x pnr %02x arr %02x\n",
			    smc->b2.ist, sc->sc_intctl, smc->b2.pnr,
			    smc->b2.arr);
#endif
		sc->sc_intctl &= ~(MSK_TX_EMPTY | MSK_TX);
		smc->b0.bsr = 0x0000;
		ecr = smc->b0.ecr;	/* Get error counters */
		if (ecr & 0xff00)
			sc->sc_if.if_collisions += ((ecr >> 8) & 15) +
			    ((ecr >> 11) & 0x1e);
		smc->b2.bsr = 0x0200;
	}
	/* output packets */
	estint(sc);
	smc->b2.msk = sc->sc_intctl;
	return (1);
}

void
esrint (sc)
	struct es_softc *sc;
{
	union smcregs *smc = sc->sc_base;
	int i;
	u_short len;
	short cnt;
	u_short pktctlw, pktlen, *buf;
	u_long *lbuf;
	volatile u_short *data;
	volatile u_long *ldata;
	struct ifnet *ifp;
	struct mbuf *top, **mp, *m;
	struct ether_header *eh;
#ifdef USEPKTBUF
	u_char *b, pktbuf[1530];
#endif

#ifdef ESDEBUG
	if (esdebug)
		printf ("%s: esrint fifo %04x", sc->sc_dev.dv_xname,
		    smc->b2.fifo);
#endif
	data = (u_short *)&smc->b2.data;
	smc->b2.ptr = PTR_RCV | PTR_AUTOINCR | PTR_READ | SWAP (0x0002);
	(void) smc->b2.mmucr;
#ifdef ESDEBUG
	if (esdebug)
		printf ("->%04x", smc->b2.fifo);
#endif
	len = *data;
	len = SWAP (len);	/* Careful of macro side-effects */
#ifdef ESDEBUG
	if (esdebug)
		printf (" length %d", len);
#endif
	smc->b2.ptr = PTR_RCV | PTR_AUTOINCR + PTR_READ | SWAP (0x0000);
	(void) smc->b2.mmucr;
	pktctlw = *data;
	pktlen = *data;
	pktctlw = SWAP (pktctlw);
	pktlen = SWAP (pktlen) - 6;
	if (pktctlw & RFSW_ODDFRM)
		pktlen++;
	/* XXX copy directly from controller to mbuf */
#ifdef USEPKTBUF
#if 1
	lbuf = (u_long *) pktbuf;
	ldata = (u_long *)data;
	cnt = (len - 4) / 4;
	while (cnt--)
		*lbuf++ = *ldata;
	if ((len - 4) & 2) {
		buf = (u_short *) lbuf;
		*buf = *data;
	}
#else
	buf = (u_short *)pktbuf;
	cnt = (len - 4) / 2;
	while (cnt--)
		*buf++ = *data;
#endif
	smc->b2.mmucr = MMUCR_REMRLS_RX;
	while (smc->b2.mmucr & MMUCR_BUSY)
		;
#ifdef ESDEBUG
	if (pktctlw & (RFSW_ALGNERR | RFSW_BADCRC | RFSW_TOOLNG | RFSW_TOOSHORT)) {
		printf ("%s: Packet error %04x\n", sc->sc_dev.dv_xname, pktctlw);
		/* count input error? */
	}
	if (esdebug) {
		printf (" pktctlw %04x pktlen %04x fifo %04x\n", pktctlw, pktlen,
		    smc->b2.fifo);
		for (i = 0; i < pktlen; ++i)
			printf ("%02x%s", pktbuf[i], ((i & 31) == 31) ? "\n" :
			    "");
		if (i & 31)
			printf ("\n");
	}
#endif
#else	/* USEPKTBUF */
#ifdef ESDEBUG
	if (pktctlw & (RFSW_ALGNERR | RFSW_BADCRC | RFSW_TOOLNG | RFSW_TOOSHORT)) {
		printf ("%s: Packet error %04x\n", sc->sc_dev.dv_xname, pktctlw);
		/* count input error? */
	}
	if (esdebug) {
		printf (" pktctlw %04x pktlen %04x fifo %04x\n", pktctlw, pktlen,
		    smc->b2.fifo);
	}
#endif
#endif /* USEPKTBUF */
	ifp = &sc->sc_ac.ac_if;
	ifp->if_ipackets++;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = pktlen;
	len = MHLEN;
	top = NULL;
	mp = &top;
#ifdef USEPKTBUF
	b = pktbuf;
#endif
	while (pktlen > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return;
			}
			len = MLEN;
		}
		if (pktlen >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if (m->m_flags & M_EXT)
				len = MCLBYTES;
		}
		m->m_len = len = min(pktlen, len);
#ifdef USEPKTBUF
		bcopy((caddr_t)b, mtod(m, caddr_t), len);
		b += len;
#else	/* USEPKTBUF */
		buf = mtod(m, u_short *);
		cnt = len / 2;
		while (cnt--)
			*buf++ = *data;
		if (len & 1)
			*buf = *data;	/* XXX should be byte store */
#ifdef ESDEBUG
		if (esdebug) {
			buf = mtod(m, u_short *);
			for (i = 0; i < len; ++i)
				printf ("%02x%s", ((u_char *)buf)[i],
				    ((i & 31) == 31) ? "\n" : "");
			if (i & 31)
				printf ("\n");
		}
#endif
#endif	/* USEPKTBUF */
		pktlen -= len;
		*mp = m;
		mp = &m->m_next;
	}
#ifndef USEPKTBUF
	smc->b2.mmucr = MMUCR_REMRLS_RX;
	while (smc->b2.mmucr & MMUCR_BUSY)
		;
#endif
	eh = mtod(top, struct ether_header *);
	top->m_pkthdr.len -= sizeof (*eh);
	top->m_len -= sizeof (*eh);
	top->m_data += sizeof (*eh);

	ether_input(ifp, eh, top);
}

void
estint (sc)
	struct es_softc *sc;
{
	esstart (&sc->sc_ac.ac_if);
}

int
esstart(ifp)
	struct ifnet *ifp;
{
	struct es_softc *sc = escd.cd_devs[ifp->if_unit];
	union smcregs *smc = sc->sc_base;
	struct mbuf *m0, *m;
#ifdef USEPKTBUF
	u_short pktbuf[ETHER_MAX_LEN + 2];
#endif
	u_short pktctlw, pktlen;
	u_short *buf;
	volatile u_short *data;
	u_long *lbuf;
	volatile u_long *ldata;
	short cnt;
	int i;

	if ((sc->sc_ac.ac_if.if_flags & (IFF_RUNNING | IFF_OACTIVE)) !=
	    IFF_RUNNING)
		return;

	for (;;) {
		/*
		 * Sneak a peek at the next packet to get the length
		 * and see if the SMC 91C90 can accept it.
		 */
		m = sc->sc_ac.ac_if.if_snd.ifq_head;
		if (!m)
			break;
#ifdef ESDEBUG
if (esdebug && (m->m_next || m->m_len & 1))
  printf("%s: esstart m_next %x m_len %d\n", sc->sc_dev.dv_xname,
    m->m_next, m->m_len);
#endif
		for (m0 = m, pktlen = 0; m0; m0 = m0->m_next)
			pktlen += m0->m_len;
#ifdef ESDEBUG
if (esdebug)
  printf("%s: esstart r1 %x len %d", sc->sc_dev.dv_xname, smc->b2.pnr, pktlen);
#endif
		pktctlw = 0;
		pktlen += 4;
		if (pktlen & 1)
			++pktlen;	/* control byte after last byte */
		else
			pktlen += 2;	/* control byte after pad byte */
		smc->b2.mmucr = MMUCR_ALLOC | (pktlen & 0x0700);
		for (i = 0; i <= 5; ++i)
			if ((smc->b2.arr & ARR_FAILED) == 0)
				break;
		if (smc->b2.arr & ARR_FAILED) {
			sc->sc_ac.ac_if.if_flags |= IFF_OACTIVE;
#ifdef ESDEBUG
if (esdebug)
  printf("-no xmit bufs r1 %x\n", smc->b2.pnr);
#endif
			sc->sc_intctl |= 0x0008;
			break;
		}
		smc->b2.pnr = smc->b2.arr;

		IF_DEQUEUE(&sc->sc_ac.ac_if.if_snd, m);
		smc->b2.ptr = PTR_AUTOINCR;
		(void) smc->b2.mmucr;
		data = (u_short *)&smc->b2.data;
		*data = SWAP (pktctlw);
		*data = SWAP (pktlen);
#ifdef USEPKTBUF
		i = 0;
		for (m0 = m; m; m = m->m_next) {
			bcopy(mtod(m, caddr_t), (char *)pktbuf + i, m->m_len);
			i += m->m_len;
		}

		if (i & 1)	/* Figure out where to put control byte */
			pktbuf[i/2] = (pktbuf[i/2] & 0xff00) | CTLB_ODD;
		else
			pktbuf[i/2] = 0;
#ifdef ESDEBUG
if (esdebug) {
  int j;
  printf("-sending len %d pktlen %d r1 %04x\n", i, pktlen, smc->b2.pnr);
  for (j = 0; j < pktlen - 4; ++j)
    printf("%02x%s", ((u_char *)pktbuf)[j], ((j & 31) == 31) ? "\n" : "");
  if (j & 31)
    printf("\n");
}
#endif
#if 0 /* doesn't quite work? */
		lbuf = (u_long *)(pktbuf);
		ldata = (u_long *)data;
		cnt = pktlen / 4;
		while(cnt--)
			*ldata = *lbuf++;
		if (pktlen & 2) {
			buf = (u_short *)lbuf;
			*data = *buf;
		}
#else
		buf = pktbuf;
		cnt = pktlen / 2;
		while (cnt--)
			*data = *buf++;
#endif
#else	/* USEPKTBUF */
#ifdef ESDEBUG
if (esdebug)
  printf("-sending pktlen %d r1 %04x\n", pktlen, smc->b2.pnr);
#endif
		pktctlw = 0;
		for (m0 = m; m; m = m->m_next) {
			buf = mtod(m, u_short *);
#ifdef ESDEBUG
if (esdebug) {
  printf(" m->m_len %d\n", m->m_len);
  for (i = 0; i < m->m_len; ++i)
    printf("%02x%s", ((u_char *)buf)[i], ((i & 31) == 31) ? "\n" : "");
  if (i & 31)
    printf("\n");
}
#endif
			cnt = m->m_len / 2;
			while (cnt--)
				*data = *buf++;
			if (m->m_len & 1)
				pktctlw = (*buf & 0xff00) | CTLB_ODD;
		}
		*data = pktctlw;
#endif	/* USEPKTBUF */
		smc->b2.mmucr = MMUCR_ENQ_TX;
#ifdef ESDEBUG
if (esdebug)
  printf("%s: esstart packet sent r1 %x\n", sc->sc_dev.dv_xname, smc->b2.pnr);
#endif
#if NBPFILTER > 0
		if (sc->sc_ac.ac_if.if_bpf)
			bpf_mtap(sc->sc_ac.ac_if.if_bpf, m0);
#endif
		m_freem(m0);
		sc->sc_ac.ac_if.if_opackets++;	/* move to interrupt? */
		sc->sc_intctl |= 0x0006;
	}
	smc->b2.msk = sc->sc_intctl;
	return 0;
}

int
esioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct es_softc *sc = escd.cd_devs[ifp->if_unit];
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

#ifdef ESDEBUG
	if (esdebug)
		printf ("esioctl called: cmd %x\n", cmd);
#endif

	s = splimp();

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			esinit(sc);
			sc->sc_ac.ac_ipaddr = IA_SIN(ifa)->sin_addr;
			arpwhohas(&sc->sc_ac, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(uniion ns_host *)(sc->sc_addr);
			else
				bcopy(ina->x_host.c_host,
				    sc->sc_addr, sizeof(sc->sc_addr));
			/* Set new address */
			esinit(sc);
			break;
#endif
		default:
			esinit(sc);
			break;
		}
		break;
	case SIOCSIFFLAGS:
		/*
		 * If interface is marked down and it is running, then stop it
		 */
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			/* esstop(sc); */
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			esinit(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			/* esstop(sc); */
			esinit(sc);
		}
#ifdef ESDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			esdebug = sc->sc_debug = 1;
		else
			esdebug = sc->sc_debug = 0;
#endif
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

void
esreset(sc)
	struct es_softc *sc;
{
	int s;

	s = splimp();
	/* esstop(sc); */
	esinit(sc);
	splx(s);
}

int
eswatchdog(unit)
	int unit;
{
	register struct es_softc *sc = escd.cd_devs[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	esreset(sc);
	return (0);
}
