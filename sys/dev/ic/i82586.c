/*	$NetBSD: i82586.c,v 1.5 1997/07/29 20:24:47 pk Exp $	*/

/*-
 * Copyright (c) 1997 Paul Kranenburg.
 * Copyright (c) 1993, 1994, 1995 Charles Hannum.
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
 *	This product includes software developed by Charles Hannum, by the
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

#include <machine/bus.h>

#include <dev/ic/i82586reg.h>
#include <dev/ic/i82586var.h>

void iewatchdog __P((struct ifnet *));
int ieinit __P((struct ie_softc *));
int ieioctl __P((struct ifnet *, u_long, caddr_t));
void iestart __P((struct ifnet *));
void iereset __P((struct ie_softc *));
static void ie_readframe __P((struct ie_softc *, int));
static void ie_drop_packet_buffer __P((struct ie_softc *));
int ie_setupram __P((struct ie_softc *));
static int command_and_wait __P((struct ie_softc *, int,
    void volatile *, int));
/*static*/ void ierint __P((struct ie_softc *));
/*static*/ void ietint __P((struct ie_softc *));
static struct mbuf *ieget __P((struct ie_softc *,
		      struct ether_header *, int *));
static void setup_bufs __P((struct ie_softc *));
static int mc_setup __P((struct ie_softc *, void *));
static void mc_reset __P((struct ie_softc *));
static __inline int ether_equal __P((u_char *, u_char *));
static __inline void ie_ack __P((struct ie_softc *, u_int));
static __inline void ie_setup_config __P((volatile struct ie_config_cmd *,
					  int, int));
static __inline int check_eh __P((struct ie_softc *, struct ether_header *,
				  int *));
static __inline int ie_buflen __P((struct ie_softc *, int));
static __inline int ie_packet_len __P((struct ie_softc *));
static __inline void iexmit __P((struct ie_softc *));

static void run_tdr __P((struct ie_softc *, struct ie_tdr_cmd *));
static void iestop __P((struct ie_softc *));

#ifdef IEDEBUG
void print_rbd __P((volatile struct ie_recv_buf_desc *));

int in_ierint = 0;
int in_ietint = 0;
#endif

struct cfdriver ie_cd = {
	NULL, "ie", DV_IFNET
};

/*
 * Address generation macros:
 *   MK_24 = KVA -> 24 bit address in native byte order
 *   MK_16 = KVA -> 16 bit address in INTEL byte order
 *   ST_24 = store a 24 bit address in native byte order to INTEL byte order
 */
#define MK_24(base, ptr) ((caddr_t)((u_long)ptr - (u_long)base))

#if BYTE_ORDER == BIG_ENDIAN
#define XSWAP(y)	( ((y) >> 8) | ((y) << 8) )
#define SWAP(x)		({u_short _z=(x); (u_short)XSWAP(_z);})

#define MK_16(base, ptr) SWAP((u_short)( ((u_long)(ptr)) - ((u_long)(base)) ))
#define ST_24(to, from) { \
	u_long fval = (u_long)(from); \
	u_char *t = (u_char *)&(to), *f = (u_char *)&fval; \
	t[0] = f[3]; t[1] = f[2]; t[2] = f[1]; /*t[3] = f[0] ;*/ \
}
#else
#define SWAP(x) x
#define MK_16(base, ptr) ((u_short)(u_long)MK_24(base, ptr))
#define ST_24(to, from) {to = (from);}
#endif

/*
 * Here are a few useful functions.  We could have done these as macros, but
 * since we have the inline facility, it makes sense to use that instead.
 */
static __inline void
ie_setup_config(cmd, promiscuous, manchester)
	volatile struct ie_config_cmd *cmd;
	int promiscuous, manchester;
{

	cmd->ie_config_count = 0x0c;
	cmd->ie_fifo = 8;
	cmd->ie_save_bad = 0x40;
	cmd->ie_addr_len = 0x2e;
	cmd->ie_priority = 0;
	cmd->ie_ifs = 0x60;
	cmd->ie_slot_low = 0;
	cmd->ie_slot_high = 0xf2;
	cmd->ie_promisc = !!promiscuous | manchester << 2;
	cmd->ie_crs_cdt = 0;
	cmd->ie_min_len = 64;
	cmd->ie_junk = 0xff;
}

static __inline void
ie_ack(sc, mask)
	struct ie_softc *sc;
	u_int mask;	/* in native byte-order */
{
	volatile struct ie_sys_ctl_block *scb = sc->scb;

	command_and_wait(sc, SWAP(scb->ie_status) & mask, 0, 0);
}


/*
 * Taken almost exactly from Bill's if_is.c, then modified beyond recognition.
 */
void
ie_attach(sc, name, etheraddr)
	struct ie_softc *sc;
	char *name;
	u_int8_t *etheraddr;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	if (ie_setupram(sc) == 0) { /* XXX - ISA version? */
		printf(": RAM CONFIG FAILED!\n");
		/* XXX should reclaim resources? */
		return;
	}

	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = iestart;
	ifp->if_ioctl = ieioctl;
	ifp->if_watchdog = iewatchdog;
	ifp->if_flags =
		IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, etheraddr);

	printf(" address %s, type %s\n", ether_sprintf(etheraddr), name);

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
}


