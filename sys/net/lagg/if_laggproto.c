/*	$NetBSD: if_laggproto.c,v 1.6.4.1 2024/10/03 11:53:46 martin Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause-NetBSD
 *
 * Copyright (c)2021 Internet Initiative Japan, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: if_laggproto.c,v 1.6.4.1 2024/10/03 11:53:46 martin Exp $");

#include <sys/param.h>
#include <sys/types.h>

#include <sys/evcnt.h>
#include <sys/kmem.h>
#include <sys/mbuf.h>
#include <sys/mutex.h>
#include <sys/pslist.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/lagg/if_lagg.h>
#include <net/lagg/if_laggproto.h>

struct lagg_proto_softc {
	struct lagg_softc	*psc_softc;
	struct pslist_head	 psc_ports;
	kmutex_t		 psc_lock;
	pserialize_t		 psc_psz;
	size_t			 psc_ctxsiz;
	void			*psc_ctx;
	size_t			 psc_nactports;
	struct workqueue	*psc_workq;
	struct lagg_work	 psc_work_linkspeed;
};

/*
 * Locking notes:
 * - Items of struct lagg_proto_softc is protected by
 *   psc_lock (an adaptive mutex)
 * - psc_ports is protected by pselialize (psc_psz) and
 *   it updates exclusively by LAGG_PROTO_LOCK.
 * - Other locking notes are described in if_laggproto.h
 */

struct lagg_failover {
	bool		 fo_rx_all;
};

struct lagg_portmap {
	struct lagg_port	*pm_ports[LAGG_MAX_PORTS];
	size_t			 pm_nports;
};

struct lagg_portmaps {
	struct lagg_portmap	 maps_pmap[2];
	size_t			 maps_activepmap;
};

struct lagg_lb {
	struct lagg_portmaps	 lb_pmaps;
};

struct lagg_proto_port {
	struct pslist_entry	 lpp_entry;
	struct lagg_port	*lpp_laggport;
	uint64_t		 lpp_linkspeed;
	bool			 lpp_active;
	bool			 lpp_running;
};

#define LAGG_PROTO_LOCK(_psc)	mutex_enter(&(_psc)->psc_lock)
#define LAGG_PROTO_UNLOCK(_psc)	mutex_exit(&(_psc)->psc_lock)
#define LAGG_PROTO_LOCKED(_psc)	mutex_owned(&(_psc)->psc_lock)

static struct lagg_proto_softc *
		lagg_proto_alloc(lagg_proto, struct lagg_softc *);
static void	lagg_proto_free(struct lagg_proto_softc *);
static void	lagg_proto_insert_port(struct lagg_proto_softc *,
		    struct lagg_proto_port *);
static void	lagg_proto_remove_port(struct lagg_proto_softc *,
		    struct lagg_proto_port *);
static struct lagg_port *
		lagg_link_active(struct lagg_proto_softc *psc,
		    struct lagg_proto_port *, struct psref *);
static void	lagg_fail_linkspeed_work(struct lagg_work *, void *);
static void	lagg_lb_linkspeed_work(struct lagg_work*,
		    void *);
static void	lagg_common_linkstate(struct lagg_proto_softc *,
		    struct lagg_port *);

static inline struct lagg_portmap *
lagg_portmap_active(struct lagg_portmaps *maps)
{
	size_t i;

	i = atomic_load_consume(&maps->maps_activepmap);

	return &maps->maps_pmap[i];
}

static inline struct lagg_portmap *
lagg_portmap_next(struct lagg_portmaps *maps)
{
	size_t i;

	i = atomic_load_consume(&maps->maps_activepmap);
	i ^= 0x1;

	return &maps->maps_pmap[i];
}

static inline void
lagg_portmap_switch(struct lagg_portmaps *maps)
{
	size_t i;

	i = atomic_load_consume(&maps->maps_activepmap);
	i &= 0x1;
	i ^= 0x1;

	atomic_store_release(&maps->maps_activepmap, i);
}

