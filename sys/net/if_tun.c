/*	$NetBSD: if_tun.c,v 1.173 2022/03/28 12:33:22 riastradh Exp $	*/

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

/*
 * tun - tunnel software network interface.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_tun.c,v 1.173 2022/03/28 12:33:22 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#endif

#include <sys/param.h>

#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/kauth.h>
#include <sys/kmem.h>
#include <sys/lwp.h>
#include <sys/mbuf.h>
#include <sys/module.h>
#include <sys/mutex.h>
#include <sys/poll.h>
#include <sys/select.h>
#include <sys/signalvar.h>
#include <sys/socket.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>

#ifdef INET
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/if_inarp.h>
#endif

#include <net/if_tun.h>

#include "ioconf.h"

#define TUNDEBUG	if (tundebug) printf
int	tundebug = 0;

extern int ifqmaxlen;

static LIST_HEAD(, tun_softc) tun_softc_list;
static LIST_HEAD(, tun_softc) tunz_softc_list;
static kmutex_t tun_softc_lock;

static int	tun_ioctl(struct ifnet *, u_long, void *);
static int	tun_output(struct ifnet *, struct mbuf *,
			const struct sockaddr *, const struct rtentry *rt);
static int	tun_clone_create(struct if_clone *, int);
static int	tun_clone_destroy(struct ifnet *);

static struct if_clone tun_cloner =
    IF_CLONE_INITIALIZER("tun", tun_clone_create, tun_clone_destroy);

static void tunattach0(struct tun_softc *);
static void tun_enable(struct tun_softc *, const struct ifaddr *);
static void tun_i_softintr(void *);
static void tun_o_softintr(void *);
#ifdef ALTQ
static void tunstart(struct ifnet *);
#endif
static struct tun_softc *tun_find_unit(dev_t);
static struct tun_softc *tun_find_zunit(int);

static dev_type_open(tunopen);
static dev_type_close(tunclose);
static dev_type_read(tunread);
static dev_type_write(tunwrite);
static dev_type_ioctl(tunioctl);
static dev_type_poll(tunpoll);
static dev_type_kqfilter(tunkqfilter);

const struct cdevsw tun_cdevsw = {
	.d_open = tunopen,
	.d_close = tunclose,
	.d_read = tunread,
	.d_write = tunwrite,
	.d_ioctl = tunioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = tunpoll,
	.d_mmap = nommap,
	.d_kqfilter = tunkqfilter,
	.d_discard = nodiscard,
	.d_flag = D_OTHER | D_MPSAFE
};

#ifdef _MODULE
devmajor_t tun_bmajor = -1, tun_cmajor = -1;
#endif

void
tunattach(int unused)
{

	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in tuninit() below).
	 */
}

static void
tuninit(void)
{

	mutex_init(&tun_softc_lock, MUTEX_DEFAULT, IPL_NONE);
	LIST_INIT(&tun_softc_list);
	LIST_INIT(&tunz_softc_list);
	if_clone_attach(&tun_cloner);
#ifdef _MODULE
	devsw_attach("tun", NULL, &tun_bmajor, &tun_cdevsw, &tun_cmajor);
#endif
}

static int
tundetach(void)
{

	if_clone_detach(&tun_cloner);
#ifdef _MODULE
	devsw_detach(NULL, &tun_cdevsw);
#endif

	if (!LIST_EMPTY(&tun_softc_list) || !LIST_EMPTY(&tunz_softc_list)) {
#ifdef _MODULE
		devsw_attach("tun", NULL, &tun_bmajor, &tun_cdevsw, &tun_cmajor);
#endif
		if_clone_attach(&tun_cloner);
		return EBUSY;
}

	mutex_destroy(&tun_softc_lock);

	return 0;
}

/*
 * Find driver instance from dev_t.
 * Returns with tp locked (if found).
 */
static struct tun_softc *
tun_find_unit(dev_t dev)
{
	struct tun_softc *tp;
	int unit = minor(dev);

	mutex_enter(&tun_softc_lock);
	LIST_FOREACH(tp, &tun_softc_list, tun_list)
		if (unit == tp->tun_unit)
			break;
	if (tp)
		mutex_enter(&tp->tun_lock);
	mutex_exit(&tun_softc_lock);

	return tp;
}

/*
 * Find zombie driver instance by unit number.
 * Remove tp from list and return it unlocked (if found).
 */
