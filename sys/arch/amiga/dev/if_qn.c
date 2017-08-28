/*	$NetBSD: if_qn.c,v 1.39.14.5 2017/08/28 17:51:28 skrll Exp $ */

/*
 * Copyright (c) 1995 Mika Kortelainen
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
 *      This product includes software developed by  Mika Kortelainen
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
 *
 * Thanks for Aspecs Oy (Finland) for the data book for the NIC used
 * in this card and also many thanks for the Resource Management Force
 * (QuickNet card manufacturer) and especially Daniel Koch for providing
 * me with the necessary 'inside' information to write the driver.
 *
 * This is partly based on other code:
 * - if_ed.c: basic function structure for Ethernet driver and now also
 *	qn_put() is done similarly, i.e. no extra packet buffers.
 *
 *	Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 *	adapters.
 *
 *	Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 *	Copyright (C) 1993, David Greenman.  This software may be used,
 *	modified, copied, distributed, and sold, in both source and binary
 *	form provided that the above copyright and these terms are retained.
 *	Under no circumstances is the author responsible for the proper
 *	functioning of this software, nor does the author assume any
 *	responsibility for damages incurred with its use.
 *
 * - if_es.c: used as an example of -current driver
 *
 *	Copyright (c) 1995 Michael L. Hitch
 *	All rights reserved.
 *
 * - if_fe.c: some ideas for error handling for qn_rint() which might
 *	have fixed those random lock ups, too.
 *
 *	All Rights Reserved, Copyright (C) Fujitsu Limited 1995
 *
 *
 * TODO:
 * - add multicast support
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_qn.c,v 1.39.14.5 2017/08/28 17:51:28 skrll Exp $");

#include "qn.h"
#if NQN > 0

#define QN_DEBUG
#define QN_DEBUG1_no /* hides some old tests */
#define QN_CHECKS_no /* adds some checks (not needed in normal situations) */


/*
 * Fujitsu MB86950 Ethernet Controller (as used in the QuickNet QN2000
 * Ethernet card)
 */

#include "opt_inet.h"
#include "opt_ns.h"

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
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#include <machine/cpu.h>
#include <amiga/amiga/device.h>
#include <amiga/amiga/isr.h>
#include <amiga/dev/zbusvar.h>
#include <amiga/dev/if_qnreg.h>


#define	NIC_R_MASK	(R_INT_PKT_RDY | R_INT_ALG_ERR |\
			 R_INT_CRC_ERR | R_INT_OVR_FLO)
#define	MAX_PACKETS	30 /* max number of packets read per interrupt */

/*
 * Ethernet software status per interface
 *
 * Each interface is referenced by a network interface structure,
 * qn_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	qn_softc {
	device_t sc_dev;
	struct	isr sc_isr;
	struct	ethercom sc_ethercom;	/* Common ethernet structures */
	u_char	volatile *sc_base;
	u_char	volatile *sc_nic_base;
	u_short	volatile *nic_fifo;
	u_short	volatile *nic_r_status;
	u_short	volatile *nic_t_status;
	u_short	volatile *nic_r_mask;
	u_short	volatile *nic_t_mask;
	u_short	volatile *nic_r_mode;
	u_short	volatile *nic_t_mode;
	u_short	volatile *nic_reset;
	u_short	volatile *nic_len;
	u_char	transmit_pending;
} qn_softc[NQN];

#include <net/bpf.h>
#include <net/bpfdesc.h>


int	qnmatch(device_t, cfdata_t, void *);
void	qnattach(device_t, device_t, void *);
int	qnintr(void *);
int	qnioctl(struct ifnet *, u_long, void *);
void	qnstart(struct ifnet *);
void	qnwatchdog(struct ifnet *);
void	qnreset(struct qn_softc *);
void	qninit(struct qn_softc *);
void	qnstop(struct qn_softc *);
static	u_short qn_put(u_short volatile *, struct mbuf *);
static	void qn_rint(struct qn_softc *, u_short);
static	void qn_flush(struct qn_softc *);
static	void inline word_copy_from_card(u_short volatile *, u_short *, u_short);
static	void inline word_copy_to_card(u_short *, u_short volatile *,
				register u_short);
