/*	$NetBSD: if_le.c,v 1.25 1995/04/17 12:09:06 cgd Exp $	*/

/*
 * LANCE Ethernet driver
 *
 * Copyright (c) 1994 Charles Hannum.
 *
 * Copyright (C) 1993, Paul Richards. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
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

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <i386/isa/isa_machdep.h>	/* XXX USES ISA HOLE DIRECTLY */
#include <dev/isa/isadmavar.h>
#include <dev/isa/if_lereg.h>


#define	ETHER_MIN_LEN	64
#define	ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

char *card_type[] = {"unknown", "BICC Isolan", "NE2100", "DEPCA"};
char *chip_type[] = {"unknown", "Am7990 LANCE", "Am79960 PCnet-ISA"};


/*
 * Ethernet software status per interface.
 *
 * Each interface is referenced by a network interface structure,
 * arpcom.ac_if, which the routing code uses to locate the interface.
 * This structure contains the output queue for the interface, its address, ...
 */
struct le_softc {
	struct	device sc_dev;
	void *sc_ih;

	struct	arpcom sc_arpcom;	/* Ethernet common part */
	int	sc_iobase;		/* IO base address of card */
	int	sc_rap, sc_rdp;
	int	sc_chip, sc_card;
	void	*sc_mem;
	struct	init_block *sc_init;	/* Lance initialisation block */
	struct	mds *sc_rd, *sc_td;
	u_char	*sc_rbuf, *sc_tbuf;
	int	sc_last_rd, sc_last_td;
	int	sc_no_td;
#ifdef LEDEBUG
	int	sc_debug;
#endif
};

int leintr __P((void *));
int leioctl __P((struct ifnet *, u_long, caddr_t));
void lestart __P((struct ifnet *));
void lewatchdog __P((short));
static inline void lewrcsr __P((struct le_softc *, u_short, u_short));
static inline u_short lerdcsr __P((struct le_softc *, u_short));
void leinit __P((struct le_softc *));
void lememinit __P((struct le_softc *));
void lereset __P((struct le_softc *));
void lestop __P((struct le_softc *));
void letint __P((struct le_softc *));
void lerint __P((struct le_softc *));
void leread __P((struct le_softc *, u_char *, int));
struct mbuf *leget __P((u_char *, int, struct ifnet *));
#ifdef LEDEBUG
void recv_print __P((struct le_softc *, int));
void xmit_print __P((struct le_softc *, int));
#endif
void lesetladrf __P((struct arpcom *, u_long *));

int leprobe __P((struct device *, void *, void *));
int depca_probe __P((struct le_softc *, struct isa_attach_args *));
int ne2100_probe __P((struct le_softc *, struct isa_attach_args *));
int bicc_probe __P((struct le_softc *, struct isa_attach_args *));
int lance_probe __P((struct le_softc *));
void leattach __P((struct device *, struct device *, void *));

struct cfdriver lecd = {
	NULL, "le", leprobe, leattach, DV_IFNET, sizeof(struct le_softc)
};

static inline void
lewrcsr(sc, port, val)
	struct le_softc *sc;
	u_short port;
	u_short val;
{

	outw(sc->sc_rap, port);
	outw(sc->sc_rdp, val);
}

static inline u_short
lerdcsr(sc, port)
	struct le_softc *sc;
	u_short port;
{
	
	outw(sc->sc_rap, port);
	return inw(sc->sc_rdp);
} 

