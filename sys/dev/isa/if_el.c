/*	$NetBSD: if_el.c,v 1.47 1997/04/28 17:04:15 mycroft Exp $	*/

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
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

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

#if NBPFILTER > 0
#include <net/bpf.h>
#include <net/bpfdesc.h>
#endif

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/bus.h>

#include <dev/isa/isavar.h>
#include <dev/isa/if_elreg.h>

#define ETHER_MIN_LEN	64
#define ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

/* for debugging convenience */
#ifdef EL_DEBUG
#define DPRINTF(x) printf x
#else
#define DPRINTF(x)
#endif

/*
 * per-line info and status
 */
struct el_softc {
	struct device sc_dev;
	void *sc_ih;

	struct ethercom sc_ethercom;	/* ethernet common */
	bus_space_tag_t sc_iot;		/* bus space identifier */
	bus_space_handle_t sc_ioh;	/* i/o handle */
};

/*
 * prototypes
 */
int elintr __P((void *));
void elinit __P((struct el_softc *));
int elioctl __P((struct ifnet *, u_long, caddr_t));
void elstart __P((struct ifnet *));
void elwatchdog __P((struct ifnet *));
void elreset __P((struct el_softc *));
void elstop __P((struct el_softc *));
static int el_xmit __P((struct el_softc *));
void elread __P((struct el_softc *, int));
struct mbuf *elget __P((struct el_softc *sc, int));
static inline void el_hardreset __P((struct el_softc *));

int elprobe __P((struct device *, void *, void *));
void elattach __P((struct device *, struct device *, void *));

struct cfattach el_ca = {
	sizeof(struct el_softc), elprobe, elattach
};

struct cfdriver el_cd = {
	NULL, "el", DV_IFNET
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
	struct isa_attach_args *ia = aux;
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	int iobase = ia->ia_iobase;
	u_int8_t station_addr[ETHER_ADDR_LEN];
	u_int8_t i;
	int rval;

	rval = 0;

	/* First check the base. */
	if (iobase < 0x280 || iobase > 0x3f0)
		return 0;

	/* Map i/o space. */
	if (bus_space_map(iot, iobase, 4, 0, &ioh))
		return 0;

	/*
	 * Now attempt to grab the station address from the PROM and see if it
	 * contains the 3com vendor code.
	 */
	DPRINTF(("Probing 3c501 at 0x%x...\n", iobase));

	/* Reset the board. */
	DPRINTF(("Resetting board...\n"));
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_RESET);
	delay(5);
	bus_space_write_1(iot, ioh, EL_AC, 0);

	/* Now read the address. */
	DPRINTF(("Reading station address...\n"));
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		bus_space_write_1(iot, ioh, EL_GPBL, i);
		station_addr[i] = bus_space_read_1(iot, ioh, EL_EAW);
	}
	DPRINTF(("Address is %s\n", ether_sprintf(station_addr)));

	/*
	 * If the vendor code is ok, return a 1.  We'll assume that whoever
	 * configured this system is right about the IRQ.
	 */
	if (station_addr[0] != 0x02 || station_addr[1] != 0x60 ||
	    station_addr[2] != 0x8c) {
		DPRINTF(("Bad vendor code.\n"));
		goto out;
	}
	DPRINTF(("Vendor code ok.\n"));

	ia->ia_iosize = 4;	/* XXX */
	ia->ia_msize = 0;
	rval = 1;

 out:
	bus_space_unmap(iot, ioh, 4);
	return rval;
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
	bus_space_tag_t iot = ia->ia_iot;
	bus_space_handle_t ioh;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	u_int8_t myaddr[ETHER_ADDR_LEN];
	u_int8_t i;

	printf("\n");

	DPRINTF(("Attaching %s...\n", sc->sc_dev.dv_xname));

	/* Map i/o space. */
	if (bus_space_map(iot, ia->ia_iobase, ia->ia_iosize, 0, &ioh)) {
		printf("%s: can't map i/o space\n", self->dv_xname);
		return;
	}

	sc->sc_iot = iot;
	sc->sc_ioh = ioh;

	/* Reset the board. */
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_RESET);
	delay(5);
	bus_space_write_1(iot, ioh, EL_AC, 0);

	/* Now read the address. */
	for (i = 0; i < ETHER_ADDR_LEN; i++) {
		bus_space_write_1(iot, ioh, EL_GPBL, i);
		myaddr[i] = bus_space_read_1(iot, ioh, EL_EAW);
	}

	/* Stop the board. */
	elstop(sc);

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = elstart;
	ifp->if_ioctl = elioctl;
	ifp->if_watchdog = elwatchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;

	/* Now we can attach the interface. */
	DPRINTF(("Attaching interface...\n"));
	if_attach(ifp);
	ether_ifattach(ifp, myaddr);

	/* Print out some information for the user. */
	printf("%s: address %s\n", self->dv_xname, ether_sprintf(myaddr));

	/* Finally, attach to bpf filter if it is present. */
