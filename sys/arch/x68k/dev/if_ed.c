/*	$NetBSD: if_ed.c,v 1.10 1999/02/28 17:11:43 explorer Exp $	*/

/*
 * Device driver for National Semiconductor DS8390/WD83C690 based ethernet
 * adapters.
 *
 * Copyright (c) 1994, 1995 Charles M. Hannum.  All rights reserved.
 *
 * Copyright (C) 1993, David Greenman.  This software may be used, modified,
 * copied, distributed, and sold, in both source and binary form provided that
 * the above copyright and these terms are retained.  Under no circumstances is
 * the author responsible for the proper functioning of this software, nor does
 * the author assume any responsibility for damages incurred with its use.
 *
 * Currently supports the Western Digital/SMC 8003 and 8013 series, the SMC
 * Elite Ultra (8216), the 3Com 3c503, the NE1000 and NE2000, and a variety of
 * similar clones.
 */

/*
 * - NetBSD/x68k -
 * Supports only the Neptune-X card based on NE2000.
 * modefied by Y.YAMASAKI <yamasaki@ise.eng.osaka-u.ac.jp>
 */

#include "opt_inet.h"
#include "opt_ns.h"
#include "bpfilter.h"
#include "rnd.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/device.h>

#if NRND > 0
#include <sys/rnd.h>
#endif

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

#include <x68k/x68k/iodevice.h>
#include <x68k/dev/dp8390reg.h>
#include <x68k/dev/if_edreg.h>

/*
 * ed_softc: per line info and status
 */
struct ed_softc {
	struct	device sc_dev;

	struct	ethercom sc_ethercom;	/* ethernet common */

	char	*type_str;	/* pointer to type string */
	u_char	type;		/* interface type code */

	caddr_t	asic_base;	/* Base ASIC I/O port */
	caddr_t	nic_base;	/* Base NIC (DS8390) I/O port */

	u_char	cr_proto;	/* values always set in CR */
	u_char	isa16bit;	/* width of access to card 0=8 or 1=16 */

	caddr_t	mem_start;	/* NIC memory start address */
	caddr_t	mem_end;	/* NIC memory end address */
	u_long	mem_size;	/* total NIC memory size */
	caddr_t	mem_ring;	/* start of RX ring-buffer (in NIC mem) */

	u_char	txb_cnt;	/* number of transmit buffers */
	u_char	txb_inuse;	/* number of transmit buffers active */

	u_char 	txb_new;	/* pointer to where new buffer will be added */
	u_char	txb_next_tx;	/* pointer to next buffer ready to xmit */
	u_short	txb_len[8];	/* buffered xmit buffer lengths */
	u_char	tx_page_start;	/* first page of TX buffer area */
	u_char	rec_page_start;	/* first page of RX ring-buffer */
	u_char	rec_page_stop;	/* last page of RX ring-buffer */
	u_char	next_packet;	/* pointer to next unread RX packet */

	u_int8_t sc_enaddr[6];

#if NRND > 0
	rndsource_element_t rnd_source;
#endif
};

int edmatch __P((struct device *, struct cfdata *, void *));
void edattach __P((struct device *, struct device *, void *));
int ed_probe_generic8390 __P((caddr_t));
int ed_find_Novell __P((caddr_t, caddr_t));
int ed_check_type __P((struct ed_softc *));
int edintr __P((int));
int edioctl __P((struct ifnet *, u_long, caddr_t));
void edstart __P((struct ifnet *));
void edwatchdog __P((struct ifnet *));
void edreset __P((struct ed_softc *));
void edinit __P((struct ed_softc *));
void edstop __P((struct ed_softc *));

/* #define inline	/* XXX for debugging porpoises */

void ed_getmcaf __P((struct ethercom *, u_long *));
void edread __P((struct ed_softc *, caddr_t, int));
struct mbuf *edget __P((struct ed_softc *, caddr_t, int));
static inline void ed_rint __P((struct ed_softc *));
static inline void ed_xmit __P((struct ed_softc *));
static inline caddr_t ed_ring_copy __P((struct ed_softc *, caddr_t, caddr_t,
					u_short));

void ed_pio_readmem __P((struct ed_softc *, u_short, caddr_t, u_short));
void ed_pio_writemem __P((struct ed_softc *, caddr_t, u_short, u_short));
u_short ed_pio_write_mbufs __P((struct ed_softc *, struct mbuf *, u_short));

struct cfattach ed_ca = {
	sizeof(struct ed_softc), edmatch, edattach
};

extern struct cfdriver ed_cd;

#define	ETHER_MIN_LEN	64
#define ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6

#define	outb(addr, val)		*(volatile u_char *)(addr) = (val)
#define	inb(addr)		*(volatile u_char *)(addr)
#define outw(addr, val)		*(volatile u_short *)(addr) = (val)

static inline void
insb(void *addr, void *dst, int cnt)
{
	cnt--;
	asm volatile("1: moveb %4@,%1@+; dbra %0,1b" :
			"=d" (cnt), "=a" (dst) :
			"0" (cnt), "1" (dst), "a" (addr));
}
static inline void
insw(void *addr, void *dst, int cnt)
{
	cnt--;
	asm volatile("1: movew %4@,%1@+; dbra %0,1b" :
			"=d" (cnt), "=a" (dst) :
			"0" (cnt), "1" (dst), "a" (addr));
}

static inline void
outsb(void *addr, void *src, int cnt)
{
	cnt--;
	asm volatile("1: moveb %1@+,%4@; dbra %0,1b" :
			"=d" (cnt), "=a" (src) :
			"0" (cnt), "1" (src), "a" (addr));
}
static inline void
outsw(void *addr, void *src, int cnt)
{
	cnt--;
	asm volatile("1: movew %1@+,%4@; dbra %0,1b" :
			"=d" (cnt), "=a" (src) :
			"0" (cnt), "1" (src), "a" (addr));
}

