/*	$NetBSD: if_tun.c,v 1.67 2003/09/22 20:49:39 cl Exp $	*/

/*
 * Copyright (c) 1988, Julian Onions <jpo@cs.nott.ac.uk>
 * Nottingham University 1987.
 *
 * This source may be freely distributed, however I would be interested
 * in any changes that are made.
 *
 * This driver takes packets off the IP i/f and hands them up to a
 * user process to have its wicked way with. This driver has its
 * roots in a similar driver written by Phil Cockcroft (formerly) at
 * UCL. This driver is based much more on read/write/poll mode of
 * operation though.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_tun.c,v 1.67 2003/09/22 20:49:39 cl Exp $");

#include "tun.h"

#include "opt_inet.h"
#include "opt_ns.h"

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/buf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/syslog.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/file.h>
#include <sys/signalvar.h>
#include <sys/conf.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/netisr.h>
#include <net/route.h>


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

#include "bpfilter.h"
#if NBPFILTER > 0
#include <sys/time.h>
#include <net/bpf.h>
#endif

#include <net/if_tun.h>

#define TUNDEBUG	if (tundebug) printf
int	tundebug = 0;

extern int ifqmaxlen;
void	tunattach __P((int));
LIST_HEAD(, tun_softc) tun_softc_list;
static struct simplelock tun_softc_lock;

int	tun_ioctl __P((struct ifnet *, u_long, caddr_t));
int	tun_output __P((struct ifnet *, struct mbuf *, struct sockaddr *,
		       struct rtentry *rt));
int	tun_clone_create __P((struct if_clone *, int));
void	tun_clone_destroy __P((struct ifnet *));

struct if_clone tun_cloner =
    IF_CLONE_INITIALIZER("tun", tun_clone_create, tun_clone_destroy);

static void tunattach0 __P((struct tun_softc *));
static void tuninit __P((struct tun_softc *));
#ifdef ALTQ
static void tunstart __P((struct ifnet *));
#endif
static struct tun_softc *tun_find_unit __P((dev_t));

dev_type_open(tunopen);
dev_type_close(tunclose);
dev_type_read(tunread);
dev_type_write(tunwrite);
dev_type_ioctl(tunioctl);
dev_type_poll(tunpoll);
dev_type_kqfilter(tunkqfilter);

const struct cdevsw tun_cdevsw = {
	tunopen, tunclose, tunread, tunwrite, tunioctl,
	nostop, notty, tunpoll, nommap, tunkqfilter,
};

void
tunattach(unused)
	int unused;
{

	simple_lock_init(&tun_softc_lock);
	LIST_INIT(&tun_softc_list);
	if_clone_attach(&tun_cloner);
}

int
tun_clone_create(ifc, unit)
	struct if_clone *ifc;
	int unit;
{
	struct tun_softc *sc;

	sc = malloc(sizeof(struct tun_softc), M_DEVBUF, M_WAITOK);
	(void)memset(sc, 0, sizeof(struct tun_softc));

	(void)snprintf(sc->tun_if.if_xname, sizeof(sc->tun_if.if_xname),
	    "%s%d", ifc->ifc_name, unit);
	sc->tun_unit = unit;
	simple_lock_init(&sc->tun_lock);

	tunattach0(sc);

	simple_lock(&tun_softc_lock);
	LIST_INSERT_HEAD(&tun_softc_list, sc, tun_list);
	simple_unlock(&tun_softc_lock);

	return (0);
}

void
tunattach0(sc)
	struct tun_softc *sc;
{
	struct ifnet *ifp = (void *)sc;

	sc->tun_flags = TUN_INITED;

	ifp = &sc->tun_if;
	ifp->if_softc = sc;
	ifp->if_mtu = TUNMTU;
	ifp->if_ioctl = tun_ioctl;
	ifp->if_output = tun_output;
#ifdef ALTQ
	ifp->if_start = tunstart;
#endif
	ifp->if_flags = IFF_POINTOPOINT;
	ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_collisions = 0;
	ifp->if_ierrors = 0;
	ifp->if_oerrors = 0;
	ifp->if_ipackets = 0;
	ifp->if_opackets = 0;
	ifp->if_ibytes   = 0;
	ifp->if_obytes   = 0;
	ifp->if_dlt = DLT_NULL;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	if_alloc_sadl(ifp);
#if NBPFILTER > 0
	bpfattach(ifp, DLT_NULL, sizeof(u_int32_t));
#endif
}

void
tun_clone_destroy(ifp)
	struct ifnet *ifp;
{
	struct tun_softc *tp = (void *)ifp;

	simple_lock(&tun_softc_lock);
	simple_lock(&tp->tun_lock);
	LIST_REMOVE(tp, tun_list);
	simple_unlock(&tp->tun_lock);
	simple_unlock(&tun_softc_lock);

	if (tp->tun_flags & TUN_RWAIT) {
		tp->tun_flags &= ~TUN_RWAIT;
		wakeup((caddr_t)tp);
	}
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_HUP, 0, NULL);

	selwakeup(&tp->tun_rsel);

#if NBPFILTER > 0
	bpfdetach(ifp);
#endif
	if_detach(ifp);

	free(tp, M_DEVBUF);
}

static struct tun_softc *
tun_find_unit(dev)
	dev_t dev;
{
	struct tun_softc *tp;
	int unit = minor(dev);

	simple_lock(&tun_softc_lock);
	LIST_FOREACH(tp, &tun_softc_list, tun_list)
		if (unit == tp->tun_unit)
			break;
	if (tp)
		simple_lock(&tp->tun_lock);
	simple_unlock(&tun_softc_lock);

	return (tp);
}

/*
 * tunnel open - must be superuser & the device must be
 * configured in
 */
