/*	$NetBSD: if_ep.c,v 1.68 1995/01/22 07:37:28 mycroft Exp $	*/

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
	int	ep_iobase;		/* i/o bus address		*/
	char    ep_connectors;		/* Connectors on this card.	*/
#define MAX_MBS	8			/* # of mbufs we keep around	*/
	struct mbuf *mb[MAX_MBS];	/* spare mbuf storage.		*/
	int	next_mb;		/* Which mbuf to use next. 	*/
	int	last_mb;		/* Last mbuf.			*/
	int	tx_start_thresh;	/* Current TX_start_thresh.	*/
	int	tx_succ_ok;		/* # packets sent in sequence   */
					/* w/o underrun			*/
	char	bus32bit;		/* 32bit access possible	*/
};

static int epprobe __P((struct device *, void *, void *));
static void epattach __P((struct device *, struct device *, void *));

struct cfdriver epcd = {
	NULL, "ep", epprobe, epattach, DV_IFNET, sizeof(struct ep_softc)
};

int epintr __P((struct ep_softc *));
static void epxstat __P((struct ep_softc *));
static int epstatus __P((struct ep_softc *));
static void epinit __P((struct ep_softc *));
static int epioctl __P((struct ifnet *, u_long, caddr_t));
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
	int	iobase;
	int	irq;
	char	available;
	char	bus32bit;
} epcards[MAXEPCARDS];
static int nepcards;

static void
epaddcard(iobase, irq, bus32bit)
	int iobase;
	int irq;
	char bus32bit;
{

	if (nepcards >= MAXEPCARDS)
		return;
	epcards[nepcards].iobase = iobase;
	epcards[nepcards].irq = (irq == 2) ? 9 : irq;
	epcards[nepcards].available = 1;
	epcards[nepcards].bus32bit = bus32bit;
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
epprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct ep_softc *sc = match;
	struct isa_attach_args *ia = aux;
	static int probed;
	int slot, iobase, i;
	u_short vendor, model;
	int k, k2;

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

			outb(iobase + EISA_CONTROL, EISA_ENABLE | EISA_RESET);
			delay(10);
			outb(iobase + EISA_CONTROL, EISA_ENABLE);
			/* Wait for reset? */
			delay(1000);

			k = inw(iobase + EP_W0_ADDRESS_CFG);
			k = (k & 0x1f) * 0x10 + 0x200;

			k2 = inw(iobase + EP_W0_RESOURCE_CFG);
			k2 >>= 12;
			epaddcard(iobase, k2, 1);
		}

		for (slot = 0; slot < 10; slot++) {
			elink_reset();
			elink_idseq(ELINK_509_POLY);

			/* Untag all the adapters so they will talk to us. */
			if (slot == 0)
				outb(ELINK_ID_PORT, TAG_ADAPTER + 0);

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
			outb(ELINK_ID_PORT, TAG_ADAPTER + 1);

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

	GO_WINDOW(0);

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
		u_short x;
		if (epbusyeeprom(sc))
			return;
		outw(BASE + EP_W0_EEPROM_COMMAND, READ_EEPROM | i);
		if (epbusyeeprom(sc))
			return;
		x = inw(BASE + EP_W0_EEPROM_DATA);
		sc->sc_arpcom.ac_enaddr[(i << 1)] = x >> 8;
		sc->sc_arpcom.ac_enaddr[(i << 1) + 1] = x;
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
	bpfattach(&sc->sc_arpcom.ac_if.if_bpf, ifp, DLT_EN10MB,
		  sizeof(struct ether_header));
#endif

	sc->tx_start_thresh = 20;	/* probably a good starting point. */

	sc->sc_ih.ih_fun = epintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_NET;
	intr_establish(ia->ia_irq, IST_EDGE, &sc->sc_ih);
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
	int s, i;

	/* Address not known. */
	if (ifp->if_addrlist == 0)
		return;

	s = splimp();

	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;

	GO_WINDOW(0);
	outw(BASE + EP_W0_CONFIG_CTRL, 0);		/* Disable the card */
	outw(BASE + EP_W0_CONFIG_CTRL, ENABLE_DRQ_IRQ);	/* Enable the card */

	GO_WINDOW(2);
	for (i = 0; i < 6; i++)	/* Reload the ether_addr. */
		outb(BASE + EP_W2_ADDR_0 + i, sc->sc_arpcom.ac_enaddr[i]);

	outw(BASE + EP_COMMAND, RX_RESET);
	outw(BASE + EP_COMMAND, TX_RESET);

	GO_WINDOW(1);		/* Window 1 is operating window */
	for (i = 0; i < 31; i++)
		inb(BASE + EP_W1_TX_STATUS);

	outw(BASE + EP_COMMAND, SET_RD_0_MASK | S_CARD_FAILURE | 
				S_RX_COMPLETE | S_TX_COMPLETE | S_TX_AVAIL);
	outw(BASE + EP_COMMAND, SET_INTR_MASK | S_CARD_FAILURE |
				S_RX_COMPLETE | S_TX_COMPLETE | S_TX_AVAIL);

	/*
	 * Attempt to get rid of any stray interrupts that occured during
	 * configuration.  On the i386 this isn't possible because one may
	 * already be queued.  However, a single stray interrupt is
	 * unimportant.
	 */
	outw(BASE + EP_COMMAND, ACK_INTR | 0xff);

	epsetfilter(sc);
	epsetlink(sc);

	outw(BASE + EP_COMMAND, RX_ENABLE);
	outw(BASE + EP_COMMAND, TX_ENABLE);

	epmbuffill(sc);

	/* Interface is now `running', with no output active. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* Attempt to start output, if any. */
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
	if (!(ifp->if_flags & IFF_LINK0) && (sc->ep_connectors & BNC)) {
		outw(BASE + EP_COMMAND, START_TRANSCEIVER);
		delay(1000);
	}
	if (ifp->if_flags & IFF_LINK0) {
		outw(BASE + EP_COMMAND, STOP_TRANSCEIVER);
		delay(1000);
		if ((ifp->if_flags & IFF_LINK1) && (sc->ep_connectors & UTP))
			outw(BASE + EP_W4_MEDIA_TYPE, ENABLE_UTP);
	}
	GO_WINDOW(1);
}