int
leprobe(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct le_softc *sc = match;
	struct isa_attach_args *ia = aux;

	if (bicc_probe(sc, ia))
		goto found;
	if (ne2100_probe(sc, ia))
		goto found;
	if (depca_probe(sc, ia))
		goto found;
	return 0;

found:
	/*
	 * XXX - hopefully have better way to get dma'able memory later,
	 * this code assumes that the physical memory address returned
	 * from malloc will be below 16Mb. The Lance's address registers
	 * are only 24 bits wide!
	 */
#define MAXMEM ((NRBUF + NTBUF) * (BUFSIZE + sizeof(struct mds)) + \
		sizeof(struct init_block))
	if (sc->sc_card == DEPCA) {
		u_char *mem, val;
		int i;

		mem = sc->sc_mem = ISA_HOLE_VADDR(ia->ia_maddr);
		/* XXX This is somewhat bogus. */
		if (ia->ia_msize < MAXMEM) {
			printf("%s: not enough memory configured\n",
			    sc->sc_dev.dv_xname);
			return 0;
		}

		val = 0xff;
		for (;;) {
			for (i = 0; i < ia->ia_msize; i++)
				mem[i] = val;
			for (i = 0; i < ia->ia_msize; i++)
				if (mem[i] != val) {
					printf("%s: failed to clear memory\n",
					    sc->sc_dev.dv_xname);
					return 0;
				}
			if (val == 0x00)
				break;
			val -= 0x55;
		}
	} else {
		sc->sc_mem = malloc(MAXMEM, M_TEMP, M_NOWAIT);
		if (!sc->sc_mem) {
			printf("%s: couldn't allocate memory for card\n",
			    sc->sc_dev.dv_xname);
			return 0;
		}
	}

	return 1;
}

int
depca_probe(sc, ia)
	struct le_softc *sc;
	struct isa_attach_args *ia;
{
	int iobase = ia->ia_iobase, port;
	u_long sum, rom_sum;
	u_char x;
	int i;

	sc->sc_iobase = iobase;
	sc->sc_rap = iobase + DEPCA_RAP;
	sc->sc_rdp = iobase + DEPCA_RDP;
	sc->sc_card = DEPCA;

	if (!(sc->sc_chip = lance_probe(sc)))
		return 0;

	outb(iobase + DEPCA_CSR, DEPCA_CSR_DUM);

	/*
	 * Extract the physical MAC address from the ROM.
	 *
	 * The address PROM is 32 bytes wide, and we access it through
	 * a single I/O port.  On each read, it rotates to the next
	 * position.  We find the ethernet address by looking for a
	 * particular sequence of bytes (0xff, 0x00, 0x55, 0xaa, 0xff,
	 * 0x00, 0x55, 0xaa), and then reading the next 8 bytes (the
	 * ethernet address and a checksum).
	 *
	 * It appears that the PROM can be at one of two locations, so
	 * we just try both.
	 */
	port = iobase + DEPCA_ADP;
	for (i = 0; i < 32; i++)
		if (inb(port) == 0xff && inb(port) == 0x00 &&
		    inb(port) == 0x55 && inb(port) == 0xaa &&
		    inb(port) == 0xff && inb(port) == 0x00 &&
		    inb(port) == 0x55 && inb(port) == 0xaa)
			goto found;
	port = iobase + DEPCA_ADP + 1;
	for (i = 0; i < 32; i++)
		if (inb(port) == 0xff && inb(port) == 0x00 &&
		    inb(port) == 0x55 && inb(port) == 0xaa &&
		    inb(port) == 0xff && inb(port) == 0x00 &&
		    inb(port) == 0x55 && inb(port) == 0xaa)
			goto found;
	printf("%s: address not found\n", sc->sc_dev.dv_xname);
	return 0;

found:
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_arpcom.ac_enaddr[i] = inb(port);

#if 0
	sum =
	    (sc->sc_arpcom.ac_enaddr[0] <<  2) +
	    (sc->sc_arpcom.ac_enaddr[1] << 10) +
	    (sc->sc_arpcom.ac_enaddr[2] <<  1) +
	    (sc->sc_arpcom.ac_enaddr[3] <<  9) +
	    (sc->sc_arpcom.ac_enaddr[4] <<  0) +
	    (sc->sc_arpcom.ac_enaddr[5] <<  8);
	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	rom_sum = inb(port);
	rom_sum |= inb(port) << 8;

	if (sum != rom_sum) {
		printf("%s: checksum mismatch; calculated %04x != read %04x",
		    sc->sc_dev.dv_xname, sum, rom_sum);
		return 0;
	}
#endif

	outb(iobase + DEPCA_CSR, DEPCA_CSR_NORMAL);

	ia->ia_iosize = 16;
	ia->ia_drq = DRQUNK;
	return 1;
}

int
ne2100_probe(sc, ia)
	struct le_softc *sc;
	struct isa_attach_args *ia;
{
	int iobase = ia->ia_iobase;
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

