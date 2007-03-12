/*	$NetBSD: if_ie.c,v 1.44.26.1 2007/03/12 05:51:06 rmind Exp $ */

/*-
 * Copyright (c) 1993, 1994, 1995 Charles M. Hannum.
 * Copyright (c) 1992, 1993, University of Vermont and State
 *  Agricultural College.
 * Copyright (c) 1992, 1993, Garrett A. Wollman.
 *
 * Portions:
 * Copyright (c) 1994, 1995, Rafal K. Boni
 * Copyright (c) 1990, 1991, William F. Jolitz
 * Copyright (c) 1990, The Regents of the University of California
 *
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
 *	This product includes software developed by Charles M. Hannum, by the
 *	University of Vermont and State Agricultural College and Garrett A.
 *	Wollman, by William F. Jolitz, and by the University of California,
 *	Berkeley, Lawrence Berkeley Laboratory, and its contributors.
 * 4. Neither the names of the Universities nor the names of the authors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR AUTHORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Intel 82586 Ethernet chip
 * Register, bit, and structure definitions.
 *
 * Original StarLAN driver written by Garrett Wollman with reference to the
 * Clarkson Packet Driver code for this chip written by Russ Nelson and others.
 *
 * BPF support code taken from hpdev/if_le.c, supplied with tcpdump.
 *
 * 3C507 support is loosely based on code donated to NetBSD by Rafal Boni.
 *
 * Majorly cleaned up and 3C507 code merged by Charles Hannum.
 *
 * Converted to SUN ie driver by Charles D. Cranor,
 *		October 1994, January 1995.
 * This sun version based on i386 version 1.30.
 * [ see sys/dev/isa/if_ie.c ]
 */

/*
 * The i82586 is a very painful chip, found in sun3's, sun-4/100's
 * sun-4/200's, and VME based suns.  The byte order is all wrong for a
 * SUN, making life difficult.  Programming this chip is mostly the same,
 * but certain details differ from system to system.  This driver is
 * written so that different "ie" interfaces can be controled by the same
 * driver.
 */

/*
   Mode of operation:

   We run the 82586 in a standard Ethernet mode.  We keep NFRAMES
   received frame descriptors around for the receiver to use, and
   NRXBUF associated receive buffer descriptors, both in a circular
   list.  Whenever a frame is received, we rotate both lists as
   necessary.  (The 586 treats both lists as a simple queue.)  We also
   keep a transmit command around so that packets can be sent off
   quickly.

   We configure the adapter in AL-LOC = 1 mode, which means that the
   Ethernet/802.3 MAC header is placed at the beginning of the receive
   buffer rather than being split off into various fields in the RFD.
   This also means that we must include this header in the transmit
   buffer as well.

   By convention, all transmit commands, and only transmit commands,
   shall have the I (IE_CMD_INTR) bit set in the command.  This way,
   when an interrupt arrives at ieintr(), it is immediately possible
   to tell what precisely caused it.  ANY OTHER command-sending
   routines should run at splnet(), and should post an acknowledgement
   to every interrupt they generate.
*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ie.c,v 1.44.26.1 2007/03/12 05:51:06 rmind Exp $");

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

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

#include <uvm/uvm_extern.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/pmap.h>

/*
 * ugly byte-order hack for SUNs
 */

#define XSWAP(y)	( (((y)&0xff00) >> 8) | (((y)&0xff) << 8) )
#define SWAP(x)		((u_short)(XSWAP((u_short)(x))))

#include "i82586.h"
#include "if_iereg.h"
#include "if_ievar.h"

/* #define	IEDEBUG	XXX */

/*
 * IED: ie debug flags
 */

#define	IED_RINT	0x01
#define	IED_TINT	0x02
#define	IED_RNR		0x04
#define	IED_CNA		0x08
#define	IED_READFRAME	0x10
#define	IED_ENQ		0x20
#define	IED_XMIT	0x40
#define	IED_ALL		0x7f

#ifdef	IEDEBUG
#define	inline	/* not */
void print_rbd(volatile struct ie_recv_buf_desc *);
int     in_ierint = 0;
int     in_ietint = 0;
int     ie_debug_flags = 0;
#endif

/* XXX - Skip TDR for now - it always complains... */
int 	ie_run_tdr = 0;

static void iewatchdog(struct ifnet *);
static int ieinit(struct ie_softc *);
static int ieioctl(struct ifnet *, u_long, void *);
static void iestart(struct ifnet *);
static void iereset(struct ie_softc *);
static int ie_setupram(struct ie_softc *);

static int cmd_and_wait(struct ie_softc *, int, void *, int);

static void ie_drop_packet_buffer(struct ie_softc *);
static void ie_readframe(struct ie_softc *, int);
static inline void ie_setup_config(struct ie_config_cmd *, int, int);

static void ierint(struct ie_softc *);
static void iestop(struct ie_softc *);
static void ietint(struct ie_softc *);
static void iexmit(struct ie_softc *);

static int mc_setup(struct ie_softc *, void *);
static void mc_reset(struct ie_softc *);
static void run_tdr(struct ie_softc *, struct ie_tdr_cmd *);
static void iememinit(struct ie_softc *);

static inline char *Align(char *);
static inline u_int Swap32(u_int);
static inline u_int vtop24(struct ie_softc *, void *);
static inline u_short vtop16sw(struct ie_softc *, void *);

static inline void ie_ack(struct ie_softc *, u_int);
static inline u_short ether_cmp(u_char *, u_char *);
static inline int check_eh(struct ie_softc *, struct ether_header *, int *);
static inline int ie_buflen(struct ie_softc *, int);
static inline int ie_packet_len(struct ie_softc *);
static inline struct mbuf * ieget(struct ie_softc *, int *);


/*
 * Here are a few useful functions.  We could have done these as macros,
 * but since we have the inline facility, it makes sense to use that
 * instead.
 */

/* KVA to 24 bit device address */
static inline u_int 
vtop24(struct ie_softc *sc, void *ptr)
{
	u_int pa;

	pa = (vaddr_t)ptr - (vaddr_t)sc->sc_iobase;
#ifdef	IEDEBUG
	if (pa & ~0xffFFff)
		panic("ie:vtop24");
#endif
	return (pa);
}