static struct tun_softc *
tun_find_zunit(int unit)
{
	struct tun_softc *tp;

	mutex_enter(&tun_softc_lock);
	LIST_FOREACH(tp, &tunz_softc_list, tun_list)
		if (unit == tp->tun_unit)
			break;
	if (tp)
		LIST_REMOVE(tp, tun_list);
	mutex_exit(&tun_softc_lock);
	KASSERTMSG(!tp || (tp->tun_flags & (TUN_INITED|TUN_OPEN)) == TUN_OPEN,
	    "tun%d: inconsistent flags: %x", unit, tp->tun_flags);

	return tp;
}

static void
tun_init(struct tun_softc *tp, int unit)
{

	tp->tun_unit = unit;
	mutex_init(&tp->tun_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	cv_init(&tp->tun_cv, "tunread");
	selinit(&tp->tun_rsel);
	selinit(&tp->tun_wsel);

	tp->tun_osih = softint_establish(SOFTINT_CLOCK, tun_o_softintr, tp);
	tp->tun_isih = softint_establish(SOFTINT_CLOCK, tun_i_softintr, tp);
}

static void
tun_fini(struct tun_softc *tp)
{

	softint_disestablish(tp->tun_isih);
	softint_disestablish(tp->tun_osih);

	seldestroy(&tp->tun_wsel);
	seldestroy(&tp->tun_rsel);
	mutex_destroy(&tp->tun_lock);
	cv_destroy(&tp->tun_cv);
}

static struct tun_softc *
tun_alloc(int unit)
{
	struct tun_softc *tp;

	tp = kmem_zalloc(sizeof(*tp), KM_SLEEP);
	tun_init(tp, unit);

	return tp;
}

static void
tun_recycle(struct tun_softc *tp)
{

	memset(&tp->tun_if, 0, sizeof(struct ifnet)); /* XXX ??? */
}

static void
tun_free(struct tun_softc *tp)
{

	tun_fini(tp);
	kmem_free(tp, sizeof(*tp));
}

static int
tun_clone_create(struct if_clone *ifc, int unit)
{
	struct tun_softc *tp;

	if ((tp = tun_find_zunit(unit)) == NULL) {
		tp = tun_alloc(unit);
	} else {
		tun_recycle(tp);
	}

	if_initname(&tp->tun_if, ifc->ifc_name, unit);
	tunattach0(tp);
	tp->tun_flags |= TUN_INITED;

	mutex_enter(&tun_softc_lock);
	LIST_INSERT_HEAD(&tun_softc_list, tp, tun_list);
	mutex_exit(&tun_softc_lock);

	return 0;
}

static void
tunattach0(struct tun_softc *tp)
{
	struct ifnet *ifp;

	ifp = &tp->tun_if;
	ifp->if_softc = tp;
	ifp->if_mtu = TUNMTU;
	ifp->if_ioctl = tun_ioctl;
	ifp->if_output = tun_output;
#ifdef ALTQ
	ifp->if_start = tunstart;
#endif
	ifp->if_flags = IFF_POINTOPOINT;
	ifp->if_type = IFT_TUNNEL;
	ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_dlt = DLT_NULL;
	IFQ_SET_READY(&ifp->if_snd);
	if_attach(ifp);
	ifp->if_link_state = LINK_STATE_DOWN;
	if_alloc_sadl(ifp);
	bpf_attach(ifp, DLT_NULL, sizeof(uint32_t));
}

static int
tun_clone_destroy(struct ifnet *ifp)
{
	struct tun_softc *tp = (void *)ifp;
	bool zombie = false;

	IF_PURGE(&ifp->if_snd);
	ifp->if_flags &= ~IFF_RUNNING;

	mutex_enter(&tun_softc_lock);
	mutex_enter(&tp->tun_lock);
	LIST_REMOVE(tp, tun_list);
	if (tp->tun_flags & TUN_OPEN) {
		/* Hang on to storage until last close. */
		tp->tun_flags &= ~TUN_INITED;
		LIST_INSERT_HEAD(&tunz_softc_list, tp, tun_list);
		zombie = true;
	}
	mutex_exit(&tun_softc_lock);

	cv_broadcast(&tp->tun_cv);
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_HUP, 0, NULL);
	selnotify(&tp->tun_rsel, 0, NOTE_SUBMIT);
	mutex_exit(&tp->tun_lock);

	bpf_detach(ifp);
	if_detach(ifp);

	if (!zombie) {
		tun_free(tp);
	}

	return 0;
}

