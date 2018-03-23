/*	$NetBSD: route.c,v 1.207 2018/03/23 04:09:41 ozaki-r Exp $	*/

/*-
 * Copyright (c) 1998, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Kevin M. Lahey of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1980, 1986, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)route.c	8.3 (Berkeley) 1/9/95
 */

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_route.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: route.c,v 1.207 2018/03/23 04:09:41 ozaki-r Exp $");

#include <sys/param.h>
#ifdef RTFLUSH_DEBUG
#include <sys/sysctl.h>
#endif
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>
#include <sys/pool.h>
#include <sys/kauth.h>
#include <sys/workqueue.h>
#include <sys/syslog.h>
#include <sys/rwlock.h>
#include <sys/mutex.h>
#include <sys/cpu.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#if defined(INET) || defined(INET6)
#include <net/if_llatbl.h>
#endif

#include <netinet/in.h>
#include <netinet/in_var.h>

#ifdef RTFLUSH_DEBUG
#define	rtcache_debug() __predict_false(_rtcache_debug)
#else /* RTFLUSH_DEBUG */
#define	rtcache_debug() 0
#endif /* RTFLUSH_DEBUG */

#ifdef RT_DEBUG
#define RT_REFCNT_TRACE(rt)	printf("%s:%d: rt=%p refcnt=%d\n", \
				    __func__, __LINE__, (rt), (rt)->rt_refcnt)
#else
#define RT_REFCNT_TRACE(rt)	do {} while (0)
#endif

#ifdef RT_DEBUG
#define dlog(level, fmt, args...)	log(level, fmt, ##args)
#else
#define dlog(level, fmt, args...)	do {} while (0)
#endif

struct rtstat		rtstat;

static int		rttrash;	/* routes not in table but not freed */

static struct pool	rtentry_pool;
static struct pool	rttimer_pool;

static struct callout	rt_timer_ch; /* callout for rt_timer_timer() */
static struct workqueue	*rt_timer_wq;
static struct work	rt_timer_wk;

static void	rt_timer_init(void);
static void	rt_timer_queue_remove_all(struct rttimer_queue *);
static void	rt_timer_remove_all(struct rtentry *);
static void	rt_timer_timer(void *);

/*
 * Locking notes:
 * - The routing table is protected by a global rwlock
 *   - API: RT_RLOCK and friends
 * - rtcaches are NOT protected by the framework
 *   - Callers must guarantee a rtcache isn't accessed simultaneously
 *   - How the constraint is guranteed in the wild
 *     - Protect a rtcache by a mutex (e.g., inp_route)
 *     - Make rtcache per-CPU and allow only accesses from softint
 *       (e.g., ipforward_rt_percpu)
 * - References to a rtentry is managed by reference counting and psref
 *   - Reference couting is used for temporal reference when a rtentry
 *     is fetched from the routing table
 *   - psref is used for temporal reference when a rtentry is fetched
 *     from a rtcache
 *     - struct route (rtcache) has struct psref, so we cannot obtain
 *       a reference twice on the same struct route
 *   - Befere destroying or updating a rtentry, we have to wait for
 *     all references left (see below for details)
 *   - APIs
 *     - An obtained rtentry via rtalloc1 or rtrequest* must be
 *       unreferenced by rt_unref
 *     - An obtained rtentry via rtcache_* must be unreferenced by
 *       rtcache_unref
 *   - TODO: once we get a lockless routing table, we should use only
 *           psref for rtentries
 * - rtentry destruction
 *   - A rtentry is destroyed (freed) only when we call rtrequest(RTM_DELETE)
 *   - If a caller of rtrequest grabs a reference of a rtentry, the caller
 *     has a responsibility to destroy the rtentry by itself by calling
 *     rt_free
 *     - If not, rtrequest itself does that
 *   - If rt_free is called in softint, the actual destruction routine is
 *     deferred to a workqueue
 * - rtentry update
 *   - When updating a rtentry, RTF_UPDATING flag is set
 *   - If a rtentry is set RTF_UPDATING, fetching the rtentry from
 *     the routing table or a rtcache results in either of the following
 *     cases:
 *     - if the caller runs in softint, the caller fails to fetch
 *     - otherwise, the caller waits for the update completed and retries
 *       to fetch (probably succeed to fetch for the second time)
 * - rtcache invalidation
 *   - There is a global generation counter that is incremented when
 *     any routes have been added or deleted
 *   - When a rtcache caches a rtentry into itself, it also stores
 *     a snapshot of the generation counter
 *   - If the snapshot equals to the global counter, the cache is valid,
 *     otherwise the cache is invalidated
 */

/*
 * Global lock for the routing table.
 */
static krwlock_t		rt_lock __cacheline_aligned;
#ifdef NET_MPSAFE
#define RT_RLOCK()		rw_enter(&rt_lock, RW_READER)
#define RT_WLOCK()		rw_enter(&rt_lock, RW_WRITER)
#define RT_UNLOCK()		rw_exit(&rt_lock)
#define RT_LOCKED()		rw_lock_held(&rt_lock)
#define	RT_ASSERT_WLOCK()	KASSERT(rw_write_held(&rt_lock))
#else
#define RT_RLOCK()		do {} while (0)
#define RT_WLOCK()		do {} while (0)
#define RT_UNLOCK()		do {} while (0)
#define RT_LOCKED()		false
#define	RT_ASSERT_WLOCK()	do {} while (0)
#endif

static uint64_t rtcache_generation;

/*
 * mutex and cv that are used to wait for references to a rtentry left
 * before updating the rtentry.
 */
static struct {
	kmutex_t		lock;
	kcondvar_t		cv;
	bool			ongoing;
	const struct lwp	*lwp;
} rt_update_global __cacheline_aligned;

/*
 * A workqueue and stuff that are used to defer the destruction routine
 * of rtentries.
 */
static struct {
	struct workqueue	*wq;
	struct work		wk;
	kmutex_t		lock;
	SLIST_HEAD(, rtentry)	queue;
	bool			enqueued;
} rt_free_global __cacheline_aligned;

/* psref for rtentry */
static struct psref_class *rt_psref_class __read_mostly;

#ifdef RTFLUSH_DEBUG
static int _rtcache_debug = 0;
#endif /* RTFLUSH_DEBUG */

static kauth_listener_t route_listener;

static int rtdeletemsg(struct rtentry *);

static void rt_maskedcopy(const struct sockaddr *,
    struct sockaddr *, const struct sockaddr *);

static void rtcache_invalidate(void);

static void rt_ref(struct rtentry *);

static struct rtentry *
    rtalloc1_locked(const struct sockaddr *, int, bool, bool);

static void rtcache_ref(struct rtentry *, struct route *);

#ifdef NET_MPSAFE
static void rt_update_wait(void);
#endif

static bool rt_wait_ok(void);
static void rt_wait_refcnt(const char *, struct rtentry *, int);
static void rt_wait_psref(struct rtentry *);

#ifdef DDB
static void db_print_sa(const struct sockaddr *);
static void db_print_ifa(struct ifaddr *);
static int db_show_rtentry(struct rtentry *, void *);
#endif

#ifdef RTFLUSH_DEBUG
static void sysctl_net_rtcache_setup(struct sysctllog **);
static void
sysctl_net_rtcache_setup(struct sysctllog **clog)
{
	const struct sysctlnode *rnode;

	if (sysctl_createv(clog, 0, NULL, &rnode, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE,
	    "rtcache", SYSCTL_DESCR("Route cache related settings"),
	    NULL, 0, NULL, 0, CTL_NET, CTL_CREATE, CTL_EOL) != 0)
		return;
	if (sysctl_createv(clog, 0, &rnode, &rnode,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE, CTLTYPE_INT,
	    "debug", SYSCTL_DESCR("Debug route caches"),
	    NULL, 0, &_rtcache_debug, 0, CTL_CREATE, CTL_EOL) != 0)
		return;
}
#endif /* RTFLUSH_DEBUG */

static inline void
rt_destroy(struct rtentry *rt)
{
	if (rt->_rt_key != NULL)
		sockaddr_free(rt->_rt_key);
	if (rt->rt_gateway != NULL)
		sockaddr_free(rt->rt_gateway);
	if (rt_gettag(rt) != NULL)
		sockaddr_free(rt_gettag(rt));
	rt->_rt_key = rt->rt_gateway = rt->rt_tag = NULL;
}

