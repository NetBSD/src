/*	$NetBSD: if_qt.c,v 1.2 2003/08/29 13:49:39 ragge Exp $	*/
/*
 * Copyright (c) 1992 Steven M. Schultz
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 *
 *	@(#)if_qt.c     1.2 (2.11BSD) 2/20/93
 */
/*
 *
 * Modification History 
 * 23-Feb-92 -- sms
 *	Rewrite the buffer handling so that fewer than the maximum number of
 *	buffers may be used (32 receive and 12 transmit buffers consume 66+kb
 *	of main system memory in addition to the internal structures in the
 *	networking code).  A freelist of available buffers is maintained now.
 *	When I/O operations complete the associated buffer is placed on the 
 *	freelist (a single linked list for simplicity) and when an I/O is 
 *	started a buffer is pulled off the list. 
 *
 * 20-Feb-92 -- sms
 *	It works!  Darned board couldn't handle "short" rings - those rings
 *	where only half the entries were made available to the board (the
 *	ring descriptors were the full size, merely half the entries were
 * 	flagged as belonging always to the driver).  Grrrr.  Would have thought
 *	the board could skip over those entries reserved by the driver. 
 *	Now to find a way not to have to allocated 32+12 times 1.5kb worth of 
 *	buffers...
 *
 * 03-Feb-92 -- sms
 *	Released but still not working.  The driver now responds to arp and
 *	ping requests.  The board is apparently not returning ring descriptors
 *	to the driver so eventually we run out of buffers.  Back to the
 *	drawing board.
 *
 * 28-Dec-92 -- sms
 *	Still not released.  Hiatus in finding free time and thin-netting
 *	the systems (many thanks Terry!).
 *	Added logic to dynamically allocate a vector and initialize it.
 *
 * 23-Oct-92 -- sms
 *	The INIT block must (apparently) be quadword aligned [no thanks to
 *	the manual for not mentioning that fact].  The necessary alignment
 *	is achieved by allocating the INIT block from main memory ('malloc'
 *	guarantees click alignment) and mapping it as needed (which is _very_
 *	infrequently).  A check for quadword alignment of the ring descriptors
 *	was added - at present the descriptors are properly aligned, if this
 *	should change then something will have to be done (like do it "right").
 *	Darned alignment restrictions!
 *
 *	A couple of typos were corrected (missing parentheses, reversed 
 *	arguments to printf calls, etc).
 *
 * 13-Oct-92 -- sms@wlv.iipo.gtegsc.com
 *	Created based on the DELQA-PLUS addendum to DELQA User's Guide.
 *	This driver ('qt') is selected at system configuration time.  If the 
 *	board *	is not a DELQA-YM an error message will be printed and the 
 *	interface will not be attached.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_qt.c,v 1.2 2003/08/29 13:49:39 ragge Exp $");

#include "opt_inet.h"
#include "bpfilter.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/netisr.h>
#include <net/route.h>

#ifdef INET
#include <sys/domain.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#endif

#ifdef NS
#include <netns/ns.h>
#include <netns/ns_if.h>
#endif

#include <machine/bus.h>
#ifdef __vax__
#include <machine/scb.h>
#endif

#include <dev/qbus/ubavar.h>
#include <dev/qbus/if_uba.h>
#include <dev/qbus/if_qtreg.h>

#define NRCV	QT_MAX_RCV	/* Receive descriptors (must be == 32) */
#define NXMT	QT_MAX_XMT	/* Transmit descriptors	(must be == 12) */
#if	NRCV != 32 || NXMT != 12
	hardware requires these sizes.
#endif
 
struct	qt_cdata {
	struct	qt_init qc_init;	/* Init block			*/
	struct	qt_rring qc_r[NRCV];	/* Receive descriptor ring	*/
	struct	qt_tring qc_t[NXMT];	/* Transmit descriptor ring	*/
};

struct	qt_softc {
	struct	device sc_dev;		/* Configuration common part */
	struct	ethercom is_ec;		/* common part - must be first  */
	struct	evcnt sc_intrcnt;	/* Interrupt counting */
#define	is_if	is_ec.ec_if		/* network-visible interface 	*/
	u_int8_t is_addr[ETHER_ADDR_LEN]; /* hardware Ethernet address	*/
	bus_space_tag_t	sc_iot;
	bus_addr_t	sc_ioh;
	bus_dma_tag_t	sc_dmat;

