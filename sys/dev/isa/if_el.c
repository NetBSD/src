/*	$NetBSD: if_el.c,v 1.27 1995/07/23 17:50:56 mycroft Exp $	*/

/*
 * Copyright (c) 1994, Matthew E. Kimmel.  Permission is hereby granted
 * to use, copy, modify and distribute this software provided that both
 * the copyright notice and this permission notice appear in all copies
 * of the software, derivative works or modified versions, and any
 * portions thereof.
 */

/*
 * 3COM Etherlink 3C501 device driver
 */

/*
 * Bugs/possible improvements:
 *	- Does not currently support DMA
 *	- Does not currently support multicasts
 */

#include "bpfilter.h"

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

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

#include <dev/isa/isavar.h>
#include <dev/isa/if_elreg.h>

#define ETHER_MIN_LEN	64
#define ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

/* for debugging convenience */
#ifdef EL_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

/*
 * per-line info and status
 */
struct el_softc {
	struct device sc_dev;
	void *sc_ih;

	struct arpcom sc_arpcom;	/* ethernet common */
	int sc_iobase;			/* base I/O addr */
};

/*
 * prototypes
 */
int elintr __P((void *));
static int el_init __P((struct el_softc *));
static int el_ioctl __P((struct ifnet *, u_long, caddr_t));
static void el_start __P((struct ifnet *));
static void el_watchdog __P((int));
static void el_reset __P((struct el_softc *));
static void el_stop __P((struct el_softc *));
static int el_xmit __P((struct el_softc *, int));
static inline void elread __P((struct el_softc *, caddr_t, int));
static struct mbuf *elget __P((caddr_t, int, struct ifnet *));
static inline void el_hardreset __P((struct el_softc *));

int elprobe __P((struct device *, void *, void *));
void elattach __P((struct device *, struct device *, void *));

/* isa_driver structure for autoconf */
struct cfdriver elcd = {
	NULL, "el", elprobe, elattach, DV_IFNET, sizeof(struct el_softc)
};

/*
 * Probe routine.
 *
 * See if the card is there and at the right place.
 * (XXX - cgd -- needs help)
 */
int
elprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct el_softc *sc = match;
	struct isa_attach_args *ia = aux;
	int iobase = ia->ia_iobase;
	u_char station_addr[ETHER_ADDR_LEN];
	int i;

	/* First check the base. */
	if (iobase < 0x280 || iobase > 0x3f0)
		return 0;

	/* Grab some info for our structure. */
	sc->sc_iobase = iobase;

	/*
	 * Now attempt to grab the station address from the PROM and see if it
	 * contains the 3com vendor code.
	 */
	dprintf(("Probing 3c501 at 0x%x...\n", iobase));

	/* Reset the board. */
	dprintf(("Resetting board...\n"));
	outb(iobase+EL_AC, EL_AC_RESET);
	delay(5);
	outb(iobase+EL_AC, 0);

	/* Now read the address. */
	dprintf(("Reading station address...\n"));
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		outb(iobase+EL_GPBL, i);
		station_addr[i] = inb(iobase+EL_EAW);
	}
	dprintf(("Address is %s\n", ether_sprintf(station_addr)));

	/*
	 * If the vendor code is ok, return a 1.  We'll assume that whoever
	 * configured this system is right about the IRQ.
	 */
	if (station_addr[0] != 0x02 || station_addr[1] != 0x60 ||
	    station_addr[2] != 0x8c) {
		dprintf(("Bad vendor code.\n"));
		return 0;
	}

	dprintf(("Vendor code ok.\n"));
	/* Copy the station address into the arpcom structure. */
	bcopy(station_addr, sc->sc_arpcom.ac_enaddr, ETHER_ADDR_LEN);

	ia->ia_iosize = 4;	/* XXX */
	ia->ia_msize = 0;
	return 1;
}

/*
 * Attach the interface to the kernel data structures.  By the time this is
 * called, we know that the card exists at the given I/O address.  We still
 * assume that the IRQ given is correct.
 */
void
elattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct el_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	dprintf(("Attaching %s...\n", sc->sc_dev.dv_xname));

	/* Stop the board. */
	el_stop(sc);

	/* Initialize ifnet structure. */
	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = elcd.cd_name;
	ifp->if_start = el_start;
	ifp->if_ioctl = el_ioctl;
	ifp->if_watchdog = el_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;

	/* Now we can attach the interface. */
	dprintf(("Attaching interface...\n"));
	if_attach(ifp);
	ether_ifattach(ifp);

	/* Print out some information for the user. */
	printf(": address %s\n", ether_sprintf(sc->sc_arpcom.ac_enaddr));

	/* Finally, attach to bpf filter if it is present. */
#if NBPFILTER > 0
	dprintf(("Attaching to BPF...\n"));
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->sc_ih = isa_intr_establish(ia->ia_irq, ISA_IST_EDGE, ISA_IPL_NET,
	    elintr, sc);

	dprintf(("elattach() finished.\n"));
}