int
tunopen(dev, flag, mode, p)
	dev_t	dev;
	int	flag, mode;
	struct proc *p;
{
	struct ifnet	*ifp;
	struct tun_softc *tp;
	int	error;

	if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
		return (error);

	if (NTUN < 1)
		return (ENXIO);

	tp = tun_find_unit(dev);

	if (!tp) {
		(void)tun_clone_create(&tun_cloner, minor(dev));
		tp = tun_find_unit(dev);
	}

	if (!tp)
		return (ENXIO);

	if (tp->tun_flags & TUN_OPEN) {
		simple_unlock(&tp->tun_lock);
		return (EBUSY);
	}

	ifp = &tp->tun_if;
	tp->tun_flags |= TUN_OPEN;
	TUNDEBUG("%s: open\n", ifp->if_xname);
	simple_unlock(&tp->tun_lock);
	return (0);
}

/*
 * tunclose - close the device - mark i/f down & delete
 * routing info
 */
int
tunclose(dev, flag, mode, p)
	dev_t	dev;
	int	flag;
	int	mode;
	struct proc *p;
{
	int	s;
	struct tun_softc *tp;
	struct ifnet	*ifp;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" before the close */
	if (tp == NULL)
		return (0);

	ifp = &tp->tun_if;

	tp->tun_flags &= ~TUN_OPEN;

	/*
	 * junk all pending output
	 */
	s = splnet();
	IFQ_PURGE(&ifp->if_snd);
	splx(s);

	if (ifp->if_flags & IFF_UP) {
		s = splnet();
		if_down(ifp);
		if (ifp->if_flags & IFF_RUNNING) {
			/* find internet addresses and delete routes */
			struct ifaddr *ifa;
			TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
#ifdef INET
				if (ifa->ifa_addr->sa_family == AF_INET) {
					rtinit(ifa, (int)RTM_DELETE,
					       tp->tun_flags & TUN_DSTADDR
							? RTF_HOST
							: 0);
				}
#endif
			}
		}
		splx(s);
	}
	tp->tun_pgid = 0;
	selnotify(&tp->tun_rsel, 0);
		
	TUNDEBUG ("%s: closed\n", ifp->if_xname);
	simple_unlock(&tp->tun_lock);
	return (0);
}

static void
tuninit(tp)
	struct tun_softc *tp;
{
	struct ifnet	*ifp = &tp->tun_if;
	struct ifaddr	*ifa;

	TUNDEBUG("%s: tuninit\n", ifp->if_xname);

	ifp->if_flags |= IFF_UP | IFF_RUNNING;

