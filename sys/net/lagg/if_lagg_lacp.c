/*	$NetBSD: if_lagg_lacp.c,v 1.18 2022/03/31 02:04:50 yamaguchi Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause-NetBSD
 *
 * Copyright (c)2005 YAMAMOTO Takashi,
 * Copyright (c)2008 Andrew Thompson <thompsa@FreeBSD.org>
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
__KERNEL_RCSID(0, "$NetBSD: if_lagg_lacp.c,v 1.18 2022/03/31 02:04:50 yamaguchi Exp $");

#ifdef _KERNEL_OPT
#include "opt_lagg.h"
#endif

#include <sys/param.h>
#include <sys/types.h>

#include <sys/evcnt.h>
#include <sys/kmem.h>
#include <sys/pcq.h>
#include <sys/pslist.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>
#include <sys/workqueue.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>

#include <net/ether_slowprotocols.h>

#include <net/lagg/if_lagg.h>
#include <net/lagg/if_laggproto.h>
#include <net/lagg/if_lagg_lacp.h>

#define LACP_SYSTEMIDSTR_LEN	32

enum {
	LACP_TIMER_CURRENT_WHILE = 0,
	LACP_TIMER_PERIODIC,
	LACP_TIMER_WAIT_WHILE,
	LACP_NTIMER
};

enum {
	LACP_PTIMER_DISTRIBUTING = 0,
	LACP_NPTIMER
};

enum lacp_selected {
	LACP_UNSELECTED,
	LACP_STANDBY,
	LACP_SELECTED,
};

enum lacp_mux_state {
	LACP_MUX_DETACHED,
	LACP_MUX_WAITING,
	LACP_MUX_STANDBY,
	LACP_MUX_ATTACHED,
	LACP_MUX_COLLECTING,
	LACP_MUX_DISTRIBUTING,
	LACP_MUX_INIT,
};

struct lacp_aggregator_systemid {
	uint16_t	 sid_prio;
	uint16_t	 sid_key;
	uint8_t		 sid_mac[LACP_MAC_LEN];
};

struct lacp_aggregator {
	TAILQ_ENTRY(lacp_aggregator)
			 la_q;
	LIST_HEAD(, lacp_port)
			 la_ports;
	ssize_t		 la_attached_port;

	struct lacp_aggregator_systemid
			 la_sid;
};

struct lacp_portinfo {
	uint8_t		 lpi_state;
	uint16_t	 lpi_portno;
#define LACP_PORTNO_NONE	0
	uint16_t	 lpi_portprio;
};

struct lacp_port {
	struct lagg_port	*lp_laggport;
	bool			 lp_added_multi;
	int			 lp_timer[LACP_NTIMER];
	uint32_t		 lp_marker_xid;
	enum lacp_selected	 lp_selected;
	enum lacp_mux_state	 lp_mux_state;

	struct lacp_portinfo	 lp_actor;
	struct lacp_portinfo	 lp_partner;
	struct lacp_aggregator	*lp_aggregator;
	struct lacp_aggregator_systemid
				 lp_aggregator_sidbuf;
	uint32_t		 lp_media;
	int			 lp_pending;
	LIST_ENTRY(lacp_port)	 lp_entry_la;
	struct timeval		 lp_last_lacpdu;
	int			 lp_lacpdu_sent;
	bool			 lp_collector;

	unsigned int		 lp_flags;
#define LACP_PORT_NTT		__BIT(0)
#define LACP_PORT_MARK		__BIT(1)

	struct lagg_work	 lp_work_smtx;
	struct lagg_work	 lp_work_marker;
};

struct lacp_portmap {
	size_t			 pm_count;
	struct lagg_port	*pm_ports[LACP_MAX_PORTS];
};

struct lacp_softc {
	struct lagg_softc	*lsc_softc;
	kmutex_t		 lsc_lock;
	pserialize_t		 lsc_psz;
	bool			 lsc_running;
	bool			 lsc_suppress_distributing;
	int			 lsc_timer[LACP_NPTIMER];
	uint8_t			 lsc_system_mac[LACP_MAC_LEN];
	uint16_t		 lsc_system_prio;
	uint16_t		 lsc_key;
	size_t			 lsc_max_ports;
	size_t			 lsc_activemap;
	struct lacp_portmap	 lsc_portmaps[2];	/* active & idle */
	struct lacp_aggregator	*lsc_aggregator;
	TAILQ_HEAD(, lacp_aggregator)
				 lsc_aggregators;
	struct workqueue	*lsc_workq;
	struct lagg_work	 lsc_work_tick;
	struct lagg_work	 lsc_work_rcvdu;
	callout_t		 lsc_tick;
	pcq_t			*lsc_du_q;

	char			 lsc_evgroup[32];
	struct evcnt		 lsc_mgethdr_failed;
	struct evcnt		 lsc_mpullup_failed;
	struct evcnt		 lsc_badlacpdu;
	struct evcnt		 lsc_badmarkerdu;
	struct evcnt		 lsc_norcvif;
	struct evcnt		 lsc_nolaggport;
	struct evcnt		 lsc_duq_nospc;

	bool			 lsc_optimistic;
	bool			 lsc_stop_lacpdu;
	bool			 lsc_dump_du;
	bool			 lsc_multi_linkspeed;
};

/*
 * Locking notes:
 * - Items in struct lacp_softc are protected by
 *   lsc_lock (an adaptive mutex)
 * - lsc_activemap is protected by pserialize (lsc_psz)
 * - Items of struct lagg_port in lsc_portmaps are protected by
 *   protected by both pserialize (lsc_psz) and psref (lp_psref)
 * - Updates for lsc_activemap and lsc_portmaps is serialized by
 *   sc_lock in struct lagg_softc
 * - Other locking notes are described in if_laggproto.h
 */

static void	lacp_dprintf(const struct lacp_softc *,
		    const struct lacp_port *, const char *, ...)
		    __attribute__((__format__(__printf__, 3, 4)));

#ifdef LACP_DEBUG
#define LACP_DPRINTF(a)	do { lacp_dprintf a; } while (/*CONSTCOND*/ 0)
#define LACP_PEERINFO_IDSTR(_pi, _b, _bs)			\
	lacp_peerinfo_idstr(_pi, _b, _bs)
#define LACP_STATE_STR(_s, _b, _bs)		lacp_state_str(_s, _b, _bs)
#define LACP_AGGREGATOR_STR(_a, _b, _bs)			\
	lacp_aggregator_str(_a, _b, _bs)
#define __LACPDEBUGUSED
#else
#define LACP_DPRINTF(a)				__nothing
#define LACP_PEERINFO_IDSTR(_pi, _b, _bs)	__nothing
#define LACP_STATE_STR(_s, _b, _bs)		__nothing
#define LACP_AGGREGATOR_STR(_a, _b, _bs)	__nothing
#define __LACPDEBUGUSED __unused
#endif

#define LACP_LOCK(_sc)		mutex_enter(&(_sc)->lsc_lock)
#define LACP_UNLOCK(_sc)	mutex_exit(&(_sc)->lsc_lock)
#define LACP_LOCKED(_sc)	mutex_owned(&(_sc)->lsc_lock)
#define LACP_TIMER_ARM(_lacpp, _timer, _val)		\
	(_lacpp)->lp_timer[(_timer)] = (_val)
#define LACP_TIMER_DISARM(_lacpp, _timer)		\
	LACP_TIMER_ARM((_lacpp), (_timer), 0)
#define LACP_TIMER_ISARMED(_lacpp, _timer)		\
	((_lacpp)->lp_timer[(_timer)] > 0)
#define LACP_PTIMER_ARM(_sc, _timer, _val)		\
	(_sc)->lsc_timer[(_timer)] = (_val)
#define LACP_PTIMER_DISARM(_sc, _timer)			\
	LACP_PTIMER_ARM((_sc), (_timer), 0)
#define LACP_PTIMER_ISARMED(_sc, _timer)		\
	((_sc)->lsc_timer[(_timer)] > 0)
#define LACP_STATE_EQ(_s1, _s2, _mask)	(!ISSET((_s1) ^ (_s2), (_mask)))
#define LACP_PORT_XNAME(_lacpp)	(_lacpp != NULL) ?	\
	    (_lacpp)->lp_laggport->lp_ifp->if_xname : "(unknown)"
#define LACP_ISDUMPING(_sc)	(_sc)->lsc_dump_du
#define LACP_PORTMAP_ACTIVE(_sc)			\
	    atomic_load_consume(&(_sc)->lsc_activemap)
#define LACP_PORTMAP_NEXT(_sc)				\
	    (((LACP_PORTMAP_ACTIVE((_sc))) ^ 0x01) &0x01)
#define LACP_SYS_PRI(_la)	ntohs((_la)->la_sid.sid_prio)
#define LACP_TLV_PARSE(_du, _st, _name, _tlvlist)	\
	    tlv_parse(&(_du)->_name,			\
	    sizeof(_st) - offsetof(_st, _name),		\
	    (_tlvlist))

static void	lacp_tick(void *);
static void	lacp_tick_work(struct lagg_work *, void *);
static void	lacp_linkstate(struct lagg_proto_softc *, struct lagg_port *);
static uint32_t	lacp_ifmedia2lacpmedia(u_int);
static void	lacp_port_disable(struct lacp_softc *, struct lacp_port *);
static void	lacp_port_enable(struct lacp_softc *, struct lacp_port *);
static void	lacp_peerinfo_actor(struct lacp_softc *, struct lacp_port *,
		    struct lacpdu_peerinfo *);
static void	lacp_peerinfo_partner(struct lacp_port *,
		    struct lacpdu_peerinfo *);
static struct lagg_port *
		lacp_select_tx_port(struct lacp_softc *, struct mbuf *,
		    struct psref *);
static void	lacp_suppress_distributing(struct lacp_softc *);
static void	lacp_distributing_timer(struct lacp_softc *);

static void	lacp_select(struct lacp_softc *, struct lacp_port *);
static void	lacp_unselect(struct lacp_softc *, struct lacp_port *);
static void	lacp_selected_update(struct lacp_softc *,
		    struct lacp_aggregator *);
static void	lacp_sm_port_init(struct lacp_softc *,
		    struct lacp_port *, struct lagg_port *);
static int	lacp_set_mux(struct lacp_softc *,
		    struct lacp_port *, enum lacp_mux_state);
static void	lacp_sm_mux(struct lacp_softc *, struct lacp_port *);
static void	lacp_sm_mux_timer(struct lacp_softc *, struct lacp_port *);
static void	lacp_sm_rx(struct lacp_softc *, struct lacp_port *,
		    struct lacpdu_peerinfo *, struct lacpdu_peerinfo *);
static void	lacp_sm_rx_set_expired(struct lacp_port *);
static void	lacp_sm_rx_timer(struct lacp_softc *, struct lacp_port *);
static void	lacp_sm_rx_record_default(struct lacp_softc *,
		    struct lacp_port *);

static void	lacp_sm_tx(struct lacp_softc *, struct lacp_port *);
static void	lacp_sm_tx_work(struct lagg_work *, void *);
static void	lacp_sm_ptx_timer(struct lacp_softc *, struct lacp_port *);
static void	lacp_sm_ptx_schedule(struct lacp_port *);
static void	lacp_sm_ptx_update_timeout(struct lacp_port *, uint8_t);

static void	lacp_rcvdu_work(struct lagg_work *, void *);
static void	lacp_marker_work(struct lagg_work *, void *);
static void	lacp_dump_lacpdutlv(const struct lacpdu_peerinfo *,
		    const struct lacpdu_peerinfo *,
		    const struct lacpdu_collectorinfo *);
static void	lacp_dump_markertlv(const struct markerdu_info *,
		    const struct markerdu_info *);

