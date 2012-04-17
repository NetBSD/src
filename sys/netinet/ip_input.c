/*	$NetBSD: ip_input.c,v 1.296.2.1 2012/04/17 00:08:40 yamt Exp $	*/

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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
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
 * Copyright (c) 1982, 1986, 1988, 1993
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
 *	@(#)ip_input.c	8.2 (Berkeley) 1/4/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip_input.c,v 1.296.2.1 2012/04/17 00:08:40 yamt Exp $");

#include "opt_inet.h"
#include "opt_compat_netbsd.h"
#include "opt_gateway.h"
#include "opt_pfil_hooks.h"
#include "opt_ipsec.h"
#include "opt_mrouting.h"
#include "opt_mbuftrace.h"
#include "opt_inet_csum.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/pool.h>
#include <sys/sysctl.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/route.h>
#include <net/pfil.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>
#include <netinet/in_proto.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>
#include <netinet/ip_private.h>
#include <netinet/ip_icmp.h>
/* just for gif_ttl */
#include <netinet/in_gif.h>
#include "gif.h"
#include <net/if_gre.h>
#include "gre.h"

#ifdef MROUTING
#include <netinet/ip_mroute.h>
#endif

#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#endif	/* FAST_IPSEC*/

#ifndef	IPFORWARDING
#ifdef GATEWAY
#define	IPFORWARDING	1	/* forward IP packets not for us */
#else /* GATEWAY */
#define	IPFORWARDING	0	/* don't forward IP packets not for us */
#endif /* GATEWAY */
#endif /* IPFORWARDING */
#ifndef	IPSENDREDIRECTS
#define	IPSENDREDIRECTS	1
#endif
#ifndef IPFORWSRCRT
#define	IPFORWSRCRT	1	/* forward source-routed packets */
#endif
#ifndef IPALLOWSRCRT
#define	IPALLOWSRCRT	1	/* allow source-routed packets */
#endif
#ifndef IPMTUDISC
#define IPMTUDISC	1
#endif
#ifndef IPMTUDISCTIMEOUT
#define IPMTUDISCTIMEOUT (10 * 60)	/* as per RFC 1191 */
#endif

#ifdef COMPAT_50
#include <compat/sys/time.h>
#include <compat/sys/socket.h>
#endif

/*
 * Note: DIRECTED_BROADCAST is handled this way so that previous
 * configuration using this option will Just Work.
 */
#ifndef IPDIRECTEDBCAST
#ifdef DIRECTED_BROADCAST
#define IPDIRECTEDBCAST	1
#else
#define	IPDIRECTEDBCAST	0
#endif /* DIRECTED_BROADCAST */
#endif /* IPDIRECTEDBCAST */
int	ipforwarding = IPFORWARDING;
int	ipsendredirects = IPSENDREDIRECTS;
int	ip_defttl = IPDEFTTL;
int	ip_forwsrcrt = IPFORWSRCRT;
int	ip_directedbcast = IPDIRECTEDBCAST;
int	ip_allowsrcrt = IPALLOWSRCRT;
int	ip_mtudisc = IPMTUDISC;
int	ip_mtudisc_timeout = IPMTUDISCTIMEOUT;
#ifdef DIAGNOSTIC
int	ipprintfs = 0;
#endif

int	ip_do_randomid = 0;

/*
 * XXX - Setting ip_checkinterface mostly implements the receive side of
 * the Strong ES model described in RFC 1122, but since the routing table
 * and transmit implementation do not implement the Strong ES model,
 * setting this to 1 results in an odd hybrid.
 *
 * XXX - ip_checkinterface currently must be disabled if you use ipnat
 * to translate the destination address to another local interface.
 *
 * XXX - ip_checkinterface must be disabled if you add IP aliases
 * to the loopback interface instead of the interface where the
 * packets for those addresses are received.
 */
int	ip_checkinterface = 0;


struct rttimer_queue *ip_mtudisc_timeout_q = NULL;

u_long	in_ifaddrhash;				/* size of hash table - 1 */
int	in_ifaddrentries;			/* total number of addrs */
struct in_ifaddrhead in_ifaddrhead;
struct	in_ifaddrhashhead *in_ifaddrhashtbl;
u_long	in_multihash;				/* size of hash table - 1 */
int	in_multientries;			/* total number of addrs */
struct	in_multihashhead *in_multihashtbl;
struct	ifqueue ipintrq;

ipid_state_t *		ip_ids;
uint16_t ip_id;

percpu_t *ipstat_percpu;

#ifdef PFIL_HOOKS
struct pfil_head inet_pfil_hook;
#endif

struct pool inmulti_pool;

#ifdef INET_CSUM_COUNTERS
#include <sys/device.h>

struct evcnt ip_hwcsum_bad = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "inet", "hwcsum bad");
struct evcnt ip_hwcsum_ok = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "inet", "hwcsum ok");
struct evcnt ip_swcsum = EVCNT_INITIALIZER(EVCNT_TYPE_MISC,
    NULL, "inet", "swcsum");

#define	INET_CSUM_COUNTER_INCR(ev)	(ev)->ev_count++

EVCNT_ATTACH_STATIC(ip_hwcsum_bad);
EVCNT_ATTACH_STATIC(ip_hwcsum_ok);
EVCNT_ATTACH_STATIC(ip_swcsum);

#else

#define	INET_CSUM_COUNTER_INCR(ev)	/* nothing */

#endif /* INET_CSUM_COUNTERS */

/*
 * We need to save the IP options in case a protocol wants to respond
 * to an incoming packet over the same route if the packet got here
 * using IP source routing.  This allows connection establishment and
 * maintenance when the remote end is on a network that is not known
 * to us.
 */
int	ip_nhops = 0;
static	struct ip_srcrt {
	struct	in_addr dst;			/* final destination */
	char	nop;				/* one NOP to align */
	char	srcopt[IPOPT_OFFSET + 1];	/* OPTVAL, OLEN and OFFSET */
	struct	in_addr route[MAX_IPOPTLEN/sizeof(struct in_addr)];
} ip_srcrt;

static int ip_drainwanted;

static void save_rte(u_char *, struct in_addr);

#ifdef MBUFTRACE
struct mowner ip_rx_mowner = MOWNER_INIT("internet", "rx");
struct mowner ip_tx_mowner = MOWNER_INIT("internet", "tx");
#endif

static void sysctl_net_inet_ip_setup(struct sysctllog **);

/*
 * IP initialization: fill in IP protocol switch table.
 * All protocols not implemented in kernel go to raw IP protocol handler.
 */
