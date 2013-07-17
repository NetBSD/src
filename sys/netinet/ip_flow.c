/*	$NetBSD: ip_flow.c,v 1.60.10.1 2013/07/17 03:16:31 rmind Exp $	*/

/*-
 * Copyright (c) 1998, 2013 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: ip_flow.c,v 1.60.10.1 2013/07/17 03:16:31 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/cpu.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/kmem.h>
#include <sys/pool.h>
#include <sys/sysctl.h>

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

struct ipflow {
	kmutex_t ipf_lock;		/* lock protecting the fields */
	LIST_ENTRY(ipflow) ipf_list;	/* next in active list */
	LIST_ENTRY(ipflow) ipf_hash;	/* next ipflow in bucket */
	struct in_addr ipf_dst;		/* destination address */
	struct in_addr ipf_src;		/* source address */
	uint8_t ipf_tos;		/* type-of-service */
	struct route ipf_ro;		/* associated route entry */
	u_long ipf_uses;		/* number of uses in this period */
	u_long ipf_last_uses;		/* number of uses in last period */
	u_long ipf_dropped;		/* ENOBUFS retured by if_output */
	u_long ipf_errors;		/* other errors returned by if_output */
	u_int ipf_timer;		/* lifetime timer */
};

#define	IPFLOW_HASHBITS		6	/* should not be a multiple of 8 */

static pool_cache_t		ipflow_cache;
static struct sysctllog *	ipflow_sysctl_log;

LIST_HEAD(ipflowhead, ipflow);

#define	IPFLOW_TIMER		(5 * PR_SLOWHZ)
#define	IPFLOW_DEFAULT_HASHSIZE	(1 << IPFLOW_HASHBITS)

/*
 * IP flow hash table, a list, and the number of entries.
 * All are protected by ipflow_lock.  TODO: Consider RW-lock.
 */
static struct ipflowhead *	ipflowtable = NULL;
static struct ipflowhead	ipflowlist;
static kmutex_t			ipflow_lock;
static u_int			ipflow_inuse;

#ifndef IPFLOW_MAX
#define	IPFLOW_MAX		256
#endif

static int			ip_maxflows = IPFLOW_MAX;
static int			ip_hashsize = IPFLOW_DEFAULT_HASHSIZE;

static void			ipflow_sysctl_init(void);

void
ipflow_poolinit(void)
{
	ipflow_cache = pool_cache_init(sizeof(struct ipflow), coherency_unit,
	    0, 0, "ipflow", NULL, IPL_SOFTNET, NULL, NULL, NULL);
}

static bool
ipflow_reinit(int table_size, bool waitok, struct ipflowhead *gclist)
{
	struct ipflowhead *new_table, *old_table;
	size_t old_size, i;

	new_table = kmem_alloc(sizeof(struct ipflowhead) * table_size,
	    waitok ? KM_SLEEP : KM_NOSLEEP);
	if (new_table == NULL) {
		return false;
	}
	for (i = 0; i < table_size; i++) {
		LIST_INIT(&new_table[i]);
	}

	mutex_enter(&ipflow_lock);
	old_table = ipflowtable;
	old_size = ip_hashsize;

	ipflowtable = new_table;
	ip_hashsize = table_size;

	if (!gclist) {
		KASSERT(old_table == NULL);
		LIST_INIT(&ipflowlist);
	} else {
		LIST_CONCAT(gclist, &ipflowlist);
	}
	mutex_exit(&ipflow_lock);

	if (old_table) {
		kmem_free(old_table, sizeof(struct ipflowhead) * old_size);
	}
	return true;
}

void
ipflow_init(void)
{
	mutex_init(&ipflow_lock, MUTEX_DEFAULT, IPL_SOFTNET);
	(void)ipflow_reinit(ip_hashsize, true, NULL);
	ipflow_sysctl_init();
}

static size_t
ipflow_hash(const struct ip *ip)
{
	size_t idx, hash = ip->ip_tos;

	for (idx = 0; idx < 32; idx += IPFLOW_HASHBITS) {
		hash += (ip->ip_dst.s_addr >> (32 - idx)) +
		    (ip->ip_src.s_addr >> idx);
	}
	return hash & (ip_hashsize - 1);
}

/*
 * ipflow_lookup: search for a flow entry in the hash table.
 *
 * => Acquires the flow lock, if entry is found.
 */