static inline const struct sockaddr *
rt_setkey(struct rtentry *rt, const struct sockaddr *key, int flags)
{
	if (rt->_rt_key == key)
		goto out;

	if (rt->_rt_key != NULL)
		sockaddr_free(rt->_rt_key);
	rt->_rt_key = sockaddr_dup(key, flags);
out:
	rt->rt_nodes->rn_key = (const char *)rt->_rt_key;
	return rt->_rt_key;
}

struct ifaddr *
rt_get_ifa(struct rtentry *rt)
{
	struct ifaddr *ifa;

	if ((ifa = rt->rt_ifa) == NULL)
		return ifa;
	else if (ifa->ifa_getifa == NULL)
		return ifa;
#if 0
	else if (ifa->ifa_seqno != NULL && *ifa->ifa_seqno == rt->rt_ifa_seqno)
		return ifa;
#endif
	else {
		ifa = (*ifa->ifa_getifa)(ifa, rt_getkey(rt));
		if (ifa == NULL)
			return NULL;
		rt_replace_ifa(rt, ifa);
		return ifa;
	}
}

static void
rt_set_ifa1(struct rtentry *rt, struct ifaddr *ifa)
{
	rt->rt_ifa = ifa;
	if (ifa->ifa_seqno != NULL)
		rt->rt_ifa_seqno = *ifa->ifa_seqno;
}

/*
 * Is this route the connected route for the ifa?
 */
static int
rt_ifa_connected(const struct rtentry *rt, const struct ifaddr *ifa)
{
	const struct sockaddr *key, *dst, *odst;
	struct sockaddr_storage maskeddst;

	key = rt_getkey(rt);
	dst = rt->rt_flags & RTF_HOST ? ifa->ifa_dstaddr : ifa->ifa_addr;
	if (dst == NULL ||
	    dst->sa_family != key->sa_family ||
	    dst->sa_len != key->sa_len)
		return 0;
	if ((rt->rt_flags & RTF_HOST) == 0 && ifa->ifa_netmask) {
		odst = dst;
		dst = (struct sockaddr *)&maskeddst;
		rt_maskedcopy(odst, (struct sockaddr *)&maskeddst,
		    ifa->ifa_netmask);
	}
	return (memcmp(dst, key, dst->sa_len) == 0);
}

void
rt_replace_ifa(struct rtentry *rt, struct ifaddr *ifa)
{
	if (rt->rt_ifa &&
	    rt->rt_ifa != ifa &&
	    rt->rt_ifa->ifa_flags & IFA_ROUTE &&
	    rt_ifa_connected(rt, rt->rt_ifa))
	{
		RT_DPRINTF("rt->_rt_key = %p, ifa = %p, "
		    "replace deleted IFA_ROUTE\n",
		    (void *)rt->_rt_key, (void *)rt->rt_ifa);
		rt->rt_ifa->ifa_flags &= ~IFA_ROUTE;
		if (rt_ifa_connected(rt, ifa)) {
			RT_DPRINTF("rt->_rt_key = %p, ifa = %p, "
			    "replace added IFA_ROUTE\n",
			    (void *)rt->_rt_key, (void *)ifa);
			ifa->ifa_flags |= IFA_ROUTE;
		}
	}

	ifaref(ifa);
	ifafree(rt->rt_ifa);
	rt_set_ifa1(rt, ifa);
}

static void
rt_set_ifa(struct rtentry *rt, struct ifaddr *ifa)
{
	ifaref(ifa);
	rt_set_ifa1(rt, ifa);
}

static int
route_listener_cb(kauth_cred_t cred, kauth_action_t action, void *cookie,
    void *arg0, void *arg1, void *arg2, void *arg3)
{
	struct rt_msghdr *rtm;
	int result;

	result = KAUTH_RESULT_DEFER;
	rtm = arg1;

	if (action != KAUTH_NETWORK_ROUTE)
		return result;

	if (rtm->rtm_type == RTM_GET)
		result = KAUTH_RESULT_ALLOW;

	return result;
}

static void rt_free_work(struct work *, void *);

void
rt_init(void)
{
	int error;

#ifdef RTFLUSH_DEBUG
	sysctl_net_rtcache_setup(NULL);
#endif

	mutex_init(&rt_free_global.lock, MUTEX_DEFAULT, IPL_SOFTNET);
	SLIST_INIT(&rt_free_global.queue);
	rt_free_global.enqueued = false;

	rt_psref_class = psref_class_create("rtentry", IPL_SOFTNET);

	error = workqueue_create(&rt_free_global.wq, "rt_free",
	    rt_free_work, NULL, PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (error)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);

	mutex_init(&rt_update_global.lock, MUTEX_DEFAULT, IPL_SOFTNET);
	cv_init(&rt_update_global.cv, "rt_update");

	pool_init(&rtentry_pool, sizeof(struct rtentry), 0, 0, 0, "rtentpl",
	    NULL, IPL_SOFTNET);
	pool_init(&rttimer_pool, sizeof(struct rttimer), 0, 0, 0, "rttmrpl",
	    NULL, IPL_SOFTNET);

	rn_init();	/* initialize all zeroes, all ones, mask table */
	rtbl_init();

	route_listener = kauth_listen_scope(KAUTH_SCOPE_NETWORK,
	    route_listener_cb, NULL);
}

static void
rtcache_invalidate(void)
{

	RT_ASSERT_WLOCK();

	if (rtcache_debug())
		printf("%s: enter\n", __func__);

	rtcache_generation++;
}

#ifdef RT_DEBUG
static void
dump_rt(const struct rtentry *rt)
{
	char buf[512];

	aprint_normal("rt: ");
	aprint_normal("p=%p ", rt);
	if (rt->_rt_key == NULL) {
		aprint_normal("dst=(NULL) ");
	} else {
		sockaddr_format(rt->_rt_key, buf, sizeof(buf));
		aprint_normal("dst=%s ", buf);
	}
	if (rt->rt_gateway == NULL) {
		aprint_normal("gw=(NULL) ");
	} else {
		sockaddr_format(rt->_rt_key, buf, sizeof(buf));
		aprint_normal("gw=%s ", buf);
	}
	aprint_normal("flags=%x ", rt->rt_flags);
	if (rt->rt_ifp == NULL) {
		aprint_normal("if=(NULL) ");
	} else {
		aprint_normal("if=%s ", rt->rt_ifp->if_xname);
	}
	aprint_normal("\n");
}
#endif /* RT_DEBUG */

/*
 * Packet routing routines. If success, refcnt of a returned rtentry
 * will be incremented. The caller has to rtfree it by itself.
 */
struct rtentry *
rtalloc1_locked(const struct sockaddr *dst, int report, bool wait_ok,
    bool wlock)
{
	rtbl_t *rtbl;
	struct rtentry *rt;
	int s;

#ifdef NET_MPSAFE
retry:
#endif
	s = splsoftnet();
	rtbl = rt_gettable(dst->sa_family);
	if (rtbl == NULL)
		goto miss;

	rt = rt_matchaddr(rtbl, dst);
	if (rt == NULL)
		goto miss;

	if (!ISSET(rt->rt_flags, RTF_UP))
		goto miss;

#ifdef NET_MPSAFE
	if (ISSET(rt->rt_flags, RTF_UPDATING) &&
	    /* XXX updater should be always able to acquire */
	    curlwp != rt_update_global.lwp) {
		if (!wait_ok || !rt_wait_ok())
			goto miss;
		RT_UNLOCK();
		splx(s);

		/* We can wait until the update is complete */
		rt_update_wait();

		if (wlock)
			RT_WLOCK();
		else
			RT_RLOCK();
		goto retry;
	}
#endif /* NET_MPSAFE */

	rt_ref(rt);
	RT_REFCNT_TRACE(rt);

	splx(s);
	return rt;
miss:
	rtstat.rts_unreach++;
	if (report) {
		struct rt_addrinfo info;

		memset(&info, 0, sizeof(info));
		info.rti_info[RTAX_DST] = dst;
		rt_missmsg(RTM_MISS, &info, 0, 0);
	}
	splx(s);
	return NULL;
}

struct rtentry *
rtalloc1(const struct sockaddr *dst, int report)
{
	struct rtentry *rt;

	RT_RLOCK();
	rt = rtalloc1_locked(dst, report, true, false);
	RT_UNLOCK();

	return rt;
}

static void
rt_ref(struct rtentry *rt)
{

	KASSERT(rt->rt_refcnt >= 0);
	atomic_inc_uint(&rt->rt_refcnt);
}

void
rt_unref(struct rtentry *rt)
{

	KASSERT(rt != NULL);
	KASSERTMSG(rt->rt_refcnt > 0, "refcnt=%d", rt->rt_refcnt);

	atomic_dec_uint(&rt->rt_refcnt);
	if (!ISSET(rt->rt_flags, RTF_UP) || ISSET(rt->rt_flags, RTF_UPDATING)) {
		mutex_enter(&rt_free_global.lock);
		cv_broadcast(&rt->rt_cv);
		mutex_exit(&rt_free_global.lock);
	}
}