/*
 * Start outputting on the interface.
 * Always called as splimp().
 */
static int
epstart(ifp)
	struct ifnet *ifp;
{
	register struct ep_softc *sc = epcd.cd_devs[ifp->if_unit];
	struct mbuf *m, *m0;
	int sh, len, pad;

	if (sc->sc_arpcom.ac_if.if_flags & IFF_OACTIVE)
		return;

startagain:
	/* Sneak a peek at the next packet */
	m0 = sc->sc_arpcom.ac_if.if_snd.ifq_head;
	if (m0 == 0)
		return;
#if 0
	len = m0->m_pkthdr.len;
#else
	for (len = 0, m = m0; m; m = m->m_next)
		len += m->m_len;
#endif

	pad = (4 - len) & 3;

	/*
	 * The 3c509 automatically pads short packets to minimum ethernet
	 * length, but we drop packets that are too large. Perhaps we should
	 * truncate them instead?
	 */
	if (len + pad > ETHER_MAX_LEN) {
		/* packet is obviously too large: toss it */
		++sc->sc_arpcom.ac_if.if_oerrors;
		IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m0);
		m_freem(m0);
		goto readcheck;
	}

	if (inw(BASE + EP_W1_FREE_TX) < len + pad + 4) {
		outw(BASE + EP_COMMAND, SET_TX_AVAIL_THRESH | (len + pad + 4));
		/* not enough room in FIFO */
		sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;
		return;
	} else {
		outw(BASE + EP_COMMAND, SET_TX_AVAIL_THRESH | 2044);
	}

	IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m0);
	if (m0 == 0)		/* not really needed */
		return;

	outw(BASE + EP_COMMAND, SET_TX_START_THRESH |
	    (len / 4 + sc->tx_start_thresh));

