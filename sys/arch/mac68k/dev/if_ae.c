/*	$NetBSD: if_ae.c,v 1.18 1995/03/23 13:00:05 briggs Exp $	*/

/*
 * Device driver for National Semiconductor DS8390 based ethernet adapters.
 *
 * Based on original ISA bus driver by David Greenman, 29-April-1993
 *
 * Copyright (C) 1993, David Greenman. This software may be used, modified,
 *   copied, distributed, and sold, in both source and binary form provided
 *   that the above copyright and these terms are retained. Under no
 *   circumstances is the author responsible for the proper functioning
 *   of this software, nor does the author assume any responsibility
 *   for damages incurred with its use.
 *
 * Adapted for MacBSD by Brad Parker <brad@fcr.com>
 *
 * Currently supports:
 *	Apples NB Ethernet card
 *	Interlan A310 Nubus Ethernet card
 *	Cayman Systems GatorCard
 *	Asante MacCon II/E
 */

/*
 * $Id: if_ae.c,v 1.18 1995/03/23 13:00:05 briggs Exp $
 */
 
#include "ae.h"
/* bpfilter included here in case it is needed in future net includes */
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>

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

#include <sys/device.h>
#include "../mac68k/via.h"
#include "nubus.h"
#include "if_aereg.h"

struct ae_device {
	struct device	ae_dev;
/*	struct nubusdev	ae_nu;
	struct intrhand	ae_ih;	*/
};

/*
 * ae_softc: per line info and status
 */
struct	ae_softc {
	struct ae_device	*sc_ae;

	struct	arpcom arpcom;	/* ethernet common */

	char	*type_str;	/* pointer to type string */
	u_char	vendor;		/* interface vendor */
	u_char	type;		/* interface type code */
	u_char	regs_rev;	/* registers are reversed */

#define	REG_MAP(sc, reg)	((sc)->regs_rev ? (0x0f-(reg))<<2 : (reg)<<2)
#define NIC_GET(sc, reg)	((sc)->nic_addr[REG_MAP(sc, reg)])
#define NIC_PUT(sc, reg, val)	((sc)->nic_addr[REG_MAP(sc, reg)] = (val))
	volatile caddr_t nic_addr; /* NIC (DS8390) I/O bus address */
	caddr_t	rom_addr;	/* on board prom address */
	caddr_t	smem_start;	/* shared memory start address */
	caddr_t	smem_end;	/* shared memory end address */
	u_long	smem_size;	/* total shared memory size */
	u_char	smem_wr_short;	/* card memory requires int16 writes */
	caddr_t	smem_ring;	/* start of RX ring-buffer (in smem) */

	caddr_t	bpf;		/* BPF "magic cookie" */

	u_char	xmit_busy;	/* transmitter is busy */
	u_char	txb_cnt;	/* Number of transmit buffers */
	u_char	txb_next;	/* Pointer to next buffer ready to xmit */
	u_short	txb_next_len;	/* next xmit buffer length */
	u_char	data_buffered;	/* data has been buffered in interface mem */
	u_char	tx_page_start;	/* first page of TX buffer area */

	u_char	rec_page_start;	/* first page of RX ring-buffer */
	u_char	rec_page_stop;	/* last page of RX ring-buffer */
	u_char	next_packet;	/* pointer to next unread RX packet */
} ae_softc[NAE];

void	ae_find(), ae_attach();
int	ae_init(), ae_ioctl(), ae_probe(),
	ae_start(), ae_reset(), ae_watchdog(), aeintr();

struct cfdriver aecd =
{ NULL, "ae", ae_probe, ae_attach, DV_IFNET, sizeof(struct ae_device), NULL, 0 };

static void ae_stop();
static inline void ae_rint();
static inline void ae_xmit();
static inline char *ae_ring_copy();

extern int ether_output();

#define	ETHER_MIN_LEN	64
#define ETHER_MAX_LEN	1518
#define	ETHER_ADDR_LEN	6
#define	ETHER_HDR_SIZE	14

char ae_name[] = "8390 Nubus Ethernet card";
static char zero = 0;
static u_char ones = 0xff;

struct vendor_S {
	char	*manu;
	int	len;
	int	vendor;
} vend[] = {
	{ "Apple", 5, AE_VENDOR_APPLE },
	{ "3Com",  4, AE_VENDOR_APPLE },
	{ "Dayna", 5, AE_VENDOR_DAYNA },
	{ "Inter", 5, AE_VENDOR_INTERLAN },
	{ "Asant", 5, AE_VENDOR_ASANTE },
};

static int numvend = sizeof(vend)/sizeof(vend[0]);

/*
 * XXX These two should be moved to locore, and maybe changed to use shorts
 * instead of bytes.  The reason for these is that bcopy and bzero use longs,
 * which the ethernet cards can't handle.
 */

void
bszero (u_short *addr, int len)
{
	while (len--) {
		*addr++ = 0;
	}
}

void
bbcopy (char *src, char *dest, int len)
{
	while (len--) {
		*dest++ = *src++;
	}
}

/*
	short copy; assume destination is always aligned
	and last byte of odd length copy is not important
*/

