/*
 * Isolan AT 4141-0 Ethernet driver
 * Isolink 4110 
 *
 * Copyright (c) 1994 Charles Hannum.
 *
 * Copyright (C) 1993, Paul Richards. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 *
 *	$Id: if_is.c,v 1.22 1994/02/16 20:15:22 mycroft Exp $
 */

/* TODO
 * 1) Advertise for more packets until all transmit buffers are full
 * 2) Add more of the timers/counters e.g. arpcom.opackets etc.
 */

#include "is.h"
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

#include <vm/vm.h>

#include <machine/cpu.h>
#include <machine/pio.h>

#include <i386/isa/isa_device.h>
#include <i386/isa/icu.h>
#include <i386/isa/if_isreg.h>


#define	ETHER_MIN_LEN	64
#define	ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

char *card_type[] = {"unknown", "BICC Isolan", "NE2100"};
char *chip_type[] = {"unknown", "Am7990 LANCE", "Am79960 PCnet-ISA"};


/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * arpcom.ac_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct is_softc {
	struct	device sc_dev;

	struct	arpcom sc_arpcom;	/* Ethernet common part */
	u_short	sc_iobase;		/* IO base address of card */
	u_short	sc_rap, sc_rdp;
	int	sc_chip, sc_card;
	struct	init_block *sc_init;	/* Lance initialisation block */
	struct	mds *sc_rd, *sc_td;
	u_char	*sc_rbuf, *sc_tbuf;
	int	sc_last_rd, sc_last_td;
	int	sc_no_td;
#ifdef ISDEBUG
	int	sc_debug;
#endif
	caddr_t sc_bpf;		      /* BPF "magic cookie" */
} is_softc[NIS];

int is_probe __P((struct isa_device *));
int ne2100_probe __P((struct isa_device *, struct is_softc *));
int bicc_probe __P((struct isa_device *, struct is_softc *));
int lance_probe __P((struct is_softc *));
int is_attach __P((struct isa_device *));
int isintr __P((int));	/* XXX can't be renamed till new interrupt code */
int is_ioctl __P((struct ifnet *, int, caddr_t));
int is_start __P((struct ifnet *));
int is_watchdog __P((/* short */));
void iswrcsr __P((/* struct is_softc *, u_short, u_short */));
u_short isrdcsr __P((/* struct is_softc *, u_short */));
void is_init __P((struct is_softc *));
void init_mem __P((struct is_softc *));
void is_reset __P((struct is_softc *));
void is_stop __P((struct is_softc *));
void is_tint __P((struct is_softc *));
void is_rint __P((struct is_softc *));
void is_read __P((/* struct is_softc *, u_char *, u_short */));
struct mbuf *is_get __P((u_char *, int, int, struct ifnet *));
#ifdef ISDEBUG
void recv_print __P((struct is_softc *, int));
void xmit_print __P((struct is_softc *, int));
#endif
void is_setladrf __P((struct arpcom *, u_long *));

struct isa_driver isdriver = {
	is_probe,
	is_attach,
	"is"
};

struct trailer_header {
	u_short ether_type;
	u_short ether_residual;
};

void
iswrcsr(sc, port, val)
	struct is_softc *sc;
	u_short port;
	u_short val;
{

	outw(sc->sc_rap, port);
	outw(sc->sc_rdp, val);
}

u_short
isrdcsr(sc, port)
	struct is_softc *sc;
	u_short port;
{
	
	outw(sc->sc_rap, port);
	return inw(sc->sc_rdp);
} 

int
is_probe(isa_dev)
	struct isa_device *isa_dev;
{
	int unit = isa_dev->id_unit;
	register struct is_softc *sc = &is_softc[unit];
	int nports;

	/* XXX HACK */
	sprintf(sc->sc_dev.dv_xname, "%s%d", isdriver.name, isa_dev->id_unit);
	sc->sc_dev.dv_unit = isa_dev->id_unit;

