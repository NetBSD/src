/*	$NetBSD: if_le.c,v 1.3 1995/03/24 14:57:12 cgd Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell and Rick Macklem.
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
 *	@(#)if_le.c	8.2 (Berkeley) 11/16/93
 */

#include "le.h"
#if NLE > 0

#include <bpfilter.h>

/*
 * AMD 7990 LANCE
 */
#include <sys/param.h>
#include <sys/device.h>
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
#include <sys/select.h>
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

#ifdef APPLETALK
#include <netddp/atalk.h>
#endif

#if defined (CCITT) && defined (LLC)
#include <sys/socketvar.h>
#include <netccitt/x25.h>
extern llc_ctlinput(), cons_rtrequest();
#endif

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <alpha/tc/tc.h>
#include <alpha/tc/asic.h>
#include <alpha/tc/if_lereg.h>

int	ledebug = 1;		/* console error messages */

#ifdef PACKETSTATS
long	lexpacketsizes[LEMTU+1];
long	lerpacketsizes[LEMTU+1];
#endif

/* Per interface statistics */
/* XXX this should go in something like if_levar.h */
struct	lestats {
	long	lexints;	/* transmitter interrupts */
	long	lerints;	/* receiver interrupts */
	long	lerbufs;	/* total buffers received during interrupts */
	long	lerhits;	/* times current rbuf was full */
	long	lerscans;	/* rbufs scanned before finding first full */
};

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * le_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	le_softc {
	struct	device sc_dev;		/* base structure */
	struct	evcnt sc_intrcnt;	/* # of interrupts, per le */
	struct	evcnt sc_errcnt;	/* # of errors, per le */

	struct	arpcom sc_ac;		/* common Ethernet structures */
#define	sc_if	sc_ac.ac_if		/* network-visible interface */
#define	sc_addr	sc_ac.ac_enaddr		/* hardware Ethernet address */
	struct	lereg1 *sc_r1;		/* LANCE registers */
	void	*sc_r2;			/* dual-port RAM */
	int	sc_ler2pad;		/* Do ring descriptors require pads? */
	void	(*sc_copytobuf)();	/* Copy to buffer */
	void	(*sc_copyfrombuf)();	/* Copy from buffer */
	void	(*sc_zerobuf)();	/* and Zero bytes in buffer */
	u_char	*sc_etheraddr;		/* location of ether address PROM */
	int	sc_rmd;			/* predicted next rmd to process */
	int	sc_tmd;			/* last tmd processed */
	int	sc_tmdnext;		/* next tmd to transmit with */
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
	struct	lestats sc_lestats;	/* per interface statistics */
};

/* access LANCE registers */
void lewritereg();
#define	LERDWR(cntl, src, dst)	{ (dst) = (src); MB(); }
#define	LEWREG(src, dst)	lewritereg(&(dst), (src))

#define CPU_TO_CHIP_ADDR(cpu) \
	((unsigned long)(&(((struct lereg2 *)0)->cpu)))

#define LE_OFFSET_RAM		0x0
#define LE_OFFSET_LANCE		0x100000
#define LE_OFFSET_ROM		0x1c0000

void copytobuf_contig(), copyfrombuf_contig(), bzerobuf_contig();
void copytobuf_gap16(), copyfrombuf_gap16(), bzerobuf_gap16();

extern caddr_t	le_iomem;

/* autoconfiguration driver */
int	lematch(struct device *, void *, void *);
void	leattach(struct device *, struct device *, void *);
struct cfdriver lecd =
    { NULL, "le", lematch, leattach, DV_IFNET, sizeof (struct le_softc) };

/* Forwards */
int	le_setup __P((int, void *, struct le_softc *));		/* XXX */
void	ledrinit(struct le_softc *);
void	leerror(struct le_softc *, int);
struct mbuf *leget(struct le_softc *, char *, int, int, struct ifnet *);
int     leinit(int);
int	leintr(void *);
int	leioctl(struct ifnet *, u_long, caddr_t);
int	leput(struct le_softc *, char *, struct mbuf *);
void	lererror(struct le_softc *, char *);
void	leread(struct le_softc *, char *, int);
void	lereset(struct device *);
void	lerint(struct le_softc *);
int	lestart(struct ifnet *);
void	lexerror(struct le_softc *);
void	lexint(struct le_softc *);

/* FROM HERE ON DOWN: XXX */