static bool
rt_wait_ok(void)
{

	KASSERT(!cpu_intr_p());
	return !cpu_softintr_p();
}

void
rt_wait_refcnt(const char *title, struct rtentry *rt, int cnt)
{
	mutex_enter(&rt_free_global.lock);
	while (rt->rt_refcnt > cnt) {
		dlog(LOG_DEBUG, "%s: %s waiting (refcnt=%d)\n",
		    __func__, title, rt->rt_refcnt);
		cv_wait(&rt->rt_cv, &rt_free_global.lock);
		dlog(LOG_DEBUG, "%s: %s waited (refcnt=%d)\n",
		    __func__, title, rt->rt_refcnt);
	}
	mutex_exit(&rt_free_global.lock);
}

void
rt_wait_psref(struct rtentry *rt)
{

	psref_target_destroy(&rt->rt_psref, rt_psref_class);
	psref_target_init(&rt->rt_psref, rt_psref_class);
}

static void
_rt_free(struct rtentry *rt)
{
	struct ifaddr *ifa;

	/*
	 * Need to avoid a deadlock on rt_wait_refcnt of update
	 * and a conflict on psref_target_destroy of update.
	 */
#ifdef NET_MPSAFE
	rt_update_wait();
#endif

	RT_REFCNT_TRACE(rt);
	KASSERTMSG(rt->rt_refcnt >= 0, "refcnt=%d", rt->rt_refcnt);
	rt_wait_refcnt("free", rt, 0);
#ifdef NET_MPSAFE
	psref_target_destroy(&rt->rt_psref, rt_psref_class);
#endif

	rt_assert_inactive(rt);
	rttrash--;
	ifa = rt->rt_ifa;
	rt->rt_ifa = NULL;
	ifafree(ifa);
	rt->rt_ifp = NULL;
	cv_destroy(&rt->rt_cv);
	rt_destroy(rt);
	pool_put(&rtentry_pool, rt);
}

static void
rt_free_work(struct work *wk, void *arg)
{

	for (;;) {
		struct rtentry *rt;

		mutex_enter(&rt_free_global.lock);
		rt_free_global.enqueued = false;
		if ((rt = SLIST_FIRST(&rt_free_global.queue)) == NULL) {
			mutex_exit(&rt_free_global.lock);
			return;
		}
		SLIST_REMOVE_HEAD(&rt_free_global.queue, rt_free);
		mutex_exit(&rt_free_global.lock);
		atomic_dec_uint(&rt->rt_refcnt);
		_rt_free(rt);
	}
}

void
rt_free(struct rtentry *rt)
{

	KASSERT(rt->rt_refcnt > 0);
	if (rt_wait_ok()) {
		atomic_dec_uint(&rt->rt_refcnt);
		_rt_free(rt);
		return;
	}

	mutex_enter(&rt_free_global.lock);
	rt_ref(rt);
	SLIST_INSERT_HEAD(&rt_free_global.queue, rt, rt_free);
	if (!rt_free_global.enqueued) {
		workqueue_enqueue(rt_free_global.wq, &rt_free_global.wk, NULL);
		rt_free_global.enqueued = true;
	}
	mutex_exit(&rt_free_global.lock);
}

#ifdef NET_MPSAFE
static void
rt_update_wait(void)
{

	mutex_enter(&rt_update_global.lock);
	while (rt_update_global.ongoing) {
		dlog(LOG_DEBUG, "%s: waiting lwp=%p\n", __func__, curlwp);
		cv_wait(&rt_update_global.cv, &rt_update_global.lock);
		dlog(LOG_DEBUG, "%s: waited lwp=%p\n", __func__, curlwp);
	}
	mutex_exit(&rt_update_global.lock);
}
#endif

int
rt_update_prepare(struct rtentry *rt)
{

	dlog(LOG_DEBUG, "%s: updating rt=%p lwp=%p\n", __func__, rt, curlwp);

	RT_WLOCK();
	/* If the entry is being destroyed, don't proceed the update. */
	if (!ISSET(rt->rt_flags, RTF_UP)) {
		RT_UNLOCK();
		return ESRCH;
	}
	rt->rt_flags |= RTF_UPDATING;
	RT_UNLOCK();

	mutex_enter(&rt_update_global.lock);
	while (rt_update_global.ongoing) {
		dlog(LOG_DEBUG, "%s: waiting ongoing updating rt=%p lwp=%p\n",
		    __func__, rt, curlwp);
		cv_wait(&rt_update_global.cv, &rt_update_global.lock);
		dlog(LOG_DEBUG, "%s: waited ongoing updating rt=%p lwp=%p\n",
		    __func__, rt, curlwp);
	}
	rt_update_global.ongoing = true;
	/* XXX need it to avoid rt_update_wait by updater itself. */
	rt_update_global.lwp = curlwp;
	mutex_exit(&rt_update_global.lock);

	rt_wait_refcnt("update", rt, 1);
	rt_wait_psref(rt);

	return 0;
}

void
rt_update_finish(struct rtentry *rt)
{

	RT_WLOCK();
	rt->rt_flags &= ~RTF_UPDATING;
	RT_UNLOCK();

	mutex_enter(&rt_update_global.lock);
	rt_update_global.ongoing = false;
	rt_update_global.lwp = NULL;
	cv_broadcast(&rt_update_global.cv);
	mutex_exit(&rt_update_global.lock);

	dlog(LOG_DEBUG, "%s: updated rt=%p lwp=%p\n", __func__, rt, curlwp);
}

/*
 * Force a routing table entry to the specified
 * destination to go through the given gateway.
 * Normally called as a result of a routing redirect
 * message from the network layer.
 *
 * N.B.: must be called at splsoftnet
 */
void
rtredirect(const struct sockaddr *dst, const struct sockaddr *gateway,
	const struct sockaddr *netmask, int flags, const struct sockaddr *src,
	struct rtentry **rtp)
{
	struct rtentry *rt;
	int error = 0;
	uint64_t *stat = NULL;
	struct rt_addrinfo info;
	struct ifaddr *ifa;
	struct psref psref;

	/* verify the gateway is directly reachable */
	if ((ifa = ifa_ifwithnet_psref(gateway, &psref)) == NULL) {
		error = ENETUNREACH;
		goto out;
	}
	rt = rtalloc1(dst, 0);
	/*
	 * If the redirect isn't from our current router for this dst,
	 * it's either old or wrong.  If it redirects us to ourselves,
	 * we have a routing loop, perhaps as a result of an interface
	 * going down recently.
	 */
	if (!(flags & RTF_DONE) && rt &&
	     (sockaddr_cmp(src, rt->rt_gateway) != 0 || rt->rt_ifa != ifa))
		error = EINVAL;
	else {
		int s = pserialize_read_enter();
		struct ifaddr *_ifa;

		_ifa = ifa_ifwithaddr(gateway);
		if (_ifa != NULL)
			error = EHOSTUNREACH;
		pserialize_read_exit(s);
	}
	if (error)
		goto done;
	/*
	 * Create a new entry if we just got back a wildcard entry
	 * or the lookup failed.  This is necessary for hosts
	 * which use routing redirects generated by smart gateways
	 * to dynamically build the routing tables.
	 */
	if (rt == NULL || (rt_mask(rt) && rt_mask(rt)->sa_len < 2))
		goto create;
	/*
	 * Don't listen to the redirect if it's
	 * for a route to an interface.
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (((rt->rt_flags & RTF_HOST) == 0) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to net.
			 */
		create:
			if (rt != NULL)
				rt_unref(rt);
			flags |=  RTF_GATEWAY | RTF_DYNAMIC;
			memset(&info, 0, sizeof(info));
			info.rti_info[RTAX_DST] = dst;
			info.rti_info[RTAX_GATEWAY] = gateway;
			info.rti_info[RTAX_NETMASK] = netmask;
			info.rti_ifa = ifa;
			info.rti_flags = flags;
			rt = NULL;
			error = rtrequest1(RTM_ADD, &info, &rt);
			if (rt != NULL)
				flags = rt->rt_flags;
			stat = &rtstat.rts_dynamic;
		} else {
			/*
			 * Smash the current notion of the gateway to
			 * this destination.  Should check about netmask!!!
			 */
#ifdef NET_MPSAFE
			KASSERT(!cpu_softintr_p());

			error = rt_update_prepare(rt);
			if (error == 0) {
#endif
				error = rt_setgate(rt, gateway);
				if (error == 0) {
					rt->rt_flags |= RTF_MODIFIED;
					flags |= RTF_MODIFIED;
				}
#ifdef NET_MPSAFE
				rt_update_finish(rt);
			} else {
				/*
				 * If error != 0, the rtentry is being
				 * destroyed, so doing nothing doesn't
				 * matter.
				 */
			}
#endif
			stat = &rtstat.rts_newgateway;
		}
	} else
		error = EHOSTUNREACH;