#if NBPFILTER > 0
	if (sc->sc_arpcom.ac_if.if_bpf)
		bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, m0);
#endif

	/*
	 * Do the output at splhigh() so that an interrupt from another device
	 * won't cause a FIFO underrun.
	 */
	sh = splhigh();

	outw(BASE + EP_W1_TX_PIO_WR_1, len);
	outw(BASE + EP_W1_TX_PIO_WR_1, 0xffff);	/* Second dword meaningless */
	if (sc->bus32bit) {
		for (m = m0; m; ) {
			if (m->m_len > 3)
				outsl(BASE + EP_W1_TX_PIO_WR_1,
				      mtod(m, caddr_t), m->m_len / 4);
			if (m->m_len & 3)
				outsb(BASE + EP_W1_TX_PIO_WR_1,
				      mtod(m, caddr_t) + (m->m_len & ~3),
				      m->m_len & 3);
			MFREE(m, m0);
			m = m0;
		}
	} else {
		for (m = m0; m; ) {
			if (m->m_len > 1)
				outsw(BASE + EP_W1_TX_PIO_WR_1,
				      mtod(m, caddr_t), m->m_len / 2);
			if (m->m_len & 1)
				outb(BASE + EP_W1_TX_PIO_WR_1,
				     *(mtod(m, caddr_t) + m->m_len - 1));
			MFREE(m, m0);
			m = m0;
		}
	}
	while (pad--)
		outb(BASE + EP_W1_TX_PIO_WR_1, 0);

	splx(sh);

	++sc->sc_arpcom.ac_if.if_opackets;

readcheck:
	if ((inw(BASE + EP_W1_RX_STATUS) & ERR_INCOMPLETE) == 0) {
		/* We received a complete packet. */
		u_short status = inw(BASE + EP_STATUS);

		if ((status & S_INTR_LATCH) == 0) {
			/*
			 * No interrupt, read the packet and continue
			 * Is  this supposed to happen? Is my motherboard 
			 * completely busted?
			 */
			epread(sc);
		}
		else
			/* Got an interrupt, return so that it gets serviced. */
			return;
	}
	else {
		/* Check if we are stuck and reset [see XXX comment] */
		if (epstatus(sc)) {
			if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
				printf("%s: adapter reset\n",
				       sc->sc_dev.dv_xname);
			epreset(sc);
		}
	}

	goto startagain;
}


/*
 * XXX: The 3c509 card can get in a mode where both the fifo status bit
 *	FIFOS_RX_OVERRUN and the status bit ERR_INCOMPLETE are set
 *	We detect this situation and we reset the adapter.
 *	It happens at times when there is a lot of broadcast traffic
 *	on the cable (once in a blue moon).
 */
