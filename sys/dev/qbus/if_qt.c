/*	$NetBSD: if_qt.c,v 1.1 2003/08/28 10:03:32 ragge Exp $	*/
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
 
#include "qt.h"
#if	NQT > 0

#include "param.h"
#include "pdp/psl.h"
#include "pdp/seg.h"
#include "map.h"
#include "systm.h"
#include "mbuf.h"
#include "buf.h"
#include "protosw.h"
#include "socket.h"
#include "ioctl.h"
#include "errno.h"
#include "syslog.h"
#include "time.h"
#include "kernel.h"

#include "../net/if.h"
#include "../net/netisr.h"
#include "../net/route.h"

#ifdef INET
#include "domain.h"
#include "../netinet/in.h"
#include "../netinet/in_systm.h"
#include "../netinet/in_var.h"
#include "../netinet/ip.h"
#include "../netinet/if_ether.h"
#endif

#ifdef NS
#include "../netns/ns.h"
#include "../netns/ns_if.h"
#endif

#include "if_qtreg.h"
#include "if_uba.h"
#include "../pdpuba/ubavar.h"
 
#define NRCV	16	 	/* Receive descriptors (must be <= 32) */
#define NXMT	6	 	/* Transmit descriptors	(must be <= 12) */
#if	NRCV > 32 || NXMT > 12
	generate an error
#endif
 
	struct	qt_uba
		{
		struct	qt_uba	*next;	/* link to next buffer in list or
					 * NULL if the last buffer
					*/
		struct	ifuba	ubabuf;	/* buffer descriptor */
		};

struct	qt_softc {
	struct	arpcom is_ac;		/* common part - must be first  */
#define	is_if	is_ac.ac_if		/* network-visible interface 	*/
#define	is_addr	is_ac.ac_enaddr		/* hardware Ethernet address 	*/
	struct	qt_uba	*freelist;	/* list of available buffers	*/
	struct	qt_uba	ifrw[NRCV + NXMT];
	u_short	initclick;		/* click addr of the INIT block */
	struct	qt_rring *rring;	/* Receive ring address 	*/
	struct	qt_tring *tring;	/* Transmit ring address 	*/
	char	r_align[QT_MAX_RCV * sizeof (struct qt_rring) + 8];
	char	t_align[QT_MAX_XMT * sizeof (struct qt_tring) + 8];
	short	qt_flags;		/* software state		*/
#define	QTF_RUNNING	0x1
#define	QTF_STARTUP	0x2		/* Waiting for start interrupt  */
	char	rindex;			/* Receive Completed Index	*/
	char	nxtrcv;			/* Next Receive Index		*/
	char	nrcv;			/* Number of Receives active	*/
	char	tindex;			/* Transmit index		*/
	char	otindex;		/* Old transmit index		*/
	char	nxmit;			/* # of xmits in progress	*/
	struct	qtdevice *addr;		/* device CSR addr		*/
} qt_softc[NQT];

struct	uba_device *qtinfo[NQT];
 
int	qtattach(), qtintr(), qtinit(), qtoutput(), qtioctl();
 
extern	struct ifnet loif;

u_short qtstd[] = { 0 };

struct	uba_driver qtdriver =
	{ 0, 0, qtattach, 0, qtstd, "qe", qtinfo };
 
/*
 * Maximum packet size needs to include 4 bytes for the CRC 
 * on received packets.
*/
#define MAXPACKETSIZE (ETHERMTU + sizeof (struct ether_header) + 4)
#define	MINPACKETSIZE 64

/*
 * The C compiler's propensity for prepending '_'s to names is the reason
 * for the hack below.  We need the "handler" address (the code which
 * sets up the interrupt stack frame) in order to initialize the vector.
*/

