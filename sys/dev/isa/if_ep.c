/*
 * Copyright (c) 1994 Herb Peyerl <hpeyerl@novatel.ca>
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
 *      This product includes software developed by Herb Peyerl.
 * 4. The name of Herb Peyerl may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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
 *	$Id: if_ep.c,v 1.48 1994/07/28 19:57:31 mycroft Exp $
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/netisr.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/netisr.h>

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

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isavar.h>
#include <i386/isa/if_epreg.h>
#include <i386/isa/elink.h>

#define ETHER_MIN_LEN 64
#define ETHER_MAX_LEN   1518
#define ETHER_ADDR_LEN  6
/*
 * Ethernet software status per interface.
 */
struct ep_softc {
	struct device sc_dev;
	struct intrhand sc_ih;

	struct arpcom sc_arpcom;	/* Ethernet common part		*/
	short   ep_iobase;		/* i/o bus address		*/
	char    ep_connectors;		/* Connectors on this card.	*/
#define MAX_MBS	8			/* # of mbufs we keep around	*/
	struct mbuf *mb[MAX_MBS];	/* spare mbuf storage.		*/
	int	next_mb;		/* Which mbuf to use next. 	*/
	int	last_mb;		/* Last mbuf.			*/
	int	tx_start_thresh;	/* Current TX_start_thresh.	*/
	int	tx_succ_ok;		/* # packets sent in sequence w/o underrun */
	char	bus32bit;		/* 32bit access possible */
};

static int epprobe();
static void epattach();

struct cfdriver epcd = {
	NULL, "ep", epprobe, epattach, DV_IFNET, sizeof(struct ep_softc)
};

int epintr __P((struct ep_softc *));
static void epinit __P((struct ep_softc *));
static int epioctl __P((struct ifnet *, int, caddr_t));
static int epstart __P((struct ifnet *));
static int epwatchdog __P((int));
static void epreset __P((struct ep_softc *));
static void epread __P((struct ep_softc *));
static void epmbuffill __P((struct ep_softc *));
static void epmbufempty __P((struct ep_softc *));
static void epstop __P((struct ep_softc *));
static void epsetfilter __P((struct ep_softc *sc));
static void epsetlink __P((struct ep_softc *sc));

static u_short epreadeeprom __P((int id_port, int offset));
static int epbusyeeprom __P((struct ep_softc *));

#define MAXEPCARDS 20	/* if you have 21 cards in your machine... you lose */

static struct epcard {
	u_short	iobase;
	u_short	irq;
	char	available;
	char	bus32bit;
} epcards[MAXEPCARDS];
static int nepcards;

static void
epaddcard(p, i, mode)
	short p;
	u_short i;
	char mode;
{
	if (nepcards >= sizeof(epcards)/sizeof(epcards[0]))
		return;
	epcards[nepcards].iobase = p;
	epcards[nepcards].irq = 1 << ((i == 2) ? 9 : i);
	epcards[nepcards].available = 1;
	epcards[nepcards].bus32bit = mode;
	nepcards++;
}
	
/*
 * 3c579 cards on the EISA bus are probed by their slot number. 3c509
 * cards on the ISA bus are probed in ethernet address order. The probe
 * sequence requires careful orchestration, and we'd like like to allow
 * the irq and base address to be wildcarded. So, we probe all the cards
 * the first time epprobe() is called. On subsequent calls we look for
 * matching cards.
 */
