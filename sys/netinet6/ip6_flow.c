/*	$NetBSD: ip6_flow.c,v 1.34.8.1 2018/01/02 10:20:34 snj Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the 3am Software Foundry ("3am").  It was developed by Liam J. Foy
 * <liamjfoy@netbsd.org> and Matt Thomas <matt@netbsd.org>.
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
 *
 * IPv6 version was developed by Liam J. Foy. Original source existed in IPv4
 * format developed by Matt Thomas. Thanks to Joerg Sonnenberger, Matt
 * Thomas and Christos Zoulas. 
 *
 * Thanks to Liverpool John Moores University, especially Dr. David Llewellyn-Jones
 * for providing resources (to test) and Professor Madjid Merabti.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip6_flow.c,v 1.34.8.1 2018/01/02 10:20:34 snj Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socketvar.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/sysctl.h>
#include <sys/workqueue.h>
#include <sys/atomic.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet/in_systm.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6_private.h>

/*
 * IPv6 Fast Forward caches/hashes flows from one source to destination.
 *
 * Upon a successful forward IPv6FF caches and hashes details such as the
 * route, source and destination. Once another packet is received matching
 * the source and destination the packet is forwarded straight onto if_output
 * using the cached details.
 *
 * Example:
 * ether/fddi_input -> ip6flow_fastforward -> if_output
 */

static struct pool ip6flow_pool;

TAILQ_HEAD(ip6flowhead, ip6flow);

/*
 * We could use IPv4 defines (IPFLOW_HASHBITS) but we'll
 * use our own (possibly for future expansion).
 */
#define	IP6FLOW_TIMER		(5 * PR_SLOWHZ)
#define	IP6FLOW_DEFAULT_HASHSIZE	(1 << IP6FLOW_HASHBITS) 

/*
 * ip6_flow.c internal lock.
 * If we use softnet_lock, it would cause recursive lock.
 *
 * This is a tentative workaround.
 * We should make it scalable somehow in the future.
 */
static kmutex_t ip6flow_lock;
static struct ip6flowhead *ip6flowtable = NULL;
static struct ip6flowhead ip6flowlist;
static int ip6flow_inuse;

static void ip6flow_slowtimo_work(struct work *, void *);
static struct workqueue	*ip6flow_slowtimo_wq;
static struct work	ip6flow_slowtimo_wk;

static int sysctl_net_inet6_ip6_hashsize(SYSCTLFN_PROTO);
static int sysctl_net_inet6_ip6_maxflows(SYSCTLFN_PROTO);
static void ip6flow_sysctl_init(struct sysctllog **);

/*
 * Insert an ip6flow into the list.
 */
#define	IP6FLOW_INSERT(hashidx, ip6f) \
do { \
	(ip6f)->ip6f_hashidx = (hashidx); \
	TAILQ_INSERT_HEAD(&ip6flowtable[(hashidx)], (ip6f), ip6f_hash); \
	TAILQ_INSERT_HEAD(&ip6flowlist, (ip6f), ip6f_list); \
} while (/*CONSTCOND*/ 0)

/*
 * Remove an ip6flow from the list.
 */
#define	IP6FLOW_REMOVE(hashidx, ip6f) \
do { \
	TAILQ_REMOVE(&ip6flowtable[(hashidx)], (ip6f), ip6f_hash); \
	TAILQ_REMOVE(&ip6flowlist, (ip6f), ip6f_list); \
} while (/*CONSTCOND*/ 0)

#ifndef IP6FLOW_DEFAULT
#define	IP6FLOW_DEFAULT		256
#endif

int ip6_maxflows = IP6FLOW_DEFAULT;
int ip6_hashsize = IP6FLOW_DEFAULT_HASHSIZE;

/*
 * Calculate hash table position.
 */
static size_t 
ip6flow_hash(const struct ip6_hdr *ip6)
{
	size_t hash;
	uint32_t dst_sum, src_sum;
	size_t idx;

	src_sum = ip6->ip6_src.s6_addr32[0] + ip6->ip6_src.s6_addr32[1]
	    + ip6->ip6_src.s6_addr32[2] + ip6->ip6_src.s6_addr32[3];
	dst_sum = ip6->ip6_dst.s6_addr32[0] + ip6->ip6_dst.s6_addr32[1]
	    + ip6->ip6_dst.s6_addr32[2] + ip6->ip6_dst.s6_addr32[3];

	hash = ip6->ip6_flow;

	for (idx = 0; idx < 32; idx += IP6FLOW_HASHBITS)
		hash += (dst_sum >> (32 - idx)) + (src_sum >> idx);

	return hash & (ip6_hashsize-1);
}