static	void qn_get_packet(struct qn_softc *, u_short);
#ifdef QN_DEBUG1
static	void qn_dump(struct qn_softc *);
#endif

CFATTACH_DECL_NEW(qn, sizeof(struct qn_softc),
    qnmatch, qnattach, NULL, NULL);

int
qnmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct zbus_args *zap;

	zap = (struct zbus_args *)aux;

	/* RMF QuickNet QN2000 EtherNet card */
	if (zap->manid == 2011 && zap->prodid == 2)
		return (1);

	return (0);
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
void
qnattach(device_t parent, device_t self, void *aux)
{
	struct zbus_args *zap;
	struct qn_softc *sc = device_private(self);
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int8_t myaddr[ETHER_ADDR_LEN];

	zap = (struct zbus_args *)aux;

	sc->sc_base = zap->va;
	sc->sc_nic_base = sc->sc_base + QUICKNET_NIC_BASE;
	sc->nic_fifo = (u_short volatile *)(sc->sc_nic_base + NIC_BMPR0);
	sc->nic_len = (u_short volatile *)(sc->sc_nic_base + NIC_BMPR2);
	sc->nic_t_status = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR0);
	sc->nic_r_status = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR2);
	sc->nic_t_mask = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR1);
	sc->nic_r_mask = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR3);
	sc->nic_t_mode = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR4);
	sc->nic_r_mode = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR5);
	sc->nic_reset = (u_short volatile *)(sc->sc_nic_base + NIC_DLCR6);
	sc->transmit_pending = 0;

	/*
	 * The ethernet address of the board (1st three bytes are the vendor
	 * address, the rest is the serial number of the board).
	 */
	myaddr[0] = 0x5c;
	myaddr[1] = 0x5c;
	myaddr[2] = 0x00;
	myaddr[3] = (zap->serno >> 16) & 0xff;
	myaddr[4] = (zap->serno >> 8) & 0xff;
	myaddr[5] = zap->serno & 0xff;

	/* set interface to stopped condition (reset) */
	qnstop(sc);

	memcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_ioctl = qnioctl;
	ifp->if_watchdog = qnwatchdog;
	ifp->if_start = qnstart;
	/* XXX IFF_MULTICAST */
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;
	ifp->if_mtu = ETHERMTU;

	/* Attach the interface. */
	if_attach(ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, myaddr);

#ifdef QN_DEBUG
	printf(": hardware address %s\n", ether_sprintf(myaddr));
#endif

	sc->sc_isr.isr_intr = qnintr;
	sc->sc_isr.isr_arg = sc;
	sc->sc_isr.isr_ipl = 2;
	add_isr(&sc->sc_isr);
}

/*
 * Initialize device
 *
 */
void
qninit(struct qn_softc *sc)
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_short i;
	static int retry = 0;

	*sc->nic_r_mask   = NIC_R_MASK;
	*sc->nic_t_mode   = NO_LOOPBACK;

	if (sc->sc_ethercom.ec_if.if_flags & IFF_PROMISC) {
		*sc->nic_r_mode = PROMISCUOUS_MODE;
		log(LOG_INFO, "qn: Promiscuous mode (not tested)\n");
	} else
		*sc->nic_r_mode = NORMAL_MODE;

	/* Set physical ethernet address. */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		*((u_short volatile *)(sc->sc_nic_base+
				       QNET_HARDWARE_ADDRESS+2*i)) =
		    ((((u_short)CLLADDR(ifp->if_sadl)[i]) << 8) |
		    CLLADDR(ifp->if_sadl)[i]);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;
	sc->transmit_pending = 0;

	qn_flush(sc);

	/* QuickNet magic. Done ONLY once, otherwise a lockup occurs. */
	if (retry == 0) {
		*((u_short volatile *)(sc->sc_nic_base + QNET_MAGIC)) = 0;
		retry = 1;
	}

	/* Enable data link controller. */
	*sc->nic_reset = ENABLE_DLC;

	/* Attempt to start output, if any. */
	if_schedule_deferred_start(ifp);
}