	struct	ubinfo	sc_ui;		/* init block address desc	*/
	struct	qt_cdata *sc_ib;	/* virt address of init block	*/
	struct	qt_cdata *sc_pib;	/* phys address of init block	*/

	struct	ifubinfo sc_ifuba;	/* UNIBUS resources */
	struct	ifrw sc_ifr[NRCV];	/* UNIBUS receive buffer maps */
	struct	ifxmt sc_ifw[NXMT];	/* UNIBUS receive buffer maps */

	char	rindex;			/* Receive Completed Index	*/
	char	nxtrcv;			/* Next Receive Index		*/
	char	nrcv;			/* Number of Receives active	*/

	int	xnext;			/* Next descriptor to transmit	*/
	int	xlast;			/* Last descriptor transmitted	*/
	int	nxmit;			/* # packets in send queue	*/
	short	vector;			/* Interrupt vector assigned	*/
	volatile struct	qtdevice *addr;	/* device CSR addr		*/
};

static	int qtmatch(struct device *, struct cfdata *, void *);
static	void qtattach(struct device *, struct device *, void *);
static	void qtintr(void *);
static	int qtinit(struct ifnet *);
static	int qtioctl(struct ifnet *, u_long, caddr_t);
static	int qtturbo(struct qt_softc *);
static	void qtstart(struct ifnet *ifp);
static	void qtsrr(struct qt_softc *, int);
static	void qtrint(struct qt_softc *sc);
static	void qttint(struct qt_softc *sc);

/* static	void qtrestart(struct qt_softc *sc); */
 
CFATTACH_DECL(qt, sizeof(struct qt_softc),
    qtmatch, qtattach, NULL, NULL);

/*
 * Maximum packet size needs to include 4 bytes for the CRC 
 * on received packets.
*/
#define MAXPACKETSIZE (ETHERMTU + sizeof (struct ether_header) + 4)
#define	MINPACKETSIZE 64

#define loint(x)	((int)(x) & 0xffff)
#define hiint(x)	(((int)(x) >> 16) & 0x3f)
#define	XNAME		sc->sc_dev.dv_xname

/*
 * Check if this card is a turbo delqa.
 */
int
qtmatch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct	qt_softc ssc;
	struct	qt_softc *sc = &ssc;
	struct	uba_attach_args *ua = aux;
	struct	uba_softc *ubasc = (struct uba_softc *)parent;


	sc->addr = (struct qtdevice *)ua->ua_ioh;
	if (qtturbo(sc) == 0)
		return 0;

	scb_fake((ubasc->uh_lastiv-4) + VAX_NBPG, 0x16); /* XXX */
	return 10;
}


/*
 * Interface exists.  More accurately, something exists at the CSR (see
 * sys/sys_net.c) -- there's no guarantee it's a DELQA-YM.
 *
 * The ring descriptors are initialized, the buffers allocated using first the
 * DMA region allocated at network load time and then later main memory.  The
 * INIT block is filled in and the device is poked/probed to see if it really
 * is a DELQA-YM.  If the device is not a -YM then a message is printed and
 * the 'if_attach' call is skipped.  For a -YM the START command is issued,
 * but the device is not marked as running|up - that happens at interrupt level
 * when the device interrupts to say it has started.
*/

void
qtattach(struct device *parent, struct device *self, void *aux)
	{
	struct	uba_softc *ubasc = (struct uba_softc *)parent;
	register struct qt_softc *sc = (struct qt_softc *)self;
	register struct ifnet *ifp = &sc->is_if;
	struct uba_attach_args *ua = aux;

	uba_intr_establish(ua->ua_icookie, ua->ua_cvec, qtintr, sc, 
	    &sc->sc_intrcnt);
	evcnt_attach_dynamic(&sc->sc_intrcnt, EVCNT_TYPE_INTR, ua->ua_evcnt,
	    sc->sc_dev.dv_xname, "intr");

	sc->addr = (struct qtdevice *)ua->ua_ioh;
	ubasc->uh_lastiv -= 4;
	sc->vector = ubasc->uh_lastiv;

/*
 * Now allocate the buffers and initialize the buffers.  This should _never_
 * fail because main memory is allocated after the DMA pool is used up.
*/

	sc->is_addr[0] = sc->addr->sarom[0];
	sc->is_addr[1] = sc->addr->sarom[2];
	sc->is_addr[2] = sc->addr->sarom[4];
	sc->is_addr[3] = sc->addr->sarom[6];
	sc->is_addr[4] = sc->addr->sarom[8];
	sc->is_addr[5] = sc->addr->sarom[10];

	strcpy(ifp->if_xname, sc->sc_dev.dv_xname);
	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST;
	ifp->if_ioctl = qtioctl;
	ifp->if_start = qtstart;
	ifp->if_init = qtinit;
/*	ifp->if_stop = qtstop;	*/
	IFQ_SET_READY(&ifp->if_snd);
 
	printf("\n%s: delqa-plus in Turbo mode, hardware address %s\n",
	    XNAME, ether_sprintf(sc->is_addr));
	if_attach(ifp);
	ether_ifattach(ifp, sc->is_addr);
	}
 
