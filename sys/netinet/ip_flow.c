/*	$NetBSD: ip_flow.c,v 1.81 2017/11/17 07:37:12 ozaki-r Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by the 3am Software Foundry ("3am").  It was developed by Matt Thomas.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip_flow.c,v 1.81 2017/11/17 07:37:12 ozaki-r Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
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
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip_private.h>

/*
 * Similar code is very well commented in netinet6/ip6_flow.c
 */ 

#define	IPFLOW_HASHBITS		6	/* should not be a multiple of 8 */

static struct pool ipflow_pool;

TAILQ_HEAD(ipflowhead, ipflow);

#define	IPFLOW_TIMER		(5 * PR_SLOWHZ)
#define	IPFLOW_DEFAULT_HASHSIZE	(1 << IPFLOW_HASHBITS)

/*
 * ip_flow.c internal lock.
 * If we use softnet_lock, it would cause recursive lock.
 *
 * This is a tentative workaround.
 * We should make it scalable somehow in the future.
 */
static kmutex_t ipflow_lock;
static struct ipflowhead *ipflowtable = NULL;
static struct ipflowhead ipflowlist;
static int ipflow_inuse;

#define	IPFLOW_INSERT(hashidx, ipf) \
do { \
	(ipf)->ipf_hashidx = (hashidx); \
	TAILQ_INSERT_HEAD(&ipflowtable[(hashidx)], (ipf), ipf_hash); \
	TAILQ_INSERT_HEAD(&ipflowlist, (ipf), ipf_list); \
} while (/*CONSTCOND*/ 0)

#define	IPFLOW_REMOVE(hashidx, ipf) \
do { \
	TAILQ_REMOVE(&ipflowtable[(hashidx)], (ipf), ipf_hash); \
	TAILQ_REMOVE(&ipflowlist, (ipf), ipf_list); \
} while (/*CONSTCOND*/ 0)

#ifndef IPFLOW_MAX
#define	IPFLOW_MAX		256
#endif
static int ip_maxflows = IPFLOW_MAX;
static int ip_hashsize = IPFLOW_DEFAULT_HASHSIZE;

static struct ipflow *ipflow_reap(bool);
static void ipflow_sysctl_init(struct sysctllog **);

static void ipflow_slowtimo_work(struct work *, void *);
static struct workqueue	*ipflow_slowtimo_wq;
static struct work	ipflow_slowtimo_wk;

static size_t 
ipflow_hash(const struct ip *ip)
{
	size_t hash = ip->ip_tos;
	size_t idx;

	for (idx = 0; idx < 32; idx += IPFLOW_HASHBITS) {
		hash += (ip->ip_dst.s_addr >> (32 - idx)) +
		    (ip->ip_src.s_addr >> idx);
	}

	return hash & (ip_hashsize-1);
}

static struct ipflow *
ipflow_lookup(const struct ip *ip)
{
	size_t hash;
	struct ipflow *ipf;

	KASSERT(mutex_owned(&ipflow_lock));

	hash = ipflow_hash(ip);

	TAILQ_FOREACH(ipf, &ipflowtable[hash], ipf_hash) {
		if (ip->ip_dst.s_addr == ipf->ipf_dst.s_addr
		    && ip->ip_src.s_addr == ipf->ipf_src.s_addr
		    && ip->ip_tos == ipf->ipf_tos)
			break;
	}
	return ipf;
}

void
ipflow_poolinit(void)
{

	pool_init(&ipflow_pool, sizeof(struct ipflow), 0, 0, 0, "ipflowpl",
	    NULL, IPL_NET);
}

static int
ipflow_reinit(int table_size)
{
	struct ipflowhead *new_table;
	size_t i;

	KASSERT(mutex_owned(&ipflow_lock));

	new_table = (struct ipflowhead *)malloc(sizeof(struct ipflowhead) *
	    table_size, M_RTABLE, M_NOWAIT);

	if (new_table == NULL)
		return 1;

	if (ipflowtable != NULL)
		free(ipflowtable, M_RTABLE);

	ipflowtable = new_table;
	ip_hashsize = table_size;

	TAILQ_INIT(&ipflowlist);
	for (i = 0; i < ip_hashsize; i++)
		TAILQ_INIT(&ipflowtable[i]);

	return 0;
}