	tp->tun_flags &= ~(TUN_IASET|TUN_DSTADDR);
	TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) {
#ifdef INET
		if (ifa->ifa_addr->sa_family == AF_INET) {
			struct sockaddr_in *sin;

			sin = satosin(ifa->ifa_addr);
			if (sin && sin->sin_addr.s_addr)
				tp->tun_flags |= TUN_IASET;

			if (ifp->if_flags & IFF_POINTOPOINT) {
				sin = satosin(ifa->ifa_dstaddr);
				if (sin && sin->sin_addr.s_addr)
					tp->tun_flags |= TUN_DSTADDR;
			}
		}
#endif
	}

	return;
}

/*
 * Process an ioctl request.
 */
int
tun_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t	data;
{
	int		error = 0, s;
	struct tun_softc *tp = (struct tun_softc *)(ifp->if_softc);

	simple_lock(&tp->tun_lock);

	s = splnet();
	switch (cmd) {
	case SIOCSIFADDR:
		tuninit((struct tun_softc *)(ifp->if_softc));
		TUNDEBUG("%s: address set\n", ifp->if_xname);
		break;
	case SIOCSIFDSTADDR:
		tuninit((struct tun_softc *)(ifp->if_softc));
		TUNDEBUG("%s: destination address set\n", ifp->if_xname);
		break;
	case SIOCSIFBRDADDR:
		TUNDEBUG("%s: broadcast address set\n", ifp->if_xname);
		break;
	case SIOCSIFMTU: {
		struct ifreq *ifr = (struct ifreq *) data;
		if (ifr->ifr_mtu > TUNMTU || ifr->ifr_mtu < 576) {
		    error = EINVAL;
		    break;
		}
		TUNDEBUG("%s: interface mtu set\n", ifp->if_xname);
		ifp->if_mtu = ifr->ifr_mtu;
		break;
	}
	case SIOCADDMULTI:
	case SIOCDELMULTI: {
		struct ifreq *ifr = (struct ifreq *) data;
		if (ifr == 0) {
	        	error = EAFNOSUPPORT;           /* XXX */
			break;
		}
		switch (ifr->ifr_addr.sa_family) {

#ifdef INET
		case AF_INET:
			break;
#endif

		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
	}
	case SIOCSIFFLAGS:
		break;
	default:
		error = EINVAL;
	}
	splx(s);
	simple_unlock(&tp->tun_lock);
	return (error);
}

/*
 * tun_output - queue packets from higher level ready to put out.
 */
int
tun_output(ifp, m0, dst, rt)
	struct ifnet   *ifp;
	struct mbuf    *m0;
	struct sockaddr *dst;
	struct rtentry *rt;
{
	struct tun_softc *tp = ifp->if_softc;
#ifdef INET
	int		s;
	int		error;
#endif
	int		mlen;
	ALTQ_DECL(struct altq_pktattr pktattr;)

	simple_lock(&tp->tun_lock);
	TUNDEBUG ("%s: tun_output\n", ifp->if_xname);

	if ((tp->tun_flags & TUN_READY) != TUN_READY) {
		TUNDEBUG ("%s: not ready 0%o\n", ifp->if_xname,
			  tp->tun_flags);
		m_freem (m0);
		simple_unlock(&tp->tun_lock);
		return (EHOSTDOWN);
	}

	/*
	 * if the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m0, dst->sa_family, &pktattr);
 
#if NBPFILTER > 0
	if (ifp->if_bpf) {
		/*
		 * We need to prepend the address family as
		 * a four byte field.  Cons up a dummy header
		 * to pacify bpf.  This is safe because bpf
		 * will only read from the mbuf (i.e., it won't
		 * try to free it or keep a pointer to it).
		 */
		struct mbuf m;
		u_int32_t af = dst->sa_family;

		m.m_flags = 0;
		m.m_next = m0;
		m.m_len = sizeof(af);
		m.m_data = (char *)&af;

		bpf_mtap(ifp->if_bpf, &m);
	}
#endif

	switch(dst->sa_family) {
#ifdef INET
	case AF_INET:
		if (tp->tun_flags & TUN_PREPADDR) {
			/* Simple link-layer header */
			M_PREPEND(m0, dst->sa_len, M_DONTWAIT);
			if (m0 == NULL) {
				IF_DROP(&ifp->if_snd);
				simple_unlock(&tp->tun_lock);
				return (ENOBUFS);
			}
			bcopy(dst, mtod(m0, char *), dst->sa_len);
		}
		/* FALLTHROUGH */
	case AF_UNSPEC:
		s = splnet();
		IFQ_ENQUEUE(&ifp->if_snd, m0, &pktattr, error);
		if (error) {
			splx(s);
			ifp->if_collisions++;
			return (error);
		}
		mlen = m0->m_pkthdr.len;
		splx(s);
		ifp->if_opackets++;
		ifp->if_obytes += mlen;
		break;
#endif
	default:
		m_freem(m0);
		simple_unlock(&tp->tun_lock);
		return (EAFNOSUPPORT);
	}

	if (tp->tun_flags & TUN_RWAIT) {
		tp->tun_flags &= ~TUN_RWAIT;
		wakeup((caddr_t)tp);
	}
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_IN, POLLIN|POLLRDNORM,
		    NULL);

	selnotify(&tp->tun_rsel, 0);
	simple_unlock(&tp->tun_lock);
	return (0);
}