void
ip_init(void)
{
	const struct protosw *pr;
	int i;

	sysctl_net_inet_ip_setup(NULL);

	pool_init(&inmulti_pool, sizeof(struct in_multi), 0, 0, 0, "inmltpl",
	    NULL, IPL_SOFTNET);

	pr = pffindproto(PF_INET, IPPROTO_RAW, SOCK_RAW);
	if (pr == 0)
		panic("ip_init");
	for (i = 0; i < IPPROTO_MAX; i++)
		ip_protox[i] = pr - inetsw;
	for (pr = inetdomain.dom_protosw;
	    pr < inetdomain.dom_protoswNPROTOSW; pr++)
		if (pr->pr_domain->dom_family == PF_INET &&
		    pr->pr_protocol && pr->pr_protocol != IPPROTO_RAW)
			ip_protox[pr->pr_protocol] = pr - inetsw;

	ip_reass_init();

	ip_ids = ip_id_init();
	ip_id = time_second & 0xfffff;

	ipintrq.ifq_maxlen = IFQ_MAXLEN;

	TAILQ_INIT(&in_ifaddrhead);
	in_ifaddrhashtbl = hashinit(IN_IFADDR_HASH_SIZE, HASH_LIST, true,
	    &in_ifaddrhash);
	in_multihashtbl = hashinit(IN_IFADDR_HASH_SIZE, HASH_LIST, true,
	    &in_multihash);
	ip_mtudisc_timeout_q = rt_timer_queue_create(ip_mtudisc_timeout);
#ifdef GATEWAY
	ipflow_init(ip_hashsize);
#endif

#ifdef PFIL_HOOKS
	/* Register our Packet Filter hook. */
	inet_pfil_hook.ph_type = PFIL_TYPE_AF;
	inet_pfil_hook.ph_af   = AF_INET;
	i = pfil_head_register(&inet_pfil_hook);
	if (i != 0)
		printf("ip_init: WARNING: unable to register pfil hook, "
		    "error %d\n", i);
#endif /* PFIL_HOOKS */

#ifdef MBUFTRACE
	MOWNER_ATTACH(&ip_tx_mowner);
	MOWNER_ATTACH(&ip_rx_mowner);
#endif /* MBUFTRACE */

	ipstat_percpu = percpu_alloc(sizeof(uint64_t) * IP_NSTATS);
}

struct	sockaddr_in ipaddr = {
	.sin_len = sizeof(ipaddr),
	.sin_family = AF_INET,
};
struct	route ipforward_rt;

/*
 * IP software interrupt routine
 */
void
ipintr(void)
{
	int s;
	struct mbuf *m;
	struct ifqueue lcl_intrq;

	memset(&lcl_intrq, 0, sizeof(lcl_intrq));

	mutex_enter(softnet_lock);
	KERNEL_LOCK(1, NULL);
	if (!IF_IS_EMPTY(&ipintrq)) {
		s = splnet();

		/* Take existing queue onto stack */
		lcl_intrq = ipintrq;

		/* Zero out global queue, preserving maxlen and drops */
		ipintrq.ifq_head = NULL;
		ipintrq.ifq_tail = NULL;
		ipintrq.ifq_len = 0;
		ipintrq.ifq_maxlen = lcl_intrq.ifq_maxlen;
		ipintrq.ifq_drops = lcl_intrq.ifq_drops;

		splx(s);
	}
	KERNEL_UNLOCK_ONE(NULL);
	while (!IF_IS_EMPTY(&lcl_intrq)) {
		IF_DEQUEUE(&lcl_intrq, m);
		if (m == NULL)
			break;
		ip_input(m);
	}
	mutex_exit(softnet_lock);
}

/*
 * Ip input routine.  Checksum and byte swap header.  If fragmented
 * try to reassemble.  Process options.  Pass to next level.
 */