void
ipflow_init(void)
{
	int error;

	error = workqueue_create(&ipflow_slowtimo_wq, "ipflow_slowtimo",
	    ipflow_slowtimo_work, NULL, PRI_SOFTNET, IPL_SOFTNET, WQ_MPSAFE);
	if (error != 0)
		panic("%s: workqueue_create failed (%d)\n", __func__, error);

	mutex_init(&ipflow_lock, MUTEX_DEFAULT, IPL_NONE);

	mutex_enter(&ipflow_lock);
	(void)ipflow_reinit(ip_hashsize);
	mutex_exit(&ipflow_lock);
	ipflow_sysctl_init(NULL);
}

int
ipflow_fastforward(struct mbuf *m)
{
	struct ip *ip;
	struct ip ip_store;
	struct ipflow *ipf;
	struct rtentry *rt = NULL;
	const struct sockaddr *dst;
	int error;
	int iplen;
	struct ifnet *ifp;
	int s;
	int ret = 0;

	mutex_enter(&ipflow_lock);
	/*
	 * Are we forwarding packets?  Big enough for an IP packet?
	 */
	if (!ipforwarding || ipflow_inuse == 0 || m->m_len < sizeof(struct ip))
		goto out;

	/*
	 * Was packet received as a link-level multicast or broadcast?
	 * If so, don't try to fast forward..
	 */
	if ((m->m_flags & (M_BCAST|M_MCAST)) != 0)
		goto out;

	/*
	 * IP header with no option and valid version and length
	 */
	if (IP_HDR_ALIGNED_P(mtod(m, const void *)))
		ip = mtod(m, struct ip *);
	else {
		memcpy(&ip_store, mtod(m, const void *), sizeof(ip_store));
		ip = &ip_store;
	}
	iplen = ntohs(ip->ip_len);
	if (ip->ip_v != IPVERSION || ip->ip_hl != (sizeof(struct ip) >> 2) ||
	    iplen < sizeof(struct ip) || iplen > m->m_pkthdr.len)
		goto out;
	/*
	 * Find a flow.
	 */
	if ((ipf = ipflow_lookup(ip)) == NULL)
		goto out;

	ifp = m_get_rcvif(m, &s);
	if (__predict_false(ifp == NULL))
		goto out_unref;
	/*
	 * Verify the IP header checksum.
	 */
	switch (m->m_pkthdr.csum_flags &
		((ifp->if_csum_flags_rx & M_CSUM_IPv4) |
		 M_CSUM_IPv4_BAD)) {
	case M_CSUM_IPv4|M_CSUM_IPv4_BAD:
		m_put_rcvif(ifp, &s);
		goto out_unref;

	case M_CSUM_IPv4:
		/* Checksum was okay. */
		break;

	default:
		/* Must compute it ourselves. */
		if (in_cksum(m, sizeof(struct ip)) != 0) {
			m_put_rcvif(ifp, &s);
			goto out_unref;
		}
		break;
	}
	m_put_rcvif(ifp, &s);

	/*
	 * Route and interface still up?
	 */
	rt = rtcache_validate(&ipf->ipf_ro);
	if (rt == NULL || (rt->rt_ifp->if_flags & IFF_UP) == 0 ||
	    (rt->rt_flags & (RTF_BLACKHOLE | RTF_BROADCAST)) != 0)
		goto out_unref;

	/*
	 * Packet size OK?  TTL?
	 */
	if (m->m_pkthdr.len > rt->rt_ifp->if_mtu || ip->ip_ttl <= IPTTLDEC)
		goto out_unref;

	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csum_flags = 0;

	/*
	 * Everything checks out and so we can forward this packet.
	 * Modify the TTL and incrementally change the checksum.
	 *
	 * This method of adding the checksum works on either endian CPU.
	 * If htons() is inlined, all the arithmetic is folded; otherwise
	 * the htons()s are combined by CSE due to the const attribute.
	 *
	 * Don't bother using HW checksumming here -- the incremental
	 * update is pretty fast.
	 */
	ip->ip_ttl -= IPTTLDEC;
	if (ip->ip_sum >= (u_int16_t) ~htons(IPTTLDEC << 8))
		ip->ip_sum -= ~htons(IPTTLDEC << 8);
	else
		ip->ip_sum += htons(IPTTLDEC << 8);

	/*
	 * Done modifying the header; copy it back, if necessary.
	 *
	 * XXX Use m_copyback_cow(9) here? --dyoung
	 */
	if (IP_HDR_ALIGNED_P(mtod(m, void *)) == 0)
		memcpy(mtod(m, void *), &ip_store, sizeof(ip_store));

	/*
	 * Trim the packet in case it's too long..
	 */
	if (m->m_pkthdr.len > iplen) {
		if (m->m_len == m->m_pkthdr.len) {
			m->m_len = iplen;
			m->m_pkthdr.len = iplen;
		} else
			m_adj(m, iplen - m->m_pkthdr.len);
	}

	/*
	 * Send the packet on its way.  All we can get back is ENOBUFS
	 */
	ipf->ipf_uses++;

#if 0
	/*
	 * Sorting list is too heavy for fast path(packet processing path).
	 * It degrades about 10% performance. So, we does not sort ipflowtable,
	 * and then we use FIFO cache replacement instead fo LRU.
	 */
	/* move to head (LRU) for ipflowlist. ipflowtable ooes not care LRU. */
	TAILQ_REMOVE(&ipflowlist, ipf, ipf_list);
	TAILQ_INSERT_HEAD(&ipflowlist, ipf, ipf_list);
#endif

	PRT_SLOW_ARM(ipf->ipf_timer, IPFLOW_TIMER);

	if (rt->rt_flags & RTF_GATEWAY)
		dst = rt->rt_gateway;
	else
		dst = rtcache_getdst(&ipf->ipf_ro);

	if ((error = if_output_lock(rt->rt_ifp, rt->rt_ifp, m, dst, rt)) != 0) {
		if (error == ENOBUFS)
			ipf->ipf_dropped++;
		else
			ipf->ipf_errors++;
	}
	ret = 1;
out_unref:
	rtcache_unref(rt, &ipf->ipf_ro);
out:
	mutex_exit(&ipflow_lock);
	return ret;
}

