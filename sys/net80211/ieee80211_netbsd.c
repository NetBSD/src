/*	$NetBSD: ieee80211_netbsd.c,v 1.31.2.5 2018/07/28 00:49:43 phil Exp $ */

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2003-2009 Sam Leffler, Errno Consulting
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
 */

#include <sys/cdefs.h>
/*  __FBSDID("$FreeBSD$");  */
__KERNEL_RCSID(0, "$NetBSD: ieee80211_netbsd.c,v 1.31.2.5 2018/07/28 00:49:43 phil Exp $");

/*
 * IEEE 802.11 support (NetBSD-specific code)
 */

#include "opt_wlan.h"

#include <sys/atomic.h>
#include <sys/param.h>
#include <sys/systm.h> 
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>   
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/sysctl.h>
#include <sys/syslog.h>

#include <sys/socket.h>

#include <net/bpf.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_ether.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <net/route.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_input.h>

static const struct sysctlnode *
    ieee80211_sysctl_treetop(struct sysctllog **log);
static void ieee80211_sysctl_setup(void);

/* NNN in .h file? */
#define SYSCTL_HANDLER_ARGS SYSCTLFN_ARGS

#ifdef IEEE80211_DEBUG
static int	ieee80211_debug = 0;
#endif

#ifdef notyet
static struct if_clone *wlan_cloner;
#endif 
/* notyet */

static const char wlanname[] = "wlan";

int
ieee80211_init0(void)
{
	ieee80211_sysctl_setup();
	return 0;
}

/*
 * "taskqueue" support
 */
void ieee80211_runwork(struct work *work2do, void *arg)
{
	struct task *work_task = (struct task *) work2do;
	printf ("runwork called! work2do is 0x%lx, t_work.wk_dummy is 0x%lx\n", 
		(long) work2do, (long)work_task->t_work.wk_dummy);
	printf ("  runwork:  t_func is 0x%lx, t_arg is 0x%lx\n",
		(long)work_task->t_func, (long)work_task->t_arg);

	mutex_enter(&work_task->t_mutex);
	work_task->t_onqueue = 0;
	mutex_exit(&work_task->t_mutex);
	
	work_task->t_func(work_task->t_arg, 0);
}

void taskqueue_enqueue(struct workqueue *wq, struct task *task_item)
{
	printf ("taskqueue_enqueue called\n");
	mutex_enter(&task_item->t_mutex);
	if (!task_item->t_onqueue) {
		printf ("   taskqueue_enqueue adding item to workqueue\n");
		workqueue_enqueue(wq, &task_item->t_work, NULL);
		task_item->t_onqueue = 1;
	}
	mutex_exit(&task_item->t_mutex);
}

void taskqueue_drain(struct workqueue *wq, struct task *task_item)
{
	printf ("taskqueue_drain called\n");
	workqueue_wait(wq, &task_item->t_work);
}


static __unused int
wlan_clone_create(struct if_clone *ifc, int unit, void * params)
{
	struct ieee80211_clone_params cp;
	struct ieee80211vap *vap;
	struct ieee80211com *ic;
	int error;

	error = copyin(params, &cp, sizeof(cp));
	if (error)
		return error;
	ic = ieee80211_find_com(cp.icp_parent);
	if (ic == NULL)
		return ENXIO;
	if (cp.icp_opmode >= IEEE80211_OPMODE_MAX) {
		ic_printf(ic, "%s: invalid opmode %d\n", __func__,
		    cp.icp_opmode);
		return EINVAL;
	}
	if ((ic->ic_caps & ieee80211_opcap[cp.icp_opmode]) == 0) {
		ic_printf(ic, "%s mode not supported\n",
		    ieee80211_opmode_name[cp.icp_opmode]);
		return EOPNOTSUPP;
	}
	if ((cp.icp_flags & IEEE80211_CLONE_TDMA) &&
#ifdef IEEE80211_SUPPORT_TDMA
	    (ic->ic_caps & IEEE80211_C_TDMA) == 0
#else
	    (1)
#endif
	) {
		ic_printf(ic, "TDMA not supported\n");
		return EOPNOTSUPP;
	}
	vap = ic->ic_vap_create(ic, wlanname, unit,
			cp.icp_opmode, cp.icp_flags, cp.icp_bssid,
			cp.icp_flags & IEEE80211_CLONE_MACADDR ?
			    cp.icp_macaddr : ic->ic_macaddr);

	return (vap == NULL ? EIO : 0);
}

static __unused void
wlan_clone_destroy(struct ifnet *ifp)
{
	struct ieee80211vap *vap = ifp->if_softc;
	struct ieee80211com *ic = vap->iv_ic;

	ic->ic_vap_delete(vap);
}

void
ieee80211_vap_destroy(struct ieee80211vap *vap)
{
#ifdef notyet
	CURVNET_SET(vap->iv_ifp->if_vnet);
	if_clone_destroyif(wlan_cloner, vap->iv_ifp);
	CURVNET_RESTORE();
#else
	printf ("vap_destroy called ... what next?\n");
#endif
}

#ifdef notyet 
int
ieee80211_sysctl_msecs_ticks(SYSCTL_HANDLER_ARGS)
{
	int msecs = ticks_to_msecs(*(int *)arg1);
	int error, t;

	error = sysctl_handle_int(oidp, &msecs, 0, req);
	if (error || !req->newptr)
		return error;
	t = msecs_to_ticks(msecs);
	*(int *)arg1 = (t < 1) ? 1 : t;
	return 0;
}

static int
ieee80211_sysctl_inact(SYSCTL_HANDLER_ARGS)
{
	int inact = (*(int *)arg1) * IEEE80211_INACT_WAIT;
	int error;

	error = sysctl_handle_int(oidp, &inact, 0, req);
	if (error || !req->newptr)
		return error;
	*(int *)arg1 = inact / IEEE80211_INACT_WAIT;
	return 0;
}
#endif

