/*
 * Isolan AT 4141-0 Ethernet driver
 * Isolink 4110 
 *
 * By Paul Richards 
 *
 * Copyright (C) 1993, Paul Richards. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 *
 */

/* TODO

1) Add working multicast support
2) Use better allocation of memory to card
3) Advertise for more packets until all transmit buffers are full
4) Add more of the timers/counters e.g. arpcom.opackets etc.
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

#include <i386/isa/isavar.h>
#include <i386/isa/icu.h>
#include <i386/isa/if_isreg.h>

#define	ETHER_MIN_LEN	64
#define	ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * arpcom.ac_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct	is_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	struct	arpcom sc_arpcom;	/* common ethernet gunk */
	u_short	sc_iobase;
	struct	init_block *sc_init;	/* Lance initialisation block */
	struct	mds *sc_rd, *sc_td;
	u_char	*sc_rbuf, *sc_tbuf;
	int	sc_last_rd, sc_last_td;
	int	sc_no_td;
	caddr_t	sc_bpf;			/* BPF "magic cookie" */
};

#ifdef ISDEBUG
int is_debug;
#endif

int is_probe __P((struct device *, struct cfdata *, void *));
void is_forceintr __P((void *));
void is_attach __P((struct device *, struct device *, void *));
int is_intr __P((void *));

struct cfdriver iscd =
{ NULL, "ed", is_probe, is_attach, DV_IFNET, sizeof(struct is_softc) };

int is_ioctl __P((struct ifnet *, int, caddr_t));
void is_start __P((struct ifnet *));
void is_watchdog __P((short));
void iswrcsr __P((u_short, u_short, u_short));
u_short isrdcsr __P((u_short, u_short));
void is_init __P((struct is_softc *));
void init_mem __P((struct is_softc *));
void is_reset __P((struct is_softc *));
void is_stop __P((struct is_softc *));
void is_tint __P((struct is_softc *));
void is_rint __P((struct is_softc *));
void is_read __P((struct is_softc *, u_char *, u_short));
struct mbuf *is_get __P((u_char *, int, int, struct ifnet *));
#ifdef ISDEBUG
void recv_print __P((struct is_softc *, int));
void xmit_print __P((struct is_softc *, int));
#endif

struct trailer_header {
	u_short ether_type;
	u_short ether_residual;
};

void
iswrcsr(iobase, port, val)
	u_short iobase;
	u_short port;
	u_short val;
{

	outw(iobase+RAP,port);
	outw(iobase+RDP,val);
}

u_short
isrdcsr(iobase, port)
	u_short iobase;
	u_short port;
{
	
	outw(iobase+RAP,port);
	return(inw(iobase+RDP));
} 

int
is_probe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	if (iobase == IOBASEUNK || ia->ia_drq == DRQUNK)
		return 0;

	/* stop the lance chip, put it in known state */	
	iswrcsr(iobase, 0, STOP);
	delay(100);

	/* is there a lance? */
	iswrcsr(iobase, 3, 0xffff);
	if (isrdcsr(iobase, 3) != 7) {
		return 0;
	}
	iswrcsr(iobase, 3, 0);

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(is_forceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;
	}

	return 1;
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
is_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct is_softc *sc = (struct is_softc *)self;
	u_short iobase = ia->ia_iobase;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
	u_char i;

	sc->sc_iobase = iobase;

	/* Extract board address */
	for(i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_arpcom.ac_enaddr[i] = inb(iobase + (i * 2));

	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = iscd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_start = is_start;
	ifp->if_ioctl = is_ioctl;
	ifp->if_watchdog = is_watchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS;

	/*
	 * XXX -- not sure this is right place to do this
	 * Allocate memory for use by Lance
	 * Memory allocated for:
	 * 	initialisation block,
	 * 	ring descriptors,
	 * 	transmit and receive buffers.
	 */

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
	}

	/*
	 * Make sure it's quadword aligned?  This is kind of bogus, but
	 * what the Hell...
	 */
	sc->sc_init += 8 - ((u_long)sc->sc_init & 7);

	at_dma_cascade(ia->ia_drq);

	if_attach(ifp);

	/*
	 * Search down the ifa address list looking for the AF_LINK type entry
	 */
	ifa = ifp->if_addrlist;
	while ((ifa != 0) && (ifa->ifa_addr != 0) &&
	  (ifa->ifa_addr->sa_family != AF_LINK))
		ifa = ifa->ifa_next;

	/*
	 * If we find an AF_LINK type entry, we fill in the hardware address for
	 * this interface.
	 */
	if ((ifa != 0) && (ifa->ifa_addr != 0)) {
		/*
		 * Fill in the link level address for this interface
		 */
		sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		sdl->sdl_type = IFT_ETHER;
		sdl->sdl_alen = ETHER_ADDR_LEN;
		sdl->sdl_slen = 0;
		bcopy(sc->sc_arpcom.ac_enaddr, LLADDR(sdl), ETHER_ADDR_LEN);
	}

	printf (": address %s\n", ether_sprintf(sc->sc_arpcom.ac_enaddr));