/*
 * Device timeout/watchdog routine.  Entered if the device neglects to
 * generate an interrupt after a transmit has been started on it.
 */
void
qnwatchdog(struct ifnet *ifp)
{
	struct qn_softc *sc = ifp->if_softc;

	log(LOG_INFO, "qn: device timeout (watchdog)\n");
	++sc->sc_ethercom.ec_if.if_oerrors;

	qnreset(sc);
}

/*
 * Flush card's buffer RAM.
 */
static void
qn_flush(struct qn_softc *sc)
{
#if 1
	/* Read data until bus read error (i.e. buffer empty). */
	while (!(*sc->nic_r_status & R_BUS_RD_ERR))
		(void)(*sc->nic_fifo);
#else
	/* Read data twice to clear some internal pipelines. */
	(void)(*sc->nic_fifo);
	(void)(*sc->nic_fifo);
#endif

	/* Clear bus read error. */
	*sc->nic_r_status = R_BUS_RD_ERR;
}

/*
 * Reset the interface.
 *
 */
void
qnreset(struct qn_softc *sc)
{
	int s;

	s = splnet();
	qnstop(sc);
	qninit(sc);
	splx(s);
}

/*
 * Take interface offline.
 */
void
qnstop(struct qn_softc *sc)
{

	/* Stop the interface. */
	*sc->nic_reset    = DISABLE_DLC;
	delay(200);
	*sc->nic_t_status = CLEAR_T_ERR;
	*sc->nic_t_mask   = CLEAR_T_MASK;
	*sc->nic_r_status = CLEAR_R_ERR;
	*sc->nic_r_mask   = CLEAR_R_MASK;

	/* Turn DMA off */
	*((u_short volatile *)(sc->sc_nic_base + NIC_BMPR4)) = 0;

	/* Accept no packets. */
	*sc->nic_r_mode = 0;
	*sc->nic_t_mode = 0;

	qn_flush(sc);
}

/*
 * Start output on interface. Get another datagram to send
 * off the interface queue, and copy it to the
 * interface before starting the output.
 *
 * This assumes that it is called inside a critical section...
 *
 */
void
qnstart(struct ifnet *ifp)
{
	struct qn_softc *sc = ifp->if_softc;
	struct mbuf *m;
	u_short len;
	int timout = 60000;


	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

	IF_DEQUEUE(&ifp->if_snd, m);
	if (m == 0)
		return;

	/*
	 * If bpf is listening on this interface, let it
	 * see the packet before we commit it to the wire
	 *
	 * (can't give the copy in QuickNet card RAM to bpf, because
	 * that RAM is not visible to the host but is read from FIFO)
	 */
	bpf_mtap(ifp, m);
	len = qn_put(sc->nic_fifo, m);
	m_freem(m);

	/*
	 * Really transmit the packet.
	 */

	/* Set packet length (byte-swapped). */
	len = ((len >> 8) & 0x0007) | TRANSMIT_START | ((len & 0x00ff) << 8);
	*sc->nic_len = len;

	/* Wait for the packet to really leave. */
	while (!(*sc->nic_t_status & T_TMT_OK) && --timout) {
		if ((timout % 10000) == 0)
			log(LOG_INFO, "qn: timout...\n");
	}

	if (timout == 0)
		/* Maybe we should try to recover from this one? */
		/* But now, let's just fall thru and hope the best... */
		log(LOG_INFO, "qn: transmit timout (fatal?)\n");

	sc->transmit_pending = 1;
	*sc->nic_t_mask = INT_TMT_OK | INT_SIXTEEN_COL;

	ifp->if_flags |= IFF_OACTIVE;
	ifp->if_timer = 2;
}

/*
 * Memory copy, copies word at a time
 */
static void inline
word_copy_from_card(u_short volatile *card, u_short *b, u_short len)
{
	register u_short l = len/2;

	while (l--)
		*b++ = *card;
}