#if NBPFILTER > 0
	DPRINTF(("Attaching to BPF...\n"));
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->sc_ih = isa_intr_establish(ia->ia_ic, ia->ia_irq, IST_EDGE,
	    IPL_NET, elintr, sc);

	DPRINTF(("elattach() finished.\n"));
}

/*
 * Reset interface.
 */
void
elreset(sc)
	struct el_softc *sc;
{
	int s;

	DPRINTF(("elreset()\n"));
	s = splnet();
	elstop(sc);
	elinit(sc);
	splx(s);
}

/*
 * Stop interface.
 */
void
elstop(sc)
	struct el_softc *sc;
{

	bus_space_write_1(sc->sc_iot, sc->sc_ioh, EL_AC, 0);
}

/*
 * Do a hardware reset of the board, and upload the ethernet address again in
 * case the board forgets.
 */
static inline void
el_hardreset(sc)
	struct el_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;

	bus_space_write_1(iot, ioh, EL_AC, EL_AC_RESET);
	delay(5);
	bus_space_write_1(iot, ioh, EL_AC, 0);

	for (i = 0; i < ETHER_ADDR_LEN; i++)
		bus_space_write_1(iot, ioh, i,
		    LLADDR(sc->sc_ethercom.ec_if.if_sadl)[i]);
}

/*
 * Initialize interface.
 */
void
elinit(sc)
	struct el_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;

	/* First, reset the board. */
	el_hardreset(sc);

	/* Configure rx. */
	DPRINTF(("Configuring rx...\n"));
	if (ifp->if_flags & IFF_PROMISC)
		bus_space_write_1(iot, ioh, EL_RXC,
		    EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB |
		    EL_RXC_DOFLOW | EL_RXC_PROMISC);
	else
		bus_space_write_1(iot, ioh, EL_RXC,
		    EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB |
		    EL_RXC_DOFLOW | EL_RXC_ABROAD);
	bus_space_write_1(iot, ioh, EL_RBC, 0);

	/* Configure TX. */
	DPRINTF(("Configuring tx...\n"));
	bus_space_write_1(iot, ioh, EL_TXC, 0);

	/* Start reception. */
	DPRINTF(("Starting reception...\n"));
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_IRQE | EL_AC_RX);

	/* Set flags appropriately. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* And start output. */
	elstart(ifp);
}

/*
 * Start output on interface.  Get datagrams from the queue and output them,
 * giving the receiver a chance between datagrams.  Call only from splnet or
 * interrupt level!
 */
void
elstart(ifp)
	struct ifnet *ifp;
{
	struct el_softc *sc = ifp->if_softc;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct mbuf *m, *m0;
	int s, i, off, retries;

	DPRINTF(("elstart()...\n"));
	s = splnet();

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
		bus_space_write_1(iot, ioh, EL_AC, EL_AC_HOST);
		bus_space_write_1(iot, ioh, EL_RBC, 0);