void
ip_input(struct mbuf *m)
{
	struct ip *ip = NULL;
	struct in_ifaddr *ia;
	struct ifaddr *ifa;
	int hlen = 0, len;
	int downmatch;
	int checkif;
	int srcrt = 0;
#ifdef FAST_IPSEC
	struct m_tag *mtag;
	struct tdb_ident *tdbi;
	struct secpolicy *sp;
	int error, s;
#endif /* FAST_IPSEC */

	MCLAIM(m, &ip_rx_mowner);
	KASSERT((m->m_flags & M_PKTHDR) != 0);

	/*
	 * If no IP addresses have been set yet but the interfaces
	 * are receiving, can't do anything with incoming packets yet.
	 */
	if (TAILQ_FIRST(&in_ifaddrhead) == 0)
		goto bad;
	IP_STATINC(IP_STAT_TOTAL);
	/*
	 * If the IP header is not aligned, slurp it up into a new
	 * mbuf with space for link headers, in the event we forward
	 * it.  Otherwise, if it is aligned, make sure the entire
	 * base IP header is in the first mbuf of the chain.
	 */
	if (IP_HDR_ALIGNED_P(mtod(m, void *)) == 0) {
		if ((m = m_copyup(m, sizeof(struct ip),
				  (max_linkhdr + 3) & ~3)) == NULL) {
			/* XXXJRT new stat, please */
			IP_STATINC(IP_STAT_TOOSMALL);
			return;
		}
	} else if (__predict_false(m->m_len < sizeof (struct ip))) {
		if ((m = m_pullup(m, sizeof (struct ip))) == NULL) {
			IP_STATINC(IP_STAT_TOOSMALL);
			return;
		}
	}
	ip = mtod(m, struct ip *);
	if (ip->ip_v != IPVERSION) {
		IP_STATINC(IP_STAT_BADVERS);
		goto bad;
	}
	hlen = ip->ip_hl << 2;
	if (hlen < sizeof(struct ip)) {	/* minimum header length */
		IP_STATINC(IP_STAT_BADHLEN);
		goto bad;
	}
	if (hlen > m->m_len) {
		if ((m = m_pullup(m, hlen)) == NULL) {
			IP_STATINC(IP_STAT_BADHLEN);
			return;
		}
		ip = mtod(m, struct ip *);
	}

	/*
	 * RFC1122: packets with a multicast source address are
	 * not allowed.
	 */
	if (IN_MULTICAST(ip->ip_src.s_addr)) {
		IP_STATINC(IP_STAT_BADADDR);
		goto bad;
	}

	/* 127/8 must not appear on wire - RFC1122 */
	if ((ntohl(ip->ip_dst.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET ||
	    (ntohl(ip->ip_src.s_addr) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET) {
		if ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) == 0) {
			IP_STATINC(IP_STAT_BADADDR);
			goto bad;
		}
	}

	switch (m->m_pkthdr.csum_flags &
		((m->m_pkthdr.rcvif->if_csum_flags_rx & M_CSUM_IPv4) |
		 M_CSUM_IPv4_BAD)) {
	case M_CSUM_IPv4|M_CSUM_IPv4_BAD:
		INET_CSUM_COUNTER_INCR(&ip_hwcsum_bad);
		goto badcsum;

	case M_CSUM_IPv4:
		/* Checksum was okay. */
		INET_CSUM_COUNTER_INCR(&ip_hwcsum_ok);
		break;

	default:
		/*
		 * Must compute it ourselves.  Maybe skip checksum on
		 * loopback interfaces.
		 */
		if (__predict_true(!(m->m_pkthdr.rcvif->if_flags &
				     IFF_LOOPBACK) || ip_do_loopback_cksum)) {
			INET_CSUM_COUNTER_INCR(&ip_swcsum);
			if (in_cksum(m, hlen) != 0)
				goto badcsum;
		}
		break;
	}

	/* Retrieve the packet length. */
	len = ntohs(ip->ip_len);

	/*
	 * Check for additional length bogosity
	 */
	if (len < hlen) {
		IP_STATINC(IP_STAT_BADLEN);
		goto bad;
	}

	/*
	 * Check that the amount of data in the buffers
	 * is as at least much as the IP header would have us expect.
	 * Trim mbufs if longer than we expect.
	 * Drop packet if shorter than we expect.
	 */
	if (m->m_pkthdr.len < len) {
		IP_STATINC(IP_STAT_TOOSHORT);
		goto bad;
	}
	if (m->m_pkthdr.len > len) {
		if (m->m_len == m->m_pkthdr.len) {
			m->m_len = len;
			m->m_pkthdr.len = len;
		} else
			m_adj(m, len - m->m_pkthdr.len);
	}

	/*
	 * Assume that we can create a fast-forward IP flow entry
	 * based on this packet.
	 */
	m->m_flags |= M_CANFASTFWD;

#ifdef PFIL_HOOKS
	/*
	 * Run through list of hooks for input packets.  If there are any
	 * filters which require that additional packets in the flow are
	 * not fast-forwarded, they must clear the M_CANFASTFWD flag.
	 * Note that filters must _never_ set this flag, as another filter
	 * in the list may have previously cleared it.
	 */
	/*
	 * let ipfilter look at packet on the wire,
	 * not the decapsulated packet.
	 */
#if defined(FAST_IPSEC)
	if (!ipsec_indone(m))
#else
	if (1)
#endif
	{
		struct in_addr odst;

		odst = ip->ip_dst;
		if (pfil_run_hooks(&inet_pfil_hook, &m, m->m_pkthdr.rcvif,
		    PFIL_IN) != 0)
			return;
		if (m == NULL)
			return;
		ip = mtod(m, struct ip *);
		hlen = ip->ip_hl << 2;
		/*
		 * XXX The setting of "srcrt" here is to prevent ip_forward()
		 * from generating ICMP redirects for packets that have
		 * been redirected by a hook back out on to the same LAN that
		 * they came from and is not an indication that the packet
		 * is being inffluenced by source routing options.  This
		 * allows things like
		 * "rdr tlp0 0/0 port 80 -> 1.1.1.200 3128 tcp"
		 * where tlp0 is both on the 1.1.1.0/24 network and is the
		 * default route for hosts on 1.1.1.0/24.  Of course this
		 * also requires a "map tlp0 ..." to complete the story.
		 * One might argue whether or not this kind of network config.
		 * should be supported in this manner...
		 */
		srcrt = (odst.s_addr != ip->ip_dst.s_addr);
	}
#endif /* PFIL_HOOKS */

#ifdef ALTQ
	/* XXX Temporary until ALTQ is changed to use a pfil hook */
	if (altq_input != NULL && (*altq_input)(m, AF_INET) == 0) {
		/* packet dropped by traffic conditioner */
		return;
	}
#endif

	/*
	 * Process options and, if not destined for us,
	 * ship it on.  ip_dooptions returns 1 when an
	 * error was detected (causing an icmp message
	 * to be sent and the original packet to be freed).
	 */
	ip_nhops = 0;		/* for source routed packets */
	if (hlen > sizeof (struct ip) && ip_dooptions(m))
		return;

	/*
	 * Enable a consistency check between the destination address
	 * and the arrival interface for a unicast packet (the RFC 1122
	 * strong ES model) if IP forwarding is disabled and the packet
	 * is not locally generated.
	 *
	 * XXX - Checking also should be disabled if the destination
	 * address is ipnat'ed to a different interface.
	 *
	 * XXX - Checking is incompatible with IP aliases added
	 * to the loopback interface instead of the interface where
	 * the packets are received.
	 *
	 * XXX - We need to add a per ifaddr flag for this so that
	 * we get finer grain control.
	 */
	checkif = ip_checkinterface && (ipforwarding == 0) &&
	    (m->m_pkthdr.rcvif != NULL) &&
	    ((m->m_pkthdr.rcvif->if_flags & IFF_LOOPBACK) == 0);

	/*
	 * Check our list of addresses, to see if the packet is for us.
	 *
	 * Traditional 4.4BSD did not consult IFF_UP at all.
	 * The behavior here is to treat addresses on !IFF_UP interface
	 * as not mine.
	 */
	downmatch = 0;
	LIST_FOREACH(ia, &IN_IFADDR_HASH(ip->ip_dst.s_addr), ia_hash) {
		if (in_hosteq(ia->ia_addr.sin_addr, ip->ip_dst)) {
			if (checkif && ia->ia_ifp != m->m_pkthdr.rcvif)
				continue;
			if ((ia->ia_ifp->if_flags & IFF_UP) != 0)
				break;
			else
				downmatch++;
		}
	}
	if (ia != NULL)
		goto ours;
	if (m->m_pkthdr.rcvif && m->m_pkthdr.rcvif->if_flags & IFF_BROADCAST) {
		IFADDR_FOREACH(ifa, m->m_pkthdr.rcvif) {
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;
			ia = ifatoia(ifa);
			if (in_hosteq(ip->ip_dst, ia->ia_broadaddr.sin_addr) ||
			    in_hosteq(ip->ip_dst, ia->ia_netbroadcast) ||
			    /*
			     * Look for all-0's host part (old broadcast addr),
			     * either for subnet or net.
			     */
			    ip->ip_dst.s_addr == ia->ia_subnet ||
			    ip->ip_dst.s_addr == ia->ia_net)
				goto ours;
			/*
			 * An interface with IP address zero accepts
			 * all packets that arrive on that interface.
			 */
			if (in_nullhost(ia->ia_addr.sin_addr))
				goto ours;
		}
	}
	if (IN_MULTICAST(ip->ip_dst.s_addr)) {
		struct in_multi *inm;
#ifdef MROUTING
		extern struct socket *ip_mrouter;

		if (ip_mrouter) {
			/*
			 * If we are acting as a multicast router, all
			 * incoming multicast packets are passed to the
			 * kernel-level multicast forwarding function.
			 * The packet is returned (relatively) intact; if
			 * ip_mforward() returns a non-zero value, the packet
			 * must be discarded, else it may be accepted below.
			 *
			 * (The IP ident field is put in the same byte order
			 * as expected when ip_mforward() is called from
			 * ip_output().)
			 */
			if (ip_mforward(m, m->m_pkthdr.rcvif) != 0) {
				IP_STATINC(IP_STAT_CANTFORWARD);
				m_freem(m);
				return;
			}

			/*
			 * The process-level routing demon needs to receive
			 * all multicast IGMP packets, whether or not this
			 * host belongs to their destination groups.
			 */
			if (ip->ip_p == IPPROTO_IGMP)
				goto ours;
			IP_STATINC(IP_STAT_CANTFORWARD);
		}
#endif
		/*
		 * See if we belong to the destination multicast group on the
		 * arrival interface.
		 */
		IN_LOOKUP_MULTI(ip->ip_dst, m->m_pkthdr.rcvif, inm);
		if (inm == NULL) {
			IP_STATINC(IP_STAT_CANTFORWARD);
			m_freem(m);
			return;
		}
		goto ours;
	}
	if (ip->ip_dst.s_addr == INADDR_BROADCAST ||
	    in_nullhost(ip->ip_dst))
		goto ours;

	/*
	 * Not for us; forward if possible and desirable.
	 */
	if (ipforwarding == 0) {
		IP_STATINC(IP_STAT_CANTFORWARD);
		m_freem(m);
	} else {
		/*
		 * If ip_dst matched any of my address on !IFF_UP interface,
		 * and there's no IFF_UP interface that matches ip_dst,
		 * send icmp unreach.  Forwarding it will result in in-kernel
		 * forwarding loop till TTL goes to 0.
		 */
		if (downmatch) {
			icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_HOST, 0, 0);
			IP_STATINC(IP_STAT_CANTFORWARD);
			return;
		}
#ifdef FAST_IPSEC
		mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_DONE, NULL);
		s = splsoftnet();
		if (mtag != NULL) {
			tdbi = (struct tdb_ident *)(mtag + 1);
			sp = ipsec_getpolicy(tdbi, IPSEC_DIR_INBOUND);
		} else {
			sp = ipsec_getpolicybyaddr(m, IPSEC_DIR_INBOUND,
						   IP_FORWARDING, &error);
		}
		if (sp == NULL) {	/* NB: can happen if error */
			splx(s);
			/*XXX error stat???*/
			DPRINTF(("ip_input: no SP for forwarding\n"));	/*XXX*/
			goto bad;
		}

		/*
		 * Check security policy against packet attributes.
		 */
		error = ipsec_in_reject(sp, m);
		KEY_FREESP(&sp);
		splx(s);
		if (error) {
			IP_STATINC(IP_STAT_CANTFORWARD);
			goto bad;
		}

		/*
		 * Peek at the outbound SP for this packet to determine if
		 * it's a Fast Forward candidate.
		 */
		mtag = m_tag_find(m, PACKET_TAG_IPSEC_PENDING_TDB, NULL);
		if (mtag != NULL)
			m->m_flags &= ~M_CANFASTFWD;
		else {
			s = splsoftnet();
			sp = ipsec4_checkpolicy(m, IPSEC_DIR_OUTBOUND,
			    (IP_FORWARDING |
			     (ip_directedbcast ? IP_ALLOWBROADCAST : 0)),
			    &error, NULL);
			if (sp != NULL) {
				m->m_flags &= ~M_CANFASTFWD;
				KEY_FREESP(&sp);
			}
			splx(s);
		}
#endif	/* FAST_IPSEC */

		ip_forward(m, srcrt);
	}
	return;