	if (nports = bicc_probe(isa_dev, sc))
		goto found;
	if (nports = ne2100_probe(isa_dev, sc))
		goto found;
	return 0;

found:
	/*
	 * XXX - hopefully have better way to get dma'able memory later,
	 * this code assumes that the physical memory address returned
	 * from malloc will be below 16Mb. The Lance's address registers
	 * are only 16 bits wide!
	 */
#define MAXMEM ((NRBUF+NTBUF)*(BUFSIZE) + (NRBUF+NTBUF)*sizeof(struct mds) \
		 + sizeof(struct init_block) + 8)
	sc->sc_init = (struct init_block *)malloc(MAXMEM, M_TEMP, M_NOWAIT);
	if (!sc->sc_init) {
		printf("%s: couldn't allocate memory for card\n",
		    sc->sc_dev.dv_xname);
		return 0;
	}

	/*
	 * Make sure it's quadword aligned?  This is kind of bogus, but
	 * what the Hell...
	 */
	sc->sc_init += 8 - ((u_long)sc->sc_init & 7);

	return nports;
}

int
ne2100_probe(isa_dev, sc)
	struct isa_device *isa_dev;
	struct is_softc *sc;
{
	u_short iobase = isa_dev->id_iobase;
	int i;

	sc->sc_iobase = iobase;
	sc->sc_rap = iobase + NE2100_RAP;
	sc->sc_rdp = iobase + NE2100_RDP;
	sc->sc_card = NE2100;

	if (!(sc->sc_chip = lance_probe(sc)))
		return 0;

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_arpcom.ac_enaddr[i] = inb(iobase + i);

	return 24;
}

int
bicc_probe(isa_dev, sc)
	struct isa_device *isa_dev;
	struct is_softc *sc;
{
	u_short iobase = isa_dev->id_iobase;
	int i;

	sc->sc_iobase = iobase;
	sc->sc_rap = iobase + BICC_RAP;
	sc->sc_rdp = iobase + BICC_RDP;
	sc->sc_card = BICC;

	if (!(sc->sc_chip = lance_probe(sc)))
		return 0;

	/*
	 * Extract the physical MAC address from the ROM.
	 */
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_arpcom.ac_enaddr[i] = inb(iobase + (i * 2));

	return 16;
}

/*
 * Determine which chip is present on the card.
 */
int
lance_probe(sc)
	struct is_softc *sc;
{
	int type;

	/* Stop the LANCE chip and put it in a known state. */
	iswrcsr(sc, 0, STOP);
	DELAY(100);

	if (isrdcsr(sc, 0) != STOP)
		return 0;

	/*
	 * Which bits are settable will depend on the type of chip.
	 */
	iswrcsr(sc, 3, PROBE_MASK);

	switch (isrdcsr(sc, 3)) {
	case LANCE_MASK:
		type = LANCE;
		break;
	case PCnet_ISA_MASK:
		type = PCnet_ISA;
		break;
	default:
		type = 0;
		break;
	}

	iswrcsr(sc, 3, 0);
	return type;
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
int
is_attach(isa_dev)
	struct isa_device *isa_dev;
{
	struct is_softc *sc = &is_softc[isa_dev->id_unit];
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;

	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = isdriver.name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_start = is_start;
	ifp->if_ioctl = is_ioctl;
	ifp->if_watchdog = is_watchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	isa_dmacascade(isa_dev->id_drq);

	/* Attach the interface. */
	if_attach(ifp);

	/*
	 * Search down the ifa address list looking for the AF_LINK type entry.
	 */
	ifa = ifp->if_addrlist;
	while (ifa && ifa->ifa_addr) {
		if (ifa->ifa_addr->sa_family == AF_LINK) {
			/*
			 * Fill in the link level address for this interface.
			 */
			sdl = (struct sockaddr_dl *)ifa->ifa_addr;
			sdl->sdl_type = IFT_ETHER;
			sdl->sdl_alen = ETHER_ADDR_LEN;
			sdl->sdl_slen = 0;
			bcopy(sc->sc_arpcom.ac_enaddr, LLADDR(sdl),
			    ETHER_ADDR_LEN);
			break;
		} else
			ifa = ifa->ifa_next;
	}

	printf("%s: address %s, type %s %s\n", sc->sc_dev.dv_xname,
	    ether_sprintf(sc->sc_arpcom.ac_enaddr),
	    card_type[sc->sc_card], chip_type[sc->sc_chip]);

#if NBPFILTER > 0
	bpfattach(&sc->sc_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
}

void
is_reset(sc)
	struct is_softc *sc;
{