#define	NIC_PUT(base, off, val)	outb((base) + (off), (val))
#define	NIC_GET(base, off)	inb((base) + (off))

/* XXX fixed address */
#define NEPTUNE_NIC	((caddr_t)IODEVbase->neptune + ED_NOVELL_NIC_OFFSET)
#define NEPTUNE_ASIC	((caddr_t)IODEVbase->neptune + ED_NOVELL_ASIC_OFFSET)

/*
 * Determine if the device is present.
 */
int
edmatch(parent, cfp, aux)
	struct device *parent;
	struct cfdata *cfp;
	void *aux;
{
	caddr_t nic_addr = NEPTUNE_NIC;
	caddr_t asic_addr = NEPTUNE_ASIC;

	if (strcmp(aux, "ed") || cfp->cf_unit > 0)
		return 0;

	if (badaddr(nic_addr))
	        return 0;
	if (!ed_find_Novell(nic_addr, asic_addr))
		return 0;

	return 1;
}

/*
 * Generic probe routine for testing for the existance of a DS8390.  Must be
 * called after the NIC has just been reset.  This routine works by looking at
 * certain register values that are guaranteed to be initialized a certain way
 * after power-up or reset.  Seems not to currently work on the 83C690.
 *
 * Specifically:
 *
 *	Register			reset bits	set bits
 *	Command Register (CR)		TXP, STA	RD2, STP
 *	Interrupt Status (ISR)				RST
 *	Interrupt Mask (IMR)		All bits
 *	Data Control (DCR)				LAS
 *	Transmit Config. (TCR)		LB1, LB0
 *
 * We only look at the CR and ISR registers, however, because looking at the
 * others would require changing register pages (which would be intrusive if
 * this isn't an 8390).
 *
 * Return 1 if 8390 was found, 0 if not.
 */
int
ed_probe_generic8390(nicbase)
	caddr_t nicbase;
{

	if ((NIC_GET(nicbase, ED_P0_CR) &
	     (ED_CR_RD2 | ED_CR_TXP | ED_CR_STA | ED_CR_STP)) !=
	    (ED_CR_RD2 | ED_CR_STP))
		return (0);
	if ((NIC_GET(nicbase, ED_P0_ISR) & ED_ISR_RST) != ED_ISR_RST)
		return (0);

	return (1);
}

/*
 * Probe and vendor-specific initialization routine for NE1000/2000 boards.
 */
int
ed_find_Novell(nicbase, asicbase)
	caddr_t nicbase, asicbase;
{
	u_char tmp;

	/* XXX - do Novell-specific probe here */

	/* Reset the board. */
#ifdef GWETHER
	outb(asicbase + ED_NOVELL_RESET, 0);
	DELAY(2000);	/* delay(200); */
#endif /* GWETHER */
	tmp = inb(asicbase + ED_NOVELL_RESET);

	/*
	 * I don't know if this is necessary; probably cruft leftover from
	 * Clarkson packet driver code. Doesn't do a thing on the boards I've
	 * tested. -DG [note that a outb(0x84, 0) seems to work here, and is
	 * non-invasive...but some boards don't seem to reset and I don't have
	 * complete documentation on what the 'right' thing to do is...so we do
	 * the invasive thing for now.  Yuck.]
	 */
	outb(asicbase + ED_NOVELL_RESET, tmp);
	DELAY(50000);	/* delay(5000); */

	/*
	 * This is needed because some NE clones apparently don't reset the NIC
	 * properly (or the NIC chip doesn't reset fully on power-up)
	 * XXX - this makes the probe invasive! ...Done against my better
	 * judgement.  -DLG
	 */
	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STP);

	DELAY(50000);	/* delay(5000); */

	/* Make sure that we really have an 8390 based board. */
	if (!ed_probe_generic8390(nicbase))
		return (0);

	return 1;
}

int
ed_check_type(sc)
	struct ed_softc *sc;
{
	u_int memsize, n;
	u_char romdata[16];
	static u_char test_pattern[32] = "THIS is A memory TEST pattern";
	u_char test_buffer[32];
	caddr_t nicbase = sc->nic_base;

	sc->cr_proto = ED_CR_RD2;

	/*
	 * Test the ability to read and write to the NIC memory.  This has the
	 * side affect of determining if this is an NE1000 or an NE2000.
	 */

	/*
	 * This prevents packets from being stored in the NIC memory when the
	 * readmem routine turns on the start bit in the CR.
	 */
	NIC_PUT(nicbase, ED_P0_RCR, ED_RCR_MON);

	/* Temporarily initialize DCR for byte operations. */
	NIC_PUT(nicbase, ED_P0_DCR, ED_DCR_FT1 | ED_DCR_LS);

	NIC_PUT(nicbase, ED_P0_PSTART, 8192 >> ED_PAGE_SHIFT);
	NIC_PUT(nicbase, ED_P0_PSTOP, 16384 >> ED_PAGE_SHIFT);

	sc->isa16bit = 0;

	/*
	 * Write a test pattern in byte mode.  If this fails, then there
	 * probably isn't any memory at 8k - which likely means that the board
	 * is an NE2000.
	 */
	ed_pio_writemem(sc, test_pattern, 8192, sizeof(test_pattern));
	ed_pio_readmem(sc, 8192, test_buffer, sizeof(test_pattern));