int
epprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ep_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	static int probed;
	int slot, iobase, i;
	u_short vendor, model;
	u_short k, k2;

	if (!probed) {
		probed = 1;

		/* find all EISA cards */
		for (slot = 1; slot < 16; slot++) {
			iobase = 0x1000 * slot;

			vendor = htons(inw(iobase + EISA_VENDOR));
			if (vendor != MFG_ID)
				continue;

			model = htons(inw(iobase + EISA_MODEL));
			if ((model & 0xfff0) != PROD_ID) {
#ifndef trusted
				printf("epprobe: ignoring model %04x\n", model);
#endif
				continue;
			}

			printf("epprobe: resetting card\n");

			outb(iobase + EISA_CONTROL, EISA_ENABLE | EISA_RESET);
			delay(10);
			outb(iobase + EISA_CONTROL, EISA_ENABLE);
			/* Wait for reset? */
			delay(1000);

			k = inw(iobase + EP_W0_ADDRESS_CFG);
			k = (k & 0x1f) * 0x10 + 0x200;
			k2 = inw(iobase + EP_W0_RESOURCE_CFG);
			k2 >>= 12;
			epaddcard(iobase, k2, 0);
		}

		/* find all isa cards */
#if 0
		outw(BASE + EP_COMMAND, GLOBAL_RESET);
#endif
		delay(1000);
		elink_reset();	/* global reset to ELINK_ID_PORT */
		delay(1000);

		for (slot = 0; slot < 10; slot++) {
			outb(ELINK_ID_PORT, 0x00);
			elink_idseq(ELINK_509_POLY);
			delay(1000);

			vendor =
			    htons(epreadeeprom(ELINK_ID_PORT, EEPROM_MFG_ID));
			if (vendor != MFG_ID)
				continue;

			model =
			    htons(epreadeeprom(ELINK_ID_PORT, EEPROM_PROD_ID));
			if ((model & 0xfff0) != PROD_ID) {
#ifndef trusted
				printf("epprobe: ignoring model %04x\n", model);
#endif
				continue;
			}

			k = epreadeeprom(ELINK_ID_PORT, EEPROM_ADDR_CFG);
			k = (k & 0x1f) * 0x10 + 0x200;

			k2 = epreadeeprom(ELINK_ID_PORT, EEPROM_RESOURCE_CFG);
			k2 >>= 12;
			epaddcard(k, k2, 0);

			/* so card will not respond to contention again */
			outb(ELINK_ID_PORT, TAG_ADAPTER_0 + 1);

			/*
			 * XXX: this should probably not be done here
			 * because it enables the drq/irq lines from
			 * the board. Perhaps it should be done after
			 * we have checked for irq/drq collisions?
			 */
			outb(ELINK_ID_PORT, ACTIVATE_ADAPTER_TO_CONFIG);
		}
		/* XXX should we sort by ethernet address? */
	}

	for (i = 0; i < nepcards; i++) {
		if (epcards[i].available == 0)
			continue;
		if (ia->ia_iobase != IOBASEUNK &&
		    ia->ia_iobase != epcards[i].iobase)
			continue;
		if (ia->ia_irq != IRQUNK &&
		    ia->ia_irq != epcards[i].irq)
			continue;
		goto good;
	}
	return 0;

good:
	epcards[i].available = 0;
	sc->bus32bit = epcards[i].bus32bit;
	ia->ia_iobase = epcards[i].iobase;
	ia->ia_irq = epcards[i].irq;
	ia->ia_iosize = 0x10;
	ia->ia_msize = 0;
	return 1;
}

static void
epattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ep_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	u_short i;

	printf(": ");

	sc->ep_iobase = ia->ia_iobase;

	sc->ep_connectors = 0;
	i = inw(ia->ia_iobase + EP_W0_CONFIG_CTRL);
	if (i & IS_AUI) {
		printf("aui");
		sc->ep_connectors |= AUI;
	}
	if (i & IS_BNC) {
		if (sc->ep_connectors)
			printf("/");
		printf("bnc");
		sc->ep_connectors |= BNC;
	}
	if (i & IS_UTP) {
		if (sc->ep_connectors)
			printf("/");
		printf("utp");
		sc->ep_connectors |= UTP;
	}
	if (!sc->ep_connectors)
		printf("no connectors!");

	/*
	 * Read the station address from the eeprom
	 */
	for (i = 0; i < 3; i++) {
		u_short *p;
		GO_WINDOW(0);
		if (epbusyeeprom(sc))
			return;
		outw(BASE + EP_W0_EEPROM_COMMAND, READ_EEPROM | i);
		if (epbusyeeprom(sc))
			return;
		p = (u_short *) & sc->sc_arpcom.ac_enaddr[i * 2];
		*p = htons(inw(BASE + EP_W0_EEPROM_DATA));
		GO_WINDOW(2);
		outw(BASE + EP_W2_ADDR_0 + (i * 2), ntohs(*p));
	}
	printf(" address %s\n", ether_sprintf(sc->sc_arpcom.ac_enaddr));

	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = epcd.cd_name;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	ifp->if_output = ether_output;
	ifp->if_start = epstart;
	ifp->if_ioctl = epioctl;
	ifp->if_watchdog = epwatchdog;

	if_attach(ifp);
	ether_ifattach(ifp);