static void
ipflow_addstats(struct ipflow *ipf)
{
	struct rtentry *rt;
	uint64_t *ips;

	rt = rtcache_validate(&ipf->ipf_ro);
	if (rt != NULL) {
		rt->rt_use += ipf->ipf_uses;
		rtcache_unref(rt, &ipf->ipf_ro);
	}
	
	ips = IP_STAT_GETREF();
	ips[IP_STAT_CANTFORWARD] += ipf->ipf_errors + ipf->ipf_dropped;
	ips[IP_STAT_TOTAL] += ipf->ipf_uses;
	ips[IP_STAT_FORWARD] += ipf->ipf_uses;
	ips[IP_STAT_FASTFORWARD] += ipf->ipf_uses;
	IP_STAT_PUTREF();
}

static void
ipflow_free(struct ipflow *ipf)
{

	KASSERT(mutex_owned(&ipflow_lock));

	/*
	 * Remove the flow from the hash table (at elevated IPL).
	 * Once it's off the list, we can deal with it at normal
	 * network IPL.
	 */
	IPFLOW_REMOVE(ipf->ipf_hashidx, ipf);

	ipflow_addstats(ipf);
	rtcache_free(&ipf->ipf_ro);
	ipflow_inuse--;
	pool_put(&ipflow_pool, ipf);
}

