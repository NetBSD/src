/* $NetBSD: if_ie.c,v 1.39.6.1 2018/07/26 23:55:28 snj Exp $ */

/*
 * Copyright (c) 1995 Melvin Tang-Richardson.
 * All rights reserved.
 *
 * This driver is a major hash up of src/sys/dev/isa/if_ie.c and
 * src/sys/arch/acorn32/podulebus/kgdb_ie.c  Please refer to copyright
 * notices from them too.
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
 *	This product includes software developed by RiscBSD.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RISCBSD ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL RISCBSD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * if_ie.c
 *
 * Ether 1 podule driver
 *
 * Created      : 26/06/95
 */

/*
 *	This driver is at its last beta release.  It should not cause
 *	any problems (Touch wood)
 *
 * 	If it passes field tests again.  This will constitute the realse
 *	version.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_ie.c,v 1.39.6.1 2018/07/26 23:55:28 snj Exp $");

#define IGNORE_ETHER1_IDROM_CHECKSUM

/* Standard podule includes */

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/param.h>

#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/conf.h>
#include <sys/malloc.h>
#include <sys/device.h>
#include <machine/io.h>
#include <machine/intr.h>
#include <acorn32/podulebus/podulebus.h>
#include <dev/podulebus/podules.h>

/* Include for interface to the net and ethernet subsystems */

#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/ioctl.h>
#include <sys/mbuf.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_dl.h>
#include <net/if_ether.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

/* Import our data structres */

#include "if_iereg.h"

/* BPF support */

#include <net/bpf.h>

/* Some useful defines and macros */

#define PODULE_IRQ_PENDING (1)
#define NFRAMES	(16)		/* number of frame to allow for receive */
#define NRXBUF	(48)		/* number of receive buffers to allocate */
#define IE_RXBUF_SIZE (256) 	/* receive buf size */
#define NTXBUF  (2)		/* number of transmit buffers to allocate */
#define IE_TXBUF_SIZE (1522) 	/* size of tx buffer */

#define PWriteShort(a,b)	WriteWord(a,(b)<<16|(b))

#define	xoffsetof(type, member)	(offsetof(type, member) << 1)

/* Some data structres local to this file */

struct ie_softc {
	device_t	sc_dev;
	int 		sc_podule_number;
	podule_t	*sc_podule;
	irqhandler_t 	sc_ih;
	int		sc_flags;
#define IE_BROKEN	1
	int		sc_iobase;
	int		sc_fastbase;
	int		sc_rom;
	int		sc_ram;
	int		sc_control;
	struct ethercom	sc_ethercom;
	int		promisc;
	int		sc_irqmode;

	u_long	rframes[NFRAMES];
	u_long	rbuffs[NRXBUF];
	u_long	cbuffs[NRXBUF];
	int 	rfhead, rftail, rbhead, rbtail;

	u_long 	xmit_cmds[NTXBUF];
	u_long 	xmit_buffs[NTXBUF];
	u_long 	xmit_cbuffs[NTXBUF];
	int	xmit_count;
	int	xmit_free;
	int	xchead;
	int 	xctail;
};

/* Function and data prototypes */

static void host2ie( struct ie_softc *sc, void *src, u_long dest, int size );
static void ie2host( struct ie_softc *sc, u_long src, void *dest, int size );
static void iezero( struct ie_softc *sc, u_long p, int size );
void        iereset( struct ie_softc *sc );
void        iewatchdog( struct ifnet *ifp );
int         ieioctl( struct ifnet *ifp, u_long cmd, void *data );
void        iestart( struct ifnet *ifp );
int 	    iestop( struct ie_softc *sc );
int         ieinit( struct ie_softc *sc );
int 	    ieintr( void *arg );
void 	    ietint( struct ie_softc *sc );

/* A whopper of a function */
static int command_and_wait( struct ie_softc *sc, u_short cmd,
			      struct ie_sys_ctl_block *pscb,
			      void *pcmd, int ocmd, int scmd, int mask );

int ieprobe(device_t, cfdata_t, void *);
void ieattach(device_t, device_t, void *);

static inline void ie_cli(struct ie_softc *);
static inline void ieattn(struct ie_softc *);
static inline void setpage(struct ie_softc *, u_long);
static void ie_ack(struct ie_softc *, u_short);
void PWriteShorts(char *, char *, int);
void ReadShorts(char *, char *, int);
static void run_tdr(struct ie_softc *);
u_long setup_rfa(struct ie_softc *, u_long);
static inline int ie_buflen(struct ie_softc *, int);
static inline int ie_packet_len(struct ie_softc *);
struct mbuf *ieget(struct ie_softc *, int *);
void ie_drop_packet_buffer(struct ie_softc *);
void ie_read_frame(struct ie_softc *, int num);
void ierint(struct ie_softc *);
void iexmit(struct ie_softc *);
static void start_receiver(struct ie_softc *);


/*
 * Our cfattach structure for the autoconfig system to chew on
 */

CFATTACH_DECL_NEW(ie, sizeof(struct ie_softc),
    ieprobe, ieattach, NULL, NULL);

/* Let's go! */

/*
 * Clear all pending interrupts from the i82586 chip
 */

static inline void
ie_cli(struct ie_softc *sc)
{
	WriteByte(sc->sc_fastbase + (IE_CONTROL<<2), IE_CONT_CLI);
}

/* 
 * Wake the i82586 chip up and get it to do something
 */

static inline void
ieattn(struct ie_softc *sc)
{
	WriteByte ( sc->sc_control + (IE_CONTROL<<2), IE_CONT_ATTN );
}

/*
 * Set the podule page register to bring a given address into view
 */

static inline void
setpage(struct ie_softc *sc, u_long off)
{
	WriteByte ( sc->sc_control + (IE_PAGE<<2), IE_COFF2PAGE(off) );
}

/*
 * Ack the i82586
 */