	log(LOG_NOTICE, "%s: reset\n", sc->sc_dev.dv_xname);
	is_init(sc);
}
 
int
is_watchdog(unit)
	short unit;
{
	struct is_softc *sc = &is_softc[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++sc->sc_arpcom.ac_if.if_oerrors;
	is_reset(sc);
}


/* Lance initialisation block set up. */
void
init_mem(sc)
	struct is_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int i;
	void *temp;

	/*
	 * At this point we assume that the memory allocated to the Lance is
	 * quadword aligned.  If it isn't then the initialisation is going
	 * fail later on.
	 */

	/* 
	 * Set up lance initialisation block.
	 */
	temp = (void *)sc->sc_init;
	temp += sizeof(struct init_block);
	sc->sc_rd = temp;
	temp += NRBUF * sizeof(struct mds);
	sc->sc_td = temp;
	temp += NTBUF * sizeof(struct mds);

#if NBPFILTER > 0
	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_init->mode = PROM;
	else
#endif
		sc->sc_init->mode = 0;
	for (i = 0; i < ETHER_ADDR_LEN; i++) 
		sc->sc_init->padr[i] = sc->sc_arpcom.ac_enaddr[i];
	is_setladrf(&sc->sc_arpcom, sc->sc_init->ladrf);
	sc->sc_init->rdra = kvtop(sc->sc_rd);
	sc->sc_init->rlen = ((kvtop(sc->sc_rd) >> 16) & 0xff) | (RLEN << 13);
	sc->sc_init->tdra = kvtop(sc->sc_td);
	sc->sc_init->tlen = ((kvtop(sc->sc_td) >> 16) & 0xff) | (TLEN << 13);

	/* 
	 * Set up receive ring descriptors.
	 */
	sc->sc_rbuf = temp;
	for (i = 0; i < NRBUF; i++) {
		sc->sc_rd[i].addr = kvtop(temp);
		sc->sc_rd[i].flags = ((kvtop(temp) >> 16) & 0xff) | OWN;
		sc->sc_rd[i].bcnt = -BUFSIZE;
		sc->sc_rd[i].mcnt = 0;
		temp += BUFSIZE;
	}

	/* 
	 * Set up transmit ring descriptors.
	 */
	sc->sc_tbuf = temp;
	for (i = 0; i < NTBUF; i++) {
		sc->sc_td[i].addr = kvtop(temp);
		sc->sc_td[i].flags= ((kvtop(temp) >> 16) & 0xff);
		sc->sc_td[i].bcnt = 0;
		sc->sc_td[i].mcnt = 0;
		temp += BUFSIZE;
	}
}

void
is_stop(sc)
	struct is_softc *sc;
{

	iswrcsr(sc, 0, STOP);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
void
is_init(sc)
	struct is_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int s;
	register i;

	/* Address not known. */
 	if (!ifp->if_addrlist)
		return;

	s = splnet();

	/* 
	 * Lance must be stopped to access registers.
	 */
	is_stop(sc);

	sc->sc_last_rd = sc->sc_last_td = sc->sc_no_td = 0;

	/* Set up lance's memory area. */
	init_mem(sc);

	/* No byte swapping etc. */
	iswrcsr(sc, 3, 0);

	/* Give lance the physical address of its memory area. */
	iswrcsr(sc, 1, kvtop(sc->sc_init));
	iswrcsr(sc, 2, (kvtop(sc->sc_init) >> 16) & 0xff);

	/* OK, let's try and initialise the Lance. */
	iswrcsr(sc, 0, INIT);

	/* Wait for initialisation to finish. */
	for (i = 0; i < 1000; i++)
		if (isrdcsr(sc, 0) & IDON)
			break;

	if (isrdcsr(sc, 0) & IDON) {
		/* Start the lance. */
		iswrcsr(sc, 0, STRT | IDON | INEA);
		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
		is_start(ifp);
	} else 
		printf("%s: card failed to initialise\n", sc->sc_dev.dv_xname);
	