/* KVA to 16 bit offset, swapped */
static inline u_short 
vtop16sw(struct ie_softc *sc, void *ptr)
{
	u_int pa;

	pa = (vaddr_t)ptr - (vaddr_t)sc->sc_maddr;
#ifdef	IEDEBUG
	if (pa & ~0xFFff)
		panic("ie:vtop16");
#endif

	return (SWAP(pa));
}

static inline u_int 
Swap32(u_int x)
{
	u_int y;

	y = x & 0xFF;
	y <<= 8; x >>= 8;
	y |= x & 0xFF;
	y <<= 8; x >>= 8;
	y |= x & 0xFF;
	y <<= 8; x >>= 8;
	y |= x & 0xFF;

	return (y);
}

static inline char *
Align(char *ptr)
{
	u_long  l = (u_long)ptr;

	l = (l + 3) & ~3L;
	return ((char *)l);
}


static inline void 
ie_ack(struct ie_softc *sc, u_int mask)
{
	volatile struct ie_sys_ctl_block *scb = sc->scb;

	cmd_and_wait(sc, scb->ie_status & mask, 0, 0);
}


/*
 * Taken almost exactly from Bill's if_is.c,
 * then modified beyond recognition...
 */
void 
ie_attach(struct ie_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;

	/* MD code has done its part before calling this. */
	printf(": macaddr %s\n", ether_sprintf(sc->sc_addr));

	/*
	 * Compute number of transmit and receive buffers.
	 * Tx buffers take 1536 bytes, and fixed in number.
	 * Rx buffers are 512 bytes each, variable number.
	 * Need at least 1 frame for each 3 rx buffers.
	 * The ratio 3bufs:2frames is a compromise.
	 */
	sc->ntxbuf = NTXBUF;	/* XXX - Fix me... */
	switch (sc->sc_msize) {
	case 16384:
		sc->nframes = 8 * 4;
		sc->nrxbuf  = 8 * 6;
		break;
	case 32768:
		sc->nframes = 16 * 4;
		sc->nrxbuf  = 16 * 6;
		break;
	case 65536:
		sc->nframes = 32 * 4;
		sc->nrxbuf  = 32 * 6;
		break;
	default:
		sc->nframes = 0;
	}
	if (sc->nframes > MXFRAMES)
		sc->nframes = MXFRAMES;
	if (sc->nrxbuf > MXRXBUF)
		sc->nrxbuf = MXRXBUF;

#ifdef	IEDEBUG
	printf("%s: %dK memory, %d tx frames, %d rx frames, %d rx bufs\n",
	    sc->sc_dev.dv_xname, (sc->sc_msize >> 10),
	    sc->ntxbuf, sc->nframes, sc->nrxbuf);
#endif

	if ((sc->nframes <= 0) || (sc->nrxbuf <= 0))
		panic("ie_attach: weird memory size");

	/*
	 * Setup RAM for transmit/receive
	 */
	if (ie_setupram(sc) == 0) {
		printf(": RAM CONFIG FAILED!\n");
		/* XXX should reclaim resources? */
		return;
	}

	/*
	 * Initialize and attach S/W interface
	 */
	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_start = iestart;
	ifp->if_ioctl = ieioctl;
	ifp->if_watchdog = iewatchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_addr);
}

/*
 * Setup IE's ram space.
 */
static int 
ie_setupram(struct ie_softc *sc)
{
	volatile struct ie_sys_conf_ptr *scp;
	volatile struct ie_int_sys_conf_ptr *iscp;
	volatile struct ie_sys_ctl_block *scb;
	int off;

	/*
	 * Allocate from end of buffer space for
	 * ISCP, SCB, and other small stuff.
	 */
	off = sc->buf_area_sz;
	off &= ~3;

	/* SCP (address already chosen). */
	scp = sc->scp;
	(sc->sc_memset)(__UNVOLATILE(scp), 0, sizeof(*scp));

	/* ISCP */
	off -= sizeof(*iscp);
	iscp = (volatile void *) (sc->buf_area + off);
	(sc->sc_memset)(__UNVOLATILE(iscp), 0, sizeof(*iscp));
	sc->iscp = iscp;

	/* SCB */
	off -= sizeof(*scb);
	scb  = (volatile void *) (sc->buf_area + off);
	(sc->sc_memset)(__UNVOLATILE(scb), 0, sizeof(*scb));
	sc->scb = scb;

	/* Remainder is for buffers, etc. */
	sc->buf_area_sz = off;

	/*
	 * Now fill in the structures we just allocated.
	 */

	/* SCP: main thing is 24-bit ptr to ISCP */
	scp->ie_bus_use = 0;	/* 16-bit */
	scp->ie_iscp_ptr = Swap32(vtop24(sc, __UNVOLATILE(iscp)));

	/* ISCP */
	iscp->ie_busy = 1;	/* ie_busy == char */
	iscp->ie_scb_offset = vtop16sw(sc, __UNVOLATILE(scb));
	iscp->ie_base = Swap32(vtop24(sc, sc->sc_maddr));

	/* SCB */
	scb->ie_command_list = SWAP(0xffff);
	scb->ie_recv_list    = SWAP(0xffff);

	/* Other stuff is done in ieinit() */
	(sc->reset_586) (sc);
	(sc->chan_attn) (sc);

	delay(100);		/* wait a while... */

	if (iscp->ie_busy) {
		return 0;
	}
	/*
	 * Acknowledge any interrupts we may have caused...
	 */
	ie_ack(sc, IE_ST_WHENCE);

	return 1;
}

/*
 * Device timeout/watchdog routine.  Entered if the device neglects to
 * generate an interrupt after a transmit has been started on it.
 */
static void 
iewatchdog(struct ifnet *ifp)
{
	struct ie_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++ifp->if_oerrors;
	iereset(sc);
}

/*
 * What to do upon receipt of an interrupt.
 */