static int qtfoo()
	{
	asm("mov $qtintr, r0");		/* return value is in r0 */
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

qtattach(ui)
	struct uba_device *ui;
	{
	register struct qt_softc *sc = &qt_softc[ui->ui_unit];
	register struct ifnet *ifp = &sc->is_if;
	register struct qt_init *iniblk = (struct qt_init *)SEG5;
	segm	seg5;
	long	bufaddr;
	extern int nextiv();
 
	ifp->if_unit = ui->ui_unit;
	ifp->if_name = "qe";
	ifp->if_mtu = ETHERMTU;
	ifp->if_flags = IFF_BROADCAST;
 
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
	sc->addr = (struct qtdevice *)ui->ui_addr;
	sc->initclick = MALLOC(coremap, btoc(sizeof (struct qt_init)));
	saveseg5(seg5);
	mapseg5(sc->initclick, 077406);
	bzero(iniblk, sizeof (struct qt_init));
	sc->rring = (struct qt_rring *)(((int)sc->r_align + 7) & ~7);
	sc->tring = (struct qt_tring *)(((int)sc->t_align + 7) & ~7);

/*
 * Fetch the next available interrupt vector.  The routine is in the kernel
 * (for several reasons) so use SKcall.  Then initialize the vector with
 * the address of our 'handler' and PSW of supervisor, priority 4 and unit
*/
	iniblk->vector = SKcall(nextiv, 0);
	mtkd(iniblk->vector, qtfoo());
	mtkd(iniblk->vector + 2, PSL_CURSUP | PSL_BR4 | ifp->if_unit);

	iniblk->options = INIT_OPTIONS_INT;
	bufaddr = startnet + (long)sc->rring;
	iniblk->rx_lo = loint(bufaddr);
	iniblk->rx_hi = hiint(bufaddr);
	bufaddr = startnet + (long)sc->tring;
	iniblk->tx_lo = loint(bufaddr);
	iniblk->tx_hi = hiint(bufaddr);
	restorseg5(seg5);

/*
 * Now allocate the buffers and initialize the buffers.  This should _never_
 * fail because main memory is allocated after the DMA pool is used up.
*/

	if	(!qbaini(sc, NRCV + NXMT))
		return;		/* XXX */
 
	ifp->if_init = qtinit;
	ifp->if_output = qtoutput;
	ifp->if_ioctl = qtioctl;
	ifp->if_reset = 0;
	if	(qtturbo(sc))
		if_attach(ifp);
	}
 
qtturbo(sc)
	register struct qt_softc *sc;
	{
	register int i;
	register struct qtdevice *addr = sc->addr;
	struct	qt_init *iniblk = (struct qt_init *)SEG5;
	segm	seg5;

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
		printf("qt@%o !-YM\n", addr);
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
		printf("qt@%o !Turbo\n", addr);
		return(0);
		}
/*
 * Board has entered Turbo mode.  Now copy the physical address from the
 * ROMs to the INIT block.  Fill in the address in the part of the structure 
 * "visible" to the rest of the system.
*/
	saveseg5(seg5);
	mapseg5(sc->initclick, 077406);
	sc->is_addr[0] = addr->sarom[0];
	sc->is_addr[1] = addr->sarom[2];
	sc->is_addr[2] = addr->sarom[4];
	sc->is_addr[3] = addr->sarom[6];
	sc->is_addr[4] = addr->sarom[8];
	sc->is_addr[5] = addr->sarom[10];
	bcopy(sc->is_addr, iniblk->paddr, 6);
	restorseg5(seg5);
	return(1);
	}

qtinit(unit)
	int unit;
	{
	int	s;
	register struct qt_softc *sc = &qt_softc[unit];
	struct qtdevice *addr = sc->addr;
	struct	ifnet *ifp = &sc->is_if;
	struct	qt_rring *rp;
	struct	qt_tring *tp;
	register struct	qt_uba	*xp;
	register int i;
	long	buf_adr;
 
	if	(!ifp->if_addrlist)		/* oops! */
		return;
/*
 * Now initialize the receive ring descriptors.  Because this routine can be
 * called with outstanding I/O operations we check the ring descriptors for
 * a non-zero 'rhost0' (or 'thost0') word and place those buffers back on
 * the free list.
*/
	for	(i = 0, rp = sc->rring; i < QT_MAX_RCV; i++, rp++)
		{
		rp->rmd3 = RMD3_OWN;
		if	(xp = rp->rhost0)
			{
			rp->rhost0 = 0;
			xp->next = sc->freelist;
			sc->freelist = xp;
			}
		}
	for	(i = 0, tp = sc->tring ; i < QT_MAX_XMT; i++, tp++)
		{
		sc->tring[i].tmd3 = TMD3_OWN;
		if	(xp = tp->thost0)
			{
			tp->thost0 = 0;
			xp->next = sc->freelist;
			sc->freelist = xp;
			}
		}
 
	sc->nxmit = 0;
	sc->otindex = 0;
	sc->tindex = 0;
	sc->rindex = 0;
	sc->nxtrcv = 0;
	sc->nrcv = 0;
 
	s = splimp();
/*
 * Now we tell the device the address of the INIT block.  The device
 * _must_ be in the Turbo mode at this time.  The "START" command is
 * then issued to the device.  A 1 second timeout is then started.
 * When the interrupt occurs the IFF_UP|IFF_RUNNING state is entered and
 * full operations will proceed.  If the timeout expires without an interrupt 
 * being received an error is printed, the flags cleared and the device left
 * marked down.
*/
	buf_adr = ctob((long)sc->initclick);
	addr->ibal = loint(buf_adr);
	addr->ibah = hiint(buf_adr);
	addr->srqr = 2;
/*
 * set internal state to 'startup' and start a one second timer.  the interrupt
 * service routine will be entered either 1) when the device posts the 'start'
 * interrupt or 2) when the timer expires.  The interrupt routine will fill
 * the receive rings, etc.
*/
	sc->qt_flags = QTF_STARTUP;
	TIMEOUT(qtintr, unit, 60);
	splx(s);
	}