/*
 * tunnel open - must be superuser & the device must be
 * configured in
 */
static int
tunopen(dev_t dev, int flag, int mode, struct lwp *l)
{
	struct ifnet	*ifp;
	struct tun_softc *tp;
	int	error;

	error = kauth_authorize_network(l->l_cred, KAUTH_NETWORK_INTERFACE_TUN,
	    KAUTH_REQ_NETWORK_INTERFACE_TUN_ADD, NULL, NULL, NULL);
	if (error)
		return error;

	tp = tun_find_unit(dev);

	if (tp == NULL) {
		(void)tun_clone_create(&tun_cloner, minor(dev));
		tp = tun_find_unit(dev);
		if (tp == NULL) {
			return ENXIO;
		}
	}

	if (tp->tun_flags & TUN_OPEN) {
		mutex_exit(&tp->tun_lock);
		return EBUSY;
	}

	ifp = &tp->tun_if;
	tp->tun_flags |= TUN_OPEN;
	TUNDEBUG("%s: open\n", ifp->if_xname);
	if_link_state_change(ifp, LINK_STATE_UP);

	mutex_exit(&tp->tun_lock);

	return error;
}

/*
 * tunclose - close the device - mark i/f down & delete
 * routing info
 */
int
tunclose(dev_t dev, int flag, int mode,
    struct lwp *l)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;

	if ((tp = tun_find_zunit(minor(dev))) != NULL) {
		/* interface was "destroyed" before the close */
		tun_free(tp);
		return 0;
	}

	if ((tp = tun_find_unit(dev)) == NULL)
		goto out_nolock;

	ifp = &tp->tun_if;

	tp->tun_flags &= ~TUN_OPEN;

	tp->tun_pgid = 0;
	selnotify(&tp->tun_rsel, 0, NOTE_SUBMIT);

	TUNDEBUG ("%s: closed\n", ifp->if_xname);
	mutex_exit(&tp->tun_lock);

	/*
	 * junk all pending output
	 */
	IFQ_PURGE(&ifp->if_snd);

	if (ifp->if_flags & IFF_UP) {
		if_down(ifp);
		if (ifp->if_flags & IFF_RUNNING) {
			/* find internet addresses and delete routes */
			struct ifaddr *ifa;
			IFADDR_READER_FOREACH(ifa, ifp) {
#if defined(INET) || defined(INET6)
				if (ifa->ifa_addr->sa_family == AF_INET ||
				    ifa->ifa_addr->sa_family == AF_INET6) {
					rtinit(ifa, (int)RTM_DELETE,
					       tp->tun_flags & TUN_DSTADDR
							? RTF_HOST
							: 0);
				}
#endif
			}
		}
	}

	if_link_state_change(ifp, LINK_STATE_DOWN);

out_nolock:
	return 0;
}

static void
tun_enable(struct tun_softc *tp, const struct ifaddr *ifa)
{
	struct ifnet	*ifp = &tp->tun_if;

	TUNDEBUG("%s: %s\n", __func__, ifp->if_xname);

	mutex_enter(&tp->tun_lock);
	tp->tun_flags &= ~(TUN_IASET|TUN_DSTADDR);

	switch (ifa->ifa_addr->sa_family) {
#ifdef INET
	case AF_INET: {
		struct sockaddr_in *sin;

		sin = satosin(ifa->ifa_addr);
		if (sin && sin->sin_addr.s_addr)
			tp->tun_flags |= TUN_IASET;

		if (ifp->if_flags & IFF_POINTOPOINT) {
			sin = satosin(ifa->ifa_dstaddr);
			if (sin && sin->sin_addr.s_addr)
				tp->tun_flags |= TUN_DSTADDR;
		}
		break;
	    }
#endif
#ifdef INET6
	case AF_INET6: {
		struct sockaddr_in6 *sin;

		sin = satosin6(ifa->ifa_addr);
		if (!IN6_IS_ADDR_UNSPECIFIED(&sin->sin6_addr))
			tp->tun_flags |= TUN_IASET;

		if (ifp->if_flags & IFF_POINTOPOINT) {
			sin = satosin6(ifa->ifa_dstaddr);
			if (sin && !IN6_IS_ADDR_UNSPECIFIED(&sin->sin6_addr))
				tp->tun_flags |= TUN_DSTADDR;
		} else
			tp->tun_flags &= ~TUN_DSTADDR;
		break;
	    }
#endif /* INET6 */
	default:
		break;
	}
	ifp->if_flags |= IFF_UP | IFF_RUNNING;
	mutex_exit(&tp->tun_lock);
}