int 
ie_intr(void *arg)
{
	struct ie_softc *sc = arg;
	u_short status;
	int loopcnt;

	/*
	 * check for parity error
	 */
	if (sc->hard_type == IE_VME) {
		volatile struct ievme *iev = (volatile struct ievme *)sc->sc_reg;
		if (iev->status & IEVME_PERR) {
			printf("%s: parity error (ctrl 0x%x @ 0x%02x%04x)\n",
			    sc->sc_dev.dv_xname, iev->pectrl,
			    iev->pectrl & IEVME_HADDR, iev->peaddr);
			iev->pectrl = iev->pectrl | IEVME_PARACK;
		}
	}

	status = sc->scb->ie_status;
	if ((status & IE_ST_WHENCE) == 0)
		return 0;

	loopcnt = sc->nframes;
loop:
	/* Ack interrupts FIRST in case we receive more during the ISR. */
	ie_ack(sc, IE_ST_WHENCE & status);

	if (status & (IE_ST_RECV | IE_ST_RNR)) {
#ifdef IEDEBUG
		in_ierint++;
		if (sc->sc_debug & IED_RINT)
			printf("%s: rint\n", sc->sc_dev.dv_xname);
#endif
		ierint(sc);
#ifdef IEDEBUG
		in_ierint--;
#endif
	}

	if (status & IE_ST_DONE) {
#ifdef IEDEBUG
		in_ietint++;
		if (sc->sc_debug & IED_TINT)
			printf("%s: tint\n", sc->sc_dev.dv_xname);
#endif
		ietint(sc);
#ifdef IEDEBUG
		in_ietint--;
#endif
	}

	/*
	 * Receiver not ready (RNR) just means it has
	 * run out of resources (buffers or frames).
	 * One can easily cause this with (i.e.) spray.
	 * This is not a serious error, so be silent.
	 */
	if (status & IE_ST_RNR) {
#ifdef IEDEBUG
		printf("%s: receiver not ready\n", sc->sc_dev.dv_xname);
#endif
		sc->sc_if.if_ierrors++;
		iereset(sc);
	}

#ifdef IEDEBUG
	if ((status & IE_ST_ALLDONE) && (sc->sc_debug & IED_CNA))
		printf("%s: cna\n", sc->sc_dev.dv_xname);
#endif

	status = sc->scb->ie_status;
	if (status & IE_ST_WHENCE) {
		/* It still wants service... */
		if (--loopcnt > 0)
			goto loop;
		/* ... but we've been here long enough. */
		log(LOG_ERR, "%s: interrupt stuck?\n",
			sc->sc_dev.dv_xname);
		iereset(sc);
	}
	return 1;
}

/*
 * Process a received-frame interrupt.
 */
void 
ierint(struct ie_softc *sc)
{
	volatile struct ie_sys_ctl_block *scb = sc->scb;
	int i, status;
	static int timesthru = 1024;

	i = sc->rfhead;
	for (;;) {
		status = sc->rframes[i]->ie_fd_status;

		if ((status & IE_FD_COMPLETE) && (status & IE_FD_OK)) {
			if (!--timesthru) {
				sc->sc_if.if_ierrors +=
				    SWAP(scb->ie_err_crc) +
				    SWAP(scb->ie_err_align) +
				    SWAP(scb->ie_err_resource) +
				    SWAP(scb->ie_err_overrun);
				scb->ie_err_crc = 0;
				scb->ie_err_align = 0;
				scb->ie_err_resource = 0;
				scb->ie_err_overrun = 0;
				timesthru = 1024;
			}
			ie_readframe(sc, i);
		} else {
			if ((status & IE_FD_RNR) != 0 &&
			    (scb->ie_status & IE_RU_READY) == 0) {
				sc->rframes[0]->ie_fd_buf_desc = vtop16sw(sc,
				    __UNVOLATILE(sc->rbuffs[0]));
				scb->ie_recv_list = vtop16sw(sc,
				    __UNVOLATILE(sc->rframes[0]));
				cmd_and_wait(sc, IE_RU_START, 0, 0);
			}
			break;
		}
		i = (i + 1) % sc->nframes;
	}
}

/*
 * Process a command-complete interrupt.  These are only generated by the
 * transmission of frames.  This routine is deceptively simple, since most
 * of the real work is done by iestart().
 */
void 
ietint(struct ie_softc *sc)
{
	struct ifnet *ifp;
	int status;

	ifp = &sc->sc_if;

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	status = sc->xmit_cmds[sc->xctail]->ie_xmit_status;

	if (!(status & IE_STAT_COMPL) || (status & IE_STAT_BUSY))
		printf("ietint: command still busy!\n");

	if (status & IE_STAT_OK) {
		ifp->if_opackets++;
		ifp->if_collisions += 
		  SWAP(status & IE_XS_MAXCOLL);
	} else {
		ifp->if_oerrors++;
		/*
		 * XXX
		 * Check SQE and DEFERRED?
		 * What if more than one bit is set?
		 */
		if (status & IE_STAT_ABORT)
			printf("%s: send aborted\n", sc->sc_dev.dv_xname);
		if (status & IE_XS_LATECOLL)
			printf("%s: late collision\n", sc->sc_dev.dv_xname);
		if (status & IE_XS_NOCARRIER)
			printf("%s: no carrier\n", sc->sc_dev.dv_xname);
		if (status & IE_XS_LOSTCTS)
			printf("%s: lost CTS\n", sc->sc_dev.dv_xname);
		if (status & IE_XS_UNDERRUN)
			printf("%s: DMA underrun\n", sc->sc_dev.dv_xname);
		if (status & IE_XS_EXCMAX) {
			/* Do not print this one (too noisy). */
			ifp->if_collisions += 16;
		}
	}

	/*
	 * If multicast addresses were added or deleted while we
	 * were transmitting, mc_reset() set the want_mcsetup flag
	 * indicating that we should do it.
	 */
	if (sc->want_mcsetup) {
		mc_setup(sc, (void *)sc->xmit_cbuffs[sc->xctail]);
		sc->want_mcsetup = 0;
	}

	/* Done with the buffer. */
	sc->xmit_busy--;
	sc->xctail = (sc->xctail + 1) % NTXBUF;

	/* Start the next packet, if any, transmitting. */
	if (sc->xmit_busy > 0)
		iexmit(sc);

	iestart(ifp);
}