ours:
	/*
	 * If offset or IP_MF are set, must reassemble.
	 */
	if (ip->ip_off & ~htons(IP_DF|IP_RF)) {
		/*
		 * Pass to IP reassembly mechanism.
		 */
		if (ip_reass_packet(&m, ip) != 0) {
			/* Failed; invalid fragment(s) or packet. */
			goto bad;
		}
		if (m == NULL) {
			/* More fragments should come; silently return. */
			return;
		}
		/*
		 * Reassembly is done, we have the final packet.
		 * Updated cached data in local variable(s).
		 */
		ip = mtod(m, struct ip *);
		hlen = ip->ip_hl << 2;
	}

#ifdef FAST_IPSEC
	/*
	 * enforce IPsec policy checking if we are seeing last header.
	 * note that we do not visit this with protocols with pcb layer
	 * code - like udp/tcp/raw ip.
	 */
	if ((inetsw[ip_protox[ip->ip_p]].pr_flags & PR_LASTHDR) != 0) {
		/*
		 * Check if the packet has already had IPsec processing
		 * done.  If so, then just pass it along.  This tag gets
		 * set during AH, ESP, etc. input handling, before the
		 * packet is returned to the ip input queue for delivery.
		 */
		mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_DONE, NULL);
		s = splsoftnet();
		if (mtag != NULL) {
			tdbi = (struct tdb_ident *)(mtag + 1);
			sp = ipsec_getpolicy(tdbi, IPSEC_DIR_INBOUND);
		} else {
			sp = ipsec_getpolicybyaddr(m, IPSEC_DIR_INBOUND,
						   IP_FORWARDING, &error);
		}
		if (sp != NULL) {
			/*
			 * Check security policy against packet attributes.
			 */
			error = ipsec_in_reject(sp, m);
			KEY_FREESP(&sp);
		} else {
			/* XXX error stat??? */
			error = EINVAL;
DPRINTF(("ip_input: no SP, packet discarded\n"));/*XXX*/
		}
		splx(s);
		if (error)
			goto bad;
	}
#endif /* FAST_IPSEC */

	/*
	 * Switch out to protocol's input routine.
	 */
#if IFA_STATS
	if (ia && ip)
		ia->ia_ifa.ifa_data.ifad_inbytes += ntohs(ip->ip_len);
#endif
	IP_STATINC(IP_STAT_DELIVERED);
    {
	int off = hlen, nh = ip->ip_p;

	(*inetsw[ip_protox[nh]].pr_input)(m, off, nh);
	return;
    }
bad:
	m_freem(m);
	return;

badcsum:
	IP_STATINC(IP_STAT_BADSUM);
	m_freem(m);
}

/*
 * IP timer processing.
 */
void
ip_slowtimo(void)
{

	mutex_enter(softnet_lock);
	KERNEL_LOCK(1, NULL);

	ip_reass_slowtimo();

	KERNEL_UNLOCK_ONE(NULL);
	mutex_exit(softnet_lock);
}

/*
 * IP drain processing.
 */
void
ip_drain(void)
{

	KERNEL_LOCK(1, NULL);
	ip_reass_drain();
	KERNEL_UNLOCK_ONE(NULL);
}

/*
 * Do option processing on a datagram,
 * possibly discarding it if bad options are encountered,
 * or forwarding it if source-routed.
 * Returns 1 if packet has been forwarded/freed,
 * 0 if the packet should be processed further.
 */