typedef void (*lacp_timer_func_t)(struct lacp_softc *, struct lacp_port *);
static const lacp_timer_func_t	 lacp_timer_funcs[] = {
	[LACP_TIMER_CURRENT_WHILE] = lacp_sm_rx_timer,
	[LACP_TIMER_PERIODIC] = lacp_sm_ptx_timer,
	[LACP_TIMER_WAIT_WHILE] = lacp_sm_mux_timer,
};
typedef void (*lacp_prototimer_func_t)(struct lacp_softc *);
static const lacp_prototimer_func_t	 lacp_ptimer_funcs[] = {
	[LACP_PTIMER_DISTRIBUTING] = lacp_distributing_timer,
};

static void
lacp_dprintf(const struct lacp_softc *lsc, const struct lacp_port *lacpp,
    const char *fmt, ...)
{
	struct lagg_softc *sc;
	va_list va;

	if (lsc != NULL && lsc->lsc_softc != NULL) {
		sc = lsc->lsc_softc;
		printf("%s", sc->sc_if.if_xname);
	} else {
		printf("lacp");
	}

	if (lacpp != NULL)
		printf("(%s)", LACP_PORT_XNAME(lacpp));

	printf(": ");

	va_start(va, fmt);
	vprintf(fmt, va);
	va_end(va);
}

static inline void
lacp_evcnt_attach(struct lacp_softc *lsc,
    struct evcnt *ev, const char *name)
{

	evcnt_attach_dynamic(ev, EVCNT_TYPE_MISC, NULL,
	    lsc->lsc_evgroup, name);
}

static inline bool
lacp_iscollecting(struct lacp_port *lacpp)
{

	return atomic_load_relaxed(&lacpp->lp_collector);
}

static inline bool
lacp_isdistributing(struct lacp_port *lacpp)
{

	return ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_DISTRIBUTING);
}

static inline bool
lacp_isactive(struct lacp_softc *lsc, struct lacp_port *lacpp)
{

	if (lacpp->lp_selected != LACP_SELECTED)
		return false;

	if (lacpp->lp_aggregator == NULL)
		return false;

	if (lacpp->lp_aggregator != lsc->lsc_aggregator)
		return false;

	return true;
}

static inline void
lacp_mcastaddr(struct ifreq *ifr, const char *if_xname)
{
	static const uint8_t addr[ETHER_ADDR_LEN] =
	    { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x02 };

	memset(ifr, 0, sizeof(*ifr));

	strlcpy(ifr->ifr_name, if_xname,
	    sizeof(ifr->ifr_name));
	ifr->ifr_addr.sa_len = sizeof(ifr->ifr_addr);
	ifr->ifr_addr.sa_family = AF_UNSPEC;

	KASSERT(sizeof(ifr->ifr_addr) >= sizeof(addr));
	memcpy(&ifr->ifr_addr.sa_data, addr, sizeof(addr));
}

static inline u_int
lacp_portmap_linkstate(struct lacp_portmap *pm)
{

	if (pm->pm_count == 0)
		return LINK_STATE_DOWN;

	return LINK_STATE_UP;
}

static inline struct lacp_port *
lacp_port_priority_max(struct lacp_port *a, struct lacp_port *b)
{
	uint16_t pri_a, pri_b;

	pri_a = ntohs(a->lp_actor.lpi_portprio);
	pri_b = ntohs(b->lp_actor.lpi_portprio);

	if (pri_a < pri_b)
		return a;
	if (pri_b < pri_a)
		return b;

	pri_a = ntohs(a->lp_partner.lpi_portprio);
	pri_b = ntohs(b->lp_partner.lpi_portprio);

	if (pri_a < pri_b)
		return a;
	if (pri_b < pri_a)
		return b;

	if (a->lp_media > b->lp_media)
		return a;
	if (b->lp_media > a->lp_media)
		return b;

	return a;
}

static void
tlv_parse(void *vp, size_t len, struct tlv *list)
{
	struct tlvhdr *th;
	uint8_t *p;
	size_t l, i;

	th = (struct tlvhdr *)vp;
	p = (uint8_t *)vp;

	for (l = 0; l < len; l += th->tlv_length) {
		th = (struct tlvhdr *)(p + l);

		if (th->tlv_type == TLV_TYPE_TERMINATE)
			break;

		if (th->tlv_length <= 0)
			break;

		for (i = 0; list[i].tlv_t != TLV_TYPE_TERMINATE; i++) {
			if (th->tlv_type != list[i].tlv_t)
				continue;

			if (th->tlv_length - sizeof(*th) != list[i].tlv_l)
				break;

			if (list[i].tlv_v == NULL) {
				list[i].tlv_v =
				    (void *)((uint8_t *)th + sizeof(*th));
			}

			break;
		}
	}
}