static void inline
word_copy_to_card(u_short *a, u_short volatile *card, register u_short len)
{
	/*register u_short l = len/2;*/

	while (len--)
		*card = *a++;
}

/*
 * Copy packet from mbuf to the board memory
 *
 */
static u_short
qn_put(u_short volatile *addr, struct mbuf *m)
{
	u_short *data;
	u_char savebyte[2];
	int len, len1, wantbyte;
	u_short totlen;

	totlen = wantbyte = 0;

	for (; m != NULL; m = m->m_next) {
		data = mtod(m, u_short *);
		len = m->m_len;
		if (len > 0) {
			totlen += len;

			/* Finish the last word. */
			if (wantbyte) {
				savebyte[1] = *((u_char *)data);
				*addr = *((u_short *)savebyte);
				data = (u_short *)((u_char *)data + 1);
				len--;
				wantbyte = 0;
			}
			/* Output contiguous words. */
			if (len > 1) {
				len1 = len/2;
				word_copy_to_card(data, addr, len1);
				data += len1;
				len &= 1;
			}
			/* Save last byte, if necessary. */
			if (len == 1) {
				savebyte[0] = *((u_char *)data);
				wantbyte = 1;
			}
		}
	}

	if (wantbyte) {
		savebyte[1] = 0;
		*addr = *((u_short *)savebyte);
	}

	if(totlen < (ETHER_MIN_LEN - ETHER_CRC_LEN)) {
		/*
		 * Fill the rest of the packet with zeros.
		 * N.B.: This is required! Otherwise MB86950 fails.
		 */
		for(len = totlen + 1; len < (ETHER_MIN_LEN - ETHER_CRC_LEN);
		    len += 2)
	  		*addr = (u_short)0x0000;
		totlen = (ETHER_MIN_LEN - ETHER_CRC_LEN);
	}

	return (totlen);
}

/*
 * Copy packet from board RAM.
 *
 * Trailers not supported.
 *
 */
static void
qn_get_packet(struct qn_softc *sc, u_short len)
{
	register u_short volatile *nic_fifo_ptr = sc->nic_fifo;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m, *dst, *head = NULL;
	register u_short len1;
	u_short amount;

	/* Allocate header mbuf. */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		goto bad;

	/*
	 * Round len to even value.
	 */
	if (len & 1)
		len++;

	m_set_rcvif(m, &sc->sc_ethercom.ec_if);
	m->m_pkthdr.len = len;
	m->m_len = 0;
	head = m;

	word_copy_from_card(nic_fifo_ptr,
	    mtod(head, u_short *),
	    sizeof(struct ether_header));

	head->m_len += sizeof(struct ether_header);
	len -= sizeof(struct ether_header);

	while (len > 0) {
		len1 = len;

		amount = M_TRAILINGSPACE(m);
		if (amount == 0) {
			/* Allocate another mbuf. */
			dst = m;
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == NULL)
				goto bad;

			if (len1 >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);

			m->m_len = 0;
			dst->m_next = m;

			amount = M_TRAILINGSPACE(m);
		}

		if (amount < len1)
			len1 = amount;

		word_copy_from_card(nic_fifo_ptr,
		    (u_short *)(mtod(m, char *) + m->m_len),
		    len1);
		m->m_len += len1;
		len -= len1;
	}

	if_percpuq_enqueue(ifp->if_percpuq, head);
	return;

bad:
	if (head) {
		m_freem(head);
		log(LOG_INFO, "qn_get_packet: mbuf alloc failed\n");
	}
}

/*
 * Ethernet interface receiver interrupt.
 */