	(void) splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue, and map it to the
 * interface before starting the output.
 * Called only at splimp or interrupt level.
 */
int
is_start(ifp)
	struct ifnet *ifp;
{
	register struct is_softc *sc = &is_softc[ifp->if_unit];
	struct mbuf *m0, *m;
	u_char *buffer;
	int len;
	int i;
	struct mds *cdm;

	if ((sc->sc_arpcom.ac_if.if_flags ^ IFF_RUNNING) &
	    (IFF_RUNNING | IFF_OACTIVE))
		return;

outloop:
	if (++sc->sc_no_td > NTBUF) {
		sc->sc_no_td = NTBUF;
		sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;
#ifdef ISDEBUG
		if (sc->sc_debug)
			printf("no_td = %x, last_td = %x\n", sc->sc_no_td,
			    sc->sc_last_td);
#endif
		return;
	}

	cdm = &sc->sc_td[sc->sc_last_td];
#if 0 /* XXX redundant */
	if (cdm->flags & OWN)
		return;
#endif
	
	IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m);
	if (!m)
		return;

	/*
	 * Copy the mbuf chain into the transmit buffer.
	 */
	buffer = sc->sc_tbuf + (BUFSIZE * sc->sc_last_td);
	len = 0;
	for (m0 = m; m; m = m->m_next) {
		bcopy(mtod(m, caddr_t), buffer, m->m_len);
		buffer += m->m_len;
		len += m->m_len;
	}

#if NBPFILTER > 0
	if (sc->sc_bpf) {
		u_short etype;
		int off, datasize, resid;
		struct ether_header *eh;
		struct trailer_header trailer_header;
		char ether_packet[ETHER_MAX_LEN];
		char *ep;

		ep = ether_packet;

		/*
		 * We handle trailers below:
		 * Copy ether header first, then residual data, then data.  Put
		 * all this in a temporary buffer 'ether_packet' and send off
		 * to bpf.  Since the system has generated this packet, we
		 * assume that all of the offsets in the packet are correct; if
		 * they're not, the system will almost certainly crash in
		 * m_copydata.  We make no assumptions about how the data is
		 * arranged in the mbuf chain (i.e. how much data is in each
		 * mbuf, if mbuf clusters are used, etc.), which is why we use
		 * m_copydata to get the ether header rather than assume that
		 * this is located in the first mbuf.
		 */
		/* Copy ether header. */
		m_copydata(m0, 0, sizeof(struct ether_header), ep);
		eh = (struct ether_header *) ep;
		ep += sizeof(struct ether_header);
		etype = ntohs(eh->ether_type);
		if (etype >= ETHERTYPE_TRAIL &&
		    etype < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
			datasize = (etype - ETHERTYPE_TRAIL) << 9;
			off = datasize + sizeof(struct ether_header);

			/* Copy trailer_header into a data structure. */
			m_copydata(m0, off, sizeof(struct trailer_header),
			    &trailer_header.ether_type);

			/* Copy residual data. */
			m_copydata(m0, off + sizeof(struct trailer_header),
			    resid = ntohs(trailer_header.ether_residual) -
			    sizeof(struct trailer_header), ep);
			ep += resid;

			/* Copy data. */
			m_copydata(m0, sizeof(struct ether_header), datasize,
			    ep);
			ep += datasize;

			/* Restore original ether packet type. */
			eh->ether_type = trailer_header.ether_type;

			bpf_tap(sc->sc_bpf, ether_packet, ep - ether_packet);
		} else
			bpf_mtap(sc->sc_bpf, m0);
	}
#endif

	m_freem(m0);
	len = max(len, ETHER_MIN_LEN);

	/*
	 * Init transmit registers, and set transmit start flag.
	 */
	cdm->flags |= OWN | STP | ENP;
	cdm->bcnt = -len;
	cdm->mcnt = 0;

#ifdef ISDEBUG
	if (sc->sc_debug)
		xmit_print(sc, sc->sc_last_td);
#endif
		
	iswrcsr(sc, 0, TDMD | INEA);
	if (++sc->sc_last_td >= NTBUF)
		sc->sc_last_td = 0;