int
lacp_attach(struct lagg_softc *sc, struct lagg_proto_softc **lscp)
{
	struct lacp_softc *lsc;
	char xnamebuf[MAXCOMLEN];
	int error;

	KASSERT(LAGG_LOCKED(sc));

	lsc = kmem_zalloc(sizeof(*lsc), KM_NOSLEEP);
	if (lsc == NULL)
		return ENOMEM;

	lsc->lsc_du_q = pcq_create(LACP_RCVDU_LIMIT, KM_NOSLEEP);
	if (lsc->lsc_du_q == NULL) {
		error = ENOMEM;
		goto free_lsc;
	}

	mutex_init(&lsc->lsc_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	lsc->lsc_softc = sc;
	lsc->lsc_key = htons(if_get_index(&sc->sc_if));
	lsc->lsc_system_prio = htons(LACP_SYSTEM_PRIO);
	lsc->lsc_running = false;
	lsc->lsc_max_ports = LACP_MAX_PORTS;
	lsc->lsc_multi_linkspeed = true;
	TAILQ_INIT(&lsc->lsc_aggregators);

	lagg_work_set(&lsc->lsc_work_tick, lacp_tick_work, lsc);
	lagg_work_set(&lsc->lsc_work_rcvdu, lacp_rcvdu_work, lsc);

	snprintf(xnamebuf, sizeof(xnamebuf), "%s.lacp",
	    sc->sc_if.if_xname);
	lsc->lsc_workq = lagg_workq_create(xnamebuf,
	    PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (lsc->lsc_workq == NULL) {
		lagg_log(sc, LOG_ERR, "workqueue create failed\n");
		error = ENOMEM;
		goto destroy_lock;
	}

	lsc->lsc_psz = pserialize_create();

	callout_init(&lsc->lsc_tick, CALLOUT_MPSAFE);
	callout_setfunc(&lsc->lsc_tick, lacp_tick, lsc);

	snprintf(lsc->lsc_evgroup, sizeof(lsc->lsc_evgroup),
	    "%s-lacp", sc->sc_if.if_xname);
	lacp_evcnt_attach(lsc, &lsc->lsc_mgethdr_failed, "MGETHDR failed");
	lacp_evcnt_attach(lsc, &lsc->lsc_mpullup_failed, "m_pullup failed");
	lacp_evcnt_attach(lsc, &lsc->lsc_badlacpdu, "Bad LACPDU recieved");
	lacp_evcnt_attach(lsc, &lsc->lsc_badmarkerdu, "Bad MarkerDU recieved");
	lacp_evcnt_attach(lsc, &lsc->lsc_norcvif, "No received interface");
	lacp_evcnt_attach(lsc, &lsc->lsc_nolaggport, "No lagg context");
	lacp_evcnt_attach(lsc, &lsc->lsc_duq_nospc, "No space left on queues");

	if_link_state_change(&sc->sc_if, LINK_STATE_DOWN);

	*lscp = (struct lagg_proto_softc *)lsc;
	return 0;
destroy_lock:
	mutex_destroy(&lsc->lsc_lock);
	pcq_destroy(lsc->lsc_du_q);
free_lsc:
	kmem_free(lsc, sizeof(*lsc));

	return error;
}

void
lacp_detach(struct lagg_proto_softc *xlsc)
{
	struct lacp_softc *lsc = (struct lacp_softc *)xlsc;
	struct lagg_softc *sc __diagused = lsc->lsc_softc;

	KASSERT(LAGG_LOCKED(lsc->lsc_softc));
	KASSERT(TAILQ_EMPTY(&lsc->lsc_aggregators));
	KASSERT(SIMPLEQ_EMPTY(&sc->sc_ports));

	lacp_down(xlsc);

	lagg_workq_wait(lsc->lsc_workq, &lsc->lsc_work_rcvdu);
	evcnt_detach(&lsc->lsc_mgethdr_failed);
	evcnt_detach(&lsc->lsc_mpullup_failed);
	evcnt_detach(&lsc->lsc_badlacpdu);
	evcnt_detach(&lsc->lsc_badmarkerdu);
	evcnt_detach(&lsc->lsc_norcvif);
	evcnt_detach(&lsc->lsc_nolaggport);
	evcnt_detach(&lsc->lsc_duq_nospc);
	lagg_workq_destroy(lsc->lsc_workq);
	pserialize_destroy(lsc->lsc_psz);
	mutex_destroy(&lsc->lsc_lock);
	pcq_destroy(lsc->lsc_du_q);
	kmem_free(lsc, sizeof(*lsc));
}

int
lacp_up(struct lagg_proto_softc *xlsc)
{
	struct lagg_softc *sc;
	struct lagg_port *lp;
	struct lacp_softc *lsc;

	lsc = (struct lacp_softc *)xlsc;
	sc = lsc->lsc_softc;

	KASSERT(LAGG_LOCKED(sc));

	LACP_LOCK(lsc);
	if (memcmp(lsc->lsc_system_mac, LAGG_CLLADDR(sc),
	    sizeof(lsc->lsc_system_mac)) != 0) {
		memcpy(lsc->lsc_system_mac, LAGG_CLLADDR(sc),
		    sizeof(lsc->lsc_system_mac));
	}
	lsc->lsc_running = true;
	callout_schedule(&lsc->lsc_tick, hz);
	LACP_UNLOCK(lsc);

	LAGG_PORTS_FOREACH(sc, lp) {
		lacp_linkstate(xlsc, lp);
	}

	LACP_DPRINTF((lsc, NULL, "lacp start\n"));

	return 0;
}

static void
lacp_down_locked(struct lacp_softc *lsc)
{
	struct lagg_softc *sc;
	struct lagg_port *lp;

	sc = lsc->lsc_softc;

	KASSERT(LAGG_LOCKED(sc));
	KASSERT(LACP_LOCKED(lsc));

	lsc->lsc_running = false;
	callout_halt(&lsc->lsc_tick, &lsc->lsc_lock);

	LAGG_PORTS_FOREACH(sc, lp) {
		lacp_port_disable(lsc, lp->lp_proto_ctx);
	}

	memset(lsc->lsc_system_mac, 0,
	    sizeof(lsc->lsc_system_mac));

	LACP_DPRINTF((lsc, NULL, "lacp stopped\n"));
}

void
lacp_down(struct lagg_proto_softc *xlsc)
{
	struct lacp_softc *lsc;

	lsc = (struct lacp_softc *)xlsc;

	KASSERT(LAGG_LOCKED(lsc->lsc_softc));

	LACP_LOCK(lsc);
	lacp_down_locked(lsc);
	LACP_UNLOCK(lsc);
}

int
lacp_transmit(struct lagg_proto_softc *xlsc, struct mbuf *m)
{
	struct lacp_softc *lsc;
	struct lagg_port *lp;
	struct ifnet *ifp;
	struct psref psref;

	lsc = (struct lacp_softc *)xlsc;

	if (__predict_false(lsc->lsc_suppress_distributing)) {
		LACP_DPRINTF((lsc, NULL, "waiting transit\n"));
		m_freem(m);
		return ENOBUFS;
	}

	lp = lacp_select_tx_port(lsc, m, &psref);
	if (__predict_false(lp == NULL)) {
		LACP_DPRINTF((lsc, NULL, "no distributing port\n"));
		ifp = &lsc->lsc_softc->sc_if;
		if_statinc(ifp, if_oerrors);
		m_freem(m);
		return ENOENT;
	}

	lagg_enqueue(lsc->lsc_softc, lp, m);
	lagg_port_putref(lp, &psref);

	return 0;
}

int
lacp_allocport(struct lagg_proto_softc *xlsc, struct lagg_port *lp)
{
	struct lagg_softc *sc;
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct ifreq ifr;
	bool added_multi;
	int error;

	lsc = (struct lacp_softc *)xlsc;
	sc = lsc->lsc_softc;

	KASSERT(LAGG_LOCKED(sc));

	IFNET_LOCK(lp->lp_ifp);
	lacp_mcastaddr(&ifr, lp->lp_ifp->if_xname);
	error = lp->lp_ioctl(lp->lp_ifp, SIOCADDMULTI, (void *)&ifr);
	IFNET_UNLOCK(lp->lp_ifp);

	switch (error) {
	case 0:
		added_multi = true;
		break;
	case EAFNOSUPPORT:
		added_multi = false;
		break;
	default:
		lagg_log(sc, LOG_ERR, "SIOCADDMULTI failed on %s\n",
		    lp->lp_ifp->if_xname);
		return error;
	}

	lacpp = kmem_zalloc(sizeof(*lacpp), KM_NOSLEEP);
	if (lacpp == NULL)
		return ENOMEM;

	lacpp->lp_added_multi = added_multi;
	lagg_work_set(&lacpp->lp_work_smtx, lacp_sm_tx_work, lsc);
	lagg_work_set(&lacpp->lp_work_marker, lacp_marker_work, lsc);

	LACP_LOCK(lsc);
	lacp_sm_port_init(lsc, lacpp, lp);
	LACP_UNLOCK(lsc);

	lp->lp_proto_ctx = (void *)lacpp;
	lp->lp_prio = ntohs(lacpp->lp_actor.lpi_portprio);
	lacp_linkstate(xlsc, lp);

	return 0;
}

void
lacp_startport(struct lagg_proto_softc *xlsc, struct lagg_port *lp)
{
	struct lacp_port *lacpp;
	uint16_t prio;

	lacpp = lp->lp_proto_ctx;

	prio = (uint16_t)MIN(lp->lp_prio, UINT16_MAX);
	lacpp->lp_actor.lpi_portprio = htons(prio);

	lacp_linkstate(xlsc, lp);
}

void
lacp_stopport(struct lagg_proto_softc *xlsc, struct lagg_port *lp)
{
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	int i;

	lsc = (struct lacp_softc *)xlsc;
	lacpp = lp->lp_proto_ctx;

	KASSERT(LAGG_LOCKED(lsc->lsc_softc));

	LACP_LOCK(lsc);
	for (i = 0; i < LACP_NTIMER; i++) {
		LACP_TIMER_DISARM(lacpp, i);
	}

	lacp_port_disable(lsc, lacpp);
	LACP_UNLOCK(lsc);
}

void
lacp_freeport(struct lagg_proto_softc *xlsc, struct lagg_port *lp)
{
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct ifreq ifr;

	lsc = (struct lacp_softc *)xlsc;
	lacpp = lp->lp_proto_ctx;

	KASSERT(LAGG_LOCKED(lsc->lsc_softc));

	lagg_workq_wait(lsc->lsc_workq, &lacpp->lp_work_smtx);
	lagg_workq_wait(lsc->lsc_workq, &lacpp->lp_work_marker);

	if (lacpp->lp_added_multi) {
		lacp_mcastaddr(&ifr, LACP_PORT_XNAME(lacpp));

		IFNET_LOCK(lp->lp_ifp);
		(void)lp->lp_ioctl(lp->lp_ifp, SIOCDELMULTI, (void *)&ifr);
		IFNET_UNLOCK(lp->lp_ifp);
	}

	lp->lp_proto_ctx = NULL;
	kmem_free(lacpp, sizeof(*lacpp));
}

void
lacp_protostat(struct lagg_proto_softc *xlsc, struct laggreqproto *resp)
{
	struct laggreq_lacp *rplacp;
	struct lacp_softc *lsc;
	struct lacp_aggregator *la;
	struct lacp_aggregator_systemid *sid;

	lsc = (struct lacp_softc *)xlsc;

	LACP_LOCK(lsc);
	la = lsc->lsc_aggregator;
	rplacp = &resp->rp_lacp;

	if (lsc->lsc_optimistic)
		SET(rplacp->flags, LAGGREQLACP_OPTIMISTIC);
	if (lsc->lsc_dump_du)
		SET(rplacp->flags, LAGGREQLACP_DUMPDU);
	if (lsc->lsc_stop_lacpdu)
		SET(rplacp->flags, LAGGREQLACP_STOPDU);
	if (lsc->lsc_multi_linkspeed)
		SET(rplacp->flags, LAGGREQLACP_MULTILS);

	rplacp->maxports = lsc->lsc_max_ports;
	rplacp->actor_prio = ntohs(lsc->lsc_system_prio);
	memcpy(rplacp->actor_mac, lsc->lsc_system_mac,
	    sizeof(rplacp->actor_mac));
	rplacp->actor_key = ntohs(lsc->lsc_key);

	if (la != NULL) {
		sid = &la->la_sid;
		rplacp->partner_prio = ntohs(sid->sid_prio);
		memcpy(rplacp->partner_mac, sid->sid_mac,
		    sizeof(rplacp->partner_mac));
		rplacp->partner_key = ntohs(sid->sid_key);
	}
	LACP_UNLOCK(lsc);
}

void
lacp_portstat(struct lagg_proto_softc *xlsc, struct lagg_port *lp,
    struct laggreqport *resp)
{
	struct laggreq_lacpport *llp;
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct lacp_aggregator *la;
	struct lacp_aggregator_systemid *sid;

	lsc = (struct lacp_softc *)xlsc;
	lacpp = lp->lp_proto_ctx;
	la = lacpp->lp_aggregator;
	llp = &resp->rp_lacpport;

	if (lacp_isactive(lsc, lacpp))
		SET(resp->rp_flags, LAGG_PORT_ACTIVE);
	if (lacp_iscollecting(lacpp))
		SET(resp->rp_flags, LAGG_PORT_COLLECTING);
	if (lacp_isdistributing(lacpp))
		SET(resp->rp_flags, LAGG_PORT_DISTRIBUTING);
	if (lacpp->lp_selected == LACP_STANDBY)
		SET(resp->rp_flags, LAGG_PORT_STANDBY);

	if (la != NULL) {
		sid = &la->la_sid;
		llp->partner_prio = ntohs(sid->sid_prio);
		memcpy(llp->partner_mac, sid->sid_mac,
		    sizeof(llp->partner_mac));
		llp->partner_key = ntohs(sid->sid_key);
	}

	llp->actor_portprio = ntohs(lacpp->lp_actor.lpi_portprio);
	llp->actor_portno = ntohs(lacpp->lp_actor.lpi_portno);
	llp->actor_state = lacpp->lp_actor.lpi_state;

	llp->partner_portprio = ntohs(lacpp->lp_partner.lpi_portprio);
	llp->partner_portno = ntohs(lacpp->lp_partner.lpi_portno);
	llp->partner_state = lacpp->lp_partner.lpi_state;
}

void
lacp_linkstate_ifnet_locked(struct lagg_proto_softc *xlsc, struct lagg_port *lp)
{
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct ifmediareq ifmr;
	struct ifnet *ifp_port;
	uint8_t old_state;
	uint32_t media, old_media;
	int error;

	KASSERT(IFNET_LOCKED(lp->lp_ifp));

	lsc = (struct lacp_softc *)xlsc;

	ifp_port = lp->lp_ifp;
	lacpp = lp->lp_proto_ctx;
	media = LACP_MEDIA_DEFAULT;

	memset(&ifmr, 0, sizeof(ifmr));
	ifmr.ifm_count = 0;
	error = if_ioctl(ifp_port, SIOCGIFMEDIA, (void *)&ifmr);
	if (error == 0) {
		media = lacp_ifmedia2lacpmedia(ifmr.ifm_active);
	} else if (error != ENOTTY){
		LACP_DPRINTF((lsc, lacpp,
		    "SIOCGIFMEDIA failed (%d)\n", error));
		return;
	}

	LACP_LOCK(lsc);
	if (lsc->lsc_running) {
		old_media = lacpp->lp_media;
		old_state = lacpp->lp_actor.lpi_state;

		if (lacpp->lp_media != media) {
			LACP_DPRINTF((lsc, lacpp,
			    "media changed 0x%"PRIx32"->0x%"PRIx32", "
			    "ether = %d, fdx = %d, link = %d, running = %d\n",
			    lacpp->lp_media, media,
			    ISSET(media, LACP_MEDIA_ETHER) != 0,
			    ISSET(media, LACP_MEDIA_FDX) != 0,
			    ifp_port->if_link_state != LINK_STATE_DOWN,
			    ISSET(ifp_port->if_flags, IFF_RUNNING) != 0));
			lacpp->lp_media = media;
		}

		if (ISSET(media, LACP_MEDIA_ETHER) &&
#ifndef LACP_NOFDX
		    ISSET(media, LACP_MEDIA_FDX) &&
#endif
		    ifp_port->if_link_state != LINK_STATE_DOWN &&
		    ISSET(ifp_port->if_flags, IFF_RUNNING)) {
			lacp_port_enable(lsc, lacpp);
		} else {
			lacp_port_disable(lsc, lacpp);
		}

		if (old_state != lacpp->lp_actor.lpi_state ||
		    old_media != media) {
			LACP_DPRINTF((lsc, lacpp,
			    "state changed to UNSELECTED\n"));
			lacpp->lp_selected = LACP_UNSELECTED;
		}
	} else {
		LACP_DPRINTF((lsc, lacpp,
		    "LACP is inactive, skip linkstate\n"));
	}

	LACP_UNLOCK(lsc);
}

int
lacp_ioctl(struct lagg_proto_softc *xlsc, struct laggreqproto *lreq)
{
	struct lacp_softc *lsc;
	struct laggreq_lacp *rplacp;
	struct lacp_aggregator *la;
	int error;
	size_t maxports;
	bool set;

	lsc = (struct lacp_softc *)xlsc;
	rplacp = &lreq->rp_lacp;
	error = 0;

	switch (rplacp->command) {
	case LAGGIOC_LACPSETFLAGS:
	case LAGGIOC_LACPCLRFLAGS:
		set = (rplacp->command == LAGGIOC_LACPSETFLAGS) ?
		    true : false;

		LACP_LOCK(lsc);

		if (ISSET(rplacp->flags, LAGGREQLACP_OPTIMISTIC))
			lsc->lsc_optimistic = set;
		if (ISSET(rplacp->flags, LAGGREQLACP_DUMPDU))
			lsc->lsc_dump_du = set;
		if (ISSET(rplacp->flags, LAGGREQLACP_STOPDU))
			lsc->lsc_stop_lacpdu = set;
		if (ISSET(rplacp->flags, LAGGREQLACP_MULTILS))
			lsc->lsc_multi_linkspeed = set;

		LACP_UNLOCK(lsc);
		break;
	case LAGGIOC_LACPSETMAXPORTS:
	case LAGGIOC_LACPCLRMAXPORTS:
		maxports = (rplacp->command == LAGGIOC_LACPSETMAXPORTS) ?
		    rplacp->maxports : LACP_MAX_PORTS;
		if (0 == maxports || LACP_MAX_PORTS < maxports) {
			error = ERANGE;
			break;
		}

		LACP_LOCK(lsc);
		if (lsc->lsc_max_ports != maxports) {
			lsc->lsc_max_ports = maxports;
			TAILQ_FOREACH(la, &lsc->lsc_aggregators, la_q) {
				lacp_selected_update(lsc, la);
			}
		}
		LACP_UNLOCK(lsc);
		break;
	default:
		error = ENOTTY;
	}

	return error;
}

static int
lacp_pdu_input(struct lacp_softc *lsc, struct lacp_port *lacpp, struct mbuf *m)
{
	enum {
		LACP_TLV_ACTOR = 0,
		LACP_TLV_PARTNER,
		LACP_TLV_COLLECTOR,
		LACP_TLV_TERM,
		LACP_TLV_NUM
	};

	struct lacpdu *du;
	struct lacpdu_peerinfo *pi_actor, *pi_partner;
	struct lacpdu_collectorinfo *lci;
	struct tlv tlvlist_lacp[LACP_TLV_NUM] = {
		[LACP_TLV_ACTOR] = {
		    .tlv_t = LACP_TYPE_ACTORINFO,
		    .tlv_l = sizeof(*pi_actor)},
		[LACP_TLV_PARTNER] = {
		    .tlv_t = LACP_TYPE_PARTNERINFO,
		    .tlv_l = sizeof(*pi_partner)},
		[LACP_TLV_COLLECTOR] = {
		    .tlv_t = LACP_TYPE_COLLECTORINFO,
		    .tlv_l = sizeof(*lci)},
		[LACP_TLV_TERM] = {
		    .tlv_t = TLV_TYPE_TERMINATE,
		    .tlv_l = 0},
	};

	if (m->m_pkthdr.len != sizeof(*du))
		goto bad;

	if ((m->m_flags & M_MCAST) == 0)
		goto bad;

	if (m->m_len < (int)sizeof(*du)) {
		m = m_pullup(m, sizeof(*du));
		if (m == NULL) {
			lsc->lsc_mpullup_failed.ev_count++;
			return ENOMEM;
		}
	}

	du = mtod(m, struct lacpdu *);

	if (memcmp(&du->ldu_eh.ether_dhost,
	    ethermulticastaddr_slowprotocols, ETHER_ADDR_LEN) != 0)
		goto bad;

	LACP_TLV_PARSE(du, struct lacpdu, ldu_tlv_actor,
	    tlvlist_lacp);

	pi_actor = tlvlist_lacp[LACP_TLV_ACTOR].tlv_v;
	pi_partner = tlvlist_lacp[LACP_TLV_PARTNER].tlv_v;
	lci = tlvlist_lacp[LACP_TLV_COLLECTOR].tlv_v;

	if (pi_actor == NULL || pi_partner == NULL)
		goto bad;

	if (LACP_ISDUMPING(lsc)) {
		lacp_dprintf(lsc, lacpp, "lacpdu received\n");
		lacp_dump_lacpdutlv(pi_actor, pi_partner, lci);
	}

	LACP_LOCK(lsc);
	lacp_sm_rx(lsc, lacpp, pi_partner, pi_actor);
	LACP_UNLOCK(lsc);

	m_freem(m);
	return 0;
bad:
	lsc->lsc_badlacpdu.ev_count++;
	m_freem(m);
	return EINVAL;
}

static int
marker_cmp(struct markerdu_info *mi,
    struct lacp_softc *lsc, struct lacp_port *lacpp)
{

	KASSERT(LACP_LOCKED(lsc));

	if (mi->mi_rq_port != lacpp->lp_actor.lpi_portno)
		return -1;

	if (ntohl(mi->mi_rq_xid) != lacpp->lp_marker_xid)
		return -1;

	return  memcmp(mi->mi_rq_system, lsc->lsc_system_mac,
	    LACP_MAC_LEN);
}

static void
lacp_marker_reply(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct mbuf *m_info)
{
	struct lagg_port *lp;
	struct markerdu *mdu;
	struct ifnet *ifp_port;
	struct psref psref;

	LACP_LOCK(lsc);
	lp = lacpp->lp_laggport;
	lagg_port_getref(lp, &psref);
	LACP_UNLOCK(lsc);

	ifp_port = lp->lp_ifp;
	mdu = mtod(m_info, struct markerdu *);

	mdu->mdu_tlv_info.tlv_type = MARKER_TYPE_RESPONSE;
	/* ether_dhost is already equals to multicast address */
	memcpy(mdu->mdu_eh.ether_shost,
	    CLLADDR(ifp_port->if_sadl), ETHER_ADDR_LEN);

	if (LACP_ISDUMPING(lsc)) {
		lacp_dprintf(lsc, lacpp, "markerdu reply\n");
		lacp_dump_markertlv(NULL, &mdu->mdu_info);
	}

	lagg_port_xmit(lp, m_info);
	lagg_port_putref(lp, &psref);
}

static int
lacp_marker_recv_response(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct markerdu_info *mi_res)
{
	struct lagg_softc *sc;
	struct lagg_port *lp0;
	struct lacp_port *lacpp0;
	bool pending;

	sc = lsc->lsc_softc;

	LACP_LOCK(lsc);
	if (marker_cmp(mi_res, lsc, lacpp) != 0) {
		LACP_UNLOCK(lsc);
		return -1;
	}
	CLR(lacpp->lp_flags, LACP_PORT_MARK);
	LACP_UNLOCK(lsc);

	LAGG_LOCK(sc);
	LACP_LOCK(lsc);

	if (lsc->lsc_suppress_distributing) {
		pending = false;
		LAGG_PORTS_FOREACH(sc, lp0) {
			lacpp0 = lp0->lp_proto_ctx;
			if (ISSET(lacpp0->lp_flags, LACP_PORT_MARK)) {
				pending = true;
				break;
			}
		}

		if (!pending) {
			LACP_DPRINTF((lsc, NULL, "queue flush complete\n"));
			LACP_PTIMER_DISARM(lsc, LACP_PTIMER_DISTRIBUTING);
			lsc->lsc_suppress_distributing = false;
		}
	}

	LACP_UNLOCK(lsc);
	LAGG_UNLOCK(sc);

	return 0;
}

static int
lacp_marker_input(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct mbuf *m)
{
	enum {
		MARKER_TLV_INFO = 0,
		MARKER_TLV_RESPONSE,
		MARKER_TLV_TERM,
		MARKER_TLV_NUM
	};

	struct markerdu *mdu;
	struct markerdu_info *mi_info, *mi_res;
	int error;
	struct tlv tlvlist_marker[MARKER_TLV_NUM] = {
		[MARKER_TLV_INFO] = {
		    .tlv_t = MARKER_TYPE_INFO,
		    .tlv_l = sizeof(*mi_info)},
		[MARKER_TLV_RESPONSE] = {
		    .tlv_t = MARKER_TYPE_RESPONSE,
		    .tlv_l = sizeof(*mi_res)},
		[MARKER_TLV_TERM] = {
		    .tlv_t = TLV_TYPE_TERMINATE,
		    .tlv_l = 0},
	};

	if (m->m_pkthdr.len != sizeof(*mdu))
		goto bad;

	if ((m->m_flags & M_MCAST) == 0)
		goto bad;

	if (m->m_len < (int)sizeof(*mdu)) {
		m = m_pullup(m, sizeof(*mdu));
		if (m == NULL) {
			lsc->lsc_mpullup_failed.ev_count++;
			return ENOMEM;
		}
	}

	mdu = mtod(m, struct markerdu *);

	if (memcmp(mdu->mdu_eh.ether_dhost,
	    ethermulticastaddr_slowprotocols, ETHER_ADDR_LEN) != 0)
		goto bad;

	LACP_TLV_PARSE(mdu, struct markerdu, mdu_tlv_info,
	    tlvlist_marker);

	mi_info = tlvlist_marker[MARKER_TLV_INFO].tlv_v;
	mi_res = tlvlist_marker[MARKER_TLV_RESPONSE].tlv_v;

	if (LACP_ISDUMPING(lsc)) {
		lacp_dprintf(lsc, lacpp, "markerdu received\n");
		lacp_dump_markertlv(mi_info, mi_res);
	}

	if (mi_info != NULL && mi_res == NULL) {
		lacp_marker_reply(lsc, lacpp, m);
	} else if (mi_info == NULL && mi_res != NULL) {
		error = lacp_marker_recv_response(lsc, lacpp,
		    mi_res);
		if (error != 0) {
			goto bad;
		} else {
			m_freem(m);
		}
	} else {
		goto bad;
	}

	return 0;
bad:
	lsc->lsc_badmarkerdu.ev_count++;
	m_freem(m);
	return EINVAL;
}

struct mbuf *
lacp_input(struct lagg_proto_softc *xlsc, struct lagg_port *lp, struct mbuf *m)
{
	struct ifnet *ifp;
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct ether_header *eh;
	uint8_t subtype;

	eh = mtod(m, struct ether_header *);
	lsc = (struct lacp_softc *)xlsc;
	ifp = &lsc->lsc_softc->sc_if;
	lacpp = lp->lp_proto_ctx;

	if (!vlan_has_tag(m) &&
	    eh->ether_type == htons(ETHERTYPE_SLOWPROTOCOLS)) {
		if (m->m_pkthdr.len < (int)(sizeof(*eh) + sizeof(subtype))) {
			m_freem(m);
			return NULL;
		}

		m_copydata(m, sizeof(struct ether_header),
		    sizeof(subtype), &subtype);

		switch (subtype) {
		case SLOWPROTOCOLS_SUBTYPE_LACP:
		case SLOWPROTOCOLS_SUBTYPE_MARKER:
			if (pcq_put(lsc->lsc_du_q, (void *)m)) {
				lagg_workq_add(lsc->lsc_workq,
				    &lsc->lsc_work_rcvdu);
			} else {
				m_freem(m);
				lsc->lsc_duq_nospc.ev_count++;
			}
			return NULL;
		}
	}

	if (!lacp_iscollecting(lacpp) || !lacp_isactive(lsc, lacpp)) {
		if_statinc(ifp, if_ierrors);
		m_freem(m);
		return NULL;
	}

	return m;
}

static void
lacp_rcvdu_work(struct lagg_work *lw __unused, void *xlsc)
{
	struct lacp_softc *lsc = (struct lacp_softc *)xlsc;
	struct ifnet *ifp;
	struct psref psref_lp;
	struct lagg_port *lp;
	struct mbuf *m;
	uint8_t subtype;
	int bound, s;

	bound = curlwp_bind();

	for (;;) {
		m = pcq_get(lsc->lsc_du_q);
		if (m == NULL)
			break;

		ifp = m_get_rcvif(m, &s);
		if (ifp == NULL) {
			m_freem(m);
			lsc->lsc_norcvif.ev_count++;
			continue;
		}

		lp = atomic_load_consume(&ifp->if_lagg);
		if (lp == NULL) {
			m_put_rcvif(ifp, &s);
			m_freem(m);
			lsc->lsc_norcvif.ev_count++;
			continue;
		}

		lagg_port_getref(lp, &psref_lp);
		m_put_rcvif(ifp, &s);

		m_copydata(m, sizeof(struct ether_header),
		    sizeof(subtype), &subtype);

		switch (subtype) {
		case SLOWPROTOCOLS_SUBTYPE_LACP:
			(void)lacp_pdu_input(lsc,
			    lp->lp_proto_ctx, m);
			break;
		case SLOWPROTOCOLS_SUBTYPE_MARKER:
			(void)lacp_marker_input(lsc,
			    lp->lp_proto_ctx, m);
			break;
		}

		lagg_port_putref(lp, &psref_lp);
	}

	curlwp_bindx(bound);
}

static bool
lacp_port_need_to_tell(struct lacp_port *lacpp)
{

	if (!ISSET(lacpp->lp_actor.lpi_state,
	    LACP_STATE_AGGREGATION)) {
		return false;
	}

	if (!ISSET(lacpp->lp_actor.lpi_state,
	    LACP_STATE_ACTIVITY)
	    && !ISSET(lacpp->lp_partner.lpi_state,
	    LACP_STATE_ACTIVITY)) {
		return false;
	}

	if (!ISSET(lacpp->lp_flags, LACP_PORT_NTT))
		return false;

	if (ppsratecheck(&lacpp->lp_last_lacpdu, &lacpp->lp_lacpdu_sent,
	    (LACP_SENDDU_PPS / LACP_FAST_PERIODIC_TIME)) == 0)
		return false;

	return true;
}

static void
lacp_sm_assert_ntt(struct lacp_port *lacpp)
{

	SET(lacpp->lp_flags, LACP_PORT_NTT);
}

static void
lacp_sm_negate_ntt(struct lacp_port *lacpp)
{

	CLR(lacpp->lp_flags, LACP_PORT_NTT);
}

static struct mbuf *
lacp_lacpdu_mbuf(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct ifnet *ifp_port;
	struct mbuf *m;
	struct lacpdu *du;

	KASSERT(LACP_LOCKED(lsc));

	ifp_port = lacpp->lp_laggport->lp_ifp;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		lsc->lsc_mgethdr_failed.ev_count++;
		return NULL;
	}

	m->m_pkthdr.len = m->m_len = sizeof(*du);

	du = mtod(m, struct lacpdu *);
	memset(du, 0, sizeof(*du));

	m->m_flags |= M_MCAST;
	memcpy(du->ldu_eh.ether_dhost, ethermulticastaddr_slowprotocols,
	    ETHER_ADDR_LEN);
	memcpy(du->ldu_eh.ether_shost, CLLADDR(ifp_port->if_sadl),
	    ETHER_ADDR_LEN);
	du->ldu_eh.ether_type = htons(ETHERTYPE_SLOWPROTOCOLS);
	du->ldu_sph.sph_subtype = SLOWPROTOCOLS_SUBTYPE_LACP;
	du->ldu_sph.sph_version = 1;

	tlv_set(&du->ldu_tlv_actor, LACP_TYPE_ACTORINFO,
	    sizeof(du->ldu_actor));
	lacp_peerinfo_actor(lsc, lacpp, &du->ldu_actor);

	tlv_set(&du->ldu_tlv_partner, LACP_TYPE_PARTNERINFO,
	    sizeof(du->ldu_partner));
	lacp_peerinfo_partner(lacpp, &du->ldu_partner);

	tlv_set(&du->ldu_tlv_collector, LACP_TYPE_COLLECTORINFO,
	    sizeof(du->ldu_collector));
	du->ldu_collector.lci_maxdelay = 0;

	du->ldu_tlv_term.tlv_type = LACP_TYPE_TERMINATE;
	du->ldu_tlv_term.tlv_length = 0;

	return m;
}

static void
lacp_sm_tx_work(struct lagg_work *lw, void *xlsc)
{
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct lagg_port *lp;
	struct lacpdu *du;
	struct mbuf *m;
	struct psref psref;
	int bound;

	lsc = xlsc;
	lacpp = container_of(lw, struct lacp_port, lp_work_smtx);

	if (lsc->lsc_stop_lacpdu)
		return;

	LACP_LOCK(lsc);
	m = lacp_lacpdu_mbuf(lsc, lacpp);
	if (m == NULL) {
		LACP_UNLOCK(lsc);
		return;
	}
	lacp_sm_negate_ntt(lacpp);
	lp = lacpp->lp_laggport;
	bound = curlwp_bind();
	lagg_port_getref(lp, &psref);
	LACP_UNLOCK(lsc);

	if (LACP_ISDUMPING(lsc)) {
		lacp_dprintf(lsc, lacpp, "lacpdu transmit\n");
		du = mtod(m, struct lacpdu *);
		lacp_dump_lacpdutlv(&du->ldu_actor,
		    &du->ldu_partner, &du->ldu_collector);
	}

	lagg_port_xmit(lp, m);
	lagg_port_putref(lp, &psref);
	curlwp_bindx(bound);
}

static void
lacp_sm_tx(struct lacp_softc *lsc, struct lacp_port *lacpp)
{

	if (!lacp_port_need_to_tell(lacpp))
		return;

	lagg_workq_add(lsc->lsc_workq, &lacpp->lp_work_smtx);
}

static void
lacp_tick(void *xlsc)
{
	struct lacp_softc *lsc;

	lsc = xlsc;

	lagg_workq_add(lsc->lsc_workq, &lsc->lsc_work_tick);

	LACP_LOCK(lsc);
	callout_schedule(&lsc->lsc_tick, hz);
	LACP_UNLOCK(lsc);
}

static void
lacp_run_timers(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	size_t i;

	for (i = 0; i < LACP_NTIMER; i++) {
		KASSERT(lacpp->lp_timer[i] >= 0);

		if (lacpp->lp_timer[i] == 0)
			continue;
		if (--lacpp->lp_timer[i] > 0)
			continue;

		KASSERT(lacp_timer_funcs[i] != NULL);
		lacp_timer_funcs[i](lsc, lacpp);
	}
}

static void
lacp_run_prototimers(struct lacp_softc *lsc)
{
	size_t i;

	for (i = 0; i < LACP_NPTIMER; i++) {
		KASSERT(lsc->lsc_timer[i] >= 0);

		if (lsc->lsc_timer[i] == 0)
			continue;
		if (--lsc->lsc_timer[i] > 0)
			continue;

		KASSERT(lacp_ptimer_funcs[i] != NULL);
		lacp_ptimer_funcs[i](lsc);
	}
}

static void
lacp_tick_work(struct lagg_work *lw __unused, void *xlsc)
{
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct lagg_softc *sc;
	struct lagg_port *lp;

	lsc = xlsc;
	sc = lsc->lsc_softc;

	LACP_LOCK(lsc);
	lacp_run_prototimers(lsc);
	LACP_UNLOCK(lsc);

	LAGG_LOCK(sc);
	LACP_LOCK(lsc);
	LAGG_PORTS_FOREACH(sc, lp) {
		lacpp = lp->lp_proto_ctx;
		if (!ISSET(lacpp->lp_actor.lpi_state,
		    LACP_STATE_AGGREGATION)) {
			continue;
		}

		lacp_run_timers(lsc, lacpp);
		lacp_select(lsc, lacpp);
		lacp_sm_mux(lsc, lacpp);
		lacp_sm_tx(lsc, lacpp);
		lacp_sm_ptx_schedule(lacpp);
	}

	LACP_UNLOCK(lsc);
	LAGG_UNLOCK(sc);
}

static void
lacp_systemid_str(char *buf, size_t buflen,
    uint16_t prio, const uint8_t *mac, uint16_t key)
{

	snprintf(buf, buflen,
	    "%04X,"
	    "%02X-%02X-%02X-%02X-%02X-%02X,"
	    "%04X",
	    (unsigned int)ntohs(prio),
	    (int)mac[0], (int)mac[1], (int)mac[2],
	    (int)mac[3], (int)mac[4], (int)mac[5],
	    (unsigned int)htons(key));

}

__LACPDEBUGUSED static void
lacp_aggregator_str(struct lacp_aggregator *la, char *buf, size_t buflen)
{

	lacp_systemid_str(buf, buflen, la->la_sid.sid_prio,
	    la->la_sid.sid_mac, la->la_sid.sid_key);
}

static void
lacp_peerinfo_idstr(const struct lacpdu_peerinfo *pi,
    char *buf, size_t buflen)
{

	lacp_systemid_str(buf, buflen, pi->lpi_system_prio,
	    pi->lpi_system_mac, pi->lpi_key);
}

static void
lacp_state_str(uint8_t state, char *buf, size_t buflen)
{

	snprintb(buf, buflen, LACP_STATE_BITS, state);
}

static struct lagg_port *
lacp_select_tx_port(struct lacp_softc *lsc, struct mbuf *m,
    struct psref *psref)
{
	struct lacp_portmap *pm;
	struct lagg_port *lp;
	uint32_t hash;
	size_t act;
	int s;

	hash = lagg_hashmbuf(lsc->lsc_softc, m);

	s = pserialize_read_enter();
	act = LACP_PORTMAP_ACTIVE(lsc);
	pm = &lsc->lsc_portmaps[act];

	if (pm->pm_count == 0) {
		pserialize_read_exit(s);
		return NULL;
	}

	hash %= pm->pm_count;
	lp = pm->pm_ports[hash];
	lagg_port_getref(lp, psref);

	pserialize_read_exit(s);

	return lp;
}

static void
lacp_peerinfo_actor(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lacpdu_peerinfo *dst)
{

	memcpy(dst->lpi_system_mac, lsc->lsc_system_mac, LACP_MAC_LEN);
	dst->lpi_system_prio = lsc->lsc_system_prio;
	dst->lpi_key = lsc->lsc_key;
	dst->lpi_port_no = lacpp->lp_actor.lpi_portno;
	dst->lpi_port_prio = lacpp->lp_actor.lpi_portprio;
	dst->lpi_state = lacpp->lp_actor.lpi_state;
}

static void
lacp_peerinfo_partner(struct lacp_port *lacpp, struct lacpdu_peerinfo *dst)
{
	struct lacp_aggregator *la;

	la = lacpp->lp_aggregator;

	if (la != NULL) {
		memcpy(dst->lpi_system_mac, la->la_sid.sid_mac, LACP_MAC_LEN);
		dst->lpi_system_prio = la->la_sid.sid_prio;
		dst->lpi_key = la->la_sid.sid_key;
	} else {
		memset(dst->lpi_system_mac, 0, LACP_MAC_LEN);
		dst->lpi_system_prio = 0;
		dst->lpi_key = 0;
	}
	dst->lpi_port_no = lacpp->lp_partner.lpi_portno;
	dst->lpi_port_prio = lacpp->lp_partner.lpi_portprio;
	dst->lpi_state = lacpp->lp_partner.lpi_state;
}

static int
lacp_compare_peerinfo(struct lacpdu_peerinfo *a, struct lacpdu_peerinfo *b)
{

	return memcmp(a, b, offsetof(struct lacpdu_peerinfo, lpi_state));
}

static void
lacp_sm_rx_record_default(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	uint8_t oldpstate;
	struct lacp_portinfo *pi;
	char buf[LACP_STATESTR_LEN] __LACPDEBUGUSED;

	pi = &lacpp->lp_partner;

	oldpstate = pi->lpi_state;
	pi->lpi_portno = htons(LACP_PORTNO_NONE);
	pi->lpi_portprio = htons(0xffff);

	if (lsc->lsc_optimistic)
		pi->lpi_state = LACP_PARTNER_ADMIN_OPTIMISTIC;
	else
		pi->lpi_state = LACP_PARTNER_ADMIN_STRICT;

	SET(lacpp->lp_actor.lpi_state, LACP_STATE_DEFAULTED);

	if (oldpstate != pi->lpi_state) {
		LACP_STATE_STR(oldpstate, buf, sizeof(buf));
		LACP_DPRINTF((lsc, lacpp, "oldpstate %s\n", buf));

		LACP_STATE_STR(pi->lpi_state, buf, sizeof(buf));
		LACP_DPRINTF((lsc, lacpp, "newpstate %s\n", buf));
	}
}

static inline bool
lacp_port_is_synced(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lacpdu_peerinfo *my_pi, struct lacpdu_peerinfo *peer_pi)
{
	struct lacpdu_peerinfo actor;

	if (!ISSET(peer_pi->lpi_state, LACP_STATE_ACTIVITY) &&
	    (!ISSET(my_pi->lpi_state, LACP_STATE_ACTIVITY) ||
	    !ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_ACTIVITY)))
		return false;

	if (!ISSET(peer_pi->lpi_state, LACP_STATE_AGGREGATION))
		return false;

	lacp_peerinfo_actor(lsc, lacpp, &actor);
	if (lacp_compare_peerinfo(&actor, my_pi) != 0)
		return false;

	if (!LACP_STATE_EQ(actor.lpi_state, my_pi->lpi_state,
	    LACP_STATE_AGGREGATION)) {
		return false;
	}

	return true;
}