int
qtturbo(sc)
	register struct qt_softc *sc;
	{
	register int i;
	volatile struct qtdevice *addr = sc->addr;

/*
 * Issue the software reset.  Delay 150us.  The board should now be in
 * DELQA-Normal mode.  Set ITB and DEQTA select.  If both bits do not
 * stay turned on then the board is not a DELQA-YM.
*/
	addr->arqr = ARQR_SR;
	addr->arqr = 0;
	delay(150L);

	addr->srr = 0x8001;		/* MS | ITB */
	i = addr->srr;
	addr->srr = 0x8000;		/* Turn off ITB, set DELQA select */
	if	(i != 0x8001)
		{
		printf("qt@%p !-YM\n", addr);
		return(0);
		}
/*
 * Board is a DELQA-YM.  Send the commands to enable Turbo mode.  Delay
 * 1 second, testing the SRR register every millisecond to see if the
 * board has shifted to Turbo mode.
*/
	addr->xcr0 = 0x0baf;
	addr->xcr1 = 0xff00;
	for	(i = 0; i < 1000; i++)
		{
		if	((addr->srr & SRR_RESP) == 1)
			break;
		delay(1000L);
		}
	if	(i >= 1000)
		{
		printf("qt@%p !Turbo\n", addr);
		return(0);
		}
	return(1);
	}