		/* Transfer datagram to board. */
		DPRINTF(("el: xfr pkt length=%d...\n", m0->m_pkthdr.len));
		off = EL_BUFSIZ - max(m0->m_pkthdr.len, ETHER_MIN_LEN);
#ifdef DIAGNOSTIC
		if ((off & 0xffff) != off)
			printf("%s: bogus off 0x%x\n",
			    sc->sc_dev.dv_xname, off);
#endif
		bus_space_write_1(iot, ioh, EL_GPBL, off & 0xff);
		bus_space_write_1(iot, ioh, EL_GPBH, (off >> 8) & 0xff);

		/* Copy the datagram to the buffer. */
		for (m = m0; m != 0; m = m->m_next)
			bus_space_write_multi_1(iot, ioh, EL_BUF,
			    mtod(m, u_int8_t *), m->m_len);

		m_freem(m0);

		/* Now transmit the datagram. */
		retries = 0;
		for (;;) {
			bus_space_write_1(iot, ioh, EL_GPBL, off & 0xff);
			bus_space_write_1(iot, ioh, EL_GPBH, (off >> 8) & 0xff);
			if (el_xmit(sc)) {
				ifp->if_oerrors++;
				break;
			}
			/* Check out status. */
			i = bus_space_read_1(iot, ioh, EL_TXS);
			DPRINTF(("tx status=0x%x\n", i));
			if ((i & EL_TXS_READY) == 0) {
				DPRINTF(("el: err txs=%x\n", i));
				if (i & (EL_TXS_COLL | EL_TXS_COLL16)) {
					ifp->if_collisions++;
					if ((i & EL_TXC_DCOLL16) == 0 &&
					    retries < 15) {
						retries++;
						bus_space_write_1(iot, ioh,
						    EL_AC, EL_AC_HOST);
					}
				} else {
					ifp->if_oerrors++;
					break;
				}
			} else {
				ifp->if_opackets++;
				break;
			}
		}

		/*
		 * Now give the card a chance to receive.
		 * Gotta love 3c501s...
		 */
		(void)bus_space_read_1(iot, ioh, EL_AS);
		bus_space_write_1(iot, ioh, EL_AC, EL_AC_IRQE | EL_AC_RX);
		splx(s);
		/* Interrupt here. */
		s = splnet();
	}

	(void)bus_space_read_1(iot, ioh, EL_AS);
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_IRQE | EL_AC_RX);
	ifp->if_flags &= ~IFF_OACTIVE;
	splx(s);
}

/*
 * This function actually attempts to transmit a datagram downloaded to the
 * board.  Call at splnet or interrupt, after downloading data!  Returns 0 on
 * success, non-0 on failure.
 */
static int
el_xmit(sc)
	struct el_softc *sc;
{
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	int i;

	/*
	 * XXX
	 * This busy-waits for the tx completion.  Can we get an interrupt
	 * instead?
	 */

