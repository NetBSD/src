/* $NetBSD: if_xb.c,v 1.9 2001/08/20 12:20:04 wiz Exp $ */

/* [Notice revision 2.2]
 * Copyright (c) 1997, 1998 Avalon Computer Systems, Inc.
 * All rights reserved.
 *
 * Author: Ross Harvey
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright and
 *    author notice, this list of conditions, and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Avalon Computer Systems, Inc. nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This copyright will be assigned to The NetBSD Foundation on
 *    1/1/2000 unless these terms (including possibly the assignment
 *    date) are updated in writing by Avalon prior to the latest specified
 *    assignment date.
 *
 * THIS SOFTWARE IS PROVIDED BY AVALON COMPUTER SYSTEMS, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL AVALON OR THE CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * Notes.
 * 
 * Since the NetBSD build rules force the use of function prototypes, even on
 * functions that are defined before they are called, I've taken advantage of
 * the opportunity and organized this module in top down fashion, with
 * functions generally calling down the page rather than up. It's different.
 * I think I'm getting to like it this way.
 * 
 * The crossbar interface is not exactly a peripheral device, and it cannot
 * appear on anything other than an alpha-based Avalon A12.  The crossbar
 * controller is built into the core logic.
 * 
 * If this version of the driver supports MPS transport, it may have some
 * large static data declarations. Don't worry about it, as Avalon a12
 * support should not appear in a GENERIC or INSTALL kernel.
 * 
 * (Every A12 ever shipped had 512 MB per CPU except one site, which had 256
 * MB.  Partly has a result of this, it is unlikely that a kernel configured
 * for an A12 would be exactly the thing to use on most workstations, so we
 * don't really need to worry that we might be configured in a generic or
 * site-wide kernel image.)
 * 
 * This preliminary crossbar driver supports IP transport using PIO.  Although
 * it would be nice to have a DMA driver, do note that the crossbar register
 * port is 128 bits wide, so we have 128-bit PIO.  (The 21164 write buffer
 * will combine two 64-bit stores before they get off-chip.) Also, the rtmon
 * driver wasn't DMA either, so at least the NetBSD driver is as good as any
 * other that exists now.
 * 
 * We'll do DMA and specialized transport ops later. Given the high speed of
 * the PIO mode, no current applications require DMA bandwidth, but everyone
 * benefits from low latency. The PIO mode is actually lower in latency
 * anyway.
 */

#include "opt_avalon_a12.h"		/* Config options headers */
#include <sys/cdefs.h>			/* RCS ID & Copyright macro defns */