int
qtinit(struct ifnet *ifp)
	{
	register struct qt_softc *sc = ifp->if_softc;
	volatile struct qtdevice *addr = sc->addr;
	register struct qt_init *iniblk;
	struct ifrw *ifrw;
	struct ifxmt *ifxp;
	struct	qt_rring *rp;
	struct	qt_tring *tp;
	register int i, error;
 
	if (ifp->if_flags & IFF_RUNNING)
		return 0; /* Already in good shape */

	if (sc->sc_ib == NULL) {
		if (if_ubaminit(&sc->sc_ifuba, (void *)sc->sc_dev.dv_parent,
		    MCLBYTES, sc->sc_ifr, NRCV, sc->sc_ifw, NXMT)) {
			printf("%s: can't initialize\n", XNAME);
			ifp->if_flags &= ~IFF_UP;
			return 0;
		}
		sc->sc_ui.ui_size = sizeof(struct qt_cdata);
		if ((error = ubmemalloc((void *)sc->sc_dev.dv_parent,
		    &sc->sc_ui, 0))) {
			printf(": failed ubmemalloc(), error = %d\n", error);
			return error;
		}
		sc->sc_ib = (struct qt_cdata *)sc->sc_ui.ui_vaddr;
		sc->sc_pib = (struct qt_cdata *)sc->sc_ui.ui_baddr;

/*
 * Fill in most of the INIT block: vector, options (interrupt enable), ring
 * locations.  The physical address is copied from the ROMs as part of the
 * -YM testing proceedure.  The CSR is saved here rather than in qtinit()
 * because the qtturbo() routine needs it.
 *
 * The INIT block must be quadword aligned.  Using malloc() guarantees click
 * (64 byte) alignment.  Since the only time the INIT block is referenced is
 * at 'startup' or 'reset' time there is really no time penalty (and a modest
 * D space savings) involved.
*/
		memset(sc->sc_ib, 0, sizeof(struct qt_cdata));
		iniblk = &sc->sc_ib->qc_init;

		iniblk->vector = sc->vector;
		memcpy(iniblk->paddr, sc->is_addr, 6);

		iniblk->options = INIT_OPTIONS_INT;
		iniblk->rx_lo = loint(&sc->sc_pib->qc_r);
		iniblk->rx_hi = hiint(&sc->sc_pib->qc_r);
		iniblk->tx_lo = loint(&sc->sc_pib->qc_t);
		iniblk->tx_hi = hiint(&sc->sc_pib->qc_t);
	}


/*
 * Now initialize the receive ring descriptors.  Because this routine can be
 * called with outstanding I/O operations we check the ring descriptors for
 * a non-zero 'rhost0' (or 'thost0') word and place those buffers back on
 * the free list.
*/
	for (i = 0; i < NRCV; i++) {
		rp = &sc->sc_ib->qc_r[i];
		ifrw = &sc->sc_ifr[i];
		rp->rmd1 = MCLBYTES;
		rp->rmd4 = loint(ifrw->ifrw_info);
		rp->rmd5 = hiint(ifrw->ifrw_info);
		rp->rmd3 = 0;			/* clear RMD3_OWN */
		}
	for (i = 0; i < NXMT; i++) {
		tp = &sc->sc_ib->qc_t[i];
		ifxp = &sc->sc_ifw[i];
		tp->tmd4 = loint(ifxp->ifw_info);
		tp->tmd5 = hiint(ifxp->ifw_info);
		tp->tmd3 = TMD3_OWN;
		}

	sc->xnext = sc->xlast = sc->nxmit = 0;
	sc->rindex = 0;
	sc->nxtrcv = 0;
	sc->nrcv = 0;
 
/*
 * Now we tell the device the address of the INIT block.  The device
 * _must_ be in the Turbo mode at this time.  The "START" command is
 * then issued to the device.  A 1 second timeout is then started.
 * When the interrupt occurs the IFF_UP|IFF_RUNNING state is entered and
 * full operations will proceed.  If the timeout expires without an interrupt 
 * being received an error is printed, the flags cleared and the device left
 * marked down.
*/
	addr->ibal = loint(&sc->sc_pib->qc_init);
	addr->ibah = hiint(&sc->sc_pib->qc_init);
	addr->srqr = 2;

	sc->is_if.if_flags |= IFF_RUNNING;
	return 0;
	}

/*
 * Start output on interface.
 */

void
qtstart(struct ifnet *ifp)
	{
	int 	len, nxmit;
	register struct qt_softc *sc = ifp->if_softc;
	register struct qt_tring *rp;
	struct	mbuf *m = NULL;
 
	for (nxmit = sc->nxmit; nxmit < NXMT; nxmit++) {
		IF_DEQUEUE(&sc->is_if.if_snd, m);
		if	(m == 0)
			break;

		rp = &sc->sc_ib->qc_t[sc->xnext];
		if ((rp->tmd3 & TMD3_OWN) == 0)
			panic("qtstart");

		len = if_ubaput(&sc->sc_ifuba, &sc->sc_ifw[sc->xnext], m);
		if (len < MINPACKETSIZE)
			len = MINPACKETSIZE;
		rp->tmd3 = len & TMD3_BCT;	/* set length,clear ownership */
		sc->addr->arqr = ARQR_TRQ;	/* tell device it has buffer */
		if	(++sc->xnext >= NXMT)
			sc->xnext = 0;
	}
	if (sc->nxmit != nxmit)
		sc->nxmit = nxmit;
	/* XXX - set OACTIVE */
}
 
/*
 * General interrupt service routine.  Receive, transmit, device start 
 * interrupts and timeouts come here.  Check for hard device errors and print a
 * message if any errors are found.  If we are waiting for the device to 
 * START then check if the device is now running.
*/

void
qtintr(void *arg)
	{
	struct qt_softc *sc = arg;
	short status;

 
	status = sc->addr->srr;
	if	(status < 0)
		/* should we reset the device after a bunch of these errs? */
		qtsrr(sc, status);
	qtrint(sc);
	qttint(sc);
	qtstart(&sc->is_ec.ec_if);
	}
 
/*
 * Transmit interrupt service.  Only called if there are outstanding transmit
 * requests which could have completed.  The DELQA-YM doesn't provide the
 * status bits telling the kind (receive, transmit) of interrupt.
*/
 
#define BBLMIS (TMD2_BBL|TMD2_MIS)