/*
 * Compare two Ether/802 addresses for equality, inlined and
 * unrolled for speed.  I'd love to have an inline assembler
 * version of this...   XXX: Who wanted that? mycroft?
 * I wrote one, but the following is just as efficient.
 * This expands to 10 short m68k instructions! -gwr
 * Note: use this like bcmp()
 */
static inline u_short
ether_cmp(u_char *one, u_char *two)
{
	u_short *a = (u_short *) one;
	u_short *b = (u_short *) two;
	u_short diff;

	diff  = *a++ - *b++;
	diff |= *a++ - *b++;
	diff |= *a++ - *b++;

	return (diff);
}
#define	ether_equal !ether_cmp

/*
 * Check for a valid address.  to_bpf is filled in with one of the following:
 *   0 -> BPF doesn't get this packet
 *   1 -> BPF does get this packet
 *   2 -> BPF does get this packet, but we don't
 * Return value is true if the packet is for us, and false otherwise.
 *
 * This routine is a mess, but it's also critical that it be as fast
 * as possible.  It could be made cleaner if we can assume that the
 * only client which will fiddle with IFF_PROMISC is BPF.  This is
 * probably a good assumption, but we do not make it here.  (Yet.)
 */
static inline int 
check_eh(struct ie_softc *sc, struct ether_header *eh, int *to_bpf)
{
#if NBPFILTER > 0
	struct ifnet *ifp;

	ifp = &sc->sc_if;
	*to_bpf = (ifp->if_bpf != 0);
#endif

	/*
	 * This is all handled at a higher level now.
	 */
	return 1;
}

/*
 * We want to isolate the bits that have meaning...  This assumes that
 * IE_RBUF_SIZE is an even power of two.  If somehow the act_len exceeds
 * the size of the buffer, then we are screwed anyway.
 */
static inline int 
ie_buflen(struct ie_softc *sc, int head)
{
	int len;

	len = SWAP(sc->rbuffs[head]->ie_rbd_actual);
	len &= (IE_RBUF_SIZE | (IE_RBUF_SIZE - 1));
	return (len);
}

static inline int 
ie_packet_len(struct ie_softc *sc)
{
	int i;
	int head = sc->rbhead;
	int acc = 0;

	do {
		if (!(sc->rbuffs[sc->rbhead]->ie_rbd_actual & IE_RBD_USED)) {
#ifdef IEDEBUG
			print_rbd(sc->rbuffs[sc->rbhead]);
#endif
			log(LOG_ERR, "%s: receive descriptors out of sync at %d\n",
			    sc->sc_dev.dv_xname, sc->rbhead);
			iereset(sc);
			return -1;
		}

		i = sc->rbuffs[head]->ie_rbd_actual & IE_RBD_LAST;

		acc += ie_buflen(sc, head);
		head = (head + 1) % sc->nrxbuf;
	} while (!i);

	return acc;
}

/*
 * Setup all necessary artifacts for an XMIT command, and then pass the XMIT
 * command to the chip to be executed.  On the way, if we have a BPF listener
 * also give him a copy.
 */
static void 
iexmit(struct ie_softc *sc)
{
	struct ifnet *ifp;

	ifp = &sc->sc_if;

#ifdef IEDEBUG
	if (sc->sc_debug & IED_XMIT)
		printf("%s: xmit buffer %d\n", sc->sc_dev.dv_xname,
		    sc->xctail);
#endif

#if NBPFILTER > 0
	/*
	 * If BPF is listening on this interface, let it see the packet before
	 * we push it on the wire.
	 */
	if (ifp->if_bpf)
		bpf_tap(ifp->if_bpf,
		    sc->xmit_cbuffs[sc->xctail],
		    SWAP(sc->xmit_buffs[sc->xctail]->ie_xmit_flags));
#endif

	sc->xmit_buffs[sc->xctail]->ie_xmit_flags |= IE_XMIT_LAST;
	sc->xmit_buffs[sc->xctail]->ie_xmit_next = SWAP(0xffff);
	sc->xmit_buffs[sc->xctail]->ie_xmit_buf =
	    Swap32(vtop24(sc, sc->xmit_cbuffs[sc->xctail]));

	sc->xmit_cmds[sc->xctail]->com.ie_cmd_link = SWAP(0xffff);
	sc->xmit_cmds[sc->xctail]->com.ie_cmd_cmd =
	    IE_CMD_XMIT | IE_CMD_INTR | IE_CMD_LAST;

	sc->xmit_cmds[sc->xctail]->ie_xmit_status = SWAP(0);
	sc->xmit_cmds[sc->xctail]->ie_xmit_desc =
	    vtop16sw(sc, __UNVOLATILE(sc->xmit_buffs[sc->xctail]));

	sc->scb->ie_command_list = 
	    vtop16sw(sc, __UNVOLATILE(sc->xmit_cmds[sc->xctail]));
	cmd_and_wait(sc, IE_CU_START, 0, 0);

	ifp->if_timer = 5;
}

/*
 * Read data off the interface, and turn it into an mbuf chain.
 *
 * This code is DRAMATICALLY different from the previous version; this
 * version tries to allocate the entire mbuf chain up front, given the
 * length of the data available.  This enables us to allocate mbuf
 * clusters in many situations where before we would have had a long
 * chain of partially-full mbufs.  This should help to speed up the
 * operation considerably.  (Provided that it works, of course.)
 */