	/* possible more packets */
	goto outloop;
}


/*
 * Controller interrupt.
 */
int
isintr(unit)
{
	register struct is_softc *sc = &is_softc[unit];
	u_short isr;

	isr = isrdcsr(sc, 0);
	if ((isr & INTR) == 0)
		return 0;

	do {
		if (isr & ERR) {
			if (isr & BABL){
				printf("%s: BABL\n", sc->sc_dev.dv_xname);
				sc->sc_arpcom.ac_if.if_oerrors++;
			}
			if (isr & CERR) {
				printf("%s: CERR\n", sc->sc_dev.dv_xname);
				sc->sc_arpcom.ac_if.if_collisions++;
			}
			if (isr & MISS) {
				printf("%s: MISS\n", sc->sc_dev.dv_xname);
				sc->sc_arpcom.ac_if.if_ierrors++;
			}
			if (isr & MERR)
				printf("%s: MERR\n", sc->sc_dev.dv_xname);
			iswrcsr(sc, 0, BABL | CERR | MISS | MERR | INEA);
		}
		if ((isr & RXON) == 0) {
			printf("%s: !RXON\n", sc->sc_dev.dv_xname);
			sc->sc_arpcom.ac_if.if_ierrors++;
			is_reset(sc);
			return 1;
		}
		if ((isr & TXON) == 0) {
			printf("%s: !TXON\n", sc->sc_dev.dv_xname);
			sc->sc_arpcom.ac_if.if_oerrors++;
			is_reset(sc);
			return 1;
		}

		if (isr & RINT) {
			/* Reset watchdog timer. */
			sc->sc_arpcom.ac_if.if_timer = 0;
			is_rint(sc);
		}
		if (isr & TINT) {
			/* Reset watchdog timer. */
			sc->sc_arpcom.ac_if.if_timer = 0;
			iswrcsr(sc, 0, TINT | INEA);
			is_tint(sc);
		}

		isr = isrdcsr(sc, 0);
	} while (isr & INTR);

	return 1;
}

void
is_tint(sc) 
	struct is_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int i, loopcount=0;
	struct mds *cdm;

	do {
		if ((i = sc->sc_last_td - sc->sc_no_td) < 0)
			i += NTBUF;
		cdm = &sc->sc_td[i];
#ifdef ISDEBUG
		if (sc->sc_debug)
			printf("trans cdm = %x\n", cdm);
#endif
		if (cdm->flags & OWN) {
			if (loopcount)
				break;
			return;
		}
		loopcount++;
		sc->sc_arpcom.ac_if.if_opackets++;
		sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;
	} while (--sc->sc_no_td > 0);

	is_start(ifp);	
}

#define NEXTRDS \
	if (++rmd == NRBUF) rmd=0, cdm=sc->sc_rd; else ++cdm
	
/* only called from one place, so may as well integrate */
void
is_rint(sc)
	struct is_softc *sc;
{
	register int rmd = sc->sc_last_rd;
	struct mds *cdm = &sc->sc_rd[rmd];

	/* Out of sync with hardware, should never happen */
	
	if (cdm->flags & OWN) {
		printf("%s: error: out of sync\n", sc->sc_dev.dv_xname);
		iswrcsr(sc, 0, RINT|INEA);
		return;
	}

	/* Process all buffers with valid data. */
	while ((cdm->flags & OWN) == 0) {
		/* Clear interrupt to avoid race condition. */
		iswrcsr(sc, 0, RINT | INEA);
		if (cdm->flags & ERR) {
			if (cdm->flags & FRAM)
				printf("%s: FRAM\n", sc->sc_dev.dv_xname);
			if (cdm->flags & OFLO)
				printf("%s: OFLO\n", sc->sc_dev.dv_xname);
			if (cdm->flags & CRC)
				printf("%s: CRC\n", sc->sc_dev.dv_xname);
			if (cdm->flags & RBUFF)
				printf("%s: RBUFF\n", sc->sc_dev.dv_xname);
		} else if (cdm->flags & (STP | ENP) != (STP | ENP)) {
			do {
				iswrcsr(sc, 0, RINT | INEA);
				cdm->mcnt = 0;
				cdm->flags |= OWN;	
				NEXTRDS;
			} while ((cdm->flags & (OWN | ERR | STP | ENP)) == 0);
			sc->sc_last_rd = rmd;
			printf("%s: chained buffer\n", sc->sc_dev.dv_xname);
			if ((cdm->flags & (OWN | ERR | STP | ENP)) != ENP) {
				is_reset(sc);
				return;
			}
		} else {
#ifdef ISDEBUG
			if (sc->sc_debug)
				recv_print(sc, sc->sc_last_rd);
#endif
			is_read(sc, sc->sc_rbuf + (BUFSIZE*rmd),
			    (int)cdm->mcnt);
			sc->sc_arpcom.ac_if.if_ipackets++;
		}
			
		cdm->flags |= OWN;
		cdm->mcnt = 0;
		NEXTRDS;
#ifdef ISDEBUG
		if (sc->sc_debug)
			printf("sc->sc_last_rd = %x, cdm = %x\n",
			    sc->sc_last_rd, cdm);
#endif
	} /* while */