#if NBPFILTER > 0
	bpfattach(&sc->sc_arpcom.ac_if.if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->tx_start_thresh = 20;	/* probably a good starting point. */

	sc->sc_ih.ih_fun = epintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_NET;
	intr_establish(ia->ia_irq, &sc->sc_ih);
}

/*
 * The order in here seems important. Otherwise we may not receive
 * interrupts. ?!
 */
static void
epinit(sc)
	register struct ep_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int     s, i;

	if (ifp->if_addrlist == 0)
		return;

	s = splimp();
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;

	GO_WINDOW(0);
	outw(BASE + EP_W0_CONFIG_CTRL, 0);	/* Disable the card */

	outw(BASE + EP_W0_CONFIG_CTRL, ENABLE_DRQ_IRQ);	/* Enable the card */

	GO_WINDOW(2);
	for (i = 0; i < 6; i++)	/* Reload the ether_addr. */
		outb(BASE + EP_W2_ADDR_0 + i, sc->sc_arpcom.ac_enaddr[i]);

	outw(BASE + EP_COMMAND, RX_RESET);
	outw(BASE + EP_COMMAND, TX_RESET);

	GO_WINDOW(1);		/* Window 1 is operating window */
	for (i = 0; i < 31; i++)
		inb(BASE + EP_W1_TX_STATUS);

	outw(BASE + EP_COMMAND, SET_RD_0_MASK | S_CARD_FAILURE | S_RX_COMPLETE |
	    S_TX_COMPLETE | S_TX_AVAIL);
	outw(BASE + EP_COMMAND, SET_INTR_MASK | S_CARD_FAILURE | S_RX_COMPLETE |
	    S_TX_COMPLETE | S_TX_AVAIL);
	/*
	 * attemp to get rid of stray interrupts. on the i386 this isn't
	 * totally possible since they may queue.
	 */
	outw(BASE + EP_COMMAND, ACK_INTR | 0xff);

	epsetfilter(sc);
	epsetlink(sc);

	outw(BASE + EP_COMMAND, RX_ENABLE);
	outw(BASE + EP_COMMAND, TX_ENABLE);

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;	/* just in case */
	epmbuffill(sc);

	epstart(ifp);
	splx(s);
}

static void
epsetfilter(sc)
	register struct ep_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	GO_WINDOW(1);		/* Window 1 is operating window */
	outw(BASE + EP_COMMAND, SET_RX_FILTER |
	    FIL_INDIVIDUAL | FIL_BRDCST |
	    ((ifp->if_flags & IFF_MULTICAST) ? FIL_MULTICAST : 0 ) |
	    ((ifp->if_flags & IFF_PROMISC) ? FIL_PROMISC : 0 ));
}

static void
epsetlink(sc)
	register struct ep_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	/*
	 * you can `ifconfig (link0|-link0) ep0' to get the following
	 * behaviour:
	 *	-link0	disable AUI/UTP. enable BNC.
	 *	link0	disable BNC. enable AUI.
	 *	link1	if the card has a UTP connector, and link0 is
	 *		set too, then you get the UTP port.
	 */
	GO_WINDOW(4);
	outw(BASE + EP_W4_MEDIA_TYPE, DISABLE_UTP);
	GO_WINDOW(1);
	if (!(ifp->if_flags & IFF_LINK0) && (sc->ep_connectors & BNC)) {
		outw(BASE + EP_COMMAND, START_TRANSCEIVER);
		delay(1000);
	}
	if (ifp->if_flags & IFF_LINK0) {
		outw(BASE + EP_COMMAND, STOP_TRANSCEIVER);
		delay(1000);
		if((ifp->if_flags & IFF_LINK1) && (sc->ep_connectors & UTP)) {
			GO_WINDOW(4);
			outw(BASE + EP_W4_MEDIA_TYPE, ENABLE_UTP);
			GO_WINDOW(1);
		}
	}
}