static struct lagg_proto_softc *
lagg_proto_alloc(lagg_proto pr, struct lagg_softc *sc)
{
	struct lagg_proto_softc *psc;
	char xnamebuf[MAXCOMLEN];
	size_t ctxsiz;

	switch (pr) {
	case LAGG_PROTO_FAILOVER:
		ctxsiz = sizeof(struct lagg_failover);
		break;
	case LAGG_PROTO_LOADBALANCE:
		ctxsiz = sizeof(struct lagg_lb);
		break;
	default:
		ctxsiz = 0;
	}

	psc = kmem_zalloc(sizeof(*psc), KM_NOSLEEP);
	if (psc == NULL)
		return NULL;

	snprintf(xnamebuf, sizeof(xnamebuf), "%s.proto",
	    sc->sc_if.if_xname);
	psc->psc_workq = lagg_workq_create(xnamebuf,
		    PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (psc->psc_workq == NULL) {
		LAGG_LOG(sc, LOG_ERR, "workqueue create failed\n");
		kmem_free(psc, sizeof(*psc));
		return NULL;
	}

	if (ctxsiz > 0) {
		psc->psc_ctx = kmem_zalloc(ctxsiz, KM_NOSLEEP);
		if (psc->psc_ctx == NULL) {
			lagg_workq_destroy(psc->psc_workq);
			kmem_free(psc, sizeof(*psc));
			return NULL;
		}

		psc->psc_ctxsiz = ctxsiz;
	}

	PSLIST_INIT(&psc->psc_ports);
	psc->psc_psz = pserialize_create();
	mutex_init(&psc->psc_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	psc->psc_softc = sc;

	return psc;
}

static void
lagg_proto_free(struct lagg_proto_softc *psc)
{

	lagg_workq_wait(psc->psc_workq, &psc->psc_work_linkspeed);
	pserialize_destroy(psc->psc_psz);
	mutex_destroy(&psc->psc_lock);
	lagg_workq_destroy(psc->psc_workq);
	PSLIST_DESTROY(&psc->psc_ports);

	if (psc->psc_ctxsiz > 0)
		kmem_free(psc->psc_ctx, psc->psc_ctxsiz);

	kmem_free(psc, sizeof(*psc));
}

static struct lagg_port *
lagg_link_active(struct lagg_proto_softc *psc,
    struct lagg_proto_port *pport, struct psref *psref)
{
	struct lagg_port *lp;
	int s;

	lp = NULL;
	s = pserialize_read_enter();

	for (;pport != NULL;
	    pport = PSLIST_READER_NEXT(pport,
	    struct lagg_proto_port, lpp_entry)) {
		if (atomic_load_relaxed(&pport->lpp_active)) {
			lp = pport->lpp_laggport;
			goto done;
		}
	}

	PSLIST_READER_FOREACH(pport, &psc->psc_ports,
	    struct lagg_proto_port, lpp_entry) {
		if (atomic_load_relaxed(&pport->lpp_active)) {
			lp = pport->lpp_laggport;
			break;
		}
	}
done:
	if (lp != NULL)
		lagg_port_getref(lp, psref);
	pserialize_read_exit(s);

	return lp;
}

int
lagg_common_allocport(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_proto_port *pport;

	KASSERT(LAGG_LOCKED(psc->psc_softc));

	pport = kmem_zalloc(sizeof(*pport), KM_NOSLEEP);
	if (pport == NULL)
		return ENOMEM;

	PSLIST_ENTRY_INIT(pport, lpp_entry);
	pport->lpp_laggport = lp;
	lp->lp_proto_ctx = (void *)pport;
	return 0;
}

void
lagg_common_freeport(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_proto_port *pport;

	pport = lp->lp_proto_ctx;
	KASSERT(!pport->lpp_running);
	lp->lp_proto_ctx = NULL;

	kmem_free(pport, sizeof(*pport));
}

static void
lagg_proto_insert_port(struct lagg_proto_softc *psc,
    struct lagg_proto_port *pport)
{
	struct lagg_proto_port *pport0;
	struct lagg_port *lp, *lp0;
	bool insert_after;

	insert_after = false;
	lp = pport->lpp_laggport;

	LAGG_PROTO_LOCK(psc);
	PSLIST_WRITER_FOREACH(pport0, &psc->psc_ports,
	    struct lagg_proto_port, lpp_entry) {
		lp0 = pport0->lpp_laggport;
		if (lp0->lp_prio > lp->lp_prio)
			break;

		if (PSLIST_WRITER_NEXT(pport0,
		    struct lagg_proto_port, lpp_entry) == NULL) {
			insert_after = true;
			break;
		}
	}

	if (pport0 == NULL) {
		PSLIST_WRITER_INSERT_HEAD(&psc->psc_ports, pport,
		    lpp_entry);
	} else if (insert_after) {
		PSLIST_WRITER_INSERT_AFTER(pport0, pport, lpp_entry);
	} else {
		PSLIST_WRITER_INSERT_BEFORE(pport0, pport, lpp_entry);
	}
	LAGG_PROTO_UNLOCK(psc);
}

static void
lagg_proto_remove_port(struct lagg_proto_softc *psc,
    struct lagg_proto_port *pport)
{

	LAGG_PROTO_LOCK(psc);
	PSLIST_WRITER_REMOVE(pport, lpp_entry);
	LAGG_PROTO_UNLOCK(psc);
	pserialize_perform(psc->psc_psz);

	/* re-initialize for reuse */
	PSLIST_ENTRY_DESTROY(pport, lpp_entry);
	PSLIST_ENTRY_INIT(pport, lpp_entry);
}

void
lagg_common_startport(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_proto_port *pport;

	pport = lp->lp_proto_ctx;
	lagg_proto_insert_port(psc, pport);

	LAGG_PROTO_LOCK(psc);
	pport->lpp_running = true;
	LAGG_PROTO_UNLOCK(psc);

	lagg_common_linkstate(psc, lp);
}

void
lagg_common_stopport(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_proto_port *pport;
	struct ifnet *ifp;

	pport = lp->lp_proto_ctx;

	LAGG_PROTO_LOCK(psc);
	pport->lpp_running = false;
	LAGG_PROTO_UNLOCK(psc);

	lagg_proto_remove_port(psc, pport);

	if (pport->lpp_active) {
		KASSERT(psc->psc_nactports > 0);
		psc->psc_nactports--;

		if (psc->psc_nactports == 0) {
			ifp = &psc->psc_softc->sc_if;
			if_link_state_change(ifp, LINK_STATE_DOWN);
		}

		pport->lpp_active = false;
	}

	lagg_workq_add(psc->psc_workq, &psc->psc_work_linkspeed);
}
static void
lagg_common_linkstate(struct lagg_proto_softc *psc, struct lagg_port *lp)
{

	IFNET_ASSERT_UNLOCKED(lp->lp_ifp);

	IFNET_LOCK(lp->lp_ifp);
	lagg_common_linkstate_ifnet_locked(psc, lp);
	IFNET_UNLOCK(lp->lp_ifp);
}

void
lagg_common_linkstate_ifnet_locked(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_proto_port *pport;
	struct ifnet *ifp, *ifp_port;
	struct ifmediareq ifmr;
	uint64_t linkspeed;
	bool is_active;
	int error;

	pport = lp->lp_proto_ctx;
	is_active = lagg_portactive(lp);
	ifp_port = lp->lp_ifp;

	KASSERT(IFNET_LOCKED(ifp_port));

	LAGG_PROTO_LOCK(psc);
	if (!pport->lpp_running ||
	    pport->lpp_active == is_active) {
		LAGG_PROTO_UNLOCK(psc);
		return;
	}

	ifp = &psc->psc_softc->sc_if;
	pport->lpp_active = is_active;

	if (is_active) {
		psc->psc_nactports++;
		if (psc->psc_nactports == 1)
			if_link_state_change(ifp, LINK_STATE_UP);
	} else {
		KASSERT(psc->psc_nactports > 0);
		psc->psc_nactports--;

		if (psc->psc_nactports == 0)
			if_link_state_change(ifp, LINK_STATE_DOWN);
	}
	LAGG_PROTO_UNLOCK(psc);

	memset(&ifmr, 0, sizeof(ifmr));
	error = if_ioctl(ifp_port, SIOCGIFMEDIA, (void *)&ifmr);
	if (error == 0) {
		linkspeed = ifmedia_baudrate(ifmr.ifm_active);
	} else {
		linkspeed = 0;
	}

	LAGG_PROTO_LOCK(psc);
	pport->lpp_linkspeed = linkspeed;
	LAGG_PROTO_UNLOCK(psc);
	lagg_workq_add(psc->psc_workq, &psc->psc_work_linkspeed);
}

void
lagg_common_detach(struct lagg_proto_softc *psc)
{

	lagg_proto_free(psc);
}

int
lagg_none_attach(struct lagg_softc *sc, struct lagg_proto_softc **pscp)
{

	*pscp = NULL;
	return 0;
}

int
lagg_fail_attach(struct lagg_softc *sc, struct lagg_proto_softc **xpsc)
{
	struct lagg_proto_softc *psc;
	struct lagg_failover *fovr;

	psc = lagg_proto_alloc(LAGG_PROTO_FAILOVER, sc);
	if (psc == NULL)
		return ENOMEM;

	fovr = psc->psc_ctx;
	fovr->fo_rx_all = true;
	lagg_work_set(&psc->psc_work_linkspeed,
	    lagg_fail_linkspeed_work, psc);

	*xpsc = psc;
	return 0;
}

int
lagg_fail_transmit(struct lagg_proto_softc *psc, struct mbuf *m)
{
	struct ifnet *ifp;
	struct lagg_port *lp;
	struct psref psref;

	lp = lagg_link_active(psc, NULL, &psref);
	if (lp == NULL) {
		ifp = &psc->psc_softc->sc_if;
		if_statinc(ifp, if_oerrors);
		m_freem(m);
		return ENOENT;
	}

	lagg_output(psc->psc_softc, lp, m);
	lagg_port_putref(lp, &psref);
	return 0;
}

struct mbuf *
lagg_fail_input(struct lagg_proto_softc *psc, struct lagg_port *lp,
    struct mbuf *m)
{
	struct lagg_failover *fovr;
	struct lagg_port *lp0;
	struct ifnet *ifp;
	struct psref psref;

	fovr = psc->psc_ctx;
	if (atomic_load_relaxed(&fovr->fo_rx_all))
		return m;

	lp0 = lagg_link_active(psc, NULL, &psref);
	if (lp0 == NULL) {
		goto drop;
	}

	if (lp0 != lp) {
		lagg_port_putref(lp0, &psref);
		goto drop;
	}

	lagg_port_putref(lp0, &psref);

	return m;
drop:
	ifp = &psc->psc_softc->sc_if;
	if_statinc(ifp, if_ierrors);
	m_freem(m);
	return NULL;
}

void
lagg_fail_portstat(struct lagg_proto_softc *psc, struct lagg_port *lp,
    struct laggreqport *resp)
{
	struct lagg_failover *fovr;
	struct lagg_proto_port *pport;
	struct lagg_port *lp0;
	struct psref psref;

	fovr = psc->psc_ctx;
	pport = lp->lp_proto_ctx;

	if (pport->lpp_active) {
		lp0 = lagg_link_active(psc, NULL, &psref);
		if (lp0 == lp) {
			SET(resp->rp_flags,
			    (LAGG_PORT_ACTIVE |
			    LAGG_PORT_COLLECTING |
			    LAGG_PORT_DISTRIBUTING));
		} else {
			if (fovr->fo_rx_all) {
				SET(resp->rp_flags,
				    LAGG_PORT_COLLECTING);
			}
		}

		if (lp0 != NULL)
			lagg_port_putref(lp0, &psref);
	}
}

int
lagg_fail_ioctl(struct lagg_proto_softc *psc, struct laggreqproto *lreq)
{
	struct lagg_failover *fovr;
	struct laggreq_fail *rpfail;
	int error;
	bool set;

	error = 0;
	fovr = psc->psc_ctx;
	rpfail = &lreq->rp_fail;

	switch (rpfail->command) {
	case LAGGIOC_FAILSETFLAGS:
	case LAGGIOC_FAILCLRFLAGS:
		set = (rpfail->command == LAGGIOC_FAILSETFLAGS) ?
			true : false;

		if (ISSET(rpfail->flags, LAGGREQFAIL_RXALL))
			fovr->fo_rx_all = set;
		break;
	default:
		error = ENOTTY;
		break;
	}

	return error;
}

void
lagg_fail_linkspeed_work(struct lagg_work *_lw __unused, void *xpsc)
{
	struct lagg_proto_softc *psc = xpsc;
	struct lagg_proto_port *pport;
	struct lagg_port *lp;
	struct psref psref;
	uint64_t linkspeed;

	kpreempt_disable();
	lp = lagg_link_active(psc, NULL, &psref);
	if (lp != NULL) {
		pport = lp->lp_proto_ctx;
		LAGG_PROTO_LOCK(psc);
		linkspeed = pport->lpp_linkspeed;
		LAGG_PROTO_UNLOCK(psc);
		lagg_port_putref(lp, &psref);
	} else {
		linkspeed = 0;
	}
	kpreempt_enable();

	LAGG_LOCK(psc->psc_softc);
	lagg_set_linkspeed(psc->psc_softc, linkspeed);
	LAGG_UNLOCK(psc->psc_softc);
}

int
lagg_lb_attach(struct lagg_softc *sc, struct lagg_proto_softc **xpsc)
{
	struct lagg_proto_softc *psc;
	struct lagg_lb *lb;

	psc = lagg_proto_alloc(LAGG_PROTO_LOADBALANCE, sc);
	if (psc == NULL)
		return ENOMEM;

	lb = psc->psc_ctx;
	lb->lb_pmaps.maps_activepmap = 0;
	lagg_work_set(&psc->psc_work_linkspeed,
	    lagg_lb_linkspeed_work, psc);

	*xpsc = psc;
	return 0;
}

void
lagg_lb_startport(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_lb *lb;
	struct lagg_portmap *pm_act, *pm_next;
	size_t n;

	lb = psc->psc_ctx;
	lagg_common_startport(psc, lp);

	LAGG_PROTO_LOCK(psc);
	pm_act = lagg_portmap_active(&lb->lb_pmaps);
	pm_next = lagg_portmap_next(&lb->lb_pmaps);

	*pm_next = *pm_act;

	n = pm_next->pm_nports;
	pm_next->pm_ports[n] = lp;

	n++;
	pm_next->pm_nports = n;

	lagg_portmap_switch(&lb->lb_pmaps);
	LAGG_PROTO_UNLOCK(psc);
	pserialize_perform(psc->psc_psz);
}

void
lagg_lb_stopport(struct lagg_proto_softc *psc, struct lagg_port *lp)
{
	struct lagg_lb *lb;
	struct lagg_portmap *pm_act, *pm_next;
	size_t i, n;

	lb = psc->psc_ctx;

	LAGG_PROTO_LOCK(psc);
	pm_act = lagg_portmap_active(&lb->lb_pmaps);
	pm_next = lagg_portmap_next(&lb->lb_pmaps);
	n = 0;

	for (i = 0; i < pm_act->pm_nports; i++) {
		if (pm_act->pm_ports[i] == lp)
			continue;

		pm_next->pm_ports[n] = pm_act->pm_ports[i];
		n++;
	}

	pm_next->pm_nports = n;

	lagg_portmap_switch(&lb->lb_pmaps);
	LAGG_PROTO_UNLOCK(psc);
	pserialize_perform(psc->psc_psz);

	lagg_common_stopport(psc, lp);
}

int
lagg_lb_transmit(struct lagg_proto_softc *psc, struct mbuf *m)
{
	struct lagg_lb *lb;
	struct lagg_portmap *pm;
	struct lagg_port *lp, *lp0;
	struct ifnet *ifp;
	struct psref psref;
	uint32_t hash;
	int s;

	lb = psc->psc_ctx;
	hash = lagg_hashmbuf(psc->psc_softc, m);

	s = pserialize_read_enter();

	pm = lagg_portmap_active(&lb->lb_pmaps);
	if (__predict_true(pm->pm_nports != 0)) {
		hash %= pm->pm_nports;
		lp0 = pm->pm_ports[hash];
		lp = lagg_link_active(psc, lp0->lp_proto_ctx, &psref);
	} else {
		lp = NULL;
	}

	pserialize_read_exit(s);

	if (__predict_false(lp == NULL)) {
		ifp = &psc->psc_softc->sc_if;
		if_statinc(ifp, if_oerrors);
		m_freem(m);
		return ENOENT;
	}

	lagg_output(psc->psc_softc, lp, m);
	lagg_port_putref(lp, &psref);

	return 0;
}

struct mbuf *
lagg_lb_input(struct lagg_proto_softc *psc __unused,
    struct lagg_port *lp __unused, struct mbuf *m)
{

	return m;
}

void
lagg_lb_portstat(struct lagg_proto_softc *psc, struct lagg_port *lp,
    struct laggreqport *resp)
{
	struct lagg_proto_port *pport;

	pport = lp->lp_proto_ctx;

	if (pport->lpp_active) {
		SET(resp->rp_flags, LAGG_PORT_ACTIVE |
		    LAGG_PORT_COLLECTING | LAGG_PORT_DISTRIBUTING);
	}
}

static void
lagg_lb_linkspeed_work(struct lagg_work *_lw __unused, void *xpsc)
{
	struct lagg_proto_softc *psc = xpsc;
	struct lagg_proto_port *pport;
	uint64_t linkspeed, l;

	linkspeed = 0;

	LAGG_PROTO_LOCK(psc); /* acquired to refer lpp_linkspeed */
	PSLIST_READER_FOREACH(pport, &psc->psc_ports,
	    struct lagg_proto_port, lpp_entry) {
		if (pport->lpp_active) {
			l = pport->lpp_linkspeed;
			linkspeed = MAX(linkspeed, l);
		}
	}
	LAGG_PROTO_UNLOCK(psc);

	LAGG_LOCK(psc->psc_softc);
	lagg_set_linkspeed(psc->psc_softc, linkspeed);
	LAGG_UNLOCK(psc->psc_softc);
}
