/*	$NetBSD: if_agr.c,v 1.1.8.6 2008/01/21 09:47:08 yamt Exp $	*/

/*-
 * Copyright (c)2005 YAMAMOTO Takashi,
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
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_agr.c,v 1.1.8.6 2008/01/21 09:47:08 yamt Exp $");

#include "bpfilter.h"
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/callout.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/sockio.h>
#include <sys/proc.h>	/* XXX for curproc */
#include <sys/kauth.h>

#if NBPFILTER > 0
#include <net/bpf.h>
#endif
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>

#if defined(INET)
#include <netinet/in.h>
#include <netinet/if_inarp.h>
#endif

#include <net/agr/if_agrvar.h>
#include <net/agr/if_agrvar_impl.h>
#include <net/agr/if_agrioctl.h>
#include <net/agr/if_agrsubr.h>
#include <net/agr/if_agrethervar.h>

void agrattach(int);

static int agr_clone_create(struct if_clone *, int);
static int agr_clone_destroy(struct ifnet *);
static void agr_start(struct ifnet *);
static int agr_setconfig(struct ifnet *, const struct agrreq *);
static int agr_getconfig(struct ifnet *, struct agrreq *);
static int agr_getportlist(struct ifnet *, struct agrreq *);
static int agr_addport(struct ifnet *, struct ifnet *);
static int agr_remport(struct ifnet *, struct ifnet *);
static int agrreq_copyin(const void *, struct agrreq *);
static int agrreq_copyout(void *, struct agrreq *);
static int agr_ioctl(struct ifnet *, u_long, void *);
static struct agr_port *agr_select_tx_port(struct agr_softc *, struct mbuf *);
static int agr_ioctl_filter(struct ifnet *, u_long, void *);
static void agr_reset_iftype(struct ifnet *);
static int agr_config_promisc(struct agr_softc *);
static int agrport_config_promisc_callback(struct agr_port *, void *);
static int agrport_config_promisc(struct agr_port *, bool);
static int agrport_cleanup(struct agr_softc *, struct agr_port *);

static struct if_clone agr_cloner =
    IF_CLONE_INITIALIZER("agr", agr_clone_create, agr_clone_destroy);

/*
 * EXPORTED FUNCTIONS
 */

/*
 * agrattch: device attach routine.
 */

void
agrattach(int count)
{

	if_clone_attach(&agr_cloner);
}

/*
 * agr_input: frame collector.
 */

void
agr_input(struct ifnet *ifp_port, struct mbuf *m)
{
	struct agr_port *port;
	struct ifnet *ifp;

	port = ifp_port->if_agrprivate;
	KASSERT(port);
	ifp = port->port_agrifp;
	if ((port->port_flags & AGRPORT_COLLECTING) == 0) {
		m_freem(m);
		ifp->if_ierrors++;
		return;
	}

	ifp->if_ipackets++;
	m->m_pkthdr.rcvif = ifp;

#if NBPFILTER > 0
	if (ifp->if_bpf) {
		bpf_mtap(ifp->if_bpf, m);
	}
#endif

	(*ifp->if_input)(ifp, m);
}

/*
 * EXPORTED AGR-INTERNAL FUNCTIONS
 */

void
agr_lock(struct agr_softc *sc)
{

	mutex_enter(&sc->sc_lock);
}

void
agr_unlock(struct agr_softc *sc)
{

	mutex_exit(&sc->sc_lock);
}

void
agr_ioctl_lock(struct agr_softc *sc)
{

	mutex_enter(&sc->sc_ioctl_lock);
}

void
agr_ioctl_unlock(struct agr_softc *sc)
{

	mutex_exit(&sc->sc_ioctl_lock);
}

/*
 * agr_xmit_frame: transmit a pre-built frame.
 */