/*
 * Process an ioctl request.
 */
static int
tun_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct tun_softc *tp = (struct tun_softc *)(ifp->if_softc);
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;

	switch (cmd) {
	case SIOCINITIFADDR:
		tun_enable(tp, ifa);
		ifa->ifa_rtrequest = p2p_rtrequest;
		TUNDEBUG("%s: address set\n", ifp->if_xname);
		break;
	case SIOCSIFBRDADDR:
		TUNDEBUG("%s: broadcast address set\n", ifp->if_xname);
		break;
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > TUNMTU || ifr->ifr_mtu < 576) {
			error = EINVAL;
			break;
		}
		TUNDEBUG("%s: interface mtu set\n", ifp->if_xname);
		if ((error = ifioctl_common(ifp, cmd, data)) == ENETRESET)
			error = 0;
		break;
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (ifr == NULL) {
	        	error = EAFNOSUPPORT;           /* XXX */
			break;
		}
		switch (ifreq_getaddr(cmd, ifr)->sa_family) {
#ifdef INET
		case AF_INET:
			break;
#endif
#ifdef INET6
		case AF_INET6:
			break;
#endif
		default:
			error = EAFNOSUPPORT;
			break;
		}
		break;
	default:
		error = ifioctl_common(ifp, cmd, data);
	}

	return error;
}

/*
 * tun_output - queue packets from higher level ready to put out.
 */
static int
tun_output(struct ifnet *ifp, struct mbuf *m0, const struct sockaddr *dst,
    const struct rtentry *rt)
{
	struct tun_softc *tp = ifp->if_softc;
	int		error;
#if defined(INET) || defined(INET6)
	int		mlen;
	uint32_t	*af;
#endif

	mutex_enter(&tp->tun_lock);
	TUNDEBUG ("%s: tun_output\n", ifp->if_xname);

	if ((tp->tun_flags & TUN_READY) != TUN_READY) {
		TUNDEBUG ("%s: not ready 0%o\n", ifp->if_xname,
			  tp->tun_flags);
		error = EHOSTDOWN;
		mutex_exit(&tp->tun_lock);
		goto out;
	}
	// XXXrmind
	mutex_exit(&tp->tun_lock);

	/*
	 * if the queueing discipline needs packet classification,
	 * do it before prepending link headers.
	 */
	IFQ_CLASSIFY(&ifp->if_snd, m0, dst->sa_family);

	bpf_mtap_af(ifp, dst->sa_family, m0, BPF_D_OUT);

	if ((error = pfil_run_hooks(ifp->if_pfil, &m0, ifp, PFIL_OUT)) != 0)
		goto out;
	if (m0 == NULL)
		goto out;

	switch(dst->sa_family) {
#ifdef INET6
	case AF_INET6:
#endif
#ifdef INET
	case AF_INET:
#endif
#if defined(INET) || defined(INET6)
		if (tp->tun_flags & TUN_PREPADDR) {
			/* Simple link-layer header */
			M_PREPEND(m0, dst->sa_len, M_DONTWAIT);
			if (m0 == NULL) {
				IF_DROP(&ifp->if_snd);
				error = ENOBUFS;
				goto out;
			}
			memcpy(mtod(m0, char *), dst, dst->sa_len);
		}

		if (tp->tun_flags & TUN_IFHEAD) {
			/* Prepend the address family */
			M_PREPEND(m0, sizeof(*af), M_DONTWAIT);
			if (m0 == NULL) {
				IF_DROP(&ifp->if_snd);
				error = ENOBUFS;
				goto out;
			}
			af = mtod(m0,uint32_t *);
			*af = htonl(dst->sa_family);
		} else {
#ifdef INET
			if (dst->sa_family != AF_INET)
#endif
			{
				error = EAFNOSUPPORT;
				goto out;
			}
		}
		/* FALLTHROUGH */
	case AF_UNSPEC:
		mlen = m0->m_pkthdr.len;
		IFQ_ENQUEUE(&ifp->if_snd, m0, error);
		if (error) {
			if_statinc(ifp, if_collisions);
			error = EAFNOSUPPORT;
			m0 = NULL;
			goto out;
		}
		if_statadd2(ifp, if_opackets, 1, if_obytes, mlen);
		break;
#endif
	default:
		error = EAFNOSUPPORT;
		goto out;
	}

	mutex_enter(&tp->tun_lock);
	cv_broadcast(&tp->tun_cv);
	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		softint_schedule(tp->tun_isih);
	selnotify(&tp->tun_rsel, 0, NOTE_SUBMIT);
	mutex_exit(&tp->tun_lock);
out:
	if (error && m0)
		m_freem(m0);

	return error;
}