	if (bcmp(test_pattern, test_buffer, sizeof(test_pattern))) {
		/* not an NE1000 - try NE2000 */

		NIC_PUT(nicbase, ED_P0_DCR,
		    ED_DCR_WTS | ED_DCR_FT1 | ED_DCR_LS);
		NIC_PUT(nicbase, ED_P0_PSTART, 16384 >> ED_PAGE_SHIFT);
		NIC_PUT(nicbase, ED_P0_PSTOP, 32768 >> ED_PAGE_SHIFT);

		sc->isa16bit = 1;

		/*
		 * Write a test pattern in word mode.  If this also fails, then
		 * we don't know what this board is.
		 */
		ed_pio_writemem(sc, test_pattern, 16384, sizeof(test_pattern));
		ed_pio_readmem(sc, 16384, test_buffer, sizeof(test_pattern));

		if (bcmp(test_pattern, test_buffer, sizeof(test_pattern))) {
			printf(": unknown type\n");
			return (0); /* not an NE2000 either */
		}

		sc->type = ED_TYPE_NE2000;
		sc->type_str = "NE2000 based Neptune-X";
	} else {
		sc->type = ED_TYPE_NE1000;
		sc->type_str = "NE1000";
	}

	/* 8k of memory plus an additional 8k if 16-bit. */
	memsize = 8192 + sc->isa16bit * 8192;

	/* NIC memory doesn't start at zero on an NE board. */
	/* The start address is tied to the bus width. */
	sc->mem_start = (caddr_t)(8192 + sc->isa16bit * 8192);
	sc->tx_page_start = memsize >> ED_PAGE_SHIFT;

#ifdef GWETHER
	{
		int x, i, mstart = 0;
		char pbuf0[ED_PAGE_SIZE], pbuf[ED_PAGE_SIZE], tbuf[ED_PAGE_SIZE];

		for (i = 0; i < ED_PAGE_SIZE; i++)
			pbuf0[i] = 0;

		/* Search for the start of RAM. */
		for (x = 1; x < 256; x++) {
			ed_pio_writemem(sc, pbuf0, x << ED_PAGE_SHIFT, ED_PAGE_SIZE);
			ed_pio_readmem(sc, x << ED_PAGE_SHIFT, tbuf, ED_PAGE_SIZE);
			if (!bcmp(pbuf0, tbuf, ED_PAGE_SIZE)) {
				for (i = 0; i < ED_PAGE_SIZE; i++)
					pbuf[i] = 255 - x;
				ed_pio_writemem(sc, pbuf, x << ED_PAGE_SHIFT, ED_PAGE_SIZE);
				ed_pio_readmem(sc, x << ED_PAGE_SHIFT, tbuf, ED_PAGE_SIZE);
				if (!bcmp(pbuf, tbuf, ED_PAGE_SIZE)) {
					mstart = x << ED_PAGE_SHIFT;
					memsize = ED_PAGE_SIZE;
					break;
				}
			}
		}

		if (mstart == 0) {
			printf("%s: cannot find start of RAM\n",
			    sc->sc_dev.dv_xname);
			return (0);
		}

		/* Search for the end of RAM. */
		for (++x; x < 256; x++) {
			ed_pio_writemem(sc, pbuf0, x << ED_PAGE_SHIFT, ED_PAGE_SIZE);
			ed_pio_readmem(sc, x << ED_PAGE_SHIFT, tbuf, ED_PAGE_SIZE);
			if (!bcmp(pbuf0, tbuf, ED_PAGE_SIZE)) {
				for (i = 0; i < ED_PAGE_SIZE; i++)
					pbuf[i] = 255 - x;
				ed_pio_writemem(sc, pbuf, x << ED_PAGE_SHIFT, ED_PAGE_SIZE);
				ed_pio_readmem(sc, x << ED_PAGE_SHIFT, tbuf, ED_PAGE_SIZE);
				if (!bcmp(pbuf, tbuf, ED_PAGE_SIZE))
					memsize += ED_PAGE_SIZE;
				else
					break;
			} else
				break;
		}

		printf("%s: RAM start %x, size %d\n",
		    sc->sc_dev.dv_xname, mstart, memsize);

		sc->mem_start = (caddr_t)mstart;
		sc->tx_page_start = mstart >> ED_PAGE_SHIFT;
	}
#endif /* GWETHER */

	sc->mem_size = memsize;
	sc->mem_end = sc->mem_start + memsize;

	/*
	 * Use one xmit buffer if < 16k, two buffers otherwise (if not told
	 * otherwise).
	 */
	if ((memsize < 16384) /* || (cf->cf_flags & ED_FLAGS_NO_MULTI_BUFFERING) */)
		sc->txb_cnt = 1;
	else
		sc->txb_cnt = 2;

	sc->rec_page_start = sc->tx_page_start + sc->txb_cnt * ED_TXBUF_SIZE;
	sc->rec_page_stop = sc->tx_page_start + (memsize >> ED_PAGE_SHIFT);

	sc->mem_ring =
	    sc->mem_start + ((sc->txb_cnt * ED_TXBUF_SIZE) << ED_PAGE_SHIFT);

	ed_pio_readmem(sc, 0, romdata, 16);
	for (n = 0; n < ETHER_ADDR_LEN; n++)
		sc->sc_enaddr[n] = romdata[n*(sc->isa16bit+1)];

#ifdef GWETHER
	if (sc->sc_enaddr[2] == 0x86)
		sc->type_str = "Gateway AT";
#endif /* GWETHER */

	/* Clear any pending interrupts that might have occurred above. */
	NIC_PUT(nicbase, ED_P0_ISR, 0xff);

	return (1);
}

/*
 * Install interface into kernel networking data structures.
 */