static void
ie_ack(struct ie_softc *sc, u_short mask)
{
	u_short stat;
	int i;
	setpage(sc, IE_IBASE + IE_SCB_OFF );

	stat = ReadShort ( sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		(xoffsetof(struct ie_sys_ctl_block, ie_status)) );

	PWriteShort ( sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		(xoffsetof(struct ie_sys_ctl_block, ie_command)),
		stat & mask );

	ieattn(sc);

	for ( i=4000; --i>=0; ) {
		if ( !ReadShort(sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		    (xoffsetof(struct ie_sys_ctl_block, ie_command))) )
			break;
		delay(100);
	}

	if ( i<=0 )
		printf ( "ie: command timed out\n" );
	ie_cli(sc);
}

/*
 * This routine does the checksumming for the idrom
 */

#ifndef IGNORE_ETHER1_IDROM_CHECKSUM
static u_long
crc32(u_char *p, int l)
{
	u_long crc=-1;
	int i, b;
	while ( --l >= 0 ) {
		b = *p++;
		for ( i=8; --i >= 0; b>>=1 )
			if ((b&1)^(crc>>31))
				crc=(crc<<1)^0x4c11db7;
			else
				crc<<=1;
	}
	return crc;
}
#endif

/*
 * Probe for the ether1 card.  return 1 on success 0 on failure
 */

int
ieprobe(device_t parent, cfdata_t cf, void *aux)
{
	struct podule_attach_args *pa = aux;

/* Look for a network slot interface */

	return (pa->pa_product == PODULE_ETHER1);
}

/*
 * Attach our driver to the interfaces it uses
 */
  
void
ieattach(device_t parent, device_t self, void *aux)
{
	struct ie_softc *sc = device_private(self);
	struct podule_attach_args *pa = aux;
	struct ifnet *ifp = &sc->sc_ethercom.ec_if;
	int i;
	char idrom[32];
	uint8_t hwaddr[ETHER_ADDR_LEN];

	/* Check a few things about the attach args */

	if (pa->pa_podule_number == -1)
		panic("Podule has disappeared !");

	sc->sc_dev = self;
	sc->sc_podule_number = pa->pa_podule_number;
	sc->sc_podule = pa->pa_podule;
	podules[sc->sc_podule_number].attached = 1;

	/*
	 * MESS MESS MESS
	 *
	 * This needs a serious clean up. Alot of this code was in the probe function
	 * but required the softc structure. As a temporary measure until I rewrite it
	 * I have just bolted in the probe code here.
	 */

	/* Index some podule areas */
	sc->sc_iobase   = sc->sc_podule->sync_base;	/* OBSOLETE */
	sc->sc_fastbase = sc->sc_podule->fast_base;	/* OBSOLETE */
	sc->sc_rom      = sc->sc_podule->sync_base;
	sc->sc_control  = sc->sc_podule->fast_base;
	sc->sc_ram      = sc->sc_podule->fast_base + IE_MEMOFF;

	/* Set the page mask to something know and neutral */
	setpage(sc, IE_SCB_OFF);

	/* Fetch the first part of the idrom */
	for ( i=0; i<16; i++ )
		idrom[i] = ReadByte ( sc->sc_rom + (i<<2) );

	/* Verify the podulebus probe incase RiscOS lied */
        if ( ReadByte ( sc->sc_rom + (3<<2) ) != 0x03 ) {
		printf(": Ether1 ROM probablly broken.  ECID corrupt\n");
		sc->sc_flags |= IE_BROKEN;
		return;
	}

	/* Reset the 82586 */
	WriteByte ( sc->sc_fastbase + (IE_CONTROL<<2), IE_CONT_RESET );
 	delay(1000);
	WriteByte ( sc->sc_fastbase + (IE_CONTROL<<2), 0 );
	delay(10000);

	/* Clear pending interrupts */
	ie_cli (sc);

	/* Setup SCP */
	{
		struct ie_sys_conf_ptr scp;
		bzero (&scp, sizeof(scp) );
		scp.ie_iscp_ptr = (void *)IE_ISCP_ADDR;
		host2ie(sc, &scp, IE_SCP_ADDR, sizeof (scp) );
	}

	/* Setup ISCP */
	{
		struct ie_int_sys_conf_ptr iscp;
		bzero ( &iscp, sizeof(iscp) );
		iscp.ie_busy = 1;
		iscp.ie_base = (void *)IE_IBASE;
		iscp.ie_scb_offset = IE_SCB_OFF;
		host2ie(sc, &iscp, IE_ISCP_ADDR, sizeof(iscp) );
	}

	/* Initialise the control block */
	iezero ( sc, IE_IBASE + IE_SCB_OFF, sizeof(struct ie_sys_ctl_block) );
	ieattn(sc);

	/* Wait for not busy */
	setpage ( sc, IE_ISCP_ADDR );
	for ( i=10000; --i>=0; ) {
		if ( !ReadShort( sc->sc_ram + IE_COFF2POFF(IE_ISCP_ADDR) +
		    ( xoffsetof(struct ie_int_sys_conf_ptr, ie_busy)) ) )
			break;
		delay (10);
	}

	/* If the busy didn't go low, the i82586 is broken or too slow */
        if ( i<=0 ) {
		printf ( ": ether1 chipset didn't respond\n" );
		sc->sc_flags |= IE_BROKEN;
		return;
	}

	/* Ensure that the podule sends interrupts */
        for ( i=1000; --i>=0 ; ) {
		if ( ReadByte(sc->sc_rom + 0) & PODULE_IRQ_PENDING )
			break;
		delay (10);
	}

	/* If we didn't see the interrupt then the IRQ line is broken */
	if ( i<=0 ) {
		printf ( ": interrupt from chipset didn't reach host\n" );
		sc->sc_flags |= IE_BROKEN;
		return;
	}

	/* Ack our little test operation */
	ie_ack(sc,IE_ST_WHENCE);
        ie_cli (sc);

	/* Get second part of idrom */
	for ( i=16; i<32; i++ )
		idrom[i] = ReadByte ( sc->sc_rom + (i<<2) );

	/* This checksum always fails.  For some reason the first 16 */
	/* bytes are duplicated in the second 16 bytes, the checksum */
	/* should be at location 28 it is clearly not		     */

	/* It is possible that this ether1 card is buggered	     */

#ifndef IGNORE_ETHER1_IDROM_CHECKSUM
	if ( crc32(idrom,28) != *(u_long *)(idrom+28) )
	{
	    printf ( "ie: ether1 idrom failed checksum %08x!=%08x\n",
					crc32(idrom,28), *(u_long *)(idrom+28));
            for ( i=0; i<32; i+=8 ) {
	        printf ( "IDROM: %02x %02x %02x %02x %02x %02x %02x %02x\n",
		    idrom[0+i], idrom[1+i], idrom[2+i], idrom[3+i], 
		    idrom[4+i], idrom[5+i], idrom[6+i], idrom[7+i] );
	    }
	    printf ( "ie: I'll ignore this fact for now!\n" );
	}
#endif

	/* Get our ethernet address.  Do explicit copy */
	for ( i=0; i<ETHER_ADDR_LEN; i++ )
	    hwaddr[i] = idrom[9+i];

	/* Fill in my application form to attach to the inet system */

	memcpy(ifp->if_xname, device_xname(sc->sc_dev), IFNAMSIZ);
	ifp->if_softc = sc;
	ifp->if_start = iestart;
	ifp->if_ioctl = ieioctl;
	ifp->if_watchdog = iewatchdog;
	ifp->if_flags = IFF_BROADCAST | IFF_NOTRAILERS;
	
	/* Signed, dated then sent */
        if_attach (ifp);
	if_deferred_start_init(ifp, NULL);
	ether_ifattach(ifp, hwaddr);

	/* "Hmm," said nuts, "what if the attach fails" */
    
	/* Write some pretty things on the annoucement line */
	printf ( ": %s using %dk card ram",
	    ether_sprintf(hwaddr),
	    ((NRXBUF*IE_RXBUF_SIZE)+(NTXBUF*IE_TXBUF_SIZE))/1024 );

	sc->sc_ih.ih_func = ieintr;
	sc->sc_ih.ih_arg = sc;
	sc->sc_ih.ih_level = IPL_NET;
	sc->sc_ih.ih_name = "net: ie";
	sc->sc_ih.ih_maskaddr = sc->sc_podule->irq_addr;
	sc->sc_ih.ih_maskbits = sc->sc_podule->irq_mask;

	if (irq_claim(sc->sc_podule->interrupt, &sc->sc_ih)) {
		sc->sc_irqmode = 0;
		printf(" POLLED");
		panic("%s: Cannot install IRQ handler", device_xname(sc->sc_dev));
	} else {
		sc->sc_irqmode = 1;
		printf(" IRQ");
	}

	printf("\n");
}
  
                  
/*
 * Oh no!! Where's my shorts!!! I'm sure I put them on this morning
 */

