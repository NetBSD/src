/*	$NetBSD: if_ln.c,v 1.9 1999/02/02 18:37:21 ragge Exp $	*/

/*-
 * Copyright (c) 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "opt_inet.h"
#include "opt_ccitt.h"
#include "opt_llc.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h> 
#include <sys/syslog.h>
#include <sys/socket.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/reboot.h>
#if NRND > 0
#include <sys/rnd.h>
#endif

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#if defined(CCITT) && defined(LLC)
#include <sys/socketvar.h>
#include <netccitt/x25.h>
#include <netccitt/pk.h>
#include <netccitt/pk_var.h>
#include <netccitt/pk_extern.h>
#endif

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <dev/ic/lancereg.h>
#include <dev/ic/am7990reg.h>

#include <machine/cpu.h>
#include <machine/rpb.h>
#include <machine/vsbus.h>

#include "ioconf.h"

#define RLEN	5
#define NRBUF	(1 << RLEN)
#define TLEN	3
#define NTBUF	(1 << TLEN)
#define BUFSIZE 1536

/*
 * Init block & buffer descriptors according to DEC system
 * specification documentation.
 */
struct initblock {
	short	ib_mode;
	char	ib_padr[6]; /* Ethernet address */
	short	ib_ladrf[4];
	int	ib_rdr; /* Receive address */
	int	ib_tdr; /* Transmit address */
};

struct buffdesc {
	int	bd_adrflg;
	short	bd_bcnt;
	short	bd_mcnt;
};

/* Flags in the address field */
#define BR_OWN	0x80000000
#define BR_ERR	0x40000000
#define BR_FRAM 0x20000000
#define BR_OFLO 0x10000000
#define BR_CRC	0x08000000
#define BR_BUFF 0x04000000
#define BR_STP	0x02000000
#define BR_ENP	0x01000000

#define BT_OWN	0x80000000
#define BT_ERR	0x40000000
#define BT_MORE 0x10000000
#define BT_ONE	0x08000000
#define BT_DEF	0x04000000
#define BT_STP	0x02000000
#define BT_ENP	0x01000000

#define TD_BUFF 0x8000
#define TD_UFLO 0x4000
#define TD_LCOL 0x1000
#define TD_LCAR 0x0800
#define TD_RTRY 0x0400
/*
 * The physical memory used by the lance-host communication is
 * only referenced by its virtual address. Whenever a physical address
 * is needed the high-order bit is just cleared.
 */

struct ln_softc {
	struct	device sc_dev;		/* base device glue */
	struct	ethercom sc_ethercom;	/* Ethernet common part */

	struct	initblock *sc_ib;	/* LANCE initblock */
	struct	buffdesc *sc_rdesc;
	struct	buffdesc *sc_tdesc;
	int	sc_first_td, sc_last_td, sc_no_td;
	int	sc_last_rd;
	char	sc_enaddr[6];
};

static	inline struct mbuf *ln_get __P((struct ln_softc *, caddr_t, int));
void	ln_start __P((struct ifnet *));
void	ln_watchdog __P((struct ifnet *));
int	ln_ioctl __P((struct ifnet *, u_long, caddr_t));
void	ln_setladrf __P((struct ethercom *, u_int16_t *));
static	void ln_intr __P((int));
int	lnmatch __P((struct device *, struct cfdata *, void *));
void	lnattach __P((struct device *, struct device *, void *));
static	void ln_init __P((struct ln_softc *));
static	inline int ln_put __P((struct ln_softc *, caddr_t, struct mbuf *));
static	inline void ln_rint __P((struct ln_softc *));
static	inline void ln_tint __P((struct ln_softc *));
static	inline void ln_read __P((struct ln_softc *, caddr_t, int)); 
void	ln_reset __P((struct ln_softc *));

static	short *lance_csr; /* LANCE CSR virtual address */
static	int *lance_addr; /* Ethernet address */

struct cfattach ln_ca = {
	sizeof(struct ln_softc), lnmatch, lnattach
};

#define LEWRCSR(port, val) { lance_csr[2] = port; lance_csr[0] = (val); }
#define LERDCSR(port) (lance_csr[2] = port, lance_csr[0])

#define ifp	(&sc->sc_ethercom.ec_if)