static const char padmap[] = {0, 3, 2, 1};

static int
epstart(ifp)
	struct ifnet *ifp;
{
	register struct ep_softc *sc = epcd.cd_devs[ifp->if_unit];
	struct mbuf *m, *top;
	int     s, sh, len, pad;

	s = splimp();
	if (sc->sc_arpcom.ac_if.if_flags & IFF_OACTIVE) {
		splx(s);
		return (0);
	}

startagain:
	/* Sneak a peek at the next packet */
	m = sc->sc_arpcom.ac_if.if_snd.ifq_head;
	if (m == 0) {
		splx(s);
		return (0);
	}
#if 0
	len = m->m_pkthdr.len;
#else
	for (len = 0, top = m; m; m = m->m_next)
		len += m->m_len;
#endif

	pad = padmap[len & 3];

	/*
	 * The 3c509 automatically pads short packets to minimum ethernet
	 * length, but we drop packets that are too large. Perhaps we should
	 * truncate them instead?
	 */
	if (len + pad > ETHER_MAX_LEN) {
		/* packet is obviously too large: toss it */
		+sc->sc_arpcom.ac_if.if_oerrors;
		IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m);
		m_freem(m);
		goto readcheck;
	}

	if (inw(BASE + EP_W1_FREE_TX) < len + pad + 4) {
		outw(BASE + EP_COMMAND, SET_TX_AVAIL_THRESH | (len + pad + 4));
		/* not enough room in FIFO */
		sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;
		splx(s);
		return (0);
	} else {
		outw(BASE + EP_COMMAND, SET_TX_AVAIL_THRESH | 2044);
	}

	IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m);
	if (m == 0) {		/* not really needed */
		splx(s);
		return (0);
	}
	outw(BASE + EP_COMMAND, SET_TX_START_THRESH |
	    (len / 4 + sc->tx_start_thresh));

	outw(BASE + EP_W1_TX_PIO_WR_1, len);
	outw(BASE + EP_W1_TX_PIO_WR_1, 0xffff);	/* Second dword meaningless */

	sh = splhigh();

	for (top = m; m != 0; m = m->m_next) {
		if (sc->bus32bit) {
			outsl(BASE + EP_W1_TX_PIO_WR_1, mtod(m, caddr_t),
			    m->m_len/4);
			if (m->m_len & 3)
				outsb(BASE + EP_W1_TX_PIO_WR_1,
				    mtod(m, caddr_t) + m->m_len/4,
				    m->m_len & 3);
		} else {
			outsw(BASE + EP_W1_TX_PIO_WR_1, mtod(m, caddr_t), m->m_len/2);
			if (m->m_len & 1)
				outb(BASE + EP_W1_TX_PIO_WR_1,
				    *(mtod(m, caddr_t) + m->m_len - 1));
		}
	}

	while (pad--)
		outb(BASE + EP_W1_TX_PIO_WR_1, 0);	/* Padding */

	splx(sh);

#if NBPFILTER > 0
	if (sc->sc_arpcom.ac_if.if_bpf)
		bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, top);
#endif

	m_freem(top);
	++sc->sc_arpcom.ac_if.if_opackets;

	/*
	 * Is another packet coming in? We don't want to overflow the
	 * tiny RX fifo.
	 */
readcheck:
	if (inw(BASE + EP_W1_RX_STATUS) & RX_BYTES_MASK) {
		splx(s);
		return (0);
	}
	goto startagain;
}


