/* $NetBSD: if_es.c,v 1.7 1997/03/15 18:09:32 is Exp $ */

/*
 * Copyright (c) 1996, Danny C Tsen.
 * Copyright (c) 1996, VLSI Technology Inc. All Rights Reserved.
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
 * Modified from Amiga's es driver for SMC 91C9x ethernet controller.
 */

/*
 * SMC 91C90 Single-Chip Ethernet Controller
 */

#include "bpfilter.h"

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
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/cpu.h>
#include <machine/irqhandler.h>
#include <machine/io.h>
#include <machine/katelib.h>
#include <arm32/mainbus/if_esreg.h>
#include <arm32/mainbus/mainbus.h>

#define SWAP(x) (((x & 0xff) << 8) | ((x >> 8) & 0xff))

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * es_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	es_softc {
	struct	device sc_dev;
	irqhandler_t sc_ih;
	struct	ethercom sc_ethercom;	/* common Ethernet structures */
	u_int	sc_base;	/* base address of board */
	int	nrxovrn;
	u_int	sc_intctl;
	u_char	pktbuf[1530];
#ifdef ESDEBUG
	int	sc_debug;
	int	sc_intbusy;
	int	sc_smcbusy;
#endif
};

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#ifdef ESDEBUG
/* console error messages */
int	esdebug = 0;
int	estxints = 0;	/* IST_TX with TX enabled */
int	estxint2 = 0;	/* IST_TX active after IST_TX_EMPTY */
int	estxint3 = 0;	/* IST_TX interrupt processed */
int	estxint4 = 0;	/* ~TEMPTY counts */
int	estxint5 = 0;	/* IST_TX_EMPTY interrupts */
#endif

static int esintr __P((void *));
static void estint __P((struct ifnet *));
static void esstart __P((struct ifnet *));
static void eswatchdog __P((struct ifnet *));
static int esioctl __P((struct ifnet *, u_long, caddr_t));
static void esrint __P((struct es_softc *));
static void esinit __P((struct es_softc *));
static void esreset __P((struct es_softc *));

int esprobe __P((struct device *, void *, void *));
void esattach __P((struct device *, struct device *, void *));

struct cfattach es_ca = {
	sizeof(struct es_softc), esprobe, esattach
};

struct cfdriver es_cd = {
	NULL, "es", DV_IFNET
};

int
esprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
/*
 * This is a driver for the ethernet controller on the RC7500 motherboard.
 * It would be nice to be able to probe rather that using conditional
 * compilation. Alternatively we should probe for the RC7500 and
 * use a flag for that to enable the driver.
 */

#ifdef RC7500
	return(1);
#else
	return(0);
#endif
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */

/*
 * XXX - FIX ME
 * fake an Ethernet address.
 */
unsigned long vendor_id = 0x007f00;	/* just make one */

void
esattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct es_softc *sc = (void *)self;
	struct mainbus_attach_args *mbap = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	unsigned long ser;
	u_int8_t myaddr[ETHER_ADDR_LEN];

	sc->sc_base = mbap->mb_iobase;

	ser = 0x5B98FF;
	/*
	 * Manufacturer decides the 3 first bytes, i.e. ethernet vendor ID.
	 * (Currently only Ameristar.)
	 */
	myaddr[0] = (vendor_id >> 16) & 0xff;
	myaddr[1] = (vendor_id >> 8) & 0xff;
	myaddr[2] = vendor_id & 0xff;

	/*
	 * XXX We should have a serial number on board!!!!
	 * Serial number for board contains last 3 bytes.
	 */
	myaddr[3] = (ser >> 16) & 0xff;
	myaddr[4] = (ser >>  8) & 0xff;
	myaddr[5] = (ser      ) & 0xff;

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_output = ether_output;
	ifp->if_ioctl = esioctl;
	ifp->if_start = estint;
	ifp->if_watchdog = eswatchdog;
	/* XXX IFF_MULTICAST */
	ifp->if_flags = IFF_BROADCAST | IFF_MULTICAST | IFF_NOTRAILERS;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, myaddr);

	/* Print additional info when attached. */
	printf(": address %s\n", ether_sprintf(myaddr));

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->sc_ih.ih_func = esintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_NET;
	if (irq_claim(IRQ_ETHERNET, &sc->sc_ih) == -1)
		panic("Cannot installed SMC 91C9x Ethernet IRQ handler\n");
}