void
PWriteShorts(char *src, char *dest, int cnt)
{
	for (cnt /= 2; --cnt >= 0; ) {
		PWriteShort(dest, *(u_short *)src);
		src+=2;
		dest+=4;
	}
}

void
ReadShorts(char *src, char *dest, int cnt)
{
	for (cnt /= 2; --cnt >= 0; ) {
		*(u_short *)dest = ReadShort(src);
		src+=4;
		dest+=2;
	}
}

/*
 * A bcopy or memcpy to adapter ram.  It handles the page register for you
 * so you dont have to worry about the ram windowing
 */

static void
host2ie(struct ie_softc *sc, void *src, u_long dest, int size)
{
	int cnt;
	char *sptr = src;

#ifdef DIAGNOSTIC
	if (size & 1)
		panic("host2ie");
#endif

	while (size > 0) {
		cnt = IE_PAGESIZE - dest % IE_PAGESIZE;
		if (cnt > size)
			cnt = size;
		setpage(sc, dest);
		PWriteShorts(sptr, (char *)sc->sc_ram + IE_COFF2POFF(dest), cnt);
		sptr+=cnt;
		dest+=cnt;
		size-=cnt;
	}
}

static void
ie2host(struct ie_softc *sc, u_long src, void *dest, int size)
{
	int cnt;
	char *dptr = dest;

#ifdef DIAGNOSTIC
	if (size & 1)
		panic ( "ie2host" );
#endif

	while (size > 0) {
		cnt = IE_PAGESIZE - src % IE_PAGESIZE;
		if (cnt > size)
			cnt = size;
		setpage(sc, src);
		ReadShorts((char *)sc->sc_ram + IE_COFF2POFF(src), dptr, cnt);
		src+=cnt;
		dptr+=cnt;
		size-=cnt;
	}
}

/*
 * Like a bzero or memset 0 for adapter memory.  It handles the page
 * register so you dont have to worry about it
 */

static void
iezero(struct ie_softc *sc, u_long p, int size)
{
	int cnt;

	while (size > 0) {
		cnt = IE_PAGESIZE - p % IE_PAGESIZE;
		if (cnt > size)
			cnt=size;
		setpage(sc, p);
		memset((char *)sc->sc_ram + IE_COFF2POFF(p), 0, 2*cnt);
		p += cnt;
		size -= cnt;
	}
}

/*
 * I/O Control interface to the kernel, entry point here
 */

int
ieioctl(struct ifnet *ifp, unsigned long cmd, void *data)
{
    struct ie_softc *sc = ifp->if_softc;
    struct ifaddr *ifa = (struct ifaddr *)data;
    int s;
    int error=0;

    s=splnet();

    switch ( cmd )
    {
	case SIOCINITIFADDR:
	    ifp->if_flags |= IFF_UP;
	    switch (ifa->ifa_addr->sa_family ) {
#ifdef INET
		case AF_INET:
		    ieinit(sc);
		    arp_ifinit(ifp, ifa );
		    break;
#endif
		default:
		    ieinit(sc);
		    break;
	    }
	    break;

#define IZSET(a,b) ((a->if_flags&b)!=0)
#define IZCLR(a,b) ((a->if_flags&b)==0)
#define DOSET(a,b) (a->if_flags|=b)
#define DOCLR(a,b) (a->if_flags&=~b)

	case SIOCSIFFLAGS:
	    if ((error = ifioctl_common(ifp, cmd, data)) != 0)
		return error;
	    sc->promisc = ifp->if_flags & ( IFF_PROMISC | IFF_ALLMULTI );

	    if ( IZCLR(ifp,IFF_UP) && IZSET(ifp,IFF_RUNNING) )
	    {
		/* Interface was marked down and its running so stop it */
		iestop(sc);
		DOCLR(ifp,IFF_RUNNING);
	    }
	    else if ( IZSET(ifp,IFF_UP) && IZCLR(ifp,IFF_RUNNING) )
	    {
		/* Just marked up and we're not running so start it */
		ieinit(sc);
	    }
	    else
	    {
		/* else reset to invoke changes in other registers */
		iestop(sc);
		ieinit(sc);
            }

	default:
	    error = ether_ioctl(ifp, cmd, data);
	    break;
    }
    (void)splx(s);
    return error;
}