void
qttint(struct qt_softc *sc)
	{
	register struct qt_tring *rp;
 
	while (sc->nxmit > 0)
		{
		rp = &sc->sc_ib->qc_t[sc->xlast];
		if ((rp->tmd3 & TMD3_OWN) == 0)
			break;
		sc->is_if.if_opackets++;
/*
 * Collisions don't count as output errors, but babbling and missing packets
 * do count as output errors.
*/
		if	(rp->tmd2 & TMD2_CER)
			sc->is_if.if_collisions++;
		if	((rp->tmd0 & TMD0_ERR1) || 
			 ((rp->tmd2 & TMD2_ERR2) && (rp->tmd2 & BBLMIS)))
			{
#ifdef QTDEBUG
			char buf[100];
			bitmask_snprintf(rp->tmd2, TMD2_BITS, buf, 100);
			printf("%s: tmd2 %s\n", XNAME, buf);
#endif
			sc->is_if.if_oerrors++;
			}
		if_ubaend(&sc->sc_ifuba, &sc->sc_ifw[sc->xlast]);
		if	(++sc->xlast >= NXMT)
			sc->xlast = 0;
		sc->nxmit--;
		}
	}
 
/*
 * Receive interrupt service.  Pull packet off the interface and put into
 * a mbuf chain for processing later.
*/

void
qtrint(struct qt_softc *sc)
	{
	register struct qt_rring *rp;
	struct ifnet *ifp = &sc->is_ec.ec_if;
	struct mbuf *m;
	int	len;
 
	while	(sc->sc_ib->qc_r[(int)sc->rindex].rmd3 & RMD3_OWN)
		{
		rp = &sc->sc_ib->qc_r[(int)sc->rindex];
		if     ((rp->rmd0 & (RMD0_STP|RMD0_ENP)) != (RMD0_STP|RMD0_ENP))
			{
			printf("%s: chained packet\n", XNAME);
			sc->is_if.if_ierrors++;
			goto rnext;
			}
		len = (rp->rmd1 & RMD1_MCNT) - 4;	/* -4 for CRC */
		sc->is_if.if_ipackets++;
 
		if	((rp->rmd0 & RMD0_ERR3) || (rp->rmd2 & RMD2_ERR4))
			{
#ifdef QTDEBUG
			char buf[100];
			bitmask_snprintf(rp->rmd0, RMD0_BITS, buf, 100);
			printf("%s: rmd0 %s\n", XNAME, buf);
			bitmask_snprintf(rp->rmd2, RMD2_BITS, buf, 100);
			printf("%s: rmd2 %s\n", XNAME, buf);
#endif
			sc->is_if.if_ierrors++;
			goto rnext;
			}
		m = if_ubaget(&sc->sc_ifuba, &sc->sc_ifr[(int)sc->rindex],
		    ifp, len);
		if (m == 0) {
			sc->is_if.if_ierrors++;
			goto rnext;
		}
		(*ifp->if_input)(ifp, m);
rnext:
		--sc->nrcv;
		rp->rmd3 = 0;
		rp->rmd1 = MCLBYTES;
		if	(++sc->rindex >= NRCV)
			sc->rindex = 0;
		}
	sc->addr->arqr = ARQR_RRQ;	/* tell device it has buffer */
	}

int
qtioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	u_long	cmd;
	caddr_t	data;
	{
	int error;
	int s = splnet();

	error = ether_ioctl(ifp, cmd, data);
	if (error == ENETRESET)
		error = 0;
	splx(s);
	return (error);
}

void
qtsrr(sc, srrbits)
	struct qt_softc *sc;
	int	srrbits;
	{
	char buf[100];
	bitmask_snprintf(srrbits, SRR_BITS, buf, sizeof buf);
	printf("%s: srr=%s\n", sc->sc_dev.dv_xname, buf);
	}

#ifdef notyet
/*
 * Reset the device.  This moves it from DELQA-T mode to DELQA-Normal mode.
 * After the reset put the device back in -T mode.  Then call qtinit() to
 * reinitialize the ring structures and issue the 'timeout' for the "device
 * started interrupt".
*/

void
qtreset(sc)
	register struct qt_softc *sc;
	{
 
	qtturbo(sc);
	qtinit(&sc->is_ec.ec_if);
	}
#endif