int
lematch(parent, cfdata, aux)
	struct device *parent;
	void *cfdata;
	void *aux;
{
	struct cfdata *cf = cfdata;
	struct confargs *ca = aux;
#ifdef notdef /* XXX */
	struct tc_cfloc *tc_locp;
	struct asic_cfloc *asic_locp;
#endif

#ifdef notdef /* XXX */
	tclocp = (struct tc_cfloc *)cf->cf_loc;
#endif

	/* XXX CHECK BUS */
	/* make sure that we're looking for this type of device. */
#ifdef notdef
	if (!BUS_MATCHNAME(ca, "PMAD-BA "))
#endif
	if (!BUS_MATCHNAME(ca, "lance"))
		return (0);

#ifdef notdef /* XXX */
	/* make sure the unit matches the cfdata */
	if ((cf->cf_unit != tap->ta_unit &&
	     tap->ta_unit != TA_ANYUNIT) ||
	    (tclocp->cf_slot != tap->ta_slot &&
	     tclocp->cf_slot != TC_SLOT_WILD) ||
	    (tclocp->cf_offset != tap->ta_offset &&
	     tclocp->cf_offset != TC_OFFSET_WILD))
		return (0);
#endif

	return (1);
}

void
leattach(parent, self, aux)
	struct device *parent;
	struct device *self;
	void *aux;
{
	register struct le_softc *sc = (struct le_softc *)self;
	struct confargs *ca = aux;
	struct lereg1 *ler1;
	struct ifnet *ifp = &sc->sc_if;
	u_char *cp;
	int i;

	le_setup(self->dv_unit, aux, sc);
	ler1 = sc->sc_r1;

	/*
	 * Get the ethernet address out of rom
	 */
	for (i = 0, cp = sc->sc_etheraddr; i < sizeof(sc->sc_addr); i++) {
		sc->sc_addr[i] = *cp;
		cp += 4;
	}
	printf(": address %s\n", ether_sprintf(sc->sc_addr));

	/* make sure the chip is stopped */
	LEWREG(LE_CSR0, ler1->ler1_rap);
	LEWREG(LE_C0_STOP, ler1->ler1_rdp);

	/*
	 * Set up event counters.
	 */
	evcnt_attach(&sc->sc_dev, "intr", &sc->sc_intrcnt);
	evcnt_attach(&sc->sc_dev, "errs", &sc->sc_errcnt);

	BUS_INTR_ESTABLISH(ca, leintr, sc);

	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = "le";
	ifp->if_ioctl = leioctl;
	ifp->if_output = ether_output;
	ifp->if_start = lestart;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
        ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
#ifdef IFF_NOTRAILERS
	/* XXX still compile when the blasted things are gone... */
	ifp->if_flags |= IFF_NOTRAILERS;
#endif
#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
	if_attach(ifp);
	ether_ifattach(ifp);
}

int
le_setup(unit, aux, sc)
	int unit;
	void *aux;
	struct le_softc *sc;
{
	struct confargs *ca = aux;

	if (unit == 0 && (hwrpb->rpb_type == ST_DEC_3000_300 ||
	    hwrpb->rpb_type == ST_DEC_3000_500)) {
		/* It's on the system ASIC */
		volatile u_int *ldp;

		sc->sc_r1 = (struct lereg1 *)BUS_CVTADDR(ca);
#ifdef SPARSE
		sc->sc_r1 = TC_DENSE_TO_SPARSE(sc->sc_r1);
#endif
		sc->sc_r2 = (void *)le_iomem;
		sc->sc_ler2pad = 1;
/* XXX */	sc->sc_etheraddr = (u_char *)ASIC_SYS_ETHER_ADDRESS(asic_base);
		sc->sc_copytobuf = copytobuf_gap16;
		sc->sc_copyfrombuf = copyfrombuf_gap16;
		sc->sc_zerobuf = bzerobuf_gap16;

		/*
		 * And enable Lance dma through the asic.
		 */
		ldp = (volatile u_int *)ASIC_REG_LANCE_DMAPTR(asic_base);
		*ldp = (((u_int64_t)le_iomem << 3) & ~(u_int64_t)0x1f) |
			(((u_int64_t)le_iomem >> 29) & 0x1f);
		*(volatile u_int *)ASIC_REG_CSR(asic_base) |=
		    ASIC_CSR_DMAEN_LANCE;
		MB();
	} else {
		/* It's on the turbochannel proper */
		sc->sc_r1 = (struct lereg1 *)
		    (BUS_CVTADDR(ca) + LE_OFFSET_LANCE);
		sc->sc_r2 = (void *)
		    (BUS_CVTADDR(ca) + LE_OFFSET_RAM);
		sc->sc_ler2pad = 0;
		sc->sc_etheraddr = (u_char *)
		    (BUS_CVTADDR(ca) + LE_OFFSET_ROM + 2);
		sc->sc_copytobuf = copytobuf_contig;
		sc->sc_copyfrombuf = copyfrombuf_contig;
		sc->sc_zerobuf = bzerobuf_contig;
	}