/*
 * Reset the card.  Completely.
 */

void
iereset(struct ie_softc *sc)
{
	struct ie_sys_ctl_block scb;
	int s = splnet();

	iestop(sc);

	ie2host(sc, IE_IBASE + IE_SCB_OFF, &scb, sizeof scb);

	if (command_and_wait(sc, IE_RU_ABORT|IE_CU_ABORT, 0, 0, 0, 0, 0))
	        printf("ie0: abort commands timed out\n");

	if (command_and_wait(sc, IE_RU_DISABLE|IE_CU_STOP, 0, 0, 0, 0, 0))
	        printf("ie0: abort commands timed out\n");

	ieinit(sc);

	(void)splx(s);
}

/*
 * Watchdog entry point.  This is the entry for the kernel to call us
 */

void
iewatchdog(struct ifnet *ifp)
{
	struct ie_softc *sc = ifp->if_softc;

	log(LOG_ERR, "%s: device timeout\n", device_xname(sc->sc_dev));
	++ifp->if_oerrors;
	iereset(sc);
}

/* 
 * Start the time-domain-refloctometer running
 */

static void
run_tdr(struct ie_softc *sc)
{
    struct ie_sys_ctl_block scb;
    u_long ptr = IE_IBASE + IE_SCB_OFF + sizeof scb;
    struct ie_tdr_cmd cmd;
    int result;

    bzero ( &scb, sizeof(scb) );
    bzero ( &cmd, sizeof(cmd) );

    cmd.com.ie_cmd_status = 0;
    cmd.com.ie_cmd_cmd = IE_CMD_TDR | IE_CMD_LAST;
    cmd.com.ie_cmd_link = 0xffff;
    cmd.ie_tdr_time = 0;

    scb.ie_command_list = (u_short)ptr;

    result=0;
    if ( command_and_wait(sc, IE_CU_START, &scb, &cmd, ptr, sizeof cmd,
	IE_STAT_COMPL) )
    {
	    result = 0x10000;
    }
    else if ( !(cmd.com.ie_cmd_status & IE_STAT_OK) )
    {
	result = 0x10000;
    }

    if ( result==0 )
	result = cmd.ie_tdr_time;

    ie_ack ( sc, IE_ST_WHENCE );

    if (result & IE_TDR_SUCCESS )
	return;

    /* Very messy.  I'll tidy it later */

    if ( result & 0x10000 )
    {
	printf ( "ie: TDR command failed\n" );
    }
    else if ( result & IE_TDR_XCVR )
    {
	printf ( "ie: tranceiver problem. Is it plugged in?\n" );
    }
    else if ( result & IE_TDR_OPEN )
    {
	if ((result & IE_TDR_TIME)>0)
	    printf ( "ie: TDR detected an open %d clocks away.\n",
			result & IE_TDR_TIME );
    }
    else if ( result & IE_TDR_SHORT )
    {
	if ((result & IE_TDR_TIME)>0)
	    printf ( "ie: TDR detected a short %d clock away.\n",
			result & IE_TDR_TIME );
    }
    else
    {
	printf ( "ie: TDR returned unknown status %x\n", result );
    }
}

u_long
setup_rfa(struct ie_softc *sc, u_long ptr)
{
    int i;
    {
	/* Receive frame descriptors */
        struct ie_recv_frame_desc rfd;
	memset( &rfd, 0, sizeof rfd );
	for ( i=0; i<NFRAMES; i++ )
	{
	    sc->rframes[i] = ptr;
	    rfd.ie_fd_next = ptr + sizeof rfd;
	    host2ie(sc, (char *)&rfd, ptr, sizeof rfd);
	    ptr += sizeof rfd;
	}
	rfd.ie_fd_next = sc->rframes[0];
	rfd.ie_fd_last |= IE_FD_LAST;
	host2ie(sc, (char *)&rfd, sc->rframes[NFRAMES-1], sizeof rfd );

	ie2host(sc, sc->rframes[0], (char *)&rfd, sizeof rfd );
	rfd.ie_fd_buf_desc = (u_short) ptr;
	host2ie(sc, (char *)&rfd, sc->rframes[0], sizeof rfd );
    }

    {
	/* Receive frame descriptors */
	struct ie_recv_buf_desc rbd;
	memset(&rbd, 0, sizeof rbd);
	for ( i=0; i<NRXBUF; i++ )
	{
	    sc->rbuffs[i] = ptr;
	    rbd.ie_rbd_length = IE_RXBUF_SIZE;
	    rbd.ie_rbd_buffer = (void *)(ptr + sizeof rbd);
	    rbd.ie_rbd_next = (u_short)(ptr + sizeof rbd + IE_RXBUF_SIZE);
	    host2ie(sc, &rbd, ptr, sizeof rbd);
	    ptr+=sizeof rbd;

	    sc->cbuffs[i] = ptr;	
	    ptr+=IE_RXBUF_SIZE;
	}
	rbd.ie_rbd_next = sc->rbuffs[0];
	rbd.ie_rbd_length |= IE_RBD_LAST;
	host2ie(sc, &rbd, sc->rbuffs[NRXBUF-1], sizeof rbd);
    }

    sc->rfhead = 0;
    sc->rftail = NFRAMES-1;
    sc->rbhead = 0;
    sc->rbtail = NRXBUF-1;

    {
	struct ie_sys_ctl_block scb;
	bzero ( &scb, sizeof scb );
	scb.ie_recv_list = (u_short)sc->rframes[0];
	host2ie(sc, (char *)&scb, (IE_IBASE + IE_SCB_OFF), sizeof scb );
    }
    return ptr;
}