int
ip_dooptions(struct mbuf *m)
{
	struct ip *ip = mtod(m, struct ip *);
	u_char *cp, *cp0;
	struct ip_timestamp *ipt;
	struct in_ifaddr *ia;
	int opt, optlen, cnt, off, code, type = ICMP_PARAMPROB, forward = 0;
	struct in_addr dst;
	n_time ntime;

	dst = ip->ip_dst;
	cp = (u_char *)(ip + 1);
	cnt = (ip->ip_hl << 2) - sizeof (struct ip);
	for (; cnt > 0; cnt -= optlen, cp += optlen) {
		opt = cp[IPOPT_OPTVAL];
		if (opt == IPOPT_EOL)
			break;
		if (opt == IPOPT_NOP)
			optlen = 1;
		else {
			if (cnt < IPOPT_OLEN + sizeof(*cp)) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
			optlen = cp[IPOPT_OLEN];
			if (optlen < IPOPT_OLEN + sizeof(*cp) || optlen > cnt) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
		}
		switch (opt) {

		default:
			break;

		/*
		 * Source routing with record.
		 * Find interface with current destination address.
		 * If none on this machine then drop if strictly routed,
		 * or do nothing if loosely routed.
		 * Record interface address and bring up next address
		 * component.  If strictly routed make sure next
		 * address is on directly accessible net.
		 */
		case IPOPT_LSRR:
		case IPOPT_SSRR:
			if (ip_allowsrcrt == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_NET_PROHIB;
				goto bad;
			}
			if (optlen < IPOPT_OFFSET + sizeof(*cp)) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			ipaddr.sin_addr = ip->ip_dst;
			ia = ifatoia(ifa_ifwithaddr(sintosa(&ipaddr)));
			if (ia == 0) {
				if (opt == IPOPT_SSRR) {
					type = ICMP_UNREACH;
					code = ICMP_UNREACH_SRCFAIL;
					goto bad;
				}
				/*
				 * Loose routing, and not at next destination
				 * yet; nothing to do except forward.
				 */
				break;
			}
			off--;			/* 0 origin */
			if ((off + sizeof(struct in_addr)) > optlen) {
				/*
				 * End of source route.  Should be for us.
				 */
				save_rte(cp, ip->ip_src);
				break;
			}
			/*
			 * locate outgoing interface
			 */
			memcpy((void *)&ipaddr.sin_addr, (void *)(cp + off),
			    sizeof(ipaddr.sin_addr));
			if (opt == IPOPT_SSRR)
				ia = ifatoia(ifa_ifwithladdr(sintosa(&ipaddr)));
			else
				ia = ip_rtaddr(ipaddr.sin_addr);
			if (ia == 0) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_SRCFAIL;
				goto bad;
			}
			ip->ip_dst = ipaddr.sin_addr;
			bcopy((void *)&ia->ia_addr.sin_addr,
			    (void *)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			/*
			 * Let ip_intr's mcast routing check handle mcast pkts
			 */
			forward = !IN_MULTICAST(ip->ip_dst.s_addr);
			break;

		case IPOPT_RR:
			if (optlen < IPOPT_OFFSET + sizeof(*cp)) {
				code = &cp[IPOPT_OLEN] - (u_char *)ip;
				goto bad;
			}
			if ((off = cp[IPOPT_OFFSET]) < IPOPT_MINOFF) {
				code = &cp[IPOPT_OFFSET] - (u_char *)ip;
				goto bad;
			}
			/*
			 * If no space remains, ignore.
			 */
			off--;			/* 0 origin */
			if ((off + sizeof(struct in_addr)) > optlen)
				break;
			memcpy((void *)&ipaddr.sin_addr, (void *)(&ip->ip_dst),
			    sizeof(ipaddr.sin_addr));
			/*
			 * locate outgoing interface; if we're the destination,
			 * use the incoming interface (should be same).
			 */
			if ((ia = ifatoia(ifa_ifwithaddr(sintosa(&ipaddr))))
			    == NULL &&
			    (ia = ip_rtaddr(ipaddr.sin_addr)) == NULL) {
				type = ICMP_UNREACH;
				code = ICMP_UNREACH_HOST;
				goto bad;
			}
			bcopy((void *)&ia->ia_addr.sin_addr,
			    (void *)(cp + off), sizeof(struct in_addr));
			cp[IPOPT_OFFSET] += sizeof(struct in_addr);
			break;

		case IPOPT_TS:
			code = cp - (u_char *)ip;
			ipt = (struct ip_timestamp *)cp;
			if (ipt->ipt_len < 4 || ipt->ipt_len > 40) {
				code = (u_char *)&ipt->ipt_len - (u_char *)ip;
				goto bad;
			}
			if (ipt->ipt_ptr < 5) {
				code = (u_char *)&ipt->ipt_ptr - (u_char *)ip;
				goto bad;
			}
			if (ipt->ipt_ptr > ipt->ipt_len - sizeof (int32_t)) {
				if (++ipt->ipt_oflw == 0) {
					code = (u_char *)&ipt->ipt_ptr -
					    (u_char *)ip;
					goto bad;
				}
				break;
			}
			cp0 = (cp + ipt->ipt_ptr - 1);
			switch (ipt->ipt_flg) {

			case IPOPT_TS_TSONLY:
				break;

			case IPOPT_TS_TSANDADDR:
				if (ipt->ipt_ptr - 1 + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len) {
					code = (u_char *)&ipt->ipt_ptr -
					    (u_char *)ip;
					goto bad;
				}
				ipaddr.sin_addr = dst;
				ia = ifatoia(ifaof_ifpforaddr(sintosa(&ipaddr),
				    m->m_pkthdr.rcvif));
				if (ia == 0)
					continue;
				bcopy(&ia->ia_addr.sin_addr,
				    cp0, sizeof(struct in_addr));
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			case IPOPT_TS_PRESPEC:
				if (ipt->ipt_ptr - 1 + sizeof(n_time) +
				    sizeof(struct in_addr) > ipt->ipt_len) {
					code = (u_char *)&ipt->ipt_ptr -
					    (u_char *)ip;
					goto bad;
				}
				memcpy(&ipaddr.sin_addr, cp0,
				    sizeof(struct in_addr));
				if (ifatoia(ifa_ifwithaddr(sintosa(&ipaddr)))
				    == NULL)
					continue;
				ipt->ipt_ptr += sizeof(struct in_addr);
				break;

			default:
				/* XXX can't take &ipt->ipt_flg */
				code = (u_char *)&ipt->ipt_ptr -
				    (u_char *)ip + 1;
				goto bad;
			}
			ntime = iptime();
			cp0 = (u_char *) &ntime; /* XXX grumble, GCC... */
			memmove((char *)cp + ipt->ipt_ptr - 1, cp0,
			    sizeof(n_time));
			ipt->ipt_ptr += sizeof(n_time);
		}
	}
	if (forward) {
		if (ip_forwsrcrt == 0) {
			type = ICMP_UNREACH;
			code = ICMP_UNREACH_SRCFAIL;
			goto bad;
		}
		ip_forward(m, 1);
		return (1);
	}
	return (0);
bad:
	icmp_error(m, type, code, 0, 0);
	IP_STATINC(IP_STAT_BADOPTIONS);
	return (1);
}

/*
 * Given address of next destination (final or next hop),
 * return internet address info of interface to be used to get there.
 */
struct in_ifaddr *
ip_rtaddr(struct in_addr dst)
{
	struct rtentry *rt;
	union {
		struct sockaddr		dst;
		struct sockaddr_in	dst4;
	} u;

	sockaddr_in_init(&u.dst4, &dst, 0);

	if ((rt = rtcache_lookup(&ipforward_rt, &u.dst)) == NULL)
		return NULL;

	return ifatoia(rt->rt_ifa);
}

/*
 * Save incoming source route for use in replies,
 * to be picked up later by ip_srcroute if the receiver is interested.
 */