static void
eptxstat(sc)
	register struct ep_softc *sc;
{
	int i;
	/*
	 * We need to read+write TX_STATUS until we get a 0 status
	 * in order to turn off the interrupt flag.
	 */
	while ((i = inb(BASE + EP_W1_TX_STATUS)) & TXS_COMPLETE) {
		outb(BASE + EP_W1_TX_STATUS, 0x0);

		if (i & TXS_JABBER) {
			++sc->sc_arpcom.ac_if.if_oerrors;
			untimeout(epmbuffill, sc);
			if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
				printf("%s: jabber (%x)\n",
				       sc->sc_dev.dv_xname, i);
			epreset(sc);
		} else if (i & TXS_UNDERRUN) {
			++sc->sc_arpcom.ac_if.if_oerrors;
			untimeout(epmbuffill, sc);
			if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
				printf("%s: fifo underrun (%x) @%d\n",
				       sc->sc_dev.dv_xname, i,
				       sc->tx_start_thresh);
			if (sc->tx_succ_ok < 100)
				    sc->tx_start_thresh = min(ETHER_MAX_LEN,
					    sc->tx_start_thresh + 20);
			sc->tx_succ_ok = 0;
			epreset(sc);
		} else if (i & TXS_MAX_COLLISION) {
			++sc->sc_arpcom.ac_if.if_collisions;
			outw(BASE + EP_COMMAND, TX_ENABLE);
			sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
		} else
			sc->tx_succ_ok = (sc->tx_succ_ok+1) & 127;
			
	}
}

int
epintr(sc)
	register struct ep_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	u_short		status;
	int		i, ret = 0;

	status = inw(BASE + EP_STATUS) &
	    (S_TX_COMPLETE | S_TX_AVAIL | S_RX_COMPLETE | S_CARD_FAILURE);
	while (status) {
		ret = 1;
		/* important that we do this first. */
		outw(BASE + EP_COMMAND, ACK_INTR | status);

		if (status & S_RX_COMPLETE)
			epread(sc);
		if (status & S_TX_AVAIL) {
			sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
			epstart(&sc->sc_arpcom.ac_if);
		}
		if (status & S_CARD_FAILURE) {
			printf("%s: adapter failure (%x)\n",
			    sc->sc_dev.dv_xname, status);
			outw(BASE + EP_COMMAND, C_INTR_LATCH);
			epreset(sc);
			return (1);
		}
		if (status & S_TX_COMPLETE) {
			eptxstat(sc);
			epstart(ifp);
		}
		status = inw(BASE + EP_STATUS) &
		    (S_TX_COMPLETE | S_TX_AVAIL | S_RX_COMPLETE | S_CARD_FAILURE);
	}	
	/* no more interrupts */
	outw(BASE + EP_COMMAND, C_INTR_LATCH);
	return (ret);
}

static void
epread(sc)
	register struct ep_softc *sc;
{
	struct ether_header *eh;
	struct mbuf *mcur, *m, *m0, *top;
	int     totlen, lenthisone;
	int     save_totlen, sh;

	totlen = inw(BASE + EP_W1_RX_STATUS);
	top = 0;

	if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG) {
		int err = totlen & ERR_MASK;
		char *s = NULL;

		if (totlen & ERR_INCOMPLETE)
			s = "incomplete packet";
		else if (err == ERR_OVERRUN)
			s = "packet overrun";
		else if (err == ERR_RUNT)
			s = "runt packet";
		else if (err == ERR_ALIGNMENT)
			s = "bad alignment";
		else if (err == ERR_CRC)
			s = "bad crc";
		else if (err == ERR_OVERSIZE)
			s = "oversized packet";
		else if (err == ERR_DRIBBLE)
			s = "dribble bits";

		if (s)
			printf("%s: %s\n", sc->sc_dev.dv_xname, s);
	}

	if (totlen & (ERR_INCOMPLETE|ERR_RX)) {
		++sc->sc_arpcom.ac_if.if_ierrors;
		goto out;
	}

	save_totlen = totlen &= RX_BYTES_MASK;	/* Lower 11 bits = RX bytes. */

	m = sc->mb[sc->next_mb];
	sc->mb[sc->next_mb] = 0;

	if (m == 0) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto out;
	} else {		/* Convert one of our saved mbuf's */
		sc->next_mb = (sc->next_mb + 1) % MAX_MBS;
		m->m_data = m->m_pktdat;
		m->m_flags = M_PKTHDR;
	}

	top = m0 = m;		/* We assign top so we can "goto out" */
