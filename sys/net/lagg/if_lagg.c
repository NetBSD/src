/*	$NetBSD: if_lagg.c,v 1.5 2021/06/16 00:21:19 riastradh Exp $	*/

/*
 * Copyright (c) 2005, 2006 Reyk Floeter <reyk@openbsd.org>
 * Copyright (c) 2007 Andrew Thompson <thompsa@FreeBSD.org>
 * Copyright (c) 2014, 2016 Marcelo Araujo <araujo@FreeBSD.org>
 * Copyright (c) 2021, Internet Initiative Japan Inc.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: if_lagg.c,v 1.5 2021/06/16 00:21:19 riastradh Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_lagg.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/cprng.h>
#include <sys/device.h>
#include <sys/evcnt.h>
#include <sys/hash.h>
#include <sys/kmem.h>
#include <sys/module.h>
#include <sys/pserialize.h>
#include <sys/pslist.h>
#include <sys/psref.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/if_vlanvar.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#if defined(INET) || defined(INET6)
#include <netinet/in.h>
#endif

#ifdef INET6
#include <netinet6/in6_ifattach.h>
#include <netinet6/in6_var.h>
#endif

#include <net/lagg/if_lagg.h>
#include <net/lagg/if_laggvar.h>
#include <net/lagg/if_laggproto.h>

#include "ioconf.h"

enum lagg_portctrl {
	LAGG_PORTCTRL_ALLOC,
	LAGG_PORTCTRL_FREE,
	LAGG_PORTCTRL_START,
	LAGG_PORTCTRL_STOP
};

enum lagg_iftypes {
	LAGG_IF_TYPE_ETHERNET,
};

static const struct lagg_proto lagg_protos[] = {
	[LAGG_PROTO_NONE] = {
		.pr_num = LAGG_PROTO_NONE,
		.pr_attach = lagg_none_attach,
		.pr_up = lagg_none_up,
	},
	[LAGG_PROTO_LACP] = {
		.pr_num = LAGG_PROTO_LACP,
		.pr_attach = lacp_attach,
		.pr_detach = lacp_detach,
		.pr_up = lacp_up,
		.pr_down = lacp_down,
		.pr_transmit = lacp_transmit,
		.pr_input = lacp_input,
		.pr_allocport = lacp_allocport,
		.pr_freeport = lacp_freeport,
		.pr_startport = lacp_startport,
		.pr_stopport = lacp_stopport,
		.pr_protostat = lacp_protostat,
		.pr_portstat = lacp_portstat,
		.pr_linkstate = lacp_linkstate,
		.pr_ioctl = lacp_ioctl,
	},
	[LAGG_PROTO_FAILOVER] = {
		.pr_num = LAGG_PROTO_FAILOVER,
		.pr_attach = lagg_fail_attach,
		.pr_detach = lagg_common_detach,
		.pr_transmit = lagg_fail_transmit,
		.pr_input = lagg_fail_input,
		.pr_allocport = lagg_common_allocport,
		.pr_freeport = lagg_common_freeport,
		.pr_startport = lagg_common_startport,
		.pr_stopport = lagg_common_stopport,
		.pr_portstat = lagg_fail_portstat,
		.pr_linkstate = lagg_common_linkstate,
		.pr_ioctl = lagg_fail_ioctl,
	},
	[LAGG_PROTO_LOADBALANCE] = {
		.pr_num = LAGG_PROTO_LOADBALANCE,
		.pr_attach = lagg_lb_attach,
		.pr_detach = lagg_common_detach,
		.pr_transmit = lagg_lb_transmit,
		.pr_input = lagg_lb_input,
		.pr_allocport = lagg_common_allocport,
		.pr_freeport = lagg_common_freeport,
		.pr_startport = lagg_lb_startport,
		.pr_stopport = lagg_lb_stopport,
		.pr_portstat = lagg_lb_portstat,
		.pr_linkstate = lagg_common_linkstate,
	},
};

static struct mbuf *
		lagg_input_ethernet(struct ifnet *, struct mbuf *);
static int	lagg_clone_create(struct if_clone *, int);
static int	lagg_clone_destroy(struct ifnet *);
static int	lagg_init(struct ifnet *);
static int	lagg_init_locked(struct lagg_softc *);
static void	lagg_stop(struct ifnet *, int);
static void	lagg_stop_locked(struct lagg_softc *);
static int	lagg_ioctl(struct ifnet *, u_long, void *);
static int	lagg_transmit(struct ifnet *, struct mbuf *);
static void	lagg_start(struct ifnet *);
static int	lagg_media_change(struct ifnet *);
static void	lagg_media_status(struct ifnet *, struct ifmediareq *);
static int	lagg_vlan_cb(struct ethercom *, uint16_t, bool);
static void	lagg_linkstate_changed(void *);
static void	lagg_ifdetach(void *);
static struct lagg_softc *
		lagg_softc_alloc(enum lagg_iftypes);
static void	lagg_softc_free(struct lagg_softc *);
static int	lagg_setup_sysctls(struct lagg_softc *);
static void	lagg_teardown_sysctls(struct lagg_softc *);
static int	lagg_proto_attach(struct lagg_softc *, lagg_proto,
		    struct lagg_proto_softc **);
static void	lagg_proto_detach(struct lagg_variant *);
static int	lagg_proto_up(struct lagg_softc *);
static void	lagg_proto_down(struct lagg_softc *);
static int	lagg_proto_allocport(struct lagg_softc *, struct lagg_port *);
static void	lagg_proto_freeport(struct lagg_softc *, struct lagg_port *);
static void	lagg_proto_startport(struct lagg_softc *,
		    struct lagg_port *);
static void	lagg_proto_stopport(struct lagg_softc *,
		    struct lagg_port *);
static struct mbuf *
		lagg_proto_input(struct lagg_softc *, struct lagg_port *,
		    struct mbuf *);
static void	lagg_proto_linkstate(struct lagg_softc *, struct lagg_port *);
static int	lagg_proto_ioctl(struct lagg_softc *, struct lagg_req *);
static int	lagg_get_stats(struct lagg_softc *, struct lagg_req *, size_t);
static int	lagg_pr_attach(struct lagg_softc *, lagg_proto);
static void	lagg_pr_detach(struct lagg_softc *);
static int	lagg_addport(struct lagg_softc *, struct ifnet *);
static int	lagg_delport(struct lagg_softc *, struct ifnet *);
static void	lagg_delport_all(struct lagg_softc *);
static int	lagg_port_ioctl(struct ifnet *, u_long, void *);
static int	lagg_port_output(struct ifnet *, struct mbuf *,
		    const struct sockaddr *, const struct rtentry *);
static int	lagg_config_promisc(struct lagg_softc *, struct lagg_port *);
static struct lagg_variant *
		lagg_variant_getref(struct lagg_softc *, struct psref *);
static void	lagg_variant_putref(struct lagg_variant *, struct psref *);
static int	lagg_ether_addmulti(struct lagg_softc *, struct ifreq *);
static int	lagg_ether_delmulti(struct lagg_softc *, struct ifreq *);
static void	lagg_port_syncmulti(struct lagg_softc *, struct lagg_port *);
static void	lagg_port_purgemulti(struct lagg_softc *, struct lagg_port *);
static void	lagg_port_syncvlan(struct lagg_softc *, struct lagg_port *);
static void	lagg_port_purgevlan(struct lagg_softc *, struct lagg_port *);
static void	lagg_lladdr_update_work(struct lagg_work *, void *);
static void	lagg_lladdr_update_ports(struct lagg_softc *);

static struct if_clone	 lagg_cloner =
    IF_CLONE_INITIALIZER("lagg", lagg_clone_create, lagg_clone_destroy);
static unsigned int	 lagg_count;
static struct psref_class
		*lagg_psref_class __read_mostly;
static struct psref_class
		*lagg_port_psref_class __read_mostly;

static enum lagg_iftypes
		 lagg_iftype = LAGG_IF_TYPE_ETHERNET;

#ifdef LAGG_DEBUG
#define LAGG_DPRINTF(_sc, _fmt, _args...)	do {	\
	printf("%s: " _fmt, (_sc) != NULL ?		\
	sc->sc_if.if_xname : "lagg", ##_args);		\
} while (0)
#else
#define LAGG_DPRINTF(_sc, _fmt, _args...)	__nothing
#endif

static inline size_t
lagg_sizeof_softc(enum lagg_iftypes ift)
{
	struct lagg_softc *_dummy = NULL;
	size_t s;

	s = sizeof(*_dummy) - sizeof(_dummy->sc_if);

	switch (ift) {
	case LAGG_IF_TYPE_ETHERNET:
		s += sizeof(struct ethercom);
		break;
	default:
		s += sizeof(struct ifnet);
		break;
	}

	return s;
}

static inline bool
lagg_debug_enable(struct lagg_softc *sc)
{
	if (__predict_false(ISSET(sc->sc_if.if_flags, IFF_DEBUG)))
		return true;

	return false;
}

static inline void
lagg_evcnt_attach(struct lagg_softc *sc,
    struct evcnt *ev, const char *name)
{

	evcnt_attach_dynamic(ev, EVCNT_TYPE_MISC, NULL,
	    sc->sc_evgroup, name);
}

static inline bool
lagg_lladdr_is_default(struct lagg_softc *sc)
{

	if (memcmp(sc->sc_lladdr, sc->sc_lladdr_default,
	    sizeof(sc->sc_lladdr)) == 0)
		return true;

	return false;
}

static inline void
lagg_in6_ifattach(struct ifnet *ifp)
{

#ifdef INET6
	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	if (in6_present) {
		if (ISSET(ifp->if_flags, IFF_UP))
			in6_ifattach(ifp, NULL);
	}
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
#endif
}

static inline void
lagg_in6_ifdetach(struct ifnet *ifp)
{

#ifdef INET6
	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	if (in6_present) {
		in6_ifdetach(ifp);
	}
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
#endif
}

static inline int
lagg_lp_ioctl(struct lagg_port *lp, u_long cmd, void *data)
{
	struct ifnet *ifp_port;
	int error;

	if (lp->lp_ioctl == NULL)
		return EINVAL;

	ifp_port = lp->lp_ifp;
	IFNET_LOCK(ifp_port);
	error = lp->lp_ioctl(ifp_port, cmd, data);
	IFNET_UNLOCK(ifp_port);

	return error;
}

void
laggattach(int n)
{

	/*
	 * Nothing to do here, initialization is handled by the
	 * module initialization code in lagginit() below).
	 */
}