static void
qn_rint(struct qn_softc *sc, u_short rstat)
{
	int i;
	u_short len, status;

	/* Clear the status register. */
	*sc->nic_r_status = CLEAR_R_ERR;

	/*
	 * Was there some error?
	 * Some of them are senseless because they are masked off.
	 * XXX
	 */
	if (rstat & R_INT_OVR_FLO) {
#ifdef QN_DEBUG
		log(LOG_INFO, "Overflow\n");
#endif
		++sc->sc_ethercom.ec_if.if_ierrors;
	}
	if (rstat & R_INT_CRC_ERR) {
#ifdef QN_DEBUG
		log(LOG_INFO, "CRC Error\n");
#endif
		++sc->sc_ethercom.ec_if.if_ierrors;
	}
	if (rstat & R_INT_ALG_ERR) {
#ifdef QN_DEBUG
		log(LOG_INFO, "Alignment error\n");
#endif
		++sc->sc_ethercom.ec_if.if_ierrors;
	}
	if (rstat & R_INT_SRT_PKT) {
		/* Short packet (these may occur and are
		 * no reason to worry about - or maybe
		 * they are?).
		 */
#ifdef QN_DEBUG
		log(LOG_INFO, "Short packet\n");
#endif
		++sc->sc_ethercom.ec_if.if_ierrors;
	}
	if (rstat & 0x4040) {
#ifdef QN_DEBUG
		log(LOG_INFO, "Bus read error\n");
#endif
		++sc->sc_ethercom.ec_if.if_ierrors;
		qnreset(sc);
	}

	/*
	 * Read at most MAX_PACKETS packets per interrupt
	 */
	for (i = 0; i < MAX_PACKETS; i++) {
		if (*sc->nic_r_mode & RM_BUF_EMP)
			/* Buffer empty. */
			break;

		/*
		 * Read the first word: upper byte contains useful
		 * information.
		 */
		status = *sc->nic_fifo;
		if ((status & 0x7000) != 0x2000) {
			log(LOG_INFO, "qn: ERROR: status=%04x\n", status);
			continue;
		}

		/*
		 * Read packet length (byte-swapped).
		 * CRC is stripped off by the NIC.
		 */
		len = *sc->nic_fifo;
		len = ((len << 8) & 0xff00) | ((len >> 8) & 0x00ff);

#ifdef QN_CHECKS
		if (len > (ETHER_MAX_LEN - ETHER_CRC_LEN) ||
		    len < ETHER_HDR_LEN) {
			log(LOG_WARNING,
			    "%s: received a %s packet? (%u bytes)\n",
			    device_xname(sc->sc_dev),
			    len < ETHER_HDR_LEN ? "partial" : "big", len);
			++sc->sc_ethercom.ec_if.if_ierrors;
			continue;
		}
#endif
#ifdef QN_CHECKS
		if (len < (ETHER_MIN_LEN - ETHER_CRC_LEN))
			log(LOG_WARNING,
			    "%s: received a short packet? (%u bytes)\n",
			    device_xname(sc->sc_dev), len);
#endif

		/* Read the packet. */
		qn_get_packet(sc, len);
	}

#ifdef QN_DEBUG
	/* This print just to see whether MAX_PACKETS is large enough. */
	if (i == MAX_PACKETS)
		log(LOG_INFO, "used all the %d loops\n", MAX_PACKETS);
#endif
}

/*
 * Our interrupt routine
 */