int
lnmatch(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct	vsbus_attach_args *va = aux;

	if (lance_csr == 0)
		lance_csr = (short *)vax_map_physmem(NI_BASE, 1);
	if (lance_csr == 0)
		return 0;
	if (lance_addr == 0)
		lance_addr = (int *)vax_map_physmem(NI_ADDR, 1);
	if (va->va_type == inr_ni)
		return 2;
	return 0;
}

void
lnattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ln_softc *sc = (void *)self;
	u_int highmark;
	int i;

	/*
	 * Get the ethernet address out of rom
	 */
	for (i = 0; i < 6; i++)
		sc->sc_enaddr[i] = (u_char)lance_addr[i];

	/* Make sure the chip is stopped. */
	LEWRCSR(LE_CSR0, LE_C0_STOP);

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = ln_start;
	ifp->if_ioctl = ln_ioctl;
	ifp->if_watchdog = ln_watchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

#define ALLOC(size) highmark; highmark += (int)(size)

	printf("\n%s: address %s\n", self->dv_xname,
	    ether_sprintf(sc->sc_enaddr));

	highmark = (u_int)le_iomem;
	sc->sc_ib = (void *)ALLOC(sizeof(struct initblock));
	bcopy(sc->sc_enaddr, sc->sc_ib->ib_padr, 6);
	sc->sc_rdesc = (void *)ALLOC(sizeof(struct buffdesc) * NRBUF);
	sc->sc_tdesc = (void *)ALLOC(sizeof(struct buffdesc) * NTBUF);
	sc->sc_ib->ib_rdr = ((int)sc->sc_rdesc & 0xffffff) | (RLEN << 29);
	sc->sc_ib->ib_tdr = ((int)sc->sc_tdesc & 0xffffff) | (TLEN << 29);

	for (i = 0; i < NRBUF; i++) {
		sc->sc_rdesc[i].bd_adrflg = ALLOC(BUFSIZE);
		sc->sc_rdesc[i].bd_adrflg &= 0xffffff;
		sc->sc_rdesc[i].bd_bcnt = -BUFSIZE;
		sc->sc_rdesc[i].bd_mcnt = 0;
	}

	for (i = 0; i < NTBUF; i++) {
		sc->sc_tdesc[i].bd_adrflg = ALLOC(BUFSIZE);
		sc->sc_tdesc[i].bd_bcnt = 0xf000;
		sc->sc_tdesc[i].bd_mcnt = 0;
	}


#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname, RND_TYPE_NET);
#endif
#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	vsbus_intr_attach(inr_ni, ln_intr, 0);
	vsbus_intr_enable(inr_ni);

	/*
	 * Register this device as boot device if we booted from it.
	 * This will fail if there are more than one le in a machine,
	 * fortunately there may be only one.
	 */
	if (B_TYPE(bootdev) == BDEV_LE)
		booted_from = self;
}

void
ln_reset(sc)
	struct ln_softc *sc;
{
	int s;

	s = splimp();
	ln_init(sc);
	splx(s);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
void
ln_init(sc)
	struct ln_softc *sc;
{
	int timo, i;

	LEWRCSR(LE_CSR0, LE_C0_STOP);
	DELAY(100);

	/* Set the correct byte swapping mode, etc. */
	LEWRCSR(LE_CSR3, 0);

#if NBPFILTER > 0
	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_ib->ib_mode = LE_MODE_NORMAL | LE_MODE_PROM;
	else
#endif
		sc->sc_ib->ib_mode = LE_MODE_NORMAL;

	ln_setladrf(&sc->sc_ethercom, sc->sc_ib->ib_ladrf);

	for (i = 0; i < NRBUF; i++) {
		sc->sc_rdesc[i].bd_adrflg &= 0xffffff;
		sc->sc_rdesc[i].bd_adrflg |= BR_OWN;
		sc->sc_rdesc[i].bd_bcnt = -BUFSIZE;
		sc->sc_rdesc[i].bd_mcnt = 0;
	}

	for (i = 0; i < NTBUF; i++) {
		sc->sc_tdesc[i].bd_adrflg &= 0xffffff;
		sc->sc_tdesc[i].bd_bcnt = 0xf000;
		sc->sc_tdesc[i].bd_mcnt = 0;
	}

	sc->sc_first_td = sc->sc_last_td = sc->sc_no_td = sc->sc_last_rd = 0;
	/* Give LANCE the physical address of its init block. */
	LEWRCSR(LE_CSR1, (int)sc->sc_ib & 0xffff);
	LEWRCSR(LE_CSR2, ((int)sc->sc_ib >> 16) & 255);