static void
lagginit(void)
{
	size_t i;

	lagg_psref_class = psref_class_create("laggvariant", IPL_SOFTNET);
	lagg_port_psref_class = psref_class_create("laggport", IPL_SOFTNET);

	for (i = 0; i < LAGG_PROTO_MAX; i++) {
		if (lagg_protos[i].pr_init != NULL)
			lagg_protos[i].pr_init();
	}

	lagg_input_ethernet_p = lagg_input_ethernet;
	if_clone_attach(&lagg_cloner);
}

static int
laggdetach(void)
{
	size_t i;

	if (lagg_count > 0)
		return EBUSY;

	if_clone_detach(&lagg_cloner);
	lagg_input_ethernet_p = NULL;

	for (i = 0; i < LAGG_PROTO_MAX; i++) {
		if (lagg_protos[i].pr_fini != NULL)
			lagg_protos[i].pr_fini();
	}

	psref_class_destroy(lagg_port_psref_class);
	psref_class_destroy(lagg_psref_class);

	return 0;
}

static int
lagg_clone_create(struct if_clone *ifc, int unit)
{
	struct lagg_softc *sc;
	struct ifnet *ifp;
	char xnamebuf[MAXCOMLEN];
	int error;

	sc = lagg_softc_alloc(lagg_iftype);
	ifp = &sc->sc_if;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	sc->sc_psz = pserialize_create();
	SIMPLEQ_INIT(&sc->sc_ports);
	LIST_INIT(&sc->sc_mclist);
	TAILQ_INIT(&sc->sc_vtags);
	sc->sc_hash_mac = true;
	sc->sc_hash_ipaddr = true;
	sc->sc_hash_ip6addr = true;
	sc->sc_hash_tcp = true;
	sc->sc_hash_udp = true;

	if_initname(ifp, ifc->ifc_name, unit);
	ifp->if_softc = sc;
	ifp->if_init = lagg_init;
	ifp->if_stop = lagg_stop;
	ifp->if_ioctl = lagg_ioctl;
	ifp->if_flags = IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST;
	ifp->if_extflags = IFEF_MPSAFE;
	ifp->if_transmit = lagg_transmit;
	ifp->if_start = lagg_start;
	IFQ_SET_READY(&ifq->if_send);

	lagg_work_set(&sc->sc_wk_lladdr,
	    lagg_lladdr_update_work, sc);
	snprintf(xnamebuf, sizeof(xnamebuf),
	    "%s_wq", ifp->if_xname);
	sc->sc_wq = lagg_workq_create(xnamebuf,
	    PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (sc->sc_wq == NULL) {
		error = ENOMEM;
		goto destroy_lock;
	}

	error = lagg_setup_sysctls(sc);
	if (error != 0)
		goto destroy_psz;

	/*XXX dependent on ethernet */
	ifmedia_init_with_lock(&sc->sc_media, 0, lagg_media_change,
	    lagg_media_status, &sc->sc_lock);
	ifmedia_add(&sc->sc_media, IFM_ETHER | IFM_AUTO, 0, NULL);
	ifmedia_set(&sc->sc_media, IFM_ETHER | IFM_AUTO);

	if_initialize(ifp);

	switch (lagg_iftype) {
	case LAGG_IF_TYPE_ETHERNET:
		cprng_fast(sc->sc_lladdr_default,
		    sizeof(sc->sc_lladdr_default));
		memcpy(sc->sc_lladdr, sc->sc_lladdr_default,
		    sizeof(sc->sc_lladdr));
		ether_set_vlan_cb((struct ethercom *)ifp, lagg_vlan_cb);
		ether_ifattach(ifp, sc->sc_lladdr);
		break;
	default:
		panic("unknown if type");
	}

	snprintf(sc->sc_evgroup, sizeof(sc->sc_evgroup),
	    "%s", ifp->if_xname);
	lagg_evcnt_attach(sc, &sc->sc_novar, "no lagg variant");
	if_link_state_change(&sc->sc_if, LINK_STATE_DOWN);
	lagg_setup_sysctls(sc);
	(void)lagg_pr_attach(sc, LAGG_PROTO_NONE);
	if_register(ifp);
	lagg_count++;

	return 0;

destroy_psz:
	pserialize_destroy(sc->sc_psz);
destroy_lock:
	mutex_destroy(&sc->sc_lock);
	lagg_softc_free(sc);

	return error;
}

static int
lagg_clone_destroy(struct ifnet *ifp)
{
	struct lagg_softc *sc = (struct lagg_softc *)ifp->if_softc;

	lagg_stop(ifp, 1);

	IFNET_LOCK(ifp);
	lagg_delport_all(sc);
	IFNET_UNLOCK(ifp);

	lagg_workq_wait(sc->sc_wq, &sc->sc_wk_lladdr);

	switch (ifp->if_type) {
	case IFT_ETHER:
		ether_ifdetach(ifp);
		KASSERT(TAILQ_EMPTY(&sc->sc_vtags));
		break;
	}

	if_detach(ifp);
	ifmedia_fini(&sc->sc_media);
	lagg_pr_detach(sc);
	evcnt_detach(&sc->sc_novar);
	lagg_teardown_sysctls(sc);

	pserialize_destroy(sc->sc_psz);
	lagg_workq_destroy(sc->sc_wq);
	mutex_destroy(&sc->sc_lock);
	lagg_softc_free(sc);

	if (lagg_count > 0)
		lagg_count--;

	return 0;
}

static int
lagg_init(struct ifnet *ifp)
{
	struct lagg_softc *sc;
	int rv;

	sc = ifp->if_softc;
	LAGG_LOCK(sc);
	rv = lagg_init_locked(sc);
	LAGG_UNLOCK(sc);

	return rv;
}

static int
lagg_init_locked(struct lagg_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;
	int rv;

	KASSERT(LAGG_LOCKED(sc));

	if (ISSET(ifp->if_flags, IFF_RUNNING))
		lagg_stop_locked(sc);

	SET(ifp->if_flags, IFF_RUNNING);

	rv = lagg_proto_up(sc);
	if (rv != 0)
		lagg_stop_locked(sc);

	return rv;
}

static void
lagg_stop(struct ifnet *ifp, int disable __unused)
{
	struct lagg_softc *sc;

	sc = ifp->if_softc;
	LAGG_LOCK(sc);
	lagg_stop_locked(sc);
	LAGG_UNLOCK(sc);
}

static void
lagg_stop_locked(struct lagg_softc *sc)
{
	struct ifnet *ifp = &sc->sc_if;

	KASSERT(LAGG_LOCKED(sc));

	if (!ISSET(ifp->if_flags, IFF_RUNNING))
		return;

	CLR(ifp->if_flags, IFF_RUNNING);
	lagg_proto_down(sc);

}

static int
lagg_config(struct lagg_softc *sc, struct lagg_req *lrq)
{
	struct ifnet *ifp_port;
	struct laggreqport *rp;
	struct lagg_port *lp;
	struct psref psref;
	size_t i;
	int error, bound;

	error = 0;
	bound = curlwp_bind();

	switch (lrq->lrq_ioctl) {
	case LAGGIOC_SETPROTO:
		if (lrq->lrq_proto >= LAGG_PROTO_MAX) {
			error = EPROTONOSUPPORT;
			break;
		}

		lagg_delport_all(sc);
		error = lagg_pr_attach(sc, lrq->lrq_proto);
		if (error != 0)
			break;

		for (i = 0; i < lrq->lrq_nports; i++) {
			rp = &lrq->lrq_reqports[i];
			ifp_port = if_get(rp->rp_portname, &psref);
			if (ifp_port == NULL) {
				error = ENOENT;
				break;	/* break for */
			}

			error = lagg_addport(sc, ifp_port);
			if_put(ifp_port, &psref);

			if (error != 0)
				break;	/* break for */
		}
		break;	/* break switch */
	case LAGGIOC_ADDPORT:
		rp = &lrq->lrq_reqports[0];
		ifp_port = if_get(rp->rp_portname, &psref);
		if (ifp_port == NULL) {
			error = ENOENT;
			break;
		}

		error = lagg_addport(sc, ifp_port);
		if_put(ifp_port, &psref);
		break;
	case LAGGIOC_DELPORT:
		rp = &lrq->lrq_reqports[0];
		ifp_port = if_get(rp->rp_portname, &psref);
		if (ifp_port == NULL) {
			error = ENOENT;
			break;
		}

		error = lagg_delport(sc, ifp_port);
		if_put(ifp_port, &psref);
		break;
	case LAGGIOC_SETPORTPRI:
		rp = &lrq->lrq_reqports[0];
		ifp_port = if_get(rp->rp_portname, &psref);
		if (ifp_port == NULL) {
			error = ENOENT;
			break;
		}

		lp = ifp_port->if_lagg;
		if (lp == NULL || lp->lp_softc != sc) {
			if_put(ifp_port, &psref);
			error = ENOENT;
			break;
		}

		lp->lp_prio = rp->rp_prio;

		/* restart protocol */
		LAGG_LOCK(sc);
		lagg_proto_stopport(sc, lp);
		lagg_proto_startport(sc, lp);
		LAGG_UNLOCK(sc);
		if_put(ifp_port, &psref);
		break;
	case LAGGIOC_SETPROTOOPT:
		error = lagg_proto_ioctl(sc, lrq);
		break;
	default:
		error = ENOTTY;
	}

	curlwp_bindx(bound);
	return error;
}

static int
lagg_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct lagg_softc *sc;
	struct ifreq *ifr = (struct ifreq *)data;
	struct lagg_req laggreq, *laggresp;
	struct lagg_port *lp;
	size_t allocsiz, outlen, nports;
	char *outbuf;
	void *buf;
	int error = 0, rv;

	sc = ifp->if_softc;

	switch (cmd) {
	case SIOCGLAGG:
		error = copyin(ifr->ifr_data, &laggreq, sizeof(laggreq));
		if (error != 0)
			break;

		nports = sc->sc_nports;
		nports = MIN(nports, laggreq.lrq_nports);

		allocsiz = sizeof(*laggresp)
		    + sizeof(laggresp->lrq_reqports[0]) * nports;
		laggresp = kmem_zalloc(allocsiz, KM_SLEEP);

		rv = lagg_get_stats(sc, laggresp, nports);

		outbuf = (char *)laggresp;

		nports = MIN(laggresp->lrq_nports, nports);
		outlen = sizeof(*laggresp)
		    + sizeof(laggresp->lrq_reqports[0]) * nports;

		error = copyout(outbuf, ifr->ifr_data, outlen);
		kmem_free(outbuf, allocsiz);

		if (error == 0 && rv != 0)
			error = rv;

		break;
	case SIOCSLAGG:
		error = copyin(ifr->ifr_data, &laggreq, sizeof(laggreq));
		if (error != 0)
			break;

		nports = laggreq.lrq_nports;
		if (nports > 1) {
			allocsiz = sizeof(struct lagg_req)
			    + sizeof(struct laggreqport) * nports;
			buf = kmem_alloc(allocsiz, KM_SLEEP);

			error = copyin(ifr->ifr_data, buf, allocsiz);
			if (error != 0) {
				kmem_free(buf, allocsiz);
				break;
			}
		} else {
			buf = (void *)&laggreq;
			allocsiz = 0;
		}

		error = lagg_config(sc, buf);
		if (allocsiz > 0)
			kmem_free(buf, allocsiz);
		break;
	case SIOCSIFFLAGS:
		error = ifioctl_common(ifp, cmd, data);
		if (error != 0)
			break;

		switch (ifp->if_flags & (IFF_UP | IFF_RUNNING)) {
		case IFF_RUNNING:
			ifp->if_stop(ifp, 1);
			break;
		case IFF_UP:
		case IFF_UP | IFF_RUNNING:
			error = ifp->if_init(ifp);
			break;
		}

		if (error != 0)
			break;

		/* Set flags on ports too */
		LAGG_LOCK(sc);
		LAGG_PORTS_FOREACH(sc, lp) {
			(void)lagg_config_promisc(sc, lp);
		}
		LAGG_UNLOCK(sc);
		break;
	case SIOCSIFMTU:
		LAGG_LOCK(sc);
		LAGG_PORTS_FOREACH(sc, lp) {
			error = lagg_lp_ioctl(lp, cmd, (void *)ifr);

			if (error != 0) {
				lagg_log(sc, LOG_ERR,
				    "failed to change MTU to %d on port %s, "
				    "reverting all ports to original "
				    "MTU(%" PRIu64 ")\n",
				    ifr->ifr_mtu, lp->lp_ifp->if_xname,
				    ifp->if_mtu);
				break;
			}
		}

		if (error == 0) {
			ifp->if_mtu = ifr->ifr_mtu;
		} else {
			/* set every port back to the original MTU */
			ifr->ifr_mtu = ifp->if_mtu;
			LAGG_PORTS_FOREACH(sc, lp) {
				if (lp->lp_ioctl != NULL) {
					lagg_lp_ioctl(lp, cmd, (void *)ifr);
				}
			}
		}
		LAGG_UNLOCK(sc);
		break;
	case SIOCADDMULTI:
		if (sc->sc_if.if_type == IFT_ETHER) {
			error = lagg_ether_addmulti(sc, ifr);
		} else {
			error = EPROTONOSUPPORT;
		}
		break;
	case SIOCDELMULTI:
		if (sc->sc_if.if_type == IFT_ETHER) {
			error = lagg_ether_delmulti(sc, ifr);
		} else {
			error = EPROTONOSUPPORT;
		}
		break;
	default:
		error = ether_ioctl(ifp, cmd, data);
	}
	return error;
}