static void
lacp_sm_rx_record_peerinfo(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lacpdu_peerinfo *my_pi, struct lacpdu_peerinfo *peer_pi)
{
	char buf[LACP_STATESTR_LEN] __LACPDEBUGUSED;
	uint8_t oldpstate;
	struct lacp_portinfo *pi;
	struct lacp_aggregator_systemid *sid;

	pi = &lacpp->lp_partner;
	sid = &lacpp->lp_aggregator_sidbuf;

	oldpstate = lacpp->lp_partner.lpi_state;

	sid->sid_prio = peer_pi->lpi_system_prio;
	sid->sid_key = peer_pi->lpi_key;
	memcpy(sid->sid_mac, peer_pi->lpi_system_mac,
	    sizeof(sid->sid_mac));

	pi->lpi_portno = peer_pi->lpi_port_no;
	pi->lpi_portprio = peer_pi->lpi_port_prio;
	pi->lpi_state = peer_pi->lpi_state;

	if (lacp_port_is_synced(lsc, lacpp, my_pi, peer_pi)) {
		if (lsc->lsc_optimistic)
			SET(lacpp->lp_partner.lpi_state, LACP_STATE_SYNC);
	} else {
		CLR(lacpp->lp_partner.lpi_state, LACP_STATE_SYNC);
	}

	CLR(lacpp->lp_actor.lpi_state, LACP_STATE_DEFAULTED);

	if (oldpstate != lacpp->lp_partner.lpi_state) {
		LACP_STATE_STR(oldpstate, buf, sizeof(buf));
		LACP_DPRINTF((lsc, lacpp, "oldpstate %s\n", buf));

		LACP_STATE_STR(lacpp->lp_partner.lpi_state,
		    buf, sizeof(buf));
		LACP_DPRINTF((lsc, lacpp, "newpstate %s\n", buf));
	}

	lacp_sm_ptx_update_timeout(lacpp, oldpstate);
}