#ifdef ESDEBUG
static void
es_dump_smcregs(where, smc)
	char *where;
	union smcregs *smc;
{
	u_int cur_bank = smc->b0.bsr & BSR_BANKMSK;

	printf("SMC registers %08x from %s bank %04x\n", smc, where,
	    smc->b0.bsr);
	smc->b0.bsr = BSR_BANK0;
	printf("TCR %04x EPHSR %04x RCR %04x ECR %04x MIR %04x MCR %04x\n",
	    smc->b0.tcr, smc->b0.ephsr, smc->b0.rcr,
	    smc->b0.ecr, smc->b0.mir, smc->b0.mcr);
	smc->b1.bsr = BSR_BANK1;
	printf("CR %04x BAR %04x IAR %04x %04x %04x GPR %04x CTR %04x\n",
	    smc->b1.cr, smc->b1.bar, smc->b1.iar[0], smc->b1.iar[1],
	    smc->b1.iar[2], smc->b1.gpr, smc->b1.ctr);
	smc->b2.bsr = BSR_BANK2;
	printf("MMUCR %04x PNR %02x ARR %02x FIFO %04x PTR %04x",
	    smc->b2.mmucr, smc->b2.pnr, smc->b2.arr, smc->b2.fifo,
	    smc->b2.ptr);
	printf(" DATA %04x %04x IST %02x MSK %02x\n", smc->b2.data,
	    smc->b2.datax, smc->b2.ist, smc->b2.msk);
	smc->b3.bsr = BSR_BANK3;
	printf("MT %04x %04x %04x %04x\n",
	    smc->b3.mt[0], smc->b3.mt[1], smc->b3.mt[2], smc->b3.mt[3]);
	smc->b3.bsr = cur_bank;
}
#endif

static void
esstop(sc)
	struct es_softc* sc;
{
}

static void
esinit(sc)
	struct es_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	register u_int iobase = sc->sc_base;
	int s;

	s = splimp();

#ifdef ESDEBUG
	if (ifp->if_flags & IFF_RUNNING)
		es_dump_smcregs("esinit", smc);
#endif
	outl(iobase + BANKSEL, BSR_BANK0);
	outl(iobase + B0RCR, RCR_EPH_RST);
	outl(iobase + B0RCR, 0);

	/*
	 * clear Multicast table
	 */
	outl(iobase + BANKSEL, BSR_BANK3);
	outl(iobase + B3MT0, 0);
	outl(iobase + B3MT2, 0);
	outl(iobase + B3MT4, 0);
	outl(iobase + B3MT6, 0);

	/* XXX set Multicast table from Multicast list */
	outl(iobase + BANKSEL, BSR_BANK1);
	outl(iobase + B1CR, (CR_ALLONES | CR_NO_WAIT_ST | CR_16BIT));
	outl(iobase + B1CTR, CTR_AUTO_RLSE);
	outl(iobase + B1IAR1, *((u_short *) LLADDR(ifp->if_sadl)[0]));
	outl(iobase + B1IAR3, *((u_short *) LLADDR(ifp->if_sadl)[2]));
	outl(iobase + B1IAR5, *((u_short *) LLADDR(ifp->if_sadl)[4]));

	outl(iobase + BANKSEL, BSR_BANK2);
	outl(iobase + B2MMUCR, MMUCR_RESET);

	outl(iobase + BANKSEL, BSR_BANK0);
	outl(iobase + B0MCR, 0); 	/* reserve 0 x 256 bytes for transmit buffers */
	outl(iobase + B0TCR, (TCR_PAD_EN | TCR_TXENA | TCR_MON_CSN | TCR_FDUPLX));
	outl(iobase + B0RCR, (RCR_STRIP_CRC | RCR_RXEN));

	/* XXX add multicast/promiscuous flags */
	outl(iobase + BANKSEL, BSR_BANK2);
	outb(iobase + B2MSK, (MSK_RX_OVRN | MSK_RX));

	sc->sc_intctl = MSK_RX_OVRN | MSK_RX;
	sc->nrxovrn = 0;

	/* Interface is now 'running', with no output active. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

#ifdef ESDEBUG
	if (ifp->if_flags & IFF_RUNNING)
		es_dump_smcregs("esinit", smc);
#endif
	/* Attempt to start output, if any. */
	esstart(ifp);
 
	splx(s);
}

