/*	$NetBSD: ip_flow.c,v 1.8 1999/01/24 12:57:38 mycroft Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/pool.h>

#include <vm/vm.h>
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

struct pool ipflow_pool;

LIST_HEAD(ipflowhead, ipflow);

#define	IPFLOW_TIMER		(5 * PR_SLOWHZ)
#define	IPFLOW_HASHSIZE		(1 << IPFLOW_HASHBITS)

static struct ipflowhead ipflowtable[IPFLOW_HASHSIZE];
static struct ipflowhead ipflowlist;
static int ipflow_inuse;

#define	IPFLOW_INSERT(bucket, ipf) \
do { \
	LIST_INSERT_HEAD((bucket), (ipf), ipf_hash); \
	LIST_INSERT_HEAD(&ipflowlist, (ipf), ipf_list); \
} while (0)

#define	IPFLOW_REMOVE(ipf) \
do { \
	LIST_REMOVE((ipf), ipf_hash); \
	LIST_REMOVE((ipf), ipf_list); \
} while (0)

#ifndef IPFLOW_MAX
#define	IPFLOW_MAX		256
#endif
int ip_maxflows = IPFLOW_MAX;

static unsigned
ipflow_hash(
	struct in_addr dst,
	struct in_addr src,
	unsigned tos)
{
	unsigned hash = tos;
	int idx;
	for (idx = 0; idx < 32; idx += IPFLOW_HASHBITS)
		hash += (dst.s_addr >> (32 - idx)) + (src.s_addr >> idx);
	return hash & (IPFLOW_HASHSIZE-1);
}

static struct ipflow *
ipflow_lookup(
	const struct ip *ip)
{
	unsigned hash;
	struct ipflow *ipf;

	hash = ipflow_hash(ip->ip_dst, ip->ip_src, ip->ip_tos);

	ipf = LIST_FIRST(&ipflowtable[hash]);
	while (ipf != NULL) {
		if (ip->ip_dst.s_addr == ipf->ipf_dst.s_addr
		    && ip->ip_src.s_addr == ipf->ipf_src.s_addr
		    && ip->ip_tos == ipf->ipf_tos)
			break;
		ipf = LIST_NEXT(ipf, ipf_hash);
	}
	return ipf;
}

void
ipflow_init()
{
	int i;

	pool_init(&ipflow_pool, sizeof(struct ipflow), 0, 0, 0, "ipflowpl",
	    0, NULL, NULL, M_IPFLOW);

	LIST_INIT(&ipflowlist);
	for (i = 0; i < IPFLOW_HASHSIZE; i++)
		LIST_INIT(&ipflowtable[i]);
}

int
ipflow_fastforward(
	struct mbuf *m)
{
	struct ip *ip;
	struct ipflow *ipf;
	struct rtentry *rt;
	int error;
	int iplen;

	/*
	 * Are we forwarding packets?  Big enough for an IP packet?
	 */
	if (!ipforwarding || ipflow_inuse == 0 || m->m_len < sizeof(struct ip))
		return 0;
	/*
	 * IP header with no option and valid version and length
	 */
	ip = mtod(m, struct ip *);
	iplen = ntohs(ip->ip_len);
	if (ip->ip_v != IPVERSION || ip->ip_hl != (sizeof(struct ip) >> 2) ||
	    iplen > m->m_pkthdr.len)
		return 0;
	/*
	 * Find a flow.
	 */
	if ((ipf = ipflow_lookup(ip)) == NULL)
		return 0;

	/*
	 * Veryify the IP header checksum.
	 */
	if (in_cksum(m, sizeof(struct ip)) != 0)
		return 0;

	/*
	 * Route and interface still up?
	 */
	rt = ipf->ipf_ro.ro_rt;
	if ((rt->rt_flags & RTF_UP) == 0 ||
	    (rt->rt_ifp->if_flags & IFF_UP) == 0)
		return 0;

	/*
	 * Packet size OK?  TTL?
	 */
	if (m->m_pkthdr.len > rt->rt_ifp->if_mtu || ip->ip_ttl <= IPTTLDEC)
		return 0;

	/*
	 * Everything checks out and so we can forward this packet.
	 * Modify the TTL and incrementally change the checksum.
	 * On little endian machine, the TTL is in LSB position 
	 * (so we can simply add) while on big-endian it's in the
	 * MSB position (so we have to do two calculation; the first
	 * is the add and second is to wrap the results into 17 bits,
	 * 16 bits and a carry).  
	 */
	ip->ip_ttl -= IPTTLDEC;
	if (ip->ip_sum >= 0xffff - htons(IPTTLDEC << 8))
		ip->ip_sum += htons(IPTTLDEC << 8) + 1;
	else
		ip->ip_sum += htons(IPTTLDEC << 8);

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
	 * Send the packet on it's way.  All we can get back is ENOBUFS
	 */
	ipf->ipf_uses++;
	PRT_SLOW_ARM(ipf->ipf_timer, IPFLOW_TIMER);
	if ((error = (*rt->rt_ifp->if_output)(rt->rt_ifp, m,
	    &ipf->ipf_ro.ro_dst, rt)) != 0) {
		if (error == ENOBUFS)
			ipf->ipf_dropped++;
		else
			ipf->ipf_errors++;
	}
	return 1;
}