static void
lacp_sm_rx_set_expired(struct lacp_port *lacpp)
{

	CLR(lacpp->lp_partner.lpi_state, LACP_STATE_SYNC);
	SET(lacpp->lp_partner.lpi_state, LACP_STATE_TIMEOUT);
	LACP_TIMER_ARM(lacpp, LACP_TIMER_CURRENT_WHILE,
	    LACP_SHORT_TIMEOUT_TIME);
	SET(lacpp->lp_actor.lpi_state, LACP_STATE_EXPIRED);
}

static void
lacp_sm_port_init(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lagg_port *lp)
{

	KASSERT(LACP_LOCKED(lsc));

	lacpp->lp_laggport = lp;
	lacpp->lp_actor.lpi_state = LACP_STATE_ACTIVITY;
	lacpp->lp_actor.lpi_portno = htons(if_get_index(lp->lp_ifp));
	lacpp->lp_actor.lpi_portprio = htons(LACP_PORT_PRIO);
	lacpp->lp_partner.lpi_state = LACP_STATE_TIMEOUT;
	lacpp->lp_aggregator = NULL;
	lacpp->lp_marker_xid = 0;
	lacpp->lp_mux_state = LACP_MUX_INIT;

	lacp_set_mux(lsc, lacpp, LACP_MUX_DETACHED);
	lacp_sm_rx_record_default(lsc, lacpp);
}