static void
tun_i_softintr(void *cookie)
{
	struct tun_softc *tp = cookie;

	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_IN, POLLIN|POLLRDNORM,
		    NULL);
}

static void
tun_o_softintr(void *cookie)
{
	struct tun_softc *tp = cookie;

	if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
		fownsignal(tp->tun_pgid, SIGIO, POLL_OUT, POLLOUT|POLLWRNORM,
		    NULL);
}

/*
 * the cdevsw interface is now pretty minimal.
 */
int
tunioctl(dev_t dev, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct tun_softc *tp;
	int error = 0;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL) {
		return ENXIO;
	}

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
			if (tp->tun_if.if_flags & IFF_UP) {
				error = EBUSY;
				goto out;
			}
			tp->tun_if.if_flags &=
				~(IFF_BROADCAST|IFF_POINTOPOINT|IFF_MULTICAST);
			tp->tun_if.if_flags |= *(int *)data;
			break;
		default:
			error = EINVAL;
			goto out;
		}
		break;

	case TUNSLMODE:
		if (*(int *)data) {
			tp->tun_flags |= TUN_PREPADDR;
			tp->tun_flags &= ~TUN_IFHEAD;
		} else
			tp->tun_flags &= ~TUN_PREPADDR;
		break;

	case TUNSIFHEAD:
		if (*(int *)data) {
			tp->tun_flags |= TUN_IFHEAD;
			tp->tun_flags &= ~TUN_PREPADDR;
		} else
			tp->tun_flags &= ~TUN_IFHEAD;
		break;

	case TUNGIFHEAD:
		*(int *)data = (tp->tun_flags & TUN_IFHEAD);
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
		if (tp->tun_if.if_snd.ifq_head)
			*(int *)data = tp->tun_if.if_snd.ifq_head->m_pkthdr.len;
		else
			*(int *)data = 0;
		break;

	case TIOCSPGRP:
	case FIOSETOWN:
		error = fsetown(&tp->tun_pgid, cmd, data);
		break;

	case TIOCGPGRP:
	case FIOGETOWN:
		error = fgetown(tp->tun_pgid, cmd, data);
		break;

	default:
		error = ENOTTY;
	}

out:
	mutex_exit(&tp->tun_lock);

	return error;
}

/*
 * The cdevsw read interface - reads a packet at a time, or at
 * least as much of a packet as can be read.
 */
int
tunread(dev_t dev, struct uio *uio, int ioflag)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	struct mbuf	*m, *m0;
	int		error = 0, len;

	tp = tun_find_unit(dev);

	/* interface was "destroyed" already */
	if (tp == NULL) {
		return ENXIO;
	}

	ifp = &tp->tun_if;

	TUNDEBUG ("%s: read\n", ifp->if_xname);
	if ((tp->tun_flags & TUN_READY) != TUN_READY) {
		TUNDEBUG ("%s: not ready 0%o\n", ifp->if_xname, tp->tun_flags);
		error = EHOSTDOWN;
		goto out;
	}

	do {
		IFQ_DEQUEUE(&ifp->if_snd, m0);
		if (m0 == 0) {
			if (tp->tun_flags & TUN_NBIO) {
				error = EWOULDBLOCK;
				goto out;
			}
			if (cv_wait_sig(&tp->tun_cv, &tp->tun_lock)) {
				error = EINTR;
				goto out;
			}
		}
	} while (m0 == 0);

	mutex_exit(&tp->tun_lock);

	/* Copy the mbuf chain */
	while (m0 && uio->uio_resid > 0 && error == 0) {
		len = uimin(uio->uio_resid, m0->m_len);
		if (len != 0)
			error = uiomove(mtod(m0, void *), len, uio);
		m0 = m = m_free(m0);
	}

	if (m0) {
		TUNDEBUG("Dropping mbuf\n");
		m_freem(m0);
	}
	if (error)
		if_statinc(ifp, if_ierrors);

	return error;