static inline struct mbuf *
ieget(struct ie_softc *sc, int *to_bpf)
{
	struct mbuf *top, **mp, *m;
	int len, totlen, resid;
	int thisrboff, thismboff;
	int head;
	struct ether_header eh;

	totlen = ie_packet_len(sc);
	if (totlen <= 0)
		return 0;

	head = sc->rbhead;

	/*
	 * Snarf the Ethernet header.
	 */
	(sc->sc_memcpy)((void *)&eh, (void *)sc->cbuffs[head],
	    sizeof(struct ether_header));

	/*
	 * As quickly as possible, check if this packet is for us.
	 * If not, don't waste a single cycle copying the rest of the
	 * packet in.
	 * This is only a consideration when FILTER is defined; i.e., when
	 * we are either running BPF or doing multicasting.
	 */
	if (!check_eh(sc, &eh, to_bpf)) {
		/* just this case, it's not an error */
		sc->sc_if.if_ierrors--;
		return 0;
	}

	resid = totlen;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;

	m->m_pkthdr.rcvif = &sc->sc_if;
	m->m_pkthdr.len = totlen;
	len = MHLEN;
	top = 0;
	mp = &top;

	/*
	 * This loop goes through and allocates mbufs for all the data we will
	 * be copying in.  It does not actually do the copying yet.
	 */
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
			if (m->m_flags & M_EXT)
				len = MCLBYTES;
		}

		if (mp == &top) {
			char *newdata = (char *)
			    ALIGN(m->m_data + sizeof(struct ether_header)) -
			    sizeof(struct ether_header);
			len -= newdata - m->m_data; 
			m->m_data = newdata;
		}

		m->m_len = len = min(totlen, len);

		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	m = top;
	thismboff = 0;

	/*
	 * Copy the Ethernet header into the mbuf chain.
	 */
	memcpy(mtod(m, void *), &eh, sizeof(struct ether_header));
	thismboff = sizeof(struct ether_header);
	thisrboff = sizeof(struct ether_header);
	resid -= sizeof(struct ether_header);

	/*
	 * Now we take the mbuf chain (hopefully only one mbuf most of the
	 * time) and stuff the data into it.  There are no possible failures
	 * at or after this point.
	 */
	while (resid > 0) {
		int thisrblen = ie_buflen(sc, head) - thisrboff;
		int thismblen = m->m_len - thismboff;

		len = min(thisrblen, thismblen);
		(sc->sc_memcpy)(mtod(m, char *) + thismboff,
		    (void *)(sc->cbuffs[head] + thisrboff),
		    (u_int)len);
		resid -= len;

		if (len == thismblen) {
			m = m->m_next;
			thismboff = 0;
		} else
			thismboff += len;

		if (len == thisrblen) {
			head = (head + 1) % sc->nrxbuf;
			thisrboff = 0;
		} else
			thisrboff += len;
	}

	/*
	 * Unless something changed strangely while we were doing the copy,
	 * we have now copied everything in from the shared memory.
	 * This means that we are done.
	 */
	return top;
}

/*
 * Read frame NUM from unit UNIT (pre-cached as IE).
 *
 * This routine reads the RFD at NUM, and copies in the buffers from
 * the list of RBD, then rotates the RBD and RFD lists so that the receiver
 * doesn't start complaining.  Trailers are DROPPED---there's no point
 * in wasting time on confusing code to deal with them.  Hopefully,
 * this machine will never ARP for trailers anyway.
 */
static void 
ie_readframe(struct ie_softc *sc, int num)
{
	int status;
	struct mbuf *m = 0;
#if NBPFILTER > 0
	int bpf_gets_it = 0;
#endif

	status = sc->rframes[num]->ie_fd_status;

	/* Advance the RFD list, since we're done with this descriptor. */
	sc->rframes[num]->ie_fd_status = SWAP(0);
	sc->rframes[num]->ie_fd_last |= IE_FD_LAST;
	sc->rframes[sc->rftail]->ie_fd_last &= ~IE_FD_LAST;
	sc->rftail = (sc->rftail + 1) % sc->nframes;
	sc->rfhead = (sc->rfhead + 1) % sc->nframes;

	if (status & IE_FD_OK) {
#if NBPFILTER > 0
		m = ieget(sc, &bpf_gets_it);
#else
		m = ieget(sc, NULL);
#endif
		ie_drop_packet_buffer(sc);
	}
	if (m == 0) {
		sc->sc_if.if_ierrors++;
		return;
	}

#ifdef IEDEBUG
	if (sc->sc_debug & IED_READFRAME) {
		struct ether_header *eh = mtod(m, struct ether_header *);

		printf("%s: frame from ether %s type 0x%x\n",
			sc->sc_dev.dv_xname,
		    ether_sprintf(eh->ether_shost), (u_int)eh->ether_type);
	}
#endif

#if NBPFILTER > 0
	/*
	 * Check for a BPF filter; if so, hand it up.
	 * Note that we have to stick an extra mbuf up front, because
	 * bpf_mtap expects to have the ether header at the front.
	 * It doesn't matter that this results in an ill-formatted mbuf chain,
	 * since BPF just looks at the data.  (It doesn't try to free the mbuf,
	 * tho' it will make a copy for tcpdump.)
	 */
	if (bpf_gets_it) {
		/* Pass it up. */
		bpf_mtap(sc->sc_if.if_bpf, m);

		/*
		 * A signal passed up from the filtering code indicating that
		 * the packet is intended for BPF but not for the protocol
		 * machinery.  We can save a few cycles by not handing it off
		 * to them.
		 */
		if (bpf_gets_it == 2) {
			m_freem(m);
			return;
		}
	}
#endif	/* NBPFILTER > 0 */

	/*
	 * In here there used to be code to check destination addresses upon
	 * receipt of a packet.  We have deleted that code, and replaced it
	 * with code to check the address much earlier in the cycle, before
	 * copying the data in; this saves us valuable cycles when operating
	 * as a multicast router or when using BPF.
	 */

	/*
	 * Finally pass this packet up to higher layers.
	 */
	(*sc->sc_if.if_input)(&sc->sc_if, m);
	sc->sc_if.if_ipackets++;
}

static void 
ie_drop_packet_buffer(struct ie_softc *sc)
{
	int i;

	do {
		/*
		 * This means we are somehow out of sync.  So, we reset the
		 * adapter.
		 */
		if (!(sc->rbuffs[sc->rbhead]->ie_rbd_actual & IE_RBD_USED)) {
#ifdef IEDEBUG
			print_rbd(sc->rbuffs[sc->rbhead]);
#endif
			log(LOG_ERR, "%s: receive descriptors out of sync at %d\n",
			    sc->sc_dev.dv_xname, sc->rbhead);
			iereset(sc);
			return;
		}

		i = sc->rbuffs[sc->rbhead]->ie_rbd_actual & IE_RBD_LAST;

		sc->rbuffs[sc->rbhead]->ie_rbd_length |= IE_RBD_LAST;
		sc->rbuffs[sc->rbhead]->ie_rbd_actual = SWAP(0);
		sc->rbhead = (sc->rbhead + 1) % sc->nrxbuf;
		sc->rbuffs[sc->rbtail]->ie_rbd_length &= ~IE_RBD_LAST;
		sc->rbtail = (sc->rbtail + 1) % sc->nrxbuf;
	} while (!i);
}