	sc->sc_last_rd = rmd;
} /* is_rint */


/*
 * Pass a packet to the higher levels.
 * We deal with the trailer protocol here.
 */
void 
is_read(sc, buf, len)
	struct is_softc *sc;
	u_char *buf;
	u_short len;
{
	struct ether_header *eh;
	struct mbuf *m;
	u_short off;
	int resid;
	u_short etype;

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *)buf;
	etype = ntohs(eh->ether_type);
	len -= sizeof(struct ether_header) + 4;
#define nedataaddr(eh, off, type)       ((type)(((caddr_t)((eh)+1)+(off))))
	if (etype >= ETHERTYPE_TRAIL &&
	    etype < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (etype - ETHERTYPE_TRAIL) << 9;
		if ((off + sizeof(struct trailer_header)) > len)
			return;
		eh->ether_type = *nedataaddr(eh, off, u_short *);
		resid = ntohs(*(nedataaddr(eh, off+2, u_short *)));
		if (off + resid > len)
			return;
		len = off + resid;
	} else
		off = 0;
	if (len == 0)
		return;

	/*
	 * Pull packet off interface.  Off is nonzero if packet has trailing
	 * header; is_get will then force this header information to be at the
	 * front, but we still have to drop the type and length which are at
	 * the front of any trailer data.
	 */
	m = is_get(buf, len, off, &sc->sc_arpcom.ac_if);
	if (m == 0)
		return;

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to bpf. 
	 */
	if (sc->sc_bpf) {
		bpf_mtap(sc->sc_bpf, m);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 *
		 * XXX This test does not support multicasts.
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
 * Supporting routines
 */

/*
 * Pull read data off a interface.
 * Len is length of data, with local net header stripped.
 * Off is non-zero if a trailer protocol was used, and
 * gives the offset of the trailer information.
 * We copy the trailer information and then all the normal
 * data into mbufs.  When full cluster sized units are present
 * we copy into clusters.
 */
struct mbuf *
is_get(buf, totlen, off0, ifp)
	u_char *buf;
	int totlen, off0;
	struct ifnet *ifp;
{
	struct mbuf *top, **mp, *m, *p;
	int off = off0, len;
	register caddr_t cp = buf;
	char *epkt;

	buf += sizeof(struct ether_header);
	cp = buf;
	epkt = cp + totlen;

	if (off) {
		cp += off + 2 * sizeof(u_short);
		totlen -= 2 * sizeof(u_short);
	}

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
 * Process an ioctl request.
 */
int
is_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	int cmd;
	caddr_t data;
{
	struct is_softc *sc = &is_softc[ifp->if_unit];
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
			is_init(sc);	/* before arpwhohas */
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
		/* XXX - This code is probably wrong. */
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
				bcopy(ina->x_host.c_host,
				    sc->sc_arpcom.ac_enaddr,
				    sizeof(sc->sc_arpcom.ac_enaddr));
			}
			/* Set new address. */
			is_init(sc); 
			break;
		    }
#endif
		default:
			is_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		/*
		 * If interface is marked down and it is running, then stop it
		 */
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			is_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			is_init(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			/*is_stop(sc);*/
			is_init(sc);
		}
#ifdef ISDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			sc->sc_debug = 1;
		else
			sc->sc_debug = 0;
#endif
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_arpcom):
		    ether_delmulti(ifr, &sc->sc_arpcom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			is_init(sc);
			error = 0;
		}
		break;

#ifdef notdef
	case SIOCGHWADDR:
		bcopy(sc->sc_arpcom.ac_enaddr, &ifr->ifr_data,
		    sizeof(sc->sc_arpcom.ac_enaddr));
		break;
#endif