static int
ieee80211_sysctl_parent(SYSCTLFN_ARGS)
{
	struct ieee80211vap *vap;
	char pname[IFNAMSIZ];
	struct sysctlnode node;

	node = *rnode;
	vap = node.sysctl_data;
	strlcpy(pname, vap->iv_ifp->if_xname, IFNAMSIZ);
	node.sysctl_data = pname;
	return sysctl_lookup(SYSCTLFN_CALL(&node));
}

#ifdef notyet
static int
ieee80211_sysctl_radar(SYSCTL_HANDLER_ARGS)
{
	struct ieee80211com *ic = arg1;
	int t = 0, error;

	error = sysctl_handle_int(oidp, &t, 0, req);
	if (error || !req->newptr)
		return error;
	IEEE80211_LOCK(ic);
	ieee80211_dfs_notify_radar(ic, ic->ic_curchan);
	IEEE80211_UNLOCK(ic);
	return 0;
}

/*
 * For now, just restart everything.
 *
 * Later on, it'd be nice to have a separate VAP restart to
 * full-device restart.
 */
static int
ieee80211_sysctl_vap_restart(SYSCTL_HANDLER_ARGS)
{
	struct ieee80211vap *vap = arg1;
	int t = 0, error;

	error = sysctl_handle_int(oidp, &t, 0, req);
	if (error || !req->newptr)
		return error;

	ieee80211_restart_all(vap->iv_ic);
	return 0;
}
#endif /* notyet */

void
ieee80211_sysctl_attach(struct ieee80211com *ic)
{
}

void
ieee80211_sysctl_detach(struct ieee80211com *ic)
{
}

/*
 * Setup sysctl(3) MIB, net.ieee80211.*
 *
 * TBD condition CTLFLAG_PERMANENT on being a module or not
 */
static struct sysctllog *ieee80211_sysctllog;
static void
ieee80211_sysctl_setup(void)
{
	int rc;
	const struct sysctlnode *rnode;

	if ((rnode = ieee80211_sysctl_treetop(&ieee80211_sysctllog)) == NULL)
		return;

#ifdef notyet
	if ((rc = sysctl_createv(&ieee80211_sysctllog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "nodes", "client/peer stations",
	    ieee80211_sysctl_node, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif

#ifdef IEEE80211_DEBUG
	/* control debugging printfs */
	if ((rc = sysctl_createv(&ieee80211_sysctllog, 0, &rnode, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("control debugging printfs"),
	    NULL, 0, &ieee80211_debug, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;
#endif

#ifdef notyet
	ieee80211_rssadapt_sysctl_setup(&ieee80211_sysctllog);
#endif

	return;
err:
	printf("%s: sysctl_createv failed (rc = %d)\n", __func__, rc);
}

/*
 * Create or get top of sysctl tree net.link.ieee80211.
 */
static const struct sysctlnode *
ieee80211_sysctl_treetop(struct sysctllog **log)
{
	int rc;
	const struct sysctlnode *rnode;

	if ((rc = sysctl_createv(log, 0, NULL, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "link",
	    "link-layer statistics and controls",
	    NULL, 0, NULL, 0, CTL_NET, PF_LINK, CTL_EOL)) != 0)
		goto err;

	if ((rc = sysctl_createv(log, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, "ieee80211",
	    "IEEE 802.11 WLAN statistics and controls",
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	return rnode;
err:
	printf("%s: sysctl_createv failed, rc = %d\n", __func__, rc);
	return NULL;
}

void
ieee80211_sysctl_vattach(struct ieee80211vap *vap)
{
	int rc;
	const struct sysctlnode *cnode, *rnode;
	char num[sizeof("vap") + 14];		/* sufficient for 32 bits */

	if ((rnode = ieee80211_sysctl_treetop(NULL)) == NULL)
		return;

	snprintf(num, sizeof(num), "vap%u", vap->iv_ifp->if_index);

	if ((rc = sysctl_createv(&vap->iv_sysctllog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT, CTLTYPE_NODE, num, SYSCTL_DESCR("virtual AP"),
	    NULL, 0, NULL, 0, CTL_CREATE, CTL_EOL)) != 0)
		goto err;

	/* control debugging printfs */
	if ((rc = sysctl_createv(&vap->iv_sysctllog, 0, &rnode, &cnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READONLY, CTLTYPE_STRING,
	    "parent", SYSCTL_DESCR("parent device"),
	    ieee80211_sysctl_parent, 0, (void *)vap, IFNAMSIZ,
	    CTL_CREATE, CTL_EOL)) != 0)
		goto err;


#ifdef notyet
	struct ifnet *ifp = vap->iv_ifp;
	struct sysctl_ctx_list *ctx;
	struct sysctl_oid *oid;
	char num[14];			/* sufficient for 32 bits */

	ctx = (struct sysctl_ctx_list *) IEEE80211_MALLOC(sizeof(struct sysctl_ctx_list),
		M_DEVBUF, IEEE80211_M_NOWAIT | IEEE80211_M_ZERO);
	if (ctx == NULL) {
		if_printf(ifp, "%s: cannot allocate sysctl context!\n",
			__func__);
		return;
	}
	sysctl_ctx_init(ctx);
	snprintf(num, sizeof(num), "%u", ifp->if_dunit);
	oid = SYSCTL_ADD_NODE(ctx, &SYSCTL_NODE_CHILDREN(_net, wlan),
		OID_AUTO, num, CTLFLAG_RD, NULL, "");
	SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"%parent", CTLTYPE_STRING | CTLFLAG_RD, vap->iv_ic, 0,
		ieee80211_sysctl_parent, "A", "parent device");
	SYSCTL_ADD_UINT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"driver_caps", CTLFLAG_RW, &vap->iv_caps, 0,
		"driver capabilities");
#ifdef IEEE80211_DEBUG
	vap->iv_debug = ieee80211_debug;
	SYSCTL_ADD_UINT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"debug", CTLFLAG_RW, &vap->iv_debug, 0,
		"control debugging printfs");
#endif
	SYSCTL_ADD_INT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"bmiss_max", CTLFLAG_RW, &vap->iv_bmiss_max, 0,
		"consecutive beacon misses before scanning");
	/* XXX inherit from tunables */
	SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"inact_run", CTLTYPE_INT | CTLFLAG_RW, &vap->iv_inact_run, 0,
		ieee80211_sysctl_inact, "I",
		"station inactivity timeout (sec)");
	SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"inact_probe", CTLTYPE_INT | CTLFLAG_RW, &vap->iv_inact_probe, 0,
		ieee80211_sysctl_inact, "I",
		"station inactivity probe timeout (sec)");
	SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"inact_auth", CTLTYPE_INT | CTLFLAG_RW, &vap->iv_inact_auth, 0,
		ieee80211_sysctl_inact, "I",
		"station authentication timeout (sec)");
	SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"inact_init", CTLTYPE_INT | CTLFLAG_RW, &vap->iv_inact_init, 0,
		ieee80211_sysctl_inact, "I",
		"station initial state timeout (sec)");
	if (vap->iv_htcaps & IEEE80211_HTC_HT) {
		SYSCTL_ADD_UINT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
			"ampdu_mintraffic_bk", CTLFLAG_RW,
			&vap->iv_ampdu_mintraffic[WME_AC_BK], 0,
			"BK traffic tx aggr threshold (pps)");
		SYSCTL_ADD_UINT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
			"ampdu_mintraffic_be", CTLFLAG_RW,
			&vap->iv_ampdu_mintraffic[WME_AC_BE], 0,
			"BE traffic tx aggr threshold (pps)");
		SYSCTL_ADD_UINT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
			"ampdu_mintraffic_vo", CTLFLAG_RW,
			&vap->iv_ampdu_mintraffic[WME_AC_VO], 0,
			"VO traffic tx aggr threshold (pps)");
		SYSCTL_ADD_UINT(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
			"ampdu_mintraffic_vi", CTLFLAG_RW,
			&vap->iv_ampdu_mintraffic[WME_AC_VI], 0,
			"VI traffic tx aggr threshold (pps)");
	}

	SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
		"force_restart", CTLTYPE_INT | CTLFLAG_RW, vap, 0,
		ieee80211_sysctl_vap_restart, "I",
		"force a VAP restart");

	if (vap->iv_caps & IEEE80211_C_DFS) {
		SYSCTL_ADD_PROC(ctx, SYSCTL_CHILDREN(oid), OID_AUTO,
			"radar", CTLTYPE_INT | CTLFLAG_RW, vap->iv_ic, 0,
			ieee80211_sysctl_radar, "I", "simulate radar event");
	}
	vap->iv_sysctl = ctx;
	vap->iv_oid = oid;