static void
start_receiver(struct ie_softc *sc)
{
    struct ie_sys_ctl_block scb;
    ie2host ( sc, IE_IBASE + IE_SCB_OFF, &scb, sizeof scb );
    scb.ie_recv_list = (u_short)sc->rframes[0];
    command_and_wait(sc, IE_RU_START, &scb, 0, 0, 0, 0);
    ie_ack(sc, IE_ST_WHENCE );
}

/*
 * Take our configuration and update all the other data structures that
 * require information from the driver.
 *
 * CALL AT SPLIMP OR HIGHER
 */

int
ieinit(struct ie_softc *sc)
{
    struct ifnet *ifp;
    struct ie_sys_ctl_block scb;
    struct ie_config_cmd cmd;
    struct ie_iasetup_cmd iasetup_cmd;
    u_long ptr = IE_IBASE + IE_SCB_OFF + sizeof scb;
    int n;

    ifp = &sc->sc_ethercom.ec_if;

    bzero ( &scb, sizeof(scb) );

    /* Send the configure command */

    cmd.com.ie_cmd_status = 0;
    cmd.com.ie_cmd_cmd = IE_CMD_CONFIG | IE_CMD_LAST;
    cmd.com.ie_cmd_link = 0xffff;

    cmd.ie_config_count = 0x0c;
    cmd.ie_fifo = 8;
    cmd.ie_save_bad = 0x40;
    cmd.ie_addr_len = 0x2e;
    cmd.ie_priority = 0;
    cmd.ie_ifs = 0x60;
    cmd.ie_slot_low = 0;
    cmd.ie_slot_high = 0xf2;
    cmd.ie_promisc = 0;		/* Hey nuts, look at this! */
    cmd.ie_crs_cdt = 0;
    cmd.ie_min_len = 64;
    cmd.ie_junk = 0xff;

    scb.ie_command_list = (u_short)ptr;

    if ( command_and_wait(sc, IE_CU_START, &scb, &cmd, ptr, sizeof cmd,
	IE_STAT_COMPL) )
    {
	printf ( "%s: command failed: timeout\n", device_xname(sc->sc_dev));
	return 0;
    }

    if ( !(cmd.com.ie_cmd_status & IE_STAT_OK) )
    {
	printf ( "%s: command failed: !IE_STAT_OK\n", device_xname(sc->sc_dev));
	return 0;
    }

    /* Individual address setup command */

    iasetup_cmd.com.ie_cmd_status = 0;
    iasetup_cmd.com.ie_cmd_cmd = IE_CMD_IASETUP | IE_CMD_LAST;
    iasetup_cmd.com.ie_cmd_link = 0xffff;

    bcopy ( CLLADDR(ifp->if_sadl), (void *) &iasetup_cmd.ie_address,
	 	sizeof (iasetup_cmd.ie_address) );

    if ( command_and_wait(sc, IE_CU_START, &scb, &iasetup_cmd, ptr, sizeof cmd,
	IE_STAT_COMPL) )
    {
	printf ( "%s: iasetup failed : timeout\n", device_xname(sc->sc_dev));
	return 0;
    }

    if ( !(cmd.com.ie_cmd_status & IE_STAT_OK) )
    {
	printf ( "%s: iasetup failed : !IE_STAT_OK\n", device_xname(sc->sc_dev));
	return 0;
    }

    ie_ack ( sc, IE_ST_WHENCE );

    /* Run the time-domain refloctometer */
    run_tdr ( sc );

    ie_ack ( sc, IE_ST_WHENCE );

    /* meminit */
    ptr = setup_rfa(sc, ptr);

    ifp->if_flags |= IFF_RUNNING;
    ifp->if_flags &= ~IFF_OACTIVE;

    /* Setup transmit buffers */

    for ( n=0; n<NTXBUF; n++ ) {
	sc->xmit_cmds[n] = ptr;
	iezero(sc, ptr, sizeof(struct ie_xmit_cmd) );
	ptr += sizeof(struct ie_xmit_cmd);

	sc->xmit_buffs[n] = ptr;
	iezero(sc, ptr, sizeof(struct ie_xmit_buf));
	ptr += sizeof(struct ie_xmit_buf);
    }

    for ( n=0; n<NTXBUF; n++ ) {
	sc->xmit_cbuffs[n] = ptr;
	ptr += IE_TXBUF_SIZE;
    }

    sc->xmit_free = NTXBUF;
    sc->xchead = sc->xctail = 0;

    {
	struct ie_xmit_cmd xmcmd;
	bzero ( &xmcmd, sizeof xmcmd );
	xmcmd.ie_xmit_status = IE_STAT_COMPL;
	host2ie(sc, &xmcmd, sc->xmit_cmds[0], sizeof xmcmd);
    }

    start_receiver (sc);

    return 0;
}

int
iestop(struct ie_softc *sc)
{
    struct ie_sys_ctl_block scb;
    int s = splnet();

    ie2host ( sc, IE_IBASE + IE_SCB_OFF, &scb, sizeof scb );

    if ( command_and_wait(sc, IE_RU_DISABLE, &scb, 0, 0, 0, 0) )
        printf ( "ie0: abort commands timed out\n" );

    (void)splx(s);
    return(0);
}

/*
 * Send a command to the card and await its completion.
 * Timeout if it's taking too long
 */

/*CAW*/