__KERNEL_RCSID(0, "$NetBSD: if_xb.c,v 1.9 2001/08/20 12:20:04 wiz Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <sys/socket.h>
#include <sys/mbuf.h>
#include <sys/sockio.h>

#include <uvm/uvm_extern.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netinet/in_var.h>

#include <machine/autoconf.h>
#include <machine/rpb.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/dec/clockvar.h>
#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <alpha/pci/a12creg.h>
#include <alpha/pci/a12cvar.h>
#include <alpha/pci/pci_a12.h>

#if 1
#define	XB_DEBUG xb_debug
#else
#define	XB_DEBUG 0
#endif

#undef	Static
#if 1
#define	Static
#else
#define	Static static
#endif

#define	IF_XB()	/* Generate ctags(1) key */

#define	XBAR_MTU	   (9*1024)	/* Quite an arbitrary number */
#define	XBAR_MAXFRAMEHDR   48		/* Used to compute if_mtu */

#define	XB_DEFAULT_MTU() (XBAR_MTU - XBAR_MAXFRAMEHDR)

#define	FIFO_WORDCOUNT 60

static int  xb_put_blk __P((struct mbuf *));
static int  xb_put __P((struct mbuf *));
static long xb_fifo_empty(void);

int	xbmatch __P((struct device *, struct cfdata *, void *));
void	xbattach __P((struct device *, struct device *, void *));

struct xb_softc {
	struct	device d;
} xb_softc;

struct cfattach xb_ca = {
	sizeof(struct xb_softc), xbmatch, xbattach,
};

extern struct cfdriver xb_cd;

long	*xb_incoming;
int	xb_incoming_max = XBAR_MTU;

typedef struct ccode_struct {
	int64_t	lo64,		/* magic channel address s-word, high part*/
		hi64;		/* magic channel address s-word, low part */
} ccode_type;
/*
 * Switch channel codes.  Prepending one of these words will get you through
 * the switch, which will eat the word, open the addressed channel, and
 * forward the rest of the switch frame.  Obviously, this helps if the second
 * switch word in the frame is the address word for a cascaded switch. (This
 * can be repeated for an arbitrary depth of MSN.) The words aren't quite as
 * weird as they look: the switch is really lots of narrow switches in an
 * array, and they don't switch an even number of hex digits.  Also, there is
 * a parity bit on most of the subunits.
 */
ccode_type channel[]={
	    { 0x0000000000000000, 0x0000000000000000  },
	    { 0x8882108421084210, 0x1104210842108421  },
	    { 0x4441084210842108, 0x2208421084210842  },
	    { 0xccc318c6318c6318, 0x330c6318c6318c63  },
	    { 0x2220842108421084, 0x4410842108421084  },
	    { 0xaaa294a5294a5294, 0x5514a5294a5294a5  },
	    { 0x66618c6318c6318c, 0x6618c6318c6318c6  },
	    { 0xeee39ce739ce739c, 0x771ce739ce739ce7  },
	    { 0x1110421084210842, 0x8821084210842108  },
	    { 0x99925294a5294a52, 0x9925294a5294a529  },
	    { 0x55514a5294a5294a, 0xaa294a5294a5294a  },
	    { 0xddd35ad6b5ad6b5a, 0xbb2d6b5ad6b5ad6b  },
	    { 0x3330c6318c6318c6, 0xcc318c6318c6318c  },
	    { 0xbbb2d6b5ad6b5ad6, 0xdd35ad6b5ad6b5ad  }
};

Static enum xb_intr_rcv_state_t {
	XBIR_PKTHDR = 0, XBIR_TRANS
} xb_intr_rcv_state;

struct xb_config { int am_i_used; } xb_configuration;

Static struct ifnet xbi;

Static int frame_len;
static int xb_debug;

Static void xb_start __P((struct ifnet *));
Static void xb_mcrp_write __P((long *, int, int));
static __inline void xb_onefree __P((void));
static long set_interrupt_on_fifo_empty(void);
static void xb_init(struct ifnet *);
static int  xb_intr __P((void *));
static void xb_intr_rcv __P((void));
Static void quickload __P((volatile long *, long *));
static void xb_init_config __P((struct xb_config *, int));
static int  xb_output __P((struct ifnet *, struct mbuf *, struct sockaddr *,
			struct rtentry *));
static int  xb_ioctl __P((struct ifnet *, u_long, caddr_t));
static void xb_stop __P((void));
static void a12_xbar_setup __P((void));

/* There Can Be Only One */

int xbfound;

int
xbmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct pcibus_attach_args *pba = aux;

	return	cputype == ST_AVALON_A12
	    &&	strcmp(pba->pba_busname, xb_cd.cd_name) == 0
		&& !xbfound;
}

void
xbattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct xb_config *ccp;

	strcpy(xbi.if_xname, self->dv_xname);
	xbfound = 1;
	ccp = &xb_configuration;
	xb_init_config(ccp, 1);
	printf(": driver %s mtu %d\n", "$Revision: 1.9 $", xbi.if_mtu);
}

static void
xb_init_config(ccp, mallocsafe)
	struct xb_config *ccp;
	int mallocsafe;
{
	/* 
	 * The driver actually only needs about 64 bytes of buffer but with a
	 * nice contiguous frame we can call m_devget()
	 */
	if (mallocsafe && xb_incoming == NULL) {
		xb_incoming = malloc(xb_incoming_max, M_DEVBUF, M_NOWAIT);
		if (xb_incoming == NULL)
			DIE();
	}
	a12_xbar_setup();
	a12_intr_register_xb(xb_intr);
}