	ia->ia_iosize = 24;
	return 1;
}

int
bicc_probe(sc, ia)
	struct le_softc *sc;
	struct isa_attach_args *ia;
{
	int iobase = ia->ia_iobase;
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

	ia->ia_iosize = 16;
	return 1;
}

/*
 * Determine which chip is present on the card.
 */
int
lance_probe(sc)
	struct le_softc *sc;
{
	int type;

	/* Stop the LANCE chip and put it in a known state. */
	lewrcsr(sc, 0, LE_STOP);
	delay(100);

	if (lerdcsr(sc, 0) != LE_STOP)
		return 0;

	/*
	 * The PCnet-ISA chip doesn't allow some bits to be set.
	 */
	lewrcsr(sc, 3, PROBE_MASK);

	switch (lerdcsr(sc, 3) & PROBE_MASK) {
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

	lewrcsr(sc, 3, sc->sc_card == DEPCA ? LE_ACON : 0);
	return type;
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.  We get the ethernet address here.
 */
void
leattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct le_softc *sc = (void *)self;
	struct isa_attach_args *ia = aux;
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;

	ifp->if_unit = sc->sc_dev.dv_unit;
	ifp->if_name = lecd.cd_name;
	ifp->if_output = ether_output;
	ifp->if_start = lestart;
	ifp->if_ioctl = leioctl;
	ifp->if_watchdog = lewatchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	if (ia->ia_drq != DRQUNK)
		isa_dmacascade(ia->ia_drq);

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp);

	printf(": address %s, type %s %s\n",
	    ether_sprintf(sc->sc_arpcom.ac_enaddr),
	    card_type[sc->sc_card], chip_type[sc->sc_chip]);

#if NBPFILTER > 0
	bpfattach(&sc->sc_arpcom.ac_if.if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

	sc->sc_ih = isa_intr_establish(ia->ia_irq, ISA_IST_EDGE, ISA_IPL_NET,
	    leintr, sc);
}

void
lereset(sc)
	struct le_softc *sc;
{

	leinit(sc);
}

void
lewatchdog(unit)
	short unit;
{
	struct le_softc *sc = lecd.cd_devs[unit];

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++sc->sc_arpcom.ac_if.if_oerrors;
	lereset(sc);
}

#define	LANCE_ADDR(sc, a) \
	(sc->sc_card == DEPCA ?	((u_long)(a) - (u_long)sc->sc_mem) : kvtop(a))

/* LANCE initialization block set up. */
void
lememinit(sc)
	register struct le_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int i;
	void *mem;
	u_long a;

	/*
	 * At this point we assume that the memory allocated to the Lance is
	 * quadword aligned.  If it isn't then the initialisation is going
	 * fail later on.
	 */
	mem = sc->sc_mem;

	sc->sc_init = mem;
#if NBPFILTER > 0
	if (ifp->if_flags & IFF_PROMISC)
		sc->sc_init->mode = LE_NORMAL | LE_PROM;
	else
#endif
		sc->sc_init->mode = LE_NORMAL;
	for (i = 0; i < ETHER_ADDR_LEN; i++)
		sc->sc_init->padr[i] = sc->sc_arpcom.ac_enaddr[i];
	lesetladrf(&sc->sc_arpcom, sc->sc_init->ladrf);
	mem += sizeof(struct init_block);

	sc->sc_rd = mem;
	a = LANCE_ADDR(sc, mem);
	sc->sc_init->rdra = a;
	sc->sc_init->rlen = ((a >> 16) & 0xff) | (RLEN << 13);
	mem += NRBUF * sizeof(struct mds);

	sc->sc_td = mem;
	a = LANCE_ADDR(sc, mem);
	sc->sc_init->tdra = a;
	sc->sc_init->tlen = ((a >> 16) & 0xff) | (TLEN << 13);
	mem += NTBUF * sizeof(struct mds);