/*
 * the cdevsw interface is now pretty minimal.
 */
int
tunioctl(dev, cmd, data, flag, p)
	dev_t		dev;
	u_long		cmd;
	caddr_t		data;
	int		flag;
	struct proc	*p;
{
	int		s;
	struct tun_softc *tp;
	int error=0;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL)
		return (ENXIO);

	switch (cmd) {
	case TUNSDEBUG:
		tundebug = *(int *)data;
		break;

	case TUNGDEBUG:
		*(int *)data = tundebug;
		break;

	case TUNSIFMODE:
		switch (*(int *)data & (IFF_POINTOPOINT|IFF_BROADCAST)) {
		case IFF_POINTOPOINT:
		case IFF_BROADCAST:
			s = splnet();
			if (tp->tun_if.if_flags & IFF_UP) {
				splx(s);
				simple_unlock(&tp->tun_lock);
				return (EBUSY);
			}
			tp->tun_if.if_flags &=
				~(IFF_BROADCAST|IFF_POINTOPOINT|IFF_MULTICAST);
			tp->tun_if.if_flags |= *(int *)data;
			splx(s);
			break;
		default:
			simple_unlock(&tp->tun_lock);
			return (EINVAL);
		}
		break;

	case TUNSLMODE:
		if (*(int *)data)
			tp->tun_flags |= TUN_PREPADDR;
		else
			tp->tun_flags &= ~TUN_PREPADDR;
		break;

	case FIONBIO:
		if (*(int *)data)
			tp->tun_flags |= TUN_NBIO;
		else
			tp->tun_flags &= ~TUN_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			tp->tun_flags |= TUN_ASYNC;
		else
			tp->tun_flags &= ~TUN_ASYNC;
		break;

	case FIONREAD:
		s = splnet();
		if (tp->tun_if.if_snd.ifq_head)
			*(int *)data = tp->tun_if.if_snd.ifq_head->m_pkthdr.len;
		else	
			*(int *)data = 0;
		splx(s);
		break;

	case TIOCSPGRP:
	case FIOSETOWN:
		error = fsetown(p, &tp->tun_pgid, cmd, data);
		break;

	case TIOCGPGRP:
	case FIOGETOWN:
		error = fgetown(p, tp->tun_pgid, cmd, data);
		break;

	default:
		simple_unlock(&tp->tun_lock);
		return (ENOTTY);
	}
	simple_unlock(&tp->tun_lock);
	return (error);
}

/*
 * The cdevsw read interface - reads a packet at a time, or at
 * least as much of a packet as can be read.
 */