static int
command_and_wait(struct ie_softc *sc, u_short cmd, struct ie_sys_ctl_block *pscb, void *pcmd, int ocmd, int scmd, int mask)
{
    int i=0;

    /* Copy the command to the card */

    if ( pcmd )
	host2ie(sc, pcmd, ocmd, scmd); /* transfer the command to the card */

    /* Copy the scb to the card */

    if ( pscb ) {
	pscb->ie_command = cmd;
	host2ie(sc, pscb, IE_IBASE + IE_SCB_OFF, sizeof *pscb);
    }
    else
    {
	setpage ( sc, IE_IBASE + IE_SCB_OFF );
	PWriteShort ( sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		(xoffsetof(struct ie_sys_ctl_block, ie_command)), cmd );
    }

    /* Prod the card to act on the newly loaded command */
    ieattn(sc);

    /* Wait for the command to complete */
    if ( IE_ACTION_COMMAND(cmd) && pcmd )
    {
	setpage(sc,ocmd);
	for ( i=4000; --i>=0; ) {
	    if ( ReadShort(sc->sc_ram + IE_COFF2POFF(ocmd) +
		(xoffsetof(struct ie_config_cmd, ie_config_status))) & mask)
		break;
	    delay(100);
	}
    }
    else
    {
	for ( i=4000; --i>=0; ) {
	    if ( !ReadShort(sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		(xoffsetof(struct ie_sys_ctl_block, ie_command))) )
		break;
	    delay(100);
	}
    }

    /* Update the host structures to reflect the state on the card */
    if ( pscb )
	ie2host(sc, IE_IBASE + IE_SCB_OFF, pscb, sizeof *pscb );
    if ( pcmd )
	ie2host(sc, ocmd, pcmd, scmd);

    return i < 0;
}

#define READ_MEMBER(sc,type,member,ptr,dest)			\
	setpage(sc, ptr);					\
	dest = ReadShort(sc->sc_ram + IE_COFF2POFF(ptr) +	\
	       (xoffsetof(type, member)) );

#define WRITE_MEMBER(sc,type,member,ptr,dest)			\
	setpage(sc, ptr);					\
	PWriteShort(sc->sc_ram + IE_COFF2POFF(ptr) +	\
	       (xoffsetof(type, member)), dest );

static inline int
ie_buflen(struct ie_softc *sc, int head)
{
	int actual;

	READ_MEMBER(sc,struct ie_recv_buf_desc, ie_rbd_actual,
	    sc->rbuffs[head], actual );

	return(actual & (IE_RXBUF_SIZE | (IE_RXBUF_SIZE-1))) ;
}

static inline int
ie_packet_len(struct ie_softc *sc)
{
    int i;
    int actual;
    int head = sc->rbhead;
    int acc=0;

    do {
	READ_MEMBER(sc,struct ie_recv_buf_desc, ie_rbd_actual,
			sc->rbuffs[sc->rbhead], actual );
	if (!(actual&IE_RBD_USED))
	{
	    return (-1);
	}

	READ_MEMBER(sc,struct ie_recv_buf_desc, ie_rbd_actual,
			sc->rbuffs[head], i );
        i = i & IE_RBD_LAST;

	acc += ie_buflen(sc, head);
	head = (head+1) % NRXBUF;
    } while (!i);

    return acc;
}

struct mbuf *
ieget(struct ie_softc *sc, int *to_bpf )
{
    struct mbuf *top, **mp, *m;
    int head;
    int resid, totlen, thisrboff, thismboff;
    int len;
    struct ether_header eh;

    totlen = ie_packet_len(sc);

    if ( totlen > ETHER_MAX_LEN )
    {
	printf ( "ie: Gosh that packet was s-o-o-o big.\n" );
	return 0;
    }

    if ( totlen<=0 )
	return 0;

    head = sc->rbhead;

    /* Read the ethernet header */
    ie2host ( sc, sc->cbuffs[head], (void *)&eh, sizeof eh );

    /* Check if the packet is for us */

    resid = totlen;

    MGETHDR ( m, M_DONTWAIT, MT_DATA );
    if ( m==0 )
	return 0;

    m_set_rcvif(m, &sc->sc_ethercom.ec_if);
    m->m_pkthdr.len = totlen;
    len = MHLEN;
    top = 0;
    mp = &top;

    /*
     * This loop goes through and allocates mbufs for all the data we will
     * be copying in.  It does not actually do the copying yet.
     */
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

	if (mp == &top) {
		void *newdata = (void *)
		    ALIGN(m->m_data + sizeof(struct ether_header)) -
		    sizeof(struct ether_header);
		len -= newdata - m->m_data; 
		m->m_data = newdata;
	}

        m->m_len = len = min(totlen, len);

        totlen -= len;
        *mp = m;
        mp = &m->m_next;
    }

    m = top;
    thismboff = 0;

    /*
     * Copy the Ethernet header into the mbuf chain.
     */
    memcpy(mtod(m, void *), &eh, sizeof(struct ether_header));
    thismboff = sizeof(struct ether_header);
    thisrboff = sizeof(struct ether_header);
    resid -= sizeof(struct ether_header);

    /*
     * Now we take the mbuf chain (hopefully only one mbuf most of the
     * time) and stuff the data into it.  There are no possible failures at
     * or after this point.
     */
    while (resid > 0) {
	int thisrblen = ie_buflen(sc, head) - thisrboff,
	    thismblen = m->m_len - thismboff;
	len = min(thisrblen, thismblen);

/*      bcopy((void *)(sc->cbuffs[head] + thisrboff),
	    mtod(m, void *) + thismboff, (u_int)len);	 */


	if ( len&1 )
	{
 	    ie2host(sc, sc->cbuffs[head]+thisrboff,
		mtod(m, void *) + thismboff, (u_int)len+1);
  	}
	else
	{
 	    ie2host(sc, sc->cbuffs[head]+thisrboff,
		mtod(m, void *) + thismboff, (u_int)len);
	}

	resid -= len;

	if (len == thismblen) {
		m = m->m_next;
		thismboff = 0;
	} else
		thismboff += len;

	if (len == thisrblen) {
		head = (head + 1) % NRXBUF;
		thisrboff = 0;
	} else
		thisrboff += len;
    }


    return top;
}