static struct ipflow *
ipflow_lookup(const struct ip *ip)
{
	size_t hash = ipflow_hash(ip);
	struct ipflow *ipf;

	mutex_enter(&ipflow_lock);
	LIST_FOREACH(ipf, &ipflowtable[hash], ipf_hash) {
		if (ip->ip_dst.s_addr == ipf->ipf_dst.s_addr &&
		    ip->ip_src.s_addr == ipf->ipf_src.s_addr &&
		    ip->ip_tos == ipf->ipf_tos) {
			mutex_enter(&ipf->ipf_lock);
			break;
		}
	}
	mutex_exit(&ipflow_lock);
	return ipf;
}

/*
 * Main routine performing fast-forward.
 *
 * => Returns true if the packet was forwarded and false otherwise.
 */
bool
ipflow_fastforward(struct mbuf *m)
{
	struct ip *ip, ip_store;
	struct ipflow *ipf;
	struct rtentry *rt;
	const struct sockaddr *dst;
	int error, iplen;

	/* Are we forwarding packets?  Pre-check without lock held. */
	if (!ipforwarding || ipflow_inuse == 0)
		return false;

	/* Big enough for an IP packet? */
	if (m->m_len < sizeof(struct ip))
		return false;

	/*
	 * Was packet received as a link-level multicast or broadcast?
	 * If so, don't try to fast forward..
	 */
	if ((m->m_flags & (M_BCAST|M_MCAST)) != 0) {
		return false;
	}

	/*
	 * Check for IP header with no option, invalid version or length.
	 */
	if (IP_HDR_ALIGNED_P(mtod(m, const void *))) {
		ip = mtod(m, struct ip *);
	} else {
		memcpy(&ip_store, mtod(m, const void *), sizeof(ip_store));
		ip = &ip_store;
	}
	iplen = ntohs(ip->ip_len);
	if (ip->ip_v != IPVERSION || ip->ip_hl != (sizeof(struct ip) >> 2) ||
	    iplen < sizeof(struct ip) || iplen > m->m_pkthdr.len) {
		return false;
	}

	/*
	 * Verify the IP header checksum.
	 */
	switch (m->m_pkthdr.csum_flags &
		((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_IPv4) |
		 M_CSUM_IPv4_BAD)) {
	case M_CSUM_IPv4|M_CSUM_IPv4_BAD:
		return false;

	case M_CSUM_IPv4:
		/* Checksum was okay. */
		break;

	default:
		/* Must compute it ourselves. */
		if (in_cksum(m, sizeof(struct ip))) {
			return false;
		}
		break;
	}

	/*
	 * Find a flow (acquires the lock if found).
	 */
	if ((ipf = ipflow_lookup(ip)) == NULL) {
		return false;
	}
	KASSERT(mutex_owned(&ipf->ipf_lock));

	/*
	 * Route and interface still up?
	 */
	if ((rt = rtcache_validate(&ipf->ipf_ro)) == NULL ||
	    (rt->rt_ifp->if_flags & IFF_UP) == 0) {
		mutex_exit(&ipf->ipf_lock);
		return false;
	}

	/*
	 * Packet size OK?  TTL?
	 */
	if (m->m_pkthdr.len > rt->rt_ifp->if_mtu || ip->ip_ttl <= IPTTLDEC) {
		mutex_exit(&ipf->ipf_lock);
		return false;
	}

	ipf->ipf_uses++;
	PRT_SLOW_ARM(ipf->ipf_timer, IPFLOW_TIMER);

	if (rt->rt_flags & RTF_GATEWAY) {
		dst = rt->rt_gateway;
	} else {
		dst = rtcache_getdst(&ipf->ipf_ro);
	}
	mutex_exit(&ipf->ipf_lock);

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
	if (ip->ip_sum >= (uint16_t)~htons(IPTTLDEC << 8)) {
		ip->ip_sum -= ~htons(IPTTLDEC << 8);
	} else {
		ip->ip_sum += htons(IPTTLDEC << 8);
	}

	/*
	 * Done modifying the header; copy it back, if necessary.
	 *
	 * XXX Use m_copyback_cow(9) here? --dyoung
	 */
	if (IP_HDR_ALIGNED_P(mtod(m, void *)) == 0) {
		memcpy(mtod(m, void *), &ip_store, sizeof(ip_store));
	}

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
	 * Send the packet on it's way.  All we can get back is ENOBUFS.
	 */
	KERNEL_LOCK(1, NULL);
	if ((error = (*rt->rt_ifp->if_output)(rt->rt_ifp, m, dst, rt)) != 0) {
		/* FIMXErmind */
		if (error == ENOBUFS)
			ipf->ipf_dropped++;
		else
			ipf->ipf_errors++;
	}
	KERNEL_UNLOCK_ONE(NULL);
	return true;
}