static int
lagg_setup_sysctls(struct lagg_softc *sc)
{
	struct sysctllog **log;
	const struct sysctlnode **rnode, *hashnode;
	const char *ifname;
	int error;

	log = &sc->sc_sysctllog;
	rnode = &sc->sc_sysctlnode;
	ifname = sc->sc_if.if_xname;

	error = sysctl_createv(log, 0, NULL, rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, ifname,
	    SYSCTL_DESCR("lagg information and settings"),
	    NULL, 0, NULL, 0, CTL_NET, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto done;

	error = sysctl_createv(log, 0, rnode, &hashnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "hash",
	    SYSCTL_DESCR("hash calculation settings"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto done;

	error = sysctl_createv(log, 0, &hashnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "macaddr",
	    SYSCTL_DESCR("use src/dst mac addresses"),
	    NULL, 0, &sc->sc_hash_mac, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto done;

	error = sysctl_createv(log, 0, &hashnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ipaddr",
	    SYSCTL_DESCR("use src/dst IPv4 addresses"),
	    NULL, 0, &sc->sc_hash_ipaddr, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto done;

	error = sysctl_createv(log, 0, &hashnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "ip6addr",
	    SYSCTL_DESCR("use src/dst IPv6 addresses"),
	    NULL, 0, &sc->sc_hash_ip6addr, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto done;

	error = sysctl_createv(log, 0, &hashnode, NULL,
	    CTLFLAG_READWRITE, CTLTYPE_BOOL, "tcp",
	    SYSCTL_DESCR("use TCP src/dst port"),
	    NULL, 0, &sc->sc_hash_tcp, 0, CTL_CREATE, CTL_EOL);
	if (error != 0)
		goto done;

	error = sysctl_createv(log, 0, &hashnode, NULL,
	   CTLFLAG_READWRITE, CTLTYPE_BOOL, "udp",
	   SYSCTL_DESCR("use UDP src/dst port"),
	   NULL, 0, &sc->sc_hash_udp, 0, CTL_CREATE, CTL_EOL);
done:
	if (error != 0) {
		lagg_log(sc, LOG_ERR, "unable to create sysctl node\n");
		sysctl_teardown(log);
	}

	return error;
}

static void
lagg_teardown_sysctls(struct lagg_softc *sc)
{

	sc->sc_sysctlnode = NULL;
	sysctl_teardown(&sc->sc_sysctllog);
}

uint32_t
lagg_hashmbuf(struct lagg_softc *sc, struct mbuf *m)
{
	union {
		struct ether_header _eh;
		struct ether_vlan_header _evl;
		struct ip _ip;
		struct ip6_hdr _ip6;
		struct tcphdr _th;
		struct udphdr _uh;
	} buf;
	const struct ether_header *eh;
	const struct ether_vlan_header *evl;
	const struct ip *ip;
	const struct ip6_hdr *ip6;
	const struct tcphdr *th;
	const struct udphdr *uh;
	uint32_t hash = HASH32_BUF_INIT;
	uint32_t flowlabel;
	uint16_t etype, vlantag;
	uint8_t proto;
	size_t off;

	KASSERT(ISSET(m->m_flags, M_PKTHDR));

	eh = lagg_m_extract(m, 0, sizeof(*eh), &buf);
	if (eh == NULL) {
		return hash;
	}
	off = ETHER_HDR_LEN;
	etype = ntohs(eh->ether_type);

	if (etype == ETHERTYPE_VLAN) {
		evl = lagg_m_extract(m, 0, sizeof(*evl), &buf);
		if (evl == NULL) {
			return hash;
		}

		vlantag = ntohs(evl->evl_tag);
		etype = ntohs(evl->evl_proto);
		off += ETHER_VLAN_ENCAP_LEN;
	} else if (vlan_has_tag(m)) {
		vlantag = vlan_get_tag(m);
	} else {
		vlantag = 0;
	}

	if (sc->sc_hash_mac) {
		hash = hash32_buf(&eh->ether_dhost,
		    sizeof(eh->ether_dhost), hash);
		hash = hash32_buf(&eh->ether_shost,
		    sizeof(eh->ether_shost), hash);
		hash = hash32_buf(&vlantag, sizeof(vlantag), hash);
	}

	switch (etype) {
	case ETHERTYPE_IP:
		ip = lagg_m_extract(m, off, sizeof(*ip), &buf);
		if (ip == NULL) {
			return hash;
		}

		if (sc->sc_hash_ipaddr) {
			hash = hash32_buf(&ip->ip_src,
			    sizeof(ip->ip_src), hash);
			hash = hash32_buf(&ip->ip_dst,
			    sizeof(ip->ip_dst), hash);
			hash = hash32_buf(&ip->ip_p, sizeof(ip->ip_p), hash);
		}
		off += ip->ip_hl << 2;
		proto = ip->ip_p;
		break;
	case ETHERTYPE_IPV6:
		ip6 = lagg_m_extract(m, off, sizeof(*ip6), &buf);
		if (ip6 == NULL) {
			return hash;
		}

		if (sc->sc_hash_ip6addr) {
			hash = hash32_buf(&ip6->ip6_src,
			    sizeof(ip6->ip6_src), hash);
			hash = hash32_buf(&ip6->ip6_dst,
			    sizeof(ip6->ip6_dst), hash);
			flowlabel = ip6->ip6_flow & IPV6_FLOWLABEL_MASK;
			hash = hash32_buf(&flowlabel, sizeof(flowlabel), hash);
		}
		proto = ip6->ip6_nxt;
		off += sizeof(*ip6);

		/* L4 header is not supported */
		return hash;
	default:
		return hash;
	}

	switch (proto) {
	case IPPROTO_TCP:
		th = lagg_m_extract(m, off, sizeof(*th), &buf);
		if (th == NULL) {
			return hash;
		}

		if (sc->sc_hash_tcp) {
			hash = hash32_buf(&th->th_sport,
			    sizeof(th->th_sport), hash);
			hash = hash32_buf(&th->th_dport,
			    sizeof(th->th_dport), hash);
		}
		break;
	case IPPROTO_UDP:
		uh = lagg_m_extract(m, off, sizeof(*uh), &buf);
		if (uh == NULL) {
			return hash;
		}

		if (sc->sc_hash_udp) {
			hash = hash32_buf(&uh->uh_sport,
			    sizeof(uh->uh_sport), hash);
			hash = hash32_buf(&uh->uh_dport,
			    sizeof(uh->uh_dport), hash);
		}
		break;
	}

	return hash;
}

static int
lagg_tx_common(struct ifnet *ifp, struct mbuf *m)
{
	struct lagg_variant *var;
	lagg_proto pr;
	struct psref psref;
	int error;

	var = lagg_variant_getref(ifp->if_softc, &psref);

	if (__predict_false(var == NULL)) {
		m_freem(m);
		if_statinc(ifp, if_oerrors);
		return ENOENT;
	}

	pr = var->lv_proto;
	if (__predict_true(lagg_protos[pr].pr_transmit != NULL)) {
		error = lagg_protos[pr].pr_transmit(var->lv_psc, m);
		/* mbuf is already freed */
	} else {
		m_freem(m);
		if_statinc(ifp, if_oerrors);
		error = ENOBUFS;
	}

	lagg_variant_putref(var, &psref);

	return error;
}

static int
lagg_transmit(struct ifnet *ifp, struct mbuf *m)
{

	return lagg_tx_common(ifp, m);
}

static void
lagg_start(struct ifnet *ifp)
{
	struct mbuf *m;

	for (;;) {
		IFQ_DEQUEUE(&ifp->if_snd, m);
		if (m == NULL)
			break;

		(void)lagg_tx_common(ifp, m);
	}
}

void
lagg_enqueue(struct lagg_softc *sc, struct lagg_port *lp, struct mbuf *m)
{
	struct ifnet *ifp;
	int len, error;
	short mflags;

	ifp = &sc->sc_if;
	len = m->m_pkthdr.len;
	mflags = m->m_flags;

	error = lagg_port_xmit(lp, m);
	if (error) {
		/* mbuf is already freed */
		if_statinc(ifp, if_oerrors);
	}

	net_stat_ref_t nsr = IF_STAT_GETREF(ifp);
	if_statinc_ref(nsr, if_opackets);
	if_statadd_ref(nsr, if_obytes, len);
	if (mflags & M_MCAST)
		if_statinc_ref(nsr, if_omcasts);
	IF_STAT_PUTREF(ifp);
}

static struct mbuf *
lagg_proto_input(struct lagg_softc *sc, struct lagg_port *lp, struct mbuf *m)
{
	struct psref psref;
	struct lagg_variant *var;
	lagg_proto pr;

	var = lagg_variant_getref(sc, &psref);

	if (var == NULL) {
		sc->sc_novar.ev_count++;
		m_freem(m);
		return NULL;
	}

	bpf_mtap((struct ifnet *)&sc->sc_if, m, BPF_D_IN);
	pr = var->lv_proto;

	if (lagg_protos[pr].pr_input != NULL) {
		m = lagg_protos[pr].pr_input(var->lv_psc, lp, m);
	} else {
		m_freem(m);
		m = NULL;
	}

	lagg_variant_putref(var, &psref);

	return m;
}

static struct mbuf *
lagg_input_ethernet(struct ifnet *ifp_port, struct mbuf *m)
{
	struct ifnet *ifp;
	struct psref psref;
	struct lagg_port *lp;
	int s;

	/* sanity check */
	s = pserialize_read_enter();
	lp = atomic_load_consume(&ifp_port->if_lagg);
	if (lp == NULL) {
		/* This interface is not a member of lagg */
		pserialize_read_exit(s);
		return m;
	}
	lagg_port_getref(lp, &psref);
	pserialize_read_exit(s);

	if (pfil_run_hooks(ifp_port->if_pfil, &m,
	    ifp_port, PFIL_IN) != 0) {
		goto out;
	}

	m = lagg_proto_input(lp->lp_softc, lp, m);
	if (m != NULL) {
		ifp = &lp->lp_softc->sc_if;
		m_set_rcvif(m, ifp);
		if_input(ifp, m);
		m = NULL;
	}

out:
	lagg_port_putref(lp, &psref);

	return m;
}

static int
lagg_media_change(struct ifnet *ifp)
{

	if (ISSET(ifp->if_flags, IFF_DEBUG))
		printf("%s: ignore media change\n", ifp->if_xname);

	return 0;
}

static void
lagg_media_status(struct ifnet *ifp, struct ifmediareq *imr)
{
	struct lagg_softc *sc;
	struct lagg_port *lp;

	sc = ifp->if_softc;

	imr->ifm_status = IFM_AVALID;
	imr->ifm_active = IFM_ETHER | IFM_AUTO;

	LAGG_LOCK(sc);
	LAGG_PORTS_FOREACH(sc, lp) {
		if (lagg_portactive(lp))
			imr->ifm_status |= IFM_ACTIVE;
	}
	LAGG_UNLOCK(sc);
}

static int
lagg_port_vlan_cb(struct lagg_port *lp,
    struct lagg_vlantag *lvt, bool set)
{
	struct ifnet *ifp_port;
	struct ethercom *ec_port;
	int error;

	if (lp->lp_ifp->if_type != IFT_ETHER)
		return 0;

	error = 0;
	ifp_port = lp->lp_ifp;
	ec_port = (struct ethercom *)ifp_port;

	if (set) {
		ec_port->ec_nvlans++;
		if (ec_port->ec_nvlans == 1) {
			IFNET_LOCK(ifp_port);
			error = ether_enable_vlan_mtu(ifp_port);
			IFNET_UNLOCK(ifp_port);

			if (error == -1) {
				error = 0;
			} else if (error != 0) {
				ec_port->ec_nvlans--;
				goto done;
			}

			if (ec_port->ec_vlan_cb != NULL) {
				error = ec_port->ec_vlan_cb(ec_port,
				    lvt->lvt_vtag, set);
				if (error != 0) {
					ec_port->ec_nvlans--;
					IFNET_LOCK(ifp_port);
					ether_disable_vlan_mtu(ifp_port);
					IFNET_UNLOCK(ifp_port);
					goto done;
				}
			}
		}
	} else {
		if (ec_port->ec_nvlans == 0) {
			error = ENOENT;
			goto done;
		}

		if (ec_port->ec_vlan_cb != NULL) {
			(void)ec_port->ec_vlan_cb(ec_port,
			    lvt->lvt_vtag, set);
		}

		ec_port->ec_nvlans--;
		if (ec_port->ec_nvlans == 0) {
			IFNET_LOCK(ifp_port);
			(void)ether_disable_vlan_mtu(ifp_port);
			IFNET_UNLOCK(ifp_port);
		}
	}

done:
	return error;
}

static int
lagg_vlan_cb(struct ethercom *ec, uint16_t vtag, bool set)
{
	struct ifnet *ifp;
	struct lagg_softc *sc;
	struct lagg_vlantag *lvt, *lvt0;
	struct lagg_port *lp;
	int error;

	ifp = (struct ifnet *)ec;
	sc = ifp->if_softc;

	if (set) {
		lvt = kmem_zalloc(sizeof(*lvt), KM_SLEEP);
		lvt->lvt_vtag = vtag;
		TAILQ_INSERT_TAIL(&sc->sc_vtags, lvt, lvt_entry);
	} else {
		TAILQ_FOREACH_SAFE(lvt, &sc->sc_vtags, lvt_entry, lvt0) {
			if (lvt->lvt_vtag == vtag) {
				TAILQ_REMOVE(&sc->sc_vtags, lvt, lvt_entry);
				break;
			}
		}

		if (lvt == NULL)
			return ENOENT;
	}

	KASSERT(lvt != NULL);
	LAGG_PORTS_FOREACH(sc, lp) {
		error = lagg_port_vlan_cb(lp, lvt, set);
		if (error != 0) {
			lagg_log(sc, LOG_WARNING,
			    "%s failed to configure vlan on %d\n",
			    lp->lp_ifp->if_xname, error);
		}
	}

	return 0;
}

static struct lagg_softc *
lagg_softc_alloc(enum lagg_iftypes ift)
{
	struct lagg_softc *sc;
	size_t s;

	s = lagg_sizeof_softc(ift);
	KASSERT(s > 0);

	sc = kmem_zalloc(s, KM_SLEEP);
	KASSERT(sc != NULL);

	return sc;
}

static void
lagg_softc_free(struct lagg_softc *sc)
{

	kmem_free(sc,
	    lagg_sizeof_softc(sc->sc_iftype));
}

static void
lagg_variant_update(struct lagg_softc *sc, struct lagg_variant *newvar)
{
	struct lagg_variant *oldvar;

	KASSERT(LAGG_LOCKED(sc));

	psref_target_init(&newvar->lv_psref, lagg_psref_class);

	oldvar = sc->sc_var;
	atomic_store_release(&sc->sc_var, newvar);
	pserialize_perform(sc->sc_psz);

	if (__predict_true(oldvar != NULL))
		psref_target_destroy(&oldvar->lv_psref, lagg_psref_class);
}

static struct lagg_variant *
lagg_variant_getref(struct lagg_softc *sc, struct psref *psref)
{
	struct lagg_variant *var;
	int s;

	s = pserialize_read_enter();
	var = atomic_load_consume(&sc->sc_var);
	if (var == NULL) {
		pserialize_read_exit(s);
		return NULL;
	}

	psref_acquire(psref, &var->lv_psref, lagg_psref_class);
	pserialize_read_exit(s);

	return var;
}

static void
lagg_variant_putref(struct lagg_variant *var, struct psref *psref)
{

	if (__predict_false(var == NULL))
		return;
	psref_release(psref, &var->lv_psref, lagg_psref_class);
}

static int
lagg_proto_attach(struct lagg_softc *sc, lagg_proto pr,
    struct lagg_proto_softc **psc)
{

	KASSERT(lagg_protos[pr].pr_attach != NULL);
	return lagg_protos[pr].pr_attach(sc, psc);
}

static void
lagg_proto_detach(struct lagg_variant *oldvar)
{
	lagg_proto pr;

	pr = oldvar->lv_proto;

	if (lagg_protos[pr].pr_detach == NULL)
		return;

	lagg_protos[pr].pr_detach(oldvar->lv_psc);
}

static int
lagg_proto_updown(struct lagg_softc *sc, bool is_up)
{
	struct lagg_variant *var;
	struct psref psref;
	lagg_proto pr;
	int error, bound;

	error = 0;
	bound = curlwp_bind();

	var = lagg_variant_getref(sc, &psref);
	if (var == NULL) {
		curlwp_bindx(bound);
		return ENXIO;
	}

	pr = var->lv_proto;

	if (is_up) {
		if (lagg_protos[pr].pr_up != NULL)
			error = lagg_protos[pr].pr_up(var->lv_psc);
		else
			error = 0;
	} else {
		if (lagg_protos[pr].pr_down != NULL)
			lagg_protos[pr].pr_down(var->lv_psc);
		else
			error = 0;
	}

	lagg_variant_putref(var, &psref);
	curlwp_bindx(bound);

	return error;
}

static int
lagg_proto_up(struct lagg_softc *sc)
{

	return lagg_proto_updown(sc, true);
}

static void
lagg_proto_down(struct lagg_softc *sc)
{

	(void)lagg_proto_updown(sc, false);
}

static int
lagg_proto_portctrl(struct lagg_softc *sc, struct lagg_port *lp,
    enum lagg_portctrl ctrl)
{
	struct lagg_variant *var;
	struct psref psref;
	lagg_proto pr;
	int error, bound;

	error = 0;
	bound = curlwp_bind();

	var = lagg_variant_getref(sc, &psref);
	if (var == NULL) {
		curlwp_bindx(bound);
		return ENXIO;
	}

	pr = var->lv_proto;

	error = EPROTONOSUPPORT;
	switch (ctrl) {
	case LAGG_PORTCTRL_ALLOC:
		if (lagg_protos[pr].pr_allocport != NULL)
			error = lagg_protos[pr].pr_allocport(var->lv_psc, lp);
		break;
	case LAGG_PORTCTRL_FREE:
		if (lagg_protos[pr].pr_freeport != NULL) {
			lagg_protos[pr].pr_freeport(var->lv_psc, lp);
			error = 0;
		}
		break;
	case LAGG_PORTCTRL_START:
		if (lagg_protos[pr].pr_startport != NULL) {
			lagg_protos[pr].pr_startport(var->lv_psc, lp);
			error = 0;
		}
		break;
	case LAGG_PORTCTRL_STOP:
		if (lagg_protos[pr].pr_stopport != NULL) {
			lagg_protos[pr].pr_stopport(var->lv_psc, lp);
			error = 0;
		}
		break;
	}

	lagg_variant_putref(var, &psref);
	curlwp_bindx(bound);
	return error;
}

static int
lagg_proto_allocport(struct lagg_softc *sc, struct lagg_port *lp)
{

	return lagg_proto_portctrl(sc, lp, LAGG_PORTCTRL_ALLOC);
}

static void
lagg_proto_freeport(struct lagg_softc *sc, struct lagg_port *lp)
{

	lagg_proto_portctrl(sc, lp, LAGG_PORTCTRL_FREE);
}

static void
lagg_proto_startport(struct lagg_softc *sc, struct lagg_port *lp)
{

	lagg_proto_portctrl(sc, lp, LAGG_PORTCTRL_START);
}

static void
lagg_proto_stopport(struct lagg_softc *sc, struct lagg_port *lp)
{

	lagg_proto_portctrl(sc, lp, LAGG_PORTCTRL_STOP);
}

static void
lagg_proto_linkstate(struct lagg_softc *sc, struct lagg_port *lp)
{
	struct lagg_variant *var;
	struct psref psref;
	lagg_proto pr;
	int bound;

	bound = curlwp_bind();
	var = lagg_variant_getref(sc, &psref);

	if (var == NULL) {
		curlwp_bindx(bound);
		return;
	}

	pr = var->lv_proto;

	if (lagg_protos[pr].pr_linkstate)
		lagg_protos[pr].pr_linkstate(var->lv_psc, lp);

	lagg_variant_putref(var, &psref);
	curlwp_bindx(bound);
}

static void
lagg_proto_stat(struct lagg_variant *var, struct laggreqproto *resp)
{
	lagg_proto pr;

	pr = var->lv_proto;

	if (lagg_protos[pr].pr_protostat != NULL)
		lagg_protos[pr].pr_protostat(var->lv_psc, resp);
}

static void
lagg_proto_portstat(struct lagg_variant *var, struct lagg_port *lp,
    struct laggreqport *resp)
{
	lagg_proto pr;

	pr = var->lv_proto;

	if (lagg_protos[pr].pr_portstat != NULL)
		lagg_protos[pr].pr_portstat(var->lv_psc, lp, resp);
}

static int
lagg_proto_ioctl(struct lagg_softc *sc, struct lagg_req *lreq)
{
	struct lagg_variant *var;
	struct psref psref;
	lagg_proto pr;
	int bound, error;

	error = ENOTTY;
	bound = curlwp_bind();
	var = lagg_variant_getref(sc, &psref);

	if (var == NULL) {
		error = ENXIO;
		goto done;
	}

	pr = var->lv_proto;
	if (pr != lreq->lrq_proto) {
		error = EBUSY;
		goto done;
	}

	if (lagg_protos[pr].pr_ioctl != NULL) {
		error = lagg_protos[pr].pr_ioctl(var->lv_psc,
		    &lreq->lrq_reqproto);
	}

done:
	if (var != NULL)
		lagg_variant_putref(var, &psref);
	curlwp_bindx(bound);
	return error;
}

static int
lagg_pr_attach(struct lagg_softc *sc, lagg_proto pr)
{
	struct lagg_variant *newvar, *oldvar;
	struct lagg_proto_softc *psc;
	bool cleanup_oldvar;
	int error;

	error = 0;
	cleanup_oldvar = false;
	newvar = kmem_alloc(sizeof(*newvar), KM_SLEEP);

	LAGG_LOCK(sc);
	oldvar = sc->sc_var;

	if (oldvar != NULL && oldvar->lv_proto == pr) {
		error = 0;
		goto done;
	}

	error = lagg_proto_attach(sc, pr, &psc);
	if (error != 0)
		goto done;

	newvar->lv_proto = pr;
	newvar->lv_psc = psc;

	lagg_variant_update(sc, newvar);
	newvar = NULL;

	if (oldvar != NULL) {
		lagg_proto_detach(oldvar);
		cleanup_oldvar = true;
	}
done:
	LAGG_UNLOCK(sc);

	if (newvar != NULL)
		kmem_free(newvar, sizeof(*newvar));
	if (cleanup_oldvar)
		kmem_free(oldvar, sizeof(*oldvar));

	return error;
}

static void
lagg_pr_detach(struct lagg_softc *sc)
{
	struct lagg_variant *var;

	LAGG_LOCK(sc);

	var = sc->sc_var;
	atomic_store_release(&sc->sc_var, NULL);
	pserialize_perform(sc->sc_psz);

	if (var != NULL)
		lagg_proto_detach(var);

	LAGG_UNLOCK(sc);

	if (var != NULL)
		kmem_free(var, sizeof(*var));
}

static void
lagg_lladdr_set(struct lagg_softc *sc, struct lagg_port *lp)
{
	struct ifnet *ifp_port;
	bool do_setsadl;

	KASSERT(LAGG_LOCKED(sc));

	switch (lp->lp_iftype) {
	case IFT_ETHER:
		break;
	default:
		return;
	}

	ifp_port = lp->lp_ifp;
	do_setsadl = false;

	IFNET_LOCK(ifp_port);
	memcpy(lp->lp_lladdr, CLLADDR(ifp_port->if_sadl), ETHER_ADDR_LEN);
	if (ifp_port->if_type != lp->lp_iftype)
		do_setsadl = true;
	IFNET_UNLOCK(ifp_port);

	if (lagg_lladdr_is_default(sc)) {
		memcpy(sc->sc_lladdr, lp->lp_lladdr, ETHER_ADDR_LEN);
		lagg_workq_add(sc->sc_wq, &sc->sc_wk_lladdr);
	} else {
		do_setsadl = true;
	}

	if (!do_setsadl)
		return;

	IFNET_LOCK(ifp_port);
	if_set_sadl(ifp_port, sc->sc_lladdr, ETHER_ADDR_LEN, false);
	IFNET_UNLOCK(ifp_port);
}

static void
lagg_lladdr_unset(struct lagg_softc *sc, struct lagg_port *lp, u_int if_type)
{
	struct ifnet *ifp_port;
	struct lagg_port *lp0;
	bool do_setsadl;
	uint8_t *lladdr;

	KASSERT(LAGG_LOCKED(sc));

	switch (lp->lp_iftype) {
	case IFT_ETHER:
		break;
	default:
		return;
	}

	do_setsadl = true;

	if (memcmp(lp->lp_lladdr, sc->sc_lladdr,
	    ETHER_ADDR_LEN) == 0) {
		if (lp->lp_iftype == if_type)
			do_setsadl = false;

		lp0 = SIMPLEQ_FIRST(&sc->sc_ports);
		if (lp0 == NULL) {
			lladdr = sc->sc_lladdr_default;
		} else {
			lladdr = lp0->lp_lladdr;
		}

		memcpy(sc->sc_lladdr, lladdr, ETHER_ADDR_LEN);
		lagg_workq_add(sc->sc_wq, &sc->sc_wk_lladdr);
		lagg_lladdr_update_ports(sc);
	}

	if (!do_setsadl)
		return;

	ifp_port = lp->lp_ifp;
	IFNET_LOCK(ifp_port);
	if_set_sadl(ifp_port, lp->lp_lladdr, ETHER_ADDR_LEN, false);
	IFNET_UNLOCK(ifp_port);
}

static void
lagg_lladdr_update_work(struct lagg_work *wk, void *xsc)
{
	struct lagg_softc *sc;
	struct ifnet *ifp;

	sc = (struct lagg_softc *)xsc;
	ifp = &sc->sc_if;

	IFNET_LOCK(ifp);
	LAGG_LOCK(sc);

	if (ifp->if_type != IFT_ETHER)
		goto out;

	if (memcmp(sc->sc_lladdr, CLLADDR(ifp->if_sadl),
	    ETHER_ADDR_LEN) == 0) {
		goto out;
	}

	if_set_sadl(ifp, sc->sc_lladdr, sizeof(sc->sc_lladdr), false);

	/* Generate new IPv6 link-local address */
	lagg_in6_ifdetach(ifp);
	lagg_in6_ifattach(ifp);
out:
	LAGG_UNLOCK(sc);
	IFNET_UNLOCK(ifp);
}

static void
lagg_lladdr_update_ports(struct lagg_softc *sc)
{
	struct lagg_port *lp;
	struct ifnet *ifp_port;
	bool stopped;
	int error;

	KASSERT(LAGG_LOCKED(sc));

	LAGG_PORTS_FOREACH(sc, lp) {
		ifp_port = lp->lp_ifp;

		if (memcmp(sc->sc_lladdr, CLLADDR(ifp_port->if_sadl),
		    ETHER_ADDR_LEN) == 0) {
			continue;
		}

		IFNET_LOCK(ifp_port);
		if (ISSET(ifp_port->if_flags, IFF_RUNNING)) {
			ifp_port->if_stop(ifp_port, 0);
			stopped = true;
		} else {
			stopped = false;
		}

		if_set_sadl(ifp_port, sc->sc_lladdr,
		    ETHER_ADDR_LEN, false);

		if (stopped) {
			error = ifp_port->if_init(ifp_port);

			if (error != 0) {
				lagg_log(sc, LOG_WARNING,
				    "%s failed to if_init() on %d\n",
				    ifp_port->if_xname, error);
			}
		}

		IFNET_UNLOCK(ifp_port);
	}
}

static int
lagg_ether_addmulti(struct lagg_softc *sc, struct ifreq *ifr)
{
	struct lagg_port *lp;
	struct lagg_mc_entry *mc;
	struct ethercom *ec;
	const struct sockaddr *sa;
	uint8_t addrlo[ETHER_ADDR_LEN], addrhi[ETHER_ADDR_LEN];
	int error;

	if (sc->sc_if.if_type != IFT_ETHER)
		return EPROTONOSUPPORT;

	ec = (struct ethercom *)&sc->sc_if;
	sa = ifreq_getaddr(SIOCADDMULTI, ifr);

	error = ether_addmulti(sa, ec);
	if (error != ENETRESET)
		return error;

	error = ether_multiaddr(sa, addrlo, addrhi);
	KASSERT(error == 0);

	mc = kmem_zalloc(sizeof(*mc), KM_SLEEP);

	ETHER_LOCK(ec);
	mc->mc_enm = ether_lookup_multi(addrlo, addrhi, ec);
	ETHER_UNLOCK(ec);

	KASSERT(mc->mc_enm != NULL);

	LAGG_LOCK(sc);
	LAGG_PORTS_FOREACH(sc, lp) {
		(void)lagg_lp_ioctl(lp, SIOCADDMULTI, (void *)ifr);
	}
	LAGG_UNLOCK(sc);

	KASSERT(sa->sa_len <= sizeof(mc->mc_addr));
	memcpy(&mc->mc_addr, sa, sa->sa_len);
	LIST_INSERT_HEAD(&sc->sc_mclist, mc, mc_entry);

	return 0;
}

static int
lagg_ether_delmulti(struct lagg_softc *sc, struct ifreq *ifr)
{
	struct lagg_port *lp;
	struct lagg_mc_entry *mc;
	const struct sockaddr *sa;
	struct ethercom *ec;
	struct ether_multi *enm;
	uint8_t addrlo[ETHER_ADDR_LEN], addrhi[ETHER_ADDR_LEN];
	int error;

	ec = (struct ethercom *)&sc->sc_if;
	sa = ifreq_getaddr(SIOCDELMULTI, ifr);
	error = ether_multiaddr(sa, addrlo, addrhi);
	if (error != 0)
		return error;

	ETHER_LOCK(ec);
	enm = ether_lookup_multi(addrlo, addrhi, ec);
	ETHER_UNLOCK(ec);

	if (enm == NULL)
		return ENOENT;

	LIST_FOREACH(mc, &sc->sc_mclist, mc_entry) {
		if (mc->mc_enm == enm)
			break;
	}

	if (mc == NULL)
		return ENOENT;

	error = ether_delmulti(sa, ec);
	if (error != ENETRESET)
		return error;

	LAGG_LOCK(sc);
	LAGG_PORTS_FOREACH(sc, lp) {
		(void)lagg_lp_ioctl(lp, SIOCDELMULTI, (void *)ifr);
	}
	LAGG_UNLOCK(sc);

	LIST_REMOVE(mc, mc_entry);
	kmem_free(mc, sizeof(*mc));

	return 0;
}

static void
lagg_port_multi(struct lagg_softc *sc, struct lagg_port *lp,
    u_long cmd)
{
	struct lagg_mc_entry *mc;
	struct ifreq ifr;
	struct ifnet *ifp_port;
	const struct sockaddr *sa;

	ifp_port = lp->lp_ifp;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp_port->if_xname, sizeof(ifr.ifr_name));

	LIST_FOREACH(mc, &sc->sc_mclist, mc_entry) {
		sa = (struct sockaddr *)&mc->mc_addr;
		KASSERT(sizeof(ifr.ifr_space) >= sa->sa_len);
		memcpy(&ifr.ifr_addr, sa, sa->sa_len);
		(void)lagg_lp_ioctl(lp, cmd, (void *)&ifr);
	}

}

static void
lagg_port_syncmulti(struct lagg_softc *sc, struct lagg_port *lp)
{

	lagg_port_multi(sc, lp, SIOCADDMULTI);
}

static void
lagg_port_purgemulti(struct lagg_softc *sc, struct lagg_port *lp)
{

	lagg_port_multi(sc, lp, SIOCDELMULTI);
}

static void
lagg_port_vlan(struct lagg_softc *sc, struct lagg_port *lp,
    bool set)
{
	struct lagg_vlantag *lvt;
	int error;

	TAILQ_FOREACH(lvt, &sc->sc_vtags, lvt_entry) {
		error = lagg_port_vlan_cb(lp, lvt, set);
		if (error != 0) {
			lagg_log(sc, LOG_WARNING,
			    "%s failed to configure vlan on %d\n",
			    lp->lp_ifp->if_xname, error);
		}
	}
}

static void
lagg_port_syncvlan(struct lagg_softc *sc, struct lagg_port *lp)

{
	lagg_port_vlan(sc, lp, true);
}

static void
lagg_port_purgevlan(struct lagg_softc *sc, struct lagg_port *lp)
{

	lagg_port_vlan(sc, lp, false);
}

static int
lagg_addport_locked(struct lagg_softc *sc, struct lagg_port *lp)
{
	struct ifnet *ifp_port;
	struct ifreq ifr;
	u_char if_type;
	uint64_t mtu_port;
	int error;
	bool stopped;

	KASSERT(IFNET_LOCKED(&sc->sc_if));
	KASSERT(LAGG_LOCKED(sc));
	KASSERT(lp != NULL);

	stopped = false;
	ifp_port = lp->lp_ifp;
	if (&sc->sc_if == ifp_port) {
		LAGG_DPRINTF(sc, "cannot add a lagg to itself as a port\n");
		return EINVAL;
	}

	if (sc->sc_nports > LAGG_MAX_PORTS) {
		return ENOSPC;
	}

	if (ifp_port->if_lagg != NULL) {
		lp = (struct lagg_port *)ifp_port->if_lagg;
		if (lp->lp_softc == sc)
			return EEXIST;
		return EBUSY;
	}

	switch (ifp_port->if_type) {
	case IFT_ETHER:
	case IFT_L2VLAN:
		if_type = IFT_IEEE8023ADLAG;
		break;
	default:
		return ENOTSUP;
	}

	mtu_port = ifp_port->if_mtu;
	if (SIMPLEQ_EMPTY(&sc->sc_ports)) {
		sc->sc_if.if_mtu = mtu_port;
	} else if (sc->sc_if.if_mtu != mtu_port) {
		if (ifp_port->if_ioctl == NULL) {
			LAGG_DPRINTF(sc, "cannot change MTU for %s\n",
			    ifp_port->if_xname);
			return EINVAL;
		}
		memset(&ifr, 0, sizeof(ifr));
		strlcpy(ifr.ifr_name, ifp_port->if_xname, sizeof(ifr.ifr_name));
		ifr.ifr_mtu = sc->sc_if.if_mtu;

		IFNET_LOCK(ifp_port);
		error = ifp_port->if_ioctl(ifp_port, SIOCSIFMTU, (void *)&ifr);
		IFNET_UNLOCK(ifp_port);

		if (error != 0) {
			LAGG_DPRINTF(sc, "invalid MTU for %s\n",
			    ifp_port->if_xname);
			return error;
		}
	}

	lp->lp_softc = sc;
	lp->lp_prio = LAGG_PORT_PRIO;
	lp->lp_linkstate_hook = if_linkstate_change_establish(ifp_port,
	    lagg_linkstate_changed, ifp_port);
	lp->lp_ifdetach_hook = ether_ifdetachhook_establish(ifp_port,
	    lagg_ifdetach, ifp_port);

	/* save and change items of ifp_port */
	IFNET_LOCK(ifp_port);
	if (ISSET(ifp_port->if_flags, IFF_RUNNING)) {
		ifp_port->if_stop(ifp_port, 0);
		stopped = true;
	}
	/* to delete ipv6 link local address */
	lagg_in6_ifdetach(ifp_port);

	lp->lp_mtu = ifp_port->if_mtu;
	lp->lp_iftype = ifp_port->if_type;
	lp->lp_ioctl = ifp_port->if_ioctl;
	lp->lp_output = ifp_port->if_output;
	lp->lp_ifcapenable = ifp_port->if_capenable;

	ifp_port->if_type = if_type;
	ifp_port->if_ioctl = lagg_port_ioctl;
	ifp_port->if_output = lagg_port_output;
	IFNET_UNLOCK(ifp_port);

	lagg_lladdr_set(sc, lp);

	psref_target_init(&lp->lp_psref, lagg_port_psref_class);

	error = lagg_proto_allocport(sc, lp);
	if (error != 0)
		goto restore_lladdr;

	lagg_port_syncmulti(sc, lp);
	lagg_port_syncvlan(sc, lp);

	atomic_store_release(&ifp_port->if_lagg, (void *)lp);
	SIMPLEQ_INSERT_TAIL(&sc->sc_ports, lp, lp_entry);
	sc->sc_nports++;

	if (stopped) {
		error = ifp_port->if_init(ifp_port);
		if (error != 0)
			goto remove_port;
	}

	lagg_proto_startport(sc, lp);

	return 0;

remove_port:
	SIMPLEQ_REMOVE(&sc->sc_ports, lp, lagg_port, lp_entry);
	sc->sc_nports--;
	atomic_store_release(&ifp_port->if_lagg, NULL);
	pserialize_perform(sc->sc_psz);
	lagg_port_purgemulti(sc, lp);
	lagg_port_purgevlan(sc, lp);

restore_lladdr:
	lagg_lladdr_unset(sc, lp, if_type);
	ether_ifdetachhook_disestablish(ifp_port,
	    lp->lp_ifdetach_hook, &sc->sc_lock);
	if_linkstate_change_disestablish(ifp_port,
	    lp->lp_linkstate_hook, NULL);
	psref_target_destroy(&lp->lp_psref, lagg_port_psref_class);

	IFNET_LOCK(ifp_port);
	ifp_port->if_type = lp->lp_iftype;
	ifp_port->if_ioctl = lp->lp_ioctl;
	ifp_port->if_output = lp->lp_output;
	if (stopped) {
		(void)ifp_port->if_init(ifp_port);
	}
	lagg_in6_ifattach(ifp_port);
	IFNET_UNLOCK(ifp_port);

	return error;
}

static int
lagg_addport(struct lagg_softc *sc, struct ifnet *ifp_port)
{
	struct lagg_port *lp;
	int error;

	lp = kmem_zalloc(sizeof(*lp), KM_SLEEP);
	lp->lp_ifp = ifp_port;

	LAGG_LOCK(sc);
	error = lagg_addport_locked(sc, lp);
	LAGG_UNLOCK(sc);

	if (error != 0)
		kmem_free(lp, sizeof(*lp));

	return error;
}

static void
lagg_delport_locked(struct lagg_softc *sc, struct lagg_port *lp,
    bool is_ifdetach)
{
	struct ifnet *ifp_port;
	struct ifreq ifr;
	u_long if_type;
	bool stopped;

	KASSERT(LAGG_LOCKED(sc));

	if (lp == NULL)
		return;

	ifp_port = lp->lp_ifp;
	stopped = false;

	SIMPLEQ_REMOVE(&sc->sc_ports, lp, lagg_port, lp_entry);
	sc->sc_nports--;
	atomic_store_release(&ifp_port->if_lagg, NULL);
	pserialize_perform(sc->sc_psz);

	if_linkstate_change_disestablish(ifp_port,
	    lp->lp_linkstate_hook, NULL);
	ether_ifdetachhook_disestablish(ifp_port,
	    lp->lp_ifdetach_hook, &sc->sc_lock);

	lagg_proto_stopport(sc, lp);
	psref_target_destroy(&lp->lp_psref, lagg_port_psref_class);

	if (ISSET(ifp_port->if_flags, IFF_RUNNING)) {
		ifp_port->if_stop(ifp_port, 0);
		stopped = true;
	}

	if (lp->lp_mtu != ifp_port->if_mtu) {
		memset(&ifr, 0, sizeof(ifr));
		strlcpy(ifr.ifr_name, ifp_port->if_xname, sizeof(ifr.ifr_name));
		ifr.ifr_mtu = lp->lp_mtu;
		(void)lagg_lp_ioctl(lp, SIOCSIFMTU, &ifr);
	}

	if (SIMPLEQ_EMPTY(&sc->sc_ports)) {
		sc->sc_if.if_mtu = 0;
	}

	lagg_port_purgemulti(sc, lp);
	lagg_port_purgevlan(sc, lp);

	IFNET_LOCK(ifp_port);
	if_type = ifp_port->if_type;
	ifp_port->if_type = lp->lp_iftype;
	ifp_port->if_ioctl = lp->lp_ioctl;
	ifp_port->if_output = lp->lp_output;
	ifp_port->if_capenable = lp->lp_ifcapenable;
	IFNET_UNLOCK(ifp_port);

	lagg_lladdr_unset(sc, lp, if_type);

	lagg_proto_freeport(sc, lp);
	kmem_free(lp, sizeof(*lp));

	if (stopped) {
		ifp_port->if_init(ifp_port);
	}

	if (!is_ifdetach) {
		IFNET_LOCK(ifp_port);
		lagg_in6_ifattach(ifp_port);
		IFNET_UNLOCK(ifp_port);
	}
}

static void
lagg_delport_all(struct lagg_softc *sc)
{
	struct lagg_port *lp, *lp0;

	LAGG_LOCK(sc);
	LAGG_PORTS_FOREACH_SAFE(sc, lp, lp0) {
		lagg_delport_locked(sc, lp, false);
	}
	LAGG_UNLOCK(sc);
}

static int
lagg_delport(struct lagg_softc *sc, struct ifnet *ifp_port)
{
	struct lagg_port *lp;

	KASSERT(IFNET_LOCKED(&sc->sc_if));

	lp = ifp_port->if_lagg;
	if (lp == NULL || lp->lp_softc != sc)
		return ENOENT;

	LAGG_LOCK(sc);
	lagg_delport_locked(sc, lp, false);
	LAGG_UNLOCK(sc);

	return 0;
}

static int
lagg_get_stats(struct lagg_softc *sc, struct lagg_req *resp,
    size_t nports)
{
	struct lagg_variant *var;
	struct lagg_port *lp;
	struct laggreqport *port;
	struct psref psref;
	struct ifnet *ifp;
	int bound;
	size_t n;

	bound = curlwp_bind();
	var = lagg_variant_getref(sc, &psref);

	resp->lrq_proto = var->lv_proto;

	lagg_proto_stat(var, &resp->lrq_reqproto);

	n = 0;
	LAGG_LOCK(sc);
	LAGG_PORTS_FOREACH(sc, lp) {
		if (n < nports) {
			port = &resp->lrq_reqports[n];

			ifp = lp->lp_ifp;
			strlcpy(port->rp_portname, ifp->if_xname,
			    sizeof(port->rp_portname));

			port->rp_prio = lp->lp_prio;
			port->rp_flags = lp->lp_flags;
			lagg_proto_portstat(var, lp, port);
		}
		n++;
	}
	LAGG_UNLOCK(sc);

	resp->lrq_nports = n;

	lagg_variant_putref(var, &psref);
	curlwp_bindx(bound);

	if (resp->lrq_nports > nports) {
		return ENOMEM;
	}
	return 0;
}

static int
lagg_config_promisc(struct lagg_softc *sc, struct lagg_port *lp)
{
	struct ifnet *ifp;
	int error;
	int status;

	ifp = &sc->sc_if;
	status = ISSET(ifp->if_flags, IFF_PROMISC) ? 1 : 0;

	error = ifpromisc(lp->lp_ifp, status);

	return error;
}

static int
lagg_port_ioctl(struct ifnet *ifp, u_long cmd, void *data)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct lagg_softc *sc;
	struct lagg_variant *var;
	struct lagg_port *lp;
	struct lagg_req laggreq;
	struct laggreqport *rp;
	struct psref psref;
	int error = 0;
	int bound;
	u_int ifflags;

	if ((lp = ifp->if_lagg) == NULL ||
	    (sc = lp->lp_softc) == NULL) {
		goto fallback;
	}

	switch (cmd) {
	case SIOCGLAGGPORT:
		error = copyin(ifr->ifr_data, &laggreq, sizeof(laggreq));
		if (error != 0)
			break;

		if (laggreq.lrq_nports != 1) {
			error = EINVAL;
			break;
		}

		rp = &laggreq.lrq_reqports[0];

		if (rp->rp_portname[0] == '\0' ||
		    ifunit(rp->rp_portname) != ifp) {
			error = EINVAL;
			break;
		}

		bound = curlwp_bind();
		var = lagg_variant_getref(sc, &psref);

		rp->rp_prio = lp->lp_prio;
		rp->rp_flags = lp->lp_flags;
		lagg_proto_portstat(var, lp, rp);
		lagg_variant_putref(var, &psref);
		curlwp_bindx(bound);
		break;
	case SIOCSIFCAP:
	case SIOCSIFMTU:
		/* Do not allow the setting to be cahanged once joined */
		error = EINVAL;
		break;
	case SIOCSIFFLAGS:
		ifflags = ifp->if_flags;
		error = LAGG_PORT_IOCTL(lp, cmd, data);
		ifflags ^= ifp->if_flags;

		if (ISSET(ifflags, IFF_RUNNING)) {
			lagg_proto_linkstate(sc, lp);
		}
		break;
	default:
		goto fallback;
	}

	return error;
fallback:
	if (lp != NULL) {
		error = LAGG_PORT_IOCTL(lp, cmd, data);
	} else {
		error = ENOTTY;
	}

	return error;
}

static int
lagg_port_output(struct ifnet *ifp, struct mbuf *m,
    const struct sockaddr *dst, const struct rtentry *rt)
{
	struct lagg_port *lp = ifp->if_lagg;
	int error = 0;

	switch (dst->sa_family) {
	case pseudo_AF_HDRCMPLT:
	case AF_UNSPEC:
		if (lp != NULL)
			error = lp->lp_output(ifp, m, dst, rt);
		else
			error = ENETDOWN;
		break;
	default:
		m_freem(m);
		error = ENETDOWN;
	}

	return error;
}

static void
lagg_ifdetach(void *xifp)
{
	struct ifnet *ifp_port;
	struct lagg_port *lp;
	struct lagg_softc *sc;
	int s;

	ifp_port = (struct ifnet *)xifp;
	IFNET_ASSERT_UNLOCKED(ifp_port);

	s = pserialize_read_enter();
	lp = atomic_load_consume(&ifp_port->if_lagg);
	if (lp == NULL) {
		pserialize_read_exit(s);
		return;
	}

	sc = lp->lp_softc;
	if (sc == NULL) {
		pserialize_read_exit(s);
		return;
	}
	pserialize_read_exit(s);

	LAGG_LOCK(sc);
	/*
	 * reload if_lagg because it may write
	 * after pserialize_read_exit.
	 */
	lp = ifp_port->if_lagg;
	lagg_delport_locked(sc, lp, true);
	LAGG_UNLOCK(sc);
}

void
lagg_linkstate_changed(void *xifp)
{
	struct ifnet *ifp = xifp;
	struct lagg_port *lp;
	struct psref psref;
	int s, bound;

	s = pserialize_read_enter();
	lp = atomic_load_consume(&ifp->if_lagg);
	if (lp != NULL) {
		bound = curlwp_bind();
		lagg_port_getref(lp, &psref);
	} else {
		pserialize_read_exit(s);
		return;
	}
	pserialize_read_exit(s);

	lagg_proto_linkstate(lp->lp_softc, lp);
	lagg_port_putref(lp, &psref);
	curlwp_bindx(bound);
}

void
lagg_port_getref(struct lagg_port *lp, struct psref *psref)
{

	psref_acquire(psref, &lp->lp_psref, lagg_port_psref_class);
}

void
lagg_port_putref(struct lagg_port *lp, struct psref *psref)
{

	psref_release(psref, &lp->lp_psref, lagg_port_psref_class);
}

void
lagg_log(struct lagg_softc *sc, int lvl, const char *fmt, ...)
{
	va_list ap;

	if (lvl == LOG_DEBUG && !lagg_debug_enable(sc))
		return;

	log(lvl, "%s: ", sc->sc_if.if_xname);
	va_start(ap, fmt);
	vlog(lvl, fmt, ap);
	va_end(ap);
}

static void
lagg_workq_work(struct work *wk, void *context)
{
	struct lagg_work *lw;

	lw = container_of(wk, struct lagg_work, lw_cookie);

	atomic_cas_uint(&lw->lw_state, LAGG_WORK_ENQUEUED, LAGG_WORK_IDLE);
	lw->lw_func(lw, lw->lw_arg);
}

struct workqueue *
lagg_workq_create(const char *name, pri_t prio, int ipl, int flags)
{
	struct workqueue *wq;
	int error;

	error = workqueue_create(&wq, name, lagg_workq_work,
	    NULL, prio, ipl, flags);

	if (error)
		return NULL;

	return wq;
}

void
lagg_workq_destroy(struct workqueue *wq)
{

	workqueue_destroy(wq);
}

void
lagg_workq_add(struct workqueue *wq, struct lagg_work *lw)
{

	if (atomic_cas_uint(&lw->lw_state, LAGG_WORK_IDLE,
	    LAGG_WORK_ENQUEUED) != LAGG_WORK_IDLE)
		return;

	KASSERT(lw->lw_func != NULL);
	kpreempt_disable();
	workqueue_enqueue(wq, &lw->lw_cookie, NULL);
	kpreempt_enable();
}

void
lagg_workq_wait(struct workqueue *wq, struct lagg_work *lw)
{

	atomic_swap_uint(&lw->lw_state, LAGG_WORK_STOPPING);
	workqueue_wait(wq, &lw->lw_cookie);
}

/*
 * Module infrastructure
 */
#include <net/if_module.h>

IF_MODULE(MODULE_CLASS_DRIVER, lagg, NULL)