#if NBPFILTER > 0
	bpfattach(&sc->sc_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	isa_establish(&sc->sc_id, &sc->sc_dev);
	sc->sc_ih.ih_fun = is_intr;
	sc->sc_ih.ih_arg = sc;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_IFNET);
}

void
is_reset(sc)
	struct is_softc *sc;
{

	log(LOG_NOTICE, "%s: reset\n", sc->sc_dev.dv_xname);
	is_init(sc);
}

void
is_watchdog(unit)
        short unit;
{
	struct is_softc *sc = iscd.cd_devs[unit];

        log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++sc->sc_arpcom.ac_if.if_oerrors;
        is_reset(sc);
}

/* Lance initialisation block set up */
void
init_mem(sc)
	struct is_softc *sc;
{
	int i;
	void *temp;

	/*
	 * At this point we assume that the
	 * memory allocated to the Lance is
	 * quadword aligned. If it isn't
	 * then the initialisation is going
	 * fail later on.
	 */

	/* 
	 * Set up lance initialisation block
	 */
	temp = (void *)sc->sc_init;
	temp += sizeof(struct init_block);
	sc->sc_rd = (struct mds *) temp;
	sc->sc_td = (struct mds *) (temp + (NRBUF*sizeof(struct mds)));
	temp += (NRBUF+NTBUF) * sizeof(struct mds);

	sc->sc_init->mode = 0;
	for (i = 0; i < ETHER_ADDR_LEN; i++) 
		sc->sc_init->padr[i] = sc->sc_arpcom.ac_enaddr[i];
	for (i = 0; i < 8; ++i)
		sc->sc_init->ladrf[i] = MULTI_INIT_ADDR;
	sc->sc_init->rdra = kvtop(sc->sc_rd);
	sc->sc_init->rlen = ((kvtop(sc->sc_rd) >> 16) & 0xff) | (RLEN<<13);
	sc->sc_init->tdra = kvtop(sc->sc_td);
	sc->sc_init->tlen = ((kvtop(sc->sc_td) >> 16) & 0xff) | (TLEN<<13);

	/* 
	 * Set up receive ring descriptors
	 */
	sc->sc_rbuf = (unsigned char *)temp;
	for (i = 0; i < NRBUF; i++) {
		(sc->sc_rd+i)->addr = kvtop(temp);
		(sc->sc_rd+i)->flags = ((kvtop(temp) >> 16) & 0xff) | OWN;
		(sc->sc_rd+i)->bcnt = -BUFSIZE;
		(sc->sc_rd+i)->mcnt = 0;
		temp += BUFSIZE;
	}

	/* 
	 * Set up transmit ring descriptors
	 */
	sc->sc_tbuf = (unsigned char *)temp;
	for (i = 0; i < NTBUF; i++) {
		(sc->sc_td+i)->addr = kvtop(temp);
		(sc->sc_td+i)->flags = ((kvtop(temp) >> 16) & 0xff);
		(sc->sc_td+i)->bcnt = 0;
		(sc->sc_td+i)->mcnt = 0;
		temp += BUFSIZE;
	}
}

void
is_stop(sc)
	struct is_softc *sc;
{

	iswrcsr(sc->sc_iobase, 0, STOP);
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
	u_short iobase = sc->sc_iobase;
	int s;
	register i;

	/* Address not known */
 	if (!ifp->if_addrlist)
		return;

	s = splnet();

