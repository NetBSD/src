/*
 * Copyright (c) 1994, Matthew E. Kimmel.  Permission is hereby granted
 * to use, copy, modify and distribute this software provided that both
 * the copyright notice and this permission notice appear in all copies
 * of the software, derivative works or modified versions, and any
 * portions thereof.
 */

/*
 * 3COM Etherlink 3C501 device driver
 *
 *	$Id: if_el.c,v 1.13 1994/05/13 06:13:48 mycroft Exp $
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

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/if_elreg.h>

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
	struct intrhand sc_ih;

	struct arpcom sc_arpcom;	/* ethernet common */
	u_short sc_iobase;		/* base I/O addr */
	char sc_pktbuf[EL_BUFSIZ]; 	/* frame buffer */
};

/*
 * prototypes
 */
int elintr __P((struct el_softc *));
static int el_init __P((struct el_softc *));
static int el_ioctl __P((struct ifnet *, int, caddr_t));
static int el_start __P((struct ifnet *));
static int el_watchdog __P((int));
static void el_reset __P((struct el_softc *));
static void el_stop __P((struct el_softc *));
static int el_xmit __P((struct el_softc *, int));
static inline void elread __P((struct el_softc *, caddr_t, int));
static struct mbuf *elget __P((caddr_t, int, struct ifnet *));
static inline void el_hardreset __P((struct el_softc *));

