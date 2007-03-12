/*
 * Copyright (c) 1997, 2000 Hellmuth Michaelis. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *---------------------------------------------------------------------------
 *
 *	i4b_ipr.c - isdn4bsd IP over raw HDLC ISDN network driver
 *	---------------------------------------------------------
 *
 *	$Id: i4b_ipr.c,v 1.24.4.2 2007/03/12 06:00:12 rmind Exp $
 *
 * $FreeBSD$
 *
 *	last edit-date: [Fri Jan  5 11:33:47 2001]
 *
 *---------------------------------------------------------------------------*
 *
 *	statistics counter usage (interface lifetime):
 *	----------------------------------------------
 *	sc->sc_if.if_ipackets	# of received packets
 *	sc->sc_if.if_ierrors	# of error packets not going to upper layers
 *	sc->sc_if.if_opackets	# of transmitted packets
 *	sc->sc_if.if_oerrors	# of error packets not being transmitted
 *	sc->sc_if.if_collisions	# of invalid ip packets after VJ decompression
 *	sc->sc_if.if_ibytes	# of bytes coming in from the line (before VJ)
 *	sc->sc_if.if_obytes	# of bytes going out to the line (after VJ)
 *	sc->sc_if.if_imcasts	  (currently unused)
 *	sc->sc_if.if_omcasts	# of frames sent out of the fastqueue
 *	sc->sc_if.if_iqdrops	# of frames dropped on input because queue full
 *	sc->sc_if.if_noproto	# of frames dropped on output because !AF_INET
 *
 *	statistics counter usage (connection lifetime):
 *	-----------------------------------------------
 *	sc->sc_iinb		# of ISDN incoming bytes from HSCX
 *	sc->sc_ioutb		# of ISDN outgoing bytes from HSCX
 *	sc->sc_inb		# of incoming bytes after decompression
 *	sc->sc_outb		# of outgoing bytes before compression
 *
 *---------------------------------------------------------------------------*/

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: i4b_ipr.c,v 1.24.4.2 2007/03/12 06:00:12 rmind Exp $");

#include "irip.h"
#include "opt_irip.h"

#if NIRIP > 0

#ifdef __FreeBSD__
#include "opt_i4b.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/errno.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/ioccom.h>
#include <sys/sockio.h>
#ifdef IPR_VJ
#include <sys/malloc.h>
#endif
#else
#include <sys/ioctl.h>
#endif

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
#include <sys/callout.h>
#endif

#include <sys/kernel.h>
#include <sys/protosw.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>

#ifdef IRIP_VJ
#include <net/slcompress.h>
#define IPR_COMPRESS IFF_LINK0  /* compress TCP traffic */
#define IPR_AUTOCOMP IFF_LINK1  /* auto-enable TCP compression */

/*---------------------------------------------------------------------------
 * NOTICE: using NO separate buffer relies on the assumption, that the HSCX
 * IRQ handler _always_ allocates a single, continuous mbuf cluster large
 * enough to hold the maximum MTU size if the ipr interface !
 *
 * CAUTION: i have re-defined IPR_VJ_USEBUFFER because it makes problems
 *          with 2 i4b's back to back running cvs over ssh, cvs simply
 *          aborts because it gets bad data. Everything else (telnet/ftp?etc)
 *          functions fine.
 *---------------------------------------------------------------------------*/
#define IPR_VJ_USEBUFFER	/* define to use an allocated separate buffer*/
				/* undef to uncompress in the mbuf itself    */
#endif /* IRIP_VJ */

#if defined(__FreeBSD_version) &&  __FreeBSD_version >= 400008
#include "bpf.h"
#else
#include "bpfilter.h"
#endif
#if NBPFILTER > 0 || NBPF > 0
#include <sys/time.h>
#include <net/bpf.h>
#endif

#ifdef __FreeBSD__
#include <machine/i4b_ioctl.h>
#include <machine/i4b_debug.h>
#else
#include <netisdn/i4b_debug.h>
#include <netisdn/i4b_ioctl.h>
#endif

#include <netisdn/i4b_global.h>
#include <netisdn/i4b_mbuf.h>
#include <netisdn/i4b_l3l4.h>

#include <netisdn/i4b_l4.h>

#ifndef __FreeBSD__
#include <machine/cpu.h> /* For softnet */
#endif

#ifdef __FreeBSD__
#define IPR_FMT	"irip%d: "
#define	IPR_ARG(sc)	((sc)->sc_if.if_unit)
#define	PDEVSTATIC	static
#elif defined(__bsdi__)
#define IPR_FMT	"irip%d: "
#define	IPR_ARG(sc)	((sc)->sc_if.if_unit)
#define	PDEVSTATIC	/* not static */
#else
#define	IPR_FMT	"%s: "
#define	IPR_ARG(sc)	((sc)->sc_if.if_xname)
#define	PDEVSTATIC	/* not static */
#endif