int
tunread(dev, uio, ioflag)
	dev_t		dev;
	struct uio	*uio;
	int		ioflag;
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	struct mbuf	*m, *m0;
	int		error=0, len, s, index;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL)
		return (ENXIO);

	index = tp->tun_if.if_index;
	ifp = &tp->tun_if;

	TUNDEBUG ("%s: read\n", ifp->if_xname);
	if ((tp->tun_flags & TUN_READY) != TUN_READY) {
		TUNDEBUG ("%s: not ready 0%o\n", ifp->if_xname, tp->tun_flags);
		simple_unlock(&tp->tun_lock);
		return EHOSTDOWN;
	}

	tp->tun_flags &= ~TUN_RWAIT;

	s = splnet();
	do {
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == 0) {
			if (tp->tun_flags & TUN_NBIO) {
				splx(s);
				simple_unlock(&tp->tun_lock);
				return (EWOULDBLOCK);
			}
			tp->tun_flags |= TUN_RWAIT;
			simple_unlock(&tp->tun_lock);
			if (tsleep((caddr_t)tp, PZERO|PCATCH, "tunread", 0)) {
				splx(s);
				return (EINTR);
			} else {
				/*
				 * Maybe the interface was destroyed while
				 * we were sleeping, so let's ensure that
				 * we're looking at the same (valid) tun
				 * interface before looping.
				 */
				tp = tun_find_unit(dev);
				if (tp == NULL ||
				    tp->tun_if.if_index != index) {
					splx(s);
					if (tp)
						simple_unlock(&tp->tun_lock);
					return (ENXIO);
				}
			}
		}
	} while (m0 == 0);
	splx(s);

	while (m0 && uio->uio_resid > 0 && error == 0) {
		len = min(uio->uio_resid, m0->m_len);
		if (len != 0)
			error = uiomove(mtod(m0, caddr_t), len, uio);
		MFREE(m0, m);
		m0 = m;
	}

	if (m0) {
		TUNDEBUG("Dropping mbuf\n");
		m_freem(m0);
	}
	if (error)
		ifp->if_ierrors++;
	simple_unlock(&tp->tun_lock);
	return (error);
}

/*
 * the cdevsw write interface - an atomic write is a packet - or else!
 */
int
tunwrite(dev, uio, ioflag)
	dev_t		dev;
	struct uio	*uio;
	int		ioflag;
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	struct mbuf	*top, **mp, *m;
	struct ifqueue	*ifq;
	struct sockaddr	dst;
	int		isr, error=0, s, tlen, mlen;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL)
		return (ENXIO);

	ifp = &tp->tun_if;

	TUNDEBUG("%s: tunwrite\n", ifp->if_xname);

	if (tp->tun_flags & TUN_PREPADDR) {
		if (uio->uio_resid < sizeof(dst)) {
			simple_unlock(&tp->tun_lock);
			return (EIO);
		}
		error = uiomove((caddr_t)&dst, sizeof(dst), uio);
		if (dst.sa_len > sizeof(dst)) {
			/* Duh.. */
			char discard;
			int n = dst.sa_len - sizeof(dst);
			while (n--)
				if ((error = uiomove(&discard, 1, uio)) != 0) {
					simple_unlock(&tp->tun_lock);
					return (error);
				}
		}
	} else {
#ifdef INET
		dst.sa_family = AF_INET;
#endif
	}

	if (uio->uio_resid > TUNMTU) {
		TUNDEBUG("%s: len=%lu!\n", ifp->if_xname,
		    (unsigned long)uio->uio_resid);
		simple_unlock(&tp->tun_lock);
		return (EIO);
	}

	switch (dst.sa_family) {
#ifdef INET
	case AF_INET:
		ifq = &ipintrq;
		isr = NETISR_IP;
		break;
#endif
	default:
		simple_unlock(&tp->tun_lock);
		return (EAFNOSUPPORT);
	}

	tlen = uio->uio_resid;

	/* get a header mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		simple_unlock(&tp->tun_lock);
		return (ENOBUFS);
	}
	mlen = MHLEN;

	top = 0;
	mp = &top;
	while (error == 0 && uio->uio_resid > 0) {
		m->m_len = min(mlen, uio->uio_resid);
		error = uiomove(mtod (m, caddr_t), m->m_len, uio);
		*mp = m;
		mp = &m->m_next;
		if (uio->uio_resid > 0) {
			MGET (m, M_DONTWAIT, MT_DATA);
			if (m == 0) {
				error = ENOBUFS;
				break;
			}
			mlen = MLEN;
		}
	}
	if (error) {
		if (top)
			m_freem (top);
		ifp->if_ierrors++;
		simple_unlock(&tp->tun_lock);
		return (error);
	}

	top->m_pkthdr.len = tlen;
	top->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
	if (ifp->if_bpf) {
		/*
		 * We need to prepend the address family as
		 * a four byte field.  Cons up a dummy header
		 * to pacify bpf.  This is safe because bpf
		 * will only read from the mbuf (i.e., it won't
		 * try to free it or keep a pointer to it).
		 */
		struct mbuf m;
		u_int32_t af = AF_INET;

		m.m_flags = 0;
		m.m_next = top;
		m.m_len = sizeof(af);
		m.m_data = (char *)&af;

		bpf_mtap(ifp->if_bpf, &m);
	}