/*
 * From The A12 Theory of Operation. Used with permission.
 *      --- --- ------ -- ---------
 *
 *	 Message Channel Status Register
 *
 *     31                                 0
 *      |                                 |
 *      10987654 32109876 54321098 76543210 
 *
 *      ........ ........ 0oiefaAr TR...... MCSR
 *
 *     Field   Type    Name    Function
 *
 *      R      R,W1C   RBC     Receive Block Complete
 *      T      R,W1C   TBC     Transmit Block Complete
 *      r      R       IMP     Incoming message pending
 *      A      R       IMFAE   Incoming message fifo almost empty
 *      a      R       OMFAF   Outgoing message fifo almost full
 *      f      R       OMFF    Outgoing message fifo full
 *      e      R       OMFE    Outgoing message fifo empty
 *      i      R       DMAin   Incoming DMA channel armed
 *      o      R       DMAout  Outgoing DMA channel armed
 *     
 *	     Interrupts Generated from MCSR
 *
 *     IMChInt <= (RBC or IMP) and not DMAin
 *     OMChInt <= ((TBC and not OMFAF) or (OMFE and OMR.E(6))
 *		) and not DMAout
 *
 */
static int
xb_intr(p)
	void *p;
{
	int	n;
	long	mcsrval;
	/* 
	 * The actual conditions under which this interrupt is generated are
	 * a bit complicated, and no status flag is available that reads out
	 * the final values of the interrupt inputs. But, it doesn't really
	 * matter. Simply check for receive data and transmitter IFF_OACTIVE.
	 */
	while ((mcsrval = REGVAL(A12_MCSR)) & A12_MCSR_IMP)
		for(n = mcsrval & A12_MCSR_IMFAE ? 1 : 5; n; --n)
			xb_intr_rcv();

	if (xbi.if_flags & IFF_OACTIVE
	&&  mcsrval & A12_MCSR_OMFE) {
		xbi.if_flags &= ~IFF_OACTIVE;
		REGVAL(A12_OMR) &= ~A12_OMR_OMF_ENABLE;
		alpha_wmb();
		xb_start(&xbi);
	}
	return 0;
}
/* 
 * The interface logic will shoot us down with MCE (Missing Close Error) or
 * ECE (Embedded Close Error) if we aren't in sync with the hardware w.r.t.
 * frame boundaries. As those are panic-level errors: Don't Get Them.
 */
static void
xb_intr_rcv()
{
struct	mbuf *m;
long	frameword[2];
static	long *xb_ibp;
int	s = 0;	/* XXX gcc */

	switch (xb_intr_rcv_state) {
	    case XBIR_PKTHDR:
		xb_ibp = xb_incoming;
		quickload(REGADDR(A12_FIFO), frameword); /* frame_len >= 16 */
		frame_len = frameword[0];
		if (!(20 <= frame_len && frame_len+16 <= xb_incoming_max))
			DIE();
		/*
		 * The extra word when frames are of an aligned size is due
		 * to the way the output routines work. After the mbuf is
		 * sent xb_put_blk(NULL) is called. If there is a leftover
		 * 127-bit-or-less fragment then the close word rides on it,
		 * otherwise it gets an entire 128 bits of zeroes.
		 */
		if (frame_len & 0xf)
			frame_len = (frame_len + 0xf) >> 4;
		else	frame_len = (frame_len >> 4) + 1;
	      --frame_len; /* we read the frame len + the first packet int64 */
		*xb_ibp++ = frameword[1];
		xb_intr_rcv_state = XBIR_TRANS;
		break;
	    case XBIR_TRANS:
		if (frame_len > 1)
			quickload(REGADDR(A12_FIFO), frameword);
		else if (frame_len == 1) {
			quickload(REGADDR(A12_FIFO_LWE), frameword);
			xb_intr_rcv_state = XBIR_PKTHDR;
		} else if (XB_DEBUG)
			DIE();
	      --frame_len;
		xb_ibp[0] = frameword[0];
		xb_ibp[1] = frameword[1];
		xb_ibp   += 2;
		if (xb_intr_rcv_state == XBIR_PKTHDR) {
			if (XB_DEBUG) {
				s = splnet();
				if (s != splnet())
					DIE();
			}
		      ++xbi.if_ipackets;
			if (IF_QFULL(&ipintrq)) {
				IF_DROP(&ipintrq);
			      ++xbi.if_iqdrops;
			} else {
				m = m_devget((caddr_t)xb_incoming,
					(caddr_t)xb_ibp - (caddr_t)xb_incoming,
					0, &xbi, 0L);
				if (m) {
					xbi.if_ibytes += m->m_pkthdr.len;
					IF_ENQUEUE(&ipintrq, m);
				} else
				      ++xbi.if_ierrors;
			}
			if (XB_DEBUG)
				splx(s);
		}
		break;
	    default:
		DIE();
	}
}
/*
 * Make it easy for gcc to load a[0..1] without interlocking between
 * a[0] and a[1]. (If it did, that would be two external bus cycles.)
 */