/*
 * Start output on interface.
 */

qtstart(unit)
	int	unit;
	{
	int 	len, s;
	register struct qt_softc *sc = &qt_softc[unit];
	register struct qt_tring *rp;
	struct	mbuf *m;
	long	buf_addr;
	register struct	qt_uba *xp;
 
	s = splimp();
	while	(sc->nxmit < NXMT)
		{
		IF_DEQUEUE(&sc->is_if.if_snd, m);
		if	(m == 0)
			break;
		rp = &sc->tring[sc->tindex];
#ifdef	QTDEBUG
		if	((rp->tmd3 & TMD3_OWN) == 0)
			printf("qt xmit in progress\n");
#endif
/*
 * Now pull a buffer off of the freelist.  Guaranteed to be a buffer
 * because both the receive and transmit sides limit themselves to
 * NRCV and NXMT buffers respectively.
*/
		xp = sc->freelist;
		sc->freelist = xp->next;

		buf_addr = xp->ubabuf.ifu_w.ifrw_info;
		len = if_wubaput(&xp->ubabuf, m);
		if	(len < MINPACKETSIZE)
			len = MINPACKETSIZE;
		rp->tmd4 = loint(buf_addr);
		rp->tmd5 = hiint(buf_addr) & TMD5_HADR;
		rp->tmd3 = len & TMD3_BCT;	/* set length,clear ownership */
		rp->thost0 = xp;		/* set entry active */
		sc->addr->arqr = ARQR_TRQ;	/* tell device it has buffer */
		sc->nxmit++;
		if	(++sc->tindex >= QT_MAX_XMT)
			sc->tindex = 0;
		}
	splx(s);
	}
 
/*
 * General interrupt service routine.  Receive, transmit, device start 
 * interrupts and timeouts come here.  Check for hard device errors and print a
 * message if any errors are found.  If we are waiting for the device to 
 * START then check if the device is now running.
*/

qtintr(unit)
	int unit;
	{
	register struct qt_softc *sc = &qt_softc[unit];
	register int status;
	int	s;
 
	status = sc->addr->srr;
	if	(status < 0)
		/* should we reset the device after a bunch of these errs? */
		qtsrr(unit, status);
	if	(sc->qt_flags == QTF_STARTUP)
		{
		if	((status & SRR_RESP) == 2)
			{
			sc->qt_flags = QTF_RUNNING;
			sc->is_if.if_flags |= (IFF_UP | IFF_RUNNING);
			}
		else
			printf("qt%d !start\n", unit);
		}
	s = splimp();
	qtrint(unit);
	if	(sc->nxmit)
		qttint(unit);
	qtstart(unit);
	splx(s);
	}
 
/*
 * Transmit interrupt service.  Only called if there are outstanding transmit
 * requests which could have completed.  The DELQA-YM doesn't provide the
 * status bits telling the kind (receive, transmit) of interrupt.
*/
 
#define BBLMIS (TMD2_BBL|TMD2_MIS)