	return (0);
}

/*
 * Setup the logical address filter
 */
void
lesetladrf(sc)
	register struct le_softc *sc;
{
	register volatile struct lereg2 *ler2 = sc->sc_r2;
	register struct ifnet *ifp = &sc->sc_if;
	register struct ether_multi *enm;
	register u_char *cp, c;
	register u_long crc;
	register int i, len;
	struct ether_multistep step;

	/*
	 * Set up multicast address filter by passing all multicast
	 * addresses through a crc generator, and then using the high
	 * order 6 bits as an index into the 64 bit logical address
	 * filter.  The high order bit selects the word, while the
	 * rest of the bits select the bit within the word.
	 */

	LER2_ladrf0(ler2, 0);
	LER2_ladrf1(ler2, 0);
	LER2_ladrf2(ler2, 0);
	LER2_ladrf3(ler2, 0);
	ifp->if_flags &= ~IFF_ALLMULTI;
	ETHER_FIRST_MULTI(step, &sc->sc_ac, enm);
	while (enm != NULL) {
		if (bcmp((caddr_t)enm->enm_addrlo,
		    (caddr_t)enm->enm_addrhi, sizeof(enm->enm_addrlo)) == 0) {
			/*
			 * We must listen to a range of multicast
			 * addresses.  For now, just accept all
			 * multicasts, rather than trying to set only
			 * those filter bits needed to match the range.
			 * (At this time, the only use of address
			 * ranges is for IP multicast routing, for
			 * which the range is big enough to require all
			 * bits set.)
			 */
			LER2_ladrf0(ler2, 0xff);
			LER2_ladrf1(ler2, 0xff);
			LER2_ladrf2(ler2, 0xff);
			LER2_ladrf3(ler2, 0xff);
			ifp->if_flags |= IFF_ALLMULTI;
			return;
		}

		/*
		 * One would think, given the AM7990 document's polynomial
		 * of 0x04c11db6, that this should be 0x6db88320 (the bit
		 * reversal of the AMD value), but that is not right.  See
		 * the BASIC listing: bit 0 (our bit 31) must then be set.
		 */
		cp = (unsigned char *)enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = 6; --len >= 0;) {
			c = *cp++;
			for (i = 0; i < 8; i++) {
				if ((crc & 0x01) ^ (c & 0x01)) {
					crc >>= 1;
					crc ^= 0xedb88320;
				} else
					crc >>= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc = crc >> 26;

		/* Turn on the corresponding bit in the filter. */
		switch (crc >> 5) {			/* XXX VS SPARC */
		case 0:
			LER2_ladrf0(ler2,
			    LER2V_ladrf0(ler2) | 1 << (crc & 0x1f));
			break;
		case 1:
			LER2_ladrf1(ler2,
			    LER2V_ladrf1(ler2) | 1 << (crc & 0x1f));
			break;
		case 2:
			LER2_ladrf2(ler2,
			    LER2V_ladrf2(ler2) | 1 << (crc & 0x1f));
			break;
		case 3:
			LER2_ladrf3(ler2,
			    LER2V_ladrf3(ler2) | 1 << (crc & 0x1f));
			break;
		}

		ETHER_NEXT_MULTI(step, enm);
	}
}

void
ledrinit(sc)
	struct le_softc *sc;
{
	register void *rp;
	register int i;

	for (i = 0; i < LERBUF; i++) {
		rp = LER2_RMDADDR(sc->sc_r2, i);
		LER2_rmd0(rp, CPU_TO_CHIP_ADDR(ler2_rbuf[i][0]));	/*XXX*/
		LER2_rmd1_bits(rp, LE_T1_OWN);
		LER2_rmd1_hadr(rp, 0);					/*XXX*/
		LER2_rmd2(rp, -LEMTU | LE_XMD2_ONES);
		LER2_rmd3(rp, 0);
	}
	for (i = 0; i < LETBUF; i++) {
		rp = LER2_TMDADDR(sc->sc_r2, i);
		LER2_tmd0(rp, CPU_TO_CHIP_ADDR(ler2_tbuf[i][0]));	/*XXX*/
		LER2_tmd1_bits(rp, 0);
		LER2_tmd1_hadr(rp, 0);					/*XXX*/
		LER2_tmd2(rp, LE_XMD2_ONES);
		LER2_tmd3(rp, 0);
	}
}

void
lereset(dev)
	struct device *dev;
{
	register struct le_softc *sc = (struct le_softc *)dev;
	register volatile struct lereg1 *ler1 = sc->sc_r1;
	register volatile void *ler2 = sc->sc_r2;
	register int timo, stat;

	/* XXX YEECH!!! */
	*(volatile u_int *)ASIC_REG_IMSK(asic_base) |= ASIC_INTR_LANCE;
	MB();

#if NBPFILTER > 0
	if (sc->sc_if.if_flags & IFF_PROMISC)
		LER2_mode(ler2, LE_MODE_NORMAL | LE_MODE_PROM);
	else
#endif
		LER2_mode(ler2, LE_MODE_NORMAL);
	LEWREG(LE_CSR0, ler1->ler1_rap);
	LEWREG(LE_C0_STOP, ler1->ler1_rdp);

	/* Setup the logical address filter */
	lesetladrf(sc);

	/* init receive and transmit rings */
	ledrinit(sc);
	sc->sc_rmd = 0;				/* XXX ??? */
	sc->sc_tmd = LETBUF - 1;		/* XXX ??? */
	sc->sc_tmdnext = 0;			/* XXX ??? */

	/*
	 * Setup for transmit/receive
	 * XXX ???
	 */
	LER2_padr0(ler2, (sc->sc_addr[1] << 8) | sc->sc_addr[0]);
	LER2_padr1(ler2, (sc->sc_addr[3] << 8) | sc->sc_addr[2]);
	LER2_padr2(ler2, (sc->sc_addr[5] << 8) | sc->sc_addr[4]);
	LER2_rlen(ler2, LE_RLEN);
	LER2_rdra(ler2, CPU_TO_CHIP_ADDR(ler2_rmd[0]));
	LER2_tlen(ler2, LE_TLEN);
	LER2_tdra(ler2, CPU_TO_CHIP_ADDR(ler2_tmd[0]));

	/* tell the chip where to find the initialization block */
	LEWREG(LE_CSR1, ler1->ler1_rap);
	LEWREG(CPU_TO_CHIP_ADDR(ler2_mode), ler1->ler1_rdp);
	LEWREG(LE_CSR2, ler1->ler1_rap);
	LEWREG(0, ler1->ler1_rdp);
	LEWREG(LE_CSR3, ler1->ler1_rap);
	LEWREG(0, ler1->ler1_rdp);
	LEWREG(LE_CSR0, ler1->ler1_rap);
	LERDWR(ler0, LE_C0_INIT, ler1->ler1_rdp);
	timo = 100000;
	while (((stat = ler1->ler1_rdp) & (LE_C0_ERR | LE_C0_IDON)) == 0) {
		if (--timo == 0) {
			printf("%s: init timeout, stat=%b\n",
			    sc->sc_dev.dv_xname, stat, LE_C0_BITS);
			break;
		}
	}
	if (stat & LE_C0_ERR)
		printf("%s: init failed, stat=%b\n",
		    sc->sc_dev.dv_xname, stat, LE_C0_BITS);
	else
		LERDWR(ler0, LE_C0_IDON, ler1->ler1_rdp);	/* clear IDON */
	LERDWR(ler0, LE_C0_STRT | LE_C0_INEA, ler1->ler1_rdp);
	sc->sc_if.if_flags &= ~IFF_OACTIVE;
}

/*
 * Initialization of interface
 */
int
leinit(unit)
	int unit;
{
        register struct le_softc *sc = lecd.cd_devs[unit];
	register struct ifnet *ifp = &sc->sc_if;
	register int s;

	/* not yet, if address still unknown */
	if (ifp->if_addrlist == (struct ifaddr *)0)
		return (0);
	if ((ifp->if_flags & IFF_RUNNING) == 0) {
		s = splimp();
		ifp->if_flags |= IFF_RUNNING;
		lereset(&sc->sc_dev);
		lestart(ifp);
		splx(s);
	}
	return (0);
}

#define	LENEXTTMP \
	if (++bix == LETBUF) \
		bix = 0; \
	tmd = LER2_TMDADDR(sc->sc_r2, bix)

/*
 * Start output on interface.  Get another datagram to send
 * off of the interface queue, and copy it to the interface
 * before starting the output.
 */
int
lestart(ifp)
	struct ifnet *ifp;
{
	register struct le_softc *sc = lecd.cd_devs[ifp->if_unit];
	register volatile void *tmd;
	register struct mbuf *m;
	register int bix, len;

	if ((sc->sc_if.if_flags & IFF_RUNNING) == 0)
		return (0);

	bix = sc->sc_tmdnext;
	len = 0;
	tmd = LER2_TMDADDR(sc->sc_r2, bix);
	while (bix != sc->sc_tmd) {
		if (LER2V_tmd1_bits(tmd) & LE_T1_OWN)
			panic("lestart");
		IF_DEQUEUE(&sc->sc_if.if_snd, m);
		if (m == 0)
			break;

#if NBPFILTER > 0
		/*
		 * If bpf is listening on this interface, let it
		 * see the packet before we commit it to the wire.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		len = leput(sc, LER2_TBUFADDR(sc->sc_r2, bix), m);
#ifdef PACKETSTATS
		if (len <= LEMTU)
			lexpacketsizes[len]++;
#endif
		LER2_tmd3(tmd, 0);
		LER2_tmd2(tmd, -len | LE_XMD2_ONES);
		LER2_tmd1_bits(tmd, LE_T1_OWN | LE_T1_STP | LE_T1_ENP);
		LENEXTTMP;
	}
	if (len != 0) {
		sc->sc_if.if_flags |= IFF_OACTIVE;
		LERDWR(ler0, LE_C0_TDMD | LE_C0_INEA, sc->sc_r1->ler1_rdp);
	}
	sc->sc_tmdnext = bix;
	return (0);
}

int
leintr(dev)
	register void *dev;
{
	register struct le_softc *sc = dev;
	register volatile struct lereg1 *ler1 = sc->sc_r1;
	register int csr0;

	csr0 = ler1->ler1_rdp;
	if ((csr0 & LE_C0_INTR) == 0)
		return (0);
	sc->sc_intrcnt.ev_count++;

	if (csr0 & LE_C0_ERR) {
		sc->sc_errcnt.ev_count++;
		leerror(sc, csr0);
		if (csr0 & LE_C0_MERR) {
			sc->sc_merr++;
			lereset(&sc->sc_dev);
			return (1);
		}
		if (csr0 & LE_C0_BABL)
			sc->sc_babl++;
		if (csr0 & LE_C0_CERR)
			sc->sc_cerr++;
		if (csr0 & LE_C0_MISS)
			sc->sc_miss++;
		LERDWR(ler0, LE_C0_BABL|LE_C0_CERR|LE_C0_MISS|LE_C0_INEA,
		    ler1->ler1_rdp);
	}
	if ((csr0 & LE_C0_RXON) == 0) {
		sc->sc_rxoff++;
		lereset(&sc->sc_dev);
		return (1);
	}
	if ((csr0 & LE_C0_TXON) == 0) {
		sc->sc_txoff++;
		lereset(&sc->sc_dev);
		return (1);
	}
	if (csr0 & LE_C0_RINT) {
		/* interrupt is cleared in lerint */
		lerint(sc);
	}
	if (csr0 & LE_C0_TINT) {
		LERDWR(ler0, LE_C0_TINT|LE_C0_INEA, ler1->ler1_rdp);
		lexint(sc);
	}
	return (1);
}

/*
 * Ethernet interface transmitter interrupt.
 * Start another output if more data to send.
 */
void
lexint(sc)
	register struct le_softc *sc;
{
	register int bix = sc->sc_tmd;
	register volatile void *tmd;

	sc->sc_lestats.lexints++;
	if ((sc->sc_if.if_flags & IFF_OACTIVE) == 0) {
		sc->sc_xint++;
		return;
	}
	LENEXTTMP;
	while (bix != sc->sc_tmdnext &&
	    (LER2V_tmd1_bits(tmd) & LE_T1_OWN) == 0) {
		sc->sc_tmd = bix;
		if (LER2V_tmd1_bits(tmd) & LE_T1_ERR) {
err:
			lexerror(sc);
			sc->sc_if.if_oerrors++;
			if (LER2V_tmd3(tmd) & (LE_T3_BUFF|LE_T3_UFLO)) {
				sc->sc_uflo++;
				lereset(&sc->sc_dev);
				break;
			}
			else if (LER2V_tmd3(tmd) & LE_T3_LCOL)
				sc->sc_if.if_collisions++;
			else if (LER2V_tmd3(tmd) & LE_T3_RTRY)
				sc->sc_if.if_collisions += 16;
		}
		else if (LER2V_tmd1_bits(tmd) & LE_T3_BUFF)
			/* XXX documentation says BUFF not included in ERR */
			goto err;
		else if (LER2V_tmd1_bits(tmd) & LE_T1_ONE)
			sc->sc_if.if_collisions++;
		else if (LER2V_tmd1_bits(tmd) & LE_T1_MORE)
			/* what is the real number? */
			sc->sc_if.if_collisions += 2;
		else
			sc->sc_if.if_opackets++;
		LENEXTTMP;
	}
	if (bix == sc->sc_tmdnext)
		sc->sc_if.if_flags &= ~IFF_OACTIVE;
	lestart(&sc->sc_if);
}

#define	LENEXTRMP							\
	if (++bix == LERBUF)						\
		bix = 0;						\
	rmd = LER2_RMDADDR(sc->sc_r2, bix)

/*
 * Ethernet interface receiver interrupt.
 * If input error just drop packet.
 * Decapsulate packet based on type and pass to type specific
 * higher-level input routine.
 */
void
lerint(sc)
	struct le_softc *sc;
{
	register int bix = sc->sc_rmd;
	register volatile void *rmd = LER2_RMDADDR(sc->sc_r2, bix);

	sc->sc_lestats.lerints++;
	/*
	 * Out of sync with hardware, should never happen?
	 */
	if (LER2V_rmd1_bits(rmd) & LE_R1_OWN) {
		do {
			sc->sc_lestats.lerscans++;
			LENEXTRMP;
		} while ((LER2V_rmd1_bits(rmd) & LE_R1_OWN) && bix != sc->sc_rmd);
		if (bix == sc->sc_rmd)
			printf("%s: RINT with no buffer\n",
			    sc->sc_dev.dv_xname);
	} else
		sc->sc_lestats.lerhits++;

	/*
	 * Process all buffers with valid data
	 */
	while ((LER2V_rmd1_bits(rmd) & LE_R1_OWN) == 0) {
		int len = LER2V_rmd3(rmd);

		/* Clear interrupt to avoid race condition */
		LERDWR(sc->sc_r0, LE_C0_RINT|LE_C0_INEA, sc->sc_r1->ler1_rdp);

		if (LER2V_rmd1_bits(rmd) & LE_R1_ERR) {
			sc->sc_rmd = bix;
			lererror(sc, "bad packet");
			sc->sc_if.if_ierrors++;
		} else if ((LER2V_rmd1_bits(rmd) & (LE_R1_STP|LE_R1_ENP)) !=
		    (LE_R1_STP|LE_R1_ENP)) {
			/* XXX make a define for LE_R1_STP|LE_R1_ENP? */
			/*
			 * Find the end of the packet so we can see how long
			 * it was.  We still throw it away.
			 */
			do {
				LERDWR(sc->sc_r0, LE_C0_RINT|LE_C0_INEA,
				    sc->sc_r1->ler1_rdp);
				LER2_rmd3(rmd, 0);
				LER2_rmd1_bits(rmd, LE_R1_OWN);
				LENEXTRMP;
			} while (!(LER2V_rmd1_bits(rmd) &
			    (LE_R1_OWN|LE_R1_ERR|LE_R1_STP|LE_R1_ENP)));
			sc->sc_rmd = bix;
			lererror(sc, "chained buffer");
			sc->sc_rxlen++;
			/*
			 * If search terminated without successful completion
			 * we reset the hardware (conservative).
			 */
			if ((LER2V_rmd1_bits(rmd) &
			    (LE_R1_OWN|LE_R1_ERR|LE_R1_STP|LE_R1_ENP)) !=
			    LE_R1_ENP) {
				lereset(&sc->sc_dev);
				return;
			}
		} else {
			leread(sc, LER2_RBUFADDR(sc->sc_r2, bix), len);
#ifdef PACKETSTATS
			lerpacketsizes[len]++;
#endif
			sc->sc_lestats.lerbufs++;
		}
		LER2_rmd3(rmd, 0);
		LER2_rmd1_bits(rmd, LE_R1_OWN);
		LENEXTRMP;
	}
	sc->sc_rmd = bix;
}

/*
 * Look at the packet in network buffer memory so we can be smart about how
 * we copy the data into mbufs.
 * This needs work since we can't just read network buffer memory like
 * regular memory.
 */
void
leread(sc, buf, len)
	register struct le_softc *sc;
	char *buf;
	int len;
{
	struct ifnet *ifp;
	struct mbuf *m;
	struct ether_header eh;

	/* get the header */
	(*sc->sc_copyfrombuf)(buf, 0, (char *)&eh, sizeof (eh));

	/* adjust input length to account for header and CRC */
	len -= sizeof(struct ether_header) + 4;
	if (len <= 0)
		return;

	/* Pull packet off interface. */
	ifp = &sc->sc_ac.ac_if;
	m = leget(sc, buf, len, 0, ifp);
	if (m == 0)
		return;

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to BPF.
	 */
	if (ifp->if_bpf) {
		M_PREPEND(m, sizeof(struct ether_header), M_DONTWAIT);
		if (m == 0)
			return;
		bcopy(&eh, mtod(m, void *), sizeof(struct ether_header));

		bpf_mtap(ifp->if_bpf, m);

		m_adj(m, sizeof(struct ether_header));

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (eh.ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh.ether_dhost, sc->sc_ac.ac_enaddr,
			    sizeof(eh.ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif

	ether_input(ifp, &eh, m);
}

/*
 * Routine to copy from mbuf chain to transmit buffer in
 * network buffer memory.
 */
int
leput(sc, lebuf, m)
	struct le_softc *sc;
	char *lebuf;
	register struct mbuf *m;
{
	register struct mbuf *mp;
	register int len, tlen = 0;
	register int boff = 0;

	for (mp = m; mp; mp = mp->m_next) {
		len = mp->m_len;
		if (len == 0)
			continue;
		tlen += len;
		(*sc->sc_copytobuf)(mtod(mp, char *), lebuf, boff, len);
		boff += len;
	}
	m_freem(m);
	if (tlen < LEMINSIZE) {
		(*sc->sc_zerobuf)(lebuf, boff, LEMINSIZE - tlen);
		tlen = LEMINSIZE;
	}
	return(tlen);
}

/*
 * Routine to copy from network buffer memory into mbufs.
 */
struct mbuf *
leget(sc, lebuf, totlen, off, ifp)
	struct le_softc *sc;
	char *lebuf;
	int totlen, off;
	struct ifnet *ifp;
{
	register struct mbuf *m;
	struct mbuf *top = 0, **mp = &top;
	register int len, resid, boff;

	boff = sizeof(struct ether_header);
	resid = totlen;

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

		if (resid >= MINCLSIZE)
			MCLGET(m, M_DONTWAIT);
		if (m->m_flags & M_EXT)
			m->m_len = min(resid, MCLBYTES);
		else if (resid < m->m_len) {
			/*
			 * Place initial small packet/header at end of mbuf.
			 */
			if (top == 0 && resid + max_linkhdr <= m->m_len)
				m->m_data += max_linkhdr;
			m->m_len = resid;
		}
		len = m->m_len;
		(*sc->sc_copyfrombuf)(lebuf, boff, mtod(m, char *), len);
		boff += len;
		*mp = m;
		mp = &m->m_next;
		totlen -= len;
		resid -= len;
		if (resid == 0) {
			boff = sizeof (struct ether_header);
			resid = totlen;
		}
	}
	return (top);
}

/*
 * Process an ioctl request.
 */
int
leioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	register struct ifaddr *ifa;
	struct le_softc *sc = lecd.cd_devs[ifp->if_unit];
	register struct lereg1 *ler1;
	int s = splimp(), error = 0;

	switch (cmd) {

	case SIOCSIFADDR:
		ifa = (struct ifaddr *)data;
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			(void)leinit(ifp->if_unit);	/* before arpwhohas */
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
				LEWREG(LE_C0_STOP, ler1->ler1_rdp);
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->sc_addr, sizeof(sc->sc_addr));
			}
			(void)leinit(ifp->if_unit); /* does le_setaddr() */
			break;
		    }
#endif
		default:
			leinit(ifp->if_unit);
			break;
		}
		break;