/*
 * Reset interface.
 */
static void
el_reset(sc)
	struct el_softc *sc;
{
	int s;

	dprintf(("elreset()\n"));
	s = splimp();
	el_stop(sc);
	el_init(sc);
	splx(s);
}

/*
 * Stop interface.
 */
static void
el_stop(sc)
	struct el_softc *sc;
{

	outb(sc->sc_iobase+EL_AC, 0);
}

/*
 * Do a hardware reset of the board, and upload the ethernet address again in
 * case the board forgets.
 */
static inline void
el_hardreset(sc)
	struct el_softc *sc;
{
	int iobase = sc->sc_iobase;
	int i;

	outb(iobase+EL_AC, EL_AC_RESET);
	delay(5);
	outb(iobase+EL_AC, 0);

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		outb(iobase+i, sc->sc_arpcom.ac_enaddr[i]);
}

/*
 * Initialize interface.
 */
static int
el_init(sc)
	struct el_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int iobase = sc->sc_iobase;
	int s;

	s = splimp();

	/* First, reset the board. */
	el_hardreset(sc);

	/* Configure rx. */
	dprintf(("Configuring rx...\n"));
	if (ifp->if_flags & IFF_PROMISC)
		outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_PROMISC);
	else
		outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_ABROAD);
	outb(iobase+EL_RBC, 0);

	/* Configure TX. */
	dprintf(("Configuring tx...\n"));
	outb(iobase+EL_TXC, 0);

	/* Start reception. */
	dprintf(("Starting reception...\n"));
	outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);

	/* Set flags appropriately. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* And start output. */
	el_start(ifp);

	splx(s);
}

/*
 * Start output on interface.  Get datagrams from the queue and output them,
 * giving the receiver a chance between datagrams.  Call only from splimp or
 * interrupt level!
 */
static void
el_start(ifp)
	struct ifnet *ifp;
{
	struct el_softc *sc = elcd.cd_devs[ifp->if_unit];
	int iobase = sc->sc_iobase;
	struct mbuf *m, *m0;
	int s, i, off, retries;

	dprintf(("el_start()...\n"));
	s = splimp();

	/* Don't do anything if output is active. */
	if ((ifp->if_flags & IFF_OACTIVE) != 0) {
		splx(s);
		return;
	}

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * The main loop.  They warned me against endless loops, but would I
	 * listen?  NOOO....
	 */
	for (;;) {
		/* Dequeue the next datagram. */
		IF_DEQUEUE(&ifp->if_snd, m0);

		/* If there's nothing to send, return. */
		if (m0 == 0)
			break;

#if NBPFILTER > 0
		/* Give the packet to the bpf, if any. */
		if (ifp->if_bpf)
			bpf_mtap(ifp->if_bpf, m0);
#endif

		/* Disable the receiver. */
		outb(iobase+EL_AC, EL_AC_HOST);
		outb(iobase+EL_RBC, 0);

		/* Transfer datagram to board. */
		dprintf(("el: xfr pkt length=%d...\n", m0->m_pkthdr.len));
		off = EL_BUFSIZ - max(m0->m_pkthdr.len, ETHER_MIN_LEN);
		outb(iobase+EL_GPBL, off);
		outb(iobase+EL_GPBH, off >> 8);

		/* Copy the datagram to the buffer. */
		for (m = m0; m != 0; m = m->m_next)
			outsb(iobase+EL_BUF, mtod(m, caddr_t), m->m_len);

		m_freem(m0);

		/* Now transmit the datagram. */
		retries = 0;
		for (;;) {
			outb(iobase+EL_GPBL, off);
			outb(iobase+EL_GPBH, off >> 8);
			if (el_xmit(sc))
				break;
			/* Check out status. */
			i = inb(iobase+EL_TXS);
			dprintf(("tx status=0x%x\n", i));
			if ((i & EL_TXS_READY) == 0) {
				dprintf(("el: err txs=%x\n", i));
				ifp->if_oerrors++;
				if (i & (EL_TXS_COLL | EL_TXS_COLL16)) {
					if ((i & EL_TXC_DCOLL16) == 0 &&
					    retries < 15) {
						retries++;
						outb(iobase+EL_AC, EL_AC_HOST);
					}
				} else
					break;
			} else {
				ifp->if_opackets++;
				break;
			}
		}

		/*
		 * Now give the card a chance to receive.
		 * Gotta love 3c501s...
		 */
		(void)inb(iobase+EL_AS);
		outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
		splx(s);
		/* Interrupt here. */
		s = splimp();
	}

	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);
}

/*
 * This function actually attempts to transmit a datagram downloaded to the
 * board.  Call at splimp or interrupt, after downloading data!  Returns 0 on
 * success, non-0 on failure.
 */
static int
el_xmit(sc)
	struct el_softc *sc;
{
	int iobase = sc->sc_iobase;
	int i;