#define I4BIPRMTU	1500		/* regular MTU */
#define I4BIPRMAXMTU	2000		/* max MTU */
#define I4BIPRMINMTU	500		/* min MTU */

#define I4BIPRMAXQLEN	50		/* max queue length */

#define I4BIPRACCT	1		/* enable accounting messages */
#define	I4BIPRACCTINTVL	2		/* accounting msg interval in secs */
#define I4BIPRADJFRXP	1		/* adjust 1st rxd packet */


struct ipr_softc {
	struct ifnet	sc_if;		/* network-visible interface	*/
	int		sc_state;	/* state of the interface	*/

	call_desc_t	*sc_cdp;	/* ptr to call descriptor	*/
	isdn_link_t	*sc_ilt;	/* ptr to B channel driver/state */

	int		sc_unit;	/* which instance are we?	*/
	int		sc_updown;	/* soft state of interface	*/
	struct ifqueue  sc_fastq;	/* interactive traffic		*/
	int		sc_dialresp;	/* dialresponse			*/
	int		sc_lastdialresp;/* last dialresponse		*/

#if I4BIPRACCT
	int		sc_iinb;	/* isdn driver # of inbytes	*/
	int		sc_ioutb;	/* isdn driver # of outbytes	*/
	int		sc_inb;		/* # of bytes rx'd		*/
	int		sc_outb;	/* # of bytes tx'd	 	*/
	int		sc_linb;	/* last # of bytes rx'd		*/
	int		sc_loutb;	/* last # of bytes tx'd 	*/
	int		sc_fn;		/* flag, first null acct	*/
#endif

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
	struct callout	sc_callout;
#endif
#if defined(__FreeBSD__)
	struct callout_handle	sc_callout;
#endif

#ifdef I4BIPRADJFRXP
	int		sc_first_pkt;	/* flag, first rxd packet	*/
#endif
#if IPR_LOG
	int		sc_log_first;	/* log first n packets          */
#endif

#ifdef IRIP_VJ
	struct slcompress sc_compr;	/* tcp compression data		*/
#ifdef IPR_VJ_USEBUFFER
	u_char		*sc_cbuf;	/* tcp decompression buffer	*/
#endif
#endif

} ipr_softc[NIRIP];

enum ipr_states {
	ST_IDLE,			/* initialized, ready, idle	*/
	ST_DIALING,			/* dialling out to remote	*/
	ST_CONNECTED_W,			/* connected to remote		*/
	ST_CONNECTED_A,			/* connected to remote		*/
};

#if defined(__FreeBSD__) || defined(__bsdi__)
#define	THE_UNIT	sc->sc_if.if_unit
#else
#define	THE_UNIT	sc->sc_unit
#endif

#ifdef __FreeBSD__
#if defined(__FreeBSD_version) && __FreeBSD_version >= 300001
#  define IOCTL_CMD_T u_long
#else
#ifdef __NetBSD__
#  define IOCTL_CMD_T u_long
#else
#  define IOCTL_CMD_T int
#endif
#endif
PDEVSTATIC void iripattach(void *);
PSEUDO_SET(iripattach, i4b_ipr);
static int irpioctl(struct ifnet *ifp, IOCTL_CMD_T cmd, void *data);
#else
PDEVSTATIC void iripattach __P((void));
static int iripioctl(struct ifnet *ifp, u_long cmd, void *data);
#endif

#ifdef __bsdi__
static int iprwatchdog(int unit);
#else
static void iprwatchdog(struct ifnet *ifp);
#endif
static void ipr_tx_queue_empty(void *);
static int iripoutput(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst, struct rtentry *rtp);
static void iripclearqueues(struct ipr_softc *sc);
static void ipr_set_linktab(void *softc, isdn_link_t *ilt);
static void ipr_activity(void *softc, int rxtx);
static void ipr_rx_data_rdy(void *softc);
static void ipr_disconnect(void *softc, void *cdp);
static void ipr_connect(void *softc, void *cdp);
static void ipr_dialresponse(void *softc, int status, cause_t cause);
static void ipr_updown(void *softc, int updown);
static void* ipr_get_softc(int unit);

static const struct isdn_l4_driver_functions
ipr_l4_functions = {
	ipr_rx_data_rdy,
	ipr_tx_queue_empty,
	ipr_activity,
	ipr_connect,
	ipr_disconnect,
	ipr_dialresponse,
	ipr_updown,
	ipr_get_softc,
	ipr_set_linktab,
	NULL
};

static int irip_driver_id = -1;

/*===========================================================================*
 *			DEVICE DRIVER ROUTINES
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 *	interface attach routine at kernel boot time
 *---------------------------------------------------------------------------*/