	/* Try to initialize the LANCE. */
	DELAY(100);
	LEWRCSR(LE_CSR0, LE_C0_INIT);

	/* Wait for initialization to finish. */
	for (timo = 100000; timo; timo--)
		if (LERDCSR(LE_CSR0) & LE_C0_IDON)
			break;

	if (LERDCSR(LE_CSR0) & LE_C0_IDON) {
		/* Start the LANCE. */
		LEWRCSR(LE_CSR0, LE_C0_INEA | LE_C0_STRT | LE_C0_IDON);
		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
		ifp->if_timer = 0;
		ln_start(ifp);
	} else
		printf("%s: card failed to initialize\n", sc->sc_dev.dv_xname);
}

/*
 * Routine to copy from mbuf chain to transmit buffer in
 * network buffer memory.
 */
static inline int
ln_put(sc, boff, m)
	struct ln_softc *sc;
	caddr_t boff;
	struct mbuf *m;
{
	struct mbuf *n;
	int len, tlen = 0;

	boff += KERNBASE;
	for (; m; m = n) {
		len = m->m_len;
		if (len == 0) {
			MFREE(m, n);
			continue;
		}
		bcopy(mtod(m, caddr_t), boff, len);
		boff += len;
		tlen += len;
		MFREE(m, n);
	}
	if (tlen < LEMINSIZE) {
		bzero(boff, LEMINSIZE - tlen);
		tlen = LEMINSIZE;
	}
	return (tlen);
}

/*
 * Pull data off an interface.
 * Len is length of data, with local net header stripped.
 * We copy the data into mbufs.	 When full cluster sized units are present
 * we copy into clusters.
 */
static inline struct mbuf *
ln_get(sc, boff, totlen)
	struct ln_softc *sc;
	caddr_t boff;
	int totlen;
{
	register struct mbuf *m;
	struct mbuf *top, **mp;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return (0);
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	len = MHLEN;
	top = 0;
	mp = &top;
	boff += KERNBASE;

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
		if (!top) {
			register int pad =
			    ALIGN(sizeof(struct ether_header)) -
				sizeof(struct ether_header);
			m->m_data += pad;
			len -= pad;
		}
		m->m_len = len = min(totlen, len);
		bcopy(boff, mtod(m, caddr_t), len);
		boff += len;
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return (top);
}

/*
 * Pass a packet to the higher levels.
 */
static inline void
ln_read(sc, boff, len)
	struct ln_softc *sc;
	caddr_t boff;
	int len;
{
	struct mbuf *m;
	struct ether_header *eh;

	if (len <= sizeof(struct ether_header) ||
	    len > ETHERMTU + sizeof(struct ether_header)) {
		ifp->if_ierrors++;
		return;
	}

	/* Pull packet off interface. */
	m = ln_get(sc, boff, len);
	if (m == 0) {
		ifp->if_ierrors++;
		return;
	}

	ifp->if_ipackets++;

	/* We assume that the header fit entirely in one mbuf. */
	eh = mtod(m, struct ether_header *);

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to BPF.
	 */
	if (ifp->if_bpf) {
		bpf_mtap(ifp->if_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.	And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) != 0 &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, sc->sc_enaddr, 6)) {
			m_freem(m);
			return;
		}
	}
#endif

	/* Pass the packet up, with the ether header sort-of removed. */
	m_adj(m, sizeof(struct ether_header));
	ether_input(ifp, eh, m);
}

static inline void
ln_rint(sc)
	struct ln_softc *sc;
{
	struct	buffdesc *bd;
	register int bix;

	bix = sc->sc_last_rd;

	/* Process all buffers with valid data. */
	for (;;) {
		bd = sc->sc_rdesc + bix;

		if (bd->bd_adrflg < 0)
			break;

		if (bd->bd_adrflg & BR_ERR) {
			if ((bd->bd_adrflg & BR_ENP) == 0) {
				if (bd->bd_adrflg & BR_OFLO)
					printf("%s: overflow\n",
					    sc->sc_dev.dv_xname);
			}
			if (bd->bd_adrflg & BR_BUFF)
				printf("%s: receive buffer error\n",
				    sc->sc_dev.dv_xname);
			ifp->if_ierrors++;
		} else if ((bd->bd_adrflg & (BR_STP | BR_ENP)) !=
		    (BR_STP | BR_ENP)) {
			printf("%s: dropping chained buffer\n",
			    sc->sc_dev.dv_xname);
			ifp->if_ierrors++;
		} else {
			ln_read(sc, (caddr_t)(bd->bd_adrflg & 0xffffff),
			    bd->bd_mcnt - 4);
		}

		bd->bd_adrflg &= 0xffffff;
		bd->bd_bcnt = -BUFSIZE;
		bd->bd_mcnt = 0;
		bd->bd_adrflg |= BR_OWN;

		if (++bix == NRBUF)
			bix = 0;
	}