static int
esintr(arg)
	void *arg;
{
	struct es_softc *sc = arg;
	u_int intsts, intact;
	register u_int iobase;
	int n = 4;

	iobase = sc->sc_base;

#ifdef ESDEBUG
	while ((smc->b2.bsr & BSR_BANKMSK) != BSR_BANK2 &&
	    sc->sc_ethercom.ec_if.if_flags & IFF_RUNNING) {
		printf("%s: intr BSR not 2: %04x\n", sc->sc_dev.dv_xname,
		    smc->b2.bsr);
		smc->b2.bsr = BSR_BANK2;
	}
#endif

	intsts = inb(iobase + B2IST);
	intact = inb(iobase + B2MSK) & intsts;
	if ((intact) == 0) {
		return (0);	/* Pass interrupt on down the chain */
	}

#ifdef ESDEBUG
	if (esdebug)
		printf ("%s: esintr ist %02x msk %02x",
		    sc->sc_dev.dv_xname, intsts, smc->b2.msk);
	if (sc->sc_intbusy++) {
		printf("%s: esintr re-entered\n", sc->sc_dev.dv_xname);
		panic("esintr re-entered");
	}
#endif

	outb(iobase + B2MSK, 0);

#ifdef ESDEBUG
	if (esdebug)
		printf ("=>%02x%02x pnr %02x arr %02x fifo %04x\n",
		    smc->b2.ist, smc->b2.ist, smc->b2.pnr, smc->b2.arr,
		    smc->b2.fifo);
#endif

	while (n-- && ((inl(iobase + B2FIFO) & FIFO_REMPTY) == 0)) {
		esrint(sc);
	}

	if (intact & IST_RX_OVRN) {
		printf ("%s: Overrun ist %02x", sc->sc_dev.dv_xname,
		    intsts);
		outb(iobase + B2IST, ACK_RX_OVRN);
		printf ("->%02x\n", inb(iobase + B2IST));
		sc->sc_ethercom.ec_if.if_ierrors++;
		if (sc->nrxovrn++ >= 10) {
			outl(iobase + B2MMUCR, MMUCR_RESET);
			sc->nrxovrn = 0;
		}
	}

	if (intact & IST_ALLOC) {
		u_char b2arr;
		sc->sc_intctl &= ~MSK_ALLOC;
		b2arr = inb(iobase + B2ARR);
		if ((b2arr & ARR_FAILED) == 0) {
			u_char save_pnr;
			save_pnr = inb(iobase + B2PNR);
			outb(iobase + B2PNR, b2arr);
			outl(iobase + B2MMUCR, MMUCR_RLSPKT);
			/*
			 * we don't want to wait forever.
			 */
			for (n = 100; n ; n--) {
				if (!(inl(iobase + B2MMUCR) & MMUCR_BUSY))
					break;
			}
			outb(iobase + B2PNR, save_pnr);
			sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;
		}
#ifdef ESDEBUG
		else if (esdebug || 1) {
			printf ("%s: ist %02x arr %02x\n", sc->sc_dev.dv_xname,
			    intsts, smc->b2.arr);
			printf (" IST_ALLOC with ARR_FAILED?\n");
		}
#endif
	}

	if (intact & IST_TX_EMPTY) {
		u_int ecr;
#ifdef ESDEBUG
		if (esdebug)
			printf ("%s: TX EMPTY %02x",
			    sc->sc_dev.dv_xname, intsts);
		++estxint5;		/* count # IST_TX_EMPTY ints */
#endif
		outb(iobase + B2IST, ACK_TX_EMPTY);
		sc->sc_intctl &= ~(MSK_TX_EMPTY | MSK_TX);
#ifdef ESDEBUG
		if (esdebug)
			printf ("->%02x intcl %x pnr %02x arr %02x\n",
			    smc->b2.ist, sc->sc_intctl, smc->b2.pnr,
			    smc->b2.arr);
#endif
		if (inb(iobase + B2IST) & IST_TX) {
			intact |= IST_TX;
#ifdef ESDEBUG
			++estxint2;		/* count # TX after TX_EMPTY */
#endif
		} else {
			outl(iobase + BANKSEL, BSR_BANK0);
			ecr = inl(iobase + B0ECR);	/* Get error counters */
			if (ecr & ECR_CCMSK)
				sc->sc_ethercom.ec_if.if_collisions += (ecr & 0x0f) +
				    ((ecr >> 4) & 0x0f);
			outl(iobase + BANKSEL, BSR_BANK2);
		}
	}
	if (intact & IST_TX) {
		u_char tx_pnr, save_pnr;
		u_int save_ptr, ephsr, tcr;
#ifdef ESDEBUG
		if (esdebug) {
			printf ("%s: TX INT ist %02x",
			    sc->sc_dev.dv_xname, intsts);
			printf ("->%02x\n", smc->b2.ist);
		}
		++estxint3;			/* count # IST_TX */
#endif

		n = 0;
zzzz:

#ifdef ESDEBUG
		++estxint4;			/* count # ~TEMPTY */
#endif
		outl(iobase + BANKSEL, BSR_BANK0);
		ephsr = inl(iobase + B0EPHSR);		/* get EPHSR */
		tcr = inl(iobase + B0TCR);		/* and TCR */
		outl(iobase + BANKSEL, BSR_BANK2);
		save_ptr = inl(iobase + B2PTR);
		save_pnr = inb(iobase + B2PNR);
		tx_pnr = inl(iobase + B2FIFO) >> 8;	/* pktno from completion fifo */
		outb(iobase + B2PNR, tx_pnr);		/* set TX packet number */
		outl(iobase + B2PTR, PTR_READ);		/* point to status word */

		if ((ephsr & EPHSR_TX_SUC) == 0 && (tcr & TCR_TXENA) == 0) {
			/*
			 * Transmitter was stopped for some error.  Enqueue
			 * the packet again and restart the transmitter.
			 * May need some check to limit the number of retries.
			 */
			outl(iobase + B2MMUCR, MMUCR_ENQ_TX);
			outl(iobase + BANKSEL, BSR_BANK0);
			outl(iobase + B0TCR, inl(iobase + B0TCR) | TCR_TXENA);
			outl(iobase + BANKSEL, BSR_BANK2);
			sc->sc_ethercom.ec_if.if_oerrors++;
			sc->sc_intctl |= MSK_TX_EMPTY | MSK_TX;
		} else {
			/*
			 * This shouldn't have happened:  IST_TX indicates
			 * the TX completion FIFO is not empty, but the
			 * status for the packet on the completion FIFO
			 * shows that the transmit was sucessful.  Since
			 * AutoRelease is being used, a sucessful transmit
			 * should not result in a packet on the completion
			 * FIFO.  Also, that packet doesn't seem to want
			 * to be acknowledged.  If this occurs, just reset
			 * the TX FIFOs.
			 */
			if (inb(iobase + B2IST) & IST_TX_EMPTY) {
				outl(iobase + B2MMUCR, MMUCR_RESET_TX);
				sc->sc_intctl &= ~(MSK_TX_EMPTY | MSK_TX);
			}
#ifdef ESDEBUG
			++estxints;	/* count IST_TX with TX enabled */
#endif
		}
		outb(iobase + B2PNR, save_pnr);
		outl(iobase + B2PTR, save_ptr);
		outb(iobase + B2IST, ACK_TX);

		if ((inl(iobase + B2FIFO) & FIFO_TEMPTY) == 0 && n++ < 10) {
			if (tx_pnr != (inl(iobase + B2FIFO) >> 8))
				goto zzzz;
		}
	}

	/* output packets */
	estint(&sc->sc_ethercom.ec_if);

#ifdef ESDEBUG
	while ((smc->b2.bsr & BSR_BANKMSK) != BSR_BANK2) {
		printf("%s: intr+++ BSR not 2: %04x\n", sc->sc_dev.dv_xname,
		    smc->b2.bsr);
		smc->b2.bsr = BSR_BANK2;
	}
#endif

	outb(iobase + B2MSK, sc->sc_intctl & 0xff);

#ifdef ESDEBUG
	if (--sc->sc_intbusy) {
		printf("%s: esintr busy on exit\n", sc->sc_dev.dv_xname);
		panic("esintr busy on exit");
	}
#endif

	return (1);	/* Claim interrupt */
}