Static void
quickload(volatile long *a, long *b)
{
long	t1,t2;

	t1 = a[0];
	t2 = a[1];
	b[0] = t1;
	b[1] = t2;
}
/*
 * Verify during debugging that we have not overflowed the FIFO 
 */
static __inline void
xb_onefree()
{
	if (XB_DEBUG && REGVAL(A12_MCSR) & A12_MCSR_OMFF)
		DIE();
}

static void
xb_init(ifp)
	struct ifnet *ifp;
{
	ifp->if_flags |= IFF_RUNNING;
}

static void
xb_stop()
{
}

static int
xb_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();
	switch (cmd) {
	case SIOCSIFADDR:
		xbi.if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			xb_init(ifp);
			break;
#endif
		default:
			xb_init(ifp);
		}
		break;
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			xb_stop();
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		           (ifp->if_flags & IFF_RUNNING) == 0) {
			xb_start(ifp);
		} else
			xb_init(ifp);
		if (ifp->if_flags & IFF_DEBUG)
			xb_debug = 1;
		break;
	default:
		error = EINVAL;
		break;
	}
	splx(s);
	return error;
}
/*
 * XXX - someday, keep a software copy of A12_OMR. We can execute up to
 * 200 or 300 instructions in the time it takes to do the read part of an
 * external bus cycle RMW op. (Or 10 - 20 cache cycles.)
 */
static __inline long
xb_fifo_empty(void)
{
	return REGVAL(A12_MCSR) & A12_MCSR_OMFE;
}
/*
 * rtmon frames
 *
 * [ (... data) : commid : sourcepid : dstpid : ktype : length : frametype ]
 *
 * At the moment, NetBSD ip frames are not compatible with rtmon frames:
 *
 * [ ... data : length ]
 */