void
save_rte(u_char *option, struct in_addr dst)
{
	unsigned olen;

	olen = option[IPOPT_OLEN];
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("save_rte: olen %d\n", olen);
#endif /* 0 */
	if (olen > sizeof(ip_srcrt) - (1 + sizeof(dst)))
		return;
	memcpy((void *)ip_srcrt.srcopt, (void *)option, olen);
	ip_nhops = (olen - IPOPT_OFFSET - 1) / sizeof(struct in_addr);
	ip_srcrt.dst = dst;
}

/*
 * Retrieve incoming source route for use in replies,
 * in the same form used by setsockopt.
 * The first hop is placed before the options, will be removed later.
 */
struct mbuf *
ip_srcroute(void)
{
	struct in_addr *p, *q;
	struct mbuf *m;

	if (ip_nhops == 0)
		return NULL;
	m = m_get(M_DONTWAIT, MT_SOOPTS);
	if (m == 0)
		return NULL;

	MCLAIM(m, &inetdomain.dom_mowner);
#define OPTSIZ	(sizeof(ip_srcrt.nop) + sizeof(ip_srcrt.srcopt))

	/* length is (nhops+1)*sizeof(addr) + sizeof(nop + srcrt header) */
	m->m_len = ip_nhops * sizeof(struct in_addr) + sizeof(struct in_addr) +
	    OPTSIZ;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf("ip_srcroute: nhops %d mlen %d", ip_nhops, m->m_len);
#endif

	/*
	 * First save first hop for return route
	 */
	p = &ip_srcrt.route[ip_nhops - 1];
	*(mtod(m, struct in_addr *)) = *p--;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf(" hops %x", ntohl(mtod(m, struct in_addr *)->s_addr));
#endif

	/*
	 * Copy option fields and padding (nop) to mbuf.
	 */
	ip_srcrt.nop = IPOPT_NOP;
	ip_srcrt.srcopt[IPOPT_OFFSET] = IPOPT_MINOFF;
	memmove(mtod(m, char *) + sizeof(struct in_addr), &ip_srcrt.nop,
	    OPTSIZ);
	q = (struct in_addr *)(mtod(m, char *) +
	    sizeof(struct in_addr) + OPTSIZ);
#undef OPTSIZ
	/*
	 * Record return path as an IP source route,
	 * reversing the path (pointers are now aligned).
	 */
	while (p >= ip_srcrt.route) {
#ifdef DIAGNOSTIC
		if (ipprintfs)
			printf(" %x", ntohl(q->s_addr));
#endif
		*q++ = *p--;
	}
	/*
	 * Last hop goes to final destination.
	 */
	*q = ip_srcrt.dst;
#ifdef DIAGNOSTIC
	if (ipprintfs)
		printf(" %x\n", ntohl(q->s_addr));
#endif
	return (m);
}

const int inetctlerrmap[PRC_NCMDS] = {
	[PRC_MSGSIZE] = EMSGSIZE,
	[PRC_HOSTDEAD] = EHOSTDOWN,
	[PRC_HOSTUNREACH] = EHOSTUNREACH,
	[PRC_UNREACH_NET] = EHOSTUNREACH,
	[PRC_UNREACH_HOST] = EHOSTUNREACH,
	[PRC_UNREACH_PROTOCOL] = ECONNREFUSED,
	[PRC_UNREACH_PORT] = ECONNREFUSED,
	[PRC_UNREACH_SRCFAIL] = EHOSTUNREACH,
	[PRC_PARAMPROB] = ENOPROTOOPT,
};

void
ip_fasttimo(void)
{
	if (ip_drainwanted) {
		ip_drain();
		ip_drainwanted = 0;
	}
}

void
ip_drainstub(void)
{
	ip_drainwanted = 1;
}

/*
 * Forward a packet.  If some error occurs return the sender
 * an icmp packet.  Note we can't always generate a meaningful
 * icmp message because icmp doesn't have a large enough repertoire
 * of codes and types.
 *
 * If not forwarding, just drop the packet.  This could be confusing
 * if ipforwarding was zero but some routing protocol was advancing
 * us as a gateway to somewhere.  However, we must let the routing
 * protocol deal with that.
 *
 * The srcrt parameter indicates whether the packet is being forwarded
 * via a source route.
 */