PDEVSTATIC void
#ifdef __FreeBSD__
iripattach(void *dummy)
#else
iripattach()
#endif
{
	struct ipr_softc *sc = ipr_softc;
	int i;

	irip_driver_id = isdn_l4_driver_attach("irip", NIRIP, &ipr_l4_functions);

	for(i=0; i < NIRIP; sc++, i++)
	{
		NDBGL4(L4_DIALST, "setting dial state to ST_IDLE");

		sc->sc_state = ST_IDLE;
		sc->sc_unit = i;

#ifdef __FreeBSD__
		sc->sc_if.if_name = "irip";
#if __FreeBSD__ < 3
		sc->sc_if.if_next = NULL;
#endif
		sc->sc_if.if_unit = i;
#elif defined(__bsdi__)
		sc->sc_if.if_name = "irip";
		sc->sc_if.if_unit = i;
#else
		snprintf(sc->sc_if.if_xname, sizeof(sc->sc_if.if_xname),
		    "irip%d", i);
		sc->sc_if.if_softc = sc;
#endif

#ifdef	IRIP_VJ
		sc->sc_if.if_flags = IFF_POINTOPOINT | IFF_SIMPLEX | IPR_AUTOCOMP;
#else
		sc->sc_if.if_flags = IFF_POINTOPOINT | IFF_SIMPLEX;
#endif

#if defined(__NetBSD__) && __NetBSD_Version__ >= 104230000
		callout_init(&sc->sc_callout);
#endif

		sc->sc_if.if_mtu = I4BIPRMTU;
		sc->sc_if.if_type = IFT_ISDNBASIC;
		sc->sc_if.if_ioctl = iripioctl;
		sc->sc_if.if_output = iripoutput;

		IFQ_SET_MAXLEN(&sc->sc_if.if_snd, I4BIPRMAXQLEN);
		sc->sc_fastq.ifq_maxlen = I4BIPRMAXQLEN;
		IFQ_SET_READY(&sc->sc_if.if_snd);

		sc->sc_if.if_ipackets = 0;
		sc->sc_if.if_ierrors = 0;
		sc->sc_if.if_opackets = 0;
		sc->sc_if.if_oerrors = 0;
		sc->sc_if.if_collisions = 0;
		sc->sc_if.if_ibytes = 0;
		sc->sc_if.if_obytes = 0;
		sc->sc_if.if_imcasts = 0;
		sc->sc_if.if_omcasts = 0;
		sc->sc_if.if_iqdrops = 0;
		sc->sc_if.if_noproto = 0;

#if I4BIPRACCT
		sc->sc_if.if_timer = 0;
		sc->sc_if.if_watchdog = iprwatchdog;
		sc->sc_iinb = 0;
		sc->sc_ioutb = 0;
		sc->sc_inb = 0;
		sc->sc_outb = 0;
		sc->sc_linb = 0;
		sc->sc_loutb = 0;
		sc->sc_fn = 1;
#endif
#if IPR_LOG
		sc->sc_log_first = IPR_LOG;
#endif

#ifdef	IRIP_VJ
#ifdef __FreeBSD__
		sl_compress_init(&sc->sc_compr, -1);
#else
		sl_compress_init(&sc->sc_compr);
#endif

#ifdef IPR_VJ_USEBUFFER
		if(!((sc->sc_cbuf =
		   (u_char *)malloc(I4BIPRMAXMTU+128, M_DEVBUF, M_WAITOK))))
		{
			panic("if_ipr.c, ipr_attach: VJ malloc failed");
		}
#endif
#endif

		sc->sc_updown = SOFT_ENA;	/* soft enabled */

		sc->sc_dialresp = DSTAT_NONE;	/* no response */
		sc->sc_lastdialresp = DSTAT_NONE;

#if defined(__FreeBSD_version) && ((__FreeBSD_version >= 500009) || (410000 <= __FreeBSD_version && __FreeBSD_version < 500000))
		/* do not call bpfattach in ether_ifattach */
		ether_ifattach(&sc->sc_if, 0);
#else
		if_attach(&sc->sc_if);
		if_alloc_sadl(&sc->sc_if);
#endif

#if NBPFILTER > 0 || NBPF > 0
#ifdef __FreeBSD__
		bpfattach(&sc->sc_if, DLT_NULL, sizeof(u_int));
#else
		bpfattach(&sc->sc_if, DLT_NULL, sizeof(u_int));
#endif
#endif
	}
}

/*---------------------------------------------------------------------------*
 *	output a packet to the ISDN B-channel
 *---------------------------------------------------------------------------*/
static int
iripoutput(struct ifnet *ifp, struct mbuf *m, const struct sockaddr *dst,
	 struct rtentry *rtp)
{
	struct ipr_softc *sc;
	int s, rv;
	struct ifqueue *ifq = NULL;
	struct ip *ip;
	ALTQ_DECL(struct altq_pktattr pktattr;)

	s = splnet();

#if defined(__FreeBSD__) || defined(__bsdi__)
	unit = ifp->if_unit;
	sc = &ipr_softc[unit];
#else
	sc = ifp->if_softc;
#endif