/*
 * Check to see if a flow already exists - if so return it.
 */
static struct ip6flow *
ip6flow_lookup(const struct ip6_hdr *ip6)
{
	size_t hash;
	struct ip6flow *ip6f;

	KASSERT(mutex_owned(&ip6flow_lock));

	hash = ip6flow_hash(ip6);

	TAILQ_FOREACH(ip6f, &ip6flowtable[hash], ip6f_hash) {
		if (IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &ip6f->ip6f_dst)
		    && IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, &ip6f->ip6f_src)
		    && ip6f->ip6f_flow == ip6->ip6_flow) {
		    	/* A cached flow has been found. */
			return ip6f;
		}
	}

	return NULL;
}

void
ip6flow_poolinit(void)
{

	pool_init(&ip6flow_pool, sizeof(struct ip6flow), 0, 0, 0, "ip6flowpl",
			NULL, IPL_NET);
}

/*
 * Allocate memory and initialise lists. This function is called
 * from ip6_init and called there after to resize the hash table.
 * If a newly sized table cannot be malloc'ed we just continue
 * to use the old one.
 */
static int
ip6flow_init_locked(int table_size)
{
	struct ip6flowhead *new_table;
	size_t i;

	KASSERT(mutex_owned(&ip6flow_lock));

	new_table = (struct ip6flowhead *)malloc(sizeof(struct ip6flowhead) *
	    table_size, M_RTABLE, M_NOWAIT);

	if (new_table == NULL)
		return 1;

	if (ip6flowtable != NULL)
		free(ip6flowtable, M_RTABLE);

	ip6flowtable = new_table;
	ip6_hashsize = table_size;

	TAILQ_INIT(&ip6flowlist);
	for (i = 0; i < ip6_hashsize; i++)
		TAILQ_INIT(&ip6flowtable[i]);

	return 0;
}

int
ip6flow_init(int table_size)
{
	int ret, error;

	error = workqueue_create(&ip6flow_slowtimo_wq, "ip6flow_slowtimo",
	    ip6flow_slowtimo_work, NULL, PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (error != 0)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);

	mutex_init(&ip6flow_lock, MUTEX_DEFAULT, IPL_NONE);

	mutex_enter(&ip6flow_lock);
	ret = ip6flow_init_locked(table_size);
	mutex_exit(&ip6flow_lock);
	ip6flow_sysctl_init(NULL);

	return ret;
}

/*
 * IPv6 Fast Forward routine. Attempt to forward the packet -
 * if any problems are found return to the main IPv6 input 
 * routine to deal with.
 */