done:
	if (rt) {
		if (rtp != NULL && !error)
			*rtp = rt;
		else
			rt_unref(rt);
	}
out:
	if (error)
		rtstat.rts_badredirect++;
	else if (stat != NULL)
		(*stat)++;
	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	info.rti_info[RTAX_AUTHOR] = src;
	rt_missmsg(RTM_REDIRECT, &info, flags, error);
	ifa_release(ifa, &psref);
}

/*
 * Delete a route and generate a message.
 * It doesn't free a passed rt.
 */
static int
rtdeletemsg(struct rtentry *rt)
{
	int error;
	struct rt_addrinfo info;
	struct rtentry *retrt;

	/*
	 * Request the new route so that the entry is not actually
	 * deleted.  That will allow the information being reported to
	 * be accurate (and consistent with route_output()).
	 */
	memset(&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rt_getkey(rt);
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_flags = rt->rt_flags;
	error = rtrequest1(RTM_DELETE, &info, &retrt);

	rt_missmsg(RTM_DELETE, &info, info.rti_flags, error);

	return error;
}

struct ifaddr *
ifa_ifwithroute_psref(int flags, const struct sockaddr *dst,
	const struct sockaddr *gateway, struct psref *psref)
{
	struct ifaddr *ifa = NULL;

	if ((flags & RTF_GATEWAY) == 0) {
		/*
		 * If we are adding a route to an interface,
		 * and the interface is a pt to pt link
		 * we should search for the destination
		 * as our clue to the interface.  Otherwise
		 * we can use the local address.
		 */
		if ((flags & RTF_HOST) && gateway->sa_family != AF_LINK)
			ifa = ifa_ifwithdstaddr_psref(dst, psref);
		if (ifa == NULL)
			ifa = ifa_ifwithaddr_psref(gateway, psref);
	} else {
		/*
		 * If we are adding a route to a remote net
		 * or host, the gateway may still be on the
		 * other end of a pt to pt link.
		 */
		ifa = ifa_ifwithdstaddr_psref(gateway, psref);
	}
	if (ifa == NULL)
		ifa = ifa_ifwithnet_psref(gateway, psref);
	if (ifa == NULL) {
		int s;
		struct rtentry *rt;

		/* XXX we cannot call rtalloc1 if holding the rt lock */
		if (RT_LOCKED())
			rt = rtalloc1_locked(gateway, 0, true, true);
		else
			rt = rtalloc1(gateway, 0);
		if (rt == NULL)
			return NULL;
		if (rt->rt_flags & RTF_GATEWAY) {
			rt_unref(rt);
			return NULL;
		}
		/*
		 * Just in case. May not need to do this workaround.
		 * Revisit when working on rtentry MP-ification.
		 */
		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, rt->rt_ifp) {
			if (ifa == rt->rt_ifa)
				break;
		}
		if (ifa != NULL)
			ifa_acquire(ifa, psref);
		pserialize_read_exit(s);
		rt_unref(rt);
		if (ifa == NULL)
			return NULL;
	}
	if (ifa->ifa_addr->sa_family != dst->sa_family) {
		struct ifaddr *nifa;
		int s;

		s = pserialize_read_enter();
		nifa = ifaof_ifpforaddr(dst, ifa->ifa_ifp);
		if (nifa != NULL) {
			ifa_release(ifa, psref);
			ifa_acquire(nifa, psref);
			ifa = nifa;
		}
		pserialize_read_exit(s);
	}
	return ifa;
}

/*
 * If it suceeds and ret_nrt isn't NULL, refcnt of ret_nrt is incremented.
 * The caller has to rtfree it by itself.
 */
int
rtrequest(int req, const struct sockaddr *dst, const struct sockaddr *gateway,
	const struct sockaddr *netmask, int flags, struct rtentry **ret_nrt)
{
	struct rt_addrinfo info;

	memset(&info, 0, sizeof(info));
	info.rti_flags = flags;
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = gateway;
	info.rti_info[RTAX_NETMASK] = netmask;
	return rtrequest1(req, &info, ret_nrt);
}

/*
 * It's a utility function to add/remove a route to/from the routing table
 * and tell user processes the addition/removal on success.
 */
int
rtrequest_newmsg(const int req, const struct sockaddr *dst,
	const struct sockaddr *gateway, const struct sockaddr *netmask,
	const int flags)
{
	int error;
	struct rtentry *ret_nrt = NULL;

	KASSERT(req == RTM_ADD || req == RTM_DELETE);

	error = rtrequest(req, dst, gateway, netmask, flags, &ret_nrt);
	if (error != 0)
		return error;

	KASSERT(ret_nrt != NULL);

	rt_newmsg(req, ret_nrt); /* tell user process */
	if (req == RTM_DELETE)
		rt_free(ret_nrt);
	else
		rt_unref(ret_nrt);

	return 0;
}

struct ifnet *
rt_getifp(struct rt_addrinfo *info, struct psref *psref)
{
	const struct sockaddr *ifpaddr = info->rti_info[RTAX_IFP];

	if (info->rti_ifp != NULL)
		return NULL;
	/*
	 * ifp may be specified by sockaddr_dl when protocol address
	 * is ambiguous
	 */
	if (ifpaddr != NULL && ifpaddr->sa_family == AF_LINK) {
		struct ifaddr *ifa;
		int s = pserialize_read_enter();

		ifa = ifa_ifwithnet(ifpaddr);
		if (ifa != NULL)
			info->rti_ifp = if_get_byindex(ifa->ifa_ifp->if_index,
			    psref);
		pserialize_read_exit(s);
	}

	return info->rti_ifp;
}

struct ifaddr *
rt_getifa(struct rt_addrinfo *info, struct psref *psref)
{
	struct ifaddr *ifa = NULL;
	const struct sockaddr *dst = info->rti_info[RTAX_DST];
	const struct sockaddr *gateway = info->rti_info[RTAX_GATEWAY];
	const struct sockaddr *ifaaddr = info->rti_info[RTAX_IFA];
	int flags = info->rti_flags;
	const struct sockaddr *sa;

	if (info->rti_ifa == NULL && ifaaddr != NULL) {
		ifa = ifa_ifwithaddr_psref(ifaaddr, psref);
		if (ifa != NULL)
			goto got;
	}

	sa = ifaaddr != NULL ? ifaaddr :
	    (gateway != NULL ? gateway : dst);
	if (sa != NULL && info->rti_ifp != NULL)
		ifa = ifaof_ifpforaddr_psref(sa, info->rti_ifp, psref);
	else if (dst != NULL && gateway != NULL)
		ifa = ifa_ifwithroute_psref(flags, dst, gateway, psref);
	else if (sa != NULL)
		ifa = ifa_ifwithroute_psref(flags, sa, sa, psref);
	if (ifa == NULL)
		return NULL;
got:
	if (ifa->ifa_getifa != NULL) {
		/* FIXME ifa_getifa is NOMPSAFE */
		ifa = (*ifa->ifa_getifa)(ifa, dst);
		if (ifa == NULL)
			return NULL;
		ifa_acquire(ifa, psref);
	}
	info->rti_ifa = ifa;
	if (info->rti_ifp == NULL)
		info->rti_ifp = ifa->ifa_ifp;
	return ifa;
}

/*
 * If it suceeds and ret_nrt isn't NULL, refcnt of ret_nrt is incremented.
 * The caller has to rtfree it by itself.
 */