	/* check for IP */

	if(dst->sa_family != AF_INET)
	{
		printf(IPR_FMT "af%d not supported\n", IPR_ARG(sc), dst->sa_family);
		m_freem(m);
		splx(s);
		sc->sc_if.if_noproto++;
		sc->sc_if.if_oerrors++;
		return(EAFNOSUPPORT);
	}

	/* check interface state = UP */

	if(!(ifp->if_flags & IFF_UP))
	{
		NDBGL4(L4_IPRDBG, "%s: interface is DOWN!", sc->sc_if.if_xname);
		m_freem(m);
		splx(s);
		sc->sc_if.if_oerrors++;
		return(ENETDOWN);
	}

	/* dial if necessary */

	if(sc->sc_state == ST_IDLE || sc->sc_state == ST_DIALING)
	{

#ifdef NOTDEF
		switch(sc->sc_dialresp)
		{
			case DSTAT_TFAIL:	/* transient failure */
				NDBGL4(L4_IPRDBG, "%s: transient dial failure!", sc->sc_if.if_xname);
				m_freem(m);
				iripclearqueues(sc);
				sc->sc_dialresp = DSTAT_NONE;
				splx(s);
				sc->sc_if.if_oerrors++;
				return(ENETUNREACH);
				break;

			case DSTAT_PFAIL:	/* permanent failure */
				NDBGL4(L4_IPRDBG, "%s: permanent dial failure!", sc->sc_if.if_xname);
				m_freem(m);
				iripclearqueues(sc);
				sc->sc_dialresp = DSTAT_NONE;
				splx(s);
				sc->sc_if.if_oerrors++;
				return(EHOSTUNREACH);
				break;

			case DSTAT_INONLY:	/* no dialout allowed*/
				NDBGL4(L4_IPRDBG, "%s: dialout not allowed failure!", sc->sc_if.if_xname);
				m_freem(m);
				iripclearqueues(sc);
				sc->sc_dialresp = DSTAT_NONE;
				splx(s);
				sc->sc_if.if_oerrors++;
				return(EHOSTUNREACH);
				break;
		}
#endif

		NDBGL4(L4_IPRDBG, "%s: send dial request message!", sc->sc_if.if_xname);
		NDBGL4(L4_DIALST, "%s: setting dial state to ST_DIALING", sc->sc_if.if_xname);
		i4b_l4_dialout(irip_driver_id, sc->sc_unit);
		sc->sc_state = ST_DIALING;
	}

#if IPR_LOG
	if(sc->sc_log_first > 0)
	{
		--(sc->sc_log_first);
		i4b_l4_packet_ind(irip_driver_id, sc->sc_unit, 1, m );
	}
#endif

	/*
	 * check, if type of service indicates interactive, i.e. telnet,
	 * traffic. in case it is interactive, put it into the fast queue,
	 * else (i.e. ftp traffic) put it into the "normal" queue
	 */

	IFQ_CLASSIFY(&ifp->if_snd, m, dst->sa_family, &pktattr);

	ip = mtod(m, struct ip *);		/* get ptr to ip header */

	if(ip->ip_tos & IPTOS_LOWDELAY)
		ifq = &sc->sc_fastq;

	/* check for space in choosen send queue */

	if ((ifq != NULL)
#ifdef ALTQ
	   && ALTQ_IS_ENABLED(&sc->sc_if.if_snd) == 0
#endif
	) {
		if(IF_QFULL(ifq))
		{
			NDBGL4(L4_IPRDBG, "%s: send queue full!", sc->sc_if.if_xname);
			IF_DROP(ifq);
			m_freem(m);
			sc->sc_if.if_oerrors++;
			splx(s);
			return(ENOBUFS);
		}
		IF_ENQUEUE(ifq, m);
	} else {
		IFQ_ENQUEUE(&sc->sc_if.if_snd, m, &pktattr, rv);
		if (rv != 0) {
			sc->sc_if.if_oerrors++;
			splx(s);
			return rv;
		}
	}

	NDBGL4(L4_IPRDBG, "%s: added packet to send queue!", sc->sc_if.if_xname);

	ipr_tx_queue_empty(sc);

	splx(s);

	return (0);
}

/*---------------------------------------------------------------------------*
 *	process ioctl
 *---------------------------------------------------------------------------*/