#define NWAIT(n)	\
{\
	int nwait = n; \
	while (nwait--) ; \
}

static void
esrint(sc)
	struct es_softc *sc;
{
	register iobase = sc->sc_base;
	u_short len;
	short cnt;
	u_short pktctlw, pktlen, *buf;
	struct ifnet *ifp;
	struct mbuf *top, **mp, *m;
	struct ether_header *eh;
	u_char *b, *pktbuf;

#ifdef ESDEBUG
	if (esdebug)
		printf ("%s: esrint fifo %04x", sc->sc_dev.dv_xname,
		    smc->b2.fifo);
	if (sc->sc_smcbusy++) {
		printf("%s: esrint re-entered\n", sc->sc_dev.dv_xname);
		panic("esrint re-entered");
	}
	while ((smc->b2.bsr & BSR_BANKMSK) != BSR_BANK2) {
		printf("%s: rint BSR not 2: %04x\n", sc->sc_dev.dv_xname,
		    smc->b2.bsr);
		smc->b2.bsr = BSR_BANK2;
	}
#endif

	outl(iobase + B2PTR, (PTR_RCV | PTR_AUTOINCR | PTR_READ | 0x0000));
	NWAIT(3);
	pktctlw = (u_short) inl(iobase + B2DATA);
	pktlen = (u_short) inl(iobase + B2DATA);
	len = pktlen;
	pktlen -= 6;