out:
	mutex_exit(&tp->tun_lock);

	return error;
}

/*
 * the cdevsw write interface - an atomic write is a packet - or else!
 */
int
tunwrite(dev_t dev, struct uio *uio, int ioflag)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	struct mbuf	*top, **mp, *m;
	pktqueue_t	*pktq;
	struct sockaddr	dst;
	int		error = 0, tlen, mlen;
	uint32_t	family;

	tp = tun_find_unit(dev);
	if (tp == NULL) {
		/* Interface was "destroyed" already. */
		return ENXIO;
	}

	/* Unlock until we've got the data */
	mutex_exit(&tp->tun_lock);

	ifp = &tp->tun_if;

	TUNDEBUG("%s: tunwrite\n", ifp->if_xname);

	if (tp->tun_flags & TUN_PREPADDR) {
		if (uio->uio_resid < sizeof(dst)) {
			error = EIO;
			goto out0;
		}
		error = uiomove((void *)&dst, sizeof(dst), uio);
		if (error)
			goto out0;
		if (dst.sa_len > sizeof(dst)) {
			/* Duh.. */
			int n = dst.sa_len - sizeof(dst);
			while (n--) {
				char discard;
				error = uiomove(&discard, 1, uio);
				if (error) {
					goto out0;
				}
			}
		}
	} else if (tp->tun_flags & TUN_IFHEAD) {
		if (uio->uio_resid < sizeof(family)){
			error = EIO;
			goto out0;
		}
		error = uiomove((void *)&family, sizeof(family), uio);
		if (error)
			goto out0;
		dst.sa_family = ntohl(family);
	} else {
#ifdef INET
		dst.sa_family = AF_INET;
#endif
	}

	if (uio->uio_resid == 0 || uio->uio_resid > TUNMTU) {
		TUNDEBUG("%s: len=%lu!\n", ifp->if_xname,
		    (unsigned long)uio->uio_resid);
		error = EIO;
		goto out0;
	}

	switch (dst.sa_family) {
#ifdef INET
	case AF_INET:
		pktq = ip_pktq;
		break;
#endif
#ifdef INET6
	case AF_INET6:
		pktq = ip6_pktq;
		break;
#endif
	default:
		error = EAFNOSUPPORT;
		goto out0;
	}

	tlen = uio->uio_resid;

	/* get a header mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		error = ENOBUFS;
		goto out0;
	}
	mlen = MHLEN;

	top = NULL;
	mp = &top;
	while (error == 0 && uio->uio_resid > 0) {
		m->m_len = uimin(mlen, uio->uio_resid);
		error = uiomove(mtod(m, void *), m->m_len, uio);
		*mp = m;
		mp = &m->m_next;
		if (error == 0 && uio->uio_resid > 0) {
			MGET(m, M_DONTWAIT, MT_DATA);
			if (m == NULL) {
				error = ENOBUFS;
				break;
			}
			mlen = MLEN;
		}
	}
	if (error) {
		if (top != NULL)
			m_freem(top);
		if_statinc(ifp, if_ierrors);
		goto out0;
	}

	top->m_pkthdr.len = tlen;
	m_set_rcvif(top, ifp);

	bpf_mtap_af(ifp, dst.sa_family, top, BPF_D_IN);

	if ((error = pfil_run_hooks(ifp->if_pfil, &top, ifp, PFIL_IN)) != 0)
		goto out0;
	if (top == NULL)
		goto out0;

	mutex_enter(&tp->tun_lock);
	if ((tp->tun_flags & TUN_INITED) == 0) {
		/* Interface was destroyed */
		error = ENXIO;
		goto out;
	}
	kpreempt_disable();
	if (__predict_false(!pktq_enqueue(pktq, top, 0))) {
		if_statinc(ifp, if_collisions);
		mutex_exit(&tp->tun_lock);
		error = ENOBUFS;
		m_freem(top);
		goto out0;
	}
	kpreempt_enable();
	if_statadd2(ifp, if_ipackets, 1, if_ibytes, tlen);
out:
	mutex_exit(&tp->tun_lock);
out0:
	return error;
}