void
ie_drop_packet_buffer(struct ie_softc *sc)
{
    int i, actual, last;

    do {
	READ_MEMBER(sc,struct ie_recv_buf_desc, ie_rbd_actual,
			sc->rbuffs[sc->rbhead], actual );
	if (!(actual&IE_RBD_USED))
	{
	    iereset(sc);
	    return;
	}

	i = actual & IE_RBD_LAST;

        READ_MEMBER(sc,struct ie_recv_buf_desc,ie_rbd_length,
			sc->rbuffs[sc->rbhead], last );
        last |= IE_RBD_LAST;
        WRITE_MEMBER(sc,struct ie_recv_buf_desc,ie_rbd_length,
			sc->rbuffs[sc->rbhead], last );

        WRITE_MEMBER(sc,struct ie_recv_buf_desc,ie_rbd_actual,
			sc->rbuffs[sc->rbhead], 0 );

	sc->rbhead = ( sc->rbhead + 1 ) % NRXBUF;

        READ_MEMBER(sc,struct ie_recv_buf_desc,ie_rbd_length,
			sc->rbuffs[sc->rbtail], last );
        last &= ~IE_RBD_LAST;
        WRITE_MEMBER(sc,struct ie_recv_buf_desc,ie_rbd_length,
			sc->rbuffs[sc->rbtail], last );

	sc->rbtail = ( sc->rbtail + 1 ) % NRXBUF;
    } while (!i);
}

void
ie_read_frame(struct ie_softc *sc, int num)
{
    int status;
    struct ie_recv_frame_desc rfd;
    struct mbuf *m=0;
    struct ifnet *ifp;
    int last;

    ifp = &sc->sc_ethercom.ec_if;

    ie2host(sc, sc->rframes[num], &rfd, sizeof rfd );
    status = rfd.ie_fd_status;

    /* Advance the RFD list, since we're done with this descriptor */

    WRITE_MEMBER(sc,struct ie_recv_frame_desc,ie_fd_status,
			sc->rframes[num], 0 );

    READ_MEMBER(sc,struct ie_recv_frame_desc,ie_fd_last,
			sc->rframes[num], last );
    last |= IE_FD_LAST;
    WRITE_MEMBER(sc,struct ie_recv_frame_desc,ie_fd_last,
			sc->rframes[num], last );

    READ_MEMBER(sc,struct ie_recv_frame_desc,ie_fd_last,
			sc->rframes[sc->rftail], last );
    last &= ~IE_FD_LAST;
    WRITE_MEMBER(sc,struct ie_recv_frame_desc,ie_fd_last,
			sc->rframes[sc->rftail], last );

    sc->rftail = ( sc->rftail + 1 ) % NFRAMES;
    sc->rfhead = ( sc->rfhead + 1 ) % NFRAMES;

    if ( status & IE_FD_OK ) {
	m = ieget(sc, 0);
	ie_drop_packet_buffer(sc);
    }

    if ( m==0 ) {
	ifp->if_ierrors++;
	return;
    }

    if_percpuq_enqueue(ifp->if_percpuq, m);
}

void
ierint(struct ie_softc *sc)
{
    int i;
    int times_thru = 1024;
    struct ie_sys_ctl_block scb;
    int status;
    int safety_catch = 0;

    i = sc->rfhead;
    for (;;) {

	if ( (safety_catch++)>100 )
	{
	    printf ( "ie: ierint safety catch tripped\n" );
	    iereset(sc);
	    return;
	}

	READ_MEMBER(sc,struct ie_recv_frame_desc,ie_fd_status,
				sc->rframes[i],status);

	if ((status&IE_FD_COMPLETE)&&(status&IE_FD_OK)) {
	    if ( !--times_thru ) {
		printf ( "IERINT: Uh oh. Nuts, look at this bit!!!\n" );
    		ie2host ( sc, IE_IBASE + IE_SCB_OFF, &scb, sizeof scb );
		sc->sc_ethercom.ec_if.if_ierrors += scb.ie_err_crc +
						  scb.ie_err_align +
						  scb.ie_err_resource +
						  scb.ie_err_overrun;
		scb.ie_err_crc      = scb.ie_err_align   = 0;
		scb.ie_err_resource = scb.ie_err_overrun = 0;
	        host2ie(sc, &scb, IE_SCP_ADDR, sizeof (scb) );
	    }
	    ie_read_frame(sc, i);
	} else {
    	    ie2host ( sc, IE_IBASE + IE_SCB_OFF, &scb, sizeof scb );

	    if ( ((status&IE_FD_RNR)!=0) && ((scb.ie_status&IE_RU_READY)==0) )
	    {
		WRITE_MEMBER(sc,struct ie_recv_frame_desc, ie_fd_buf_desc,
				sc->rframes[0], sc->rbuffs[0] );

		scb.ie_recv_list = sc->rframes[0];
	        host2ie(sc, (char *)&scb, IE_IBASE + IE_SCB_OFF, sizeof (scb) );
    		command_and_wait(sc, IE_RU_START, &scb, 0, 0, 0, 0);
	    }
	    break;
	}
	i = (i + 1) % NFRAMES;
    }
}

static int in_intr = 0;

int
ieintr(void *arg)
{
    struct ie_softc *sc = arg;
    u_short status;
    int safety_catch = 0;
    static int safety_net = 0;

    if (in_intr == 1)
	panic ( "ie: INTERRUPT REENTERED\n" );

    /* Clear the interrrupt */
    ie_cli (sc);

    setpage(sc, IE_IBASE + IE_SCB_OFF );
    status = ReadShort ( sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		(xoffsetof(struct ie_sys_ctl_block, ie_status)) );

    status = status & IE_ST_WHENCE;

    if (status == 0) {
	in_intr = 0;
	return(0);
    }

loop:

    ie_ack(sc, status);

    if (status & (IE_ST_FR | IE_ST_RNR))
	ierint(sc);

    if (status & IE_ST_CX)
	ietint(sc);

    if (status & IE_ST_RNR) {
	printf ( "ie: receiver not ready\n" );
	sc->sc_ethercom.ec_if.if_ierrors++;
	iereset(sc);
    }

    setpage(sc, IE_IBASE + IE_SCB_OFF );
    status = ReadShort ( sc->sc_ram + IE_COFF2POFF(IE_IBASE+IE_SCB_OFF) +
		(xoffsetof(struct ie_sys_ctl_block, ie_status)) );
    status = status & IE_ST_WHENCE;

    ie_cli(sc);

    if (status == 0) {
	in_intr = 0;
	return(0);
    }

    /* This is prehaps a little over cautious */
    if ( safety_catch++ > 10 )
    {
	printf ( "ie: Interrupt couldn't be cleared\n" );
	delay ( 1000 );
	ie_cli(sc);
	if ( safety_net++ > 50 )
	{
/*	    printf ( "ie: safety net catches driver, shutting down\n" );
	    disable_irq ( IRQ_PODULE );*/
	}
	in_intr = 0;
	return(0);
    }

    goto loop;
}