#endif
	return;
err:
	printf("%s: sysctl_createv failed, rc = %d\n", __func__, rc);
}

void
ieee80211_sysctl_vdetach(struct ieee80211vap *vap)
{
#ifdef notyet
	if (vap->iv_sysctl != NULL) {
		sysctl_ctx_free(vap->iv_sysctl);
		IEEE80211_FREE(vap->iv_sysctl, M_DEVBUF);
		vap->iv_sysctl = NULL;
	}
#endif
}


int
ieee80211_node_dectestref(struct ieee80211_node *ni)
{
	/* XXX need equivalent of atomic_dec_and_test */
	atomic_subtract_int(&ni->ni_refcnt, 1);
	return atomic_cas_uint(&ni->ni_refcnt, 0, 1) == 0;
}

void
ieee80211_drain_ifq(struct ifqueue *ifq)
{
	struct ieee80211_node *ni;
	struct mbuf *m;

	for (;;) {
		IF_DEQUEUE(ifq, m);
		if (m == NULL)
			break;

		ni = (struct ieee80211_node *)m_get_rcvif_NOMPSAFE(m);
		FBSDKASSERT(ni != NULL, ("frame w/o node"));
		ieee80211_free_node(ni);
		ieee80211_free_mbuf(m);
	}
}

void
ieee80211_flush_ifq(struct ifqueue *ifq, struct ieee80211vap *vap)
{
	struct ieee80211_node *ni;
	struct mbuf *m, **mprev;

	IFQ_LOCK(ifq);
	mprev = &ifq->ifq_head;
	while ((m = *mprev) != NULL) {
		ni = (struct ieee80211_node *)m_get_rcvif_NOMPSAFE(m);
		if (ni != NULL && ni->ni_vap == vap) {
			*mprev = m->m_nextpkt;		/* remove from list */
			ifq->ifq_len--;

			ieee80211_free_node(ni);	/* reclaim ref */
			ieee80211_free_mbuf(m);
		} else
			mprev = &m->m_nextpkt;
	}
	/* recalculate tail ptr */
	m = ifq->ifq_head;
	for (; m != NULL && m->m_nextpkt != NULL; m = m->m_nextpkt)
		;
	ifq->ifq_tail = m;
	IFQ_UNLOCK(ifq);
}

/*
 * As above, for mbufs allocated with m_gethdr/MGETHDR
 * or initialized by M_COPY_PKTHDR.
 */
#define	MC_ALIGN(m, len)						\
do {									\
	(m)->m_data += rounddown2(MCLBYTES - (len), sizeof(long));	\
} while (/* CONSTCOND */ 0)

/*
 * Allocate and setup a management frame of the specified
 * size.  We return the mbuf and a pointer to the start
 * of the contiguous data area that's been reserved based
 * on the packet length.  The data area is forced to 32-bit
 * alignment and the buffer length to a multiple of 4 bytes.
 * This is done mainly so beacon frames (that require this)
 * can use this interface too.
 */
struct mbuf *
ieee80211_getmgtframe(uint8_t **frm, int headroom, int pktlen)
{
	struct mbuf *m;
	u_int len;

	/*
	 * NB: we know the mbuf routines will align the data area
	 *     so we don't need to do anything special.
	 */
	len = roundup2(headroom + pktlen, 4);
	FBSDKASSERT(len <= MCLBYTES, ("802.11 mgt frame too large: %u", len));
	if (len < MINCLSIZE) {
		m = m_gethdr(M_NOWAIT, MT_DATA);
		/*
		 * Align the data in case additional headers are added.
		 * This should only happen when a WEP header is added
		 * which only happens for shared key authentication mgt
		 * frames which all fit in MHLEN.
		 */
		if (m != NULL)
			MH_ALIGN(m, len);
	} else {
		m = m_getcl(M_NOWAIT, MT_DATA, M_PKTHDR);
		if (m != NULL)
			MC_ALIGN(m, len);
	}
	if (m != NULL) {
		m->m_data += headroom;
		*frm = m->m_data;
	}
	return m;
}