#endif

	s = splnet();
	if (IF_QFULL(ifq)) {
		IF_DROP(ifq);
		splx(s);
		ifp->if_collisions++;
		m_freem(top);
		simple_unlock(&tp->tun_lock);
		return (ENOBUFS);
	}
	IF_ENQUEUE(ifq, top);
	splx(s);
	ifp->if_ipackets++;
	ifp->if_ibytes += tlen;
	schednetisr(isr);
	simple_unlock(&tp->tun_lock);
	return (error);
}

#ifdef ALTQ
/*
 * Start packet transmission on the interface.
 * when the interface queue is rate-limited by ALTQ or TBR,
 * if_start is needed to drain packets from the queue in order
 * to notify readers when outgoing packets become ready.
 */
static void
tunstart(ifp)
	struct ifnet *ifp;
{
	struct tun_softc *tp = ifp->if_softc;
	struct mbuf *m;

	if (!ALTQ_IS_ENABLED(&ifp->if_snd) && !TBR_IS_ENABLED(&ifp->if_snd))
		return;

	IFQ_POLL(&ifp->if_snd, m);
	if (m != NULL) {
		if (tp->tun_flags & TUN_RWAIT) {
			tp->tun_flags &= ~TUN_RWAIT;
			wakeup((caddr_t)tp);
		}
		if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
			fownsignal(tp->tun_pgid, SIGIO, POLL_OUT,
				POLLOUT|POLLWRNORM, NULL);
	
		selwakeup(&tp->tun_rsel);
	}
}
#endif /* ALTQ */
/*
 * tunpoll - the poll interface, this is only useful on reads
 * really. The write detect always returns true, write never blocks
 * anyway, it either accepts the packet or drops it.
 */
int
tunpoll(dev, events, p)
	dev_t		dev;
	int		events;
	struct proc	*p;
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	int		s, revents = 0;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL)
		return (0);

	ifp = &tp->tun_if;

	s = splnet();
	TUNDEBUG("%s: tunpoll\n", ifp->if_xname);

	if (events & (POLLIN | POLLRDNORM)) {
		if (IFQ_IS_EMPTY(&ifp->if_snd) == 0) {
			TUNDEBUG("%s: tunpoll q=%d\n", ifp->if_xname,
			    ifp->if_snd.ifq_len);
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			TUNDEBUG("%s: tunpoll waiting\n", ifp->if_xname);
			selrecord(p, &tp->tun_rsel);
		}
	}

	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);

	splx(s);
	simple_unlock(&tp->tun_lock);
	return (revents);
}

static void
filt_tunrdetach(struct knote *kn)
{
	struct tun_softc *tp = kn->kn_hook;
	int s;

	s = splnet();
	SLIST_REMOVE(&tp->tun_rsel.sel_klist, kn, knote, kn_selnext);
	splx(s);
}

static int
filt_tunread(struct knote *kn, long hint)
{
	struct tun_softc *tp = kn->kn_hook;
	struct ifnet *ifp = &tp->tun_if;
	struct mbuf *m;
	int s;

	s = splnet();
	IF_POLL(&ifp->if_snd, m);
	if (m == NULL) {
		splx(s);
		return (0);
	}

	for (kn->kn_data = 0; m != NULL; m = m->m_next)
		kn->kn_data += m->m_len;

	splx(s);
	return (1);
}

static const struct filterops tunread_filtops =
	{ 1, NULL, filt_tunrdetach, filt_tunread };

static const struct filterops tun_seltrue_filtops =
	{ 1, NULL, filt_tunrdetach, filt_seltrue };

int
tunkqfilter(dev_t dev, struct knote *kn)
{
	struct tun_softc *tp = tun_find_unit(dev);
	struct klist *klist;
	int s;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		klist = &tp->tun_rsel.sel_klist;
		kn->kn_fop = &tunread_filtops;
		break;

	case EVFILT_WRITE:
		klist = &tp->tun_rsel.sel_klist;
		kn->kn_fop = &tun_seltrue_filtops;
		break;

	default:
		return (1);
	}

	kn->kn_hook = tp;

	s = splnet();
	SLIST_INSERT_HEAD(klist, kn, kn_selnext);
	splx(s);

	return (0);
}