/*
 * Start transmission on an interface.
 */
static void 
iestart(struct ifnet *ifp)
{
	struct ie_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	u_char *buffer;
	u_short len;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		if (sc->xmit_busy == sc->ntxbuf) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IF_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == 0)
			break;

		/* We need to use m->m_pkthdr.len, so require the header */
		if ((m0->m_flags & M_PKTHDR) == 0)
			panic("iestart: no header mbuf");

#if NBPFILTER > 0
		/* Tap off here if there is a BPF listener. */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif

#ifdef IEDEBUG
		if (sc->sc_debug & IED_ENQ)
			printf("%s: fill buffer %d\n", sc->sc_dev.dv_xname,
			    sc->xchead);
#endif

		buffer = sc->xmit_cbuffs[sc->xchead];
		for (m = m0; m != 0; m = m->m_next) {
			(sc->sc_memcpy)(buffer, mtod(m, void *), m->m_len);
			buffer += m->m_len;
		}
		if (m0->m_pkthdr.len < ETHER_MIN_LEN - ETHER_CRC_LEN) {
			sc->sc_memset(buffer, 0,
			    ETHER_MIN_LEN - ETHER_CRC_LEN - m0->m_pkthdr.len);
			len = ETHER_MIN_LEN - ETHER_CRC_LEN;
		} else
			len = m0->m_pkthdr.len;

		m_freem(m0);
		sc->xmit_buffs[sc->xchead]->ie_xmit_flags = SWAP(len);

		/* Start the first packet transmitting. */
		if (sc->xmit_busy == 0)
			iexmit(sc);

		sc->xchead = (sc->xchead + 1) % sc->ntxbuf;
		sc->xmit_busy++;
	}
}

static void 
iereset(struct ie_softc *sc)
{
	int s;

	s = splnet();

	/* No message here.  The caller does that. */
	iestop(sc);

	/*
	 * Stop i82586 dead in its tracks.
	 */
	if (cmd_and_wait(sc, IE_RU_ABORT | IE_CU_ABORT, 0, 0))
		printf("%s: abort commands timed out\n", sc->sc_dev.dv_xname);

	if (cmd_and_wait(sc, IE_RU_DISABLE | IE_CU_STOP, 0, 0))
		printf("%s: disable commands timed out\n", sc->sc_dev.dv_xname);

	ieinit(sc);

	splx(s);
}

/*
 * Send a command to the controller and wait for it to either
 * complete or be accepted, depending on the command.  If the
 * command pointer is null, then pretend that the command is
 * not an action command.  If the command pointer is not null,
 * and the command is an action command, wait for
 * ((volatile struct ie_cmd_common *)pcmd)->ie_cmd_status & MASK
 * to become true.
 */
static int 
cmd_and_wait(struct ie_softc *sc, int cmd, void *pcmd, int mask)
{
	volatile struct ie_cmd_common *cc = pcmd;
	volatile struct ie_sys_ctl_block *scb = sc->scb;
	int tmo;

	scb->ie_command = (u_short)cmd;
	(sc->chan_attn)(sc);

	/* Wait for the command to be accepted by the CU. */
	tmo = 10;
	while (scb->ie_command && --tmo)
		delay(10);
	if (scb->ie_command) {
#ifdef	IEDEBUG
		printf("%s: cmd_and_wait, CU stuck (1)\n",
		    sc->sc_dev.dv_xname);
#endif
		return -1;	/* timed out */
	}

	/*
	 * If asked, also wait for it to finish.
	 */
	if (IE_ACTION_COMMAND(cmd) && pcmd) {

		/*
		 * According to the packet driver, the minimum timeout should
		 * be .369 seconds, which we round up to .4.
		 */
		tmo = 36900;

		/*
		 * Now spin-lock waiting for status.  This is not a very nice
		 * thing to do, but I haven't figured out how, or indeed if, we
		 * can put the process waiting for action to sleep.  (We may
		 * be getting called through some other timeout running in the
		 * kernel.)
		 */
		while (((cc->ie_cmd_status & mask) == 0) && --tmo)
			delay(10);

		if ((cc->ie_cmd_status & mask) == 0) {
#ifdef	IEDEBUG
			printf("%s: cmd_and_wait, CU stuck (2)\n",
				   sc->sc_dev.dv_xname);
#endif
			return -1;	/* timed out */
		}
	}
	return 0;
}

/*
 * Run the time-domain reflectometer.
 */
static void 
run_tdr(struct ie_softc *sc, struct ie_tdr_cmd *cmd)
{
	int result;

	cmd->com.ie_cmd_status = SWAP(0);
	cmd->com.ie_cmd_cmd = IE_CMD_TDR | IE_CMD_LAST;
	cmd->com.ie_cmd_link = SWAP(0xffff);

	sc->scb->ie_command_list = vtop16sw(sc, cmd);
	cmd->ie_tdr_time = SWAP(0);

	if (cmd_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
	    !(cmd->com.ie_cmd_status & IE_STAT_OK))
		result = 0x10000;	/* impossible value */
	else
		result = cmd->ie_tdr_time;

	ie_ack(sc, IE_ST_WHENCE);

	if (result & IE_TDR_SUCCESS)
		return;

	if (result & 0x10000) {
		printf("%s: TDR command failed\n", sc->sc_dev.dv_xname);
	} else if (result & IE_TDR_XCVR) {
		printf("%s: transceiver problem\n", sc->sc_dev.dv_xname);
	} else if (result & IE_TDR_OPEN) {
		printf("%s: TDR detected an open %d clocks away\n",
		    sc->sc_dev.dv_xname, SWAP(result & IE_TDR_TIME));
	} else if (result & IE_TDR_SHORT) {
		printf("%s: TDR detected a short %d clocks away\n",
		    sc->sc_dev.dv_xname, SWAP(result & IE_TDR_TIME));
	} else {
		printf("%s: TDR returned unknown status 0x%x\n",
		    sc->sc_dev.dv_xname, result);
	}
}