#if defined (CCITT) && defined (LLC)
	case SIOCSIFCONF_X25:
		ifp->if_flags |= IFF_UP;
		ifa->ifa_rtrequest = (void (*)())cons_rtrequest; /* XXX */
		error = x25_llcglue(PRC_IFUP, ifa->ifa_addr);
		if (error == 0)
			leinit(ifp->if_unit);
		break;
#endif /* CCITT && LLC */

	case SIOCSIFFLAGS:
		ler1 = sc->sc_r1;
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    ifp->if_flags & IFF_RUNNING) {
			LEWREG(LE_C0_STOP, ler1->ler1_rdp);
			ifp->if_flags &= ~IFF_RUNNING;
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
			lereset(&sc->sc_dev);
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
			lereset(&sc->sc_dev);
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
	if ((stat & LE_C0_CERR) && sc->sc_cerr)
		return;
	log(LOG_WARNING, "%s: error: stat=%b\n",
	    sc->sc_dev.dv_xname, stat, LE_C0_BITS);
}

void
lererror(sc, msg)
	register struct le_softc *sc;
	char *msg;
{
	register void *rmd;
	u_char eaddr[6];
	int len;

	if (!ledebug)
		return;

	rmd = LER2_RMDADDR(sc->sc_r2, sc->sc_rmd);
	len = LER2V_rmd3(rmd);
	if (len > 11)
		(*sc->sc_copyfrombuf)(LER2_RBUFADDR(sc->sc_r2, sc->sc_rmd),
			6, eaddr, 6);
	log(LOG_WARNING,
	    "%s: ierror(%s): from %s: buf=%d, len=%d, rmd1_bits=%b\n",
	    sc->sc_dev.dv_xname,
	    msg, len > 11 ? ether_sprintf(eaddr) : "unknown",
	    sc->sc_rmd, len, LER2V_rmd1_bits(rmd), LE_R1_BITS);
}