int
rtrequest1(int req, struct rt_addrinfo *info, struct rtentry **ret_nrt)
{
	int s = splsoftnet(), ss;
	int error = 0, rc;
	struct rtentry *rt;
	rtbl_t *rtbl;
	struct ifaddr *ifa = NULL;
	struct sockaddr_storage maskeddst;
	const struct sockaddr *dst = info->rti_info[RTAX_DST];
	const struct sockaddr *gateway = info->rti_info[RTAX_GATEWAY];
	const struct sockaddr *netmask = info->rti_info[RTAX_NETMASK];
	int flags = info->rti_flags;
	struct psref psref_ifp, psref_ifa;
	int bound = 0;
	struct ifnet *ifp = NULL;
	bool need_to_release_ifa = true;
	bool need_unlock = true;
#define senderr(x) { error = x ; goto bad; }

	RT_WLOCK();

	bound = curlwp_bind();
	if ((rtbl = rt_gettable(dst->sa_family)) == NULL)
		senderr(ESRCH);
	if (flags & RTF_HOST)
		netmask = NULL;
	switch (req) {
	case RTM_DELETE:
		if (netmask) {
			rt_maskedcopy(dst, (struct sockaddr *)&maskeddst,
			    netmask);
			dst = (struct sockaddr *)&maskeddst;
		}
		if ((rt = rt_lookup(rtbl, dst, netmask)) == NULL)
			senderr(ESRCH);
		if ((rt = rt_deladdr(rtbl, dst, netmask)) == NULL)
			senderr(ESRCH);
		rt->rt_flags &= ~RTF_UP;
		if ((ifa = rt->rt_ifa)) {
			if (ifa->ifa_flags & IFA_ROUTE &&
			    rt_ifa_connected(rt, ifa)) {
				RT_DPRINTF("rt->_rt_key = %p, ifa = %p, "
				    "deleted IFA_ROUTE\n",
				    (void *)rt->_rt_key, (void *)ifa);
				ifa->ifa_flags &= ~IFA_ROUTE;
			}
			if (ifa->ifa_rtrequest)
				ifa->ifa_rtrequest(RTM_DELETE, rt, info);
			ifa = NULL;
		}
		rttrash++;
		if (ret_nrt) {
			*ret_nrt = rt;
			rt_ref(rt);
			RT_REFCNT_TRACE(rt);
		}
		rtcache_invalidate();
		RT_UNLOCK();
		need_unlock = false;
		rt_timer_remove_all(rt);
#if defined(INET) || defined(INET6)
		if (netmask != NULL)
			lltable_prefix_free(dst->sa_family, dst, netmask, 0);
#endif
		if (ret_nrt == NULL) {
			/* Adjust the refcount */
			rt_ref(rt);
			RT_REFCNT_TRACE(rt);
			rt_free(rt);
		}
		break;

	case RTM_ADD:
		if (info->rti_ifa == NULL) {
			ifp = rt_getifp(info, &psref_ifp);
			ifa = rt_getifa(info, &psref_ifa);
			if (ifa == NULL)
				senderr(ENETUNREACH);
		} else {
			/* Caller should have a reference of ifa */
			ifa = info->rti_ifa;
			need_to_release_ifa = false;
		}
		rt = pool_get(&rtentry_pool, PR_NOWAIT);
		if (rt == NULL)
			senderr(ENOBUFS);
		memset(rt, 0, sizeof(*rt));
		rt->rt_flags = RTF_UP | flags;
		LIST_INIT(&rt->rt_timer);

		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
		if (netmask) {
			rt_maskedcopy(dst, (struct sockaddr *)&maskeddst,
			    netmask);
			rt_setkey(rt, (struct sockaddr *)&maskeddst, M_NOWAIT);
		} else {
			rt_setkey(rt, dst, M_NOWAIT);
		}
		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
		if (rt_getkey(rt) == NULL ||
		    rt_setgate(rt, gateway) != 0) {
			pool_put(&rtentry_pool, rt);
			senderr(ENOBUFS);
		}

		rt_set_ifa(rt, ifa);
		if (info->rti_info[RTAX_TAG] != NULL) {
			const struct sockaddr *tag;
			tag = rt_settag(rt, info->rti_info[RTAX_TAG]);
			if (tag == NULL)
				senderr(ENOBUFS);
		}
		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);

		ss = pserialize_read_enter();
		if (info->rti_info[RTAX_IFP] != NULL) {
			struct ifaddr *ifa2;
			ifa2 = ifa_ifwithnet(info->rti_info[RTAX_IFP]);
			if (ifa2 != NULL)
				rt->rt_ifp = ifa2->ifa_ifp;
			else
				rt->rt_ifp = ifa->ifa_ifp;
		} else
			rt->rt_ifp = ifa->ifa_ifp;
		pserialize_read_exit(ss);
		cv_init(&rt->rt_cv, "rtentry");
		psref_target_init(&rt->rt_psref, rt_psref_class);

		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
		rc = rt_addaddr(rtbl, rt, netmask);
		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
		if (rc != 0) {
			ifafree(ifa); /* for rt_set_ifa above */
			cv_destroy(&rt->rt_cv);
			rt_destroy(rt);
			pool_put(&rtentry_pool, rt);
			senderr(rc);
		}
		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
		if (ifa->ifa_rtrequest)
			ifa->ifa_rtrequest(req, rt, info);
		if (need_to_release_ifa)
			ifa_release(ifa, &psref_ifa);
		ifa = NULL;
		if_put(ifp, &psref_ifp);
		ifp = NULL;
		RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
		if (ret_nrt) {
			*ret_nrt = rt;
			rt_ref(rt);
			RT_REFCNT_TRACE(rt);
		}
		rtcache_invalidate();
		RT_UNLOCK();
		need_unlock = false;
		break;
	case RTM_GET:
		if (netmask != NULL) {
			rt_maskedcopy(dst, (struct sockaddr *)&maskeddst,
			    netmask);
			dst = (struct sockaddr *)&maskeddst;
		}
		if ((rt = rt_lookup(rtbl, dst, netmask)) == NULL)
			senderr(ESRCH);
		if (ret_nrt != NULL) {
			*ret_nrt = rt;
			rt_ref(rt);
			RT_REFCNT_TRACE(rt);
		}
		break;
	}
bad:
	if (need_to_release_ifa)
		ifa_release(ifa, &psref_ifa);
	if_put(ifp, &psref_ifp);
	curlwp_bindx(bound);
	if (need_unlock)
		RT_UNLOCK();
	splx(s);
	return error;
}

int
rt_setgate(struct rtentry *rt, const struct sockaddr *gate)
{
	struct sockaddr *new, *old;

	KASSERT(rt->_rt_key != NULL);
	RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);

	new = sockaddr_dup(gate, M_ZERO | M_NOWAIT);
	if (new == NULL)
		return ENOMEM;

	old = rt->rt_gateway;
	rt->rt_gateway = new;
	if (old != NULL)
		sockaddr_free(old);

	KASSERT(rt->_rt_key != NULL);
	RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);

	if (rt->rt_flags & RTF_GATEWAY) {
		struct rtentry *gwrt;

		/* XXX we cannot call rtalloc1 if holding the rt lock */
		if (RT_LOCKED())
			gwrt = rtalloc1_locked(gate, 1, false, true);
		else
			gwrt = rtalloc1(gate, 1);
		/*
		 * If we switched gateways, grab the MTU from the new
		 * gateway route if the current MTU, if the current MTU is
		 * greater than the MTU of gateway.
		 * Note that, if the MTU of gateway is 0, we will reset the
		 * MTU of the route to run PMTUD again from scratch. XXX
		 */
		if (gwrt != NULL) {
			KASSERT(gwrt->_rt_key != NULL);
			RT_DPRINTF("gwrt->_rt_key = %p\n", gwrt->_rt_key);
			if ((rt->rt_rmx.rmx_locks & RTV_MTU) == 0 &&
			    rt->rt_rmx.rmx_mtu &&
			    rt->rt_rmx.rmx_mtu > gwrt->rt_rmx.rmx_mtu) {
				rt->rt_rmx.rmx_mtu = gwrt->rt_rmx.rmx_mtu;
			}
			rt_unref(gwrt);
		}
	}
	KASSERT(rt->_rt_key != NULL);
	RT_DPRINTF("rt->_rt_key = %p\n", (void *)rt->_rt_key);
	return 0;
}

static void
rt_maskedcopy(const struct sockaddr *src, struct sockaddr *dst,
	const struct sockaddr *netmask)
{
	const char *netmaskp = &netmask->sa_data[0],
	           *srcp = &src->sa_data[0];
	char *dstp = &dst->sa_data[0];
	const char *maskend = (char *)dst + MIN(netmask->sa_len, src->sa_len);
	const char *srcend = (char *)dst + src->sa_len;

	dst->sa_len = src->sa_len;
	dst->sa_family = src->sa_family;

	while (dstp < maskend)
		*dstp++ = *srcp++ & *netmaskp++;
	if (dstp < srcend)
		memset(dstp, 0, (size_t)(srcend - dstp));
}

/*
 * Inform the routing socket of a route change.
 */