static int
xb_output(ifp, m0, dst, rt0)
	struct ifnet *ifp;
	struct mbuf *m0;
	struct sockaddr *dst;
	struct rtentry *rt0;
{
	int	i,s;
	struct	mbuf *m = m0;
	char	*lladdr;
	caddr_t	xbh;
	long	xbo_framesize;
	struct	sockaddr_dl *llsa;
	int	xbaddr;

#ifdef DIAGNOSTIC
	if (ifp != &xbi)
		DIE();
#endif
	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		m_freem(m);
		return ENETDOWN;
	}
	/*
	 * We want an IP packet with a link level route, on a silver platter.
	 */
	if (rt0 == NULL
	|| (rt0->rt_flags & (RTF_GATEWAY | RTF_LLINFO))
	|| (llsa = (struct sockaddr_dl *)rt0->rt_gateway) == NULL
	||  llsa->sdl_family != AF_LINK
	||  llsa->sdl_slen   != 0) {
	      ++ifp->if_oerrors;
		m_freem(m);
		return EHOSTUNREACH;
	}
	if (dst == NULL
	||  dst->sa_family != AF_INET) {
		/*
		 * This is because we give all received packets to ipintrq
		 * right now.
		 */
		What();
		m_freem(m);
	      ++ifp->if_noproto;
		return EAFNOSUPPORT;
	}
	/* 
	 * The a12MppSwitch is a wormhole routed MSN consisting of a number
	 * (usually n==1) of 14 channel crossbar switches.  Each route through
	 * a switch requires a 128 bit address word that specifies the channel
	 * to emerge on. The address word is eaten by the switch and the
	 * rest of the packet is routed through.
	 */
	lladdr = LLADDR(llsa);
	if (llsa->sdl_alen != 1)			/* XXX */
		DIE();	/* OK someday, but totally unexpected right now */
	/*
	 * Alternatively, we could lookup the address word and output
	 * it with PIO when the mbuf is dequeued
	 */
	xbo_framesize = m->m_pkthdr.len + 8;
	M_PREPEND(m, 16 * llsa->sdl_alen + 8, M_DONTWAIT);
	if (m == NULL)
		return ENOBUFS;
	xbh = mtod(m, caddr_t);
	for (i=0; i<llsa->sdl_alen; ++i) {
		xbaddr = (lladdr[i] & 0xff) - 1;
		if (!(0 <= xbaddr && xbaddr <= 11))	/* XXX */
			DIE();		/* 12 or 13 will be OK later */
		memcpy(xbh, &channel[xbaddr].lo64, 16);
		xbh += 16;
	}
	memcpy(xbh, &xbo_framesize, 8);
	s = splnet();
	if (IF_QFULL(&ifp->if_snd)) {
		IF_DROP(&ifp->if_snd);
	      ++ifp->if_oerrors;
		splx(s);
		m_freem(m);
		return ENOBUFS;
	}
	ifp->if_obytes += m->m_pkthdr.len;
      ++ifp->if_opackets;
	IF_ENQUEUE(&ifp->if_snd, m);
	if ((ifp->if_flags & IFF_OACTIVE) == 0)
		xb_start(ifp);
	splx(s);
	if (m->m_flags & M_MCAST)
		ifp->if_omcasts++;
	return 0;
}

void
xb_start(ifp)
	struct ifnet *ifp;
{
	struct	mbuf *m;