#define EROUND  ((sizeof(struct ether_header) + 3) & ~3)
#define EOFF    (EROUND - sizeof(struct ether_header))
	m0->m_data += EOFF;

	sh = splhigh();

	/* Read what should be the header. */
	insw(BASE + EP_W1_RX_PIO_RD_1,
	    mtod(m0, caddr_t), sizeof(struct ether_header) / 2);
	m->m_len = sizeof(struct ether_header);
	totlen -= sizeof(struct ether_header);
	eh = mtod(m, struct ether_header *);
	while (totlen > 0) {
		lenthisone = min(totlen, M_TRAILINGSPACE(m));
		if (lenthisone == 0) {	/* no room in this one */
			mcur = m;
			m = sc->mb[sc->next_mb];
			sc->mb[sc->next_mb] = 0;
			if (!m) {
				MGET(m, M_DONTWAIT, MT_DATA);
				if (m == 0) {
					splx(sh);
					goto out;
				}
			} else {
				timeout(epmbuffill, sc, 1);
				sc->next_mb = (sc->next_mb + 1) % MAX_MBS;
			}
			if (totlen >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);
			m->m_len = 0;
			mcur->m_next = m;
			lenthisone = min(totlen, M_TRAILINGSPACE(m));
		}
		if (sc->bus32bit) {
			insl(BASE + EP_W1_RX_PIO_RD_1, mtod(m, caddr_t) + m->m_len,
			    lenthisone / 4);
			m->m_len += (lenthisone & ~3);
			if (lenthisone & 3)
				insb(BASE + EP_W1_RX_PIO_RD_1,
				    mtod(m, caddr_t) + m->m_len,
				    lenthisone & 3);
			m->m_len += (lenthisone & 3);
		} else {
			insw(BASE + EP_W1_RX_PIO_RD_1, mtod(m, caddr_t) + m->m_len,
			    lenthisone / 2);
			m->m_len += lenthisone;
			if (lenthisone & 1)
				*(mtod(m, caddr_t) + m->m_len - 1) = inb(BASE + EP_W1_RX_PIO_RD_1);
		}
		totlen -= lenthisone;
	}
	top = m0;
	top->m_pkthdr.len = save_totlen;
	top->m_pkthdr.rcvif = &sc->sc_arpcom.ac_if;
	outw(BASE + EP_COMMAND, RX_DISCARD_TOP_PACK);
	splx(sh);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;
	++sc->sc_arpcom.ac_if.if_ipackets;
#if NBPFILTER > 0
	if (sc->sc_arpcom.ac_if.if_bpf) {
		bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, top);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 &&
		    bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
			 sizeof(eh->ether_dhost)) != 0) {
			m_freem(top);
			return;
		}
	}
#endif
	m_adj(top, sizeof(struct ether_header));
	ether_input(&sc->sc_arpcom.ac_if, eh, top);
	return;

out:	outw(BASE + EP_COMMAND, RX_DISCARD_TOP_PACK);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;
	if (top)
		m_freem(top);
}


/*
 * Look familiar?
 */
static int
epioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int     cmd;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *) data;
	struct ep_softc *sc = epcd.cd_devs[ifp->if_unit];
	struct ifreq *ifr = (struct ifreq *) data;
	int error = 0;

	switch (cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			epinit(sc);	/* before arpwhohas */
			((struct arpcom *) ifp)->ac_ipaddr = IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *) ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)(sc->sc_arpcom.ac_enaddr);
			else {
				ifp->if_flags &= ~IFF_RUNNING;
				bcopy(ina->x_host.c_host,
				    sc->sc_arpcom.ac_enaddr,
				    sizeof(sc->sc_arpcom.ac_enaddr));
			}
			epinit(sc);
			break;
		    }
#endif
		default:
			epinit(sc);
			break;
		}
		break;
	case SIOCSIFFLAGS:
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			ifp->if_flags &= ~IFF_RUNNING;
			epstop(sc);
			epmbufempty(sc);
			break;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    (ifp->if_flags & IFF_RUNNING) == 0) {
			epinit(sc);
		} else {
			/*
			 * deal with flags changes:
			 * IFF_MULTICAST, IFF_PROMISC,
			 * IFF_LINK0, IFF_LINK1,
			 */
			epsetfilter(sc);
			epsetlink(sc);
		}
		break;