void
bscopy (char *src, char *dest, int len)
{
	u_short *d = (u_short *)dest;
	u_short *s = (u_short *)src;
	char b1, b2;

	/* odd case, src addr is unaligned */
	if ( ((u_long)src) & 1 ) {
		while (len > 0) {
			b1 = *src++;
			b2 = len > 1 ? *src++ : (*d & 0xff);
			*d++ = (b1 << 8) | b2;
			len -= 2;
		}
		return;
	}

	/* normal case, aligned src & dst */
	while (len > 0) {
		*d++ = *s++;
		len -= 2;
	}
}

void
ae_id_card(nu, sc)
	struct nubus_hw	*nu;
	struct ae_softc	*sc;
{
	int	i;

	/*
	 * Try to determine what type of card this is...
	 */
	sc->vendor = AE_VENDOR_UNKNOWN;
	for (i=0 ; i<numvend ; i++) {
		if (!strncmp(nu->Slot.manufacturer, vend[i].manu, vend[i].len)) 
		{
			sc->vendor = vend[i].vendor;
			break;
		}
	}
	sc->type_str = (char *) (nu->Slot.manufacturer);

}

int
ae_size_card_memory(sc)
	struct ae_softc	*sc;
{
	u_short *p;
	u_short i1, i2, i3, i4;
	int size;
	
	p = (u_short *)sc->smem_start;

	/*
	 * very simple size memory, assuming it's installed in 8k
	 * banks; also assume it will generally mirror in upper banks
	 * if not installed.
	 */
	i1 = (8192*0)/2;
	i2 = (8192*1)/2;
	i3 = (8192*2)/2;
	i4 = (8192*3)/2;
	
	p[i1] = 0x1111;
	p[i2] = 0x2222;
	p[i3] = 0x3333;
	p[i4] = 0x4444;
	
	size = 0;
	if (p[i1] == 0x1111 && p[i2] == 0x2222 &&
	    p[i3] == 0x3333 && p[i4] == 0x4444)
		size = 8192*4;
	else
		if ((p[i1] == 0x1111 && p[i2] == 0x2222) ||
		    (p[i1] == 0x3333 && p[i2] == 0x4444))
			size = 8192*2;
		else
			if (p[i1] == 0x1111 || p[i1] == 0x4444)
				size = 8192;

	if (size == 0)
	  return 0;

	return size;
}

int
ae_probe(parent, cf, aux)
	struct cfdriver	*parent;
	struct cfdata	*cf;
	void		*aux;
{
	register struct nubus_hw *nu = (struct nubus_hw *) aux;
	struct ae_softc *sc = &ae_softc[cf->cf_unit];
	int i, memsize;
	int flags = 0;

	if (nu->Slot.type != NUBUS_NETWORK)
		return 0;

	ae_id_card(nu, sc);

	sc->regs_rev = 0;
	sc->smem_wr_short = 0;

	switch (sc->vendor) {
	      case AE_VENDOR_INTERLAN:
		sc->nic_addr = nu->addr + GC_NIC_OFFSET;
		sc->rom_addr = nu->addr + GC_ROM_OFFSET;
		sc->smem_start = nu->addr + GC_DATA_OFFSET;
		if ((memsize = ae_size_card_memory(sc)) == 0)
			return 0;

		/* reset the NIC chip */
		*((caddr_t)nu->addr + GC_RESET_OFFSET) = (char)zero;
	
		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->arpcom.ac_enaddr[i] = *(sc->rom_addr + i*4);
		break;

	      case AE_VENDOR_ASANTE:
		/* memory writes require *(u_short *) */
		sc->smem_wr_short = 1;
		/* otherwise, pretend to be an apple card (fall through) */

	      case AE_VENDOR_APPLE:
		sc->regs_rev = 1;
		sc->nic_addr = nu->addr + AE_NIC_OFFSET;
		sc->rom_addr = nu->addr + AE_ROM_OFFSET;
		sc->smem_start = nu->addr + AE_DATA_OFFSET;
		if ((memsize = ae_size_card_memory(sc)) == 0)
			return 0;

		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->arpcom.ac_enaddr[i] = *(sc->rom_addr + i*2);
		break;

	      case AE_VENDOR_DAYNA:
		printf("We think we are a Dayna card, but ");
		sc->nic_addr = nu->addr + DP_NIC_OFFSET;
		sc->rom_addr = nu->addr + DP_ROM_OFFSET;
		sc->smem_start = nu->addr + DP_DATA_OFFSET;
		memsize = 8192;

		/* Get station address from on-board ROM */
		for (i = 0; i < ETHER_ADDR_LEN; ++i)
			sc->arpcom.ac_enaddr[i] = *(sc->rom_addr + i*2);
		printf("it is dangerous to continue.\n");
		return 0; /* Since we don't work yet... */
		break;

	      default:
		return 0;
		break;
	}

	/*
	 * allocate one xmit buffer if < 16k, two buffers otherwise
	 */
	if ((memsize < 16384) || (flags & AE_FLAGS_NO_DOUBLE_BUFFERING)) {
		sc->smem_ring =
		  sc->smem_start + (AE_PAGE_SIZE * AE_TXBUF_SIZE);
		sc->txb_cnt = 1;
		sc->rec_page_start = AE_TXBUF_SIZE;
	} else {
		sc->smem_ring =
		  sc->smem_start + (AE_PAGE_SIZE * AE_TXBUF_SIZE * 2);
		sc->txb_cnt = 2;
		sc->rec_page_start = AE_TXBUF_SIZE * 2;
	}

	sc->smem_size = memsize;
	sc->smem_end = sc->smem_start + memsize;
	sc->rec_page_stop = memsize / AE_PAGE_SIZE;
	sc->tx_page_start = 0;

	/*
	 * Now zero memory and verify that it is clear
	 */
	bszero((u_short *)sc->smem_start, memsize / 2);

	for (i = 0; i < memsize; ++i)
		if (sc->smem_start[i]) {
	        	printf("ae: failed to clear shared memory at %x\n",
			       sc->smem_start + i);

			return(0);
		}

#ifdef DEBUG_PRINT
	printf("nic_addr %x, rom_addr %x\n",
		sc->nic_addr, sc->rom_addr);
	printf("smem_size %d\n", sc->smem_size);
	printf("smem_start %x, smem_ring %x, smem_end %x\n",
		sc->smem_start, sc->smem_ring, sc->smem_end);
	printf("phys address %02x:%02x:%02x:%02x:%02x:%02x\n",
		sc->arpcom.ac_enaddr[0],
		sc->arpcom.ac_enaddr[1],
		sc->arpcom.ac_enaddr[2],
		sc->arpcom.ac_enaddr[3],
		sc->arpcom.ac_enaddr[4],
		sc->arpcom.ac_enaddr[5]);
#endif

	return(1);
}
 