	default:
		error = EINVAL;
	}
	(void) splx(s);
	return error;
}

#ifdef ISDEBUG
void
recv_print(sc, no)
	struct is_softc *sc;
	int no;
{
	struct mds *rmd;
	int i, printed = 0;
	u_short len;
	
	rmd = &sc->sc_rd[no];
	len = rmd->mcnt;
	printf("%s: receive buffer %d, len = %d\n", sc->sc_dev.dv_xname, no,
	    len);
	printf("%s: status %x\n", sc->sc_dev.dv_xname, isrdcsr(sc, 0));
	for (i = 0; i < len; i++) {
		if (!printed) {
			printed = 1;
			printf("%s: data: ", sc->sc_dev.dv_xname);
		}
		printf("%x ", *(sc->sc_rbuf + (BUFSIZE*no) + i));
	}
	if (printed)
		printf("\n");
}
		
void
xmit_print(sc, no)
	struct is_softc *sc;
	int no;
{
	struct mds *rmd;
	int i, printed=0;
	u_short len;
	
	rmd = &sc->sc_td[no];
	len = -rmd->bcnt;
	printf("%s: transmit buffer %d, len = %d\n", sc->sc_dev.dv_xname, no,
	    len);
	printf("%s: status %x\n", sc->sc_dev.dv_xname, isrdcsr(sc, 0));
	printf("%s: addr %x, flags %x, bcnt %x, mcnt %x\n",
	    sc->sc_dev.dv_xname, rmd->addr, rmd->flags, rmd->bcnt, rmd->mcnt);
	for (i = 0; i < len; i++)  {
		if (!printed) {
			printed = 1;
			printf("%s: data: ", sc->sc_dev.dv_xname);
		}
		printf("%x ", *(sc->sc_tbuf + (BUFSIZE*no) + i));
	}
	if (printed)
		printf("\n");
}
#endif /* ISDEBUG */

/*
 * Set up the logical address filter.
 */
void
is_setladrf(ac, af)
	struct arpcom *ac;
	u_long *af;
{
	struct ifnet *ifp = &ac->ac_if;
	struct ether_multi *enm;
	register u_char *cp, c;
	register u_long crc;
	register int i, len;
	struct ether_multistep step;

	/*
	 * Set up multicast address filter by passing all multicast addresses
	 * through a crc generator, and then using the high order 6 bits as an
	 * index into the 64 bit logical address filter.  The high order bit
	 * selects the word, while the rest of the bits select the bit within
	 * the word.
	 */

	if (ifp->if_flags & IFF_PROMISC)) {
		ifp->if_flags |= IFF_ALLMULTI;
		af[0] = af[1] = 0xffffffff;
		return;
	}

	af[0] = af[1] = 0;
	ETHER_FIRST_MULTI(step, ac, enm);
	while (enm != NULL) {
		if (bcmp(enm->enm_addrlo, enm->enm_addrhi,
		    sizeof(enm->enm_addrlo)) != 0) {
			/*
			 * We must listen to a range of multicast addresses.
			 * For now, just accept all multicasts, rather than
			 * trying to set only those filter bits needed to match
			 * the range.  (At this time, the only use of address
			 * ranges is for IP multicast routing, for which the
			 * range is big enough to require all bits set.)
			 */
			ifp->if_flags |= IFF_ALLMULTI;
			af[0] = af[1] = 0xffffffff;
			return;
		}

		cp = enm->enm_addrlo;
		crc = 0xffffffff;
		for (len = sizeof(enm->enm_addrlo); --len >= 0;) {
			c = *cp++;
			for (i = 8; --i >= 0;) {
				if (((crc & 0x80000000) ? 1 : 0) ^ (c & 0x01)) {
					crc <<= 1;
					crc ^= 0x04c11db6 | 1;
				} else
					crc <<= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Turn on the corresponding bit in the filter. */
		af[crc >> 5] |= 1 << ((crc & 0x1f) ^ 24);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
}