static void
lacp_port_disable(struct lacp_softc *lsc, struct lacp_port *lacpp)
{

	if (ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_AGGREGATION))
		LACP_DPRINTF((lsc, lacpp, "enable -> disable\n"));

	lacp_set_mux(lsc, lacpp, LACP_MUX_DETACHED);
	lacp_sm_rx_record_default(lsc, lacpp);
	CLR(lacpp->lp_actor.lpi_state,
	    LACP_STATE_AGGREGATION | LACP_STATE_EXPIRED);
	CLR(lacpp->lp_partner.lpi_state, LACP_STATE_AGGREGATION);
}

static void
lacp_port_enable(struct lacp_softc *lsc __LACPDEBUGUSED,
    struct lacp_port *lacpp)
{

	if (!ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_AGGREGATION))
		LACP_DPRINTF((lsc, lacpp, "disable -> enable\n"));

	SET(lacpp->lp_actor.lpi_state, LACP_STATE_AGGREGATION);
	lacp_sm_rx_set_expired(lacpp);
}

static void
lacp_sm_rx_timer(struct lacp_softc *lsc, struct lacp_port *lacpp)
{

	if (!ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_EXPIRED)) {
		/* CURRENT -> EXPIRED */
		LACP_DPRINTF((lsc, lacpp, "CURRENT -> EXPIRED\n"));
		lacp_sm_rx_set_expired(lacpp);
	} else {
		LACP_DPRINTF((lsc, lacpp, "EXPIRED -> DEFAULTED\n"));
		lacp_set_mux(lsc, lacpp, LACP_MUX_DETACHED);
		lacp_sm_rx_record_default(lsc, lacpp);
	}
}

static void
lacp_sm_ptx_timer(struct lacp_softc *lsc __unused, struct lacp_port *lacpp)
{

	lacp_sm_assert_ntt(lacpp);
}

static void
lacp_sm_ptx_schedule(struct lacp_port *lacpp)
{
	int timeout;

	/* no periodic */
	if (!ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_ACTIVITY) &&
	    !ISSET(lacpp->lp_partner.lpi_state, LACP_STATE_ACTIVITY)) {
		LACP_TIMER_DISARM(lacpp, LACP_TIMER_PERIODIC);
		return;
	}

	if (LACP_TIMER_ISARMED(lacpp, LACP_TIMER_PERIODIC))
		return;

	timeout = ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_TIMEOUT) ?
		LACP_FAST_PERIODIC_TIME : LACP_SLOW_PERIODIC_TIME;

	LACP_TIMER_ARM(lacpp, LACP_TIMER_PERIODIC, timeout);
}

static void
lacp_sm_ptx_update_timeout(struct lacp_port *lacpp, uint8_t oldpstate)
{

	if (LACP_STATE_EQ(oldpstate, lacpp->lp_partner.lpi_state,
	    LACP_STATE_TIMEOUT))
		return;

	LACP_DPRINTF((NULL, lacpp, "partner timeout changed\n"));

	LACP_TIMER_DISARM(lacpp, LACP_TIMER_PERIODIC);

	/* if timeout has been shorted, assert NTT */
	if (ISSET(lacpp->lp_partner.lpi_state, LACP_STATE_TIMEOUT))
		lacp_sm_assert_ntt(lacpp);
}

static void
lacp_sm_mux_timer(struct lacp_softc *lsc __LACPDEBUGUSED,
    struct lacp_port *lacpp)
{
	char buf[LACP_SYSTEMIDSTR_LEN] __LACPDEBUGUSED;

	KASSERT(lacpp->lp_pending > 0);

	LACP_AGGREGATOR_STR(lacpp->lp_aggregator, buf, sizeof(buf));
	LACP_DPRINTF((lsc, lacpp, "aggregator %s, pending %d -> %d\n",
	    buf, lacpp->lp_pending, lacpp->lp_pending -1));

	lacpp->lp_pending--;
}

static void
lacp_sm_rx_update_selected(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lacpdu_peerinfo *peer_pi)
{
	struct lacpdu_peerinfo partner;
	char str0[LACP_SYSTEMIDSTR_LEN] __LACPDEBUGUSED;
	char str1[LACP_SYSTEMIDSTR_LEN] __LACPDEBUGUSED;

	if (lacpp->lp_aggregator == NULL)
		return;

	lacp_peerinfo_partner(lacpp, &partner);
	if (lacp_compare_peerinfo(peer_pi, &partner) != 0) {
		LACP_PEERINFO_IDSTR(&partner, str0, sizeof(str0));
		LACP_PEERINFO_IDSTR(peer_pi, str1, sizeof(str1));
		LACP_DPRINTF((lsc, lacpp,
		    "different peerinfo, %s vs %s\n", str0, str1));
		goto do_unselect;
	}

	if (!LACP_STATE_EQ(lacpp->lp_partner.lpi_state,
	    peer_pi->lpi_state, LACP_STATE_AGGREGATION)) {
		LACP_DPRINTF((lsc, lacpp,
		    "STATE_AGGREGATION changed %d -> %d\n",
		    ISSET(lacpp->lp_partner.lpi_state,
		    LACP_STATE_AGGREGATION) != 0,
		    ISSET(peer_pi->lpi_state, LACP_STATE_AGGREGATION) != 0));
		goto do_unselect;
	}

	return;

do_unselect:
	lacpp->lp_selected = LACP_UNSELECTED;
	/* lacpp->lp_aggregator will be released at lacp_set_mux() */
}

static void
lacp_sm_rx_update_ntt(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lacpdu_peerinfo *my_pi)
{
	struct lacpdu_peerinfo actor;

	lacp_peerinfo_actor(lsc, lacpp, &actor);

	if (lacp_compare_peerinfo(&actor, my_pi) != 0 ||
	    !LACP_STATE_EQ(lacpp->lp_actor.lpi_state, my_pi->lpi_state,
	    LACP_STATE_ACTIVITY | LACP_STATE_SYNC | LACP_STATE_AGGREGATION)) {
		LACP_DPRINTF((lsc, lacpp, "assert ntt\n"));
		lacp_sm_assert_ntt(lacpp);
	}
}

static void
lacp_sm_rx(struct lacp_softc *lsc, struct lacp_port *lacpp,
    struct lacpdu_peerinfo *my_pi, struct lacpdu_peerinfo *peer_pi)
{
	int timeout;

	KASSERT(LACP_LOCKED(lsc));

	/* check LACP disabled first */
	if (!ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_AGGREGATION))
		return;

	/* check loopback condition */
	if (memcmp(lsc->lsc_system_mac, peer_pi->lpi_system_mac,
	    LACP_MAC_LEN) == 0 &&
	    lsc->lsc_system_prio == peer_pi->lpi_system_prio)
		return;

	lacp_sm_rx_update_selected(lsc, lacpp, peer_pi);
	lacp_sm_rx_update_ntt(lsc, lacpp, my_pi);
	lacp_sm_rx_record_peerinfo(lsc, lacpp, my_pi, peer_pi);

	timeout = ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_TIMEOUT) ?
	    LACP_SHORT_TIMEOUT_TIME : LACP_LONG_TIMEOUT_TIME;
	LACP_TIMER_ARM(lacpp, LACP_TIMER_CURRENT_WHILE, timeout);

	CLR(lacpp->lp_actor.lpi_state, LACP_STATE_EXPIRED);

	/* kick transmit machine without timeout. */
	lacp_sm_tx(lsc, lacpp);
}

static void
lacp_disable_collecting(struct lacp_port *lacpp)
{

	LACP_DPRINTF((NULL, lacpp, "collecting disabled\n"));
	CLR(lacpp->lp_actor.lpi_state, LACP_STATE_COLLECTING);
	atomic_store_relaxed(&lacpp->lp_collector, false);
}

static void
lacp_enable_collecting(struct lacp_port *lacpp)
{
	LACP_DPRINTF((NULL, lacpp, "collecting enabled\n"));
	SET(lacpp->lp_actor.lpi_state, LACP_STATE_COLLECTING);
	atomic_store_relaxed(&lacpp->lp_collector, true);
}