void
edattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct ed_softc *sc = (void *)self;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;

	sc->nic_base = NEPTUNE_NIC;
	sc->asic_base = NEPTUNE_ASIC;
	if (!ed_check_type(sc))
		return;		/* attach failed */

	/* Set interface to stopped condition (reset). */
	edstop(sc);

	/* Initialize ifnet structure. */
	bcopy(sc->sc_dev.dv_xname, ifp->if_xname, IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = edstart;
	ifp->if_ioctl = edioctl;
	ifp->if_watchdog = edwatchdog;
	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;

	/* Attach the interface. */
	if_attach(ifp);
	ether_ifattach(ifp, sc->sc_enaddr);

	/* Print additional info when attached. */
	printf("\n%s: address %s, ", sc->sc_dev.dv_xname,
	       ether_sprintf(sc->sc_enaddr));

	if (sc->type_str)
		printf("type %s ", sc->type_str);
	else
		printf("type unknown (0x%x) ", sc->type);

	printf("%s", sc->isa16bit ? "(16-bit)" : "(8-bit)");
	printf("\n");

#if NBPFILTER > 0
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif

#if NRND > 0
	rnd_attach_source(&sc->rnd_source, sc->sc_dev.dv_xname,
			  RND_TYPE_NET, 0);
#endif
}

/*
 * Reset interface.
 */
void
edreset(sc)
	struct ed_softc *sc;
{
	int s;

	s = splnet();
	edstop(sc);
	edinit(sc);
	splx(s);
}

/*
 * Take interface offline.
 */
void
edstop(sc)
	struct ed_softc *sc;
{
	caddr_t nicbase = sc->nic_base;
	int n = 5000;

	/* Stop everything on the interface, and select page 0 registers. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STP);

	/*
	 * Wait for interface to enter stopped state, but limit # of checks to
	 * 'n' (about 5ms).  It shouldn't even take 5us on modern DS8390's, but
	 * just in case it's an old one.
	 */
	while (((NIC_GET(nicbase, ED_P0_ISR) & ED_ISR_RST) == 0) && --n);
}

/*
 * Device timeout/watchdog routine.  Entered if the device neglects to generate
 * an interrupt after a transmit has been started on it.
 */
void
edwatchdog(ifp)
	struct ifnet *ifp;
{
	struct ed_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", sc->sc_dev.dv_xname);
	++sc->sc_ethercom.ec_if.if_oerrors;

	edreset(sc);
}

/*
 * Initialize device.
 */
void
edinit(sc)
	struct ed_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	caddr_t nicbase = sc->nic_base;
	int i;
	u_long mcaf[2];

	/*
	 * Initialize the NIC in the exact order outlined in the NS manual.
	 * This init procedure is "mandatory"...don't change what or when
	 * things happen.
	 */

	/* Reset transmitter flags. */
	ifp->if_timer = 0;

	sc->txb_inuse = 0;
	sc->txb_new = 0;
	sc->txb_next_tx = 0;

	/* Set interface for page 0, remote DMA complete, stopped. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STP);

	if (sc->isa16bit) {
		/*
		 * Set FIFO threshold to 8, No auto-init Remote DMA, byte
		 * order=80x86, word-wide DMA xfers,
		 */
		NIC_PUT(nicbase, ED_P0_DCR,
		    ED_DCR_FT1 | ED_DCR_WTS | ED_DCR_LS);
	} else {
		/* Same as above, but byte-wide DMA xfers. */
		NIC_PUT(nicbase, ED_P0_DCR, ED_DCR_FT1 | ED_DCR_LS);
	}

	/* Clear remote byte count registers. */
	NIC_PUT(nicbase, ED_P0_RBCR0, 0);
	NIC_PUT(nicbase, ED_P0_RBCR1, 0);

	/* Tell RCR to do nothing for now. */
	NIC_PUT(nicbase, ED_P0_RCR, ED_RCR_MON);

	/* Place NIC in internal loopback mode. */
	NIC_PUT(nicbase, ED_P0_TCR, ED_TCR_LB0);

	/* Initialize receive buffer ring. */
	NIC_PUT(nicbase, ED_P0_BNRY, sc->rec_page_start);
	NIC_PUT(nicbase, ED_P0_PSTART, sc->rec_page_start);
	NIC_PUT(nicbase, ED_P0_PSTOP, sc->rec_page_stop);

	/*
	 * Clear all interrupts.  A '1' in each bit position clears the
	 * corresponding flag.
	 */
	NIC_PUT(nicbase, ED_P0_ISR, 0xff);

	/*
	 * Enable the following interrupts: receive/transmit complete,
	 * receive/transmit error, and Receiver OverWrite.
	 *
	 * Counter overflow and Remote DMA complete are *not* enabled.
	 */
	NIC_PUT(nicbase, ED_P0_IMR,
	    ED_IMR_PRXE | ED_IMR_PTXE | ED_IMR_RXEE | ED_IMR_TXEE |
	    ED_IMR_OVWE);

	/* Program command register for page 1. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_1 | ED_CR_STP);

	/* Copy out our station address. */
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		NIC_PUT(nicbase, ED_P1_PAR0 + i*2, sc->sc_enaddr[i]);

	/* Set multicast filter on chip. */
	ed_getmcaf(&sc->sc_ethercom, mcaf);
	for (i = 0; i < 8; i++)
		NIC_PUT(nicbase, ED_P1_MAR0 + i*2, ((u_char *)mcaf)[i]);

	/*
	 * Set current page pointer to one page after the boundary pointer, as
	 * recommended in the National manual.
	 */
	sc->next_packet = sc->rec_page_start + 1;
	NIC_PUT(nicbase, ED_P1_CURR, sc->next_packet);

	/* Program command register for page 0. */
	NIC_PUT(nicbase, ED_P1_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STP);

	i = ED_RCR_AB | ED_RCR_AM;
	if (ifp->if_flags & IFF_PROMISC) {
		/*
		 * Set promiscuous mode.  Multicast filter was set earlier so
		 * that we should receive all multicast packets.
		 */
		i |= ED_RCR_PRO | ED_RCR_AR | ED_RCR_SEP;
	}
	NIC_PUT(nicbase, ED_P0_RCR, i);

	/* Take interface out of loopback. */
	NIC_PUT(nicbase, ED_P0_TCR, 0);

	/* Fire up the interface. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set 'running' flag, and clear output active flag. */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/* ...and attempt to start output. */
	edstart(ifp);
}