void
lexerror(sc)
	register struct le_softc *sc;
{
	register void *tmd;
	u_char eaddr[6];
	int len;

	if (!ledebug)
		return;

	tmd = LER2_TMDADDR(sc->sc_r2, 0);
	len = -LER2V_tmd2(tmd);
	if (len > 5)
		(*sc->sc_copyfrombuf)(LER2_TBUFADDR(sc->sc_r2, 0), 0, eaddr, 6);
	log(LOG_WARNING,
	    "%s: oerror: to %s: buf=%d, len=%d, tmd1_bits=%b, tmd3=%b\n",
	    sc->sc_dev.dv_xname,
	    len > 5 ? ether_sprintf(eaddr) : "unknown", 0, len,
	    LER2V_tmd1_bits(tmd), LE_T1_BITS,
	    LER2V_tmd3(tmd), LE_T3_BITS);
}

/*
 * Write a lance register port, reading it back to ensure success. This seems
 * to be necessary during initialization, since the chip appears to be a bit
 * pokey sometimes.
 */
void
lewritereg(regptr, val)
	register volatile u_short *regptr;
	register u_short val;
{
	register int i = 0;

	while (*regptr != val) {
		*regptr = val;
		MB();
		if (++i > 10000) {
			printf("le: Reg did not settle (to x%x): x%x\n", val,
			    *regptr);
			return;
		}
		DELAY(100);
	}
}