#ifdef ALTQ
/*
 * Start packet transmission on the interface.
 * when the interface queue is rate-limited by ALTQ or TBR,
 * if_start is needed to drain packets from the queue in order
 * to notify readers when outgoing packets become ready.
 */
static void
tunstart(struct ifnet *ifp)
{
	struct tun_softc *tp = ifp->if_softc;

	if (!ALTQ_IS_ENABLED(&ifp->if_snd) && !TBR_IS_ENABLED(&ifp->if_snd))
		return;

	mutex_enter(&tp->tun_lock);
	if (!IF_IS_EMPTY(&ifp->if_snd)) {
		cv_broadcast(&tp->tun_cv);
		if (tp->tun_flags & TUN_ASYNC && tp->tun_pgid)
			softint_schedule(tp->tun_osih);

		selnotify(&tp->tun_rsel, 0, NOTE_SUBMIT);
	}
	mutex_exit(&tp->tun_lock);
}
#endif /* ALTQ */
/*
 * tunpoll - the poll interface, this is only useful on reads
 * really. The write detect always returns true, write never blocks
 * anyway, it either accepts the packet or drops it.
 */
int
tunpoll(dev_t dev, int events, struct lwp *l)
{
	struct tun_softc *tp;
	struct ifnet	*ifp;
	int revents = 0;

	tp = tun_find_unit(dev);
	if (tp == NULL) {
		/* Interface was "destroyed" already. */
		return 0;
	}
	ifp = &tp->tun_if;

	TUNDEBUG("%s: tunpoll\n", ifp->if_xname);

	if (events & (POLLIN | POLLRDNORM)) {
		if (!IFQ_IS_EMPTY(&ifp->if_snd)) {
			TUNDEBUG("%s: tunpoll q=%d\n", ifp->if_xname,
			    ifp->if_snd.ifq_len);
			revents |= events & (POLLIN | POLLRDNORM);
		} else {
			TUNDEBUG("%s: tunpoll waiting\n", ifp->if_xname);
			selrecord(l, &tp->tun_rsel);
		}
	}

	if (events & (POLLOUT | POLLWRNORM))
		revents |= events & (POLLOUT | POLLWRNORM);

	mutex_exit(&tp->tun_lock);

	return revents;
}

static void
filt_tunrdetach(struct knote *kn)
{
	struct tun_softc *tp = kn->kn_hook;

	mutex_enter(&tp->tun_lock);
	selremove_knote(&tp->tun_rsel, kn);
	mutex_exit(&tp->tun_lock);
}

static int
filt_tunread(struct knote *kn, long hint)
{
	struct tun_softc *tp = kn->kn_hook;
	struct ifnet *ifp = &tp->tun_if;
	struct mbuf *m;
	int ready;

	if (hint & NOTE_SUBMIT)
		KASSERT(mutex_owned(&tp->tun_lock));
	else
		mutex_enter(&tp->tun_lock);

	IF_POLL(&ifp->if_snd, m);
	ready = (m != NULL);
	for (kn->kn_data = 0; m != NULL; m = m->m_next)
		kn->kn_data += m->m_len;

	if (hint & NOTE_SUBMIT)
		KASSERT(mutex_owned(&tp->tun_lock));
	else
		mutex_exit(&tp->tun_lock);

	return ready;
}

static const struct filterops tunread_filtops = {
	.f_flags = FILTEROP_ISFD,
	.f_attach = NULL,
	.f_detach = filt_tunrdetach,
	.f_event = filt_tunread,
};

int
tunkqfilter(dev_t dev, struct knote *kn)
{
	struct tun_softc *tp;
	int rv = 0;

	tp = tun_find_unit(dev);
	if (tp == NULL)
		goto out_nolock;

	switch (kn->kn_filter) {
	case EVFILT_READ:
		kn->kn_fop = &tunread_filtops;
		kn->kn_hook = tp;
		selrecord_knote(&tp->tun_rsel, kn);
		break;

	case EVFILT_WRITE:
		kn->kn_fop = &seltrue_filtops;
		break;

	default:
		rv = EINVAL;
		goto out;
	}

out:
	mutex_exit(&tp->tun_lock);
out_nolock:
	return rv;
}

/*
 * Module infrastructure
 */
#include "if_module.h"

IF_MODULE(MODULE_CLASS_DRIVER, tun, NULL)