/*
 * Install interface into kernel networking data structures
 */
void
ae_attach(parent, self, aux)
	struct cfdriver	*parent, *self;
	void		*aux;
{
	struct nubus_hw	*nu = aux;
	struct ae_device *ae = (struct ae_device *) self;
	struct ae_softc *sc = &ae_softc[ae->ae_dev.dv_unit];
	struct cfdata *cf = ae->ae_dev.dv_cfdata;
	struct ifnet *ifp = &sc->arpcom.ac_if;
	struct ifaddr *ifa;
	struct sockaddr_dl *sdl;
 
	sc->sc_ae = ae;

	/*
	 * Set interface to stopped condition (reset)
	 */
	ae_stop(sc);

	/*
	 * Initialize ifnet structure
	 */
	ifp->if_unit = ae->ae_dev.dv_unit;
	ifp->if_name = aecd.cd_name;
	ifp->if_mtu = ETHERMTU;
	ifp->if_output = ether_output;
	ifp->if_start = ae_start;
	ifp->if_ioctl = ae_ioctl;
	ifp->if_reset = ae_reset;
	ifp->if_watchdog = ae_watchdog;
	ifp->if_flags = (IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS);

#if 0
	/*
	 * Set default state for ALTPHYS flag (used to disable the transceiver
	 * for AUI operation), based on compile-time config option.
	 */
	if (cf->cf_flags & AE_FLAGS_DISABLE_TRANSCEIVER)
		ifp->if_flags |= IFF_ALTPHYS;
#endif

	/*
	 * Attach the interface
	 */
	if_attach(ifp);

	/*
	 * Search down the ifa address list looking for the AF_LINK type entry
	 */
 	ifa = ifp->if_addrlist;
	while ((ifa != 0) && (ifa->ifa_addr != 0) &&
	    (ifa->ifa_addr->sa_family != AF_LINK))
		ifa = ifa->ifa_next;
	/*
	 * If we find an AF_LINK type entry we fill in the hardware address.
	 *	This is useful for netstat(1) to keep track of which interface
	 *	is which.
	 */
	if ((ifa != 0) && (ifa->ifa_addr != 0)) {
		/*
		 * Fill in the link-level address for this interface
		 */
		sdl = (struct sockaddr_dl *)ifa->ifa_addr;
		sdl->sdl_type = IFT_ETHER;
		sdl->sdl_alen = ETHER_ADDR_LEN;
		sdl->sdl_slen = 0;
		bbcopy(sc->arpcom.ac_enaddr, LLADDR(sdl), ETHER_ADDR_LEN);
	}

	/*
	 * Print additional info when attached
	 */
	printf(": address %s, ", ether_sprintf(sc->arpcom.ac_enaddr));

	if (sc->type_str && (*sc->type_str != 0))
		printf("type %s", sc->type_str);
	else
		printf("type unknown (0x%x)", sc->type);

	printf(", %dk mem", sc->smem_size / 1024);

	printf("\n");

	/*
	 * If BPF is in the kernel, call the attach for it
	 */
#if NBPFILTER > 0
	bpfattach(&sc->bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
#endif
}

/*
 * Reset interface.
 */
int
ae_reset(sc)
	struct ae_softc *sc;
{
	int s;

	s = splnet();

	/*
	 * Stop interface and re-initialize.
	 */
	ae_stop(sc);
	ae_init(sc);

	(void) splx(s);
}
 
/*
 * Take interface offline.
 */
void
ae_stop(sc)
	struct ae_softc *sc;
{
	int n = 5000;

	/*
	 * Stop everything on the interface, and select page 0 registers.
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_STP);

	/*
	 * Wait for interface to enter stopped state, but limit # of checks
	 *	to 'n' (about 5ms). It shouldn't even take 5us on modern
	 *	DS8390's, but just in case it's an old one.
	 */
	while (((NIC_GET(sc, AE_P0_ISR) & AE_ISR_RST) == 0) && --n);
}

/*
 * Device timeout/watchdog routine. Entered if the device neglects to
 *	generate an interrupt after a transmit has been started on it.
 */
static	int aeintr_ctr=0;
int
ae_watchdog(unit)
	short unit;
{
	struct ae_softc *sc = &ae_softc[unit];

#if 1
/*
 * This is a kludge!  The via code seems to miss slot interrupts
 * sometimes.  This kludges around that by calling the handler
 * by hand if the watchdog is activated. -- XXX (akb)
 */
	int	i;

	i = aeintr_ctr;

	(*via2itab[1])(1);

	if (i != aeintr_ctr)
		return;
#endif

	log(LOG_ERR, "ae%d: device timeout\n", unit);
	ae_reset(sc);
}

/*
 * Initialize device. 
 */
ae_init(sc)
	struct ae_softc *sc;
{
	struct ifnet *ifp = &sc->arpcom.ac_if;
	int i, s;
	u_char	command;

	/* address not known */
	if (ifp->if_addrlist == (struct ifaddr *)0) return;

	/*
	 * Initialize the NIC in the exact order outlined in the NS manual.
	 *	This init procedure is "mandatory"...don't change what or when
	 *	things happen.
	 */
	s = splnet();

	/* reset transmitter flags */
	sc->data_buffered = 0;
	sc->xmit_busy = 0;
	sc->arpcom.ac_if.if_timer = 0;

	sc->txb_next = 0;

	/* This variable is used below - don't move this assignment */
	sc->next_packet = sc->rec_page_start + 1;

#ifdef DEBUG_PRINT
	printf("page_start %d, page_stop %d, next %d\n", 
		sc->rec_page_start, sc->rec_page_stop, sc->next_packet);
#endif

	/*
	 * Set interface for page 0, Remote DMA complete, Stopped
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_STP);

	/*
	 * Set FIFO threshold to 4, No auto-init Remote DMA, Burst mode,
	 *	byte order=80x86, word-wide DMA xfers,
	 */
	NIC_PUT(sc, AE_P0_DCR, AE_DCR_FT1|AE_DCR_BMS|AE_DCR_WTS);

	/*
	 * Clear Remote Byte Count Registers
	 */
	NIC_PUT(sc, AE_P0_RBCR0, zero);
	NIC_PUT(sc, AE_P0_RBCR1, zero);

	/*
	 * Enable reception of broadcast packets
	 */
	NIC_PUT(sc, AE_P0_RCR, AE_RCR_AB);

	/*
	 * Place NIC in internal loopback mode
	 */
	NIC_PUT(sc, AE_P0_TCR, AE_TCR_LB0);

	/*
	 * Initialize transmit/receive (ring-buffer) Page Start
	 */
	NIC_PUT(sc, AE_P0_TPSR, sc->tx_page_start);
	NIC_PUT(sc, AE_P0_PSTART, sc->rec_page_start);

	/*
	 * Initialize Receiver (ring-buffer) Page Stop and Boundry
	 */
	NIC_PUT(sc, AE_P0_PSTOP, sc->rec_page_stop);
	NIC_PUT(sc, AE_P0_BNRY, sc->rec_page_start);

	/*
	 * Clear all interrupts. A '1' in each bit position clears the
	 *	corresponding flag.
	 */
	NIC_PUT(sc, AE_P0_ISR, ones);

	/* make sure interrupts are vectored to us */
	add_nubus_intr((int)sc->rom_addr & 0xFF000000, aeintr, sc - ae_softc);

	/*
	 * Enable the following interrupts: receive/transmit complete,
	 *	receive/transmit error, and Receiver OverWrite.
	 *
	 * Counter overflow and Remote DMA complete are *not* enabled.
	 */
	NIC_PUT(sc, AE_P0_IMR,
		AE_IMR_PRXE|AE_IMR_PTXE|AE_IMR_RXEE|AE_IMR_TXEE|AE_IMR_OVWE);

	/*
	 * Program Command Register for page 1
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_PAGE_1|AE_CR_RD2|AE_CR_STP);

	/*
	 * Copy out our station address
	 */
	for (i = 0; i < ETHER_ADDR_LEN; ++i)
		NIC_PUT(sc, AE_P1_PAR0 + i, sc->arpcom.ac_enaddr[i]);

#if NBPFILTER > 0
	/*
	 * Initialize multicast address hashing registers to accept
	 *	 all multicasts (only used when in promiscuous mode)
	 */
	for (i = 0; i < 8; ++i)
		NIC_PUT(sc, AE_P1_MAR0 + i, 0xff);
#endif

	/*
	 * Set Current Page pointer to next_packet (initialized above)
	 */
	NIC_PUT(sc, AE_P1_CURR, sc->next_packet);

	/*
	 * Set Command Register for page 0, Remote DMA complete,
	 * 	and interface Start.
	 */
	NIC_PUT(sc, AE_P1_CR, AE_CR_RD2|AE_CR_STA);

	/*
	 * Take interface out of loopback
	 */
	NIC_PUT(sc, AE_P0_TCR, zero);

	/*
	 * Set 'running' flag, and clear output active flag.
	 */
	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	/*
	 * ...and attempt to start output
	 */
	ae_start(ifp);

	(void) splx(s);
}
 
/*
 * This routine actually starts the transmission on the interface
 */
static inline void ae_xmit(ifp)
	struct ifnet *ifp;
{
	struct ae_softc *sc = &ae_softc[ifp->if_unit];
	u_short len = sc->txb_next_len;

	/*
	 * Set NIC for page 0 register access
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_STA);

	/*
	 * Set TX buffer start page
	 */
	NIC_PUT(sc, AE_P0_TPSR, sc->tx_page_start +
		sc->txb_next * AE_TXBUF_SIZE);

	/*
	 * Set TX length
	 */
	NIC_PUT(sc, AE_P0_TBCR0, len & 0xff);
	NIC_PUT(sc, AE_P0_TBCR1, len >> 8);

	/*
	 * Set page 0, Remote DMA complete, Transmit Packet, and *Start*
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_TXP|AE_CR_STA);

	sc->xmit_busy = 1;
	sc->data_buffered = 0;
	
	/*
	 * Switch buffers if we are doing double-buffered transmits
	 */
	if ((sc->txb_next == 0) && (sc->txb_cnt > 1)) 
		sc->txb_next = 1;
	else
		sc->txb_next = 0;

	/*
	 * Set a timer just in case we never hear from the board again
	 */
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
int
ae_start(ifp)
	struct ifnet *ifp;
{
	struct ae_softc *sc = &ae_softc[ifp->if_unit];
	struct mbuf *m0, *m;
	caddr_t buffer;
	int len;

outloop:
	/*
	 * See if there is room to send more data (i.e. one or both of the
	 *	buffers is empty).
	 */
	if (sc->data_buffered)
		if (sc->xmit_busy) {
			/*
			 * No room. Indicate this to the outside world
			 *	and exit.
			 */
			ifp->if_flags |= IFF_OACTIVE;
			return;
		} else {
			/*
			 * Data is buffered, but we're not transmitting, so
			 *	start the xmit on the buffered data.
			 * Note that ae_xmit() resets the data_buffered flag
			 *	before returning.
			 */
			ae_xmit(ifp);
		}

	IF_DEQUEUE(&sc->arpcom.ac_if.if_snd, m);
	if (m == 0) {
	/*
	 * The following isn't pretty; we are using the !OACTIVE flag to
	 * indicate to the outside world that we can accept an additional
	 * packet rather than that the transmitter is _actually_
	 * active. Indeed, the transmitter may be active, but if we haven't
	 * filled the secondary buffer with data then we still want to
	 * accept more.
	 * Note that it isn't necessary to test the data_buffered flag -
	 * we wouldn't have tried to de-queue the packet in the first place
	 * if it was set.
	 */
		ifp->if_flags &= ~IFF_OACTIVE;
		return;
	}

	/*
	 * Copy the mbuf chain into the transmit buffer
	 */
	buffer = sc->smem_start + (sc->txb_next * AE_TXBUF_SIZE * AE_PAGE_SIZE);
	len = 0;
	for (m0 = m; m != 0; m = m->m_next) {
		/*printf("ae: copy %d bytes @ %x\n", m->m_len, buffer);*/
		bscopy(mtod(m, caddr_t), buffer, m->m_len); 
		buffer += m->m_len;
       		len += m->m_len;
	}
	if (len & 1) len++;

	sc->txb_next_len = max(len, ETHER_MIN_LEN);

	if (sc->txb_cnt > 1)
		/*
		 * only set 'buffered' flag if doing multiple buffers
		 */
		sc->data_buffered = 1;

	if (sc->xmit_busy == 0)
		ae_xmit(ifp);
	/*
	 * If there is BPF support in the configuration, tap off here.
	 *   The following has support for converting trailer packets
	 *   back to normal.
	 */
#if NBPFILTER > 0
	if (sc->bpf) {
		u_short etype;
		int off, datasize, resid;
		struct ether_header *eh;
		struct trailer_header {
			u_short ether_type;
			u_short ether_residual;
		} trailer_header;
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

			bpf_tap(sc->bpf, ether_packet, ep - ether_packet);
		} else
			bpf_mtap(sc->bpf, m0);
	}
#endif

	m_freem(m0);

	/*
	 * If we are doing double-buffering, a buffer might be free to
	 *	fill with another packet, so loop back to the top.
	 */
	if (sc->txb_cnt > 1)
		goto outloop;
	else {
		ifp->if_flags |= IFF_OACTIVE;
		return;
	}
}
 
/*
 * Ethernet interface receiver interrupt.
 */
static inline void
ae_rint(unit)
	int unit;
{
	register struct ae_softc *sc = &ae_softc[unit];
	u_char boundry, current;
	u_short len;
	struct ae_ring *packet_ptr;

	/*
	 * Set NIC to page 1 registers to get 'current' pointer
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_PAGE_1|AE_CR_RD2|AE_CR_STA);

	/*
	 * 'sc->next_packet' is the logical beginning of the ring-buffer - i.e.
	 *	it points to where new data has been buffered. The 'CURR'
	 *	(current) register points to the logical end of the ring-buffer
	 *	- i.e. it points to where additional new data will be added.
	 *	We loop here until the logical beginning equals the logical
	 *	end (or in other words, until the ring-buffer is empty).
	 */
	while (sc->next_packet != NIC_GET(sc, AE_P1_CURR)) {

		/* get pointer to this buffer header structure */
		packet_ptr = (struct ae_ring *)(sc->smem_ring +
			 (sc->next_packet - sc->rec_page_start) * AE_PAGE_SIZE);

		/*
		 * The byte count includes the FCS - Frame Check Sequence (a
		 *	32 bit CRC).
		 */
		len = packet_ptr->count[0] | (packet_ptr->count[1] << 8);
		if ((len >= ETHER_MIN_LEN) && (len <= ETHER_MAX_LEN)) {
			/*
			 * Go get packet. len - 4 removes CRC from length.
			 * (packet_ptr + 1) points to data just after the
			 * packet ring header (+4 bytes)
			 */
			ae_get_packet(sc, (caddr_t)(packet_ptr + 1), len - 4);
			++sc->arpcom.ac_if.if_ipackets;
		} else {
			/*
			 * Really BAD...probably indicates that the ring
			 * pointers are corrupted. Also seen on early rev
			 * chips under high load - the byte order of the
			 * length gets switched.
			 */
			log(LOG_ERR,
				"ae%d: shared memory corrupt - invalid packet length %d\n",
				unit, len);
			ae_reset(sc);
			return;
		}

		/*
		 * Update next packet pointer
		 */
		sc->next_packet = packet_ptr->next_packet;

		/*
		 * Update NIC boundry pointer - being careful to keep it
		 *	one buffer behind. (as recommended by NS databook)
		 */
		boundry = sc->next_packet - 1;
		if (boundry < sc->rec_page_start)
			boundry = sc->rec_page_stop - 1;

		/*
		 * Set NIC to page 0 registers to update boundry register
		 */
		NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_STA);

		NIC_PUT(sc, AE_P0_BNRY, boundry);

		/*
		 * Set NIC to page 1 registers before looping to top
		 * (prepare to get 'CURR' current pointer)
		 */
		NIC_PUT(sc, AE_P0_CR, AE_CR_PAGE_1|AE_CR_RD2|AE_CR_STA);
	}
}