/*
 * iememinit: set up the buffers
 *
 * we have a block of KVA at sc->buf_area which is of size sc->buf_area_sz.
 * this is to be used for the buffers.  the chip indexs its control data
 * structures with 16 bit offsets, and it indexes actual buffers with
 * 24 bit addresses.   so we should allocate control buffers first so that
 * we don't overflow the 16 bit offset field.   The number of transmit
 * buffers is fixed at compile time.
 *
 * note: this function was written to be easy to understand, rather than
 *       highly efficient (it isn't in the critical path).
 *
 * The memory layout is: tbufs, rbufs, (gap), control blocks
 * [tbuf0, tbuf1] [rbuf0,...rbufN] gap [rframes] [tframes]
 * XXX - This needs review...
 */
static void 
iememinit(struct ie_softc *sc)
{
	char *ptr;
	int i;
	u_short nxt;

	/* First, zero all the memory. */
	ptr = sc->buf_area;
	(sc->sc_memset)(ptr, 0, sc->buf_area_sz);

	/* Allocate tx/rx buffers. */
	for (i = 0; i < NTXBUF; i++) {
		sc->xmit_cbuffs[i] = ptr;
		ptr += IE_TBUF_SIZE;
	}
	for (i = 0; i < sc->nrxbuf; i++) {
		sc->cbuffs[i] = ptr;
		ptr += IE_RBUF_SIZE;
	}

	/* Small pad (Don't trust the chip...) */
	ptr += 16;

	/* Allocate and fill in xmit buffer descriptors. */
	for (i = 0; i < NTXBUF; i++) {
		sc->xmit_buffs[i] = (volatile void *) ptr;
		ptr = Align(ptr + sizeof(*sc->xmit_buffs[i]));
		sc->xmit_buffs[i]->ie_xmit_buf =
		    Swap32(vtop24(sc, sc->xmit_cbuffs[i]));
		sc->xmit_buffs[i]->ie_xmit_next = SWAP(0xffff);
	}

	/* Allocate and fill in recv buffer descriptors. */
	for (i = 0; i < sc->nrxbuf; i++) {
		sc->rbuffs[i] = (volatile void *) ptr;
		ptr = Align(ptr + sizeof(*sc->rbuffs[i]));
		sc->rbuffs[i]->ie_rbd_buffer = 
			Swap32(vtop24(sc, sc->cbuffs[i]));
		sc->rbuffs[i]->ie_rbd_length = SWAP(IE_RBUF_SIZE);
	}

	/* link together recv bufs and set EOL on last */
	i = sc->nrxbuf - 1;
	sc->rbuffs[i]->ie_rbd_length |= IE_RBD_LAST;
	nxt = vtop16sw(sc, __UNVOLATILE(sc->rbuffs[0]));
	do {
		sc->rbuffs[i]->ie_rbd_next = nxt;
		nxt = vtop16sw(sc, __UNVOLATILE(sc->rbuffs[i]));
	} while (--i >= 0);

	/* Allocate transmit commands. */
	for (i = 0; i < NTXBUF; i++) {
		sc->xmit_cmds[i] = (volatile void *) ptr;
		ptr = Align(ptr + sizeof(*sc->xmit_cmds[i]));
		sc->xmit_cmds[i]->com.ie_cmd_link = SWAP(0xffff);
	}

	/* Allocate receive frames. */
	for (i = 0; i < sc->nframes; i++) {
		sc->rframes[i] = (volatile void *) ptr;
		ptr = Align(ptr + sizeof(*sc->rframes[i]));
	}

	/* Link together recv frames and set EOL on last */
	i = sc->nframes - 1;
	sc->rframes[i]->ie_fd_last |= IE_FD_LAST;
	nxt = vtop16sw(sc, __UNVOLATILE(sc->rframes[0]));
	do {
		sc->rframes[i]->ie_fd_next = nxt;
		nxt = vtop16sw(sc, __UNVOLATILE(sc->rframes[i]));
	} while (--i >= 0);


	/* Pointers to last packet sent and next available transmit buffer. */
	sc->xchead = sc->xctail = 0;

	/* Clear transmit-busy flag. */
	sc->xmit_busy = 0;

	/*
	 * Set the head and tail pointers on receive to keep track of
	 * the order in which RFDs and RBDs are used.   link the
	 * recv frames and buffer into the scb.
	 */
	sc->rfhead = 0;
	sc->rftail = sc->nframes - 1;
	sc->rbhead = 0;
	sc->rbtail = sc->nrxbuf - 1;

	sc->scb->ie_recv_list =
	    vtop16sw(sc, __UNVOLATILE(sc->rframes[0]));
	sc->rframes[0]->ie_fd_buf_desc =
	    vtop16sw(sc, __UNVOLATILE(sc->rbuffs[0]));

	i = (ptr - sc->buf_area);
#ifdef IEDEBUG
	printf("IE_DEBUG: used %d of %d bytes\n", i, sc->buf_area_sz);
#endif
	if (i > sc->buf_area_sz)
		panic("ie: iememinit, out of space");
}

/*
 * Run the multicast setup command.
 * Called at splnet().
 */
static int 
mc_setup(struct ie_softc *sc, void *ptr)
{
	struct ie_mcast_cmd *cmd = ptr;	/* XXX - Was volatile */

	cmd->com.ie_cmd_status = SWAP(0);
	cmd->com.ie_cmd_cmd = IE_CMD_MCAST | IE_CMD_LAST;
	cmd->com.ie_cmd_link = SWAP(0xffff);

	(sc->sc_memcpy)((void *)cmd->ie_mcast_addrs,
	    (void *)sc->mcast_addrs,
	    sc->mcast_count * sizeof *sc->mcast_addrs);

	cmd->ie_mcast_bytes =
		SWAP(sc->mcast_count * ETHER_ADDR_LEN);	/* grrr... */

	sc->scb->ie_command_list = vtop16sw(sc, cmd);
	if (cmd_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
	    !(cmd->com.ie_cmd_status & IE_STAT_OK)) {
		printf("%s: multicast address setup command failed\n",
		    sc->sc_dev.dv_xname);
		return 0;
	}
	return 1;
}