	DPRINTF(("el: xmit..."));
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_TXFRX);
	i = 20000;
	while ((bus_space_read_1(iot, ioh, EL_AS) & EL_AS_TXBUSY) && (i > 0))
		i--;
	if (i == 0) {
		DPRINTF(("tx not ready\n"));
		return -1;
	}
	DPRINTF(("%d cycles.\n", 20000 - i));
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
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	u_int8_t rxstat;
	int len;

	DPRINTF(("elintr: "));

	/* Check board status. */
	if ((bus_space_read_1(iot, ioh, EL_AS) & EL_AS_RXBUSY) != 0) {
		(void)bus_space_read_1(iot, ioh, EL_RXC);
		bus_space_write_1(iot, ioh, EL_AC, EL_AC_IRQE | EL_AC_RX);
		return 0;
	}

	for (;;) {
		rxstat = bus_space_read_1(iot, ioh, EL_RXS);
		if (rxstat & EL_RXS_STALE)
			break;

		/* If there's an overflow, reinit the board. */
		if ((rxstat & EL_RXS_NOFLOW) == 0) {
			DPRINTF(("overflow.\n"));
			el_hardreset(sc);
			/* Put board back into receive mode. */
			if (sc->sc_ethercom.ec_if.if_flags & IFF_PROMISC)
				bus_space_write_1(iot, ioh, EL_RXC,
				    EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB |
				    EL_RXC_DOFLOW | EL_RXC_PROMISC);
			else
				bus_space_write_1(iot, ioh, EL_RXC,
				    EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB |
				    EL_RXC_DOFLOW | EL_RXC_ABROAD);
			(void)bus_space_read_1(iot, ioh, EL_AS);
			bus_space_write_1(iot, ioh, EL_RBC, 0);
			break;
		}

		/* Incoming packet. */
		len = bus_space_read_1(iot, ioh, EL_RBL);
		len |= bus_space_read_1(iot, ioh, EL_RBH) << 8;
		DPRINTF(("receive len=%d rxstat=%x ", len, rxstat));
		bus_space_write_1(iot, ioh, EL_AC, EL_AC_HOST);

		/* Pass data up to upper levels. */
		elread(sc, len);

		/* Is there another packet? */
		if ((bus_space_read_1(iot, ioh, EL_AS) & EL_AS_RXBUSY) != 0)
			break;

		DPRINTF(("<rescan> "));
	}

	(void)bus_space_read_1(iot, ioh, EL_RXC);
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_IRQE | EL_AC_RX);
	return 1;
}

/*
 * Pass a packet to the higher levels.
 */
void
elread(sc, len)
	register struct el_softc *sc;
	int len;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	struct ether_header *eh;

	if (len <= sizeof(struct ether_header) ||
	    len > ETHER_MAX_LEN) {
		printf("%s: invalid packet size %d; dropping\n",
		    sc->sc_dev.dv_xname, len);
		ifp->if_ierrors++;
		return;
	}

	/* Pull packet off interface. */
	m = elget(sc, len);
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
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((ifp->if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, LLADDR(ifp->if_sadl),
			    sizeof(eh->ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif

	/* We assume that the header fit entirely in one mbuf. */
	m_adj(m, sizeof(struct ether_header));
	ether_input(ifp, eh, m);
}

/*
 * Pull read data off a interface.  Len is length of data, with local net
 * header stripped.  We copy the data into mbufs.  When full cluster sized
 * units are present we copy into clusters.
 */
struct mbuf *
elget(sc, totlen)
	struct el_softc *sc;
	int totlen;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	bus_space_tag_t iot = sc->sc_iot;
	bus_space_handle_t ioh = sc->sc_ioh;
	struct mbuf *top, **mp, *m;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = totlen;
	len = MHLEN;
	top = 0;
	mp = &top;

	bus_space_write_1(iot, ioh, EL_GPBL, 0);
	bus_space_write_1(iot, ioh, EL_GPBH, 0);

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
		m->m_len = len = min(totlen, len);
		bus_space_read_multi_1(iot, ioh, EL_BUF, mtod(m, u_int8_t *), len);
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	bus_space_write_1(iot, ioh, EL_RBC, 0);
	bus_space_write_1(iot, ioh, EL_AC, EL_AC_RX);

	return top;
}

/*
 * Process an ioctl request. This code needs some work - it looks pretty ugly.
 */
int
elioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct el_softc *sc = ifp->if_softc;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			elinit(sc);
			arp_ifinit(ifp, ifa);
			break;
#endif
#ifdef NS
		/* XXX - This code is probably wrong. */
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)LLADDR(ifp->if_sadl);
			else
				bcopy(ina->x_host.c_host, LLADDR(ifp->if_sadl),
				    ETHER_ADDR_LEN);
			/* Set new address. */
			elinit(sc);
			break;
		    }
#endif
		default:
			elinit(sc);
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
			elstop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			elinit(sc);
		} else {
			/*
			 * Some other important flag might have changed, so
			 * reset.
			 */
			elreset(sc);
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
void
elwatchdog(ifp)
	struct ifnet *ifp;
{
	struct el_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	sc->sc_ethercom.ec_if.if_oerrors++;

	elreset(sc);
}