	/* 
	 * Set up receive ring descriptors.
	 */
	sc->sc_rbuf = mem;
	for (i = 0; i < NRBUF; i++) {
		a = LANCE_ADDR(sc, mem);
		sc->sc_rd[i].addr = a;
		sc->sc_rd[i].flags = ((a >> 16) & 0xff) | LE_OWN;
		sc->sc_rd[i].bcnt = -BUFSIZE;
		sc->sc_rd[i].mcnt = 0;
		mem += BUFSIZE;
	}

	/* 
	 * Set up transmit ring descriptors.
	 */
	sc->sc_tbuf = mem;
	for (i = 0; i < NTBUF; i++) {
		a = LANCE_ADDR(sc, mem);
		sc->sc_td[i].addr = a;
		sc->sc_td[i].flags= ((a >> 16) & 0xff);
		sc->sc_td[i].bcnt = 0xf000;
		sc->sc_td[i].mcnt = 0;
		mem += BUFSIZE;
	}
}

void
lestop(sc)
	struct le_softc *sc;
{

	lewrcsr(sc, 0, LE_STOP);
}

/*
 * Initialization of interface; set up initialization block
 * and transmit/receive descriptor rings.
 */
void
leinit(sc)
	register struct le_softc *sc;
{
	struct ifnet *ifp = &sc->sc_arpcom.ac_if;
	int s;
	register int timo;
	u_long a;

	/* Address not known. */
	if (!ifp->if_addrlist)
		return;

	s = splimp();

	/* Don't want to get in a weird state. */
	lewrcsr(sc, 0, LE_STOP);
	delay(100);

	sc->sc_last_rd = sc->sc_last_td = sc->sc_no_td = 0;

	/* Set up LANCE init block. */
	lememinit(sc);

	/* No byte swapping etc. */
	lewrcsr(sc, 3, sc->sc_card == DEPCA ? LE_ACON : 0);

	/* Give LANCE the physical address of its init block. */
	a = LANCE_ADDR(sc, sc->sc_init);
	lewrcsr(sc, 1, a);
	lewrcsr(sc, 2, (a >> 16) & 0xff);

	/* Try to initialize the LANCE. */
	delay(100);
	lewrcsr(sc, 0, LE_INIT);

	/* Wait for initialization to finish. */
	for (timo = 1000; timo; timo--)
		if (lerdcsr(sc, 0) & LE_IDON)
			break;

	if (lerdcsr(sc, 0) & LE_IDON) {
		/* Start the LANCE. */
		lewrcsr(sc, 0, LE_INEA | LE_STRT | LE_IDON);
		ifp->if_flags |= IFF_RUNNING;
		ifp->if_flags &= ~IFF_OACTIVE;
		lestart(ifp);
	} else
		printf("%s: card failed to initialize\n", sc->sc_dev.dv_xname);
	
	(void) splx(s);
}

/*
 * Controller interrupt.
 */
int
leintr(arg)
	void *arg;
{
	register struct le_softc *sc = arg;
	register u_short isr;

	isr = lerdcsr(sc, 0);
#ifdef LEDEBUG
	if (sc->sc_debug)
		printf("%s: leintr entering with isr=%04x\n",
		    sc->sc_dev.dv_xname, isr);
#endif
	if ((isr & LE_INTR) == 0)
		return 0;

	if (sc->sc_card == DEPCA)
		outb(sc->sc_iobase + DEPCA_CSR, DEPCA_CSR_NORMAL|DEPCA_CSR_IM);

	do {
		lewrcsr(sc, 0,
		    isr & (LE_INEA | LE_BABL | LE_MISS | LE_MERR |
			   LE_RINT | LE_TINT | LE_IDON));
		if (isr & (LE_BABL | LE_CERR | LE_MISS | LE_MERR)) {
			if (isr & LE_BABL) {
				printf("%s: babble\n", sc->sc_dev.dv_xname);
				sc->sc_arpcom.ac_if.if_oerrors++;
			}
#if 0
			if (isr & LE_CERR) {
				printf("%s: collision error\n", sc->sc_dev.dv_xname);
				sc->sc_arpcom.ac_if.if_collisions++;
			}
#endif
			if (isr & LE_MISS) {
#if 0
				printf("%s: missed packet\n", sc->sc_dev.dv_xname);
#endif
				sc->sc_arpcom.ac_if.if_ierrors++;
			}
			if (isr & LE_MERR) {
				printf("%s: memory error\n", sc->sc_dev.dv_xname);
				lereset(sc);
				goto out;
			}
		}

		if ((isr & LE_RXON) == 0) {
			printf("%s: receiver disabled\n", sc->sc_dev.dv_xname);
			sc->sc_arpcom.ac_if.if_ierrors++;
			lereset(sc);
			goto out;
		}
		if ((isr & LE_TXON) == 0) {
			printf("%s: transmitter disabled\n", sc->sc_dev.dv_xname);
			sc->sc_arpcom.ac_if.if_oerrors++;
			lereset(sc);
			goto out;
		}

		if (isr & LE_RINT) {
			/* Reset watchdog timer. */
			sc->sc_arpcom.ac_if.if_timer = 0;
			lerint(sc);
		}
		if (isr & LE_TINT) {
			/* Reset watchdog timer. */
			sc->sc_arpcom.ac_if.if_timer = 0;
			letint(sc);
		}

		isr = lerdcsr(sc, 0);
	} while ((isr & LE_INTR) != 0);

#ifdef LEDEBUG
	if (sc->sc_debug)
		printf("%s: leintr returning with isr=%04x\n",
		    sc->sc_dev.dv_xname, isr);
#endif

out:
	if (sc->sc_card == DEPCA)
		outb(sc->sc_iobase + DEPCA_CSR, DEPCA_CSR_NORMAL);
	return 1;
}