#ifdef notdef
	case SIOCGHWADDR:
		bcopy((caddr_t) sc->sc_addr, (caddr_t) &ifr->ifr_data,
		    sizeof(sc->sc_addr));
		break;
#endif
	default:
		error = EINVAL;
	}
	return (error);
}

static void
epreset(sc)
	struct ep_softc *sc;
{
	int s = splimp();

	epstop(sc);
	epinit(sc);
	splx(s);
}

static int
epwatchdog(unit)
	int     unit;
{
	register struct ep_softc *sc = epcd.cd_devs[unit];

	log(LOG_ERR, "%s: watchdog\n", sc->sc_dev.dv_xname);
	epreset(sc);
	return 0;
}

static void
epstop(sc)
	register struct ep_softc *sc;
{

	outw(BASE + EP_COMMAND, RX_DISABLE);
	outw(BASE + EP_COMMAND, RX_DISCARD_TOP_PACK);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;
	outw(BASE + EP_COMMAND, TX_DISABLE);
	outw(BASE + EP_COMMAND, STOP_TRANSCEIVER);
	outw(BASE + EP_COMMAND, RX_RESET);
	outw(BASE + EP_COMMAND, TX_RESET);
	outw(BASE + EP_COMMAND, C_INTR_LATCH);
	outw(BASE + EP_COMMAND, SET_RD_0_MASK);
	outw(BASE + EP_COMMAND, SET_INTR_MASK);
	outw(BASE + EP_COMMAND, SET_RX_FILTER);
}

/*
 * We get eeprom data from the id_port given an offset into the
 * eeprom.  Basically; after the ID_sequence is sent to all of
 * the cards; they enter the ID_CMD state where they will accept
 * command requests. 0x80-0xbf loads the eeprom data.  We then
 * read the port 16 times and with every read; the cards check
 * for contention (ie: if one card writes a 0 bit and another
 * writes a 1 bit then the host sees a 0. At the end of the cycle;
 * each card compares the data on the bus; if there is a difference
 * then that card goes into ID_WAIT state again). In the meantime;
 * one bit of data is returned in the AX register which is conveniently
 * returned to us by inb().  Hence; we read 16 times getting one
 * bit of data with each read.
 */
static u_short
epreadeeprom(id_port, offset)
	int     id_port;
	int     offset;
{
	int     i, data = 0;

	outb(id_port, 0x80 + offset);
	delay(1000);
	for (i = 0; i < 16; i++)
		data = (data << 1) | (inw(id_port) & 1);
	return (data);
}

static int
epbusyeeprom(sc)
	struct ep_softc *sc;
{
	int     i = 100, j;

	while (i--) {
		j = inw(BASE + EP_W0_EEPROM_COMMAND);
		if (j & EEPROM_BUSY)
			delay(100);
		else
			break;
	}
	if (!i) {
		printf("\n%s: eeprom failed to come ready\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}
	if (j & EEPROM_TST_MODE) {
		printf("\n%s: 3c509 is in test mode -- erase pencil mark!\n",
		    sc->sc_dev.dv_xname);
		return (1);
	}
	return (0);
}

static void
epmbuffill(sc)
	struct ep_softc *sc;
{
	int s, i;

	s = splimp();
	i = sc->last_mb;
	do {
		if (sc->mb[i] == NULL)
			MGET(sc->mb[i], M_DONTWAIT, MT_DATA);
		if (sc->mb[i] == NULL)
			break;
		i = (i + 1) % MAX_MBS;
	} while (i != sc->next_mb);
	sc->last_mb = i;
	splx(s);
}

static void
epmbufempty(sc)
	struct ep_softc *sc;
{
	int s, i;

	s = splimp();
	for (i = 0; i<MAX_MBS; i++) {
		if (sc->mb[i]) {
			m_freem(sc->mb[i]);
			sc->mb[i] = NULL;
		}
	}
	sc->last_mb = sc->next_mb = 0;
	untimeout(epmbuffill, sc);
	splx(s);
}