	/* 
	 * Lance must be stopped
	 * to access registers.
	 */
	is_stop(sc);

	sc->sc_last_rd = sc->sc_last_td = sc->sc_no_td = 0;

	/* Set up lance's memory area */
	init_mem(sc);

	/* No byte swapping etc */
	iswrcsr(iobase, 3, 0);

	/* Give lance the physical address of its memory area */
	iswrcsr(iobase, 1, kvtop(sc->sc_init));
	iswrcsr(iobase, 2, (kvtop(sc->sc_init) >> 16) & 0xff);

	/* OK, let's try and initialise the Lance */
	iswrcsr(iobase, 0, INIT);

	/* Wait for initialisation to finish */
	for(i = 0; i < 1000; i++)
		if (isrdcsr(iobase, 0) & IDON)
			break;

	if (isrdcsr(iobase, 0) & IDON) {
		/* start lance */
		iswrcsr(iobase, 0, STRT|IDON|INEA);
		ifp->if_flags |= IFF_RUNNING;
        	ifp->if_flags &= ~IFF_OACTIVE;
		is_start(ifp);
	}
	else 
		printf("%s: card failed to initialise\n", sc->sc_dev.dv_xname);
	
	(void) splx(s);
}

/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue,
 * and map it to the interface before starting the output.
 * called only at splimp or interrupt level.
 */
void
is_start(ifp)
	struct ifnet *ifp;
{
	register struct is_softc *sc = iscd.cd_devs[ifp->if_unit];
	struct mbuf *m0, *m;
	u_char *buffer;
	int len;
	int i;
	struct mds *cdm;

	if ((sc->sc_arpcom.ac_if.if_flags ^ IFF_RUNNING) & (IFF_RUNNING|IFF_OACTIVE))
		return;

    outloop:
	if (++sc->sc_no_td >= NTBUF) {
		sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;	
#ifdef ISDEBUG
		if (is_debug)	
			printf("no_td = %x, last_td = %x\n",sc->sc_no_td, sc->sc_last_td);
#endif
		return;
	}

	cdm = (sc->sc_td + sc->sc_last_td);
#if 0 /* XXX redundant */
	if (cdm->flags & OWN)
		return;
#endif
	
	IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m);

	if (!m)
		return;

	/*
	 * copy the mbuf chain into the transmit buffer
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
                 * Copy ether header first, then residual data,
                 * then data. Put all this in a temporary buffer
                 * 'ether_packet' and send off to bpf. Since the
                 * system has generated this packet, we assume
                 * that all of the offsets in the packet are
                 * correct; if they're not, the system will almost
                 * certainly crash in m_copydata.
                 * We make no assumptions about how the data is
                 * arranged in the mbuf chain (i.e. how much
                 * data is in each mbuf, if mbuf clusters are
                 * used, etc.), which is why we use m_copydata
                 * to get the ether header rather than assume
                 * that this is located in the first mbuf.
                 */
                /* copy ether header */
                m_copydata(m0, 0, sizeof(struct ether_header), ep);
                eh = (struct ether_header *) ep;
                ep += sizeof(struct ether_header);
                etype = ntohs(eh->ether_type);
                if (etype >= ETHERTYPE_TRAIL &&
                    etype < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
			datasize = ((etype - ETHERTYPE_TRAIL) << 9);
			off = datasize + sizeof(struct ether_header);

			/* copy trailer_header into a data structure */
			m_copydata(m0, off, sizeof(struct trailer_header),
				&trailer_header.ether_type);

			/* copy residual data */
			m_copydata(m0, off+sizeof(struct trailer_header),
				resid = ntohs(trailer_header.ether_residual) -
				sizeof(struct trailer_header), ep);
			ep += resid;

			/* copy data */
			m_copydata(m0, sizeof(struct ether_header),
				datasize, ep);
			ep += datasize;

			/* restore original ether packet type */
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
	if (is_debug)
		xmit_print(ifp->if_unit, sc->sc_last_td);
#endif
		
	iswrcsr(sc->sc_iobase, 0, TDMD | INEA);
	if (++sc->sc_last_td >= NTBUF)
		sc->sc_last_td = 0;

	/* possibly more packets */
	goto outloop;
}


/*
 * Controller interrupt.
 */