void
rt_newmsg(const int cmd, const struct rtentry *rt)
{
	struct rt_addrinfo info;

	memset((void *)&info, 0, sizeof(info));
	info.rti_info[RTAX_DST] = rt_getkey(rt);
	info.rti_info[RTAX_GATEWAY] = rt->rt_gateway;
	info.rti_info[RTAX_NETMASK] = rt_mask(rt);
	if (rt->rt_ifp) {
		info.rti_info[RTAX_IFP] = rt->rt_ifp->if_dl->ifa_addr;
		info.rti_info[RTAX_IFA] = rt->rt_ifa->ifa_addr;
	}

	rt_missmsg(cmd, &info, rt->rt_flags, 0);
}

/*
 * Set up or tear down a routing table entry, normally
 * for an interface.
 */
int
rtinit(struct ifaddr *ifa, int cmd, int flags)
{
	struct rtentry *rt;
	struct sockaddr *dst, *odst;
	struct sockaddr_storage maskeddst;
	struct rtentry *nrt = NULL;
	int error;
	struct rt_addrinfo info;

	dst = flags & RTF_HOST ? ifa->ifa_dstaddr : ifa->ifa_addr;
	if (cmd == RTM_DELETE) {
		if ((flags & RTF_HOST) == 0 && ifa->ifa_netmask) {
			/* Delete subnet route for this interface */
			odst = dst;
			dst = (struct sockaddr *)&maskeddst;
			rt_maskedcopy(odst, dst, ifa->ifa_netmask);
		}
		if ((rt = rtalloc1(dst, 0)) != NULL) {
			if (rt->rt_ifa != ifa) {
				rt_unref(rt);
				return (flags & RTF_HOST) ? EHOSTUNREACH
							: ENETUNREACH;
			}
			rt_unref(rt);
		}
	}
	memset(&info, 0, sizeof(info));
	info.rti_ifa = ifa;
	info.rti_flags = flags | ifa->ifa_flags;
	info.rti_info[RTAX_DST] = dst;
	info.rti_info[RTAX_GATEWAY] = ifa->ifa_addr;

	/*
	 * XXX here, it seems that we are assuming that ifa_netmask is NULL
	 * for RTF_HOST.  bsdi4 passes NULL explicitly (via intermediate
	 * variable) when RTF_HOST is 1.  still not sure if i can safely
	 * change it to meet bsdi4 behavior.
	 */
	if (cmd != RTM_LLINFO_UPD)
		info.rti_info[RTAX_NETMASK] = ifa->ifa_netmask;
	error = rtrequest1((cmd == RTM_LLINFO_UPD) ? RTM_GET : cmd, &info,
	    &nrt);
	if (error != 0)
		return error;

	rt = nrt;
	RT_REFCNT_TRACE(rt);
	switch (cmd) {
	case RTM_DELETE:
		rt_newmsg(cmd, rt);
		rt_free(rt);
		break;
	case RTM_LLINFO_UPD:
		if (cmd == RTM_LLINFO_UPD && ifa->ifa_rtrequest != NULL)
			ifa->ifa_rtrequest(RTM_LLINFO_UPD, rt, &info);
		rt_newmsg(RTM_CHANGE, rt);
		rt_unref(rt);
		break;
	case RTM_ADD:
		/*
		 * XXX it looks just reverting rt_ifa replaced by ifa_rtrequest
		 * called via rtrequest1. Can we just prevent the replacement
		 * somehow and remove the following code? And also doesn't
		 * calling ifa_rtrequest(RTM_ADD) replace rt_ifa again?
		 */
		if (rt->rt_ifa != ifa) {
			printf("rtinit: wrong ifa (%p) was (%p)\n", ifa,
				rt->rt_ifa);
#ifdef NET_MPSAFE
			KASSERT(!cpu_softintr_p());

			error = rt_update_prepare(rt);
			if (error == 0) {
#endif
				if (rt->rt_ifa->ifa_rtrequest != NULL) {
					rt->rt_ifa->ifa_rtrequest(RTM_DELETE,
					    rt, &info);
				}
				rt_replace_ifa(rt, ifa);
				rt->rt_ifp = ifa->ifa_ifp;
				if (ifa->ifa_rtrequest != NULL)
					ifa->ifa_rtrequest(RTM_ADD, rt, &info);
#ifdef NET_MPSAFE
				rt_update_finish(rt);
			} else {
				/*
				 * If error != 0, the rtentry is being
				 * destroyed, so doing nothing doesn't
				 * matter.
				 */
			}
#endif
		}
		rt_newmsg(cmd, rt);
		rt_unref(rt);
		RT_REFCNT_TRACE(rt);
		break;
	}
	return error;
}

/*
 * Create a local route entry for the address.
 * Announce the addition of the address and the route to the routing socket.
 */
int
rt_ifa_addlocal(struct ifaddr *ifa)
{
	struct rtentry *rt;
	int e;

	/* If there is no loopback entry, allocate one. */
	rt = rtalloc1(ifa->ifa_addr, 0);
#ifdef RT_DEBUG
	if (rt != NULL)
		dump_rt(rt);
#endif
	if (rt == NULL || (rt->rt_flags & RTF_HOST) == 0 ||
	    (rt->rt_ifp->if_flags & IFF_LOOPBACK) == 0)
	{
		struct rt_addrinfo info;
		struct rtentry *nrt;

		memset(&info, 0, sizeof(info));
		info.rti_flags = RTF_HOST | RTF_LOCAL;
		info.rti_info[RTAX_DST] = ifa->ifa_addr;
		info.rti_info[RTAX_GATEWAY] =
		    (const struct sockaddr *)ifa->ifa_ifp->if_sadl;
		info.rti_ifa = ifa;
		nrt = NULL;
		e = rtrequest1(RTM_ADD, &info, &nrt);
		if (nrt && ifa != nrt->rt_ifa)
			rt_replace_ifa(nrt, ifa);
		rt_newaddrmsg(RTM_ADD, ifa, e, nrt);
		if (nrt != NULL) {
#ifdef RT_DEBUG
			dump_rt(nrt);
#endif
			rt_unref(nrt);
			RT_REFCNT_TRACE(nrt);
		}
	} else {
		e = 0;
		rt_newaddrmsg(RTM_NEWADDR, ifa, 0, NULL);
	}
	if (rt != NULL)
		rt_unref(rt);
	return e;
}

/*
 * Remove the local route entry for the address.
 * Announce the removal of the address and the route to the routing socket.
 */
int
rt_ifa_remlocal(struct ifaddr *ifa, struct ifaddr *alt_ifa)
{
	struct rtentry *rt;
	int e = 0;

	rt = rtalloc1(ifa->ifa_addr, 0);

	/*
	 * Before deleting, check if a corresponding loopbacked
	 * host route surely exists.  With this check, we can avoid
	 * deleting an interface direct route whose destination is
	 * the same as the address being removed.  This can happen
	 * when removing a subnet-router anycast address on an
	 * interface attached to a shared medium.
	 */
	if (rt != NULL &&
	    (rt->rt_flags & RTF_HOST) &&
	    (rt->rt_ifp->if_flags & IFF_LOOPBACK))
	{
		/* If we cannot replace the route's ifaddr with the equivalent
		 * ifaddr of another interface, I believe it is safest to
		 * delete the route.
		 */
		if (alt_ifa == NULL) {
			e = rtdeletemsg(rt);
			if (e == 0) {
				rt_unref(rt);
				rt_free(rt);
				rt = NULL;
			}
			rt_newaddrmsg(RTM_DELADDR, ifa, 0, NULL);
		} else {
			rt_replace_ifa(rt, alt_ifa);
			rt_newmsg(RTM_CHANGE, rt);
		}
	} else
		rt_newaddrmsg(RTM_DELADDR, ifa, 0, NULL);
	if (rt != NULL)
		rt_unref(rt);
	return e;
}

/*
 * Route timer routines.  These routes allow functions to be called
 * for various routes at any time.  This is useful in supporting
 * path MTU discovery and redirect route deletion.
 *
 * This is similar to some BSDI internal functions, but it provides
 * for multiple queues for efficiency's sake...
 */

LIST_HEAD(, rttimer_queue) rttimer_queue_head;
static int rt_init_done = 0;

/*
 * Some subtle order problems with domain initialization mean that
 * we cannot count on this being run from rt_init before various
 * protocol initializations are done.  Therefore, we make sure
 * that this is run when the first queue is added...
 */

static void rt_timer_work(struct work *, void *);