	sc->sc_last_rd = bix;
}

static inline void
ln_tint(sc)
	register struct ln_softc *sc;
{
	struct buffdesc *bd;
	register int bix;

	bix = sc->sc_first_td;

	for (;;) {
		if (sc->sc_no_td <= 0)
			break;

		bd = sc->sc_tdesc + bix;

		if (bd->bd_adrflg < 0)
			break;

		ifp->if_flags &= ~IFF_OACTIVE;

		if (bd->bd_adrflg & BT_ERR) {
			if (bd->bd_mcnt & TD_BUFF)
				printf("%s: transmit buffer error\n",
				    sc->sc_dev.dv_xname);
			else if (bd->bd_mcnt & TD_UFLO)
				printf("%s: underflow\n", sc->sc_dev.dv_xname);
			if (bd->bd_mcnt & (TD_BUFF | TD_UFLO)) {
				ln_reset(sc);
				return;
			}
			if (bd->bd_mcnt & TD_LCAR) {
				printf("%s: lost carrier\n",
				    sc->sc_dev.dv_xname);
			}
			if (bd->bd_mcnt & TD_LCOL)
				ifp->if_collisions++;
			if (bd->bd_mcnt & TD_RTRY) {
				printf("%s: excessive collisions, tdr %d\n",
				    sc->sc_dev.dv_xname,
				    bd->bd_mcnt & LE_T3_TDR_MASK);
				ifp->if_collisions += 16;
			}
			ifp->if_oerrors++;
		} else {
			if (bd->bd_adrflg & BT_ONE)
				ifp->if_collisions++;
			else if (bd->bd_adrflg & BT_MORE)
				/* Real number is unknown. */
				ifp->if_collisions += 2;
			ifp->if_opackets++;
		}

		if (++bix == NTBUF)
			bix = 0;

		--sc->sc_no_td;
	}

	sc->sc_first_td = bix;

	ln_start(ifp);

	if (sc->sc_no_td == 0)
		ifp->if_timer = 0;
}

/*
 * Controller interrupt.
 */
static void
ln_intr(arg)
	int arg;
{
	register struct ln_softc *sc = ln_cd.cd_devs[arg];
	unsigned short r0;

	/*
	 * This is the way recommended by DEC to not loose interrupts
	 * while handling them.
	 */
	r0 = LERDCSR(LE_CSR0);
	r0 &= ~LE_C0_INEA;
	LEWRCSR(LE_CSR0, r0);
	LEWRCSR(LE_CSR0, LE_C0_INEA);

	if (r0 & LE_C0_ERR) {
		if (r0 & LE_C0_BABL) {
			ifp->if_oerrors++;
		}
		if (r0 & LE_C0_MISS) {
			ln_init(sc);
			ifp->if_ierrors++;
		}
		if (r0 & LE_C0_MERR) {
			printf("%s: memory error\n", sc->sc_dev.dv_xname);
			ln_init(sc);
			return;
		}
	}

	if ((r0 & LE_C0_RXON) == 0) {
		printf("%s: receiver disabled\n", sc->sc_dev.dv_xname);
		ifp->if_ierrors++;
		ln_init(sc);
		return;
	}
	if ((r0 & LE_C0_TXON) == 0) {
		printf("%s: transmitter disabled\n", sc->sc_dev.dv_xname);
		ifp->if_oerrors++;
		ln_init(sc);
		return;
	}

	if (r0 & LE_C0_RINT)
		ln_rint(sc);
	if (r0 & LE_C0_TINT)
		ln_tint(sc);

#if NRND > 0
	rnd_add_uint32(&sc->rnd_source, r0);
#endif

	return;
}

#undef	ifp