	if (pktctlw & RFSW_ODDFRM)
		pktlen++;

	if (len > 1530) {
		printf("%s: Corrupted packet length-sts %04x bytcnt %04x len %04x bank %04x\n",
		    sc->sc_dev.dv_xname, pktctlw, pktlen, len, inl(iobase + BANKSEL));
		/* XXX ignore packet, or just truncate? */
#if defined(ESDEBUG) && defined(DDB)
		if ((smc->b2.bsr & BSR_BANKMSK) != BSR_BANK2)
			Debugger();
#endif
		outl(iobase + BANKSEL, BSR_BANK2);
		outl(iobase + B2MMUCR, MMUCR_REMRLS_RX);
		while (inl(iobase + B2MMUCR) & MMUCR_BUSY)
			;
		++sc->sc_ethercom.ec_if.if_ierrors;
#ifdef ESDEBUG
		if (--sc->sc_smcbusy) {
			printf("%s: esrintr busy on bad packet exit\n",
			    sc->sc_dev.dv_xname);
			panic("esrintr busy on exit");
		}
#endif
		return;
	}

	pktbuf = sc->pktbuf;
	buf = (u_short *)pktbuf;
	cnt = (len - 4) / 2;

	while (cnt--) {
		*buf = (u_short) (inl(iobase + B2DATA) & 0x0000ffff);
		buf++;
	}

	outl(iobase + B2MMUCR, MMUCR_REMRLS_RX);
	/*
	 * We will check MMUCR_BUSY later.
	 */

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

	pktlen -= sizeof(struct ether_header);
	ifp = &sc->sc_ethercom.ec_if;
	ifp->if_ipackets++;
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = pktlen;
	len = MHLEN;
	top = NULL;
	mp = &top;

	eh = (struct ether_header *) pktbuf;

	b = pktbuf + sizeof(struct ether_header);

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
		bcopy((caddr_t)b, mtod(m, caddr_t), len);
		b += len;
		pktlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.  If so, hand off
	 * the raw packet to bpf.
	 */
	if (sc->sc_ethercom.ec_if.if_bpf) {
		bpf_tap(sc->sc_ethercom.ec_if.if_bpf, pktbuf, pktlen);
		/* bpf_mtap(sc->sc_ethercom.ec_if.if_bpf, top);*/

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((sc->sc_ethercom.ec_if.if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, LLADDR(ifp->if_sadl),
			    sizeof(eh->ether_dhost)) != 0) {
			m_freem(top);
			return;
		}
	}
#endif

	ether_input(ifp, eh, top);
#ifdef ESDEBUG
	if (--sc->sc_smcbusy) {
		printf("%s: esintr busy on exit\n", sc->sc_dev.dv_xname);
		panic("esintr busy on exit");
	}
#endif
	while (inl(iobase + B2MMUCR) & MMUCR_BUSY)
		;
}

static void
estint(ifp)
	struct ifnet *ifp;
{
	int s;
	s = splnet();
	esstart(ifp);
	splx(s);
}

static void
esstart(ifp)
	struct ifnet *ifp;
{
	struct es_softc *sc = ifp->if_softc;
	register u_int iobase = sc->sc_base;
	struct mbuf *m0, *m;
	u_short *pktbuf;
	u_short pktctlw, pktlen, len;
	u_short *buf;
	short cnt;
	int i;
	u_char active_pnr;