int
qnintr(void *arg)
{
	struct qn_softc *sc = arg;
	u_short tint, rint, tintmask;
	char return_tintmask = 0;

	/*
	 * If the driver has not been initialized, just return immediately.
	 * This also happens if there is no QuickNet board present.
	 */
	if (sc->sc_base == NULL)
		return (0);

	/* Get interrupt statuses and masks. */
	rint = (*sc->nic_r_status) & NIC_R_MASK;
	tintmask = *sc->nic_t_mask;
	tint = (*sc->nic_t_status) & tintmask;
	if (tint == 0 && rint == 0)
		return (0);

	/* Disable interrupts so that we won't miss anything. */
	*sc->nic_r_mask = CLEAR_R_MASK;
	*sc->nic_t_mask = CLEAR_T_MASK;

	/*
	 * Handle transmitter interrupts. Some of them are not asked for
	 * but do happen, anyway.
	 */

	if (tint != 0) {
		/* Clear transmit interrupt status. */
		*sc->nic_t_status = CLEAR_T_ERR;

		if (sc->transmit_pending && (tint & T_TMT_OK)) {
			sc->transmit_pending = 0;
			/*
			 * Update total number of successfully
			 * transmitted packets.
			 */
			sc->sc_ethercom.ec_if.if_opackets++;
		}

		if (tint & T_SIXTEEN_COL) {
			/*
			 * 16 collision (i.e., packet lost).
			 */
			log(LOG_INFO, "qn: 16 collision - packet lost\n");
#ifdef QN_DEBUG1
			qn_dump(sc);
#endif
			sc->sc_ethercom.ec_if.if_oerrors++;
			sc->sc_ethercom.ec_if.if_collisions += 16;
			sc->transmit_pending = 0;
		}

		if (sc->transmit_pending) {
			log(LOG_INFO, "qn:still pending...\n");

			/* Must return transmission interrupt mask. */
			return_tintmask = 1;
		} else {
			sc->sc_ethercom.ec_if.if_flags &= ~IFF_OACTIVE;

			/* Clear watchdog timer. */
			sc->sc_ethercom.ec_if.if_timer = 0;
		}
	} else
		return_tintmask = 1;

	/*
	 * Handle receiver interrupts.
	 */
	if (rint != 0)
		qn_rint(sc, rint);

	if ((sc->sc_ethercom.ec_if.if_flags & IFF_OACTIVE) == 0)
		if_schedule_deferred_start(&sc->sc_ethercom.ec_if);
	else if (return_tintmask == 1)
		*sc->nic_t_mask = tintmask;

	/* Set receive interrupt mask back. */
	*sc->nic_r_mask = NIC_R_MASK;

	return (1);
}

/*
 * Process an ioctl request. This code needs some work - it looks pretty ugly.
 * I somehow think that this is quite a common excuse... ;-)
 */
int
qnioctl(register struct ifnet *ifp, u_long cmd, void *data)
{
	struct qn_softc *sc = ifp->if_softc;
	register struct ifaddr *ifa = (struct ifaddr *)data;
#if 0
	struct ifreg *ifr = (struct ifreg *)data;
#endif
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCINITIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			qnstop(sc);
			qninit(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			log(LOG_INFO, "qn:sa_family:default (not tested)\n");
			qnstop(sc);
			qninit(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		if ((error = ifioctl_common(ifp, cmd, data)) != 0)
			break;
		/* XXX see the comment in ed_ioctl() about code re-use */
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
#ifdef QN_DEBUG1
			qn_dump(sc);
#endif
			qnstop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			qninit(sc);
		} else {
			/*
			 * Something else... we won't do anything so we won't
			 * break anything (hope so).
			 */
#ifdef QN_DEBUG1
			log(LOG_INFO, "Else branch...\n");
#endif
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		log(LOG_INFO, "qnioctl: multicast not done yet\n");
#if 0
		if ((error = ether_ioctl(ifp, cmd, data)) == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			log(LOG_INFO, "qnioctl: multicast not done yet\n");
			error = 0;
		}
#else
		error = EINVAL;
#endif
		break;

	default:
		error = ether_ioctl(ifp, cmd, data);
	}

	splx(s);
	return (error);
}

/*
 * Dump some register information.
 */
#ifdef QN_DEBUG1
static void
qn_dump(struct qn_softc *sc)
{

	log(LOG_INFO, "t_status  : %04x\n", *sc->nic_t_status);
	log(LOG_INFO, "t_mask    : %04x\n", *sc->nic_t_mask);
	log(LOG_INFO, "t_mode    : %04x\n", *sc->nic_t_mode);
	log(LOG_INFO, "r_status  : %04x\n", *sc->nic_r_status);
	log(LOG_INFO, "r_mask    : %04x\n", *sc->nic_r_mask);
	log(LOG_INFO, "r_mode    : %04x\n", *sc->nic_r_mode);
	log(LOG_INFO, "pending   : %02x\n", sc->transmit_pending);
	log(LOG_INFO, "if_flags  : %04x\n", sc->sc_ethercom.ec_if.if_flags);
}
#endif

#endif /* NQN > 0 */