#ifdef __FreeBSD__
static int
iripioctl(struct ifnet *ifp, IOCTL_CMD_T cmd, void *data)
#else
static int
iripioctl(struct ifnet *ifp, u_long cmd, void *data)
#endif
{
#if defined(__FreeBSD__) || defined(__bsdi__)
	struct ipr_softc *sc = &ipr_softc[ifp->if_unit];
#else
	struct ipr_softc *sc = ifp->if_softc;
#endif

	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int s;
	int error = 0;

	s = splnet();

	switch (cmd)
	{
		case SIOCAIFADDR:	/* add interface address */
		case SIOCSIFADDR:	/* set interface address */
		case SIOCSIFDSTADDR:	/* set interface destination address */
			if(ifa->ifa_addr->sa_family != AF_INET)
				error = EAFNOSUPPORT;
			else
				sc->sc_if.if_flags |= IFF_UP;
			break;

		case SIOCSIFFLAGS:	/* set interface flags */
			if(!(ifr->ifr_flags & IFF_UP))
			{
				if(sc->sc_if.if_flags & IFF_RUNNING)
				{
					/* disconnect ISDN line */
					i4b_l4_drvrdisc(sc->sc_cdp->cdid);
					sc->sc_if.if_flags &= ~IFF_RUNNING;
				}

				sc->sc_state = ST_IDLE;

				/* empty queues */

				iripclearqueues(sc);
			}

			if(ifr->ifr_flags & IFF_DEBUG)
			{
				/* enable debug messages */
			}

			break;

#if !defined(__OpenBSD__)
		case SIOCSIFMTU:	/* set interface MTU */
			if(ifr->ifr_mtu > I4BIPRMAXMTU)
				error = EINVAL;
			else if(ifr->ifr_mtu < I4BIPRMINMTU)
				error = EINVAL;
			else
				ifp->if_mtu = ifr->ifr_mtu;
			break;
#endif /* __OPENBSD__ */

#if 0
	/* not needed for FreeBSD, done in sl_compress_init() (-hm) */

			/* need to add an ioctl:	set VJ max slot ID
			 * #define IPRIOCSMAXCID	_IOW('I', XXX, int)
			 */
#ifdef IPR_VJ
		case IPRIOCSMAXCID:
			{
#if defined(__FreeBSD_version) && __FreeBSD_version >= 400005
			struct proc *p = curproc;	/* XXX */

			if((error = suser(p)) != 0)
#else
			struct lwp *l = curlwp;		/* XXX */

			if((error = kauth_authorize_network(l->l_cred,
			    KAUTH_NETWORK_INTERFACE,
			    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp,
			    (void *)cmd, NULL)) != 0)
#endif
				break;
		        sl_compress_setup(sc->sc_compr, *(int *)data);
			}
			break;
#endif
#endif
		default:
			error = EINVAL;
			break;
	}

	splx(s);

	return(error);
}

/*---------------------------------------------------------------------------*
 *	clear the interface's send queues
 *---------------------------------------------------------------------------*/
static void
iripclearqueues(struct ipr_softc *sc)
{
	int x;
	struct mbuf *m;

	for(;;)
	{
		x = splnet();
		IF_DEQUEUE(&sc->sc_fastq, m);
		splx(x);

		if(m)
			m_freem(m);
		else
			break;
	}

	for(;;)
	{
		x = splnet();
		IFQ_DEQUEUE(&sc->sc_if.if_snd, m);
		splx(x);

		if(m)
			m_freem(m);
		else
			break;
	}
}

#if I4BIPRACCT
/*---------------------------------------------------------------------------*
 *	watchdog routine
 *---------------------------------------------------------------------------*/