static void
ipflow_addstats(struct ipflow *ipf)
{
	struct rtentry *rt;
	uint64_t *ips;

	if ((rt = rtcache_validate(&ipf->ipf_ro)) != NULL)
		rt->rt_use += ipf->ipf_uses;

	ips = IP_STAT_GETREF();
	ips[IP_STAT_CANTFORWARD] += ipf->ipf_errors + ipf->ipf_dropped;
	ips[IP_STAT_TOTAL] += ipf->ipf_uses;
	ips[IP_STAT_FORWARD] += ipf->ipf_uses;
	ips[IP_STAT_FASTFORWARD] += ipf->ipf_uses;
	IP_STAT_PUTREF();
}

static void
ipflow_insert(size_t hash, struct ipflow *ipf)
{
	KASSERT(mutex_owned(&ipflow_lock));
	LIST_INSERT_HEAD(&ipflowtable[hash], ipf, ipf_hash);
	LIST_INSERT_HEAD(&ipflowlist, ipf, ipf_list);
	ipflow_inuse++;
}

static void
ipflow_remove(struct ipflow *ipf)
{
	KASSERT(mutex_owned(&ipflow_lock));
	LIST_REMOVE(ipf, ipf_hash);
	LIST_REMOVE(ipf, ipf_list);
	ipflow_inuse--;
	ipflow_addstats(ipf);
}

static void
ipflow_free(struct ipflow *ipf)
{
	rtcache_free(&ipf->ipf_ro);
	pool_cache_put(ipflow_cache, ipf);
}

static struct ipflow *
ipflow_reap(bool just_one)
{
	struct ipflowhead ipf_gclist;
	struct ipflow *ipf;

	LIST_INIT(&ipf_gclist);
	mutex_enter(&ipflow_lock);
	while (just_one || ipflow_inuse > ip_maxflows) {
		struct ipflow *maybe_ipf = NULL;

		ipf = LIST_FIRST(&ipflowlist);
		while (ipf != NULL) {
			/*
			 * If this no longer points to a valid route
			 * reclaim it.
			 */
			if (rtcache_validate(&ipf->ipf_ro) == NULL) {
				goto found;
			}

			/*
			 * Choose the one that has been least recently
			 * used or has had the least uses in the
			 * last 1.5 intervals.
			 */
			if (maybe_ipf == NULL ||
			    ipf->ipf_timer < maybe_ipf->ipf_timer ||
			    (ipf->ipf_timer == maybe_ipf->ipf_timer &&
			     ipf->ipf_last_uses + ipf->ipf_uses <
			         maybe_ipf->ipf_last_uses +
			         maybe_ipf->ipf_uses))
				maybe_ipf = ipf;
			ipf = LIST_NEXT(ipf, ipf_list);
		}
		ipf = maybe_ipf;
found:
		/*
		 * Remove the entry from the flow table.
		 */
		ipflow_remove(ipf);
		if (just_one) {
			/* Unlock, free the route cache and return. */
			mutex_exit(&ipflow_lock);
			rtcache_free(&ipf->ipf_ro);
			return ipf;
		}
		LIST_INSERT_HEAD(&ipf_gclist, ipf, ipf_list);
	}
	mutex_exit(&ipflow_lock);

	while ((ipf = LIST_FIRST(&ipf_gclist)) != NULL) {
		LIST_REMOVE(ipf, ipf_list);
		ipflow_free(ipf);
	}
	return NULL;
}

void
ipflow_slowtimo(void)
{
	struct ipflowhead ipf_gclist;
	struct ipflow *ipf, *next_ipf;

	if (!ipflow_inuse) {
		return;
	}
	LIST_INIT(&ipf_gclist);

	mutex_enter(&ipflow_lock);
	for (ipf = LIST_FIRST(&ipflowlist); ipf != NULL; ipf = next_ipf) {
		struct rtentry *rt;
		uint64_t *ips;

		/*
		 * Destroy if entry has expired or its route no longer valid.
		 */
		next_ipf = LIST_NEXT(ipf, ipf_list);
		if (PRT_SLOW_ISEXPIRED(ipf->ipf_timer) ||
		    (rt = rtcache_validate(&ipf->ipf_ro)) == NULL) {
			/* Move to destruction list. */
			ipflow_remove(ipf);
			LIST_INSERT_HEAD(&ipf_gclist, ipf, ipf_list);
			continue;
		}

		/* Lockless access - for statistics only. */
		ipf->ipf_last_uses = ipf->ipf_uses;
		rt->rt_use += ipf->ipf_uses;
		ips = IP_STAT_GETREF();
		ips[IP_STAT_TOTAL] += ipf->ipf_uses;
		ips[IP_STAT_FORWARD] += ipf->ipf_uses;
		ips[IP_STAT_FASTFORWARD] += ipf->ipf_uses;
		IP_STAT_PUTREF();
		ipf->ipf_uses = 0;
	}
	mutex_exit(&ipflow_lock);

	while ((ipf = LIST_FIRST(&ipf_gclist)) != NULL) {
		LIST_REMOVE(ipf, ipf_list);
		ipflow_free(ipf);
	}
}