qttint(unit)
	int unit;
	{
	register struct qt_softc *sc = &qt_softc[unit];
	register struct qt_tring *rp;
	register struct	qt_uba *xp;
 
	while	(sc->nxmit > 0)
		{
		rp = &sc->tring[sc->otindex];
		if	((rp->tmd3 & TMD3_OWN) == 0)
			break;
/*
 * Now check the buffer address (the first word in the ring descriptor
 * available for the host's use).  If it is NULL then we have already seen
 * and processed (or never presented to the board in the first place) this 
 * entry and the ring descriptor should not be counted.
*/
		xp = rp->thost0;
		if	(xp == 0)
			break;
/*
 * Clear the buffer address from the ring descriptor and put the
 * buffer back on the freelist for future use.
*/
		rp->thost0 = 0;
		xp->next = sc->freelist;
		sc->freelist = xp;

		sc->nxmit--;
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
			printf("qt%d tmd2 %b\n", unit, rp->tmd2, TMD2_BITS);
#endif
			sc->is_if.if_oerrors++;
			}
		if	(++sc->otindex >= QT_MAX_XMT)
			sc->otindex = 0;
		}
	}
 
/*
 * Receive interrupt service.  Pull packet off the interface and put into
 * a mbuf chain for processing later.
*/

qtrint(unit)
	int unit;
	{
	register struct qt_softc *sc = &qt_softc[unit];
	register struct qt_rring *rp;
	register struct	qt_uba *xp;
	int	len;
	long	bufaddr;
 
	while	(sc->rring[sc->rindex].rmd3 & RMD3_OWN)
		{
		rp = &sc->rring[sc->rindex];
/*
 * If the host word is 0 then this is a buffer either already seen or not
 * presented to the device in the first place.
*/
		xp = rp->rhost0;
		if	(xp == 0)
			break;

		if     ((rp->rmd0 & (RMD0_STP|RMD0_ENP)) != (RMD0_STP|RMD0_ENP))
			{
			printf("qt%d chained packet\n", unit);
			sc->is_if.if_ierrors++;
			goto rnext;
			}
		len = (rp->rmd1 & RMD1_MCNT) - 4;	/* -4 for CRC */
		sc->is_if.if_ipackets++;
 
		if	((rp->rmd0 & RMD0_ERR3) || (rp->rmd2 & RMD2_ERR4))
			{
			sc->is_if.if_ierrors++;
#ifdef QTDEBUG
			printf("qt%d rmd0 %b\n", unit, rp->rmd0, RMD0_BITS);
			printf("qt%d rmd2 %b\n", unit, rp->rmd2, RMD2_BITS);
#endif
			}
		else
			qtread(sc, &xp->ubabuf,
				len - sizeof (struct ether_header));
rnext:
		--sc->nrcv;
		if	(++sc->rindex >= QT_MAX_RCV)
			sc->rindex = 0;
/*
 * put the buffer back on the free list and clear the first host word
 * in the ring descriptor so we don't process this one again.
*/
		xp->next = sc->freelist;
		sc->freelist = xp;
		rp->rhost0 = 0;
		}
	while	(sc->nrcv < NRCV)
		{
		rp = &sc->rring[sc->nxtrcv];
#ifdef	QTDEBUG
		if	((rp->rmd3 & RMD3_OWN) == 0)
			printf("qtrint: !OWN\n");
#endif
		xp = sc->freelist;
		sc->freelist = xp->next;
		bufaddr = xp->ubabuf.ifu_r.ifrw_info;
		rp->rmd1 = MAXPACKETSIZE;
		rp->rmd4 = loint(bufaddr);
		rp->rmd5 = hiint(bufaddr);
		rp->rhost0 = xp;
		rp->rmd3 = 0;			/* clear RMD3_OWN */
		++sc->nrcv;
		sc->addr->arqr = ARQR_RRQ;	/* tell device it has buffer */
		if	(++sc->nxtrcv >= QT_MAX_RCV)
			sc->nxtrcv = 0;
		}
	}

/*
 * Place data on the appropriate queue and call the start routine to
 * send the data to the device.
*/