int
is_intr(arg)
	void *arg;
{
	register struct is_softc *sc = arg;
	u_short isr;

	isr = isrdcsr(sc->sc_iobase, 0);
	if ((isr & INTR) == 0)
		return 0;

	do {
		if (isr & ERR) {
			if (isr & BABL) {
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
			iswrcsr(sc->sc_iobase, 0, BABL|CERR|MISS|MERR|INEA);
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
			/* reset watchdog timer */
			sc->sc_arpcom.ac_if.if_timer = 0;
			is_rint(sc);
		}
		if (isr & TINT) {
			/* reset watchdog timer */
			sc->sc_arpcom.ac_if.if_timer = 0;
			iswrcsr(sc->sc_iobase, 0, TINT|INEA);
			is_tint(sc);
		}

		isr = isrdcsr(sc->sc_iobase, 0);
	} while (isr & INTR);

	return 1;
}

void
is_tint(sc) 
	struct is_softc *sc;
{
	register struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int i, loopcount = 0;
	struct mds *cdm;

	do {
		if ((i = sc->sc_last_td - sc->sc_no_td) < 0)
			i += NTBUF;
		cdm = sc->sc_td+i;
#ifdef ISDEBUG
		if (is_debug)
			printf("trans cdm = %x\n",cdm);
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
	struct mds *cdm = (sc->sc_rd+rmd);

	/* out of sync with hardware, should never happen */
	if (cdm->flags & OWN) {
		printf("%s: error: out of sync\n", sc->sc_dev.dv_xname);
		iswrcsr(sc->sc_iobase, 0, RINT|INEA);
		return;
	}

	/* process all buffers with valid data */
	while ((cdm->flags & OWN) == 0) {
		/* clear interrupt to avoid race condition */
		iswrcsr(sc->sc_iobase, 0, RINT|INEA);
		if (cdm->flags & ERR) {
			if (cdm->flags & FRAM)
				printf("%s: FRAM\n", sc->sc_dev.dv_xname);
			if (cdm->flags & OFLO)
				printf("%s: OFLO\n", sc->sc_dev.dv_xname);
			if (cdm->flags & CRC)
				printf("%s: CRC\n", sc->sc_dev.dv_xname);
			if (cdm->flags & RBUFF)
				printf("%s: RBUFF\n", sc->sc_dev.dv_xname);
		} else if (cdm->flags & (STP|ENP) != (STP|ENP)) {
			do {
				iswrcsr(sc->sc_iobase, 0, RINT|INEA);
				cdm->mcnt = 0;
				cdm->flags |= OWN;	
				NEXTRDS;
			} while ((cdm->flags & (OWN|ERR|STP|ENP)) == 0);
			sc->sc_last_rd = rmd;
			printf("%s: chained buffer\n", sc->sc_dev.dv_xname);
			if ((cdm->flags & (OWN|ERR|STP|ENP)) != ENP) {
				is_reset(sc);
				return;
			}
		} else {
#ifdef ISDEBUG
			if (is_debug)
				recv_print(sc, sc->sc_last_rd);
#endif
			is_read(sc, sc->sc_rbuf + (BUFSIZE*rmd), (int)cdm->mcnt);
			sc->sc_arpcom.ac_if.if_ipackets++;
		}
			
		cdm->flags |= OWN;
		cdm->mcnt = 0;
		NEXTRDS;
#ifdef ISDEBUG
		if (is_debug)
			printf("last_rd = %x, cdm = %x\n", sc->sc_last_rd, cdm);
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

	/*
	 * Deal with trailer protocol: if type is trailer type
	 * get true type from first 16-bit word past data.
	 * Remember that type was trailer by setting off.
	 */
	eh = (struct ether_header *)buf;
	eh->ether_type = ntohs((u_short)eh->ether_type);
	len = len - sizeof(struct ether_header) - 4;
#define nedataaddr(eh, off, type)       ((type)(((caddr_t)((eh)+1)+(off))))
	if (eh->ether_type >= ETHERTYPE_TRAIL &&
	    eh->ether_type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {
		off = (eh->ether_type - ETHERTYPE_TRAIL) * 512;
		if (off >= ETHERMTU)
			return;
		eh->ether_type = ntohs(*nedataaddr(eh, off, u_short *));
		resid = ntohs(*(nedataaddr(eh, off+2, u_short *)));
		if (off + resid > len)
			return;
		len = off + resid;
	} else  off = 0;
	if (len == 0)
		return;

	/*
	 * Pull packet off interface.  Off is nonzero if packet
	 * has trailing header; neget will then force this header
	 * information to be at the front, but we still have to drop
	 * the type and length which are at the front of any trailer data.
	 */
	m = is_get(buf, len, off, &sc->sc_arpcom.ac_if);
	if (m == 0)
		return;

	sc->sc_arpcom.ac_if.if_ipackets++;

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
			bcmp(eh->ether_dhost, sc->sc_arpcom.ac_enaddr,
				sizeof(eh->ether_dhost)) != 0 &&
			bcmp(eh->ether_dhost, etherbroadcastaddr,
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
	return (top);
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
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct is_softc *sc = iscd.cd_devs[ifp->if_unit];
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
			((struct arpcom *)ifp)->ac_ipaddr =
				IA_SIN(ifa)->sin_addr;
			arpwhohas((struct arpcom *)ifp, &IA_SIN(ifa)->sin_addr);
			break;
#endif
#ifdef NS
		/*
		 * XXX - This code is probably wrong
		 */
		case AF_NS:
		    {
			register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);

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
			/*
			 * Set new address
			 */
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
		    ifp->if_flags & IFF_RUNNING) {
			is_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else {
		/*
		 * If interface is marked up and it is stopped, then start it
		 */
			if (ifp->if_flags & IFF_UP &&
			    (ifp->if_flags & IFF_RUNNING) == 0)
			is_init(sc);
		}
#ifdef ISDEBUG
		if (ifp->if_flags & IFF_DEBUG)
			is_debug = 1;
		else
			is_debug = 0;
#endif
#if NBPFILTER > 0
		if (ifp->if_flags & IFF_PROMISC) {
			/*
			 * Set promiscuous mode on interface.
			 *      XXX - for multicasts to work, we would need to
			 *	      write 1's in all bits of multicast
			 *	      hashing array. For now we assume that
			 *	      this was done in is_init().
			 */
			 sc->sc_init->mode = PROM;	
		} else
			/*
			 * XXX - for multicasts to work, we would need to
			 *      rewrite the multicast hashing array with the
			 *      proper hash (would have been destroyed above).
			 */
			{ /* Don't know about this */};
#endif
		break;

#ifdef notdef
	case SIOCGHWADDR:
		bcopy((caddr_t)sc->sc_arpcom.ac_enaddr, (caddr_t) &ifr->ifr_data,
		      sizeof(sc->sc_arpcom.ac_enaddr));
		break;
#endif

	default:
		error = EINVAL;
	}
	(void) splx(s);
	return (error);
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
	
	rmd = sc->sc_rd+no;
	len = rmd->mcnt;
	printf("%s: receive buffer %d, len = %d\n",sc->sc_dev.dv_xname, no, len);
	printf("%s: status %x\n", sc->sc_dev.dv_xname, isrdcsr(sc->sc_iobase, 0));
	for (i = 0; i < len; i++) {
		if (!printed) {
			printed=1;
			printf("%s: data: ", sc->sc_dev.dv_xname);
		}
		printf("%x ",*(sc->sc_rbuf + (BUFSIZE*no) + i));
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
	int i, printed = 0;
	u_short len;
	
	rmd = (sc->sc_td+no);
	len = -(rmd->bcnt);
	printf("%s: transmit buffer %d, len = %d\n", sc->sc_dev.dv_xname, no, len);
	printf("%s: status %x\n", sc->sc_dev.dv_xname, isrdcsr(sc->sc_iobase, 0));
	printf("%s: addr %x, flags %x, bcnt %x, mcnt %x\n", sc->sc_dev.dv_xname,
	       rmd->addr,rmd->flags,rmd->bcnt,rmd->mcnt);
	for (i = 0; i < len; i++)  {
		if (!printed) {
			printed = 1;
			printf("%s: data: ", sc->sc_dev.dv_xname);
		}
		printf("%x ",*(sc->sc_tbuf + (BUFSIZE*no) + i));
	}
	if (printed)
		printf("\n");
}
#endif /* ISDEBUG */