/*
 * Ethernet interface interrupt processor
 */
int
aeintr(unit)
	int unit;
{
	struct ae_softc *sc = &ae_softc[unit];
	u_char isr;

	aeintr_ctr++;
	/*
	 * Set NIC to page 0 registers
	 */
	NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_STA);

	/*
	 * loop until there are no more new interrupts
	 */
	while (isr = NIC_GET(sc, AE_P0_ISR)) {

		/*
		 * reset all the bits that we are 'acknowledging'
		 *	by writing a '1' to each bit position that was set
		 * (writing a '1' *clears* the bit)
		 */
		NIC_PUT(sc, AE_P0_ISR, isr);

		/*
		 * Handle transmitter interrupts. Handle these first
		 *	because the receiver will reset the board under
		 *	some conditions.
		 */
		if (isr & (AE_ISR_PTX|AE_ISR_TXE)) {
			u_char collisions = NIC_GET(sc, AE_P0_NCR);

			/*
			 * Check for transmit error. If a TX completed with an
			 * error, we end up throwing the packet away. Really
			 * the only error that is possible is excessive
			 * collisions, and in this case it is best to allow the
			 * automatic mechanisms of TCP to backoff the flow. Of
			 * course, with UDP we're screwed, but this is expected
			 * when a network is heavily loaded.
			 */
			if (isr & AE_ISR_TXE) {

				/*
				 * Excessive collisions (16)
				 */
				if ((NIC_GET(sc, AE_P0_TSR) & AE_TSR_ABT)
					&& (collisions == 0)) {
					/*
					 *    When collisions total 16, the
					 * P0_NCR will indicate 0, and the
					 * TSR_ABT is set.
					 */
					collisions = 16;
				}

				/*
				 * update output errors counter
				 */
				++sc->arpcom.ac_if.if_oerrors;
			} else {
				/*
				 * Update total number of successfully
				 * 	transmitted packets.
				 */
				++sc->arpcom.ac_if.if_opackets;
			}

			/*
			 * reset tx busy and output active flags
			 */
			sc->xmit_busy = 0;
			sc->arpcom.ac_if.if_flags &= ~IFF_OACTIVE;

			/*
			 * clear watchdog timer
			 */
			sc->arpcom.ac_if.if_timer = 0;

			/*
			 * Add in total number of collisions on last
			 *	transmission.
			 */
			sc->arpcom.ac_if.if_collisions += collisions;

			/*
			 * If data is ready to transmit, start it transmitting,
			 *	otherwise defer until after handling receiver
			 */
			if (sc->data_buffered)
				ae_xmit(&sc->arpcom.ac_if);
		}

		/*
		 * Handle receiver interrupts
		 */
		if (isr & (AE_ISR_PRX|AE_ISR_RXE|AE_ISR_OVW)) {
		    /*
		     * Overwrite warning. In order to make sure that a lockup
		     *	of the local DMA hasn't occurred, we reset and
		     *	re-init the NIC. The NSC manual suggests only a
		     *	partial reset/re-init is necessary - but some
		     *	chips seem to want more. The DMA lockup has been
		     *	seen only with early rev chips - Methinks this
		     *	bug was fixed in later revs. -DG
		     */
			if (isr & AE_ISR_OVW) {
				++sc->arpcom.ac_if.if_ierrors;
				log(LOG_WARNING,
					"ae%d: warning - receiver ring buffer overrun\n",
					unit);
				/*
				 * Stop/reset/re-init NIC
				 */
				ae_reset(sc);
			} else {

			    /*
			     * Receiver Error. One or more of: CRC error, frame
			     *	alignment error FIFO overrun, or missed packet.
			     */
				if (isr & AE_ISR_RXE) {
					++sc->arpcom.ac_if.if_ierrors;
#ifdef AE_DEBUG
					printf("ae%d: receive error %x\n", unit,
						NIC_GET(sc, AE_P0_RSR));
#endif
				}

				/*
				 * Go get the packet(s)
				 * XXX - Doing this on an error is dubious
				 *    because there shouldn't be any data to
				 *    get (we've configured the interface to
				 *    not accept packets with errors).
				 */
				ae_rint (unit);
			}
		}

		/*
		 * If it looks like the transmitter can take more data,
		 * 	attempt to start output on the interface.
		 *	This is done after handling the receiver to
		 *	give the receiver priority.
		 */
		if ((sc->arpcom.ac_if.if_flags & IFF_OACTIVE) == 0)
			ae_start(&sc->arpcom.ac_if);

		/*
		 * return NIC CR to standard state: page 0, remote DMA complete,
		 * 	start (toggling the TXP bit off, even if was just set
		 *	in the transmit routine, is *okay* - it is 'edge'
		 *	triggered from low to high)
		 */
		NIC_PUT(sc, AE_P0_CR, AE_CR_RD2|AE_CR_STA);

		/*
		 * If the Network Talley Counters overflow, read them to
		 *	reset them. It appears that old 8390's won't
		 *	clear the ISR flag otherwise - resulting in an
		 *	infinite loop.
		 */
		if (isr & AE_ISR_CNT) {
			(void) NIC_GET(sc, AE_P0_CNTR0);
			(void) NIC_GET(sc, AE_P0_CNTR1);
			(void) NIC_GET(sc, AE_P0_CNTR2);
		}
	}
	return 0;
}
 