qtoutput(ifp, m0, dst)
	struct	ifnet *ifp;
	struct	mbuf *m0;
	struct	sockaddr *dst;
	{
	int	type, s, trail;
	u_char	edst[6];
	struct	in_addr idst;
	register struct ether_header *eh;
	register struct qt_softc *is = &qt_softc[ifp->if_unit];
	register struct mbuf *m = m0;
	struct	mbuf *mcopy = (struct mbuf *)0;
 
	if	((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		{
		m_freem(m0);
		return(ENETDOWN);
		}
	switch	(dst->sa_family)
		{
#ifdef INET
		case	AF_INET:
			idst = ((struct sockaddr_in *)dst)->sin_addr;
			if	(!arpresolve(&is->is_ac, m, &idst, edst,&trail))
				return(0);	/* wait for arp to finish */
			if	(!bcmp(edst, etherbroadcastaddr,sizeof (edst)))
				mcopy = m_copy(m, 0, (int)M_COPYALL);
			type = ETHERTYPE_IP;
			break;
#endif
#ifdef NS
		case	AF_NS:
			type = ETHERTYPE_NS;
			bcopy(&(((struct sockaddr_ns *)dst)->sns_addr.x_host),
		    		edst, sizeof (edst));
			if	(!bcmp(edst, &ns_broadcast, sizeof (edst)))
				return(looutput(&loif, m, dst));
			break;
#endif
		case	AF_UNSPEC:
			eh = (struct ether_header *)dst->sa_data;
			bcopy(eh->ether_dhost, (caddr_t)edst, sizeof (edst));
			type = eh->ether_type;
			break;
		default:
			printf("qt%d can't handle af%d\n", ifp->if_unit,
				dst->sa_family);
			m_freem(m);
			return(EAFNOSUPPORT);
		}
/*
 * Add local net header.  If no space in first mbuf, allocate another.
*/
	if	(m->m_off > MMAXOFF ||
			MMINOFF + sizeof (struct ether_header) > m->m_off)
		{
		m = m_get(M_DONTWAIT, MT_HEADER);
		if	(m == 0)
			{
			m_freem(m0);
nobufs:			if	(mcopy)
				m_freem(mcopy);
			return(ENOBUFS);
			}
		m->m_next = m0;
		m->m_off = MMINOFF;
		m->m_len = sizeof (struct ether_header);
		}
	else
		{
		m->m_off -= sizeof (struct ether_header);
		m->m_len += sizeof (struct ether_header);
		}
	eh = mtod(m, struct ether_header *);
	eh->ether_type = htons((u_short)type);
 	bcopy(edst, (caddr_t)eh->ether_dhost, sizeof (edst));
 	bcopy(is->is_addr, (caddr_t)eh->ether_shost, sizeof (is->is_addr));
 
	s = splimp();
	if	(IF_QFULL(&ifp->if_snd))
		{
		IF_DROP(&ifp->if_snd);
		m_freem(m);
		splx(s);
		goto nobufs;
		}
	IF_ENQUEUE(&ifp->if_snd, m);
	qtstart(ifp->if_unit);
	splx(s);
	return(mcopy ? looutput(&loif, mcopy, dst) : 0);
	}
 
qtioctl(ifp, cmd, data)
	register struct ifnet *ifp;
	int	cmd;
	caddr_t	data;
	{
	struct qt_softc *sc = &qt_softc[ifp->if_unit];
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s = splimp(), error = 0;
#ifdef	NS
	register struct ns_addr *ina = &(IA_SNS(ifa)->sns_addr);
#endif
 
	switch	(cmd)
		{
		case	SIOCSIFADDR:
/*
 * Resetting the board is probably overkill, but then again this is only
 * done when the system comes up or possibly when a reset is needed after a
 * major network fault (open wire, etc).
*/
			qtrestart(sc);
			switch	(ifa->ifa_addr.sa_family)
				{
#ifdef INET
				case	AF_INET:
					((struct arpcom *)ifp)->ac_ipaddr =
						IA_SIN(ifa)->sin_addr;
					arpwhohas(ifp, &IA_SIN(ifa)->sin_addr);
					break;
#endif
#ifdef NS
				case	AF_NS:
					if	(ns_nullhost(*ina))
						ina->x_host = 
						*(union ns_host *)(sc->is_addr);
					else
						{
						qt_ns(ina->x_host.c_host);
						qtrestart(sc);
						}
					break;
#endif
				}
			break;
		case	SIOCSIFFLAGS:
			if	((ifp->if_flags & IFF_UP) == 0 &&
				    sc->qt_flags & QTF_RUNNING)
				{
/*
 * We've been asked to stop the board and leave it that way.  qtturbo()
 * does this with the side effect of placing the device back in Turbo mode.
*/
				qtturbo(sc);
				sc->qt_flags &= ~QTF_RUNNING;
				}
			else if (ifp->if_flags & IFF_UP &&
					!(sc->qt_flags & QTF_RUNNING))
				qtrestart(sc);
			break;
		default:
			error = EINVAL;
		}
	splx(s);
	return(error);
	}
 
#ifdef	NS
qt_ns(cp)
	register char *cp;
	{
	segm	seg5;
	register struct qt_init *iniblk = (struct qt_init *)SEG5;

	saveseg5(seg5);
	mapseg5(sc->initclick, 077406);
	bcopy(cp, sc->is_addr, 6);
	bcopy(cp, iniblk->paddr, 6);
	restorseg5(seg5);
	}
#endif

/*
 * Pull the data off of the board and pass back to the upper layers of
 * the networking code.  Trailers are counted as errors and the packet
 * dropped.  SEG5 is saved and restored (used to peek at the packet type).
*/

qtread(sc, ifuba, len)
	register struct qt_softc *sc;
	struct	ifuba *ifuba;
	int	len;
	{
	struct	ether_header *eh;
    	register struct	mbuf *m;
	struct	ifqueue *inq;
	int	type;
	segm	seg5;
 
	saveseg5(seg5);
	mapseg5(ifuba->ifu_r.ifrw_click, 077406);
	eh = (struct ether_header *)SEG5;
	eh->ether_type = ntohs((u_short)eh->ether_type);
	type = eh->ether_type;
	restorseg5(seg5);
	if	(len == 0 || type >= ETHERTYPE_TRAIL &&
			type < ETHERTYPE_TRAIL+ETHERTYPE_NTRAILER)
		{
		sc->is_if.if_ierrors++;
		return;
		}

	m = if_rubaget(ifuba, len, 0, &sc->is_if);
	if	(m == 0)
		return;
 
	switch	(type)
		{
#ifdef INET
		case	ETHERTYPE_IP:
			schednetisr(NETISR_IP);
			inq = &ipintrq;
			break;
		case	ETHERTYPE_ARP:
			arpinput(&sc->is_ac, m);
			return;
#endif
#ifdef NS
		case	ETHERTYPE_NS:
			schednetisr(NETISR_NS);
			inq = &nsintrq;
			break;
#endif
		default:
			m_freem(m);
			return;
		}
 
	if	(IF_QFULL(inq))
		{
		IF_DROP(inq);
		m_freem(m);
		return;
		}
	IF_ENQUEUE(inq, m);
	}


qtsrr(unit, srrbits)
	int	unit, srrbits;
	{
	printf("qt%d srr=%b\n", unit, srrbits, SRR_BITS);
	}

/*
 * Reset the device.  This moves it from DELQA-T mode to DELQA-Normal mode.
 * After the reset put the device back in -T mode.  Then call qtinit() to
 * reinitialize the ring structures and issue the 'timeout' for the "device
 * started interrupt".
*/

qtrestart(sc)
	register struct qt_softc *sc;
	{
 
	qtturbo(sc);
	qtinit(sc - qt_softc);
	}

qbaini(sc, num)
	struct	qt_softc *sc;
	int num;
	{
	register int i;
	register memaddr click;
	struct	qt_uba	*xp;
	register struct	ifuba	*ifuba;

	for	(i = 0; i < num; i++)
		{
		xp = &sc->ifrw[i];
		ifuba = &xp->ubabuf;
		click = m_ioget(MAXPACKETSIZE);
		if	(click == 0)
			{
			click = MALLOC(coremap, btoc(MAXPACKETSIZE));
			if	(click == 0)
				return(0);	/* _can't_ happen */
			}
		ifuba->ifu_hlen = sizeof (struct ether_header);
		ifuba->ifu_w.ifrw_click = ifuba->ifu_r.ifrw_click = click;
		ifuba->ifu_w.ifrw_info = ifuba->ifu_r.ifrw_info = 
			ctob((long)click);
		xp->next = sc->freelist;
		sc->freelist = xp;
		}
	return(1);
	}
#endif