int
agr_xmit_frame(struct ifnet *ifp_port, struct mbuf *m)
{
	int error;

	struct sockaddr_storage dst0;
	struct sockaddr *dst;
	int hdrlen;

	/*
	 * trim off link level header and let if_output re-add it.
	 * XXX better to introduce an API to transmit pre-built frames.
	 */

	hdrlen = ifp_port->if_hdrlen;
	if (m->m_pkthdr.len < hdrlen) {
		m_freem(m);
		return EINVAL;
	}
	memset(&dst0, 0, sizeof(dst0));
	dst = (struct sockaddr *)&dst0;
	dst->sa_family = pseudo_AF_HDRCMPLT;
	dst->sa_len = hdrlen;
	m_copydata(m, 0, hdrlen, &dst->sa_data);
	m_adj(m, hdrlen);

	error = (*ifp_port->if_output)(ifp_port, m, dst, NULL);

	return error;
}

int
agrport_ioctl(struct agr_port *port, u_long cmd, void *arg)
{
	struct ifnet *ifp = port->port_ifp;

	KASSERT(ifp->if_agrprivate == (void *)port);
	KASSERT(ifp->if_ioctl == agr_ioctl_filter);

	return (*port->port_ioctl)(ifp, cmd, arg);
}

/*
 * INTERNAL FUNCTIONS
 */

static int
agr_clone_create(struct if_clone *ifc, int unit)
{
	struct agr_softc *sc;
	struct ifnet *ifp;

	sc = agr_alloc_softc();
	TAILQ_INIT(&sc->sc_ports);
	mutex_init(&sc->sc_ioctl_lock, MUTEX_DRIVER, IPL_NONE);
	mutex_init(&sc->sc_lock, MUTEX_DRIVER, IPL_NET);
	agrtimer_init(sc);
	ifp = &sc->sc_if;
	snprintf(ifp->if_xname, sizeof(ifp->if_xname), "%s%d",
	    ifc->ifc_name, unit);

	ifp->if_softc = sc;
	ifp->if_flags = IFF_BROADCAST | IFF_SIMPLEX | IFF_MULTICAST;
	ifp->if_start = agr_start;
	ifp->if_ioctl = agr_ioctl;
	IFQ_SET_READY(&ifp->if_snd);

	if_attach(ifp);

	agr_reset_iftype(ifp);

	return 0;
}

static void
agr_reset_iftype(struct ifnet *ifp)
{

	ifp->if_type = IFT_OTHER;
	ifp->if_dlt = DLT_NULL;
	ifp->if_addrlen = 0;
	if_alloc_sadl(ifp);
}

static int
agr_clone_destroy(struct ifnet *ifp)
{
	struct agr_softc *sc = ifp->if_softc;
	int error;

	agr_ioctl_lock(sc);

	AGR_LOCK(sc);
	if (sc->sc_nports > 0) {
		error = EBUSY;
	} else {
		error = 0;
	}
	AGR_UNLOCK(sc);

	agr_ioctl_unlock(sc);

	if (error == 0) {
		if_detach(ifp);
		mutex_destroy(&sc->sc_ioctl_lock);
		mutex_destroy(&sc->sc_lock);
		agr_free_softc(sc);
	}

	return error;
}

static struct agr_port *
agr_select_tx_port(struct agr_softc *sc, struct mbuf *m)
{

	return (*sc->sc_iftop->iftop_select_tx_port)(sc, m);
}

#if 0 /* "generic" version */
static struct agr_port *
agr_select_tx_port(struct agr_softc *sc, struct mbuf *m)
{
	struct agr_port *port;
	uint32_t hash;

	hash = (*sc->sc_iftop->iftop_hashmbuf)(sc, m);
	if (sc->sc_nports == 0)
		return NULL;
	hash %= sc->sc_nports;
	port = TAILQ_FIRST(&sc->sc_ports);
	KASSERT(port != NULL);
	while (hash--) {
		port = TAILQ_NEXT(port, port_q);
		KASSERT(port != NULL);
	}

	return port;
}
#endif /* 0 */