static int
epstatus(sc)
	register struct ep_softc *sc;
{
	u_short fifost;

	/*
	 * Check the FIFO status and act accordingly
	 */
	GO_WINDOW(4);
	fifost = inw(BASE + EP_W4_FIFO_DIAG);
	GO_WINDOW(1);

	if (fifost & FIFOS_RX_UNDERRUN) {
		if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
			printf("%s: RX underrun\n", sc->sc_dev.dv_xname);
		epreset(sc);
		return 0;
	}

	if (fifost & FIFOS_RX_STATUS_OVERRUN) {
		if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
			printf("%s: RX Status overrun\n", sc->sc_dev.dv_xname);
		return 1;
	}

	if (fifost & FIFOS_RX_OVERRUN) {
		if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
			printf("%s: RX overrun\n", sc->sc_dev.dv_xname);
		return 1;
	}

	if (fifost & FIFOS_TX_OVERRUN) {
		if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
			printf("%s: TX overrun\n", sc->sc_dev.dv_xname);
		epreset(sc);
		return 0;
	}

	return 0;
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
			if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
				printf("%s: jabber (%x)\n",
				       sc->sc_dev.dv_xname, i);
			epreset(sc);
		} else if (i & TXS_UNDERRUN) {
			++sc->sc_arpcom.ac_if.if_oerrors;
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
	u_short status;
	int ret = 0;

	for (;;) {
		status = inw(BASE + EP_STATUS);

		if ((status & S_INTR_LATCH) == 0)
			/* Not from this card; maybe a chained interrupt? */
			return;

		if ((status & (S_TX_COMPLETE | S_TX_AVAIL |
			       S_RX_COMPLETE | S_CARD_FAILURE)) == 0)
			break;

		ret = 1;

		/*
		 * Acknowledge any interrupts.  It's important that we do this
		 * first, since there would otherwise be a race condition.
		 * Due to the i386 interrupt queueing, we may get spurious
		 * interrupts occasionally.
		 */
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
	struct mbuf *mcur, *m, *m0;
	u_short totlen, mlen, save_totlen;
	int sh;

	totlen = inw(BASE + EP_W1_RX_STATUS);
again:
	m0 = 0;

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

	if (totlen & ERR_INCOMPLETE)
		return;

	if (totlen & ERR_RX) {
		++sc->sc_arpcom.ac_if.if_ierrors;
		goto abort;
	}

	save_totlen = totlen &= RX_BYTES_MASK;	/* Lower 11 bits = RX bytes. */

	m = sc->mb[sc->next_mb];
	sc->mb[sc->next_mb] = NULL;

	if (m == 0) {
		MGETHDR(m, M_DONTWAIT, MT_DATA);
		if (m == 0)
			goto abort;
	} else {
		/* Convert one of our saved mbuf's. */
		sc->next_mb = (sc->next_mb + 1) % MAX_MBS;
		m->m_data = m->m_pktdat;
		m->m_flags = M_PKTHDR;
	}

	m0 = m;
#define EROUND  ((sizeof(struct ether_header) + 3) & ~3)
#define EOFF    (EROUND - sizeof(struct ether_header))
	m0->m_data += EOFF;

	/*
	 * We read the packet at splhigh() so that an interrupt from another
	 * device doesn't cause the card's buffer to overflow while we're
	 * reading it.  We may still lose packets at other times.
	 */
	sh = splhigh();

	/* Read what should be the header. */
	insw(BASE + EP_W1_RX_PIO_RD_1,
	    mtod(m0, caddr_t), sizeof(struct ether_header) / 2);
	m0->m_len = sizeof(struct ether_header);
	totlen -= sizeof(struct ether_header);
	eh = mtod(m0, struct ether_header *);

	/* Read packet data. */
	while (totlen > 0) {
		mlen = min(totlen, M_TRAILINGSPACE(m));
		if (mlen < 4) {
			/* not enough room in this mbuf */
			mcur = m;
			m = sc->mb[sc->next_mb];
			sc->mb[sc->next_mb] = 0;
			if (m == 0) {
				MGET(m, M_DONTWAIT, MT_DATA);
				if (m == 0) {
					splx(sh);
					goto abort;
				}
			} else {
				timeout(epmbuffill, sc, 1);
				sc->next_mb = (sc->next_mb + 1) % MAX_MBS;
			}
			if (totlen >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);
			m->m_len = 0;
			mcur->m_next = m;
			mlen = min(totlen, M_TRAILINGSPACE(m));
		}
		if (sc->bus32bit) {
			if (mlen > 3) {
				mlen &= ~3;
				insl(BASE + EP_W1_RX_PIO_RD_1,
				     mtod(m, caddr_t) + m->m_len, mlen / 4);
			} else
				insb(BASE + EP_W1_RX_PIO_RD_1,
				     mtod(m, caddr_t) + m->m_len, mlen);
		} else {
			if (mlen > 1) {
				mlen &= ~1;
				insw(BASE + EP_W1_RX_PIO_RD_1,
				     mtod(m, caddr_t) + m->m_len, mlen / 2);
			} else
				*(mtod(m, caddr_t) + m->m_len) =
				    inb(BASE + EP_W1_RX_PIO_RD_1);
		}
		m->m_len += mlen;
		totlen -= mlen;
	}

	outw(BASE + EP_COMMAND, RX_DISCARD_TOP_PACK);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;

	splx(sh);

	m0->m_pkthdr.len = save_totlen;
	m0->m_pkthdr.rcvif = &sc->sc_arpcom.ac_if;

	++sc->sc_arpcom.ac_if.if_ipackets;

#if NBPFILTER > 0
	if (sc->sc_arpcom.ac_if.if_bpf) {
		bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, m0);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 &&
		    bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
			 sizeof(eh->ether_dhost)) != 0) {
			m_freem(m0);
			return;
		}
	}