void
ip_forward(struct mbuf *m, int srcrt)
{
	struct ip *ip = mtod(m, struct ip *);
	struct rtentry *rt;
	int error, type = 0, code = 0, destmtu = 0;
	struct mbuf *mcopy;
	n_long dest;
	union {
		struct sockaddr		dst;
		struct sockaddr_in	dst4;
	} u;

	/*
	 * We are now in the output path.
	 */
	MCLAIM(m, &ip_tx_mowner);

	/*
	 * Clear any in-bound checksum flags for this packet.
	 */
	m->m_pkthdr.csum_flags = 0;

	dest = 0;
#ifdef DIAGNOSTIC
	if (ipprintfs) {
		printf("forward: src %s ", inet_ntoa(ip->ip_src));
		printf("dst %s ttl %x\n", inet_ntoa(ip->ip_dst), ip->ip_ttl);
	}
#endif
	if (m->m_flags & (M_BCAST|M_MCAST) || in_canforward(ip->ip_dst) == 0) {
		IP_STATINC(IP_STAT_CANTFORWARD);
		m_freem(m);
		return;
	}
	if (ip->ip_ttl <= IPTTLDEC) {
		icmp_error(m, ICMP_TIMXCEED, ICMP_TIMXCEED_INTRANS, dest, 0);
		return;
	}

	sockaddr_in_init(&u.dst4, &ip->ip_dst, 0);
	if ((rt = rtcache_lookup(&ipforward_rt, &u.dst)) == NULL) {
		icmp_error(m, ICMP_UNREACH, ICMP_UNREACH_NET, dest, 0);
		return;
	}

	/*
	 * Save at most 68 bytes of the packet in case
	 * we need to generate an ICMP message to the src.
	 * Pullup to avoid sharing mbuf cluster between m and mcopy.
	 */
	mcopy = m_copym(m, 0, imin(ntohs(ip->ip_len), 68), M_DONTWAIT);
	if (mcopy)
		mcopy = m_pullup(mcopy, ip->ip_hl << 2);

	ip->ip_ttl -= IPTTLDEC;

	/*
	 * If forwarding packet using same interface that it came in on,
	 * perhaps should send a redirect to sender to shortcut a hop.
	 * Only send redirect if source is sending directly to us,
	 * and if packet was not source routed (or has any options).
	 * Also, don't send redirect if forwarding using a default route
	 * or a route modified by a redirect.
	 */
	if (rt->rt_ifp == m->m_pkthdr.rcvif &&
	    (rt->rt_flags & (RTF_DYNAMIC|RTF_MODIFIED)) == 0 &&
	    !in_nullhost(satocsin(rt_getkey(rt))->sin_addr) &&
	    ipsendredirects && !srcrt) {
		if (rt->rt_ifa &&
		    (ip->ip_src.s_addr & ifatoia(rt->rt_ifa)->ia_subnetmask) ==
		    ifatoia(rt->rt_ifa)->ia_subnet) {
			if (rt->rt_flags & RTF_GATEWAY)
				dest = satosin(rt->rt_gateway)->sin_addr.s_addr;
			else
				dest = ip->ip_dst.s_addr;
			/*
			 * Router requirements says to only send host
			 * redirects.
			 */
			type = ICMP_REDIRECT;
			code = ICMP_REDIRECT_HOST;
#ifdef DIAGNOSTIC
			if (ipprintfs)
				printf("redirect (%d) to %x\n", code,
				    (u_int32_t)dest);
#endif
		}
	}

	error = ip_output(m, NULL, &ipforward_rt,
	    (IP_FORWARDING | (ip_directedbcast ? IP_ALLOWBROADCAST : 0)),
	    NULL, NULL);

	if (error)
		IP_STATINC(IP_STAT_CANTFORWARD);
	else {
		uint64_t *ips = IP_STAT_GETREF();
		ips[IP_STAT_FORWARD]++;
		if (type) {
			ips[IP_STAT_REDIRECTSENT]++;
			IP_STAT_PUTREF();
		} else {
			IP_STAT_PUTREF();
			if (mcopy) {
#ifdef GATEWAY
				if (mcopy->m_flags & M_CANFASTFWD)
					ipflow_create(&ipforward_rt, mcopy);
#endif
				m_freem(mcopy);
			}
			return;
		}
	}
	if (mcopy == NULL)
		return;

	switch (error) {

	case 0:				/* forwarded, but need redirect */
		/* type, code set above */
		break;

	case ENETUNREACH:		/* shouldn't happen, checked above */
	case EHOSTUNREACH:
	case ENETDOWN:
	case EHOSTDOWN:
	default:
		type = ICMP_UNREACH;
		code = ICMP_UNREACH_HOST;
		break;

	case EMSGSIZE:
		type = ICMP_UNREACH;
		code = ICMP_UNREACH_NEEDFRAG;

		if ((rt = rtcache_validate(&ipforward_rt)) != NULL)
			destmtu = rt->rt_ifp->if_mtu;

#if defined(FAST_IPSEC)
		{
			/*
			 * If the packet is routed over IPsec tunnel, tell the
			 * originator the tunnel MTU.
			 *	tunnel MTU = if MTU - sizeof(IP) - ESP/AH hdrsiz
			 * XXX quickhack!!!
			 */

			struct secpolicy *sp;
			int ipsecerror;
			size_t ipsechdr;
			struct route *ro;

			sp = ipsec4_getpolicybyaddr(mcopy,
			    IPSEC_DIR_OUTBOUND, IP_FORWARDING,
			    &ipsecerror);

			if (sp != NULL) {
				/* count IPsec header size */
				ipsechdr = ipsec4_hdrsiz(mcopy,
				    IPSEC_DIR_OUTBOUND, NULL);

				/*
				 * find the correct route for outer IPv4
				 * header, compute tunnel MTU.
				 */

				if (sp->req != NULL
				 && sp->req->sav != NULL
				 && sp->req->sav->sah != NULL) {
					ro = &sp->req->sav->sah->sa_route;
					rt = rtcache_validate(ro);
					if (rt && rt->rt_ifp) {
						destmtu =
						    rt->rt_rmx.rmx_mtu ?
						    rt->rt_rmx.rmx_mtu :
						    rt->rt_ifp->if_mtu;
						destmtu -= ipsechdr;
					}
				}

				KEY_FREESP(&sp);
			}
		}
#endif /*defined(FAST_IPSEC)*/
		IP_STATINC(IP_STAT_CANTFRAG);
		break;

	case ENOBUFS:
#if 1
		/*
		 * a router should not generate ICMP_SOURCEQUENCH as
		 * required in RFC1812 Requirements for IP Version 4 Routers.
		 * source quench could be a big problem under DoS attacks,
		 * or if the underlying interface is rate-limited.
		 */
		if (mcopy)
			m_freem(mcopy);
		return;
#else
		type = ICMP_SOURCEQUENCH;
		code = 0;
		break;
#endif
	}
	icmp_error(mcopy, type, code, dest, destmtu);
}

void
ip_savecontrol(struct inpcb *inp, struct mbuf **mp, struct ip *ip,
    struct mbuf *m)
{