static void
agr_start(struct ifnet *ifp)
{
	struct agr_softc *sc = ifp->if_softc;
	struct mbuf *m;

	AGR_LOCK(sc);

	while (/* CONSTCOND */ 1) {
		struct agr_port *port;

		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL) {
			break;
		}
#if NBPFILTER > 0
		if (ifp->if_bpf) {
			bpf_mtap(ifp->if_bpf, m);
		}
#endif
		port = agr_select_tx_port(sc, m);
		if (port) {
			int error;

			error = agr_xmit_frame(port->port_ifp, m);
			if (error) {
				ifp->if_oerrors++;
			} else {
				ifp->if_opackets++;
			}
		} else {
			m_freem(m);
			ifp->if_oerrors++;
		}
	}

	AGR_UNLOCK(sc);

	ifp->if_flags &= ~IFF_OACTIVE;
}

static int
agr_setconfig(struct ifnet *ifp, const struct agrreq *ar)
{
	int cmd = ar->ar_cmd;
	struct ifnet *ifp_port;
	int error = 0;
	char ifname[IFNAMSIZ];

	memset(ifname, 0, sizeof(ifname));
	error = copyin(ar->ar_buf, ifname,
	    MIN(ar->ar_buflen, sizeof(ifname) - 1));
	if (error) {
		return error;
	}
	ifp_port = ifunit(ifname);
	if (ifp_port == NULL) {
		return ENOENT;
	}

	switch (cmd) {
	case AGRCMD_ADDPORT:
		error = agr_addport(ifp, ifp_port);
		break;

	case AGRCMD_REMPORT:
		error = agr_remport(ifp, ifp_port);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
agr_getportlist(struct ifnet *ifp, struct agrreq *ar)
{
	struct agr_softc *sc = ifp->if_softc;
	struct agr_port *port;
	struct agrportlist apl;
	struct agrportinfo api;
	char *cp = ar->ar_buf;
	size_t bufleft = (cp == NULL) ? 0 : ar->ar_buflen;
	int error;

	if (cp != NULL) {
		memset(&apl, 0, sizeof(apl));
		memset(&api, 0, sizeof(api));

		if (bufleft < sizeof(apl)) {
			return E2BIG;
		}
		apl.apl_nports = sc->sc_nports;
		error = copyout(&apl, cp, sizeof(apl));
		if (error) {
			return error;
		}
		cp += sizeof(apl);
	}
	bufleft -= sizeof(apl);

	TAILQ_FOREACH(port, &sc->sc_ports, port_q) {
		if (cp != NULL) {
			if (bufleft < sizeof(api)) {
				return E2BIG;
			}
			memcpy(api.api_ifname, port->port_ifp->if_xname,
			    sizeof(api.api_ifname));
			api.api_flags = 0;
			if (port->port_flags & AGRPORT_COLLECTING) {
				api.api_flags |= AGRPORTINFO_COLLECTING;
			}
			if (port->port_flags & AGRPORT_DISTRIBUTING) {
				api.api_flags |= AGRPORTINFO_DISTRIBUTING;
			}
			error = copyout(&api, cp, sizeof(api));
			if (error) {
				return error;
			}
			cp += sizeof(api);
		}
		bufleft -= sizeof(api);
	}

	if (cp == NULL) {
		ar->ar_buflen = -bufleft; /* necessary buffer size */
	}

	return 0;
}

static int
agr_getconfig(struct ifnet *ifp, struct agrreq *ar)
{
	int cmd = ar->ar_cmd;
	int error;

	switch (cmd) {
	case AGRCMD_PORTLIST:
		error = agr_getportlist(ifp, ar);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

static int
agr_addport(struct ifnet *ifp, struct ifnet *ifp_port)
{
	const struct ifaddr *ifa;
	struct agr_softc *sc = ifp->if_softc;
	struct agr_port *port = NULL;
	int error = 0;

	if (ifp_port->if_ioctl == NULL) {
		error = EOPNOTSUPP;
		goto out;
	}

	if (ifp_port->if_agrprivate) {
		error = EBUSY;
		goto out;
	}

	if (ifp_port->if_start == agr_start) {
		error = EINVAL;
		goto out;
	}

	port = malloc(sizeof(*port) + ifp_port->if_addrlen, M_DEVBUF,
	    M_WAITOK | M_ZERO);
	if (port == NULL) {
		error = ENOMEM;
		goto out;
	}
	port->port_flags = AGRPORT_LARVAL;

	IFADDR_FOREACH(ifa, ifp_port) {
		if (ifa->ifa_addr->sa_family != AF_LINK) {
			error = EBUSY;
			goto out;
		}
	}

	if (sc->sc_nports == 0) {
		switch (ifp_port->if_type) {
		case IFT_ETHER:
			sc->sc_iftop = &agrether_ops;
			break;

		default:
			error = EPROTONOSUPPORT; /* XXX */
			goto out;
		}

		error = (*sc->sc_iftop->iftop_ctor)(sc, ifp_port);
		if (error)
			goto out;
		agrtimer_start(sc);
	} else {
		if (ifp->if_type != ifp_port->if_type) {
			error = EINVAL;
			goto out;
		}
		if (ifp->if_addrlen != ifp_port->if_addrlen) {
			error = EINVAL;
			goto out;
		}
	}

	memcpy(port->port_origlladdr, CLLADDR(ifp_port->if_sadl),
	    ifp_port->if_addrlen);

	/*
	 * start to modify ifp_port.
	 */

	error = (*ifp_port->if_ioctl)(ifp_port, SIOCSIFADDR,
	    (void *)ifp->if_dl);

	if (error) {
		printf("%s: SIOCSIFADDR error %d\n", __func__, error);
		goto cleanup;
	}
	port->port_flags |= AGRPORT_LADDRCHANGED;

	ifp->if_type = ifp_port->if_type;
	AGR_LOCK(sc);

	port->port_ifp = ifp_port;
	ifp_port->if_agrprivate = port;
	port->port_agrifp = ifp;
	TAILQ_INSERT_TAIL(&sc->sc_ports, port, port_q);
	sc->sc_nports++;

	port->port_ioctl = ifp_port->if_ioctl;
	ifp_port->if_ioctl = agr_ioctl_filter;

	port->port_flags |= AGRPORT_ATTACHED;

	AGR_UNLOCK(sc);

	error = (*sc->sc_iftop->iftop_portinit)(sc, port);
	if (error) {
		printf("%s: portinit error %d\n", __func__, error);
		goto cleanup;
	}

	ifp->if_flags |= IFF_RUNNING;

	agrport_config_promisc(port, (ifp->if_flags & IFF_PROMISC) != 0);
	error = (*sc->sc_iftop->iftop_configmulti_port)(sc, port, true);
	if (error) {
		printf("%s: configmulti error %d\n", __func__, error);
		goto cleanup;
	}

	AGR_LOCK(sc);
	port->port_flags &= ~AGRPORT_LARVAL;
	AGR_UNLOCK(sc);
out:
	if (error && port) {
		free(port, M_DEVBUF);
	}
	return error;

cleanup:
	if (agrport_cleanup(sc, port)) {
		printf("%s: error on cleanup\n", __func__);

		port = NULL; /* XXX */
	}

	if (sc->sc_nports == 0) {
		KASSERT(TAILQ_EMPTY(&sc->sc_ports));
		agrtimer_stop(sc);
		(*sc->sc_iftop->iftop_dtor)(sc);
		sc->sc_iftop = NULL;
		agr_reset_iftype(ifp);
	} else {
		KASSERT(!TAILQ_EMPTY(&sc->sc_ports));
	}

	goto out;
}

static int
agr_remport(struct ifnet *ifp, struct ifnet *ifp_port)
{
	struct agr_softc *sc = ifp->if_softc;
	struct agr_port *port;
	int error = 0;

	if (ifp_port->if_agrprivate == NULL) {
		error = ENOENT;
		return error;
	}

	port = ifp_port->if_agrprivate;
	if (port->port_agrifp != ifp) {
		error = EINVAL;
		return error;
	}

	KASSERT(sc->sc_nports > 0);

	AGR_LOCK(sc);
	port->port_flags |= AGRPORT_DETACHING;
	AGR_UNLOCK(sc);

	error = (*sc->sc_iftop->iftop_portfini)(sc, port);
	if (error) {
		/* XXX XXX */
		printf("%s: portfini error %d\n", __func__, error);
		goto out;
	}

	error = (*sc->sc_iftop->iftop_configmulti_port)(sc, port, false);
	if (error) {
		/* XXX XXX */
		printf("%s: configmulti_port error %d\n", __func__, error);
		goto out;
	}

	error = agrport_cleanup(sc, port);
	if (error) {
		/* XXX XXX */
		printf("%s: agrport_cleanup error %d\n", __func__, error);
		goto out;
	}

	free(port, M_DEVBUF);

out:
	if (sc->sc_nports == 0) {
		KASSERT(TAILQ_EMPTY(&sc->sc_ports));
		agrtimer_stop(sc);
		(*sc->sc_iftop->iftop_dtor)(sc);
		sc->sc_iftop = NULL;
		/* XXX should purge all addresses? */
		agr_reset_iftype(ifp);
	} else {
		KASSERT(!TAILQ_EMPTY(&sc->sc_ports));
	}

	return error;
}

static int
agrport_cleanup(struct agr_softc *sc, struct agr_port *port)
{
	struct ifnet *ifp_port = port->port_ifp;
	int error;
	int result = 0;

	error = agrport_config_promisc(port, false);
	if (error) {
		printf("%s: config_promisc error %d\n", __func__, error);
		result = error;
	}

	if ((port->port_flags & AGRPORT_LADDRCHANGED)) {
#if 0
		memcpy(LLADDR(ifp_port->if_sadl), port->port_origlladdr,
		    ifp_port->if_addrlen);
		if (ifp_port->if_init != NULL) {
			error = (*ifp_port->if_init)(ifp_port);
		}
#else
		union {
			struct sockaddr sa;
			struct sockaddr_dl sdl;
			struct sockaddr_storage ss;
		} u;
		struct ifaddr ifa;

		sockaddr_dl_init(&u.sdl, sizeof(u.ss),
		    0, ifp_port->if_type, NULL, 0,
		    port->port_origlladdr, ifp_port->if_addrlen);
		memset(&ifa, 0, sizeof(ifa));
		ifa.ifa_addr = &u.sa;
		error = agrport_ioctl(port, SIOCSIFADDR, &ifa);
#endif
		if (error) {
			printf("%s: if_init error %d\n", __func__, error);
			result = error;
		} else {
			port->port_flags &= ~AGRPORT_LADDRCHANGED;
		}
	}

	AGR_LOCK(sc);
	if ((port->port_flags & AGRPORT_ATTACHED)) {
		ifp_port->if_agrprivate = NULL;

		TAILQ_REMOVE(&sc->sc_ports, port, port_q);
		sc->sc_nports--;

		KASSERT(ifp_port->if_ioctl == agr_ioctl_filter);
		ifp_port->if_ioctl = port->port_ioctl;

		port->port_flags &= ~AGRPORT_ATTACHED;
	}
	AGR_UNLOCK(sc);

	return result;
}

static int
agr_ioctl_multi(struct ifnet *ifp, u_long cmd, struct ifreq *ifr)
{
	struct agr_softc *sc = ifp->if_softc;
	int error;

	error = (*sc->sc_iftop->iftop_configmulti_ifreq)(sc, ifr,
	    (cmd == SIOCADDMULTI));

	return error;
}

/* XXX an incomplete hack; can't filter ioctls handled ifioctl(). */
static int
agr_ioctl_filter(struct ifnet *ifp, u_long cmd, void *arg)
{
	struct agr_port *port = ifp->if_agrprivate;
	int error;

	KASSERT(port);

	switch (cmd) {
	case SIOCGIFADDR:
	case SIOCGIFMEDIA:
	case SIOCSIFFLAGS: /* XXX */
		error = agrport_ioctl(port, cmd, arg);
		break;
	default:
		error = EBUSY;
		break;
	}
	return error;
}

static int
agrreq_copyin(const void *ubuf, struct agrreq *ar)
{
	int error;
			
	error = copyin(ubuf, ar, sizeof(*ar));
	if (error) {
		return error;
	}

	if (ar->ar_version != AGRREQ_VERSION) {
		return EINVAL;
	}

	return 0;
}

static int
agrreq_copyout(void *ubuf, struct agrreq *ar)
{
	int error;
			
	KASSERT(ar->ar_version == AGRREQ_VERSION);

	error = copyout(ar, ubuf, sizeof(*ar));
	if (error) {
		return error;
	}

	return 0;
}

static int
agr_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct agr_softc *sc = ifp->if_softc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	struct sockaddr *sa;
	struct agrreq ar;
	int error = 0;
	int s;

	agr_ioctl_lock(sc);

	s = splnet();

	switch (cmd) {
	case SIOCSIFADDR:
		if (sc->sc_nports == 0) {
			error = EINVAL;
			break;
		}
		ifp->if_flags |= IFF_UP;
		switch (ifa->ifa_addr->sa_family) {
#if defined(INET)
		case AF_INET:
			arp_ifinit(ifp, ifa);
			break;
#endif
		default:
			break;
		}
		break;

	case SIOCGIFADDR:
		sa = (struct sockaddr *)&ifr->ifr_data;
		memcpy(sa->sa_data, CLLADDR(ifp->if_sadl), ifp->if_addrlen);
		break;

#if 0 /* notyet */
	case SIOCSIFMTU:
#endif

	case SIOCSIFFLAGS:
		agr_config_promisc(sc);
		break;

	case SIOCSETAGR:
		splx(s);
		error = kauth_authorize_network(kauth_cred_get(),
		    KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL);
		if (!error) {
			error = agrreq_copyin(ifr->ifr_data, &ar);
		}
		if (!error) {
			error = agr_setconfig(ifp, &ar);
		}
		s = splnet();
		break;

	case SIOCGETAGR:
		splx(s);
		error = agrreq_copyin(ifr->ifr_data, &ar);
		if (!error) {
			error = agr_getconfig(ifp, &ar);
		}
		if (!error) {
			error = agrreq_copyout(ifr->ifr_data, &ar);
		}
		s = splnet();
		break;

	case SIOCADDMULTI:
	case SIOCDELMULTI:
		if (sc->sc_nports == 0) {
			error = EINVAL;
			break;
		}
		error = agr_ioctl_multi(ifp, cmd, ifr);
		break;

	default:
		error = EINVAL;
		break;
	}

	splx(s);

	agr_ioctl_unlock(sc);

	return error;
}

static int
agr_config_promisc(struct agr_softc *sc)
{
	int error;

	agr_port_foreach(sc, agrport_config_promisc_callback, &error);

	return error;
}

static int
agrport_config_promisc_callback(struct agr_port *port, void *arg)
{
	struct agr_softc *sc = AGR_SC_FROM_PORT(port);
	int *errorp = arg;
	int error;
	bool promisc;

	promisc = (sc->sc_if.if_flags & IFF_PROMISC) != 0;

	error = agrport_config_promisc(port, promisc);
	if (error) {
		*errorp = error;
	}

	return 0;
}

static int
agrport_config_promisc(struct agr_port *port, bool promisc)
{
	int error;

	if (( promisc && (port->port_flags & AGRPORT_PROMISC) != 0) ||
	    (!promisc && (port->port_flags & AGRPORT_PROMISC) == 0)) {
		return 0;
	}

	error = ifpromisc(port->port_ifp, promisc);
	if (error == 0) {
		if (promisc) {
			port->port_flags |= AGRPORT_PROMISC;
		} else {
			port->port_flags &= ~AGRPORT_PROMISC;
		}
	}

	return error;
}