/*
 * Process an ioctl request. This code needs some work - it looks
 *	pretty ugly.
 */
int
ae_ioctl(ifp, command, data)
	register struct ifnet *ifp;
	int command;
	caddr_t data;
{
	register struct ifaddr *ifa = (struct ifaddr *)data;
	struct ae_softc *sc = &ae_softc[ifp->if_unit];
	struct ifreq *ifr = (struct ifreq *)data;
	int s, error = 0;

	s = splnet();

	switch (command) {

	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP;

		switch (ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			ae_init(sc);	/* before arpwhohas */
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
					*(union ns_host *)(sc->arpcom.ac_enaddr);
			else {
				/* 
				 * 
				 */
				bbcopy((caddr_t)ina->x_host.c_host,
				    (caddr_t)sc->arpcom.ac_enaddr,
					sizeof(sc->arpcom.ac_enaddr));
			}
			/*
			 * Set new address
			 */
			ae_init(sc);
			break;
		    }
#endif
		default:
			ae_init(sc);
			break;
		}
		break;

	case SIOCSIFFLAGS:
		/*
		 * If interface is marked down and it is running, then stop it
		 */
		if (((ifp->if_flags & IFF_UP) == 0) &&
		    (ifp->if_flags & IFF_RUNNING)) {
			ae_stop(sc);
			ifp->if_flags &= ~IFF_RUNNING;
		} else {
		/*
		 * If interface is marked up and it is stopped, then start it
		 */
			if ((ifp->if_flags & IFF_UP) &&
		    	    ((ifp->if_flags & IFF_RUNNING) == 0))
				ae_init(sc);
		}
#if NBPFILTER > 0
		if (ifp->if_flags & IFF_PROMISC) {
			/*
			 * Set promiscuous mode on interface.
			 *	XXX - for multicasts to work, we would need to
			 *		write 1's in all bits of multicast
			 *		hashing array. For now we assume that
			 *		this was done in ae_init().
			 */
			NIC_PUT(sc, AE_P0_RCR,
				AE_RCR_PRO|AE_RCR_AM|AE_RCR_AB);
		} else {
			/*
			 * XXX - for multicasts to work, we would need to
			 *	rewrite the multicast hashing array with the
			 *	proper hash (would have been destroyed above).
			 */
			NIC_PUT(sc, AE_P0_RCR, AE_RCR_AB);
		}
#endif
		break;

	default:
		error = EINVAL;
	}
	(void) splx(s);
	return (error);
}
 