#ifndef __NO_STRICT_ALIGNMENT
/*
 * Re-align the payload in the mbuf.  This is mainly used (right now)
 * to handle IP header alignment requirements on certain architectures.
 */
struct mbuf *
ieee80211_realign(struct ieee80211vap *vap, struct mbuf *m, size_t align)
{
	int pktlen, space;
	struct mbuf *n;

	pktlen = m->m_pkthdr.len;
	space = pktlen + align;
	if (space < MINCLSIZE)
		n = m_gethdr(M_NOWAIT, MT_DATA);
	else {
		n = m_getjcl(M_NOWAIT, MT_DATA, M_PKTHDR,
		    space <= MCLBYTES ?     MCLBYTES :
#if MJUMPAGESIZE != MCLBYTES
		    space <= MJUMPAGESIZE ? MJUMPAGESIZE :
#endif
		    space <= MJUM9BYTES ?   MJUM9BYTES : MJUM16BYTES);
	}
	if (__predict_true(n != NULL)) {
		m_move_pkthdr(n, m);
		n->m_data = (caddr_t)(ALIGN(n->m_data + align) - align);
		m_copydata(m, 0, pktlen, mtod(n, caddr_t));
		n->m_len = pktlen;
	} else {
		IEEE80211_DISCARD(vap, IEEE80211_MSG_ANY,
		    mtod(m, const struct ieee80211_frame *), NULL,
		    "%s", "no mbuf to realign");
		vap->iv_stats.is_rx_badalign++;
	}
	m_freem(m);
	return n;
}
#endif /* !__NO_STRICT_ALIGNMENT */

int
ieee80211_add_callback(struct mbuf *m,
	void (*func)(struct ieee80211_node *, void *, int), void *arg)
{
	struct m_tag *mtag;
	struct ieee80211_cb *cb;

	mtag = m_tag_get(/*MTAG_ABI_NET80211*/ NET80211_TAG_CALLBACK,
			sizeof(struct ieee80211_cb), M_NOWAIT);
	if (mtag == NULL)
		return 0;

	cb = (struct ieee80211_cb *)(mtag+1);
	cb->func = func;
	cb->arg = arg;
	m_tag_prepend(m, mtag);
	m->m_flags |= M_TXCB;
	return 1;
}

int
ieee80211_add_xmit_params(struct mbuf *m,
    const struct ieee80211_bpf_params *params)
{
	struct m_tag *mtag;
	struct ieee80211_tx_params *tx;

	mtag = m_tag_get(/*MTAG_ABI_NET80211*/ NET80211_TAG_XMIT_PARAMS,
	    sizeof(struct ieee80211_tx_params), M_NOWAIT);
	if (mtag == NULL)
		return (0);

	tx = (struct ieee80211_tx_params *)(mtag+1);
	memcpy(&tx->params, params, sizeof(struct ieee80211_bpf_params));
	m_tag_prepend(m, mtag);
	return (1);
}

int
ieee80211_get_xmit_params(struct mbuf *m,
    struct ieee80211_bpf_params *params)
{
	struct m_tag *mtag;
	struct ieee80211_tx_params *tx;

	mtag = m_tag_find(m, /*MTAG_ABI_NET80211,*/ NET80211_TAG_XMIT_PARAMS,
	    NULL);
	if (mtag == NULL)
		return (-1);
	tx = (struct ieee80211_tx_params *)(mtag + 1);
	memcpy(params, &tx->params, sizeof(struct ieee80211_bpf_params));
	return (0);
}

void
ieee80211_process_callback(struct ieee80211_node *ni,
	struct mbuf *m, int status)
{
	struct m_tag *mtag;

	mtag = m_tag_find(m, /*MTAG_ABI_NET80211,*/ NET80211_TAG_CALLBACK, NULL);
	if (mtag != NULL) {
		struct ieee80211_cb *cb = (struct ieee80211_cb *)(mtag+1);
		cb->func(ni, cb->arg, status);
	}
}

/*
 * Add RX parameters to the given mbuf.
 *
 * Returns 1 if OK, 0 on error.
 */
int
ieee80211_add_rx_params(struct mbuf *m, const struct ieee80211_rx_stats *rxs)
{
	struct m_tag *mtag;
	struct ieee80211_rx_params *rx;

	mtag = m_tag_get(/*MTAG_ABI_NET80211,*/ NET80211_TAG_RECV_PARAMS,
	    sizeof(struct ieee80211_rx_stats), M_NOWAIT);
	if (mtag == NULL)
		return (0);

	rx = (struct ieee80211_rx_params *)(mtag + 1);
	memcpy(&rx->params, rxs, sizeof(*rxs));
	m_tag_prepend(m, mtag);
	return (1);
}

int
ieee80211_get_rx_params(struct mbuf *m, struct ieee80211_rx_stats *rxs)
{
	struct m_tag *mtag;
	struct ieee80211_rx_params *rx;

	mtag = m_tag_find(m, /*MTAG_ABI_NET80211,*/ NET80211_TAG_RECV_PARAMS,
	    NULL);
	if (mtag == NULL)
		return (-1);
	rx = (struct ieee80211_rx_params *)(mtag + 1);
	memcpy(rxs, &rx->params, sizeof(*rxs));
	return (0);
}

const struct ieee80211_rx_stats *
ieee80211_get_rx_params_ptr(struct mbuf *m)
{
	struct m_tag *mtag;
	struct ieee80211_rx_params *rx;

	mtag = m_tag_find(m, /*MTAG_ABI_NET80211,*/ NET80211_TAG_RECV_PARAMS,
	    NULL);
	if (mtag == NULL)
		return (NULL);
	rx = (struct ieee80211_rx_params *)(mtag + 1);
	return (&rx->params);
}