#endif

	m_adj(m0, sizeof(struct ether_header));
	ether_input(&sc->sc_arpcom.ac_if, eh, m0);

	/*
	 * In periods of high traffic we can actually receive enough
	 * packets so that the fifo overrun bit will be set at this point,
	 * even though we just read a packet. In this case we
	 * are not going to receive any more interrupts. We check for
	 * this condition and read again until the fifo is not full.
	 * We could simplify this test by not using epstatus(), but
	 * rechecking the RX_STATUS register directly. This test could
	 * result in unnecessary looping in cases where there is a new
	 * packet but the fifo is not full, but it will not fix the
	 * stuck behavior.
	 *
	 * Even with this improvement, we still get packet overrun errors
	 * which are hurting performance. Maybe when I get some more time
	 * I'll modify epread() so that it can handle RX_EARLY interrupts.
	 */
	if (epstatus(sc)) {
		totlen = inw(BASE + EP_W1_RX_STATUS);
		/* Check if we are stuck and reset [see XXX comment] */
		if (totlen & ERR_INCOMPLETE) {
			if (sc->sc_arpcom.ac_if.if_flags & IFF_DEBUG)
				printf("%s: adapter reset\n",
				       sc->sc_dev.dv_xname);
			epreset(sc);
			return;
		}
		goto again;
	}

	return;

abort:
	outw(BASE + EP_COMMAND, RX_DISCARD_TOP_PACK);
	while (inw(BASE + EP_STATUS) & S_COMMAND_IN_PROGRESS)
		;
	if (m0)
		m_freem(m0);
}

static int
epioctl(ifp, command, data)
	register struct ifnet *ifp;
	u_long command;
	caddr_t data;
{
	struct ep_softc *sc = epcd.cd_devs[ifp->if_unit];
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
			epinit(sc);	/* before arpwhohas */
			/*
			 * See if another station has *our* IP address.
			 * i.e.: There is an address conflict! If a
			 * conflict exists, a message is sent to the
			 * console.
			 */
			sc->sc_arpcom.ac_ipaddr = IA_SIN(ifa)->sin_addr;
			arpwhohas(&sc->sc_arpcom, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)(sc->sc_arpcom.ac_enaddr);
			else
				bcopy(ina->x_host.c_host,
				    sc->sc_arpcom.ac_enaddr,
				    sizeof(sc->sc_arpcom.ac_enaddr));
			/* Set new address. */
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
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			epstop(sc);
			epmbufempty(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
			   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
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

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/* Update our multicast list. */
		error = (command == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_arpcom) :
		    ether_delmulti(ifr, &sc->sc_arpcom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			epreset(sc);
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
epreset(sc)
	struct ep_softc *sc;
{
	int s;

	s = splimp();
	untimeout(epmbuffill, sc);
	epstop(sc);
	epinit(sc);
	splx(s);
}

static int
epwatchdog(unit)
	int unit;
{
	register struct ep_softc *sc = epcd.cd_devs[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
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
	int i, data = 0;

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
	int i = 100, j;

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
		printf("\n%s: erase pencil mark, or disable plug-n-play mode!\n",
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