	if ((xbi.if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;
	for (;;) {
		IF_DEQUEUE(&xbi.if_snd, m);
		if (m == 0)
			return;
		/*
		 * XXX The variable-length switch address words cause problems
		 * for bpf, for now, leave it out. XXX It's not too hard to
		 * fix, though, as there are lots of techniques that will
		 * identify the number of switch address words.
		 */
		if (!xb_put(m)) {
			xbi.if_flags |= IFF_OACTIVE;
			return;
		}
	}
}

static int
xb_put(m)
	struct mbuf *m;
{
	struct mbuf *n;
	int len;

	if (XB_DEBUG && (alpha_pal_rdps() & 7) < 3)
		DIE();	/* this "cannot happen", of course */
	for (; m; m = n) {
		len = m->m_len;
		if (len == 0 || xb_put_blk(m))
			MFREE(m, n);
		else	return 0;
	}
	xb_put_blk(NULL);
	return 1;
}
/*
 * Write a single mbuf to the transmit channel fifo. We can only write 128-bit
 * words. Right now, we pad at the end.  It is possible to pad at the
 * beginning, especially since lots of games can be played at the receiver
 * with the mbuf data pointer. Padding at the beginning requires a pad-count
 * field in a header, but it means you can always DMA the data, regardless of
 * alignment. Of course, we don't DMA at all, right now.
 */
static int
xb_put_blk(m)
	struct	mbuf *m;
{
	static	long leftover[2];	/* 0-15 bytes from last xb_put_blk() */
	static	int  leftover_len;	/* non-aligned amount from last call */
	long	xfertmp[8];	/* aligned switch word buffer */
	int 	frag_len,	/* fifo stream unit */
		fifo_len,	/* space left in fifo */
		fillin,		/* amount needed to complete a switch word */
		full,		/* remember to restart on fifo full */
		len;		/* amount of mbuf left to do */
	caddr_t	blk;		/* location we are at in mbuf */
	static	int fifo_free;	/* current # of switch words free in fifo */

#define	XFERADJ() ((caddr_t)xfertmp+leftover_len)

	/* There is always room for the close word */

	if (m == NULL) {
		if (leftover_len)
			leftover_len = 0;
		else	leftover[0]  = leftover[1] = 0;
		xb_mcrp_write(leftover, 1, 1);
	      --fifo_free;
		return 1;
	}

restart:
	if (fifo_free < 2) {
		if (!xb_fifo_empty()) {
			if(!set_interrupt_on_fifo_empty())  {
				/* still empty */
				xbi.if_flags |= IFF_OACTIVE;
				IF_PREPEND(&xbi.if_snd, m);
				return 0;
			}
		}
		fifo_free = FIFO_WORDCOUNT;
	}
	len = m->m_len;
	if (len == 0)
		return 1;	/* clean finish, nothing left over */
	blk = mtod(m, caddr_t);
	if (leftover_len) {
		/* See function intro comment regarding padding */
		if (leftover_len + len < sizeof leftover) {
			/* Heh, not even enough to write out */
			memcpy(XFERADJ(), blk, len);
			leftover_len += len;
			return 1;
		}
		xfertmp[0] = leftover[0];
		xfertmp[1] = leftover[1];
		fillin = sizeof leftover - leftover_len;
		memcpy(XFERADJ(), blk, fillin);
		blk += fillin;
		len -= fillin;
		xb_mcrp_write(xfertmp, 1, 0);
		leftover_len = 0;
	      --fifo_free;
	}
	/* fifo_free is known to be >= 1 at this point */
	while (len >= 16) {
		full = 0;
		frag_len = sizeof xfertmp;
		if (frag_len > len)
			frag_len = len;
		fifo_len = fifo_free * 16;
		if (frag_len > fifo_len) {
			frag_len = fifo_len;
			full = 1;
		}
		frag_len &= ~0xf;
		memcpy(xfertmp, blk, frag_len);
		frag_len >>= 4;		/* Round down to switch word size */
		xb_mcrp_write(xfertmp, frag_len, 0);
		fifo_free -= frag_len;
		frag_len <<= 4;
		len -= frag_len;
		blk += frag_len;
		if (full) {
			m_adj(m, blk - mtod(m, caddr_t));
			goto restart;
		}
	}
	memcpy(leftover, blk, len);
	leftover_len = len;
	return 1;
}

static long
set_interrupt_on_fifo_empty(void)
{
	REGVAL(A12_OMR) |= A12_OMR_OMF_ENABLE;
	alpha_mb();
	if(xb_fifo_empty()) {
		REGVAL(A12_OMR) &= ~A12_OMR_OMF_ENABLE;
		alpha_mb();
		return 1;
	}
	return 0;
}
/*
 * Write an aligned block of switch words to the FIFO
 */
Static void
xb_mcrp_write(d, n, islast)
	long *d;
{
	volatile long *xb_fifo = islast ? REGADDR(A12_FIFO_LWE) 
					: REGADDR(A12_FIFO);
	int i;

	if (XB_DEBUG && islast && n != 1)
		DIE();
	n <<= 1;
	for (i = 0; i < n; i += 2) {
		xb_onefree();
		xb_fifo[0] = d[i];
		xb_fifo[1] = d[i+1];
		alpha_wmb();
	}
}

/*
const
int32_t	xbar_bc_addr = XBAR_BROADCAST;
 */

static void
a12_xbar_setup()
{
	xbi.if_softc    = &xb_softc;
	xbi.if_start    = xb_start;
	xbi.if_ioctl    = xb_ioctl;
	xbi.if_flags    = IFF_BROADCAST	/* ha ha */
		        | IFF_SIMPLEX;

	xbi.if_type     = IFT_A12MPPSWITCH;
	xbi.if_addrlen  = 32;
	xbi.if_hdrlen   = 32;
	xbi.if_mtu      = XB_DEFAULT_MTU();
	xbi.if_output   = xb_output;
	/* xbi.if_broadcastaddr = (u_int8_t)&xbar_bc_addr; */

	if_attach(&xbi);
	if_alloc_sadl(&xbi);

#if NBPFILTER > 0
	bpfattach(&xbi, DLT_NULL, 0);
#endif
}