	if (inp->inp_socket->so_options & SO_TIMESTAMP
#ifdef SO_OTIMESTAMP
	    || inp->inp_socket->so_options & SO_OTIMESTAMP
#endif
	    ) {
		struct timeval tv;

		microtime(&tv);
#ifdef SO_OTIMESTAMP
		if (inp->inp_socket->so_options & SO_OTIMESTAMP) {
			struct timeval50 tv50;
			timeval_to_timeval50(&tv, &tv50);
			*mp = sbcreatecontrol((void *) &tv50, sizeof(tv50),
			    SCM_OTIMESTAMP, SOL_SOCKET);
		} else
#endif
		*mp = sbcreatecontrol((void *) &tv, sizeof(tv),
		    SCM_TIMESTAMP, SOL_SOCKET);
		if (*mp)
			mp = &(*mp)->m_next;
	}
	if (inp->inp_flags & INP_RECVDSTADDR) {
		*mp = sbcreatecontrol((void *) &ip->ip_dst,
		    sizeof(struct in_addr), IP_RECVDSTADDR, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
#ifdef notyet
	/*
	 * XXX
	 * Moving these out of udp_input() made them even more broken
	 * than they already were.
	 *	- fenner@parc.xerox.com
	 */
	/* options were tossed already */
	if (inp->inp_flags & INP_RECVOPTS) {
		*mp = sbcreatecontrol((void *) opts_deleted_above,
		    sizeof(struct in_addr), IP_RECVOPTS, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
	/* ip_srcroute doesn't do what we want here, need to fix */
	if (inp->inp_flags & INP_RECVRETOPTS) {
		*mp = sbcreatecontrol((void *) ip_srcroute(),
		    sizeof(struct in_addr), IP_RECVRETOPTS, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
#endif
	if (inp->inp_flags & INP_RECVIF) {
		struct sockaddr_dl sdl;

		sockaddr_dl_init(&sdl, sizeof(sdl),
		    (m->m_pkthdr.rcvif != NULL)
		        ?  m->m_pkthdr.rcvif->if_index
			: 0,
			0, NULL, 0, NULL, 0);
		*mp = sbcreatecontrol(&sdl, sdl.sdl_len, IP_RECVIF, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
	if (inp->inp_flags & INP_RECVTTL) {
		*mp = sbcreatecontrol((void *) &ip->ip_ttl,
		    sizeof(uint8_t), IP_RECVTTL, IPPROTO_IP);
		if (*mp)
			mp = &(*mp)->m_next;
	}
}

/*
 * sysctl helper routine for net.inet.ip.forwsrcrt.
 */
static int
sysctl_net_inet_ip_forwsrcrt(SYSCTLFN_ARGS)
{
	int error, tmp;
	struct sysctlnode node;

	node = *rnode;
	tmp = ip_forwsrcrt;
	node.sysctl_data = &tmp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);

	error = kauth_authorize_network(l->l_cred, KAUTH_NETWORK_FORWSRCRT,
	    0, NULL, NULL, NULL);
	if (error)
		return (error);

	ip_forwsrcrt = tmp;

	return (0);
}

/*
 * sysctl helper routine for net.inet.ip.mtudisctimeout.  checks the
 * range of the new value and tweaks timers if it changes.
 */
static int
sysctl_net_inet_ip_pmtudto(SYSCTLFN_ARGS)
{
	int error, tmp;
	struct sysctlnode node;

	node = *rnode;
	tmp = ip_mtudisc_timeout;
	node.sysctl_data = &tmp;
	error = sysctl_lookup(SYSCTLFN_CALL(&node));
	if (error || newp == NULL)
		return (error);
	if (tmp < 0)
		return (EINVAL);

	mutex_enter(softnet_lock);

	ip_mtudisc_timeout = tmp;
	rt_timer_queue_change(ip_mtudisc_timeout_q, ip_mtudisc_timeout);

	mutex_exit(softnet_lock);

	return (0);
}

#ifdef GATEWAY
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

	mutex_enter(softnet_lock);
	KERNEL_LOCK(1, NULL);

	ipflow_prune();

	KERNEL_UNLOCK_ONE(NULL);
	mutex_exit(softnet_lock);

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
		mutex_enter(softnet_lock);
		KERNEL_LOCK(1, NULL);

		error = ipflow_invalidate_all(tmp);

		KERNEL_UNLOCK_ONE(NULL);
		mutex_exit(softnet_lock);

	} else {
		/*
		 * EINVAL if not a power of 2
	         */
		error = EINVAL;
	}

	return error;
}
#endif /* GATEWAY */

static int
sysctl_net_inet_ip_stats(SYSCTLFN_ARGS)
{

	return (NETSTAT_SYSCTL(ipstat_percpu, IP_NSTATS));
}

static void
sysctl_net_inet_ip_setup(struct sysctllog **clog)
{
	extern int subnetsarelocal, hostzeroisbroadcast;

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "net", NULL,
		       NULL, 0, NULL, 0,
		       CTL_NET, CTL_EOL);
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
		       CTLTYPE_INT, "forwarding",
		       SYSCTL_DESCR("Enable forwarding of INET datagrams"),
		       NULL, 0, &ipforwarding, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_FORWARDING, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "redirect",
		       SYSCTL_DESCR("Enable sending of ICMP redirect messages"),
		       NULL, 0, &ipsendredirects, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_SENDREDIRECTS, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "ttl",
		       SYSCTL_DESCR("Default TTL for an INET datagram"),
		       NULL, 0, &ip_defttl, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_DEFTTL, CTL_EOL);
#ifdef IPCTL_DEFMTU
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT /* |CTLFLAG_READWRITE? */,
		       CTLTYPE_INT, "mtu",
		       SYSCTL_DESCR("Default MTA for an INET route"),
		       NULL, 0, &ip_mtu, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_DEFMTU, CTL_EOL);
#endif /* IPCTL_DEFMTU */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "forwsrcrt",
		       SYSCTL_DESCR("Enable forwarding of source-routed "
				    "datagrams"),
		       sysctl_net_inet_ip_forwsrcrt, 0, &ip_forwsrcrt, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_FORWSRCRT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "directed-broadcast",
		       SYSCTL_DESCR("Enable forwarding of broadcast datagrams"),
		       NULL, 0, &ip_directedbcast, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_DIRECTEDBCAST, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "allowsrcrt",
		       SYSCTL_DESCR("Accept source-routed datagrams"),
		       NULL, 0, &ip_allowsrcrt, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_ALLOWSRCRT, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "subnetsarelocal",
		       SYSCTL_DESCR("Whether logical subnets are considered "
				    "local"),
		       NULL, 0, &subnetsarelocal, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_SUBNETSARELOCAL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "mtudisc",
		       SYSCTL_DESCR("Use RFC1191 Path MTU Discovery"),
		       NULL, 0, &ip_mtudisc, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_MTUDISC, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "anonportmin",
		       SYSCTL_DESCR("Lowest ephemeral port number to assign"),
		       sysctl_net_inet_ip_ports, 0, &anonportmin, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_ANONPORTMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "anonportmax",
		       SYSCTL_DESCR("Highest ephemeral port number to assign"),
		       sysctl_net_inet_ip_ports, 0, &anonportmax, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_ANONPORTMAX, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "mtudisctimeout",
		       SYSCTL_DESCR("Lifetime of a Path MTU Discovered route"),
		       sysctl_net_inet_ip_pmtudto, 0, &ip_mtudisc_timeout, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_MTUDISCTIMEOUT, CTL_EOL);
#ifdef GATEWAY
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
#endif /* GATEWAY */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "hostzerobroadcast",
		       SYSCTL_DESCR("All zeroes address is broadcast address"),
		       NULL, 0, &hostzeroisbroadcast, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_HOSTZEROBROADCAST, CTL_EOL);
#if NGIF > 0
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "gifttl",
		       SYSCTL_DESCR("Default TTL for a gif tunnel datagram"),
		       NULL, 0, &ip_gif_ttl, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_GIF_TTL, CTL_EOL);
#endif /* NGIF */
#ifndef IPNOPRIVPORTS
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "lowportmin",
		       SYSCTL_DESCR("Lowest privileged ephemeral port number "
				    "to assign"),
		       sysctl_net_inet_ip_ports, 0, &lowportmin, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_LOWPORTMIN, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "lowportmax",
		       SYSCTL_DESCR("Highest privileged ephemeral port number "
				    "to assign"),
		       sysctl_net_inet_ip_ports, 0, &lowportmax, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_LOWPORTMAX, CTL_EOL);
#endif /* IPNOPRIVPORTS */
#if NGRE > 0
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "grettl",
		       SYSCTL_DESCR("Default TTL for a gre tunnel datagram"),
		       NULL, 0, &ip_gre_ttl, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_GRE_TTL, CTL_EOL);
#endif /* NGRE */
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "checkinterface",
		       SYSCTL_DESCR("Enable receive side of Strong ES model "
				    "from RFC1122"),
		       NULL, 0, &ip_checkinterface, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_CHECKINTERFACE, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "random_id",
		       SYSCTL_DESCR("Assign random ip_id values"),
		       NULL, 0, &ip_do_randomid, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_RANDOMID, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "do_loopback_cksum",
		       SYSCTL_DESCR("Perform IP checksum on loopback"),
		       NULL, 0, &ip_do_loopback_cksum, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_LOOPBACKCKSUM, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_STRUCT, "stats",
		       SYSCTL_DESCR("IP statistics"),
		       sysctl_net_inet_ip_stats, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IP, IPCTL_STATS,
		       CTL_EOL);
}

void
ip_statinc(u_int stat)
{

	KASSERT(stat < IP_NSTATS);
	IP_STATINC(stat);
}