/*
 * Add TOA parameters to the given mbuf.
 */
int
ieee80211_add_toa_params(struct mbuf *m, const struct ieee80211_toa_params *p)
{
	struct m_tag *mtag;
	struct ieee80211_toa_params *rp;

	mtag = m_tag_get(/*MTAG_ABI_NET80211,*/ NET80211_TAG_TOA_PARAMS,
	    sizeof(struct ieee80211_toa_params), M_NOWAIT);
	if (mtag == NULL)
		return (0);

	rp = (struct ieee80211_toa_params *)(mtag + 1);
	memcpy(rp, p, sizeof(*rp));
	m_tag_prepend(m, mtag);
	return (1);
}

int
ieee80211_get_toa_params(struct mbuf *m, struct ieee80211_toa_params *p)
{
	struct m_tag *mtag;
	struct ieee80211_toa_params *rp;

	mtag = m_tag_find(m, /*MTAG_ABI_NET80211,*/ NET80211_TAG_TOA_PARAMS,
	    NULL);
	if (mtag == NULL)
		return (0);
	rp = (struct ieee80211_toa_params *)(mtag + 1);
	if (p != NULL)
		memcpy(p, rp, sizeof(*p));
	return (1);
}

/*
 * Transmit a frame to the parent interface.
 */
int
ieee80211_parent_xmitpkt(struct ieee80211com *ic, struct mbuf *m)
{
	int error;
	printf ("ieee80211_parent_xmitpkt called\n");
	/*
	 * Assert the IC TX lock is held - this enforces the
	 * processing -> queuing order is maintained
	 */
	IEEE80211_TX_LOCK_ASSERT(ic);
	error = ic->ic_transmit(ic, m);
	if (error) {
		struct ieee80211_node *ni;

		ni = (struct ieee80211_node *)m_get_rcvif_NOMPSAFE(m);

		/* XXX number of fragments */
		if_inc_counter(ni->ni_vap->iv_ifp, IFCOUNTER_OERRORS, 1);
		ieee80211_free_node(ni);
		ieee80211_free_mbuf(m);
	}
	return (error);
}

/*
 * Transmit a frame to the VAP interface.
 */
int
ieee80211_vap_xmitpkt(struct ieee80211vap *vap, struct mbuf *m)
{
	struct ifnet *ifp = vap->iv_ifp;

	/*
	 * When transmitting via the VAP, we shouldn't hold
	 * any IC TX lock as the VAP TX path will acquire it.
	 */
	IEEE80211_TX_UNLOCK_ASSERT(vap->iv_ic);

	return (ifp->if_transmit(ifp, m));

}

void
get_random_bytes(void *p, size_t n)
{
	uint8_t *dp = p;

	while (n > 0) {
		uint32_t v = arc4random();
		size_t nb = n > sizeof(uint32_t) ? sizeof(uint32_t) : n;
		bcopy(&v, dp, n > sizeof(uint32_t) ? sizeof(uint32_t) : n);
		dp += sizeof(uint32_t), n -= nb;
	}
}

/*
 * Helper function for events that pass just a single mac address.
 */
static void
notify_macaddr(struct ifnet *ifp, int op, const uint8_t mac[IEEE80211_ADDR_LEN])
{
	struct ieee80211_join_event iev;

	CURVNET_SET(ifp->if_vnet);
	memset(&iev, 0, sizeof(iev));
	IEEE80211_ADDR_COPY(iev.iev_addr, mac);
	rt_ieee80211msg(ifp, op, &iev, sizeof(iev));
	CURVNET_RESTORE();
}

void
ieee80211_notify_node_join(struct ieee80211_node *ni, int newassoc)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	CURVNET_SET_QUIET(ifp->if_vnet);
	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%snode join",
	    (ni == vap->iv_bss) ? "bss " : "");

	if (ni == vap->iv_bss) {
		notify_macaddr(ifp, newassoc ?
		    RTM_IEEE80211_ASSOC : RTM_IEEE80211_REASSOC, ni->ni_bssid);
		if_link_state_change(ifp, LINK_STATE_UP);
	} else {
		notify_macaddr(ifp, newassoc ?
		    RTM_IEEE80211_JOIN : RTM_IEEE80211_REJOIN, ni->ni_macaddr);
	}
	CURVNET_RESTORE();
}

void
ieee80211_notify_node_leave(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	CURVNET_SET_QUIET(ifp->if_vnet);
	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%snode leave",
	    (ni == vap->iv_bss) ? "bss " : "");

	if (ni == vap->iv_bss) {
		rt_ieee80211msg(ifp, RTM_IEEE80211_DISASSOC, NULL, 0);
		if_link_state_change(ifp, LINK_STATE_DOWN);
	} else {
		/* fire off wireless event station leaving */
		notify_macaddr(ifp, RTM_IEEE80211_LEAVE, ni->ni_macaddr);
	}
	CURVNET_RESTORE();
}

void
ieee80211_notify_scan_done(struct ieee80211vap *vap)
{
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_DPRINTF(vap, IEEE80211_MSG_SCAN, "%s\n", "notify scan done");

	/* dispatch wireless event indicating scan completed */
	CURVNET_SET(ifp->if_vnet);
	rt_ieee80211msg(ifp, RTM_IEEE80211_SCAN, NULL, 0);
	CURVNET_RESTORE();
}

void
ieee80211_notify_replay_failure(struct ieee80211vap *vap,
	const struct ieee80211_frame *wh, const struct ieee80211_key *k,
	u_int64_t rsc, int tid)
{
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_CRYPTO, wh->i_addr2,
	    "%s replay detected tid %d <rsc %ju, csc %ju, keyix %u rxkeyix %u>",
	    k->wk_cipher->ic_name, tid, (intmax_t) rsc,
	    (intmax_t) k->wk_keyrsc[tid],
	    k->wk_keyix, k->wk_rxkeyix);

	if (ifp != NULL) {		/* NB: for cipher test modules */
		struct ieee80211_replay_event iev;

		IEEE80211_ADDR_COPY(iev.iev_dst, wh->i_addr1);
		IEEE80211_ADDR_COPY(iev.iev_src, wh->i_addr2);
		iev.iev_cipher = k->wk_cipher->ic_cipher;
		if (k->wk_rxkeyix != IEEE80211_KEYIX_NONE)
			iev.iev_keyix = k->wk_rxkeyix;
		else
			iev.iev_keyix = k->wk_keyix;
		iev.iev_keyrsc = k->wk_keyrsc[tid];
		iev.iev_rsc = rsc;
		CURVNET_SET(ifp->if_vnet);
		rt_ieee80211msg(ifp, RTM_IEEE80211_REPLAY, &iev, sizeof(iev));
		CURVNET_RESTORE();
	}
}