#define NEXTTDS \
	if (++tmd == NTBUF) tmd=0, cdm=sc->sc_td; else ++cdm
	
/*
 * Setup output on interface.
 * Get another datagram to send off of the interface queue, and map it to the
 * interface before starting the output.
 * Called only at splimp or interrupt level.
 */
void
lestart(ifp)
	struct ifnet *ifp;
{
	register struct le_softc *sc = lecd.cd_devs[ifp->if_unit];
	register int tmd;
	struct mds *cdm;
	struct mbuf *m0, *m;
	u_char *buffer;
	int len;

	if ((sc->sc_arpcom.ac_if.if_flags & (IFF_RUNNING | IFF_OACTIVE)) !=
	    IFF_RUNNING)
		return;

	tmd = sc->sc_last_td;
	cdm = &sc->sc_td[tmd];

	for (;;) {
		if (sc->sc_no_td >= NTBUF) {
			sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;
#ifdef LEDEBUG
			if (sc->sc_debug)
				printf("no_td = %d, last_td = %d\n", sc->sc_no_td,
				    sc->sc_last_td);
#endif
			break;
		}

#ifdef LEDEBUG
		if (cdm->flags & LE_OWN) {
			sc->sc_arpcom.ac_if.if_flags |= IFF_OACTIVE;
			printf("missing buffer, no_td = %d, last_td = %d\n",
			    sc->sc_no_td, sc->sc_last_td);
		}
#endif

		IF_DEQUEUE(&sc->sc_arpcom.ac_if.if_snd, m);
		if (!m)
			break;

		++sc->sc_no_td;

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

#ifdef LEDEBUG
		if (len > ETHER_MAX_LEN)
			printf("packet length %d\n", len);
#endif

#if NBPFILTER > 0
		if (sc->sc_arpcom.ac_if.if_bpf)
			bpf_mtap(sc->sc_arpcom.ac_if.if_bpf, m0);
#endif

		m_freem(m0);
		len = max(len, ETHER_MIN_LEN);

		/*
		 * Init transmit registers, and set transmit start flag.
		 */
		cdm->bcnt = -len;
		cdm->mcnt = 0;
		cdm->flags |= LE_OWN | LE_STP | LE_ENP;

#ifdef LEDEBUG
		if (sc->sc_debug)
			xmit_print(sc, sc->sc_last_td);
#endif
		
		lewrcsr(sc, 0, LE_INEA | LE_TDMD);

		NEXTTDS;
	}

	sc->sc_last_td = tmd;
}

void
letint(sc) 
	struct le_softc *sc;
{
	register int tmd = (sc->sc_last_td - sc->sc_no_td + NTBUF) % NTBUF;
	struct mds *cdm = &sc->sc_td[tmd];