/*
 * This routine actually starts the transmission on the interface.
 */
static inline void
ed_xmit(sc)
	struct ed_softc *sc;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	caddr_t nicbase = sc->nic_base;
	u_short len;

	len = sc->txb_len[sc->txb_next_tx];

	/* Set NIC for page 0 register access. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	/* Set TX buffer start page. */
	NIC_PUT(nicbase, ED_P0_TPSR, sc->tx_page_start +
	    sc->txb_next_tx * ED_TXBUF_SIZE);

	/* Set TX length. */
	NIC_PUT(nicbase, ED_P0_TBCR0, len);
	NIC_PUT(nicbase, ED_P0_TBCR1, len >> 8);

	/* Set page 0, remote DMA complete, transmit packet, and *start*. */
	NIC_PUT(nicbase, ED_P0_CR,
	    sc->cr_proto | ED_CR_PAGE_0 | ED_CR_TXP | ED_CR_STA);

	/* Point to next transmit buffer slot and wrap if necessary. */
	sc->txb_next_tx++;
	if (sc->txb_next_tx == sc->txb_cnt)
		sc->txb_next_tx = 0;

	/* Set a timer just in case we never hear from the board again. */
	ifp->if_timer = 2;
}

/*
 * Start output on interface.
 * We make two assumptions here:
 *  1) that the current priority is set to splnet _before_ this code
 *     is called *and* is returned to the appropriate priority after
 *     return
 *  2) that the IFF_OACTIVE flag is checked before this code is called
 *     (i.e. that the output part of the interface is idle)
 */
void
edstart(ifp)
	struct ifnet *ifp;
{
	struct ed_softc *sc = ifp->if_softc;
	struct mbuf *m0;
	caddr_t buffer;
	int len;

	if ((ifp->if_flags & (IFF_RUNNING | IFF_OACTIVE)) != IFF_RUNNING)
		return;

outloop:
	/* See if there is room to put another packet in the buffer. */
	if (sc->txb_inuse == sc->txb_cnt) {
		/* No room.  Indicate this to the outside world and exit. */
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}

	IF_DEQUEUE(&ifp->if_snd, m0);
	if (m0 == 0)
		return;

	/* We need to use m->m_pkthdr.len, so require the header */
	if ((m0->m_flags & M_PKTHDR) == 0)
		panic("edstart: no header mbuf");

#if NBPFILTER > 0
	/* Tap off here if there is a BPF listener. */
	if (ifp->if_bpf)
		bpf_mtap(ifp->if_bpf, m0);
#endif

	/* txb_new points to next open buffer slot. */
	buffer = sc->mem_start + ((sc->txb_new * ED_TXBUF_SIZE) << ED_PAGE_SHIFT);

	len = ed_pio_write_mbufs(sc, m0, (long)buffer);

	m_freem(m0);
	sc->txb_len[sc->txb_new] = max(len, ETHER_MIN_LEN);

	/* Start the first packet transmitting. */
	if (sc->txb_inuse == 0)
		ed_xmit(sc);

	/* Point to next buffer slot and wrap if necessary. */
	if (++sc->txb_new == sc->txb_cnt)
		sc->txb_new = 0;
	sc->txb_inuse++;

	/* Loop back to the top to possibly buffer more packets. */
	goto outloop;
}

/*
 * Ethernet interface receiver interrupt.
 */
static inline void
ed_rint(sc)
	struct ed_softc *sc;
{
	caddr_t nicbase = sc->nic_base;
	u_char boundary, current;
	u_short len;
#ifdef DIAGNOSTIC
	u_short count;
#endif
	u_char nlen;
	struct ed_ring packet_hdr;
	caddr_t packet_ptr;

loop:
	/* Set NIC to page 1 registers to get 'current' pointer. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_1 | ED_CR_STA);

	/*
	 * 'sc->next_packet' is the logical beginning of the ring-buffer - i.e.
	 * it points to where new data has been buffered.  The 'CURR' (current)
	 * register points to the logical end of the ring-buffer - i.e. it
	 * points to where additional new data will be added.  We loop here
	 * until the logical beginning equals the logical end (or in other
	 * words, until the ring-buffer is empty).
	 */
	current = NIC_GET(nicbase, ED_P1_CURR);
	if (sc->next_packet == current)
		return;