void
ieee80211_notify_michael_failure(struct ieee80211vap *vap,
	const struct ieee80211_frame *wh, u_int keyix)
{
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_NOTE_MAC(vap, IEEE80211_MSG_CRYPTO, wh->i_addr2,
	    "michael MIC verification failed <keyix %u>", keyix);
	vap->iv_stats.is_rx_tkipmic++;

	if (ifp != NULL) {		/* NB: for cipher test modules */
		struct ieee80211_michael_event iev;

		IEEE80211_ADDR_COPY(iev.iev_dst, wh->i_addr1);
		IEEE80211_ADDR_COPY(iev.iev_src, wh->i_addr2);
		iev.iev_cipher = IEEE80211_CIPHER_TKIP;
		iev.iev_keyix = keyix;
		CURVNET_SET(ifp->if_vnet);
		rt_ieee80211msg(ifp, RTM_IEEE80211_MICHAEL, &iev, sizeof(iev));
		CURVNET_RESTORE();
	}
}

void
ieee80211_notify_wds_discover(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	notify_macaddr(ifp, RTM_IEEE80211_WDS, ni->ni_macaddr);
}

void
ieee80211_notify_csa(struct ieee80211com *ic,
	const struct ieee80211_channel *c, int mode, int count)
{
	struct ieee80211_csa_event iev;
	struct ieee80211vap *vap;
	struct ifnet *ifp;

	memset(&iev, 0, sizeof(iev));
	iev.iev_flags = c->ic_flags;
	iev.iev_freq = c->ic_freq;
	iev.iev_ieee = c->ic_ieee;
	iev.iev_mode = mode;
	iev.iev_count = count;
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		ifp = vap->iv_ifp;
		CURVNET_SET(ifp->if_vnet);
		rt_ieee80211msg(ifp, RTM_IEEE80211_CSA, &iev, sizeof(iev));
		CURVNET_RESTORE();
	}
}

void
ieee80211_notify_radar(struct ieee80211com *ic,
	const struct ieee80211_channel *c)
{
	struct ieee80211_radar_event iev;
	struct ieee80211vap *vap;
	struct ifnet *ifp;

	memset(&iev, 0, sizeof(iev));
	iev.iev_flags = c->ic_flags;
	iev.iev_freq = c->ic_freq;
	iev.iev_ieee = c->ic_ieee;
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		ifp = vap->iv_ifp;
		CURVNET_SET(ifp->if_vnet);
		rt_ieee80211msg(ifp, RTM_IEEE80211_RADAR, &iev, sizeof(iev));
		CURVNET_RESTORE();
	}
}

void
ieee80211_notify_cac(struct ieee80211com *ic,
	const struct ieee80211_channel *c, enum ieee80211_notify_cac_event type)
{
	struct ieee80211_cac_event iev;
	struct ieee80211vap *vap;
	struct ifnet *ifp;

	memset(&iev, 0, sizeof(iev));
	iev.iev_flags = c->ic_flags;
	iev.iev_freq = c->ic_freq;
	iev.iev_ieee = c->ic_ieee;
	iev.iev_type = type;
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		ifp = vap->iv_ifp;
		CURVNET_SET(ifp->if_vnet);
		rt_ieee80211msg(ifp, RTM_IEEE80211_CAC, &iev, sizeof(iev));
		CURVNET_RESTORE();
	}
}

void
ieee80211_notify_node_deauth(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%s", "node deauth");

	notify_macaddr(ifp, RTM_IEEE80211_DEAUTH, ni->ni_macaddr);
}

void
ieee80211_notify_node_auth(struct ieee80211_node *ni)
{
	struct ieee80211vap *vap = ni->ni_vap;
	struct ifnet *ifp = vap->iv_ifp;

	IEEE80211_NOTE(vap, IEEE80211_MSG_NODE, ni, "%s", "node auth");

	notify_macaddr(ifp, RTM_IEEE80211_AUTH, ni->ni_macaddr);
}

void
ieee80211_notify_country(struct ieee80211vap *vap,
	const uint8_t bssid[IEEE80211_ADDR_LEN], const uint8_t cc[2])
{
	struct ifnet *ifp = vap->iv_ifp;
	struct ieee80211_country_event iev;

	memset(&iev, 0, sizeof(iev));
	IEEE80211_ADDR_COPY(iev.iev_addr, bssid);
	iev.iev_cc[0] = cc[0];
	iev.iev_cc[1] = cc[1];
	CURVNET_SET(ifp->if_vnet);
	rt_ieee80211msg(ifp, RTM_IEEE80211_COUNTRY, &iev, sizeof(iev));
	CURVNET_RESTORE();
}

void
ieee80211_notify_radio(struct ieee80211com *ic, int state)
{
	struct ieee80211_radio_event iev;
	struct ieee80211vap *vap;
	struct ifnet *ifp;

	memset(&iev, 0, sizeof(iev));
	iev.iev_state = state;
	TAILQ_FOREACH(vap, &ic->ic_vaps, iv_next) {
		ifp = vap->iv_ifp;
		CURVNET_SET(ifp->if_vnet);
		rt_ieee80211msg(ifp, RTM_IEEE80211_RADIO, &iev, sizeof(iev));
		CURVNET_RESTORE();
	}
}