void
ln_watchdog(ifp)
	struct ifnet *ifp;
{
	struct ln_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout %x\n", sc->sc_dev.dv_xname,
	    LERDCSR(LE_CSR0));
	++ifp->if_oerrors;

	ln_reset(sc);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue, and map it to the
 * interface before starting the output.
 * Called only at splimp or interrupt level.
 */
void
ln_start(ifp)
	register struct ifnet *ifp;
{
	register struct ln_softc *sc = ifp->if_softc;
	struct buffdesc *bd;
	register int bix;
	register struct mbuf *m, *m0;
	caddr_t addr, faddr;
	int len;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	bix = sc->sc_last_td;

	for (;;) {
		bd = sc->sc_tdesc + bix;

		if (bd->bd_adrflg < 0) {
			ifp->if_flags |= IFF_OACTIVE;
			printf("missing buffer, no_td = %d, last_td = %d\n",
			    sc->sc_no_td, sc->sc_last_td);
		}

		IF_DEQUEUE(&ifp->if_snd, m);
		if (m == 0)
			break;

#if NBPFILTER > 0
		/*
		 * If BPF is listening on this interface, let it see the packet
		 * before we commit it to the wire.
		 */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m);
#endif

		addr = (caddr_t)(bd->bd_adrflg & 0xffffff) + KERNBASE;
		m0 = m;
		len = 0;
		while (m0) {
			faddr = mtod(m0, caddr_t);
			bcopy(faddr, addr, m0->m_len);
			len += m0->m_len;
			addr += m0->m_len;
			m0 = m0->m_next;
		}
		m_freem(m);

		ifp->if_timer = 5;

		if (len < LEMINSIZE)
			bzero(addr, LEMINSIZE - len);
		/*
		 * Init transmit registers, and set transmit start flag.
		 */
		bd->bd_bcnt = -max(len, LEMINSIZE);
		bd->bd_mcnt = 0;
		bd->bd_adrflg |= BT_OWN | BT_STP | BT_ENP;

		LEWRCSR(LE_CSR0, LE_C0_INEA | LE_C0_TDMD);

		if (++bix == NTBUF)
			bix = 0;

		if (++sc->sc_no_td == NTBUF) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

	}

	sc->sc_last_td = bix;
}

/*
 * Process an ioctl request.
 */
int
ln_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	register struct ln_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splimp();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			ln_init(sc);
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
			else {
				bcopy(ina->x_host.c_host,
				    LLADDR(ifp->if_sadl),
				    sizeof(sc->sc_enaddr));
			}	
			/* Set new address. */
			ln_init(sc);
			break;
		    }
#endif
		default:
			ln_init(sc);
			break;
		}
		break;

#if defined(CCITT) && defined(LLC)
	case SIOCSIFCONF_X25:
		ifp->if_flags |= IFF_UP;
		ifa->ifa_rtrequest = cons_rtrequest; /* XXX */
		error = x25_llcglue(PRC_IFUP, ifa->ifa_addr);
		if (error == 0)
			ln_init(sc);
		break;
#endif /* CCITT && LLC */

	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			LEWRCSR(LE_CSR0, LE_C0_STOP);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			ln_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			/*ln_stop(sc);*/
			ln_init(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			ln_reset(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);
	return (error);
}

/*
 * Set up the logical address filter.
 */
void
ln_setladrf(ac, af)
	struct ethercom *ac;
	u_int16_t *af;
{
	struct ifnet *ifp = &ac->ec_if;
	struct ether_multi *enm;
	register u_char *cp, c;
	register u_int32_t crc;
	register int i, len;
	struct ether_multistep step;

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 6 bits as an
	 * index into the 64 bit logical address filter.  The high order bit
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	if (ifp->if_flags & IFF_PROMISC)
		goto allmulti;

	af[0] = af[1] = af[2] = af[3] = 0x0000;
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi, 6)) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			goto allmulti;
		}

		cp = enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = sizeof(enm->enm_addrlo); --len >= 0;) {
			c = *cp++;
			for (i = 8; --i >= 0;) {
				if ((crc & 0x01) ^ (c & 0x01)) {
					crc >>= 1;
					crc ^= 0xedb88320;
				} else
					crc >>= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Set the corresponding bit in the filter. */
		af[crc >> 4] |= 1 << (crc & 0xf);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
	return;

allmulti:
	ifp->if_flags |= IFF_ALLMULTI;
	af[0] = af[1] = af[2] = af[3] = 0xffff;
}