	/* Set NIC to page 0 registers to update boundary register. */
	NIC_PUT(nicbase, ED_P1_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	do {
		/* Get pointer to this buffer's header structure. */
		packet_ptr = sc->mem_ring +
		    ((sc->next_packet - sc->rec_page_start) << ED_PAGE_SHIFT);

		/*
		 * The byte count includes a 4 byte header that was added by
		 * the NIC.
		 */
		ed_pio_readmem(sc, (long)packet_ptr,
			    (caddr_t) &packet_hdr, sizeof(packet_hdr));
		len = (packet_hdr.count_h << 8) | packet_hdr.count_l;
#ifdef DIAGNOSTIC
		count = len;
#endif

		/*
		 * Try do deal with old, buggy chips that sometimes duplicate
		 * the low byte of the length into the high byte.  We do this
		 * by simply ignoring the high byte of the length and always
		 * recalculating it.
		 *
		 * NOTE: sc->next_packet is pointing at the current packet.
		 */
		if (packet_hdr.next_packet >= sc->next_packet)
			nlen = (packet_hdr.next_packet - sc->next_packet);
		else
			nlen = ((packet_hdr.next_packet - sc->rec_page_start) +
				(sc->rec_page_stop - sc->next_packet));
		--nlen;
		if ((len & ED_PAGE_MASK) + sizeof(packet_hdr) > ED_PAGE_SIZE)
			--nlen;
		len = (len & ED_PAGE_MASK) | (nlen << ED_PAGE_SHIFT);
#ifdef DIAGNOSTIC
		if (len != count) {
			printf("%s: length does not match next packet pointer\n",
			    sc->sc_dev.dv_xname);
			printf("%s: len %04x nlen %04x start %02x first %02x curr %02x next %02x stop %02x\n",
			    sc->sc_dev.dv_xname, count, len,
			    sc->rec_page_start, sc->next_packet, current,
			    packet_hdr.next_packet, sc->rec_page_stop);
		}
#endif

		/*
		 * Be fairly liberal about what we allow as a "reasonable"
		 * length so that a [crufty] packet will make it to BPF (and
		 * can thus be analyzed).  Note that all that is really
		 * important is that we have a length that will fit into one
		 * mbuf cluster or less; the upper layer protocols can then
		 * figure out the length from their own length field(s).
		 */
		if (len <= MCLBYTES &&
		    packet_hdr.next_packet >= sc->rec_page_start &&
		    packet_hdr.next_packet < sc->rec_page_stop) {
			/* Go get packet. */
			edread(sc, packet_ptr + sizeof(struct ed_ring),
			    len - sizeof(struct ed_ring));
		} else {
			/* Really BAD.  The ring pointers are corrupted. */
			log(LOG_ERR,
			    "%s: NIC memory corrupt - invalid packet length %d\n",
			    sc->sc_dev.dv_xname, len);
			++sc->sc_ethercom.ec_if.if_ierrors;
			edreset(sc);
			return;
		}

		/* Update next packet pointer. */
		sc->next_packet = packet_hdr.next_packet;

		/*
		 * Update NIC boundary pointer - being careful to keep it one
		 * buffer behind (as recommended by NS databook).
		 */
		boundary = sc->next_packet - 1;
		if (boundary < sc->rec_page_start)
			boundary = sc->rec_page_stop - 1;
		NIC_PUT(nicbase, ED_P0_BNRY, boundary);
	} while (sc->next_packet != current);

	goto loop;
}

/* Ethernet interface interrupt processor. */
int
edintr(unit)
	int unit;
{
	struct ed_softc *sc = ed_cd.cd_devs[unit];
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	caddr_t nicbase = sc->nic_base;
	u_char isr;

	/* Set NIC to page 0 registers. */
	NIC_PUT(nicbase, ED_P0_CR, sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

	isr = NIC_GET(nicbase, ED_P0_ISR);
	if (!isr)
		return (0);

	/* Loop until there are no more new interrupts. */
	for (;;) {
		/*
		 * Reset all the bits that we are 'acknowledging' by writing a
		 * '1' to each bit position that was set.
		 * (Writing a '1' *clears* the bit.)
		 */
		NIC_PUT(nicbase, ED_P0_ISR, isr);

		/*
		 * Handle transmitter interrupts.  Handle these first because
		 * the receiver will reset the board under some conditions.
		 */
		if (isr & (ED_ISR_PTX | ED_ISR_TXE)) {
			u_char collisions = NIC_GET(nicbase, ED_P0_NCR) & 0x0f;

			/*
			 * Check for transmit error.  If a TX completed with an
			 * error, we end up throwing the packet away.  Really
			 * the only error that is possible is excessive
			 * collisions, and in this case it is best to allow the
			 * automatic mechanisms of TCP to backoff the flow.  Of
			 * course, with UDP we're screwed, but this is expected
			 * when a network is heavily loaded.
			 */
			(void) NIC_GET(nicbase, ED_P0_TSR);
			if (isr & ED_ISR_TXE) {
				/*
				 * Excessive collisions (16).
				 */
				if ((NIC_GET(nicbase, ED_P0_TSR) & ED_TSR_ABT)
				    && (collisions == 0)) {
					/*
					 * When collisions total 16, the P0_NCR
					 * will indicate 0, and the TSR_ABT is
					 * set.
					 */
					collisions = 16;
				}

				/* Update output errors counter. */
				++ifp->if_oerrors;
			} else {
				/*
				 * Update total number of successfully
				 * transmitted packets.
				 */
				++ifp->if_opackets;
			}

			/* Done with the buffer. */
			sc->txb_inuse--;

			/* Clear watchdog timer. */
			ifp->if_timer = 0;
			ifp->if_flags &= ~IFF_OACTIVE;

			/*
			 * Add in total number of collisions on last
			 * transmission.
			 */
			ifp->if_collisions += collisions;

			/*
			 * Decrement buffer in-use count if not zero (can only
			 * be zero if a transmitter interrupt occured while not
			 * actually transmitting).
			 * If data is ready to transmit, start it transmitting,
			 * otherwise defer until after handling receiver.
			 */
			if (sc->txb_inuse > 0)
				ed_xmit(sc);
		}

		/* Handle receiver interrupts. */
		if (isr & (ED_ISR_PRX | ED_ISR_RXE | ED_ISR_OVW)) {
			/*
			 * Overwrite warning.  In order to make sure that a
			 * lockup of the local DMA hasn't occurred, we reset
			 * and re-init the NIC.  The NSC manual suggests only a
			 * partial reset/re-init is necessary - but some chips
			 * seem to want more.  The DMA lockup has been seen
			 * only with early rev chips - Methinks this bug was
			 * fixed in later revs.  -DG
			 */
			if (isr & ED_ISR_OVW) {
				++ifp->if_ierrors;
#ifdef DIAGNOSTIC
				log(LOG_WARNING,
				    "%s: warning - receiver ring buffer overrun\n",
				    sc->sc_dev.dv_xname);
#endif
				/* Stop/reset/re-init NIC. */
				edreset(sc);
			} else {
				/*
				 * Receiver Error.  One or more of: CRC error,
				 * frame alignment error FIFO overrun, or
				 * missed packet.
				 */
				if (isr & ED_ISR_RXE) {
					++ifp->if_ierrors;
#ifdef ED_DEBUG
					printf("%s: receive error %x\n",
					    sc->sc_dev.dv_xname,
					    NIC_GET(nicbase, ED_P0_RSR));
#endif
				}

				/*
				 * Go get the packet(s).
				 * XXX - Doing this on an error is dubious
				 * because there shouldn't be any data to get
				 * (we've configured the interface to not
				 * accept packets with errors).
				 */

				ed_rint(sc);
			}
		}

		/*
		 * If it looks like the transmitter can take more data,	attempt
		 * to start output on the interface.  This is done after
		 * handling the receiver to give the receiver priority.
		 */
		edstart(ifp);

		/*
		 * Return NIC CR to standard state: page 0, remote DMA
		 * complete, start (toggling the TXP bit off, even if was just
		 * set in the transmit routine, is *okay* - it is 'edge'
		 * triggered from low to high).
		 */
		NIC_PUT(nicbase, ED_P0_CR,
		    sc->cr_proto | ED_CR_PAGE_0 | ED_CR_STA);

		/*
		 * If the Network Talley Counters overflow, read them to reset
		 * them.  It appears that old 8390's won't clear the ISR flag
		 * otherwise - resulting in an infinite loop.
		 */
		if (isr & ED_ISR_CNT) {
			(void) NIC_GET(nicbase, ED_P0_CNTR0);
			(void) NIC_GET(nicbase, ED_P0_CNTR1);
			(void) NIC_GET(nicbase, ED_P0_CNTR2);
		}

#if NRND > 0
		rnd_add_uint32(&sc->rnd_source, isr);
#endif

		isr = NIC_GET(nicbase, ED_P0_ISR);
		if (!isr)
			return (1);
	}
}

/*
 * Process an ioctl request.  This code needs some work - it looks pretty ugly.
 */
int
edioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long cmd;
	caddr_t data;
{
	struct ed_softc *sc = ifp->if_softc;
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (cmd) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			edinit(sc);
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
				    sizeof(sc->sc_enaddr));
			/* Set new address. */
			edinit(sc);
			break;
		    }
#endif
		default:
			edinit(sc);
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
			edstop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			edinit(sc);
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			edstop(sc);
			edinit(sc);
		}
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		/* Update our multicast list. */
		error = (cmd == SIOCADDMULTI) ?
		    ether_addmulti(ifr, &sc->sc_ethercom) :
		    ether_delmulti(ifr, &sc->sc_ethercom);

		if (error == ENETRESET) {
			/*
			 * Multicast list has changed; set the hardware filter
			 * accordingly.
			 */
			edstop(sc); /* XXX for ds_setmcaf? */
			edinit(sc);
			error = 0;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);
	return (error);
}