#ifdef notyet
void
ieee80211_load_module(const char *modname)
{
	struct thread *td = curthread;

	if (suser(td) == 0 && securelevel_gt(td->td_ucred, 0) == 0) {
		mtx_lock(&Giant);
		(void) linker_load_module(modname, NULL, NULL, NULL, NULL);
		mtx_unlock(&Giant);
	}
}
#endif

#ifdef notyet
static eventhandler_tag wlan_bpfevent;
static eventhandler_tag wlan_ifllevent;

static void
bpf_track(void *arg, struct ifnet *ifp, int dlt, int attach)
{
	/* NB: identify vap's by if_init */
	if (dlt == DLT_IEEE802_11_RADIO &&
	    ifp->if_init == ieee80211_init) {
		struct ieee80211vap *vap = ifp->if_softc;
		/*
		 * Track bpf radiotap listener state.  We mark the vap
		 * to indicate if any listener is present and the com
		 * to indicate if any listener exists on any associated
		 * vap.  This flag is used by drivers to prepare radiotap
		 * state only when needed.
		 */
		if (attach) {
			ieee80211_syncflag_ext(vap, IEEE80211_FEXT_BPF);
			if (vap->iv_opmode == IEEE80211_M_MONITOR)
				atomic_add_int(&vap->iv_ic->ic_montaps, 1);
		} else if (!bpf_peers_present(vap->iv_rawbpf)) {
			ieee80211_syncflag_ext(vap, -IEEE80211_FEXT_BPF);
			if (vap->iv_opmode == IEEE80211_M_MONITOR)
				atomic_subtract_int(&vap->iv_ic->ic_montaps, 1);
		}
	}
}

/*
 * Change MAC address on the vap (if was not started).
 */
static void
wlan_iflladdr(void *arg __unused, struct ifnet *ifp)
{
	/* NB: identify vap's by if_init */
	if (ifp->if_init == ieee80211_init &&
	    (ifp->if_flags & IFF_UP) == 0) {
		struct ieee80211vap *vap = ifp->if_softc;

		IEEE80211_ADDR_COPY(vap->iv_myaddr, IF_LLADDR(ifp));
	}
}
#endif

void
if_inc_counter(struct ifnet *ifp, ift_counter ifc, int64_t value)
{
	switch (ifc) {
	case IFCOUNTER_IPACKETS:
		ifp->if_data.ifi_ipackets += value;
		break;
	case IFCOUNTER_IERRORS:
		ifp->if_data.ifi_ierrors += value;
		break;
	case IFCOUNTER_OPACKETS:
		ifp->if_data.ifi_opackets += value;
		break;
	case IFCOUNTER_OERRORS:
		ifp->if_data.ifi_oerrors += value;
		break;
        case IFCOUNTER_COLLISIONS:
		ifp->if_data.ifi_collisions += value;
		break;
        case IFCOUNTER_IBYTES:
		ifp->if_data.ifi_ibytes += value;
		break;
        case IFCOUNTER_OBYTES:
		ifp->if_data.ifi_obytes += value;
		break;
        case IFCOUNTER_IMCASTS:
		ifp->if_data.ifi_imcasts += value;
		break;
        case IFCOUNTER_OMCASTS:
		ifp->if_data.ifi_omcasts += value;
		break;
        case IFCOUNTER_IQDROPS:
		ifp->if_data.ifi_iqdrops += value;
		break;
        case IFCOUNTER_OQDROPS:
		/* ifp->if_data.ifi_oqdrops += value; No such field, just ignore it q*/
		break;
        case IFCOUNTER_NOPROTO:
		ifp->if_data.ifi_noproto += value;
		break;
	default:
		panic("if_inc_counter: non-existant counter");
	} 
}


#ifdef notyet
/*
 * Module glue.
 *
 * NB: the module name is "wlan" for compatibility with NetBSD.
 */
static int
wlan_modevent(module_t mod, int type, void *unused)
{
	switch (type) {
	case MOD_LOAD:
		if (bootverbose)
			printf("wlan: <802.11 Link Layer>\n");
		wlan_bpfevent = EVENTHANDLER_REGISTER(bpf_track,
		    bpf_track, 0, EVENTHANDLER_PRI_ANY);
		wlan_ifllevent = EVENTHANDLER_REGISTER(iflladdr_event,
		    wlan_iflladdr, NULL, EVENTHANDLER_PRI_ANY);
		wlan_cloner = if_clone_simple(wlanname, wlan_clone_create,
		    wlan_clone_destroy, 0);
		return 0;
	case MOD_UNLOAD:
		if_clone_detach(wlan_cloner);
		EVENTHANDLER_DEREGISTER(bpf_track, wlan_bpfevent);
		EVENTHANDLER_DEREGISTER(iflladdr_event, wlan_ifllevent);
		return 0;
	}
	return EINVAL;
}

static moduledata_t wlan_mod = {
	wlanname,
	wlan_modevent,
	0
};
DECLARE_MODULE(wlan, wlan_mod, SI_SUB_DRIVERS, SI_ORDER_FIRST);
MODULE_VERSION(wlan, 1);
MODULE_DEPEND(wlan, ether, 1, 1, 1);
#endif

#ifdef	IEEE80211_ALQ
MODULE_DEPEND(wlan, alq, 1, 1, 1);
#endif	/* IEEE80211_ALQ */

/* Missing support for if_printf in NetBSD ... */
int
if_printf(struct ifnet *ifp, const char *fmt, ...)
{
        char if_fmt[256];
        va_list ap;

        snprintf(if_fmt, sizeof(if_fmt), "%s: %s", ifp->if_xname, fmt);
        va_start(ap, fmt);
        vlog(LOG_INFO, if_fmt, ap);
        va_end(ap);
        return (0);
}

/*
 * Set the m_data pointer of a newly-allocated mbuf
 * to place an object of the specified size at the
 * end of the mbuf, longword aligned.
 */
void
m_align(struct mbuf *m, int len)
{
	int adjust;

	KASSERT(len != M_COPYALL);

	if (m->m_flags & M_EXT)
		adjust = m->m_ext.ext_size - len;
	else if (m->m_flags & M_PKTHDR)
		adjust = MHLEN - len;
	else
		adjust = MLEN - len;
	m->m_data += adjust &~ (sizeof(long)-1);
}