static struct ipflow *
ipflow_reap(bool just_one)
{
	struct ipflow *ipf;

	KASSERT(mutex_owned(&ipflow_lock));

	/*
	 * This case must remove one ipflow. Furthermore, this case is used in
	 * fast path(packet processing path). So, simply remove TAILQ_LAST one.
	 */
	if (just_one) {
		ipf = TAILQ_LAST(&ipflowlist, ipflowhead);
		KASSERT(ipf != NULL);

		IPFLOW_REMOVE(ipf->ipf_hashidx, ipf);

		ipflow_addstats(ipf);
		rtcache_free(&ipf->ipf_ro);
		return ipf;
	}

	/*
	 * This case is used in slow path(sysctl).
	 * At first, remove invalid rtcache ipflow, and then remove TAILQ_LAST
	 * ipflow if it is ensured least recently used by comparing last_uses.
	 */
	while (ipflow_inuse > ip_maxflows) {
		struct ipflow *maybe_ipf = TAILQ_LAST(&ipflowlist, ipflowhead);

		TAILQ_FOREACH(ipf, &ipflowlist, ipf_list) {
			struct rtentry *rt;
			/*
			 * If this no longer points to a valid route
			 * reclaim it.
			 */
			rt = rtcache_validate(&ipf->ipf_ro);
			if (rt == NULL)
				goto done;
			rtcache_unref(rt, &ipf->ipf_ro);
			/*
			 * choose the one that's been least recently
			 * used or has had the least uses in the
			 * last 1.5 intervals.
			 */
			if (ipf->ipf_timer < maybe_ipf->ipf_timer
			    || ((ipf->ipf_timer == maybe_ipf->ipf_timer)
				&& (ipf->ipf_last_uses + ipf->ipf_uses
				    < maybe_ipf->ipf_last_uses + maybe_ipf->ipf_uses)))
				maybe_ipf = ipf;
		}
		ipf = maybe_ipf;
	    done:
		/*
		 * Remove the entry from the flow table.
		 */
		IPFLOW_REMOVE(ipf->ipf_hashidx, ipf);

		ipflow_addstats(ipf);
		rtcache_free(&ipf->ipf_ro);
		pool_put(&ipflow_pool, ipf);
		ipflow_inuse--;
	}
	return NULL;
}

static unsigned int ipflow_work_enqueued = 0;