/*
 * Retreive packet from shared memory and send to the next level up via
 * ether_input().  If there is a BPF listener, give a copy to BPF, too.
 */
void
edread(sc, buf, len)
	struct ed_softc *sc;
	caddr_t buf;
	int len;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *m;
	struct ether_header *eh;

	/* Pull packet off interface. */
	m = edget(sc, buf, len);
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
 * Supporting routines.
 */

/*
 * Given a NIC memory source address and a host memory destination address,
 * copy 'amount' from NIC to host using Programmed I/O.  The 'amount' is
 * rounded up to a word - okay as long as mbufs are word sized.
 * This routine is currently Novell-specific.
 */
void
ed_pio_readmem(sc, src, dst, amount)
	struct ed_softc *sc;
	u_short src;
	caddr_t dst;
	u_short amount;
{
	caddr_t nicbase = sc->nic_base;

	/* Select page 0 registers. */
	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Round up to a word. */
	if (amount & 1)
		++amount;

	/* Set up DMA byte count. */
	NIC_PUT(nicbase, ED_P0_RBCR0, amount);
	NIC_PUT(nicbase, ED_P0_RBCR1, amount >> 8);

	/* Set up source address in NIC mem. */
	NIC_PUT(nicbase, ED_P0_RSAR0, src);
	NIC_PUT(nicbase, ED_P0_RSAR1, src >> 8);

	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD0 | ED_CR_PAGE_0 | ED_CR_STA);

	if (sc->isa16bit)
		insw(sc->asic_base + ED_NOVELL_DATA, dst, amount / 2);
	else
		insb(sc->asic_base + ED_NOVELL_DATA, dst, amount);
}

/*
 * Stripped down routine for writing a linear buffer to NIC memory.  Only used
 * in the probe routine to test the memory.  'len' must be even.
 */