static inline void 
ie_setup_config(struct ie_config_cmd *cmd, int promiscuous, int manchester)
{

	/*
	 * these are all char's so no need to byte-swap
	 */
	cmd->ie_config_count = 0x0c;
	cmd->ie_fifo = 8;
	cmd->ie_save_bad = 0x40;
	cmd->ie_addr_len = 0x2e;
	cmd->ie_priority = 0;
	cmd->ie_ifs = 0x60;
	cmd->ie_slot_low = 0;
	cmd->ie_slot_high = 0xf2;
	cmd->ie_promisc = promiscuous | manchester << 2;
	cmd->ie_crs_cdt = 0;
	cmd->ie_min_len = 64;
	cmd->ie_junk = 0xff;
}

/*
 * This routine inits the ie.
 * This includes executing the CONFIGURE, IA-SETUP, and MC-SETUP commands,
 * starting the receiver unit, and clearing interrupts.
 *
 * THIS ROUTINE MUST BE CALLED AT splnet() OR HIGHER.
 */
static int 
ieinit(struct ie_softc *sc)
{
	volatile struct ie_sys_ctl_block *scb = sc->scb;
	void *ptr;
	struct ifnet *ifp;

	ifp = &sc->sc_if;
	ptr = sc->buf_area;	/* XXX - Use scb instead? */

	/*
	 * Send the configure command first.
	 */
	{
		struct ie_config_cmd *cmd = ptr;	/* XXX - Was volatile */

		scb->ie_command_list = vtop16sw(sc, cmd);
		cmd->com.ie_cmd_status = SWAP(0);
		cmd->com.ie_cmd_cmd = IE_CMD_CONFIG | IE_CMD_LAST;
		cmd->com.ie_cmd_link = SWAP(0xffff);

		ie_setup_config(cmd, (sc->promisc != 0), 0);

		if (cmd_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
		    !(cmd->com.ie_cmd_status & IE_STAT_OK)) {
			printf("%s: configure command failed\n",
			    sc->sc_dev.dv_xname);
			return 0;
		}
	}

	/*
	 * Now send the Individual Address Setup command.
	 */
	{
		struct ie_iasetup_cmd *cmd = ptr;	/* XXX - Was volatile */

		scb->ie_command_list = vtop16sw(sc, cmd);
		cmd->com.ie_cmd_status = SWAP(0);
		cmd->com.ie_cmd_cmd = IE_CMD_IASETUP | IE_CMD_LAST;
		cmd->com.ie_cmd_link = SWAP(0xffff);

		(sc->sc_memcpy)((void *)&cmd->ie_address,
		    LLADDR(ifp->if_sadl), sizeof(cmd->ie_address));

		if (cmd_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
		    !(cmd->com.ie_cmd_status & IE_STAT_OK)) {
			printf("%s: individual address setup command failed\n",
			    sc->sc_dev.dv_xname);
			return 0;
		}
	}

	/*
	 * Now run the time-domain reflectometer.
	 */
	if (ie_run_tdr)
		run_tdr(sc, ptr);

	/*
	 * Acknowledge any interrupts we have generated thus far.
	 */
	ie_ack(sc, IE_ST_WHENCE);

	/*
	 * Set up the transmit and recv buffers.
	 */
	iememinit(sc);

	/* tell higher levels that we are here */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	sc->scb->ie_recv_list =
	    vtop16sw(sc, __UNVOLATILE(sc->rframes[0]));
	cmd_and_wait(sc, IE_RU_START, 0, 0);

	ie_ack(sc, IE_ST_WHENCE);

	if (sc->run_586)
		(sc->run_586)(sc);

	return 0;
}

static void 
iestop(struct ie_softc *sc)
{

	cmd_and_wait(sc, IE_RU_DISABLE, 0, 0);
}

static int 
ieioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ie_softc *sc = ifp->if_softc;
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
			ieinit(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		/* XXX - This code is probably wrong. */
		case AF_NS:
		    {
			struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				memcpy(LLADDR(ifp->if_sadl),
				    ina->x_host.c_host, ETHER_ADDR_LEN);
			/* Set new address. */
			ieinit(sc);
			break;
		    }
#endif /* NS */
		default:
			ieinit(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		sc->promisc = ifp->if_flags & (IFF_PROMISC | IFF_ALLMULTI);

		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			iestop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			(ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			ieinit(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			iestop(sc);
			ieinit(sc);
		}
#ifdef IEDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_debug = IED_ALL;
		else
			sc->sc_debug = ie_debug_flags;
#endif
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
			if (ifp->if_flags & IFF_RUNNING)
				mc_reset(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
	}
	splx(s);
	return error;
}

static void 
mc_reset(struct ie_softc *sc)
{
	struct ether_multi *enm;
	struct ether_multistep step;
	struct ifnet *ifp;

	ifp = &sc->sc_if;

	/*
	 * Step through the list of addresses.
	 */
	sc->mcast_count = 0;
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm) {
		if (sc->mcast_count >= MAXMCAST ||
		    ether_cmp(enm->enm_addrlo, enm->enm_addrhi) != 0) {
			ifp->if_flags |= IFF_ALLMULTI;
			ieioctl(ifp, SIOCSIFFLAGS, (void *)0);
			goto setflag;
		}
		memcpy(&sc->mcast_addrs[sc->mcast_count], enm->enm_addrlo,
		    ETHER_ADDR_LEN);
		sc->mcast_count++;
		ETHER_NEXT_MULTI(step, enm);
	}
setflag:
	sc->want_mcsetup = 1;
}

#ifdef IEDEBUG
void 
print_rbd(volatile struct ie_recv_buf_desc *rbd)
{

	printf("RBD at %08lx:\nactual %04x, next %04x, buffer %08x\n"
	    "length %04x, mbz %04x\n", (u_long)rbd, rbd->ie_rbd_actual,
	    rbd->ie_rbd_next, rbd->ie_rbd_buffer, rbd->ie_rbd_length,
	    rbd->mbz);
}
#endif