int
ip6flow_fastforward(struct mbuf **mp)
{
	struct ip6flow *ip6f;
	struct ip6_hdr *ip6;
	struct rtentry *rt = NULL;
	struct mbuf *m;
	const struct sockaddr *dst;
	int error;
	int ret = 0;

	mutex_enter(&ip6flow_lock);

	/*
	 * Are we forwarding packets and have flows?
	 */
	if (!ip6_forwarding || ip6flow_inuse == 0)
		goto out;

	m = *mp;
	/*
	 * At least size of IPv6 Header?
	 */
	if (m->m_len < sizeof(struct ip6_hdr))
		goto out;
	/*
	 * Was packet received as a link-level multicast or broadcast?
	 * If so, don't try to fast forward.
	 */
	if ((m->m_flags & (M_BCAST|M_MCAST)) != 0)
		goto out;

	if (IP6_HDR_ALIGNED_P(mtod(m, const void *)) == 0) {
		if ((m = m_copyup(m, sizeof(struct ip6_hdr),
				(max_linkhdr + 3) & ~3)) == NULL) {
			goto out;
		}
		*mp = m;
	} else if (__predict_false(m->m_len < sizeof(struct ip6_hdr))) {
		if ((m = m_pullup(m, sizeof(struct ip6_hdr))) == NULL) {
			goto out;
		}
		*mp = m;
	}

	ip6 = mtod(m, struct ip6_hdr *);

	if ((ip6->ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
		/* Bad version. */
		goto out;
	}

	/*
	 * If we have a hop-by-hop extension we must process it.
	 * We just leave this up to ip6_input to deal with. 
	 */
	if (ip6->ip6_nxt == IPPROTO_HOPOPTS)
		goto out;

	/*
	 * Attempt to find a flow.
	 */
	if ((ip6f = ip6flow_lookup(ip6)) == NULL) {
		/* No flow found. */
		goto out;
	}

	/*
	 * Route and interface still up?
	 */
	if ((rt = rtcache_validate(&ip6f->ip6f_ro)) == NULL ||
	    (rt->rt_ifp->if_flags & IFF_UP) == 0 ||
	    (rt->rt_flags & RTF_BLACKHOLE) != 0)
		goto out_unref;

	/*
	 * Packet size greater than MTU?
	 */
	if (m->m_pkthdr.len > rt->rt_ifp->if_mtu) {
		/* Return to main IPv6 input function. */
		goto out_unref;
	}

	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csum_flags = 0;

	if (ip6->ip6_hlim <= IPV6_HLIMDEC)
		goto out_unref;

	/* Decrement hop limit (same as TTL) */
	ip6->ip6_hlim -= IPV6_HLIMDEC;

	if (rt->rt_flags & RTF_GATEWAY)
		dst = rt->rt_gateway;
	else
		dst = rtcache_getdst(&ip6f->ip6f_ro);

	PRT_SLOW_ARM(ip6f->ip6f_timer, IP6FLOW_TIMER);

	ip6f->ip6f_uses++;

#if 0
	/*
	 * We use FIFO cache replacement instead of LRU the same ip_flow.c.
	 */
	/* move to head (LRU) for ip6flowlist. ip6flowtable does not care LRU. */
	TAILQ_REMOVE(&ip6flowlist, ip6f, ip6f_list);
	TAILQ_INSERT_HEAD(&ip6flowlist, ip6f, ip6f_list);
#endif

	/* Send on its way - straight to the interface output routine. */
	if ((error = if_output_lock(rt->rt_ifp, rt->rt_ifp, m, dst, rt)) != 0) {
		ip6f->ip6f_dropped++;
	} else {
		ip6f->ip6f_forwarded++;
	}
	ret = 1;
out_unref:
	rtcache_unref(rt, &ip6f->ip6f_ro);
out:
	mutex_exit(&ip6flow_lock);
	return ret;
}

/*
 * Add the IPv6 flow statistics to the main IPv6 statistics.
 */
static void
ip6flow_addstats_rt(struct rtentry *rt, struct ip6flow *ip6f)
{
	uint64_t *ip6s;

	if (rt != NULL)
		rt->rt_use += ip6f->ip6f_uses;
	ip6s = IP6_STAT_GETREF();
	ip6s[IP6_STAT_FASTFORWARDFLOWS] = ip6flow_inuse;
	ip6s[IP6_STAT_CANTFORWARD] += ip6f->ip6f_dropped;
	ip6s[IP6_STAT_ODROPPED] += ip6f->ip6f_dropped;
	ip6s[IP6_STAT_TOTAL] += ip6f->ip6f_uses;
	ip6s[IP6_STAT_FORWARD] += ip6f->ip6f_forwarded;
	ip6s[IP6_STAT_FASTFORWARD] += ip6f->ip6f_forwarded;
	IP6_STAT_PUTREF();
}

static void
ip6flow_addstats(struct ip6flow *ip6f)
{
	struct rtentry *rt;

	rt = rtcache_validate(&ip6f->ip6f_ro);
	ip6flow_addstats_rt(rt, ip6f);
	rtcache_unref(rt, &ip6f->ip6f_ro);
}

/*
 * Add statistics and free the flow.
 */
static void
ip6flow_free(struct ip6flow *ip6f)
{

	KASSERT(mutex_owned(&ip6flow_lock));

	/*
	 * Remove the flow from the hash table (at elevated IPL).
	 * Once it's off the list, we can deal with it at normal
	 * network IPL.
	 */
	IP6FLOW_REMOVE(ip6f->ip6f_hashidx, ip6f);

	ip6flow_inuse--;
	ip6flow_addstats(ip6f);
	rtcache_free(&ip6f->ip6f_ro);
	pool_put(&ip6flow_pool, ip6f);
}

static struct ip6flow *
ip6flow_reap_locked(int just_one)
{
	struct ip6flow *ip6f;

	KASSERT(mutex_owned(&ip6flow_lock));

	/*
	 * This case must remove one ip6flow. Furthermore, this case is used in
	 * fast path(packet processing path). So, simply remove TAILQ_LAST one.
	 */
	if (just_one) {
		ip6f = TAILQ_LAST(&ip6flowlist, ip6flowhead);
		KASSERT(ip6f != NULL);

		IP6FLOW_REMOVE(ip6f->ip6f_hashidx, ip6f);

		ip6flow_addstats(ip6f);
		rtcache_free(&ip6f->ip6f_ro);
		return ip6f;
	}

	/*
	 * This case is used in slow path(sysctl).
	 * At first, remove invalid rtcache ip6flow, and then remove TAILQ_LAST
	 * ip6flow if it is ensured least recently used by comparing last_uses.
	 */
	while (ip6flow_inuse > ip6_maxflows) {
		struct ip6flow *maybe_ip6f = TAILQ_LAST(&ip6flowlist, ip6flowhead);

		TAILQ_FOREACH(ip6f, &ip6flowlist, ip6f_list) {
			struct rtentry *rt;
			/*
			 * If this no longer points to a valid route -
			 * reclaim it.
			 */
			if ((rt = rtcache_validate(&ip6f->ip6f_ro)) == NULL)
				goto done;
			rtcache_unref(rt, &ip6f->ip6f_ro);
			/*
			 * choose the one that's been least recently
			 * used or has had the least uses in the
			 * last 1.5 intervals.
			 */
			if (ip6f->ip6f_timer < maybe_ip6f->ip6f_timer
			    || ((ip6f->ip6f_timer == maybe_ip6f->ip6f_timer)
				&& (ip6f->ip6f_last_uses + ip6f->ip6f_uses
				    < maybe_ip6f->ip6f_last_uses + maybe_ip6f->ip6f_uses)))
				maybe_ip6f = ip6f;
		}
		ip6f = maybe_ip6f;
	    done:
		/*
		 * Remove the entry from the flow table
		 */
		IP6FLOW_REMOVE(ip6f->ip6f_hashidx, ip6f);

		rtcache_free(&ip6f->ip6f_ro);
		ip6flow_inuse--;
		ip6flow_addstats(ip6f);
		pool_put(&ip6flow_pool, ip6f);
	}
	return NULL;
}

/*
 * Reap one or more flows - ip6flow_reap may remove
 * multiple flows if net.inet6.ip6.maxflows is reduced. 
 */
struct ip6flow *
ip6flow_reap(int just_one)
{
	struct ip6flow *ip6f;

	mutex_enter(&ip6flow_lock);
	ip6f = ip6flow_reap_locked(just_one);
	mutex_exit(&ip6flow_lock);
	return ip6f;
}

static unsigned int ip6flow_work_enqueued = 0;

void
ip6flow_slowtimo_work(struct work *wk, void *arg)
{
	struct ip6flow *ip6f, *next_ip6f;

	/* We can allow enqueuing another work at this point */
	atomic_swap_uint(&ip6flow_work_enqueued, 0);

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	mutex_enter(&ip6flow_lock);

	for (ip6f = TAILQ_FIRST(&ip6flowlist); ip6f != NULL; ip6f = next_ip6f) {
		struct rtentry *rt = NULL;
		next_ip6f = TAILQ_NEXT(ip6f, ip6f_list);
		if (PRT_SLOW_ISEXPIRED(ip6f->ip6f_timer) ||
		    (rt = rtcache_validate(&ip6f->ip6f_ro)) == NULL) {
			ip6flow_free(ip6f);
		} else {
			ip6f->ip6f_last_uses = ip6f->ip6f_uses;
			ip6flow_addstats_rt(rt, ip6f);
			ip6f->ip6f_uses = 0;
			ip6f->ip6f_dropped = 0;
			ip6f->ip6f_forwarded = 0;
		}
		rtcache_unref(rt, &ip6f->ip6f_ro);
	}

	mutex_exit(&ip6flow_lock);
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

void
ip6flow_slowtimo(void)
{

	/* Avoid enqueuing another work when one is already enqueued */
	if (atomic_swap_uint(&ip6flow_work_enqueued, 1) == 1)
		return;

	workqueue_enqueue(ip6flow_slowtimo_wq, &ip6flow_slowtimo_wk, NULL);
}

/*
 * We have successfully forwarded a packet using the normal
 * IPv6 stack. Now create/update a flow.
 */
void
ip6flow_create(struct route *ro, struct mbuf *m)
{
	const struct ip6_hdr *ip6;
	struct ip6flow *ip6f;
	size_t hash;

	ip6 = mtod(m, const struct ip6_hdr *);

	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	mutex_enter(&ip6flow_lock);

	/*
	 * If IPv6 Fast Forward is disabled, don't create a flow.
	 * It can be disabled by setting net.inet6.ip6.maxflows to 0.
	 *
	 * Don't create a flow for ICMPv6 messages.
	 */
	if (ip6_maxflows == 0 || ip6->ip6_nxt == IPPROTO_IPV6_ICMP)
		goto out;

	/*
	 * See if an existing flow exists.  If so:
	 *	- Remove the flow
	 *	- Add flow statistics
	 *	- Free the route
	 *	- Reset statistics
	 *
	 * If a flow doesn't exist allocate a new one if
	 * ip6_maxflows hasn't reached its limit. If it has
	 * been reached, reap some flows.
	 */
	ip6f = ip6flow_lookup(ip6);
	if (ip6f == NULL) {
		if (ip6flow_inuse >= ip6_maxflows) {
			ip6f = ip6flow_reap_locked(1);
		} else {
			ip6f = pool_get(&ip6flow_pool, PR_NOWAIT);
			if (ip6f == NULL)
				goto out;
			ip6flow_inuse++;
		}
		memset(ip6f, 0, sizeof(*ip6f));
	} else {
		IP6FLOW_REMOVE(ip6f->ip6f_hashidx, ip6f);

		ip6flow_addstats(ip6f);
		rtcache_free(&ip6f->ip6f_ro);
		ip6f->ip6f_uses = 0;
		ip6f->ip6f_last_uses = 0;
		ip6f->ip6f_dropped = 0;
		ip6f->ip6f_forwarded = 0;
	}

	/*
	 * Fill in the updated/new details.
	 */
	rtcache_copy(&ip6f->ip6f_ro, ro);
	ip6f->ip6f_dst = ip6->ip6_dst;
	ip6f->ip6f_src = ip6->ip6_src;
	ip6f->ip6f_flow = ip6->ip6_flow;
	PRT_SLOW_ARM(ip6f->ip6f_timer, IP6FLOW_TIMER);

	/*
	 * Insert into the approriate bucket of the flow table.
	 */
	hash = ip6flow_hash(ip6);
	IP6FLOW_INSERT(hash, ip6f);

 out:
	mutex_exit(&ip6flow_lock);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

/*
 * Invalidate/remove all flows - if new_size is positive we
 * resize the hash table.
 */
int
ip6flow_invalidate_all(int new_size)
{
	struct ip6flow *ip6f, *next_ip6f;
	int error;

	error = 0;

	mutex_enter(&ip6flow_lock);

	for (ip6f = TAILQ_FIRST(&ip6flowlist); ip6f != NULL; ip6f = next_ip6f) {
		next_ip6f = TAILQ_NEXT(ip6f, ip6f_list);
		ip6flow_free(ip6f);
	}

	if (new_size) 
		error = ip6flow_init_locked(new_size);

	mutex_exit(&ip6flow_lock);

	return error;
}

/*
 * sysctl helper routine for net.inet.ip6.maxflows. Since
 * we could reduce this value, call ip6flow_reap();
 */
static int
sysctl_net_inet6_ip6_maxflows(SYSCTLFN_ARGS)
{
	int error;

	error = sysctl_lookup(SYSCTLFN_CALL(rnode));
	if (error || newp == NULL)
		return (error);

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();

	ip6flow_reap(0);

	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();

	return (0);
}

static int
sysctl_net_inet6_ip6_hashsize(SYSCTLFN_ARGS)
{
	int error, tmp;
	struct sysctlnode node;

	node = *rnode;
	tmp = ip6_hashsize;
	node.sysctl_data = &tmp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if ((tmp & (tmp - 1)) == 0 && tmp != 0) {
		/*
		 * Can only fail due to malloc()
		 */
		SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
		error = ip6flow_invalidate_all(tmp);
		SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
	} else {
		/*
		 * EINVAL if not a power of 2
		 */
		error = EINVAL;
	}

	return error;
}

static void
ip6flow_sysctl_init(struct sysctllog **clog)
{

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet6",
		       SYSCTL_DESCR("PF_INET6 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ip6",
		       SYSCTL_DESCR("IPv6 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET6, IPPROTO_IPV6, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "maxflows",
			SYSCTL_DESCR("Number of flows for fast forwarding (IPv6)"),
			sysctl_net_inet6_ip6_maxflows, 0, &ip6_maxflows, 0,
			CTL_NET, PF_INET6, IPPROTO_IPV6,
			CTL_CREATE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "hashsize",
			SYSCTL_DESCR("Size of hash table for fast forwarding (IPv6)"),
			sysctl_net_inet6_ip6_hashsize, 0, &ip6_hashsize, 0,
			CTL_NET, PF_INET6, IPPROTO_IPV6,
			CTL_CREATE, CTL_EOL);
}