int elprobe();
void elattach();

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
elprobe(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct el_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;
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
	ifp->if_output = ether_output;
	ifp->if_start = el_start;
	ifp->if_ioctl = el_ioctl;
	ifp->if_watchdog = el_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;

	/* Now we can attach the interface. */
	dprintf(("Attaching interface...\n"));
	if_attach(ifp);
	ether_ifattach(ifp);

	/* Print out some information for the user. */
	printf("%s: address %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(sc->sc_arpcom.ac_enaddr));

	/* Finally, attach to bpf filter if it is present. */
#if NBPFILTER > 0
	dprintf(("Attaching to BPF...\n"));
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->sc_ih.ih_fun = elintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_NET;
	intr_establish(ia->ia_irq, &sc->sc_ih);

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
	u_short iobase = sc->sc_iobase;
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
	u_short iobase = sc->sc_iobase;
	int s;

	/* If address not known, do nothing. */
	if (ifp->if_addrlist == 0)
		return;

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
static int
el_start(ifp)
	struct ifnet *ifp;
{
	struct el_softc *sc = elcd.cd_devs[ifp->if_unit];
	u_short iobase = sc->sc_iobase;
	struct mbuf *m, *m0;
	int s, i, len, retries, done;

	dprintf(("el_start()...\n"));
	s = splimp();

	/* Don't do anything if output is active. */
	if (sc->sc_arpcom.ac_if.if_flags & IFF_OACTIVE)
		return;
	sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;

	/*
	 * The main loop.  They warned me against endless loops, but would I
	 * listen?  NOOO....
	 */
	for (;;) {
		/* Dequeue the next datagram. */
		IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m0);

		/* If there's nothing to send, return. */
		if (!m0) {
			sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
			splx(s);
			return;
		}

		/* Disable the receiver. */
		outb(iobase+EL_AC, EL_AC_HOST);
		outb(iobase+EL_RBC, 0);

		/* Copy the datagram to the buffer. */
		len = 0;
		for (m = m0; m; m = m->m_next) {
			if (m->m_len == 0)
				continue;
			bcopy(mtod(m, caddr_t), sc->sc_pktbuf + len, m->m_len);
			len += m->m_len;
		}
		m_freem(m0);

		len = max(len, ETHER_MIN_LEN);

		/* Give the packet to the bpf, if any. */
#if NBPFILTER > 0
		if (sc->sc_arpcom.ac_if.if_bpf)
			bpf_tap(sc->sc_arpcom.ac_if.if_bpf, sc->sc_pktbuf, len);
#endif

		/* Transfer datagram to board. */
		dprintf(("el: xfr pkt length=%d...\n", len));
		i = EL_BUFSIZ - len;
		outb(iobase+EL_GPBL, i);
		outb(iobase+EL_GPBH, i >> 8);
		outsb(iobase+EL_BUF, sc->sc_pktbuf, len);

		/* Now transmit the datagram. */
		retries = 0;
		done = 0;
		while (!done) {
			if (el_xmit(sc, len)) {
				/* Something went wrong. */
				done = -1;
				break;
			}
			/* Check out status. */
			i = inb(iobase+EL_TXS);
			dprintf(("tx status=0x%x\n", i));
			if ((i & EL_TXS_READY) == 0) {
				dprintf(("el: err txs=%x\n", i));
				sc->sc_arpcom.ac_if.if_oerrors++;
				if (i & (EL_TXS_COLL | EL_TXS_COLL16)) {
					if ((i & EL_TXC_DCOLL16) == 0 &&
					    retries < 15) {
						retries++;
						outb(iobase+EL_AC, EL_AC_HOST);
					}
				} else
					done = 1;
			} else {
				sc->sc_arpcom.ac_if.if_opackets++;
				done = 1;
			}
		}
		if (done == -1)
			/* Packet not transmitted. */
			continue;

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
}

/*
 * This function actually attempts to transmit a datagram downloaded to the
 * board.  Call at splimp or interrupt, after downloading data!  Returns 0 on
 * success, non-0 on failure.
 */
static int
el_xmit(sc, len)
	struct el_softc *sc;
	int len;
{
	u_short iobase = sc->sc_iobase;
	int gpl;
	int i;

	gpl = EL_BUFSIZ - len;
	dprintf(("el: xmit..."));
	outb(iobase+EL_GPBL, gpl);
	outb(iobase+EL_GPBH, gpl >> 8);
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
elintr(sc)
	register struct el_softc *sc;
{
	u_short iobase = sc->sc_iobase;
	int stat, rxstat, len, done;

	dprintf(("elintr: "));

	/* Check board status. */
	stat = inb(iobase+EL_AS);
	if (stat & EL_AS_RXBUSY) {
		(void)inb(iobase+EL_RXC);
		outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
		return 0;
	}

	done = 0;
	while (!done) {
		rxstat = inb(iobase+EL_RXS);
		if (rxstat & EL_RXS_STALE) {
			(void)inb(iobase+EL_RXC);
			outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
			return 1;
		}

		/* If there's an overflow, reinit the board. */
		if ((rxstat & EL_RXS_NOFLOW) == 0) {
			dprintf(("overflow.\n"));
			el_hardreset(sc);
			/* Put board back into receive mode. */
			if (sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC)
				outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_PROMISC);
			else
				outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_ABROAD);
			(void)inb(iobase+EL_AS);
			outb(iobase+EL_RBC, 0);
			(void)inb(iobase+EL_RXC);
			outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
			return 1;
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
		    len > ETHER_MAX_LEN) {
			if (sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC)
				outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_PROMISC);
			else
				outb(iobase+EL_RXC, EL_RXC_AGF | EL_RXC_DSHORT | EL_RXC_DDRIB | EL_RXC_DOFLOW | EL_RXC_ABROAD);
			(void)inb(iobase+EL_AS);
			outb(iobase+EL_RBC, 0);
			(void)inb(iobase+EL_RXC);
			outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
			return 1;
		}

		sc->sc_arpcom.ac_if.if_ipackets++;

		/* Copy the data into our buffer. */
		outb(iobase+EL_GPBL, 0);
		outb(iobase+EL_GPBH, 0);
		insb(iobase+EL_BUF, sc->sc_pktbuf, len);
		outb(iobase+EL_RBC, 0);
		outb(iobase+EL_AC, EL_AC_RX);
		dprintf(("%s-->", ether_sprintf(sc->sc_pktbuf+6)));
		dprintf(("%s\n", ether_sprintf(sc->sc_pktbuf)));

		/* Pass data up to upper levels. */
		elread(sc, (caddr_t)sc->sc_pktbuf, len);

		/* Is there another packet? */
		stat = inb(iobase+EL_AS);

		/* If so, do it all again (i.e. don't set done to 1). */
		if ((stat & EL_AS_RXBUSY) == 0) 
			dprintf(("<rescan> "));
		else
			done = 1;
	}

	(void)inb(iobase+EL_RXC);
	outb(iobase+EL_AC, EL_AC_IRQE | EL_AC_RX);
	return 1;
}