/*
 * Macro to calculate a new address within shared memory when given an offset
 *	from an address, taking into account ring-wrap.
 */
#define	ringoffset(sc, start, off, type) \
	((type)( ((caddr_t)(start)+(off) >= (sc)->smem_end) ? \
		(((caddr_t)(start)+(off))) - (sc)->smem_end \
		+ (sc)->smem_ring: \
		((caddr_t)(start)+(off)) ))

/*
 * Retreive packet from shared memory and send to the next level up via
 *	ether_input(). If there is a BPF listener, give a copy to BPF, too.
 */
ae_get_packet(sc, buf, len)
	struct ae_softc *sc;
	char *buf;
	u_short len;
{
	struct ether_header *eh;
    	struct mbuf *m, *head, *ae_ring_to_mbuf();
	u_short off;
	int resid;
	u_short etype;
	struct trailer_header {
		u_short	trail_type;
		u_short trail_residual;
	} trailer_header;

	/* Allocate a header mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == 0)
		goto bad;
	m->m_pkthdr.rcvif = &sc->arpcom.ac_if;
	m->m_pkthdr.len = len;
	m->m_len = 0;
	head = m;

	eh = (struct ether_header *)buf;

	/* The following sillines is to make NFS happy */
#define EROUND	((sizeof(struct ether_header) + 3) & ~3)
#define EOFF	(EROUND - sizeof(struct ether_header))

	/*
	 * The following assumes there is room for
	 * the ether header in the header mbuf
	 */
	head->m_data += EOFF;
	bbcopy(buf, mtod(head, caddr_t), sizeof(struct ether_header));
	buf += sizeof(struct ether_header);
	head->m_len += sizeof(struct ether_header);
	len -= sizeof(struct ether_header);

	etype = ntohs((u_short)eh->ether_type);

	/*
	 * Deal with trailer protocol:
	 * If trailer protocol, calculate the datasize as 'off',
	 * which is also the offset to the trailer header.
	 * Set resid to the amount of packet data following the
	 * trailer header.
	 * Finally, copy residual data into mbuf chain.
	 */
	if (etype >= ETHERTYPE_TRAIL &&
	    etype < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER) {

		off = (etype - ETHERTYPE_TRAIL) << 9;
		if ((off + sizeof(struct trailer_header)) > len)
			goto bad;	/* insanity */

		eh->ether_type = *ringoffset(sc, buf, off, u_short *);
		resid = ntohs(*ringoffset(sc, buf, off+2, u_short *));

		if ((off + resid) > len) goto bad;	/* insanity */

		resid -= sizeof(struct trailer_header);
		if (resid < 0) goto bad;	/* insanity */

		m = ae_ring_to_mbuf(sc, ringoffset(sc, buf, off+4, char *),
				    head, resid);
		if (m == 0) goto bad;

		len = off;
		head->m_pkthdr.len -= 4; /* subtract trailer header */
	}

	/*
	 * Pull packet off interface. Or if this was a trailer packet,
	 * the data portion is appended.
	 */
	m = ae_ring_to_mbuf(sc, buf, m, len);
	if (m == 0) goto bad;

#if NBPFILTER > 0
	/*
	 * Check if there's a BPF listener on this interface.
	 * If so, hand off the raw packet to bpf. 
	 */
	if (sc->bpf) {
		bpf_mtap(sc->bpf, head);

		/*
		 * Note that the interface cannot be in promiscuous mode if
		 * there are no BPF listeners.  And if we are in promiscuous
		 * mode, we have to check if this packet is really ours.
		 *
		 * XXX This test does not support multicasts.
		 */
		if ((sc->arpcom.ac_if.if_flags & IFF_PROMISC) &&
			bcmp(eh->ether_dhost, sc->arpcom.ac_enaddr,
				sizeof(eh->ether_dhost)) != 0 &&
			bcmp(eh->ether_dhost, etherbroadcastaddr,
				sizeof(eh->ether_dhost)) != 0) {

			m_freem(head);
			return;
		}
	}
#endif

	/*
	 * Fix up data start offset in mbuf to point past ether header
	 */
	m_adj(head, sizeof(struct ether_header));

	ether_input(&sc->arpcom.ac_if, eh, head);
	return;

bad:	if (head)
		m_freem(head);
	return;
}