	if (cdm->flags & LE_OWN) {
		/* Race condition with loop below. */
#ifdef LEDEBUG
		if (sc->sc_debug)
			printf("%s: extra tint\n", sc->sc_dev.dv_xname);
#endif
		return;
	}

	sc->sc_arpcom.ac_if.if_flags &= ~IFF_OACTIVE;

	do {
		if (sc->sc_no_td <= 0)
			break;
#ifdef LEDEBUG
		if (sc->sc_debug)
			printf("trans cdm = %x\n", cdm);
#endif
		sc->sc_arpcom.ac_if.if_opackets++;
		--sc->sc_no_td;
		if (cdm->mcnt & (LE_TBUFF | LE_UFLO | LE_LCOL | LE_LCAR | LE_RTRY)) {
			if (cdm->mcnt & LE_TBUFF)
				printf("%s: transmit buffer error\n", sc->sc_dev.dv_xname);
			if ((cdm->mcnt & (LE_TBUFF | LE_UFLO)) == LE_UFLO)
				printf("%s: underflow\n", sc->sc_dev.dv_xname);
			if (cdm->mcnt & LE_UFLO) {
				lereset(sc);
				return;
			}
#if 0
			if (cdm->mcnt & LE_LCOL) {
				printf("%s: late collision\n", sc->sc_dev.dv_xname);
				sc->sc_arpcom.ac_if.if_collisions++;
			}
			if (cdm->mcnt & LE_LCAR)
				printf("%s: lost carrier\n", sc->sc_dev.dv_xname);
			if (cdm->mcnt & LE_RTRY) {
				printf("%s: excessive collisions, tdr %d\n",
				    sc->sc_dev.dv_xname, cdm->flags & 0x1ff);
				sc->sc_arpcom.ac_if.if_collisions += 16;
			}
#endif
		} else if (cdm->flags & LE_ONE)
			sc->sc_arpcom.ac_if.if_collisions++;
		else if (cdm->flags & LE_MORE)
			/* Real number is unknown. */
			sc->sc_arpcom.ac_if.if_collisions += 2;
		NEXTTDS;
	} while ((cdm->flags & LE_OWN) == 0);

	lestart(&sc->sc_arpcom.ac_if);
}

#define NEXTRDS \
	if (++rmd == NRBUF) rmd=0, cdm=sc->sc_rd; else ++cdm
	
/* only called from one place, so may as well integrate */
void
lerint(sc)
	struct le_softc *sc;
{
	register int rmd = sc->sc_last_rd;
	struct mds *cdm = &sc->sc_rd[rmd];

	if (cdm->flags & LE_OWN) {
		/* Race condition with loop below. */
#ifdef LEDEBUG
		if (sc->sc_debug)
			printf("%s: extra rint\n", sc->sc_dev.dv_xname);
#endif
		return;
	}

	/* Process all buffers with valid data. */
	do {
		if (cdm->flags & (LE_FRAM | LE_OFLO | LE_CRC | LE_RBUFF)) {
			if ((cdm->flags & (LE_FRAM | LE_OFLO | LE_ENP)) == (LE_FRAM | LE_ENP))
				printf("%s: framing error\n", sc->sc_dev.dv_xname);
			if ((cdm->flags & (LE_OFLO | LE_ENP)) == LE_OFLO)
				printf("%s: overflow\n", sc->sc_dev.dv_xname);
			if ((cdm->flags & (LE_CRC | LE_OFLO | LE_ENP)) == (LE_CRC | LE_ENP))
				printf("%s: crc mismatch\n", sc->sc_dev.dv_xname);
			if (cdm->flags & LE_RBUFF)
				printf("%s: receive buffer error\n", sc->sc_dev.dv_xname);
		} else if (cdm->flags & (LE_STP | LE_ENP) != (LE_STP | LE_ENP)) {
			do {
				cdm->mcnt = 0;
				cdm->flags |= LE_OWN;
				NEXTRDS;
			} while ((cdm->flags & (LE_OWN | LE_ERR | LE_STP | LE_ENP)) == 0);
			sc->sc_last_rd = rmd;
			printf("%s: chained buffer\n", sc->sc_dev.dv_xname);
			if ((cdm->flags & (LE_OWN | LE_ERR | LE_STP | LE_ENP)) != LE_ENP) {
				lereset(sc);
				return;
			}
		} else {
#ifdef LEDEBUG
			if (sc->sc_debug)
				recv_print(sc, sc->sc_last_rd);
#endif
			leread(sc, sc->sc_rbuf + (BUFSIZE * rmd),
			    (int)cdm->mcnt);
			sc->sc_arpcom.ac_if.if_ipackets++;
		}

		cdm->bcnt = -BUFSIZE;
		cdm->mcnt = 0;
		cdm->flags |= LE_OWN;
		NEXTRDS;
#ifdef LEDEBUG
		if (sc->sc_debug)
			printf("sc->sc_last_rd = %x, cdm = %x\n",
			    sc->sc_last_rd, cdm);
#endif
	} while ((cdm->flags & LE_OWN) == 0);