	dprintf(("el: xmit..."));
	outb(iobase+EL_AC, EL_AC_TXFRX);
	i = 20000;
	while ((inb(iobase+EL_AS) & EL_AS_TXBUSY) && (i > 0))
		i--;
	if (i == 0) {
		dprintf(("tx not ready\n"));
		sc->sc_arpcom.ac_if.if_oerrors++;
		return -1;
	}
	dprintf(("%d cycles.\n", 20000 - i));
	return 0;
}

/*
 * Controller interrupt.
 */
int
elintr(arg)
	void *arg;
{
	register struct el_softc *sc = arg;
	int iobase = sc->sc_iobase;
	int stat, rxstat, len;

	dprintf(("elintr: "));

	/* Check board status. */
	stat = inb(iobase+EL_AS);
	if (stat & EL_AS_RXBUSY) {
		(void)inb(iobase+EL_RXC);
		outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
		return 0;
	}

	for (;;) {
		rxstat = inb(iobase+EL_RXS);
		if (rxstat & EL_RXS_STALE)
			break;

		/* If there's an overflow, reinit the board. */
		if ((rxstat & EL_RXS_NOFLOW) == 0) {
			dprintf(("overflow.\n"));
			el_hardreset(sc);
		reset:
			/* Put board back into receive mode. */
			if (sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC)
				outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_PROMISC);
			else
				outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_ABROAD);
			(void)inb(iobase+EL_AS);
			outb(iobase+EL_RBC, 0);
			break;
		}

		/* Incoming packet. */
		len = inb(iobase+EL_RBL);
		len |= inb(iobase+EL_RBH) << 8;
		dprintf(("receive len=%d rxstat=%x ", len, rxstat));
		outb(iobase+EL_AC, EL_AC_HOST);

		/*
		 * If packet too short or too long, restore rx mode and return.
		 */
		if (len <= sizeof(struct ether_header) ||
		    len > ETHER_MAX_LEN)
			goto reset;

		sc->sc_arpcom.ac_if.if_ipackets++;

		/* Pass data up to upper levels. */
		elread(sc, len);

		/* Is there another packet? */
		stat = inb(iobase+EL_AS);

		/* If so, do it all again. */
		if ((stat & EL_AS_RXBUSY) != 0) 
			break;

		dprintf(("<rescan> "));
	}

	(void)inb(iobase+EL_RXC);
	outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
	return 1;
}

/*
 * Pass a packet up to the higher levels.
 */
static inline void
elread(sc, len)
	struct el_softc *sc;
	int len;
{
	struct ifnet *ifp;
	struct mbuf *m;
	struct ether_header *eh;

	/* Pull packet off interface. */
	ifp = &sc->sc_arpcom.ac_if;
	m = elget(sc, len, ifp);
	if (m == 0)
		return;

	/* We assume that the header fit entirely in one mbuf. */
	eh = mtod(m, struct ether_header *);

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to bpf.
	 */
	if (ifp->if_bpf) {
		bpf_mtap(ifp->if_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
			    sizeof(eh->ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif

	/* We assume that the header fit entirely in one mbuf. */
	m->m_pkthdr.len -= sizeof(*eh);
	m->m_len -= sizeof(*eh);
	m->m_data += sizeof(*eh);

	ether_input(ifp, eh, m);
}

/*
 * Pull read data off a interface.  Len is length of data, with local net
 * header stripped.  We copy the data into mbufs.  When full cluster sized
 * units are present we copy into clusters.
 */
struct mbuf *
elget(sc, totlen, ifp)
	struct el_softc *sc;
	int totlen;
	struct ifnet *ifp;
{
	struct mbuf *top, **mp, *m;
	int len;
	int iobase = sc->sc_iobase;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	len = MHLEN;
	top = 0;
	mp = &top;

	outb(iobase+EL_GPBL, 0);
	outb(iobase+EL_GPBH, 0);

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
		m->m_len = len = min(totlen, len);
		insb(iobase+EL_BUF, mtod(m, caddr_t), len);
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	outb(iobase+EL_RBC, 0);
	outb(iobase+EL_AC, EL_AC_RX);

	return top;
}

/*
 * Process an ioctl request. This code needs some work - it looks pretty ugly.
 */
static int
el_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct el_softc *sc = elcd.cd_devs[ifp->if_unit];
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
			el_init(sc);
			arp_ifinit(&sc->sc_arpcom, ifa);
			break;
#endif
#ifdef NS
		/* XXX - This code is probably wrong. */
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
			el_init(sc);
			break;
		    }
#endif
		default:
			el_init(sc);
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
			el_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			el_init(sc);
		} else {
			/*
			 * Some other important flag might have changed, so
			 * reset.
			 */
			el_reset(sc);
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);
	return error;
}

/*
 * Device timeout routine.
 */
static void
el_watchdog(unit)
	int unit;
{
	struct el_softc *sc = elcd.cd_devs[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	sc->sc_arpcom.ac_if.if_oerrors++;

	el_reset(sc);
}