/*
 * Pass a packet up to the higher levels.
 */
static inline void
elread(sc, buf, len)
	struct el_softc *sc;
	caddr_t buf;
	int len;
{
	register struct ether_header *eh;
	struct mbuf *m;

	eh = (struct ether_header *)buf;
	len -= sizeof(struct ether_header);
	if (len <= 0)
		return;

	/* Pull packet off interface. */
	m = elget(buf, len, &sc->sc_arpcom.ac_if);
	if (m == 0)
		return;

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to bpf.
	 */
	if (sc->sc_arpcom.ac_if.if_bpf) {
		bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 */
		if ((sc->sc_arpcom.ac_if.if_flags & IFF_PROMISC) &&
		    (eh->ether_dhost[0] & 1) == 0 && /* !mcast and !bcast */
		    bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
			    sizeof(eh->ether_dhost)) != 0) {
			m_freem(m);
			return;
		}
	}
#endif

	ether_input(&sc->sc_arpcom.ac_if, eh, m);
}

/*
 * Pull read data off a interface.  Len is length of data, with local net
 * header stripped.  We copy the data into mbufs.  When full cluster sized
 * units are present we copy into clusters.
 */
struct mbuf *
elget(buf, totlen, ifp)
        caddr_t buf;
        int totlen;
        struct ifnet *ifp;
{
        struct mbuf *top, **mp, *m, *p;
        int len;
        register caddr_t cp = buf;
        char *epkt;

        buf += sizeof(struct ether_header);
        cp = buf;
        epkt = cp + totlen;

        MGETHDR(m, M_DONTWAIT, MT_DATA);
        if (m == 0)
                return 0;
        m->m_pkthdr.rcvif = ifp;
        m->m_pkthdr.len = totlen;
        m->m_len = MHLEN;
        top = 0;
        mp = &top;

        while (totlen > 0) {
                if (top) {
                        MGET(m, M_DONTWAIT, MT_DATA);
                        if (m == 0) {
                                m_freem(top);
                                return 0;
                        }
                        m->m_len = MLEN;
                }
                len = min(totlen, epkt - cp);
                if (len >= MINCLSIZE) {
                        MCLGET(m, M_DONTWAIT);
                        if (m->m_flags & M_EXT)
                                m->m_len = len = min(len, MCLBYTES);
                        else
                                len = m->m_len;
                } else {
                        /*
                         * Place initial small packet/header at end of mbuf.
                         */
                        if (len < m->m_len) {
                                if (top == 0 && len + max_linkhdr <= m->m_len)
                                        m->m_data += max_linkhdr;
                                m->m_len = len;
                        } else
                                len = m->m_len;
                }
                bcopy(cp, mtod(m, caddr_t), (unsigned)len);
                cp += len;
                *mp = m;
                mp = &m->m_next;
                totlen -= len;
                if (cp == epkt)
                        cp = buf;
        }

        return top;
}

/*
 * Process an ioctl request. This code needs some work - it looks pretty ugly.
 */
static int
el_ioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int cmd;
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
			el_init(sc);	/* before arpwhohas */
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
		/*
		 * XXX - This code is probably wrong.
		 */
		case AF_NS:
		    {
			register struct ns_addr *ina = &IA_SNS(ifa)->sns_addr;

			if (ns_nullhost(*ina))
				ina->x_host =
				    *(union ns_host *)(sc->sc_arpcom.ac_enaddr);
			else {
				/* 
				 * 
				 */
				bcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->sc_arpcom.ac_enaddr,
				    sizeof(sc->sc_arpcom.ac_enaddr));
			}
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

	default:
		error = EINVAL;
	}
	(void) splx(s);
	return error;
}

/*
 * Device timeout routine.
 */
static int
el_watchdog(unit)
	int unit;
{
	struct el_softc *sc = elcd.cd_devs[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	sc->sc_arpcom.ac_if.if_oerrors++;

	el_reset(sc);
}