	sc->sc_last_rd = rmd;
}

/*
 * Pass a packet to the higher levels.
 */
void
leread(sc, buf, len)
	register struct le_softc *sc;
	u_char *buf;
	int len;
{
	struct ifnet *ifp;
	struct mbuf *m;
	struct ether_header *eh;

	len -= 4;
	if (len <= 0)
		return;

	/* Pull packet off interface. */
	ifp = &sc->sc_arpcom.ac_if;
	m = leget(buf, len, ifp);
	if (m == 0)
		return;

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
 * Supporting routines
 */

/*
 * Pull data off an interface.
 * Len is length of data, with local net header stripped.
 * We copy the data into mbufs.  When full cluster sized units are present
 * we copy into clusters.
 */
struct mbuf *
leget(buf, totlen, ifp)
	u_char *buf;
	int totlen;
	struct ifnet *ifp;
{
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
		bcopy((caddr_t)buf, mtod(m, caddr_t), len);
		buf += len;
		totlen -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return top;
}

/*
 * Process an ioctl request.
 */
int
leioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct le_softc *sc = lecd.cd_devs[ifp->if_unit];
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
			leinit(sc);	/* before arpwhohas */
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
			else
				bcopy(ina->x_host.c_host,
				    sc->sc_arpcom.ac_enaddr,
				    sizeof(sc->sc_arpcom.ac_enaddr));
			/* Set new address. */
			leinit(sc);
			break;
		    }
#endif
		default:
			leinit(sc);
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
			lestop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			leinit(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			/*lestop(sc);*/
			leinit(sc);
		}
#ifdef LEDEBUG
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
			leinit(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
	}
	(void) splx(s);
	return error;
}

#ifdef LEDEBUG
void
recv_print(sc, no)
	struct le_softc *sc;
	int no;
{
	struct mds *rmd;
	int i, printed = 0;
	u_short len;
	
	rmd = &sc->sc_rd[no];
	len = rmd->mcnt;
	printf("%s: receive buffer %d, len = %d\n", sc->sc_dev.dv_xname, no,
	    len);
	printf("%s: status %x\n", sc->sc_dev.dv_xname, lerdcsr(sc, 0));
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
	struct le_softc *sc;
	int no;
{
	struct mds *rmd;
	int i, printed=0;
	u_short len;
	
	rmd = &sc->sc_td[no];
	len = -rmd->bcnt;
	printf("%s: transmit buffer %d, len = %d\n", sc->sc_dev.dv_xname, no,
	    len);
	printf("%s: status %x\n", sc->sc_dev.dv_xname, lerdcsr(sc, 0));
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
#endif /* LEDEBUG */

/*
 * Set up the logical address filter.
 */
void
lesetladrf(ac, af)
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

	if (ifp->if_flags & IFF_PROMISC) {
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
				if ((crc & 0x01) ^ (c & 0x01)) {
					crc >>= 1;
					crc ^= 0x6db88320 | 0x80000000;
				} else
					crc >>= 1;
				c >>= 1;
			}
		}
		/* Just want the 6 most significant bits. */
		crc >>= 26;

		/* Turn on the corresponding bit in the filter. */
		af[crc >> 5] |= 1 << ((crc & 0x1f) ^ 0);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
}