/*
 * Supporting routines
 */

/*
 * Given a source and destination address, copy 'amount' of a packet from
 *	the ring buffer into a linear destination buffer. Takes into account
 *	ring-wrap.
 */
static inline char *
ae_ring_copy(sc,src,dst,amount)
	struct ae_softc *sc;
	char	*src;
	char	*dst;
	u_short	amount;
{
	u_short	tmp_amount;

	/* does copy wrap to lower addr in ring buffer? */
	if (src + amount > sc->smem_end) {
		tmp_amount = sc->smem_end - src;
		/* copy amount up to end of smem */
		bbcopy(src, dst, tmp_amount);
		amount -= tmp_amount;
		src = sc->smem_ring;
		dst += tmp_amount;
	}

	bbcopy(src, dst, amount);

	return(src + amount);
}

/*
 * Copy data from receive buffer to end of mbuf chain
 * allocate additional mbufs as needed. return pointer
 * to last mbuf in chain.
 * sc = ed info (softc)
 * src = pointer in ed ring buffer
 * dst = pointer to last mbuf in mbuf chain to copy to
 * amount = amount of data to copy
 */
struct mbuf *
ae_ring_to_mbuf(sc,src,dst,total_len)
	struct ae_softc *sc;
	char *src;
	struct mbuf *dst;
	u_short total_len;
{
	register struct mbuf *m = dst;

	while (total_len) {
		register u_short amount = min(total_len, M_TRAILINGSPACE(m));

		if (amount == 0) {
		  /* no more data in this mbuf, alloc another */
			/*
			 * If there is enough data for an mbuf cluster, attempt
			 * 	to allocate one of those, otherwise, a regular
			 *	mbuf will do.
			 * Note that a regular mbuf is always required, even if
			 *	we get a cluster - getting a cluster does not
			 *	allocate any mbufs, and one is needed to assign
			 *	the cluster to. The mbuf that has a cluster
			 *	extension can not be used to contain data -
			 *	only the cluster can contain data.
			 */ 
			dst = m;
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == 0)
				return (0);

			if (total_len >= MINCLSIZE)
				MCLGET(m, M_DONTWAIT);

			m->m_len = 0;
			dst->m_next = m;
			amount = min(total_len, M_TRAILINGSPACE(m));
		}

		src = ae_ring_copy(sc, src, mtod(m, caddr_t) + m->m_len,
				   amount);

		m->m_len += amount;
		total_len -= amount;

	}
	return (m);
}