/*
 * Device timeout/watchdog routine.  Entered if the device neglects to generate
 * an interrupt after a transmit has been started on it.
 */
void
iewatchdog(ifp)
	struct ifnet *ifp;
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
ieintr(v)
void *v;
{
	struct ie_softc *sc = v;
	register u_short status;

	bus_space_barrier(sc->bt, sc->bh, 0, 0, BUS_SPACE_BARRIER_READ);
	status = SWAP(sc->scb->ie_status);

        /*
         * Implementation dependent interrupt handling.
         */
	if (sc->intrhook)
		(*sc->intrhook)(sc);

loop:
	/* Ack interrupts FIRST in case we receive more during the ISR. */
	ie_ack(sc, IE_ST_WHENCE & status);

	if (status & (IE_ST_FR | IE_ST_RNR)) {
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

	if (status & IE_ST_CX) {
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

	if (status & IE_ST_RNR) {
		printf("%s: receiver not ready\n", sc->sc_dev.dv_xname);
		sc->sc_ethercom.ec_if.if_ierrors++;
		iereset(sc);
		return (1);
	}

#ifdef IEDEBUG
	if ((status & IE_ST_CNA) && (sc->sc_debug & IED_CNA))
		printf("%s: cna\n", sc->sc_dev.dv_xname);
#endif

	bus_space_barrier(sc->bt, sc->bh, 0, 0, BUS_SPACE_BARRIER_READ);
	status = SWAP(sc->scb->ie_status);
	if (status & IE_ST_WHENCE)
		goto loop;

	return (1);
}

/*
 * Process a received-frame interrupt.
 */
void
ierint(sc)
	struct ie_softc *sc;
{
	volatile struct ie_sys_ctl_block *scb = sc->scb;
	int i, status;
	static int timesthru = 1024;

	i = sc->rfhead;
	for (;;) {
		status = SWAP(sc->rframes[i]->ie_fd_status);

		if ((status & IE_FD_COMPLETE) && (status & IE_FD_OK)) {
			if (--timesthru == 0) {
				sc->sc_ethercom.ec_if.if_ierrors +=
				    SWAP(scb->ie_err_crc) +
				    SWAP(scb->ie_err_align) +
				    SWAP(scb->ie_err_resource) +
				    SWAP(scb->ie_err_overrun);
				scb->ie_err_crc = scb->ie_err_align =
				scb->ie_err_resource = scb->ie_err_overrun =
				    SWAP(0);
				timesthru = 1024;
			}
			ie_readframe(sc, i);
		} else {
			if ((status & IE_FD_RNR) != 0 &&
			    (SWAP(scb->ie_status) & IE_RU_READY) == 0) {
				sc->rframes[0]->ie_fd_buf_desc =
					MK_16(sc->sc_maddr, sc->rbuffs[0]);
				scb->ie_recv_list =
					MK_16(sc->sc_maddr, sc->rframes[0]);
				command_and_wait(sc, IE_RU_START, 0, 0);
			}
			break;
		}
		i = (i + 1) % sc->nframes;
	}
}

/*
 * Process a command-complete interrupt.  These are only generated by the
 * transmission of frames.  This routine is deceptively simple, since most of
 * the real work is done by iestart().
 */
void
ietint(sc)
	struct ie_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int status;

	ifp->if_timer = 0;
	ifp->if_flags &= ~IFF_OACTIVE;

	status = SWAP(sc->xmit_cmds[sc->xctail]->ie_xmit_status);

	if ((status & IE_STAT_COMPL) == 0 || (status & IE_STAT_BUSY))
		printf("ietint: command still busy!\n");

	if (status & IE_STAT_OK) {
		ifp->if_opackets++;
		ifp->if_collisions += (status & IE_XS_MAXCOLL);
	} else {
		ifp->if_oerrors++;
		/*
		 * Check SQE and DEFERRED?
		 * What if more than one bit is set?
		 */
		if (status & IE_STAT_ABORT)
			printf("%s: send aborted\n", sc->sc_dev.dv_xname);
		else if (status & IE_XS_NOCARRIER)
			printf("%s: no carrier\n", sc->sc_dev.dv_xname);
		else if (status & IE_XS_LOSTCTS)
			printf("%s: lost CTS\n", sc->sc_dev.dv_xname);
		else if (status & IE_XS_UNDERRUN)
			printf("%s: DMA underrun\n", sc->sc_dev.dv_xname);
		else if (status & IE_XS_EXCMAX) {
			printf("%s: too many collisions\n",
				sc->sc_dev.dv_xname);
			sc->sc_ethercom.ec_if.if_collisions += 16;
		}
	}

	/*
	 * If multicast addresses were added or deleted while transmitting,
	 * mc_reset() set the want_mcsetup flag indicating that we should do
	 * it.
	 */
	if (sc->want_mcsetup) {
		mc_setup(sc, (caddr_t)sc->xmit_cbuffs[sc->xctail]);
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
 * Compare two Ether/802 addresses for equality, inlined and unrolled for
 * speed.
 */
static __inline int
ether_equal(one, two)
	u_char *one, *two;
{

	if (one[5] != two[5] || one[4] != two[4] || one[3] != two[3] ||
	    one[2] != two[2] || one[1] != two[1] || one[0] != two[0])
		return 0;
	return 1;
}

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
static __inline int
check_eh(sc, eh, to_bpf)
	struct ie_softc *sc;
	struct ether_header *eh;
	int *to_bpf;
{
	struct ifnet *ifp;
	int i;

	ifp = &sc->sc_ethercom.ec_if;

	switch(sc->promisc) {
	case IFF_ALLMULTI:
		/*
		 * Receiving all multicasts, but no unicasts except those
		 * destined for us.
		 */
#if NBPFILTER > 0
		/* BPF gets this packet if anybody cares */
		*to_bpf = (ifp->if_bpf != 0);
#endif
		if (eh->ether_dhost[0] & 1)
			return 1;
		if (ether_equal(eh->ether_dhost, LLADDR(ifp->if_sadl)))
			return 1;
		return 0;

	case IFF_PROMISC:
		/*
		 * Receiving all packets.  These need to be passed on to BPF.
		 */
#if NBPFILTER > 0
		*to_bpf = (ifp->if_bpf != 0);
#endif
		/* If for us, accept and hand up to BPF */
		if (ether_equal(eh->ether_dhost, LLADDR(ifp->if_sadl)))
			return 1;

#if NBPFILTER > 0
		if (*to_bpf)
			*to_bpf = 2; /* we don't need to see it */
#endif

		/*
		 * Not a multicast, so BPF wants to see it but we don't.
		 */
		if ((eh->ether_dhost[0] & 1) == 0)
			return 1;

		/*
		 * If it's one of our multicast groups, accept it and pass it
		 * up.
		 */
		for (i = 0; i < sc->mcast_count; i++) {
			if (ether_equal(eh->ether_dhost,
					(u_char *)&sc->mcast_addrs[i])) {
#if NBPFILTER > 0
				if (*to_bpf)
					*to_bpf = 1;
#endif
				return 1;
			}
		}
		return 1;

	case IFF_ALLMULTI | IFF_PROMISC:
		/*
		 * Acting as a multicast router, and BPF running at the same
		 * time.  Whew!  (Hope this is a fast machine...)
		 */
#if NBPFILTER > 0
		*to_bpf = (ifp->if_bpf != 0);
#endif
		/* We want to see multicasts. */
		if (eh->ether_dhost[0] & 1)
			return 1;

		/* We want to see our own packets */
		if (ether_equal(eh->ether_dhost, LLADDR(ifp->if_sadl)))
			return 1;

		/* Anything else goes to BPF but nothing else. */
#if NBPFILTER > 0
		if (*to_bpf)
			*to_bpf = 2;
#endif
		return 1;

	default:
		/*
		 * Only accept unicast packets destined for us, or multicasts
		 * for groups that we belong to.  For now, we assume that the
		 * '586 will only return packets that we asked it for.  This
		 * isn't strictly true (it uses hashing for the multicast
		 * filter), but it will do in this case, and we want to get
		 * out of here as quickly as possible.
		 */
#if NBPFILTER > 0
		*to_bpf = (ifp->if_bpf != 0);
#endif
		return 1;
	}
	return 0;
}

/*
 * We want to isolate the bits that have meaning...  This assumes that
 * IE_RBUF_SIZE is an even power of two.  If somehow the act_len exceeds
 * the size of the buffer, then we are screwed anyway.
 */
static __inline int
ie_buflen(sc, head)
	struct ie_softc *sc;
	int head;
{

	return (SWAP(sc->rbuffs[head]->ie_rbd_actual)
		& (IE_RBUF_SIZE | (IE_RBUF_SIZE - 1)));
}


static __inline int
ie_packet_len(sc)
	struct ie_softc *sc;
{
	int i;
	int head = sc->rbhead;
	int acc = 0;
	int oldhead = head;

	do {
		bus_space_barrier(sc->bt, sc->bh, 0, 0, BUS_SPACE_BARRIER_READ);
		i = SWAP(sc->rbuffs[head]->ie_rbd_actual);
		if ((i & IE_RBD_USED) == 0) {
#ifdef IEDEBUG
			print_rbd(sc->rbuffs[head]);
#endif
			log(LOG_ERR, "%s: receive descriptors out of sync at %d\n",
			    sc->sc_dev.dv_xname, sc->rbhead);
			iereset(sc);
			return -1;
		}

		i = (i & IE_RBD_LAST) != 0;

		acc += ie_buflen(sc, head);
		head = (head + 1) % sc->nrxbuf;
		if (oldhead == head) {
			printf("ie: packet len: looping: acc = %d (head=%d)\n",
				acc, head);
			iereset(sc);
			return -1;
		}
	} while (!i);

	return acc;
}

/*
 * Setup all necessary artifacts for an XMIT command, and then pass the XMIT
 * command to the chip to be executed.  On the way, if we have a BPF listener
 * also give him a copy.
 */
static __inline void
iexmit(sc)
	struct ie_softc *sc;
{

#ifdef IEDEBUG
	if (sc->sc_debug & IED_XMIT)
		printf("%s: xmit buffer %d\n", sc->sc_dev.dv_xname,
			sc->xctail);
#endif

	sc->xmit_buffs[sc->xctail]->ie_xmit_flags |= SWAP(IE_XMIT_LAST);
	sc->xmit_buffs[sc->xctail]->ie_xmit_next = SWAP(0xffff);
	ST_24(sc->xmit_buffs[sc->xctail]->ie_xmit_buf,
	      MK_24(sc->sc_iobase, sc->xmit_cbuffs[sc->xctail]));

	sc->xmit_cmds[sc->xctail]->com.ie_cmd_link = SWAP(0xffff);
	sc->xmit_cmds[sc->xctail]->com.ie_cmd_cmd =
		SWAP(IE_CMD_XMIT | IE_CMD_INTR | IE_CMD_LAST);

	sc->xmit_cmds[sc->xctail]->ie_xmit_status = SWAP(0);
	sc->xmit_cmds[sc->xctail]->ie_xmit_desc =
		MK_16(sc->sc_maddr, sc->xmit_buffs[sc->xctail]);

	sc->scb->ie_command_list =
		MK_16(sc->sc_maddr, sc->xmit_cmds[sc->xctail]);

	command_and_wait(sc, IE_CU_START, 0, 0);

	sc->sc_ethercom.ec_if.if_timer = 5;
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
struct mbuf *
ieget(sc, ehp, to_bpf)
	struct ie_softc *sc;
	struct ether_header *ehp;
	int *to_bpf;
{
	struct mbuf *top, **mp, *m;
	int len, totlen, resid;
	int thisrboff, thismboff;
	int head;

	totlen = ie_packet_len(sc);
	if (totlen <= 0)
		return 0;

	head = sc->rbhead;

	/*
	 * Snarf the Ethernet header.
	 */
	(sc->memcopy)((caddr_t)sc->cbuffs[head], (caddr_t)ehp, sizeof *ehp);

	/*
	 * As quickly as possible, check if this packet is for us.
	 * If not, don't waste a single cycle copying the rest of the
	 * packet in.
	 * This is only a consideration when FILTER is defined; i.e., when
	 * we are either running BPF or doing multicasting.
	 */
	if (!check_eh(sc, ehp, to_bpf)) {
		/* just this case, it's not an error */
		sc->sc_ethercom.ec_if.if_ierrors--;
		return 0;
	}

	resid = totlen -= (thisrboff = sizeof *ehp);

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m->m_pkthdr.rcvif = &sc->sc_ethercom.ec_if;
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
			if ((m->m_flags & M_EXT) == 0) {
				m_freem(top);
				return 0;
			}
			len = MCLBYTES;
		}
		m->m_len = len = min(totlen, len);
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	m = top;
	thismboff = 0;

	/*
	 * Now we take the mbuf chain (hopefully only one mbuf most of the
	 * time) and stuff the data into it.  There are no possible failures at
	 * or after this point.
	 */
	while (resid > 0) {
		int thisrblen = ie_buflen(sc, head) - thisrboff,
		    thismblen = m->m_len - thismboff;
		len = min(thisrblen, thismblen);

		(sc->memcopy)((caddr_t)(sc->cbuffs[head] + thisrboff),
			       mtod(m, caddr_t) + thismboff, (u_int)len);
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
	 * Unless something changed strangely while we were doing the copy, we
	 * have now copied everything in from the shared memory.
	 * This means that we are done.
	 */
	return top;
}

/*
 * Read frame NUM from unit UNIT (pre-cached as IE).
 *
 * This routine reads the RFD at NUM, and copies in the buffers from the list
 * of RBD, then rotates the RBD and RFD lists so that the receiver doesn't
 * start complaining.  Trailers are DROPPED---there's no point in wasting time
 * on confusing code to deal with them.  Hopefully, this machine will never ARP
 * for trailers anyway.
 */
static void
ie_readframe(sc, num)
	struct ie_softc *sc;
	int num;			/* frame number to read */
{
	int status;
	struct mbuf *m = 0;
	struct ether_header eh;
#if NBPFILTER > 0
	int bpf_gets_it = 0;
#endif

	status = SWAP(sc->rframes[num]->ie_fd_status);

	/* Immediately advance the RFD list, since we have copied ours now. */
	sc->rframes[num]->ie_fd_status = SWAP(0);
	sc->rframes[num]->ie_fd_last |= SWAP(IE_FD_LAST);
	sc->rframes[sc->rftail]->ie_fd_last &= ~SWAP(IE_FD_LAST);
	sc->rftail = (sc->rftail + 1) % sc->nframes;
	sc->rfhead = (sc->rfhead + 1) % sc->nframes;

	if (status & IE_FD_OK) {
#if NBPFILTER > 0
		m = ieget(sc, &eh, &bpf_gets_it);
#else
		m = ieget(sc, &eh, 0);
#endif
		ie_drop_packet_buffer(sc);
	}
	if (m == 0) {
		sc->sc_ethercom.ec_if.if_ierrors++;
		return;
	}

#ifdef IEDEBUG
	if (sc->sc_debug & IED_READFRAME)
		printf("%s: frame from ether %s type 0x%x\n",
			sc->sc_dev.dv_xname,
			ether_sprintf(eh.ether_shost), (u_int)eh.ether_type);
#endif

#if NBPFILTER > 0
	/*
	 * Check for a BPF filter; if so, hand it up.
	 * Note that we have to stick an extra mbuf up front, because bpf_mtap
	 * expects to have the ether header at the front.
	 * It doesn't matter that this results in an ill-formatted mbuf chain,
	 * since BPF just looks at the data.  (It doesn't try to free the mbuf,
	 * tho' it will make a copy for tcpdump.)
	 */
	if (bpf_gets_it) {
		struct mbuf m0;
		m0.m_len = sizeof eh;
		m0.m_data = (caddr_t)&eh;
		m0.m_next = m;

		/* Pass it up. */
		bpf_mtap(sc->sc_ethercom.ec_if.if_bpf, &m0);

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
#endif /* NBPFILTER > 0 */

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
	ether_input(&sc->sc_ethercom.ec_if, &eh, m);
	sc->sc_ethercom.ec_if.if_ipackets++;
}

static void
ie_drop_packet_buffer(sc)
	struct ie_softc *sc;
{
	int i;

	do {
		bus_space_barrier(sc->bt, sc->bh, 0, 0, BUS_SPACE_BARRIER_READ);
		i = SWAP(sc->rbuffs[sc->rbhead]->ie_rbd_actual);
		if ((i & IE_RBD_USED) == 0) {
			/*
			 * This means we are somehow out of sync.  So, we
			 * reset the adapter.
			 */
#ifdef IEDEBUG
			print_rbd(sc->rbuffs[sc->rbhead]);
#endif
			log(LOG_ERR, "%s: receive descriptors out of sync at %d\n",
			    sc->sc_dev.dv_xname, sc->rbhead);
			iereset(sc);
			return;
		}

		i = (i & IE_RBD_LAST) != 0;

		sc->rbuffs[sc->rbhead]->ie_rbd_length |= SWAP(IE_RBD_LAST);
		sc->rbuffs[sc->rbhead]->ie_rbd_actual = SWAP(0);
		sc->rbhead = (sc->rbhead + 1) % sc->nrxbuf;
		sc->rbuffs[sc->rbtail]->ie_rbd_length &= ~SWAP(IE_RBD_LAST);
		sc->rbtail = (sc->rbtail + 1) % sc->nrxbuf;
	} while (!i);
}


/*
 * Start transmission on an interface.
 */
void
iestart(ifp)
	struct ifnet *ifp;
{
	struct ie_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	u_char *buffer;
	u_short len;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	for (;;) {
		if (sc->xmit_busy == NTXBUF) {
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

		if (m0->m_pkthdr.len > IE_TBUF_SIZE)
			printf("%s: tbuf overflow\n", sc->sc_dev.dv_xname);

		buffer = sc->xmit_cbuffs[sc->xchead];
		for (m = m0; m != 0; m = m->m_next) {
			(sc->memcopy)(mtod(m, caddr_t), buffer, m->m_len);
			buffer += m->m_len;
		}

		len = max(m0->m_pkthdr.len, ETHER_MIN_LEN);
		m_freem(m0);

		sc->xmit_buffs[sc->xchead]->ie_xmit_flags = SWAP(len);

		/* Start the first packet transmitting. */
		if (sc->xmit_busy == 0)
			iexmit(sc);

		sc->xchead = (sc->xchead + 1) % NTXBUF;
		sc->xmit_busy++;
	}
}

/*
 * set up IE's ram space
 */
int
ie_setupram(sc)
	struct ie_softc *sc;
{
	volatile struct ie_sys_conf_ptr *scp;
	volatile struct ie_int_sys_conf_ptr *iscp;
	volatile struct ie_sys_ctl_block *scb;
	int     s;

	s = splnet();

	scp = sc->scp;
	(sc->memzero)((char *) scp, sizeof *scp);

	iscp = sc->iscp;
	(sc->memzero)((char *) iscp, sizeof *iscp);

	scb = sc->scb;
	(sc->memzero)((char *) scb, sizeof *scb);

	scp->ie_bus_use = 0;	/* 16-bit */
	ST_24(scp->ie_iscp_ptr, MK_24(sc->sc_iobase, iscp));

	iscp->ie_busy = 1;	/* ie_busy == char */
	iscp->ie_scb_offset = MK_16(sc->sc_maddr, scb);
	ST_24(iscp->ie_base, MK_24(sc->sc_iobase, sc->sc_maddr));

	if (sc->hwreset)
		(sc->hwreset)(sc);

	(sc->chan_attn) (sc);

	delay(100);		/* wait a while... */

	if (iscp->ie_busy) {
		splx(s);
		return 0;
	}
	/*
	 * Acknowledge any interrupts we may have caused...
	 */
	ie_ack(sc, IE_ST_WHENCE);
	splx(s);

	return 1;
}

void
iereset(sc)
	struct ie_softc *sc;
{
	int s = splnet();

	printf("%s: reset\n", sc->sc_dev.dv_xname);

	/* Clear OACTIVE in case we're called from watchdog (frozen xmit). */
	sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;

	/*
	 * Stop i82586 dead in its tracks.
	 */
	if (command_and_wait(sc, IE_RU_ABORT | IE_CU_ABORT, 0, 0))
		printf("%s: abort commands timed out\n", sc->sc_dev.dv_xname);

	if (command_and_wait(sc, IE_RU_DISABLE | IE_CU_STOP, 0, 0))
		printf("%s: disable commands timed out\n", sc->sc_dev.dv_xname);


#if 1
	if (sc->hwreset)
		(sc->hwreset)(sc);
#endif
	ie_ack(sc, IE_ST_WHENCE);
#ifdef notdef
	if (!check_ie_present(sc, sc->sc_maddr, sc->sc_msize))
		panic("ie disappeared!\n");
#endif

	ieinit(sc);

	splx(s);
}

/*
 * Send a command to the controller and wait for it to either complete
 * or be accepted, depending on the command.  If the command pointer
 * is null, then pretend that the command is not an action command.
 * If the command pointer is not null, and the command is an action
 * command, wait for
 * ((volatile struct ie_cmd_common *)pcmd)->ie_cmd_status & MASK
 * to become true.
 */
static int
command_and_wait(sc, cmd, pcmd, mask)
	struct ie_softc *sc;
	int cmd;	/* native byte-order */
	volatile void *pcmd;
	int mask;	/* native byte-order */
{
	volatile struct ie_cmd_common *cc = pcmd;
	volatile struct ie_sys_ctl_block *scb = sc->scb;
	int i;

	scb->ie_command = (u_short)SWAP(cmd);
	bus_space_barrier(sc->bt, sc->bh, 0, 0, BUS_SPACE_BARRIER_WRITE);
	(sc->chan_attn)(sc);

	if (IE_ACTION_COMMAND(cmd) && pcmd) {
		/*
		 * According to the packet driver, the minimum timeout should
		 * be .369 seconds, which we round up to .4.
		 */

		/*
		 * Now spin-lock waiting for status.  This is not a very nice
		 * thing to do, but I haven't figured out how, or indeed if, we
		 * can put the process waiting for action to sleep.  (We may
		 * be getting called through some other timeout running in the
		 * kernel.)
		 */
		for (i = 0; i < 369000; i++) {
			delay(1);
			if ((SWAP(cc->ie_cmd_status) & mask))
				return (0);
		}

	} else {
		/*
		 * Otherwise, just wait for the command to be accepted.
		 */

		/* XXX spin lock; wait at most 0.1 seconds */
		for (i = 0; i < 100000; i++) {
			if (scb->ie_command)
				return (0);
			delay(1);
		}
	}

	/* Timeout */
	return (1);
}

/*
 * Run the time-domain reflectometer.
 */
static void
run_tdr(sc, cmd)
	struct ie_softc *sc;
	struct ie_tdr_cmd *cmd;
{
	int result;

	cmd->com.ie_cmd_status = SWAP(0);
	cmd->com.ie_cmd_cmd = SWAP(IE_CMD_TDR | IE_CMD_LAST);
	cmd->com.ie_cmd_link = SWAP(0xffff);

	sc->scb->ie_command_list = MK_16(sc->sc_maddr, cmd);
	cmd->ie_tdr_time = SWAP(0);

	if (command_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
	    (SWAP(cmd->com.ie_cmd_status) & IE_STAT_OK) == 0)
		result = 0x10000; /* XXX */
	else
		result = SWAP(cmd->ie_tdr_time);

	ie_ack(sc, IE_ST_WHENCE);

	if (result & IE_TDR_SUCCESS)
		return;

	if (result & 0x10000)
		printf("%s: TDR command failed\n", sc->sc_dev.dv_xname);
	else if (result & IE_TDR_XCVR)
		printf("%s: transceiver problem\n", sc->sc_dev.dv_xname);
	else if (result & IE_TDR_OPEN)
		printf("%s: TDR detected an open %d clocks away\n",
		    sc->sc_dev.dv_xname, result & IE_TDR_TIME);
	else if (result & IE_TDR_SHORT)
		printf("%s: TDR detected a short %d clocks away\n",
		    sc->sc_dev.dv_xname, result & IE_TDR_TIME);
	else
		printf("%s: TDR returned unknown status 0x%x\n",
		    sc->sc_dev.dv_xname, result);
}

#ifdef notdef
/* ALIGN works on 8 byte boundaries.... but 4 byte boundaries are ok for sun */
#define	_ALLOC(p, n)	(bzero(p, n), p += n, p - n)
#define	ALLOC(p, n)	_ALLOC(p, ALIGN(n)) /* XXX convert to this? */
#endif

/*
 * setup_bufs: set up the buffers
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
 */
static void
setup_bufs(sc)
	struct ie_softc *sc;
{
	caddr_t ptr = sc->buf_area;	/* memory pool */
	int     n, r;

	/*
	 * step 0: zero memory and figure out how many recv buffers and
	 * frames we can have.
	 */
	(sc->memzero)(ptr, sc->buf_area_sz);
	ptr = (sc->align)(ptr);	/* set alignment and stick with it */

	n = (int)(sc->align)((caddr_t) sizeof(struct ie_xmit_cmd)) +
	    (int)(sc->align)((caddr_t) sizeof(struct ie_xmit_buf)) + IE_TBUF_SIZE;
	n *= NTXBUF;		/* n = total size of xmit area */

	n = sc->buf_area_sz - n;/* n = free space for recv stuff */

	r = (int)(sc->align)((caddr_t) sizeof(struct ie_recv_frame_desc)) +
	    (((int)(sc->align)((caddr_t) sizeof(struct ie_recv_buf_desc)) +
		IE_RBUF_SIZE) * B_PER_F);

	/* r = size of one R frame */

	sc->nframes = n / r;
	if (sc->nframes <= 0)
		panic("ie: bogus buffer calc\n");
	if (sc->nframes > MAXFRAMES)
		sc->nframes = MAXFRAMES;

	sc->nrxbuf = sc->nframes * B_PER_F;

#ifdef IEDEBUG
	printf("IEDEBUG: %d frames %d bufs\n", sc->nframes, sc->nrxbuf);
#endif

	/*
	 *  step 1a: lay out and zero frame data structures for transmit and recv
	 */
	for (n = 0; n < NTXBUF; n++) {
		sc->xmit_cmds[n] = (volatile struct ie_xmit_cmd *) ptr;
		ptr = (sc->align)(ptr + sizeof(struct ie_xmit_cmd));
	}

	for (n = 0; n < sc->nframes; n++) {
		sc->rframes[n] = (volatile struct ie_recv_frame_desc *) ptr;
		ptr = (sc->align)(ptr + sizeof(struct ie_recv_frame_desc));
	}

	/*
	 * step 1b: link together the recv frames and set EOL on last one
	 */
	for (n = 0; n < sc->nframes; n++) {
		sc->rframes[n]->ie_fd_next =
		    MK_16(sc->sc_maddr, sc->rframes[(n + 1) % sc->nframes]);
	}
	sc->rframes[sc->nframes - 1]->ie_fd_last |= SWAP(IE_FD_LAST);

	/*
	 * step 2a: lay out and zero frame buffer structures for xmit and recv
	 */
	for (n = 0; n < NTXBUF; n++) {
		sc->xmit_buffs[n] = (volatile struct ie_xmit_buf *) ptr;
		ptr = (sc->align)(ptr + sizeof(struct ie_xmit_buf));
	}

	for (n = 0; n < sc->nrxbuf; n++) {
		sc->rbuffs[n] = (volatile struct ie_recv_buf_desc *) ptr;
		ptr = (sc->align)(ptr + sizeof(struct ie_recv_buf_desc));
	}

	/*
	 * step 2b: link together recv bufs and set EOL on last one
	 */
	for (n = 0; n < sc->nrxbuf; n++) {
		sc->rbuffs[n]->ie_rbd_next =
		    MK_16(sc->sc_maddr, sc->rbuffs[(n + 1) % sc->nrxbuf]);
	}
	sc->rbuffs[sc->nrxbuf - 1]->ie_rbd_length |= SWAP(IE_RBD_LAST);

	/*
	 * step 3: allocate the actual data buffers for xmit and recv
	 * recv buffer gets linked into recv_buf_desc list here
	 */
	for (n = 0; n < NTXBUF; n++) {
		sc->xmit_cbuffs[n] = (u_char *) ptr;
		ptr = (sc->align)(ptr + IE_TBUF_SIZE);
	}

	/* Pointers to last packet sent and next available transmit buffer. */
	sc->xchead = sc->xctail = 0;

	/* Clear transmit-busy flag and set number of free transmit buffers. */
	sc->xmit_busy = 0;

	for (n = 0; n < sc->nrxbuf; n++) {
		sc->cbuffs[n] = (char *) ptr;	/* XXX why char vs uchar? */
		sc->rbuffs[n]->ie_rbd_length = SWAP(IE_RBUF_SIZE);
		ST_24(sc->rbuffs[n]->ie_rbd_buffer, MK_24(sc->sc_iobase, ptr));
		ptr = (sc->align)(ptr + IE_RBUF_SIZE);
	}

	/*
	 * step 4: set the head and tail pointers on receive to keep track of
	 * the order in which RFDs and RBDs are used.   link in recv frames
	 * and buffer into the scb.
	 */

	sc->rfhead = 0;
	sc->rftail = sc->nframes - 1;
	sc->rbhead = 0;
	sc->rbtail = sc->nrxbuf - 1;

	sc->scb->ie_recv_list = MK_16(sc->sc_maddr, sc->rframes[0]);
	sc->rframes[0]->ie_fd_buf_desc = MK_16(sc->sc_maddr, sc->rbuffs[0]);

#ifdef IEDEBUG
	printf("IE_DEBUG: reserved %d bytes\n", ptr - sc->buf_area);
#endif
}

/*
 * Run the multicast setup command.
 * Called at splnet().
 */
static int
mc_setup(sc, ptr)
	struct ie_softc *sc;
	void *ptr;
{
	volatile struct ie_mcast_cmd *cmd = ptr;

	cmd->com.ie_cmd_status = SWAP(0);
	cmd->com.ie_cmd_cmd = SWAP(IE_CMD_MCAST | IE_CMD_LAST);
	cmd->com.ie_cmd_link = SWAP(0xffff);

	(sc->memcopy)((caddr_t)sc->mcast_addrs, (caddr_t)cmd->ie_mcast_addrs,
	    sc->mcast_count * sizeof *sc->mcast_addrs);

	cmd->ie_mcast_bytes =
	  SWAP(sc->mcast_count * ETHER_ADDR_LEN); /* grrr... */

	sc->scb->ie_command_list = MK_16(sc->sc_maddr, cmd);
	if (command_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
	    (SWAP(cmd->com.ie_cmd_status) & IE_STAT_OK) == 0) {
		printf("%s: multicast address setup command failed\n",
		    sc->sc_dev.dv_xname);
		return 0;
	}
	return 1;
}

/*
 * This routine takes the environment generated by check_ie_present() and adds
 * to it all the other structures we need to operate the adapter.  This
 * includes executing the CONFIGURE, IA-SETUP, and MC-SETUP commands, starting
 * the receiver unit, and clearing interrupts.
 *
 * THIS ROUTINE MUST BE CALLED AT splnet() OR HIGHER.
 */
int
ieinit(sc)
	struct ie_softc *sc;
{
	volatile struct ie_sys_ctl_block *scb = sc->scb;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	void *ptr;

	ptr = sc->buf_area;

	/*
	 * Send the configure command first.
	 */
	{
		volatile struct ie_config_cmd *cmd = ptr;

		scb->ie_command_list = MK_16(sc->sc_maddr, cmd);
		cmd->com.ie_cmd_status = SWAP(0);
		cmd->com.ie_cmd_cmd = SWAP(IE_CMD_CONFIG | IE_CMD_LAST);
		cmd->com.ie_cmd_link = SWAP(0xffff);

		ie_setup_config(cmd, sc->promisc, 0);

		if (command_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
		    (SWAP(cmd->com.ie_cmd_status) & IE_STAT_OK) == 0) {
			printf("%s: configure command failed\n",
			    sc->sc_dev.dv_xname);
			return 0;
		}
	}

	/*
	 * Now send the Individual Address Setup command.
	 */
	{
		volatile struct ie_iasetup_cmd *cmd = ptr;

		scb->ie_command_list = MK_16(sc->sc_maddr, cmd);
		cmd->com.ie_cmd_status = SWAP(0);
		cmd->com.ie_cmd_cmd = SWAP(IE_CMD_IASETUP | IE_CMD_LAST);
		cmd->com.ie_cmd_link = SWAP(0xffff);

		(sc->memcopy)(LLADDR(ifp->if_sadl),
		      (caddr_t)&cmd->ie_address, sizeof cmd->ie_address);

		if (command_and_wait(sc, IE_CU_START, cmd, IE_STAT_COMPL) ||
		    (SWAP(cmd->com.ie_cmd_status) & IE_STAT_OK) == 0) {
			printf("%s: individual address setup command failed\n",
			    sc->sc_dev.dv_xname);
			return 0;
		}
	}

	/*
	 * Now run the time-domain reflectometer.
	 */
	run_tdr(sc, ptr);

	/*
	 * Acknowledge any interrupts we have generated thus far.
	 */
	ie_ack(sc, IE_ST_WHENCE);

	/*
	 * Set up the transmit and recv buffers.
	 */
	setup_bufs(sc);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	sc->scb->ie_recv_list = MK_16(sc->sc_maddr, sc->rframes[0]);
	command_and_wait(sc, IE_RU_START, 0, 0);

	ie_ack(sc, IE_ST_WHENCE);

	if (sc->hwinit)
		(sc->hwinit)(sc);

	return 0;
}

static void
iestop(sc)
	struct ie_softc *sc;
{

	command_and_wait(sc, IE_RU_DISABLE, 0, 0);
}

int
ieioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ie_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch(cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch(ifa->ifa_addr->sa_family) {
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
				bcopy(ina->x_host.c_host,
				    LLADDR(ifp->if_sadl), ETHER_ADDR_LEN);
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
			sc->sc_debug = 0;
#endif
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom):
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
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
mc_reset(sc)
	struct ie_softc *sc;
{
	struct ether_multi *enm;
	struct ether_multistep step;

	/*
	 * Step through the list of addresses.
	 */
	sc->mcast_count = 0;
	ETHER_FIRST_MULTI(step, &sc->sc_ethercom, enm);
	while (enm) {
		if (sc->mcast_count >= MAXMCAST ||
		    bcmp(enm->enm_addrlo, enm->enm_addrhi, 6) != 0) {
			sc->sc_ethercom.ec_if.if_flags |= IFF_ALLMULTI;
			ieioctl(&sc->sc_ethercom.ec_if, SIOCSIFFLAGS, (void *)0);
			goto setflag;
		}

		bcopy(enm->enm_addrlo, &sc->mcast_addrs[sc->mcast_count], 6);
		sc->mcast_count++;
		ETHER_NEXT_MULTI(step, enm);
	}
setflag:
	sc->want_mcsetup = 1;
}

#ifdef IEDEBUG
void
print_rbd(rbd)
	volatile struct ie_recv_buf_desc *rbd;
{
	u_long bufval;

	bcopy((char *)&rbd->ie_rbd_buffer, &bufval, 4); /*XXX*/

	printf("RBD at %08lx:\nactual %04x, next %04x, buffer %lx\n"
		"length %04x, mbz %04x\n", (u_long)rbd,
		SWAP(rbd->ie_rbd_actual),
		SWAP(rbd->ie_rbd_next),
		bufval,
		SWAP(rbd->ie_rbd_length),
		rbd->mbz);
}
#endif