void
ipflow_create(const struct route *ro, struct mbuf *m)
{
	const struct ip *const ip = mtod(m, const struct ip *);
	struct ipflow *ipf;
	size_t hash;

	/* Do not create cache entries for ICMP messages. */
	if (ip_maxflows == 0 || ip->ip_p == IPPROTO_ICMP) {
		return;
	}

	/*
	 * See if there is an existing flow.  If so, remove it from the list
	 * and free the old route.  If not, try to allocate a new one,
	 * unless we hit our limit.
	 *
	 * Note: if found, flow lock is acquired.
	 */
	ipf = ipflow_lookup(ip);
	if (ipf == NULL) {
		if (ipflow_inuse < ip_maxflows) {
			ipf = pool_cache_get(ipflow_cache, PR_NOWAIT);
			if (ipf == NULL)
				return;
		} else {
			ipf = ipflow_reap(true);
		}
		memset(ipf, 0, sizeof(*ipf));
	} else {
		KASSERT(mutex_owned(&ipf->ipf_lock));
		ipflow_remove(ipf);
		mutex_exit(&ipflow_lock);

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
	 * Insert into the appropriate bucket of the flow table.
	 */
	hash = ipflow_hash(ip);
	mutex_enter(&ipflow_lock);
	ipflow_insert(hash, ipf);
	mutex_exit(&ipflow_lock);
}

int
ipflow_invalidate_all(int new_size)
{
	struct ipflowhead ipf_gclist;
	struct ipflow *ipf;
	int error = 0;

	LIST_INIT(&ipf_gclist);

	if (new_size) {
		if (!ipflow_reinit(new_size, false, &ipf_gclist)) {
			return ENOMEM;
		}
	} else {
		mutex_enter(&ipflow_lock);
		LIST_CONCAT(&ipf_gclist, &ipflowlist);
		mutex_exit(&ipflow_lock);
	}

	while ((ipf = LIST_FIRST(&ipf_gclist)) != NULL) {
		LIST_REMOVE(ipf, ipf_list);
		ipflow_free(ipf);
	}
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
	if (error || newp == NULL) {
		return error;
	}
	mutex_enter(&ipflow_lock);
	(void)ipflow_reap(false);
	mutex_exit(&ipflow_lock);
	return 0;
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
	if (error || newp == NULL) {
		return error;
	}

	if (tmp && powerof2(tmp)) {
		error = ipflow_invalidate_all(tmp);
	} else {
		error = EINVAL;
	}
	return error;
}

static void
ipflow_sysctl_init(void)
{
	ipflow_sysctl_log = NULL;

	sysctl_createv(&ipflow_sysctl_log, 0, NULL, NULL, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "net", NULL, NULL, 0, NULL, 0, CTL_NET, CTL_EOL);
	sysctl_createv(&ipflow_sysctl_log, 0, NULL, NULL, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "inet", SYSCTL_DESCR("PF_INET related settings"),
	    NULL, 0, NULL, 0, CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(&ipflow_sysctl_log, 0, NULL, NULL, CTLFLAG_PERMANENT,
	    CTLTYPE_NODE, "ip", SYSCTL_DESCR("IPv4 related settings"), NULL,
	    0, NULL, 0, CTL_NET, PF_INET, IPPROTO_IP, CTL_EOL);

	sysctl_createv(&ipflow_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "maxflows",
	    SYSCTL_DESCR("Number of flows for fast forwarding"),
	    sysctl_net_inet_ip_maxflows, 0, &ip_maxflows, 0,
	    CTL_NET, PF_INET, IPPROTO_IP, IPCTL_MAXFLOWS, CTL_EOL);
	sysctl_createv(&ipflow_sysctl_log, 0, NULL, NULL,
	    CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
	    CTLTYPE_INT, "hashsize",
	    SYSCTL_DESCR("Size of hash table for fast forwarding (IPv4)"),
	    sysctl_net_inet_ip_hashsize, 0, &ip_hashsize, 0,
	    CTL_NET, PF_INET, IPPROTO_IP, CTL_CREATE, CTL_EOL);
}