static void
rt_timer_init(void)
{
	int error;

	assert(rt_init_done == 0);

	/* XXX should be in rt_init */
	rw_init(&rt_lock);

	LIST_INIT(&rttimer_queue_head);
	callout_init(&rt_timer_ch, CALLOUT_MPSAFE);
	error = workqueue_create(&rt_timer_wq, "rt_timer",
	    rt_timer_work, NULL, PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (error)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);
	callout_reset(&rt_timer_ch, hz, rt_timer_timer, NULL);
	rt_init_done = 1;
}

struct rttimer_queue *
rt_timer_queue_create(u_int timeout)
{
	struct rttimer_queue *rtq;

	if (rt_init_done == 0)
		rt_timer_init();

	R_Malloc(rtq, struct rttimer_queue *, sizeof *rtq);
	if (rtq == NULL)
		return NULL;
	memset(rtq, 0, sizeof(*rtq));

	rtq->rtq_timeout = timeout;
	TAILQ_INIT(&rtq->rtq_head);
	RT_WLOCK();
	LIST_INSERT_HEAD(&rttimer_queue_head, rtq, rtq_link);
	RT_UNLOCK();

	return rtq;
}

void
rt_timer_queue_change(struct rttimer_queue *rtq, long timeout)
{

	rtq->rtq_timeout = timeout;
}

static void
rt_timer_queue_remove_all(struct rttimer_queue *rtq)
{
	struct rttimer *r;

	RT_ASSERT_WLOCK();

	while ((r = TAILQ_FIRST(&rtq->rtq_head)) != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&rtq->rtq_head, r, rtt_next);
		rt_ref(r->rtt_rt); /* XXX */
		RT_REFCNT_TRACE(r->rtt_rt);
		RT_UNLOCK();
		(*r->rtt_func)(r->rtt_rt, r);
		pool_put(&rttimer_pool, r);
		RT_WLOCK();
		if (rtq->rtq_count > 0)
			rtq->rtq_count--;
		else
			printf("rt_timer_queue_remove_all: "
			    "rtq_count reached 0\n");
	}
}

void
rt_timer_queue_destroy(struct rttimer_queue *rtq)
{

	RT_WLOCK();
	rt_timer_queue_remove_all(rtq);
	LIST_REMOVE(rtq, rtq_link);
	RT_UNLOCK();

	/*
	 * Caller is responsible for freeing the rttimer_queue structure.
	 */
}

unsigned long
rt_timer_count(struct rttimer_queue *rtq)
{
	return rtq->rtq_count;
}

static void
rt_timer_remove_all(struct rtentry *rt)
{
	struct rttimer *r;

	RT_WLOCK();
	while ((r = LIST_FIRST(&rt->rt_timer)) != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&r->rtt_queue->rtq_head, r, rtt_next);
		if (r->rtt_queue->rtq_count > 0)
			r->rtt_queue->rtq_count--;
		else
			printf("rt_timer_remove_all: rtq_count reached 0\n");
		pool_put(&rttimer_pool, r);
	}
	RT_UNLOCK();
}

int
rt_timer_add(struct rtentry *rt,
	void (*func)(struct rtentry *, struct rttimer *),
	struct rttimer_queue *queue)
{
	struct rttimer *r;

	KASSERT(func != NULL);
	RT_WLOCK();
	/*
	 * If there's already a timer with this action, destroy it before
	 * we add a new one.
	 */
	LIST_FOREACH(r, &rt->rt_timer, rtt_link) {
		if (r->rtt_func == func)
			break;
	}
	if (r != NULL) {
		LIST_REMOVE(r, rtt_link);
		TAILQ_REMOVE(&r->rtt_queue->rtq_head, r, rtt_next);
		if (r->rtt_queue->rtq_count > 0)
			r->rtt_queue->rtq_count--;
		else
			printf("rt_timer_add: rtq_count reached 0\n");
	} else {
		r = pool_get(&rttimer_pool, PR_NOWAIT);
		if (r == NULL) {
			RT_UNLOCK();
			return ENOBUFS;
		}
	}

	memset(r, 0, sizeof(*r));

	r->rtt_rt = rt;
	r->rtt_time = time_uptime;
	r->rtt_func = func;
	r->rtt_queue = queue;
	LIST_INSERT_HEAD(&rt->rt_timer, r, rtt_link);
	TAILQ_INSERT_TAIL(&queue->rtq_head, r, rtt_next);
	r->rtt_queue->rtq_count++;

	RT_UNLOCK();

	return 0;
}

static void
rt_timer_work(struct work *wk, void *arg)
{
	struct rttimer_queue *rtq;
	struct rttimer *r;

	RT_WLOCK();
	LIST_FOREACH(rtq, &rttimer_queue_head, rtq_link) {
		while ((r = TAILQ_FIRST(&rtq->rtq_head)) != NULL &&
		    (r->rtt_time + rtq->rtq_timeout) < time_uptime) {
			LIST_REMOVE(r, rtt_link);
			TAILQ_REMOVE(&rtq->rtq_head, r, rtt_next);
			rt_ref(r->rtt_rt); /* XXX */
			RT_REFCNT_TRACE(r->rtt_rt);
			RT_UNLOCK();
			(*r->rtt_func)(r->rtt_rt, r);
			pool_put(&rttimer_pool, r);
			RT_WLOCK();
			if (rtq->rtq_count > 0)
				rtq->rtq_count--;
			else
				printf("rt_timer_timer: rtq_count reached 0\n");
		}
	}
	RT_UNLOCK();

	callout_reset(&rt_timer_ch, hz, rt_timer_timer, NULL);
}

static void
rt_timer_timer(void *arg)
{

	workqueue_enqueue(rt_timer_wq, &rt_timer_wk, NULL);
}

static struct rtentry *
_rtcache_init(struct route *ro, int flag)
{
	struct rtentry *rt;

	rtcache_invariants(ro);
	KASSERT(ro->_ro_rt == NULL);

	if (rtcache_getdst(ro) == NULL)
		return NULL;
	rt = rtalloc1(rtcache_getdst(ro), flag);
	if (rt != NULL) {
		RT_RLOCK();
		if (ISSET(rt->rt_flags, RTF_UP)) {
			ro->_ro_rt = rt;
			ro->ro_rtcache_generation = rtcache_generation;
			rtcache_ref(rt, ro);
		}
		RT_UNLOCK();
		rt_unref(rt);
	}

	rtcache_invariants(ro);
	return ro->_ro_rt;
}

struct rtentry *
rtcache_init(struct route *ro)
{

	return _rtcache_init(ro, 1);
}

struct rtentry *
rtcache_init_noclone(struct route *ro)
{

	return _rtcache_init(ro, 0);
}

struct rtentry *
rtcache_update(struct route *ro, int clone)
{

	ro->_ro_rt = NULL;
	return _rtcache_init(ro, clone);
}

void
rtcache_copy(struct route *new_ro, struct route *old_ro)
{
	struct rtentry *rt;
	int ret;

	KASSERT(new_ro != old_ro);
	rtcache_invariants(new_ro);
	rtcache_invariants(old_ro);

	rt = rtcache_validate(old_ro);

	if (rtcache_getdst(old_ro) == NULL)
		goto out;
	ret = rtcache_setdst(new_ro, rtcache_getdst(old_ro));
	if (ret != 0)
		goto out;

	RT_RLOCK();
	new_ro->_ro_rt = rt;
	new_ro->ro_rtcache_generation = rtcache_generation;
	RT_UNLOCK();
	rtcache_invariants(new_ro);
out:
	rtcache_unref(rt, old_ro);
	return;
}

#if defined(RT_DEBUG) && defined(NET_MPSAFE)
static void
rtcache_trace(const char *func, struct rtentry *rt, struct route *ro)
{
	char dst[64];

	sockaddr_format(ro->ro_sa, dst, 64);
	printf("trace: %s:\tdst=%s cpu=%d lwp=%p psref=%p target=%p\n", func, dst,
	    cpu_index(curcpu()), curlwp, &ro->ro_psref, &rt->rt_psref);
}
#define RTCACHE_PSREF_TRACE(rt, ro)	rtcache_trace(__func__, (rt), (ro))
#else
#define RTCACHE_PSREF_TRACE(rt, ro)	do {} while (0)
#endif

static void
rtcache_ref(struct rtentry *rt, struct route *ro)
{

	KASSERT(rt != NULL);

#ifdef NET_MPSAFE
	RTCACHE_PSREF_TRACE(rt, ro);
	ro->ro_bound = curlwp_bind();
	psref_acquire(&ro->ro_psref, &rt->rt_psref, rt_psref_class);
#endif
}