static void
lacp_update_portmap(struct lacp_softc *lsc)
{
	struct lagg_softc *sc;
	struct lacp_aggregator *la;
	struct lacp_portmap *pm_act, *pm_next;
	struct lacp_port *lacpp;
	size_t pmap, n;
	u_int link;

	KASSERT(LACP_LOCKED(lsc));

	la = lsc->lsc_aggregator;

	pmap = LACP_PORTMAP_ACTIVE(lsc);
	pm_act = &lsc->lsc_portmaps[pmap];

	pmap = LACP_PORTMAP_NEXT(lsc);
	pm_next = &lsc->lsc_portmaps[pmap];

	n = 0;
	if (la != NULL) {
		LIST_FOREACH(lacpp, &la->la_ports, lp_entry_la) {
			if (!ISSET(lacpp->lp_actor.lpi_state,
			    LACP_STATE_DISTRIBUTING)) {
				continue;
			}

			pm_next->pm_ports[n] = lacpp->lp_laggport;
			n++;

			if (n >= LACP_MAX_PORTS)
				break;
		}
	}
	pm_next->pm_count = n;

	atomic_store_release(&lsc->lsc_activemap, pmap);
	pserialize_perform(lsc->lsc_psz);

	LACP_DPRINTF((lsc, NULL, "portmap count updated (%zu -> %zu)\n",
	    pm_act->pm_count, pm_next->pm_count));

	link = lacp_portmap_linkstate(pm_next);
	if (link != lacp_portmap_linkstate(pm_act)) {
		sc = lsc->lsc_softc;
		if_link_state_change(&sc->sc_if, link);
	}

	/* cleanup */
	pm_act->pm_count = 0;
	memset(pm_act->pm_ports, 0, sizeof(pm_act->pm_ports));
}

static void
lacp_disable_distributing(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct lacp_portmap *pm;
	bool do_update;
	size_t act, i;
	int s;

	KASSERT(LACP_LOCKED(lsc));

	LACP_DPRINTF((lsc, lacpp, "distributing disabled\n"));
	CLR(lacpp->lp_actor.lpi_state, LACP_STATE_DISTRIBUTING);

	s = pserialize_read_enter();
	act = LACP_PORTMAP_ACTIVE(lsc);
	pm = &lsc->lsc_portmaps[act];

	do_update = false;
	for (i = 0; i < pm->pm_count; i++) {
		if (pm->pm_ports[i] == lacpp->lp_laggport) {
			do_update = true;
			break;
		}
	}
	pserialize_read_exit(s);

	if (do_update)
		lacp_update_portmap(lsc);
}

static void
lacp_enable_distributing(struct lacp_softc *lsc, struct lacp_port *lacpp)
{

	KASSERT(LACP_LOCKED(lsc));

	KASSERT(lacp_isactive(lsc, lacpp));

	LACP_DPRINTF((lsc, lacpp, "distributing enabled\n"));
	SET(lacpp->lp_actor.lpi_state, LACP_STATE_DISTRIBUTING);
	lacp_suppress_distributing(lsc);
	lacp_update_portmap(lsc);
}

static void
lacp_select_active_aggregator(struct lacp_softc *lsc)
{
	struct lacp_aggregator *la, *best_la;
	char str[LACP_SYSTEMIDSTR_LEN] __LACPDEBUGUSED;

	KASSERT(LACP_LOCKED(lsc));

	la = lsc->lsc_aggregator;
	if (la != NULL && la->la_attached_port > 0) {
		best_la = la;
	} else {
		best_la = NULL;
	}

	TAILQ_FOREACH(la, &lsc->lsc_aggregators, la_q) {
		if (la->la_attached_port <= 0)
			continue;

		if (best_la == NULL ||
		    LACP_SYS_PRI(la) < LACP_SYS_PRI(best_la))
			best_la = la;
	}

	if (best_la != lsc->lsc_aggregator) {
		LACP_DPRINTF((lsc, NULL, "active aggregator changed\n"));

		if (lsc->lsc_aggregator != NULL) {
			LACP_AGGREGATOR_STR(lsc->lsc_aggregator,
			    str, sizeof(str));
		} else {
			snprintf(str, sizeof(str), "(null)");
		}
		LACP_DPRINTF((lsc, NULL, "old aggregator=%s\n", str));

		if (best_la != NULL) {
			LACP_AGGREGATOR_STR(best_la, str, sizeof(str));
		} else {
			snprintf(str, sizeof(str), "(null)");
		}
		LACP_DPRINTF((lsc, NULL, "new aggregator=%s\n", str));

		lsc->lsc_aggregator = best_la;
	}
}

static void
lacp_port_attached(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct lacp_aggregator *la;

	KASSERT(LACP_LOCKED(lsc));

	if (ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_SYNC))
		return;

	la = lacpp->lp_aggregator;
	KASSERT(la != NULL);
	KASSERT(la->la_attached_port >= 0);

	SET(lacpp->lp_actor.lpi_state, LACP_STATE_SYNC);
	la->la_attached_port++;
	lacp_select_active_aggregator(lsc);
}

static void
lacp_port_detached(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct lacp_aggregator *la;

	KASSERT(LACP_LOCKED(lsc));

	if (!ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_SYNC))
		return;

	la = lacpp->lp_aggregator;
	KASSERT(la != NULL);
	KASSERT(la->la_attached_port > 0);

	CLR(lacpp->lp_actor.lpi_state, LACP_STATE_SYNC);
	la->la_attached_port--;
	lacp_select_active_aggregator(lsc);
}

static int
lacp_set_mux(struct lacp_softc *lsc, struct lacp_port *lacpp,
    enum lacp_mux_state new_state)
{
	struct lagg_softc *sc;
	struct ifnet *ifp;

	KASSERT(LACP_LOCKED(lsc));

	sc = lacpp->lp_laggport->lp_softc;
	ifp = &sc->sc_if;

	if (lacpp->lp_mux_state == new_state)
		return -1;

	switch (new_state) {
	case LACP_MUX_DETACHED:
		lacp_port_detached(lsc, lacpp);
		lacp_disable_distributing(lsc, lacpp);
		lacp_disable_collecting(lacpp);
		lacp_sm_assert_ntt(lacpp);
		/* cancel timer */
		if (LACP_TIMER_ISARMED(lacpp, LACP_TIMER_WAIT_WHILE)) {
			KASSERT(lacpp->lp_pending > 0);
			lacpp->lp_pending--;
			LACP_TIMER_DISARM(lacpp, LACP_TIMER_WAIT_WHILE);
		}
		lacp_unselect(lsc, lacpp);
		break;
	case LACP_MUX_WAITING:
		LACP_TIMER_ARM(lacpp, LACP_TIMER_WAIT_WHILE,
		    LACP_AGGREGATE_WAIT_TIME);
		lacpp->lp_pending++;
		break;
	case LACP_MUX_STANDBY:
#ifdef LACP_STANDBY_SYNCED
		lacp_port_attached(lsc, lacpp);
		lacp_disable_collecting(lacpp);
		lacp_sm_assert_ntt(lacpp);
#endif
		break;
	case LACP_MUX_ATTACHED:
		lacp_port_attached(lsc, lacpp);
		lacp_disable_collecting(lacpp);
		lacp_sm_assert_ntt(lacpp);
		break;
	case LACP_MUX_COLLECTING:
		lacp_enable_collecting(lacpp);
		lacp_disable_distributing(lsc, lacpp);
		lacp_sm_assert_ntt(lacpp);
		break;
	case LACP_MUX_DISTRIBUTING:
		lacp_enable_distributing(lsc, lacpp);
		break;
	case LACP_MUX_INIT:
	default:
		panic("%s: unknown state", ifp->if_xname);
	}

	LACP_DPRINTF((lsc, lacpp, "mux_state %d -> %d\n",
	    lacpp->lp_mux_state, new_state));

	lacpp->lp_mux_state = new_state;
	return 0;
}

static void
lacp_sm_mux(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct lacp_aggregator *la __diagused;
	enum lacp_mux_state  next_state;
	enum lacp_selected selected;
	bool p_sync, p_collecting;

	p_sync = ISSET(lacpp->lp_partner.lpi_state, LACP_STATE_SYNC);
	p_collecting = ISSET(lacpp->lp_partner.lpi_state,
	    LACP_STATE_COLLECTING);

	do {
		next_state = lacpp->lp_mux_state;
		la = lacpp->lp_aggregator;
		selected = lacpp->lp_selected;
		KASSERT(la != NULL ||
		    lacpp->lp_mux_state == LACP_MUX_DETACHED);

		switch (lacpp->lp_mux_state) {
		case LACP_MUX_DETACHED:
			if (selected != LACP_UNSELECTED)
				next_state = LACP_MUX_WAITING;
			break;
		case LACP_MUX_WAITING:
			KASSERTMSG((lacpp->lp_pending > 0 ||
			    !LACP_TIMER_ISARMED(lacpp, LACP_TIMER_WAIT_WHILE)),
			    "lp_pending=%d, timer=%d", lacpp->lp_pending,
			    !LACP_TIMER_ISARMED(lacpp, LACP_TIMER_WAIT_WHILE));

			if (selected == LACP_UNSELECTED) {
				next_state = LACP_MUX_DETACHED;
			} else if (lacpp->lp_pending == 0) {
				if (selected == LACP_SELECTED) {
					next_state = LACP_MUX_ATTACHED;
				} else if (selected == LACP_STANDBY) {
					next_state = LACP_MUX_STANDBY;
				} else {
					next_state = LACP_MUX_DETACHED;
				}
			}
			break;
		case LACP_MUX_STANDBY:
			if (selected == LACP_SELECTED) {
				next_state = LACP_MUX_ATTACHED;
			} else if (selected != LACP_STANDBY) {
				next_state = LACP_MUX_DETACHED;
			}
			break;
		case LACP_MUX_ATTACHED:
			if (selected != LACP_SELECTED) {
				next_state = LACP_MUX_DETACHED;
			} else if (lacp_isactive(lsc, lacpp) && p_sync) {
				next_state = LACP_MUX_COLLECTING;
			}
			break;
		case LACP_MUX_COLLECTING:
			if (selected != LACP_SELECTED ||
			    !lacp_isactive(lsc, lacpp)
			    || !p_sync) {
				next_state = LACP_MUX_ATTACHED;
			} else if (p_collecting) {
				next_state = LACP_MUX_DISTRIBUTING;
			}
			break;
		case LACP_MUX_DISTRIBUTING:
			if (selected != LACP_SELECTED ||
			    !lacp_isactive(lsc, lacpp)
			    || !p_sync || !p_collecting) {
				next_state = LACP_MUX_COLLECTING;
				LACP_DPRINTF((lsc, lacpp,
				    "Interface stopped DISTRIBUTING,"
				    " possible flapping\n"));
			}
			break;
		case LACP_MUX_INIT:
		default:
			panic("%s: unknown state",
			    lsc->lsc_softc->sc_if.if_xname);
		}
	} while (lacp_set_mux(lsc, lacpp, next_state) == 0);
}

static bool
lacp_aggregator_is_match(struct lacp_aggregator_systemid *a,
struct lacp_aggregator_systemid *b)
{

	if (a->sid_prio != b->sid_prio)
		return false;

	if (a->sid_key != b->sid_key)
		return false;

	if (memcmp(a->sid_mac, b->sid_mac, sizeof(a->sid_mac)) != 0)
		return false;

	return true;
}

static void
lacp_selected_update(struct lacp_softc *lsc, struct lacp_aggregator *la)
{
	struct lacp_port *lacpp;
	size_t nselected;
	uint32_t media;

	KASSERT(LACP_LOCKED(lsc));

	lacpp = LIST_FIRST(&la->la_ports);
	if (lacpp == NULL)
		return;

	media = lacpp->lp_media;
	nselected = 0;
	LIST_FOREACH(lacpp, &la->la_ports, lp_entry_la) {
		if (nselected >= lsc->lsc_max_ports ||
		    (!lsc->lsc_multi_linkspeed && media != lacpp->lp_media)) {
			if (lacpp->lp_selected == LACP_SELECTED)
				lacpp->lp_selected = LACP_STANDBY;
			continue;
		}

		switch (lacpp->lp_selected) {
		case LACP_STANDBY:
			lacpp->lp_selected = LACP_SELECTED;
			/* fall through */
		case LACP_SELECTED:
			nselected++;
			break;
		default:
			/* do nothing */
			break;
		}
	}
}