static void
ipflow_slowtimo_work(struct work *wk, void *arg)
{
	struct rtentry *rt;
	struct ipflow *ipf, *next_ipf;
	uint64_t *ips;

	/* We can allow enqueuing another work at this point */
	atomic_swap_uint(&ipflow_work_enqueued, 0);

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	mutex_enter(&ipflow_lock);
	for (ipf = TAILQ_FIRST(&ipflowlist); ipf != NULL; ipf = next_ipf) {
		next_ipf = TAILQ_NEXT(ipf, ipf_list);
		if (PRT_SLOW_ISEXPIRED(ipf->ipf_timer) ||
		    (rt = rtcache_validate(&ipf->ipf_ro)) == NULL) {
			ipflow_free(ipf);
		} else {
			ipf->ipf_last_uses = ipf->ipf_uses;
			rt->rt_use += ipf->ipf_uses;
			rtcache_unref(rt, &ipf->ipf_ro);
			ips = IP_STAT_GETREF();
			ips[IP_STAT_TOTAL] += ipf->ipf_uses;
			ips[IP_STAT_FORWARD] += ipf->ipf_uses;
			ips[IP_STAT_FASTFORWARD] += ipf->ipf_uses;
			IP_STAT_PUTREF();
			ipf->ipf_uses = 0;
		}
	}
	mutex_exit(&ipflow_lock);
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

void
ipflow_slowtimo(void)
{

	/* Avoid enqueuing another work when one is already enqueued */
	if (atomic_swap_uint(&ipflow_work_enqueued, 1) == 1)
		return;

	workqueue_enqueue(ipflow_slowtimo_wq, &ipflow_slowtimo_wk, NULL);
}

void
ipflow_create(struct route *ro, struct mbuf *m)
{
	const struct ip *const ip = mtod(m, const struct ip *);
	struct ipflow *ipf;
	size_t hash;

	KERNEL_LOCK_UNLESS_NET_MPSAFE();
	mutex_enter(&ipflow_lock);

	/*
	 * Don't create cache entries for ICMP messages.
	 */
	if (ip_maxflows == 0 || ip->ip_p == IPPROTO_ICMP)
		goto out;

	/*
	 * See if an existing flow struct exists.  If so remove it from its
	 * list and free the old route.  If not, try to malloc a new one
	 * (if we aren't at our limit).
	 */
	ipf = ipflow_lookup(ip);
	if (ipf == NULL) {
		if (ipflow_inuse >= ip_maxflows) {
			ipf = ipflow_reap(true);
		} else {
			ipf = pool_get(&ipflow_pool, PR_NOWAIT);
			if (ipf == NULL)
				goto out;
			ipflow_inuse++;
		}
		memset(ipf, 0, sizeof(*ipf));
	} else {
		IPFLOW_REMOVE(ipf->ipf_hashidx, ipf);

		ipflow_addstats(ipf);
		rtcache_free(&ipf->ipf_ro);
		ipf->ipf_uses = ipf->ipf_last_uses = 0;
		ipf->ipf_errors = ipf->ipf_dropped = 0;
	}

	/*
	 * Fill in the updated information.
	 */
	rtcache_copy(&ipf->ipf_ro, ro);
	ipf->ipf_dst = ip->ip_dst;
	ipf->ipf_src = ip->ip_src;
	ipf->ipf_tos = ip->ip_tos;
	PRT_SLOW_ARM(ipf->ipf_timer, IPFLOW_TIMER);

	/*
	 * Insert into the approriate bucket of the flow table.
	 */
	hash = ipflow_hash(ip);
	IPFLOW_INSERT(hash, ipf);

 out:
	mutex_exit(&ipflow_lock);
	KERNEL_UNLOCK_UNLESS_NET_MPSAFE();
}

int
ipflow_invalidate_all(int new_size)
{
	struct ipflow *ipf, *next_ipf;
	int error;

	error = 0;

	mutex_enter(&ipflow_lock);

	for (ipf = TAILQ_FIRST(&ipflowlist); ipf != NULL; ipf = next_ipf) {
		next_ipf = TAILQ_NEXT(ipf, ipf_list);
		ipflow_free(ipf);
	}

	if (new_size)
		error = ipflow_reinit(new_size);

	mutex_exit(&ipflow_lock);

	return error;
}

/*
 * sysctl helper routine for net.inet.ip.maxflows.
 */
static int
sysctl_net_inet_ip_maxflows(SYSCTLFN_ARGS)
{
	int error;

	error = sysctl_lookup(SYSCTLFN_CALL(rnode));
	if (error || newp == NULL)
		return (error);

	SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
	mutex_enter(&ipflow_lock);

	ipflow_reap(false);

	mutex_exit(&ipflow_lock);
	SOFTNET_KERNEL_UNLOCK_UNLESS_NET_MPSAFE();

	return (0);
}

static int
sysctl_net_inet_ip_hashsize(SYSCTLFN_ARGS)
{
	int error, tmp;
	struct sysctlnode node;

	node = *rnode;
	tmp = ip_hashsize;
	node.sysctl_data = &tmp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	if ((tmp & (tmp - 1)) == 0 && tmp != 0) {
		/*
		 * Can only fail due to malloc()
		 */
		SOFTNET_KERNEL_LOCK_UNLESS_NET_MPSAFE();
		error = ipflow_invalidate_all(tmp);
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
ipflow_sysctl_init(struct sysctllog **clog)
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet",
		       SYSCTL_DESCR("PF_INET related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ip",
		       SYSCTL_DESCR("IPv4 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "maxflows",
		       SYSCTL_DESCR("Number of flows for fast forwarding"),
		       sysctl_net_inet_ip_maxflows, 0, &ip_maxflows, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_MAXFLOWS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
			CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
			CTLTYPE_INT, "hashsize",
			SYSCTL_DESCR("Size of hash table for fast forwarding (IPv4)"),
			sysctl_net_inet_ip_hashsize, 0, &ip_hashsize, 0,
			CTL_NET, PF_INET, IPPROTO_IP,
			CTL_CREATE, CTL_EOL);
}