#ifdef __bsdi__
static int
iprwatchdog(int unit)
{
#else
static void
iprwatchdog(struct ifnet *ifp)
{
#endif
#ifdef __FreeBSD__
	int unit = ifp->if_unit;
	struct ipr_softc *sc = &ipr_softc[unit];
#elif defined(__bsdi__)
	struct ipr_softc *sc = &ipr_softc[unit];
	struct ifnet *ifp = &ipr_softc[unit].sc_if;
#else
	struct ipr_softc *sc = ifp->if_softc;
#endif
	bchan_statistics_t bs;

	/* get # of bytes in and out from the HSCX driver */

	(*sc->sc_ilt->bchannel_driver->bch_stat)
		(sc->sc_ilt->l1token, sc->sc_ilt->channel, &bs);

	sc->sc_ioutb += bs.outbytes;
	sc->sc_iinb += bs.inbytes;

	if((sc->sc_iinb != sc->sc_linb) || (sc->sc_ioutb != sc->sc_loutb) || sc->sc_fn)
	{
		int ri = (sc->sc_iinb - sc->sc_linb)/I4BIPRACCTINTVL;
		int ro = (sc->sc_ioutb - sc->sc_loutb)/I4BIPRACCTINTVL;

		if((sc->sc_iinb == sc->sc_linb) && (sc->sc_ioutb == sc->sc_loutb))
			sc->sc_fn = 0;
		else
			sc->sc_fn = 1;

		sc->sc_linb = sc->sc_iinb;
		sc->sc_loutb = sc->sc_ioutb;

		if (sc->sc_cdp)
			i4b_l4_accounting(sc->sc_cdp->cdid, ACCT_DURING,
			    sc->sc_ioutb, sc->sc_iinb, ro, ri, sc->sc_outb, sc->sc_inb);
 	}
	sc->sc_if.if_timer = I4BIPRACCTINTVL;
#ifdef __bsdi__
	return 0;
#endif
}
#endif /* I4BIPRACCT */

/*===========================================================================*
 *			ISDN INTERFACE ROUTINES
 *===========================================================================*/

/*---------------------------------------------------------------------------*
 *	start transmitting after connect
 *---------------------------------------------------------------------------*/
static void
i4bipr_connect_startio(struct ipr_softc *sc)
{
	int s = splnet();

	if(sc->sc_state == ST_CONNECTED_W)
	{
		sc->sc_state = ST_CONNECTED_A;
		ipr_tx_queue_empty(sc);
	}

	splx(s);
}

/*---------------------------------------------------------------------------*
 *	this routine is called from L4 handler at connect time
 *---------------------------------------------------------------------------*/
static void
ipr_connect(void *softc, void *cdp)
{
	struct ipr_softc *sc = softc;
	int s;

	sc->sc_cdp = (call_desc_t *)cdp;

	s = splnet();

	NDBGL4(L4_DIALST, "%s: setting dial state to ST_CONNECTED", sc->sc_if.if_xname);

	sc->sc_if.if_flags |= IFF_RUNNING;
	sc->sc_state = ST_CONNECTED_W;

	sc->sc_dialresp = DSTAT_NONE;
	sc->sc_lastdialresp = DSTAT_NONE;

#if I4BIPRACCT
	sc->sc_iinb = 0;
	sc->sc_ioutb = 0;
	sc->sc_inb = 0;
	sc->sc_outb = 0;
	sc->sc_linb = 0;
	sc->sc_loutb = 0;
	sc->sc_if.if_timer = I4BIPRACCTINTVL;
#endif

#ifdef I4BIPRADJFRXP
	sc->sc_first_pkt = 1;
#endif

	/*
	 * Sometimes ISDN B-channels are switched thru asymmetic. This
	 * means that under such circumstances B-channel data (the first
	 * three packets of a TCP connection in my case) may get lost,
	 * causing a large delay until the connection is started.
	 * When the sending of the very first packet of a TCP connection
	 * is delayed for a to be empirically determined delay (close
	 * to a second in my case) those packets go thru and the TCP
	 * connection comes up "almost" immediately (-hm).
	 */

	if(sc->sc_cdp->isdntxdelay > 0)
	{
		int xdelay;

		if (hz == 100) {
			xdelay = sc->sc_cdp->isdntxdelay;	/* avoid any rounding */
		} else {
			xdelay = sc->sc_cdp->isdntxdelay*hz;
			xdelay /= 100;
		}

		START_TIMER(sc->sc_callout, (TIMEOUT_FUNC_T)i4bipr_connect_startio, (void *)sc,  xdelay);
	}
	else
	{
		sc->sc_state = ST_CONNECTED_A;
		ipr_tx_queue_empty(sc);
	}

	splx(s);

	/* we don't need any negotiation - pass event back right now */
	i4b_l4_negcomplete(sc->sc_cdp);
}

/*---------------------------------------------------------------------------*
 *	this routine is called from L4 handler at disconnect time
 *---------------------------------------------------------------------------*/
static void
ipr_disconnect(void *softc, void *cdp)
{
	call_desc_t *cd = (call_desc_t *)cdp;
	struct ipr_softc *sc = softc;

	/* new stuff to check that the active channel is being closed */

	if (cd != sc->sc_cdp)
	{
		NDBGL4(L4_IPRDBG, "%s: channel %d not active",
				sc->sc_if.if_xname, cd->channelid);
		return;
	}

#if I4BIPRACCT
	sc->sc_if.if_timer = 0;
#endif
#if IPR_LOG
	/* show next IPR_LOG packets again */
	sc->sc_log_first = IPR_LOG;
#endif

	i4b_l4_accounting(cd->cdid, ACCT_FINAL,
		 sc->sc_ioutb, sc->sc_iinb, 0, 0, sc->sc_outb, sc->sc_inb);

	sc->sc_cdp = NULL;

	NDBGL4(L4_DIALST, "setting dial state to ST_IDLE");

	sc->sc_dialresp = DSTAT_NONE;
	sc->sc_lastdialresp = DSTAT_NONE;

	sc->sc_if.if_flags &= ~IFF_RUNNING;
	sc->sc_state = ST_IDLE;
}

/*---------------------------------------------------------------------------*
 *	this routine is used to give a feedback from userland daemon
 *	in case of dial problems
 *---------------------------------------------------------------------------*/
static void
ipr_dialresponse(void *softc, int status, cause_t cause)
{
	struct ipr_softc *sc = softc;
	sc->sc_dialresp = status;

	NDBGL4(L4_IPRDBG, "%s: last=%d, this=%d",
		sc->sc_if.if_xname, sc->sc_lastdialresp, sc->sc_dialresp);

	if(status != DSTAT_NONE)
	{
		NDBGL4(L4_IPRDBG, "%s: clearing queues", sc->sc_if.if_xname);
		iripclearqueues(sc);
	}
}

/*---------------------------------------------------------------------------*
 *	interface soft up/down
 *---------------------------------------------------------------------------*/
static void
ipr_updown(void *softc, int updown)
{
	struct ipr_softc *sc = softc;
	sc->sc_updown = updown;
}

/*---------------------------------------------------------------------------*
 *	this routine is called from the HSCX interrupt handler
 *	when a new frame (mbuf) has been received and was put on
 *	the rx queue. It is assumed that this routines runs at
 *	appropriate protection level! Keep it short !
 *---------------------------------------------------------------------------*/
static void
ipr_rx_data_rdy(void *softc)
{
	register struct ipr_softc *sc = softc;
	register struct mbuf *m;
#ifdef IRIP_VJ
#ifdef IPR_VJ_USEBUFFER
	u_char *cp = sc->sc_cbuf;
#endif
	int len, c;
#endif

	if((m = *sc->sc_ilt->rx_mbuf) == NULL)
		return;

	m->m_pkthdr.rcvif = &sc->sc_if;

	m->m_pkthdr.len = m->m_len;

#ifdef I4BIPRADJFRXP

	/*
	 * The very first packet after the B channel is switched thru
	 * has very often several bytes of random data prepended. This
	 * routine looks where the IP header starts and removes the
	 * the bad data.
	 */

	if(sc->sc_first_pkt)
	{
		unsigned char *mp = m->m_data;
		int i;

		sc->sc_first_pkt = 0;

		for(i = 0; i < m->m_len; i++, mp++)
		{
			if( ((*mp & 0xf0) == 0x40) &&
			    ((*mp & 0x0f) >= 0x05) )
			{
				m->m_data = mp;
				m->m_pkthdr.len -= i;
				break;
			}
		}
	}
#endif

	sc->sc_if.if_ipackets++;
	sc->sc_if.if_ibytes += m->m_pkthdr.len;

#ifdef	IRIP_VJ
	if((c = (*(mtod(m, u_char *)) & 0xf0)) != (IPVERSION << 4))
	{
		/* copy data to buffer */

		len = m->m_len;

#ifdef IPR_VJ_USEBUFFER
/* XXX */	m_copydata(m, 0, len, cp);
#endif

		if(c & 0x80)
		{
			c = TYPE_COMPRESSED_TCP;
		}
		else if(c == TYPE_UNCOMPRESSED_TCP)
		{
#ifdef IPR_VJ_USEBUFFER
			*cp &= 0x4f;		/* XXX */
#else
			*(mtod(m, u_char *)) &= 0x4f;
#endif
		}

		/*
		 * We've got something that's not an IP packet.
		 * If compression is enabled, try to decompress it.
		 * Otherwise, if `auto-enable' compression is on and
		 * it's a reasonable packet, decompress it and then
		 * enable compression.  Otherwise, drop it.
		 */
		if(sc->sc_if.if_flags & IPR_COMPRESS)
		{
#ifdef IPR_VJ_USEBUFFER
			len = sl_uncompress_tcp(&cp,len,(u_int)c,&sc->sc_compr);
#else
			len = sl_uncompress_tcp((u_char **)&m->m_data, len,
					(u_int)c, &sc->sc_compr);
#endif

			if(len <= 0)
			{
#ifdef DEBUG_IPR_VJ
				printf("i4b_ipr, ipr_rx_data_rdy: len <= 0 IPR_COMPRESS!\n");
#endif
				goto error;
			}
		}
		else if((sc->sc_if.if_flags & IPR_AUTOCOMP) &&
			(c == TYPE_UNCOMPRESSED_TCP) && (len >= 40))
		{
#ifdef IPR_VJ_USEBUFFER
			len = sl_uncompress_tcp(&cp,len,(u_int)c,&sc->sc_compr);
#else
			len = sl_uncompress_tcp((u_char **)&m->m_data, len,
					(u_int)c, &sc->sc_compr);
#endif

			if(len <= 0)
			{
#ifdef DEBUG_IPR_VJ
				printf("i4b_ipr, ipr_rx_data_rdy: len <= 0 IPR_AUTOCOMP!\n");
#endif
				goto error;
			}

			sc->sc_if.if_flags |= IPR_COMPRESS;
		}
		else
		{
#ifdef DEBUG_IPR_VJ
			printf("i4b_ipr, ipr_input: invalid ip packet!\n");
#endif

error:
			sc->sc_if.if_ierrors++;
			sc->sc_if.if_collisions++;
			m_freem(m);
			return;
		}
#ifdef IPR_VJ_USEBUFFER
/* XXX */	m_copyback(m, 0, len, cp);
#else
		m->m_len = m->m_pkthdr.len = len;
#endif
	}
#endif

#if I4BIPRACCT
	/* NB. do the accounting after decompression!		*/
	sc->sc_inb += m->m_pkthdr.len;
#endif
#if IPR_LOG
	if(sc->sc_log_first > 0)
	{
		--(sc->sc_log_first);
		i4b_l4_packet_ind(irip_driver_id, sc->sc_unit, 0, m );
	}
#endif

#if NBPFILTER > 0 || NBPF > 0
	if(sc->sc_if.if_bpf)
	{
		/* prepend the address family as a four byte field */
		struct mbuf mm;
		u_int af = AF_INET;
		mm.m_next = m;
		mm.m_len = 4;
		mm.m_data = (char *)&af;

#ifdef __FreeBSD__
		bpf_mtap(&sc->sc_if, &mm);
#else
		bpf_mtap(sc->sc_if.if_bpf, &mm);
#endif
	}
#endif /* NBPFILTER > 0  || NBPF > 0 */

	if(IF_QFULL(&ipintrq))
	{
		NDBGL4(L4_IPRDBG, "%s: ipintrq full!", sc->sc_if.if_xname);

		IF_DROP(&ipintrq);
		sc->sc_if.if_ierrors++;
		sc->sc_if.if_iqdrops++;
		m_freem(m);
	}
	else
	{
		IF_ENQUEUE(&ipintrq, m);
		schednetisr(NETISR_IP);
	}
}

/*---------------------------------------------------------------------------*
 *	this routine is called from the HSCX interrupt handler
 *	when the last frame has been sent out and there is no
 *	further frame (mbuf) in the tx queue.
 *---------------------------------------------------------------------------*/
static void
ipr_tx_queue_empty(void *softc)
{
	register struct ipr_softc *sc = softc;
	register struct mbuf *m;
#ifdef	IRIP_VJ
	struct ip *ip;
#endif
	int x = 0;

	if(sc->sc_state != ST_CONNECTED_A)
		return;

	for(;;)
	{
                IF_DEQUEUE(&sc->sc_fastq, m);
                if(m)
                {
			sc->sc_if.if_omcasts++;
		}
		else
		{
			IFQ_DEQUEUE(&sc->sc_if.if_snd, m);
			if(m == NULL)
				break;
		}

#if NBPFILTER > 0 || NBPF > 0
		if(sc->sc_if.if_bpf)
		{
			/* prepend the address family as a four byte field */

			struct mbuf mm;
			u_int af = AF_INET;
			mm.m_next = m;
			mm.m_len = 4;
			mm.m_data = (char *)&af;

#ifdef __FreeBSD__
			bpf_mtap(&sc->sc_if, &mm);
#else
			bpf_mtap(sc->sc_if.if_bpf, &mm);
#endif
		}
#endif /* NBPFILTER */

#if I4BIPRACCT
		sc->sc_outb += m->m_pkthdr.len;	/* size before compression */
#endif

#ifdef	IRIP_VJ
		if((ip = mtod(m, struct ip *))->ip_p == IPPROTO_TCP)
		{
			if(sc->sc_if.if_flags & IPR_COMPRESS)
			{
				*mtod(m, u_char *) |= sl_compress_tcp(m, ip,
					&sc->sc_compr, 1);
			}
		}
#endif
		x = 1;

		if(IF_QFULL(sc->sc_ilt->tx_queue))
		{
			NDBGL4(L4_IPRDBG, "%s: tx queue full!", sc->sc_if.if_xname);
			m_freem(m);
		}
		else
		{
			IF_ENQUEUE(sc->sc_ilt->tx_queue, m);

			sc->sc_if.if_obytes += m->m_pkthdr.len;

			sc->sc_if.if_opackets++;
		}
	}

	if(x)
		(*sc->sc_ilt->bchannel_driver->bch_tx_start)(sc->sc_ilt->l1token, sc->sc_ilt->channel);
}

/*---------------------------------------------------------------------------*
 *	this routine is called from the HSCX interrupt handler
 *	each time a packet is received or transmitted. It should
 *	be used to implement an activity timeout mechanism.
 *---------------------------------------------------------------------------*/
static void
ipr_activity(void *softc, int rxtx)
{
	struct ipr_softc *sc = softc;
	sc->sc_cdp->last_active_time = SECOND;
}

/*---------------------------------------------------------------------------*
 *	return this drivers linktab address
 *---------------------------------------------------------------------------*/
static void*
ipr_get_softc(int unit)
{
	return &ipr_softc[unit];
}

/*---------------------------------------------------------------------------*
 *	setup the isdn_linktab for this driver
 *---------------------------------------------------------------------------*/
static void
ipr_set_linktab(void *softc, isdn_link_t *ilt)
{
	struct ipr_softc *sc = softc;
	sc->sc_ilt = ilt;
}

/*===========================================================================*/

#endif /* NIRIP > 0 */