void
iexmit(struct ie_softc *sc)
{
/*    int actual;*/
    struct ie_sys_ctl_block scb;

    struct ie_xmit_cmd xc;
    struct ie_xmit_buf xb;
    
    ie2host(sc, sc->xmit_buffs[sc->xctail], (char *)&xb, sizeof xb );
    xb.ie_xmit_flags |= IE_XMIT_LAST;
    xb.ie_xmit_next = 0xffff;
    xb.ie_xmit_buf = (void *)sc->xmit_cbuffs[sc->xctail];
    host2ie(sc, &xb, sc->xmit_buffs[sc->xctail], sizeof xb );

    bzero ( &xc, sizeof xc );
    xc.com.ie_cmd_link = 0xffff;
    xc.com.ie_cmd_cmd = IE_CMD_XMIT | IE_CMD_INTR | IE_CMD_LAST;
    xc.ie_xmit_status = 0x0000;
    xc.ie_xmit_desc = sc->xmit_buffs[sc->xctail];
    host2ie(sc, (char *)&xc, sc->xmit_cmds[sc->xctail], sizeof xc );

    ie2host ( sc, IE_IBASE + IE_SCB_OFF, &scb, sizeof scb );
    scb.ie_command_list = sc->xmit_cmds[sc->xctail];
    host2ie(sc, (char *)&scb, (IE_IBASE + IE_SCB_OFF), sizeof scb );

    command_and_wait(sc, IE_CU_START, &scb, &xc, sc->xmit_cmds[sc->xctail]
			, sizeof xc, IE_STAT_COMPL);

    sc->sc_ethercom.ec_if.if_timer = 5;
}
/*
 * Start sending all the queued buffers.
 */

void
iestart(struct ifnet *ifp)
{
	struct ie_softc *sc = ifp->if_softc;
	struct mbuf *m0, *m;
	u_char *buffer;
	u_short len;
	char txbuf[IE_TXBUF_SIZE];
	int safety_catch = 0;

	if ((ifp->if_flags & IFF_OACTIVE) != 0)
		return;

	for (;;) {
		if ( (safety_catch++)>100 )
		{
		    printf ( "ie: iestart safety catch tripped\n" );
		    iereset(sc);
		    return;
		}
		if (sc->xmit_free == 0) {
			ifp->if_flags |= IFF_OACTIVE;
			break;
		}

		IF_DEQUEUE(&ifp->if_snd, m);
		if (!m)
			break;

		/* TODO: Write directly to the card */
		len = 0;
		/* buffer = sc->xmit_cbuffs[sc->xchead]; */
		buffer = txbuf;

		for (m0 = m; m && (len + m->m_len) < IE_TXBUF_SIZE;
		     m = m->m_next) {
			memcpy(buffer, mtod(m, void *), m->m_len);
			buffer += m->m_len;
			len += m->m_len;
		}

		bpf_mtap(ifp, m0);

		m_freem(m0);
		if (len < ETHER_MIN_LEN - ETHER_CRC_LEN) {
			memset(buffer, 0, ETHER_MIN_LEN - ETHER_CRC_LEN - len);
			len = ETHER_MIN_LEN - ETHER_CRC_LEN;
			buffer += ETHER_MIN_LEN - ETHER_CRC_LEN;
		}
			
		/* When we write directly to the card we dont need this */
    		if (len&1)
   		    host2ie(sc, txbuf, sc->xmit_cbuffs[sc->xchead], len+1 );
		else
   		    host2ie(sc, txbuf, sc->xmit_cbuffs[sc->xchead], len );

		/* sc->xmit_buffs[sc->xchead]->ie_xmit_flags = len; */
		
		WRITE_MEMBER(sc,struct ie_xmit_buf, ie_xmit_flags,
				sc->xmit_buffs[sc->xchead], len)

		/* Start the first packet transmitting. */
		if (sc->xmit_free == NTXBUF)
			iexmit(sc);

		sc->xchead = (sc->xchead + 1) % NTXBUF;
		sc->xmit_free--;
	}
}

void
ietint(struct ie_softc *sc)
{
    struct ifnet *ifp = &sc->sc_ethercom.ec_if;

    int status;

    ifp->if_timer=0;
    ifp->if_flags &= ~IFF_OACTIVE;
 
    READ_MEMBER(sc,struct ie_xmit_cmd, ie_xmit_status,
	sc->xmit_cmds[sc->xctail], status );

    if (!(status&IE_STAT_COMPL) || (status & IE_STAT_BUSY) )
	printf ( "ietint: command still busy!\n" );
    
    if ( status & IE_STAT_OK ) {
	ifp->if_opackets++;
	ifp->if_collisions += status & IE_XS_MAXCOLL;
    } else {
	ifp->if_oerrors++;	
	if ( status & IE_STAT_ABORT )
	    printf ( "ie: send aborted\n" );
	if ( status & IE_XS_LATECOLL )
	    printf ( "ie: late collision\n" );
	if ( status & IE_XS_NOCARRIER )
	    printf ( "ie: no carrier\n" );
	if ( status & IE_XS_LOSTCTS )
	    printf ( "ie: lost CTS\n" );
	if ( status & IE_XS_UNDERRUN )
	    printf ( "ie: DMA underrun\n" );
	if ( status & IE_XS_EXCMAX )
	    printf ( "ie: too many collisions\n" );
	ifp->if_collisions+=16;
    }
    /* Done with the buffer */
    sc->xmit_free++;
    sc->xctail = (sc->xctail + 1 ) % NTXBUF;
 
    /* Start the next packet transmitting, if any */
    if ( sc->xmit_free<NTXBUF )
	iexmit(sc);

    if_schedule_deferred_start(ifp);
}

/* End of if_ie.c */