void
rtcache_unref(struct rtentry *rt, struct route *ro)
{

	if (rt == NULL)
		return;

#ifdef NET_MPSAFE
	psref_release(&ro->ro_psref, &rt->rt_psref, rt_psref_class);
	curlwp_bindx(ro->ro_bound);
	RTCACHE_PSREF_TRACE(rt, ro);
#endif
}

struct rtentry *
rtcache_validate(struct route *ro)
{
	struct rtentry *rt = NULL;

#ifdef NET_MPSAFE
retry:
#endif
	rtcache_invariants(ro);
	RT_RLOCK();
	if (ro->ro_rtcache_generation != rtcache_generation) {
		/* The cache is invalidated */
		rt = NULL;
		goto out;
	}

	rt = ro->_ro_rt;
	if (rt == NULL)
		goto out;

	if ((rt->rt_flags & RTF_UP) == 0) {
		rt = NULL;
		goto out;
	}
#ifdef NET_MPSAFE
	if (ISSET(rt->rt_flags, RTF_UPDATING)) {
		if (rt_wait_ok()) {
			RT_UNLOCK();

			/* We can wait until the update is complete */
			rt_update_wait();
			goto retry;
		} else {
			rt = NULL;
		}
	} else
#endif
		rtcache_ref(rt, ro);
out:
	RT_UNLOCK();
	return rt;
}

struct rtentry *
rtcache_lookup2(struct route *ro, const struct sockaddr *dst,
    int clone, int *hitp)
{
	const struct sockaddr *odst;
	struct rtentry *rt = NULL;

	odst = rtcache_getdst(ro);
	if (odst == NULL)
		goto miss;

	if (sockaddr_cmp(odst, dst) != 0) {
		rtcache_free(ro);
		goto miss;
	}

	rt = rtcache_validate(ro);
	if (rt == NULL) {
		ro->_ro_rt = NULL;
		goto miss;
	}

	rtcache_invariants(ro);

	if (hitp != NULL)
		*hitp = 1;
	return rt;
miss:
	if (hitp != NULL)
		*hitp = 0;
	if (rtcache_setdst(ro, dst) == 0)
		rt = _rtcache_init(ro, clone);

	rtcache_invariants(ro);

	return rt;
}

void
rtcache_free(struct route *ro)
{

	ro->_ro_rt = NULL;
	if (ro->ro_sa != NULL) {
		sockaddr_free(ro->ro_sa);
		ro->ro_sa = NULL;
	}
	rtcache_invariants(ro);
}

int
rtcache_setdst(struct route *ro, const struct sockaddr *sa)
{
	KASSERT(sa != NULL);

	rtcache_invariants(ro);
	if (ro->ro_sa != NULL) {
		if (ro->ro_sa->sa_family == sa->sa_family) {
			ro->_ro_rt = NULL;
			sockaddr_copy(ro->ro_sa, ro->ro_sa->sa_len, sa);
			rtcache_invariants(ro);
			return 0;
		}
		/* free ro_sa, wrong family */
		rtcache_free(ro);
	}

	KASSERT(ro->_ro_rt == NULL);

	if ((ro->ro_sa = sockaddr_dup(sa, M_ZERO | M_NOWAIT)) == NULL) {
		rtcache_invariants(ro);
		return ENOMEM;
	}
	rtcache_invariants(ro);
	return 0;
}

const struct sockaddr *
rt_settag(struct rtentry *rt, const struct sockaddr *tag)
{
	if (rt->rt_tag != tag) {
		if (rt->rt_tag != NULL)
			sockaddr_free(rt->rt_tag);
		rt->rt_tag = sockaddr_dup(tag, M_ZERO | M_NOWAIT);
	}
	return rt->rt_tag;
}

struct sockaddr *
rt_gettag(const struct rtentry *rt)
{
	return rt->rt_tag;
}

int
rt_check_reject_route(const struct rtentry *rt, const struct ifnet *ifp)
{

	if ((rt->rt_flags & RTF_REJECT) != 0) {
		/* Mimic looutput */
		if (ifp->if_flags & IFF_LOOPBACK)
			return (rt->rt_flags & RTF_HOST) ?
			    EHOSTUNREACH : ENETUNREACH;
		else if (rt->rt_rmx.rmx_expire == 0 ||
		    time_uptime < rt->rt_rmx.rmx_expire)
			return (rt->rt_flags & RTF_GATEWAY) ?
			    EHOSTUNREACH : EHOSTDOWN;
	}

	return 0;
}

void
rt_delete_matched_entries(sa_family_t family, int (*f)(struct rtentry *, void *),
    void *v)
{

	for (;;) {
		int s;
		int error;
		struct rtentry *rt, *retrt = NULL;

		RT_RLOCK();
		s = splsoftnet();
		rt = rtbl_search_matched_entry(family, f, v);
		if (rt == NULL) {
			splx(s);
			RT_UNLOCK();
			return;
		}
		rt->rt_refcnt++;
		splx(s);
		RT_UNLOCK();

		error = rtrequest(RTM_DELETE, rt_getkey(rt), rt->rt_gateway,
		    rt_mask(rt), rt->rt_flags, &retrt);
		if (error == 0) {
			KASSERT(retrt == rt);
			KASSERT((retrt->rt_flags & RTF_UP) == 0);
			retrt->rt_ifp = NULL;
			rt_unref(rt);
			rt_free(retrt);
		} else if (error == ESRCH) {
			/* Someone deleted the entry already. */
			rt_unref(rt);
		} else {
			log(LOG_ERR, "%s: unable to delete rtentry @ %p, "
			    "error = %d\n", rt->rt_ifp->if_xname, rt, error);
			/* XXX how to treat this case? */
		}
	}
}

static int
rt_walktree_locked(sa_family_t family, int (*f)(struct rtentry *, void *),
    void *v)
{

	return rtbl_walktree(family, f, v);
}

int
rt_walktree(sa_family_t family, int (*f)(struct rtentry *, void *), void *v)
{
	int error;

	RT_RLOCK();
	error = rt_walktree_locked(family, f, v);
	RT_UNLOCK();

	return error;
}

#ifdef DDB

#include <machine/db_machdep.h>
#include <ddb/db_interface.h>
#include <ddb/db_output.h>

#define	rt_expire rt_rmx.rmx_expire

static void
db_print_sa(const struct sockaddr *sa)
{
	int len;
	const u_char *p;

	if (sa == NULL) {
		db_printf("[NULL]");
		return;
	}

	p = (const u_char *)sa;
	len = sa->sa_len;
	db_printf("[");
	while (len > 0) {
		db_printf("%d", *p);
		p++; len--;
		if (len) db_printf(",");
	}
	db_printf("]\n");
}

static void
db_print_ifa(struct ifaddr *ifa)
{
	if (ifa == NULL)
		return;
	db_printf("  ifa_addr=");
	db_print_sa(ifa->ifa_addr);
	db_printf("  ifa_dsta=");
	db_print_sa(ifa->ifa_dstaddr);
	db_printf("  ifa_mask=");
	db_print_sa(ifa->ifa_netmask);
	db_printf("  flags=0x%x,refcnt=%d,metric=%d\n",
			  ifa->ifa_flags,
			  ifa->ifa_refcnt,
			  ifa->ifa_metric);
}

/*
 * Function to pass to rt_walktree().
 * Return non-zero error to abort walk.
 */
static int
db_show_rtentry(struct rtentry *rt, void *w)
{
	db_printf("rtentry=%p", rt);

	db_printf(" flags=0x%x refcnt=%d use=%"PRId64" expire=%"PRId64"\n",
			  rt->rt_flags, rt->rt_refcnt,
			  rt->rt_use, (uint64_t)rt->rt_expire);

	db_printf(" key="); db_print_sa(rt_getkey(rt));
	db_printf(" mask="); db_print_sa(rt_mask(rt));
	db_printf(" gw="); db_print_sa(rt->rt_gateway);

	db_printf(" ifp=%p ", rt->rt_ifp);
	if (rt->rt_ifp)
		db_printf("(%s)", rt->rt_ifp->if_xname);
	else
		db_printf("(NULL)");

	db_printf(" ifa=%p\n", rt->rt_ifa);
	db_print_ifa(rt->rt_ifa);

	db_printf(" gwroute=%p llinfo=%p\n",
			  rt->rt_gwroute, rt->rt_llinfo);

	return 0;
}

/*
 * Function to print all the route trees.
 * Use this from ddb:  "show routes"
 */
void
db_show_routes(db_expr_t addr, bool have_addr,
    db_expr_t count, const char *modif)
{

	/* Taking RT_LOCK will fail if LOCKDEBUG is enabled. */
	rt_walktree_locked(AF_INET, db_show_rtentry, NULL);
}
#endif