/*
 * Routines for accessing the transmit and receive buffers. Unfortunately,
 * CPU addressing of these buffers is done in one of 3 ways:
 * - contiguous (for the 3max and turbochannel option card)
 * - gap2, which means shorts (2 bytes) interspersed with short (2 byte)
 *   spaces (for the pmax)
 * - gap16, which means 16bytes interspersed with 16byte spaces
 *   for buffers which must begin on a 32byte boundary (for 3min and maxine)
 * The buffer offset is the logical byte offset, assuming contiguous storage.
 */
void
copytobuf_contig(from, lebuf, boff, len)
	char *from;
	void *lebuf;
	int boff;
	int len;
{

	/*
	 * Just call bcopy() to do the work.
	 */
	bcopy(from, ((char *)lebuf) + boff, len);
}

void
copyfrombuf_contig(lebuf, boff, to, len)
	void *lebuf;
	int boff;
	char *to;
	int len;
{

	/*
	 * Just call bcopy() to do the work.
	 */
	bcopy(((char *)lebuf) + boff, to, len);
}

void
bzerobuf_contig(lebuf, boff, len)
	void *lebuf;
	int boff;
	int len;
{

	/*
	 * Just let bzero() do the work
	 */
	bzero(((char *)lebuf) + boff, len);
}