void
ed_pio_writemem(sc, src, dst, len)
	struct ed_softc *sc;
	caddr_t src;
	u_short dst;
	u_short len;
{
	caddr_t nicbase = sc->nic_base;
	int maxwait = 100; /* about 120us */

	/* Select page 0 registers. */
	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Reset remote DMA complete flag. */
	NIC_PUT(nicbase, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(nicbase, ED_P0_RBCR0, len);
	NIC_PUT(nicbase, ED_P0_RBCR1, len >> 8);

	/* Set up destination address in NIC mem. */
	NIC_PUT(nicbase, ED_P0_RSAR0, dst);
	NIC_PUT(nicbase, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);

	if (sc->isa16bit)
		outsw(sc->asic_base + ED_NOVELL_DATA, src, len / 2);
	else
		outsb(sc->asic_base + ED_NOVELL_DATA, src, len);

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes.  Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the ISA bus.
	 */
	while (((NIC_GET(nicbase, ED_P0_ISR) & ED_ISR_RDC) != ED_ISR_RDC) &&
	    --maxwait);
}

/*
 * Write an mbuf chain to the destination NIC memory address using programmed
 * I/O.
 */
u_short
ed_pio_write_mbufs(sc, m, dst)
	struct ed_softc *sc;
	struct mbuf *m;
	u_short dst;
{
	caddr_t nicbase = sc->nic_base, asicbase = sc->asic_base;
	u_short len;
	int maxwait = 100; /* about 120us */

	len = m->m_pkthdr.len;

	/* Select page 0 registers. */
	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD2 | ED_CR_PAGE_0 | ED_CR_STA);

	/* Reset remote DMA complete flag. */
	NIC_PUT(nicbase, ED_P0_ISR, ED_ISR_RDC);

	/* Set up DMA byte count. */
	NIC_PUT(nicbase, ED_P0_RBCR0, len);
	NIC_PUT(nicbase, ED_P0_RBCR1, len >> 8);

	/* Set up destination address in NIC mem. */
	NIC_PUT(nicbase, ED_P0_RSAR0, dst);
	NIC_PUT(nicbase, ED_P0_RSAR1, dst >> 8);

	/* Set remote DMA write. */
	NIC_PUT(nicbase, ED_P0_CR, ED_CR_RD1 | ED_CR_PAGE_0 | ED_CR_STA);

	/*
	 * Transfer the mbuf chain to the NIC memory.
	 * 16-bit cards require that data be transferred as words, and only
	 * words, so that case requires some extra code to patch over
	 * odd-length mbufs.
	 */
	if (!sc->isa16bit) {
		/* NE1000s are easy. */
		for (; m != 0; m = m->m_next) {
			if (m->m_len) {
				outsb(asicbase + ED_NOVELL_DATA,
				    mtod(m, u_char *), m->m_len);
			}
		}
	} else {
		/* NE2000s are a bit trickier. */
		u_int8_t *data, savebyte[2];
		int len, wantbyte;

		wantbyte = 0;
		for (; m != 0; m = m->m_next) {
			len = m->m_len;
			if (len == 0)
				continue;
			data = mtod(m, u_int8_t *);
			/* Finish the last word. */
			if (wantbyte) {
				savebyte[1] = *data;
				outw(asicbase + ED_NOVELL_DATA,
				    *(u_short *)savebyte);
				data++;
				len--;
				wantbyte = 0;
			}
			/* Output contiguous words. */
			if (len > 1)
				outsw(asicbase + ED_NOVELL_DATA,
				    data, len >> 1);
			/* Save last byte, if necessary. */
			if (len & 1) {
				data += len & ~1;
				savebyte[0] = *data;
				wantbyte = 1;
			}
		}

		if (wantbyte) {
			savebyte[1] = 0;
			outw(asicbase + ED_NOVELL_DATA, *(u_short *)savebyte);
		}
	}

	/*
	 * Wait for remote DMA complete.  This is necessary because on the
	 * transmit side, data is handled internally by the NIC in bursts and
	 * we can't start another remote DMA until this one completes. 	Not
	 * waiting causes really bad things to happen - like the NIC
	 * irrecoverably jamming the ISA bus.
	 */
	while (((NIC_GET(nicbase, ED_P0_ISR) & ED_ISR_RDC) != ED_ISR_RDC) &&
	    --maxwait);

	if (!maxwait) {
		log(LOG_WARNING,
		    "%s: remote transmit DMA failed to complete\n",
		    sc->sc_dev.dv_xname);
		edreset(sc);
	}

	return (len);
}

/*
 * Given a source and destination address, copy 'amount' of a packet from the
 * ring buffer into a linear destination buffer.  Takes into account ring-wrap.
 */
static inline caddr_t
ed_ring_copy(sc, src, dst, amount)
	struct ed_softc *sc;
	caddr_t src, dst;
	u_short	amount;
{
	u_short	tmp_amount;

	/* Does copy wrap to lower addr in ring buffer? */
	if (src + amount > sc->mem_end) {
		tmp_amount = sc->mem_end - src;

		/* Copy amount up to end of NIC memory. */
		ed_pio_readmem(sc, (long)src, dst, tmp_amount);

		amount -= tmp_amount;
		src = sc->mem_ring;
		dst += tmp_amount;
	}

	ed_pio_readmem(sc, (long)src, dst, amount);

	return (src + amount);
}

/*
 * Copy data from receive buffer to end of mbuf chain allocate additional mbufs
 * as needed.  Return pointer to last mbuf in chain.
 * sc = ed info (softc)
 * src = pointer in ed ring buffer
 * dst = pointer to last mbuf in mbuf chain to copy to
 * amount = amount of data to copy
 */
struct mbuf *
edget(sc, src, total_len)
	struct ed_softc *sc;
	caddr_t src;
	u_short total_len;
{
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	struct mbuf *top, **mp, *m;
	int len;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		return 0;
	m->m_pkthdr.rcvif = ifp;
	m->m_pkthdr.len = total_len;
	len = MHLEN;
	top = 0;
	mp = &top;

	while (total_len > 0) {
		if (top) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				m_freem(top);
				return 0;
			}
			len = MLEN;
		}
		if (total_len >= MINCLSIZE) {
			MCLGET(m, M_DONTWAIT);
			if ((m->m_flags & M_EXT) == 0) {
				m_free(m);
				m_freem(top);
				return 0;
			}
			len = MCLBYTES;
		}
		m->m_len = len = min(total_len, len);
		src = ed_ring_copy(sc, src, mtod(m, caddr_t), len);
		total_len -= len;
		*mp = m;
		mp = &m->m_next;
	}

	return top;
}

/*
 * Compute the multicast address filter from the list of multicast addresses we
 * need to listen to.
 */
void
ed_getmcaf(ec, af)
	struct ethercom *ec;
	u_long *af;
{
	struct ifnet *ifp = &ec->ec_if;
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
	ETHER_FIRST_MULTI(step, ec, enm);
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
		af[crc >> 5] |= 1 << ((crc & 0x1f) ^ 0);

		ETHER_NEXT_MULTI(step, enm);
	}
	ifp->if_flags &= ~IFF_ALLMULTI;
}