static void
lacp_select(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct lacp_aggregator *la;
	struct lacp_aggregator_systemid *sid;
	struct lacp_port *lacpp0;
	char buf[LACP_SYSTEMIDSTR_LEN] __LACPDEBUGUSED;

	if (lacpp->lp_aggregator != NULL)
		return;

	/* If we haven't heard from our peer, skip this step. */
	if (ISSET(lacpp->lp_actor.lpi_state, LACP_STATE_DEFAULTED))
		return

	KASSERT(!LACP_TIMER_ISARMED(lacpp, LACP_TIMER_WAIT_WHILE));

	sid = &lacpp->lp_aggregator_sidbuf;

	TAILQ_FOREACH(la, &lsc->lsc_aggregators, la_q) {
		if (lacp_aggregator_is_match(&la->la_sid, sid))
			break;
	}

	if (la == NULL) {
		la = kmem_zalloc(sizeof(*la), KM_NOSLEEP);
		if (la == NULL) {
			LACP_DPRINTF((lsc, lacpp,
			    "couldn't allocate aggregator\n"));
			/* will retry the next tick. */
			return;
		}
		LIST_INIT(&la->la_ports);

		la->la_sid = *sid;
		TAILQ_INSERT_TAIL(&lsc->lsc_aggregators, la, la_q);
		LACP_DPRINTF((lsc, lacpp, "a new aggregator created\n"));
	} else {
		LACP_DPRINTF((lsc, lacpp, "aggregator found\n"));
	}

	KASSERT(la != NULL);
	LACP_AGGREGATOR_STR(la, buf, sizeof(buf));
	LACP_DPRINTF((lsc, lacpp, "aggregator lagid=%s\n", buf));

	lacpp->lp_aggregator = la;
	lacpp->lp_selected = LACP_STANDBY;

	LIST_FOREACH(lacpp0, &la->la_ports, lp_entry_la) {
		if (lacp_port_priority_max(lacpp0, lacpp) == lacpp) {
			LIST_INSERT_BEFORE(lacpp0, lacpp, lp_entry_la);
			break;
		}

		if (LIST_NEXT(lacpp0, lp_entry_la) == NULL) {
			LIST_INSERT_AFTER(lacpp0, lacpp, lp_entry_la);
			break;
		}
	}

	if (lacpp0 == NULL)
		LIST_INSERT_HEAD(&la->la_ports, lacpp, lp_entry_la);

	lacp_selected_update(lsc, la);
}

static void
lacp_unselect(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct lacp_aggregator *la;
	char buf[LACP_SYSTEMIDSTR_LEN] __LACPDEBUGUSED;
	bool remove_actaggr;

	KASSERT(LACP_LOCKED(lsc));
	KASSERT(!LACP_TIMER_ISARMED(lacpp, LACP_TIMER_WAIT_WHILE));

	la = lacpp->lp_aggregator;
	lacpp->lp_selected = LACP_UNSELECTED;

	if (la == NULL)
		return;

	KASSERT(!LIST_EMPTY(&la->la_ports));

	LACP_AGGREGATOR_STR(la, buf, sizeof(buf));
	LACP_DPRINTF((lsc, lacpp, "unselect aggregator lagid=%s\n", buf));

	LIST_REMOVE(lacpp, lp_entry_la);
	lacpp->lp_aggregator = NULL;

	if (LIST_EMPTY(&la->la_ports)) {
		remove_actaggr = false;

		if (la == lsc->lsc_aggregator) {
			LACP_DPRINTF((lsc, NULL, "remove active aggregator\n"));
			lsc->lsc_aggregator = NULL;
			remove_actaggr = true;
		}

		TAILQ_REMOVE(&lsc->lsc_aggregators, la, la_q);
		kmem_free(la, sizeof(*la));

		if (remove_actaggr)
			lacp_select_active_aggregator(lsc);
	} else {
		lacp_selected_update(lsc, la);
	}
}

static void
lacp_suppress_distributing(struct lacp_softc *lsc)
{
	struct lacp_aggregator *la;
	struct lacp_port *lacpp;

	KASSERT(LACP_LOCKED(lsc));

	la = lsc->lsc_aggregator;

	LIST_FOREACH(lacpp, &la->la_ports, lp_entry_la) {
		if (ISSET(lacpp->lp_actor.lpi_state,
		    LACP_STATE_DISTRIBUTING)) {
			lagg_workq_add(lsc->lsc_workq,
			    &lacpp->lp_work_marker);
		}
	}

	LACP_PTIMER_ARM(lsc, LACP_PTIMER_DISTRIBUTING,
	    LACP_TRANSIT_DELAY);
}

static void
lacp_distributing_timer(struct lacp_softc *lsc)
{

	KASSERT(LACP_LOCKED(lsc));

	if (lsc->lsc_suppress_distributing) {
		LACP_DPRINTF((lsc, NULL,
		    "disable suppress distributing\n"));
		lsc->lsc_suppress_distributing = false;
	}
}

static struct mbuf *
lacp_markerdu_mbuf(struct lacp_softc *lsc, struct lacp_port *lacpp)
{
	struct ifnet *ifp_port;
	struct mbuf *m;
	struct markerdu *mdu;
	struct markerdu_info *mi;

	KASSERT(LACP_LOCKED(lsc));

	ifp_port = lacpp->lp_laggport->lp_ifp;

	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		lsc->lsc_mgethdr_failed.ev_count++;
		return NULL;
	}

	m->m_pkthdr.len = m->m_len = sizeof(*mdu);

	mdu = mtod(m, struct markerdu *);

	memset(mdu, 0, sizeof(*mdu));

	m->m_flags |= M_MCAST;
	memcpy(mdu->mdu_eh.ether_dhost, ethermulticastaddr_slowprotocols,
	    ETHER_ADDR_LEN);
	memcpy(mdu->mdu_eh.ether_shost, CLLADDR(ifp_port->if_sadl),
	    ETHER_ADDR_LEN);
	mdu->mdu_eh.ether_type = ntohs(ETHERTYPE_SLOWPROTOCOLS);
	mdu->mdu_sph.sph_subtype = SLOWPROTOCOLS_SUBTYPE_MARKER;
	mdu->mdu_sph.sph_version = 1;

	mi = &mdu->mdu_info;
	tlv_set(&mdu->mdu_tlv_info, MARKER_TYPE_INFO,
	    sizeof(*mi));
	mi->mi_rq_port = lacpp->lp_actor.lpi_portno;
	mi->mi_rq_xid = htonl(lacpp->lp_marker_xid);
	memcpy(mi->mi_rq_system, lsc->lsc_system_mac, LACP_MAC_LEN);

	mdu->mdu_tlv_term.tlv_type = MARKER_TYPE_TERMINATE;
	mdu->mdu_tlv_term.tlv_length = 0;

	return m;
}

static void
lacp_marker_work(struct lagg_work *lw, void *xlsc)
{
	struct lacp_softc *lsc;
	struct lacp_port *lacpp;
	struct lagg_port *lp;
	struct markerdu *mdu;
	struct mbuf *m;
	struct psref psref;
	int bound;

	lsc = xlsc;
	lacpp = container_of(lw, struct lacp_port, lp_work_marker);

	LACP_LOCK(lsc);
	lacpp->lp_marker_xid++;
	m = lacp_markerdu_mbuf(lsc, lacpp);
	if (m == NULL) {
		LACP_UNLOCK(lsc);
		return;
	}
	SET(lacpp->lp_flags, LACP_PORT_MARK);
	lsc->lsc_suppress_distributing = true;
	lp = lacpp->lp_laggport;
	bound = curlwp_bind();
	lagg_port_getref(lp, &psref);
	LACP_UNLOCK(lsc);

	if (LACP_ISDUMPING(lsc)) {
		lacp_dprintf(lsc, lacpp, "markerdu transmit\n");
		mdu = mtod(m, struct markerdu *);
		lacp_dump_markertlv(&mdu->mdu_info, NULL);
	}

	lagg_port_xmit(lp, m);
	lagg_port_putref(lp, &psref);
	curlwp_bindx(bound);
}

static void
lacp_dump_lacpdutlv(const struct lacpdu_peerinfo *pi_actor,
    const struct lacpdu_peerinfo *pi_partner,
    const struct lacpdu_collectorinfo *lci)
{
	char str[LACP_STATESTR_LEN];

	if (pi_actor != NULL) {
		lacp_peerinfo_idstr(pi_actor, str, sizeof(str));
		printf("actor=%s\n", str);
		lacp_state_str(pi_actor->lpi_state,
		    str, sizeof(str));
		printf("actor.state=%s portno=%d portprio=0x%04x\n",
		    str,
		    ntohs(pi_actor->lpi_port_no),
		    ntohs(pi_actor->lpi_port_prio));
	} else {
		printf("no actor info\n");
	}

	if (pi_partner != NULL) {
		lacp_peerinfo_idstr(pi_partner, str, sizeof(str));
		printf("partner=%s\n", str);
		lacp_state_str(pi_partner->lpi_state,
		    str, sizeof(str));
		printf("partner.state=%s portno=%d portprio=0x%04x\n",
		    str,
		    ntohs(pi_partner->lpi_port_no),
		    ntohs(pi_partner->lpi_port_prio));
	} else {
		printf("no partner info\n");
	}

	if (lci != NULL) {
		printf("maxdelay=%d\n", ntohs(lci->lci_maxdelay));
	} else {
		printf("no collector info\n");
	}
}

static void
lacp_dump_markertlv(const struct markerdu_info *mi_info,
    const struct markerdu_info *mi_res)
{

	if (mi_info != NULL) {
		printf("marker info: port=%d, sys=%s, id=%u\n",
		    ntohs(mi_info->mi_rq_port),
		    ether_sprintf(mi_info->mi_rq_system),
		    ntohl(mi_info->mi_rq_xid));
	}

	if (mi_res != NULL) {
		printf("marker resp: port=%d, sys=%s, id=%u\n",
		    ntohs(mi_res->mi_rq_port),
		    ether_sprintf(mi_res->mi_rq_system),
		    ntohl(mi_res->mi_rq_xid));
	}
}

static uint32_t
lacp_ifmedia2lacpmedia(u_int ifmedia)
{
	uint32_t rv;

	switch (IFM_SUBTYPE(ifmedia)) {
	case IFM_10_T:
	case IFM_10_2:
	case IFM_10_5:
	case IFM_10_STP:
	case IFM_10_FL:
		rv = LACP_LINKSPEED_10;
		break;
	case IFM_100_TX:
	case IFM_100_FX:
	case IFM_100_T4:
	case IFM_100_VG:
	case IFM_100_T2:
		rv = LACP_LINKSPEED_100;
		break;
	case IFM_1000_SX:
	case IFM_1000_LX:
	case IFM_1000_CX:
	case IFM_1000_T:
	case IFM_1000_BX10:
	case IFM_1000_KX:
		rv = LACP_LINKSPEED_1000;
		break;
	case IFM_2500_SX:
	case IFM_2500_KX:
		rv = LACP_LINKSPEED_2500;
		break;
	case IFM_5000_T:
		rv = LACP_LINKSPEED_5000;
		break;
	case IFM_10G_LR:
	case IFM_10G_SR:
	case IFM_10G_CX4:
	case IFM_10G_TWINAX:
	case IFM_10G_TWINAX_LONG:
	case IFM_10G_LRM:
	case IFM_10G_T:
		rv = LACP_LINKSPEED_10G;
		break;
	default:
		rv = LACP_LINKSPEED_UNKNOWN;
	}

	if (IFM_TYPE(ifmedia) == IFM_ETHER)
		SET(rv, LACP_MEDIA_ETHER);
	if ((ifmedia & IFM_FDX) != 0)
		SET(rv, LACP_MEDIA_FDX);

	return rv;
}

static void
lacp_linkstate(struct lagg_proto_softc *xlsc, struct lagg_port *lp)
{

	IFNET_ASSERT_UNLOCKED(lp->lp_ifp);

	IFNET_LOCK(lp->lp_ifp);
	lacp_linkstate_ifnet_locked(xlsc, lp);
	IFNET_UNLOCK(lp->lp_ifp);
}