/*
 * For the 3min and maxine, the buffers are in main memory filled in with
 * 16byte blocks interspersed with 16byte spaces.
 */
void
copytobuf_gap16(from, lebuf, boff, len)
	register char *from;
	void *lebuf;
	int boff;
	register int len;
{
	register char *bptr;
	register int xfer;

	bptr = ((char *)lebuf) + ((boff << 1) & ~0x1f);
	boff &= 0xf;
	xfer = min(len, 16 - boff);
	while (len > 0) {
		bcopy(from, ((char *)bptr) + boff, xfer);
		from += xfer;
		bptr += 32;
		boff = 0;
		len -= xfer;
		xfer = min(len, 16);
	}
}

void
copyfrombuf_gap16(lebuf, boff, to, len)
	void *lebuf;
	int boff;
	register char *to;
	register int len;
{
	register char *bptr;
	register int xfer;

	bptr = ((char *)lebuf) + ((boff << 1) & ~0x1f);
	boff &= 0xf;
	xfer = min(len, 16 - boff);
	while (len > 0) {
		bcopy(((char *)bptr) + boff, to, xfer);
		to += xfer;
		bptr += 32;
		boff = 0;
		len -= xfer;
		xfer = min(len, 16);
	}
}

void
bzerobuf_gap16(lebuf, boff, len)
	void *lebuf;
	int boff;
	register int len;
{
	register char *bptr;
	register int xfer;

	bptr = ((char *)lebuf) + ((boff << 1) & ~0x1f);
	boff &= 0xf;
	xfer = min(len, 16 - boff);
	while (len > 0) {
		bzero(((char *)bptr) + boff, xfer);
		bptr += 32;
		boff = 0;
		len -= xfer;
		xfer = min(len, 16);
	}
}
#endif /* NLE */