	if ((sc->sc_ethercom.ec_if.if_flags & (IFF_RUNNING | IFF_OACTIVE)) !=
	    IFF_RUNNING) {
		IF_DEQUEUE(&sc->sc_ethercom.ec_if.if_snd, m);
		if (m)
			m_freem(m);
		return;
	}

#ifdef ESDEBUG
	if (sc->sc_smcbusy++) {
		printf("%s: esstart re-entered\n", sc->sc_dev.dv_xname);
		panic("esstart re-entred");
	}
	while ((smc->b2.bsr & BSR_BANKMSK) != BSR_BANK2) {
		printf("%s: esstart BSR not 2: %04x\n", sc->sc_dev.dv_xname,
		    smc->b2.bsr);
		smc->b2.bsr = BSR_BANK2;
	}
#endif

	IF_DEQUEUE(&sc->sc_ethercom.ec_if.if_snd, m);
	if (!m)
		return;

	pktbuf = (u_short *) sc->pktbuf;
	for (m0 = m, pktlen = 0; m; m = m->m_next) {
		bcopy(mtod(m, caddr_t), (char *)pktbuf + pktlen, m->m_len);
		pktlen += m->m_len;
	}
	len = pktlen;
	m_freem(m0);

	if (len & 1)	{ /* Figure out where to put control byte */
		pktbuf[len/2] = (pktbuf[len/2] & 0x00ff) | CTLB_ODD;
		pktlen++;
	} else {
		pktbuf[len/2] = 0;
		pktlen += 2;
	}

	if (pktlen > (ETHERMTU + 18)) {
		printf("es: packet too long = %d bytes\n", pktlen);
	}

#if NBPFILTER > 0
	if (sc->sc_ethercom.ec_if.if_bpf)
		bpf_tap(sc->sc_ethercom.ec_if.if_bpf, (char *)pktbuf, len);
#endif

	pktctlw = 0;
	pktlen += 4;
	outl(iobase + B2MMUCR, (MMUCR_ALLOC | (pktlen >> 8)));

	for (i = 0; i <= 100; ++i) {
		if ((inb(iobase + B2ARR) & ARR_FAILED) == 0)
			break;
	}

	if (inb(iobase + B2ARR) & ARR_FAILED) {
		sc->sc_ethercom.ec_if.if_flags |= IFF_OACTIVE;
		sc->sc_intctl |= MSK_ALLOC;
		goto esstart_out;
	}
	active_pnr = inb(iobase + B2ARR);
	outb(iobase + B2PNR, active_pnr);

	outl(iobase + B2PTR, PTR_AUTOINCR);
	NWAIT(3);
	outl(iobase + B2DATA, pktctlw);
	outl(iobase + B2DATA, pktlen);

	buf = pktbuf;
	cnt = (pktlen - 4) / 2;
	while (cnt--) {
		outl(iobase + B2DATA, (u_short) *buf);
		buf++;
	}

	outl(iobase + B2MMUCR, MMUCR_ENQ_TX);
	if (inb(iobase + B2PNR) != active_pnr)
		printf("%s: esstart - PNR changed %x->%x\n",
		    sc->sc_dev.dv_xname, active_pnr, inb(iobase + B2PNR));

	sc->sc_ethercom.ec_if.if_opackets++;	/* move to interrupt? */
	sc->sc_intctl |= MSK_TX_EMPTY | MSK_TX;

esstart_out:
	outb(iobase + B2MSK, sc->sc_intctl & 0xff);

#ifdef ESDEBUG
	while ((smc->b2.bsr & BSR_BANKMSK) != BSR_BANK2) {
		printf("%s: esstart++++ BSR not 2: %04x\n", sc->sc_dev.dv_xname,
		    smc->b2.bsr);
		smc->b2.bsr = BSR_BANK2;
	}
	if (--sc->sc_smcbusy) {
		printf("%s: esstart busy on exit\n", sc->sc_dev.dv_xname);
		panic("esstart busy on exit");
	}
#endif
}

static int
esioctl(ifp, command, data)
	register struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct es_softc *sc = ifp->if_softc;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splimp();

	switch (command) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			esinit(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host,
				    LLADDR(ifp->if_sadl), ETHER_ADDR_LEN));
			/* Set new address. */
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
			esstop(sc);
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
			esstop(sc);
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
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			/* XXX */
			error = 0;
		}
		break;

	default:
		error = EINVAL;
	}

	splx(s);
	return (error);
}

static void
esreset(sc)
	struct es_softc *sc;
{
	int s;

	s = splimp();
	esstop(sc);
	esinit(sc);
	splx(s);
}

static void
eswatchdog(ifp)
	struct ifnet *ifp;
{
	struct es_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->.if_oerrors;

	esreset(sc);
}