/*
 * Append the specified data to the indicated mbuf chain,
 * Extend the mbuf chain if the new data does not fit in
 * existing space.
 *
 * Return 1 if able to complete the job; otherwise 0.
 */
int
m_append(struct mbuf *m0, int len, const void *cpv)
{
	struct mbuf *m, *n;
	int remainder, space;
	const char *cp = cpv;

	KASSERT(len != M_COPYALL);
	for (m = m0; m->m_next != NULL; m = m->m_next)
		continue;
	remainder = len;
	space = M_TRAILINGSPACE(m);
	if (space > 0) {
		/*
		 * Copy into available space.
		 */
		if (space > remainder)
			space = remainder;
		memmove(mtod(m, char *) + m->m_len, cp, space);
		m->m_len += space;
		cp = cp + space, remainder -= space;
	}
	while (remainder > 0) {
		/*
		 * Allocate a new mbuf; could check space
		 * and allocate a cluster instead.
		 */
		n = m_get(M_DONTWAIT, m->m_type);
		if (n == NULL)
			break;
		n->m_len = min(MLEN, remainder);
		memmove(mtod(n, void *), cp, n->m_len);
		cp += n->m_len, remainder -= n->m_len;
		m->m_next = n;
		m = n;
	}
	if (m0->m_flags & M_PKTHDR)
		m0->m_pkthdr.len += len - remainder;
	return (remainder == 0);
}

/*
 * Create a writable copy of the mbuf chain.  While doing this
 * we compact the chain with a goal of producing a chain with
 * at most two mbufs.  The second mbuf in this chain is likely
 * to be a cluster.  The primary purpose of this work is to create
 * a writable packet for encryption, compression, etc.  The
 * secondary goal is to linearize the data so the data can be
 * passed to crypto hardware in the most efficient manner possible.
 */
struct mbuf *
m_unshare(struct mbuf *m0, int how)
{
	struct mbuf *m, *mprev;
	struct mbuf *n, *mfirst, *mlast;
	int len, off;

	mprev = NULL;
	for (m = m0; m != NULL; m = mprev->m_next) {
		/*
		 * Regular mbufs are ignored unless there's a cluster
		 * in front of it that we can use to coalesce.  We do
		 * the latter mainly so later clusters can be coalesced
		 * also w/o having to handle them specially (i.e. convert
		 * mbuf+cluster -> cluster).  This optimization is heavily
		 * influenced by the assumption that we're running over
		 * Ethernet where MCLBYTES is large enough that the max
		 * packet size will permit lots of coalescing into a
		 * single cluster.  This in turn permits efficient
		 * crypto operations, especially when using hardware.
		 */
		if ((m->m_flags & M_EXT) == 0) {
			if (mprev && (mprev->m_flags & M_EXT) &&
			    m->m_len <= M_TRAILINGSPACE(mprev)) {
				/* XXX: this ignores mbuf types */
				memcpy(mtod(mprev, __uint8_t *) + mprev->m_len,
				    mtod(m, __uint8_t *), m->m_len);
				mprev->m_len += m->m_len;
				mprev->m_next = m->m_next;	/* unlink from chain */
				m_free(m);			/* reclaim mbuf */
			} else {
				mprev = m;
			}
			continue;
		}
		/*
		 * Writable mbufs are left alone (for now).
		 */
		if (!M_READONLY(m)) {
			mprev = m;
			continue;
		}

		/*
		 * Not writable, replace with a copy or coalesce with
		 * the previous mbuf if possible (since we have to copy
		 * it anyway, we try to reduce the number of mbufs and
		 * clusters so that future work is easier).
		 */
		FBSDKASSERT(m->m_flags & M_EXT, ("m_flags 0x%x", m->m_flags));
		/* NB: we only coalesce into a cluster or larger */
		if (mprev != NULL && (mprev->m_flags & M_EXT) &&
		    m->m_len <= M_TRAILINGSPACE(mprev)) {
			/* XXX: this ignores mbuf types */
			memcpy(mtod(mprev, __uint8_t *) + mprev->m_len,
			    mtod(m, __uint8_t *), m->m_len);
			mprev->m_len += m->m_len;
			mprev->m_next = m->m_next;	/* unlink from chain */
			m_free(m);			/* reclaim mbuf */
			continue;
		}

		/*
		 * Allocate new space to hold the copy and copy the data.
		 * We deal with jumbo mbufs (i.e. m_len > MCLBYTES) by
		 * splitting them into clusters.  We could just malloc a
		 * buffer and make it external but too many device drivers
		 * don't know how to break up the non-contiguous memory when
		 * doing DMA.
		 */
		n = m_getcl(how, m->m_type, m->m_flags & M_COPYFLAGS);
		if (n == NULL) {
			m_freem(m0);
			return (NULL);
		}
		if (m->m_flags & M_PKTHDR) {
			FBSDKASSERT(mprev == NULL, ("%s: m0 %p, m %p has M_PKTHDR",
			    __func__, m0, m));
			m_move_pkthdr(n, m);
		}
		len = m->m_len;
		off = 0;
		mfirst = n;
		mlast = NULL;
		for (;;) {
			int cc = min(len, MCLBYTES);
			memcpy(mtod(n, __uint8_t *), mtod(m, __uint8_t *) + off, cc);
			n->m_len = cc;
			if (mlast != NULL)
				mlast->m_next = n;
			mlast = n;
#if 0
			newipsecstat.ips_clcopied++;
#endif

			len -= cc;
			if (len <= 0)
				break;
			off += cc;

			n = m_getcl(how, m->m_type, m->m_flags & M_COPYFLAGS);
			if (n == NULL) {
				m_freem(mfirst);
				m_freem(m0);
				return (NULL);
			}
		}
		n->m_next = m->m_next;
		if (mprev == NULL)
			m0 = mfirst;		/* new head of chain */
		else
			mprev->m_next = mfirst;	/* replace old mbuf */
		m_free(m);			/* release old mbuf */
		mprev = mfirst;
	}
	return (m0);
}