static void
ipflow_addstats(
	struct ipflow *ipf)
{
	ipf->ipf_ro.ro_rt->rt_use += ipf->ipf_uses;
	ipstat.ips_cantforward += ipf->ipf_errors + ipf->ipf_dropped;
	ipstat.ips_forward += ipf->ipf_uses;
	ipstat.ips_fastforward += ipf->ipf_uses;
}

static void
ipflow_free(
	struct ipflow *ipf)
{
	int s;
	/*
	 * Remove the flow from the hash table (at elevated IPL).
	 * Once it's off the list, we can deal with it at normal
	 * network IPL.
	 */
	s = splimp();
	IPFLOW_REMOVE(ipf);
	splx(s);
	ipflow_addstats(ipf);
	RTFREE(ipf->ipf_ro.ro_rt);
	ipflow_inuse--;
	pool_put(&ipflow_pool, ipf);
}

struct ipflow *
ipflow_reap(
	int just_one)
{
	while (just_one || ipflow_inuse > ip_maxflows) {
		struct ipflow *ipf, *maybe_ipf = NULL;
		int s;

		ipf = LIST_FIRST(&ipflowlist);
		while (ipf != NULL) {
			/*
			 * If this no longer points to a valid route
			 * reclaim it.
			 */
			if ((ipf->ipf_ro.ro_rt->rt_flags & RTF_UP) == 0)
				goto done;
			/*
			 * choose the one that's been least recently
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
	    done:
		/*
		 * Remove the entry from the flow table.
		 */
		s = splimp();
		IPFLOW_REMOVE(ipf);
		splx(s);
		ipflow_addstats(ipf);
		RTFREE(ipf->ipf_ro.ro_rt);
		if (just_one)
			return ipf;
		pool_put(&ipflow_pool, ipf);
		ipflow_inuse--;
	}
	return NULL;
}

void
ipflow_slowtimo(
	void)
{
	struct ipflow *ipf, *next_ipf;

	ipf = LIST_FIRST(&ipflowlist);
	while (ipf != NULL) {
		next_ipf = LIST_NEXT(ipf, ipf_list);
		if (PRT_SLOW_ISEXPIRED(ipf->ipf_timer)) {
			ipflow_free(ipf);
		} else {
			ipf->ipf_last_uses = ipf->ipf_uses;
			ipf->ipf_ro.ro_rt->rt_use += ipf->ipf_uses;
			ipstat.ips_forward += ipf->ipf_uses;
			ipstat.ips_fastforward += ipf->ipf_uses;
			ipf->ipf_uses = 0;
		}
		ipf = next_ipf;
	}
}

void
ipflow_create(
	const struct route *ro,
	struct mbuf *m)
{
	const struct ip *const ip = mtod(m, struct ip *);
	struct ipflow *ipf;
	unsigned hash;
	int s;

	/*
	 * Don't create cache entries for ICMP messages.
	 */
	if (ip_maxflows == 0 || ip->ip_p == IPPROTO_ICMP)
		return;
	/*
	 * See if an existing flow struct exists.  If so remove it from it's
	 * list and free the old route.  If not, try to malloc a new one
	 * (if we aren't at our limit).
	 */
	ipf = ipflow_lookup(ip);
	if (ipf == NULL) {
		if (ipflow_inuse >= ip_maxflows) {
			ipf = ipflow_reap(1);
		} else {
			ipf = pool_get(&ipflow_pool, PR_NOWAIT);
			if (ipf == NULL)
				return;
			ipflow_inuse++;
		}
		bzero((caddr_t) ipf, sizeof(*ipf));
	} else {
		s = splimp();
		IPFLOW_REMOVE(ipf);
		splx(s);
		ipflow_addstats(ipf);
		RTFREE(ipf->ipf_ro.ro_rt);
		ipf->ipf_uses = ipf->ipf_last_uses = 0;
		ipf->ipf_errors = ipf->ipf_dropped = 0;
	}

	/*
	 * Fill in the updated information.
	 */
	ipf->ipf_ro = *ro;
	ro->ro_rt->rt_refcnt++;
	ipf->ipf_dst = ip->ip_dst;
	ipf->ipf_src = ip->ip_src;
	ipf->ipf_tos = ip->ip_tos;
	PRT_SLOW_ARM(ipf->ipf_timer, IPFLOW_TIMER);
	ipf->ipf_start = time.tv_sec;
	/*
	 * Insert into the approriate bucket of the flow table.
	 */
	hash = ipflow_hash(ip->ip_dst, ip->ip_src, ip->ip_tos);
	s = splimp();
	IPFLOW_INSERT(&ipflowtable[hash], ipf);
	splx(s);
}
