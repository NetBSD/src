/*	$NetBSD: mip6.c,v 1.1.2.1 2008/02/22 02:53:34 keiichi Exp $	*/
/*	$Id: mip6.c,v 1.1.2.1 2008/02/22 02:53:34 keiichi Exp $	*/

/*
 * Copyright (C) 2004 WIDE Project.  All rights reserved.
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

#ifdef __FreeBSD__
#include "opt_ipsec.h"
#include "opt_inet.h"
#include "opt_inet6.h"
#include "opt_mip6.h"
#endif
#ifdef __NetBSD__
#include "opt_ipsec.h"
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#if defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/malloc.h>
#endif
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#include <net/if.h>
#include <net/if_types.h>
#include <net/route.h>
#include <net/mipsock.h>
#include <net/net_osdep.h>
#ifdef __APPLE__
#include <net/kpi_protocol.h>
#endif /* __APPLE__ */

#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/ip_encap.h>
#include <netinet/icmp6.h>
#include <netinet/ip6mh.h>

#include <netinet6/in6_var.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mip6.h>
#include <netinet6/mip6_var.h>
#include <net/if_mip.h>

#ifndef __OpenBSD__
#ifdef IPSEC
#include <netinet6/ipsec.h>
#include <netkey/key.h>
#include <netkey/keydb.h>
#endif /* IPSEC */
#ifdef FAST_IPSEC
#include <netipsec/ipsec.h>
#include <netipsec/key.h>
#include <netipsec/keydb.h>
#endif /* FAST_IPSEC */
#endif /* !__OpenBSD__ */

#ifdef __OpenBSD__
#include <netinet/ip_ipsp.h>
#endif

#ifdef __APPLE__
#include <machine/spl.h>
#endif /* __APPLE__ */

#include "mip.h"

#ifndef MIP6_BC_HASH_SIZE
#define MIP6_BC_HASH_SIZE 35			/* XXX */
#endif
/* XXX I'm not sure the effectivity of this hash function */
#define MIP6_IN6ADDR_HASH(addr)					\
	((addr)->s6_addr32[0] ^ (addr)->s6_addr32[1] ^		\
	 (addr)->s6_addr32[2] ^ (addr)->s6_addr32[3])
#define MIP6_BC_HASH_ID(addr1, addr2)				\
	((MIP6_IN6ADDR_HASH(addr1) )	\
	 % MIP6_BC_HASH_SIZE)
struct mip6_bc_internal *mip6_bc_hash[MIP6_BC_HASH_SIZE];
struct mip6_bc_list mip6_bc_list = LIST_HEAD_INITIALIZER(mip6_bc_list);
const struct encaptab *mbc_encap = NULL;

struct mip6stat mip6stat;
u_int8_t mip6_nodetype = MIP6_NODETYPE_NONE;

/* sysctl parameters. */
int mip6ctl_use_ipsec = 1;
#ifdef MIP6_DEBUG
int mip6ctl_debug = 1;
#else
int mip6ctl_debug = 0;
#endif
int mip6ctl_rr_hint_ppslim = 10;
int mip6ctl_use_migrate = 0;
#ifndef IFA_MBUL_LIST
struct mip6_bul_list mbul_list;
#endif

#ifdef __APPLE__
lck_mtx_t 	*bc_mtx;	/*### global binding cache mutex for now */
lck_attr_t 	*bc_mtx_attr;
lck_grp_t 	*bc_mtx_grp;
lck_grp_attr_t 	*bc_mtx_grp_attr;

lck_mtx_t 	*bul_mtx;	/*### global binding uptade list mutex for now */
lck_attr_t 	*bul_mtx_attr;
lck_grp_t 	*bul_mtx_grp;
lck_grp_attr_t 	*bul_mtx_grp_attr;
#endif /* __APPLE__ */


extern struct ip6protosw mip6_tunnel_protosw;

#if NMIP > 0
static int mip6_rr_hint_pps_count = 0;
static struct timeval mip6_rr_hint_ppslim_last;

static struct mip6_bul_internal *mip6_bul_create(const struct in6_addr *,
    const struct in6_addr *, const struct in6_addr *, u_int16_t, u_int8_t,
    struct mip_softc *, u_int16_t);
#if defined(__NetBSD__) && __NetBSD_Version__ >= 400000000
int mip6_bul_encapcheck(struct mbuf *, int, int, void *arg);
#else
int mip6_bul_encapcheck(const struct mbuf *, int, int, void *arg);
#endif /* __NetBSD__ && __NetBSD_Version__ >= 400000000 */
static void mip6_bul_update_ipsecdb(struct mip6_bul_internal *);
#endif /* NMIP > 0 */
#if defined(__NetBSD__) && __NetBSD_Version__ >= 400000000
int mip6_rev_encapcheck(struct mbuf *, int, int, void *arg);
#else
int mip6_rev_encapcheck(const struct mbuf *, int, int, void *arg);
#endif /* __NetBSD__ && __NetBSD_Version__ >= 400000000 */

static struct mip6_bc_internal *mip6_bce_new_entry(struct in6_addr *,
    struct in6_addr *, struct in6_addr *, struct ifaddr *, u_int16_t,
    u_int16_t);
static void mip6_bc_list_insert(struct mip6_bc_internal *);
static void mip6_bc_list_remove(struct mip6_bc_internal *);
static void mip6_bce_update_ipsecdb(struct mip6_bc_internal *);

#if NMIP > 0
static int mip6_rr_hint_ratelimit(const struct in6_addr *,
    const struct in6_addr *);
#endif /* NMIP > 0 */

static int mhdefaultlen[] = {
	sizeof(struct ip6_mh_binding_request), 
	sizeof(struct ip6_mh_home_test_init), 
	sizeof(struct ip6_mh_careof_test_init),
	sizeof(struct ip6_mh_home_test),
	sizeof(struct ip6_mh_careof_test),
	sizeof(struct ip6_mh_binding_update),
	sizeof(struct ip6_mh_binding_ack),
	sizeof(struct ip6_mh_binding_error)
};

/*
 * sysctl knobs.
 */
#if defined(__OpenBSD__)
int
mip6_sysctl(name, namelen, oldp, oldlenp, newp, newlen)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
{
	/* All sysctl names at this level are terminal. */
	if (namelen != 1)
		return ENOTDIR;

	switch (name[0]) {
	case MIP6CTL_DEBUG:
		return sysctl_int(oldp, oldlenp, newp, newlen, &mip6ctl_debug);
	case MIP6CTL_USE_IPSEC:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &mip6ctl_use_ipsec);
	case MIP6CTL_RR_HINT_PPSLIM:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &mip6ctl_rr_hint_ppslim);
	case MIP6CTL_USE_MIGRATE:
		return sysctl_int(oldp, oldlenp, newp, newlen,
		    &mip6ctl_use_migrate);
	default:
		return (EOPNOTSUPP);
	}
	/* NOTREACHED */
}
#endif /* __OpenBSD__ */

#ifdef __NetBSD__
/*
 * sysctl for MIP6
 */
SYSCTL_SETUP(sysctl_net_inet6_mip6_setup, "sysctl net.inet6.mip6 subtree setup")
{
        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT,
                       CTLTYPE_NODE, "net", NULL,
                       NULL, 0, NULL, 0,
                       CTL_NET, CTL_EOL);
        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT,
                       CTLTYPE_NODE, "inet6",
                       SYSCTL_DESCR("PF_INET6 related settings"),
                       NULL, 0, NULL, 0,
                       CTL_NET, PF_INET6, CTL_EOL);
        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT,
                       CTLTYPE_NODE, "mip6",
                       SYSCTL_DESCR("MIPv6 related settings"),
                       NULL, 0, NULL, 0,
                       CTL_NET, PF_INET6, IPPROTO_MH, CTL_EOL);

        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "debug",
                       SYSCTL_DESCR("Enable mip6 debug output"),
                       NULL, 0, &mip6ctl_debug, 0,
                       CTL_NET, PF_INET6, IPPROTO_MH,
                       MIP6CTL_DEBUG, CTL_EOL);
        sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "use_ipsec",
                       SYSCTL_DESCR("Enable ipsec of mip6"),
                       NULL, 0, &mip6ctl_use_ipsec, 0,
                       CTL_NET, PF_INET6, IPPROTO_MH,
                       MIP6CTL_USE_IPSEC, CTL_EOL);
         sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "rr_hint_ppslimit",
                       SYSCTL_DESCR("Maximum RR hints sent per second"),
                       NULL, 0, &mip6ctl_rr_hint_ppslim, 0,
                       CTL_NET, PF_INET6, IPPROTO_MH,
                       MIP6CTL_RR_HINT_PPSLIM, CTL_EOL);
         sysctl_createv(clog, 0, NULL, NULL,
                       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
                       CTLTYPE_INT, "use_migrate",
                       SYSCTL_DESCR("Enable SADB_X_MIGRATE"),
                       NULL, 0, &mip6ctl_use_migrate, 0,
                       CTL_NET, PF_INET6, IPPROTO_MH,
                       MIP6CTL_RR_HINT_PPSLIM, CTL_EOL);
}
#endif /* __NetBSD__ */

#if defined(__FreeBSD__) || defined(__APPLE__)
SYSCTL_DECL(_net_inet6_mip6);
 
SYSCTL_INT(_net_inet6_mip6, MIP6CTL_DEBUG, debug, CTLFLAG_RW,
    &mip6ctl_debug, 0, "");
SYSCTL_INT(_net_inet6_mip6, MIP6CTL_USE_IPSEC, use_ipsec, CTLFLAG_RW,
    &mip6ctl_use_ipsec, 0, "");
SYSCTL_INT(_net_inet6_mip6, MIP6CTL_RR_HINT_PPSLIM, rr_hint_ppslimit,
    CTLFLAG_RW, &mip6ctl_rr_hint_ppslim, 0, "");
SYSCTL_INT(_net_inet6_mip6, MIP6CTL_USE_MIGRATE, use_migrate, CTLFLAG_RW,
    &mip6ctl_use_migrate, 0, "");
#endif /* __FreeBSD__ || __APPLE__*/

/*
 * Mobility Header processing.
 */
int
#ifndef __APPLE__
mip6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
#else
mip6_input(mp, offp)
	struct mbuf **mp;
	int *offp;
#endif
{
	struct mbuf *m = *mp;
	struct ip6_hdr *ip6;
	struct ip6_mh *mh;
	int off = *offp, mhlen, sum, presence = 0;
	struct in6_addr src, dst, hoa, rt;

	mip6stat.mip6s_mh++;

	ip6 = mtod(m, struct ip6_hdr *);

	/* validation of the length of the header */
	IP6_EXTHDR_GET(mh, struct ip6_mh *, m, off, sizeof(*mh));
	if (mh == NULL)
		return (IPPROTO_DONE);

	/* 
	 * Section 9.2 of RFC3775 said the node MUST discard
	 * and SHOULD send ICMP Parameter Problem
	 */
	if (mh->ip6mh_proto != IPPROTO_NONE) {
		mip6log((LOG_INFO,
		    "mip6_input:%d: Payload Proto %d.\n",
		    __LINE__, mh->ip6mh_proto));

		mip6stat.mip6s_payloadproto++;

                /* If either a home address option or a routing header
		 * option is present, the source address of icmp6
		 * error messages must be carefully choosen. If the
		 * home registration is not yet completed, the home
		 * address cannot be used for the source
		 * address. Instead, the care-of address must be used
		 * to send the icmp6 error messages.
		 */
		if (mip6_get_ip6hdrinfo(m, &src, &dst, &hoa, &rt, 0, &presence)==0) {
			if (presence & RTHDR_PRESENT) {
				bcopy(&rt, &ip6->ip6_dst, sizeof(struct in6_addr));
			}
		}
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
		    (char *)&mh->ip6mh_proto - (char *)ip6);
		return (IPPROTO_DONE);
	}

	/*
	 * Section 9.2 of RFC3775. If the length is invalid, the node
	 * MUST discard the packet and SHOULD send ICMP parameter
	 * problem, Code 0.
	 */
	mhlen = (mh->ip6mh_len + 1) << 3;
	if (mhlen < IP6OPT_MINLEN ||
	    (mh->ip6mh_type < sizeof(mhdefaultlen) / sizeof(int)
	     && mhlen < mhdefaultlen[mh->ip6mh_type])) {
		mip6log((LOG_INFO, "%s:%d: Mobility Header Length %d.\n",
			__FILE__, __LINE__, mhlen));

		ip6stat.ip6s_toosmall++;

                /* If either a home address option or a routing header
		 * option is present, the source address of icmp6
		 * error messages must be carefully choosen. If the
		 * home registration is not yet completed, the home
		 * address cannot be used for the source
		 * address. Instead, the care-of address must be used
		 * to send the icmp6 error messages.
		 */
		if (mip6_get_ip6hdrinfo(m, &src, &dst, &hoa, &rt, 0, &presence)==0) {
			if (presence & RTHDR_PRESENT) {
				bcopy(&rt, &ip6->ip6_dst, sizeof(struct in6_addr));
			}
		}

		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
		    (char *)&mh->ip6mh_len - (char *)ip6);

		return (IPPROTO_DONE);
	}

	/* validate the checksum */
	IP6_EXTHDR_GET(mh, struct ip6_mh *, m, off, mhlen);
	if (mh == NULL)
		return (IPPROTO_DONE);
	if ((sum = in6_cksum(m, IPPROTO_MH, off, mhlen)) != 0) {
		mip6log((LOG_ERR,
		    "mip6_input:%d: Mobility Header checksum error"
		    "(type = %d, sum = %x) from %s\n",
		    __LINE__,
		    mh->ip6mh_type, sum, ip6_sprintf(&ip6->ip6_src)));
		m_freem(m);
		mip6stat.mip6s_checksum++;

		return (IPPROTO_DONE);
	}

	switch (mh->ip6mh_type) {
#if NMIP > 0
	case IP6_MH_TYPE_BACK:
	{
		/*
		 * Search the binding update entry correspondent to
		 * the Binding Acknowledgement (BA) message. If this BA is
		 * for the home registration, the BA MUST be protected
		 * by IPsec. Otherwise, BA is silently discarded.
		 */
		struct mip6_bul_internal *mbul;

		mbul = mip6_bul_get(&ip6->ip6_src, &ip6->ip6_dst, 0 /* XXX */);
		if (mbul != NULL
		    && (mbul->mbul_flags & IP6_MH_BU_HOME) != 0
		    && mip6ctl_use_ipsec) {
#ifndef __OpenBSD__
			if (((m->m_flags & M_DECRYPTED) == 0)
			    && (m->m_flags & M_AUTHIPHDR) == 0)
#else /* !__OpenBSD__ */
			if ((m->m_flags & M_AUTH) == 0)
#endif /* __OpenBSD__ */
			{
				mip6log((LOG_ERR,
				    "mip6_input: an unprotected binding "
				    "acknowledgement from %s\n",
				    ip6_sprintf(&ip6->ip6_src)));
				m_freem(m);
				mip6stat.mip6s_unprotected++;

				return (IPPROTO_DONE);
			}
		}
		break;
	}
#endif /* NMIP > 0 */
	case IP6_MH_TYPE_BU:
	{
		/*
		 * If this Binding Update message (BU) is for the home
		 * registration, the BU MUST be protected by
		 * IPsec. Otherwise, BU is silently discarded.
		 */
		struct ip6_mh_binding_update *bu;

		bu = (struct ip6_mh_binding_update *)mh;
		if ((bu->ip6mhbu_flags & IP6_MH_BU_HOME)
		    && mip6ctl_use_ipsec) {
#ifndef __OpenBSD__
			if (((m->m_flags & M_DECRYPTED) == 0)
			    && (m->m_flags & M_AUTHIPHDR) == 0)
#else /* !__OpenBSD__ */
			if ((m->m_flags & M_AUTH) == 0)
#endif /* __OpenBSD__ */
			{
				mip6log((LOG_ERR,
					 "mip6_input: an unprotected binding "
					 "update from %s\n",
					 ip6_sprintf(&ip6->ip6_src)));
				m_freem(m);
				mip6stat.mip6s_unprotected++;
				
				return (IPPROTO_DONE);
			}
		}
		break;
	}

	default:
		break;
	}

	/* deliver the packet using Raw IPv6 interface. */
	return (rip6_input(mp, offp
#ifndef __APPLE__
			   ,proto
#endif
			   ));
}

int
#ifndef __APPLE__
mip6_tunnel_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;
#else
mip6_tunnel_input(mp, offp)
	struct mbuf **mp;
	int *offp;
#endif
{
	struct mbuf *m = *mp;
#if NMIP > 0
	int off, nxt;
	struct icmp6_hdr icmp6;
	struct in6_addr src, dst;
	struct mip6_bul_internal *bul, *cnbul;
	int presence;
#ifdef __APPLE__
	int proto;
	u_int8_t exthdr[2];
#endif /* __APPLE__ */
#endif /* NMIP > 0 */
#ifndef __FreeBSD__
	int s;
#endif

#ifdef __APPLE__
	off = sizeof(struct ip6_hdr);
	proto = mtod(m, struct ip6_hdr *)->ip6_nxt;
	while (off < *offp) {
		m_copydata(m, off, sizeof(exthdr), (void *)exthdr);
		proto = exthdr[0];
		off += (exthdr[1] + 1) << 3;
	}
#endif /* __APPLE__ */

	m_adj(m, *offp);

	switch (proto) {
	case IPPROTO_IPV6:
		if (m->m_len < sizeof(struct ip6_hdr)) {
			m = m_pullup(m, sizeof(struct ip6_hdr));
			if (!m)
				return (IPPROTO_DONE);
		}

#if NMIP > 0
		if ((off = ip6_lasthdr(m, 0, IPPROTO_IPV6, &nxt)) == -1) {
			return (IPPROTO_DONE);
		}
		/* a mobile node must not start the RR procedure
		   when receiving tunneled MH messages. */
		if (nxt == IPPROTO_MH)
			goto dontstartrr;

		/* a mobile node must not start the RR procedure
		   when receiving ICMPv6 error messages. */
		if (nxt == IPPROTO_ICMPV6) {
			m_copydata(m, off, sizeof(struct icmp6_hdr),
			    (void *)&icmp6);
			if (icmp6.icmp6_type < ICMP6_ECHO_REQUEST)
				goto dontstartrr;
		}

		if (mip6_get_ip6hdrinfo(m, &src, &dst, NULL, NULL, 1 /* logical */, &presence) < 0) {
			mip6log((LOG_ERR, "mip6_tunnel_input: "
			    "failded to get logical source and destination "
			    "addresses.\n"));
			return (IPPROTO_DONE);
		}

		bul = mip6_bul_get_home_agent(&dst);
		if (bul == NULL)
			goto dontstartrr;
		cnbul = mip6_bul_get(&dst, &src, 0);
		if (cnbul != NULL)
			goto dontstartrr;

		mip6_notify_rr_hint(&dst, &src);

	dontstartrr:
#endif /* NMIP > 0 */

		mip6stat.mip6s_revtunnel++;

#if defined(__APPLE__)
		proto_input(PF_INET6, m);
#else
#if defined(__NetBSD__)
		s = splnet();
#elif defined(__OpenBSD__)
		s = splimp();
#endif

#ifdef __FreeBSD__
		if (!IF_HANDOFF(&ip6intrq, m, NULL))
			goto bad;
#else
		if (IF_QFULL(&ip6intrq)) {
			IF_DROP(&ip6intrq);	/* update statistics */
			splx(s);
			goto bad;
		}
		IF_ENQUEUE(&ip6intrq, m);
#endif /* __FreeBSD__ */

#ifndef __FreeBSD__
		splx(s);
#endif /* !__FreeBSD__ */
#endif /* __APPLE__ */		
		break;
	default:
		mip6log((LOG_ERR, "protocol %d not supported.\n", proto));
		goto bad;
	}

	return (IPPROTO_DONE);

 bad:
	m_freem(m);
	return (IPPROTO_DONE);
}

struct mip6_bc_internal *
mip6_bce_get(hoa, cnaddr, coa, bid)
	struct in6_addr *hoa;
	struct in6_addr *cnaddr;
	struct in6_addr *coa;
	u_int16_t bid;
{
	struct mip6_bc_internal *mbc;
	int hash = MIP6_BC_HASH_ID(hoa, cnaddr);

	for (mbc = mip6_bc_hash[hash];
	     mbc && mbc->mbc_hash_cache == hash;
	     mbc = LIST_NEXT(mbc, mbc_entry)) {
#ifdef MIP6_MCOA
		if (bid && (mbc->mbc_bid != bid))
			continue;
		if (coa && !IN6_ARE_ADDR_EQUAL(&mbc->mbc_coa, coa))
			continue;
#endif /* MIP6_COA */
		if (IN6_ARE_ADDR_EQUAL(&mbc->mbc_hoa, hoa) &&
		    (cnaddr ? IN6_ARE_ADDR_EQUAL(&mbc->mbc_cnaddr, cnaddr) : 1))
			break;
	}

	return (mbc ? (mbc->mbc_hash_cache == hash ? mbc : NULL) : NULL);
}

static struct mip6_bc_internal *
mip6_bce_new_entry(cnaddr, hoa, coa, ifa, flags, bid)
	struct in6_addr *cnaddr, *hoa, *coa;
	struct ifaddr *ifa;
	u_int16_t flags, bid;
{
	struct mip6_bc_internal *mbc = NULL;

	MALLOC(mbc, struct mip6_bc_internal *,
	       sizeof(struct mip6_bc_internal), M_TEMP, M_NOWAIT);
	if (mbc == NULL)
		return (NULL);

	bzero(mbc, sizeof(*mbc));
	mbc->mbc_cnaddr = *cnaddr;
	mbc->mbc_hoa = *hoa;
	mbc->mbc_coa = *coa;
	mbc->mbc_ifaddr = ifa;
	mbc->mbc_flags = flags;
#ifdef MIP6_MCOA
	mbc->mbc_bid = bid;
#endif /* MIP6_MCOA */

	return (mbc);
}

static void
mip6_bc_list_insert(mbc)
	struct mip6_bc_internal *mbc;
{
	int hash = MIP6_BC_HASH_ID(&mbc->mbc_hoa, &mbc->mbc_cnaddr);

	if (mip6_bc_hash[hash] != NULL)
		LIST_INSERT_BEFORE(mip6_bc_hash[hash], mbc, mbc_entry);
	else
		LIST_INSERT_HEAD(&mip6_bc_list, mbc, mbc_entry);
	mip6_bc_hash[hash] = mbc;
	mbc->mbc_hash_cache = hash;
}

int
mip6_bce_update(cnaddr, hoa, coa, flags, bid)
	struct sockaddr_in6 *cnaddr, *hoa, *coa;
	u_int16_t flags, bid;
{
	int s;
	int error = 0;
	struct mip6_bc_internal *bce = NULL;
	struct ifaddr *ifa;

	/* Non IPv6 address is not support (only for MIP6) */
	if ((cnaddr->sin6_family != AF_INET6) ||
	    (hoa->sin6_family != AF_INET6) ||
	    (coa->sin6_family != AF_INET6))
		return (EAFNOSUPPORT); /* XXX ? */

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif

	bce = mip6_bce_get((struct in6_addr *)(&hoa->sin6_addr),
	    (struct in6_addr *)(&cnaddr->sin6_addr), NULL, bid);
	if (bce) {
		bce->mbc_coa = coa->sin6_addr;
		goto bc_update_ipsecdb;
	} 

	/* No existing BC entry is found.  Create a new one. */
	ifa = ifa_ifwithaddr((struct sockaddr *)cnaddr);
	bce = mip6_bce_new_entry(&cnaddr->sin6_addr, &hoa->sin6_addr,
	    &coa->sin6_addr, ifa, flags, bid);
	if (bce == NULL) {
		error = ENOMEM;
		goto done;
	}
	mip6_bc_list_insert(bce);
		
	/* The home agent creates a proxy ND entry for the mobile node. */
	if (MIP6_IS_HA && bce != NULL &&
	    (flags & IP6_MH_BU_HOME) != 0) {
		if (mbc_encap == NULL) {
			mbc_encap = encap_attach_func(AF_INET6, IPPROTO_IPV6,
			    mip6_rev_encapcheck,
			    (void *)&mip6_tunnel_protosw, NULL);
		}
		if (mbc_encap == NULL) {
			mip6log((LOG_ERR, "mip6_bce_update: "
			    "attaching an encaptab on a home agent "
			    "failed.\n"));
			error = EIO; /* XXX ? */
			goto done;
		}

		error = mip6_bc_proxy_control(&hoa->sin6_addr,
					      &cnaddr->sin6_addr, RTM_ADD);
		if (error) {
			mip6log((LOG_ERR, "mip6_bce_update: "
				 "adding a proxy ND entry failed.\n"));
			goto done;
		}
	}

 bc_update_ipsecdb:
	if (MIP6_IS_HA && bce != NULL &&
	    (flags & IP6_MH_BU_HOME) != 0) {
		mip6_bce_update_ipsecdb(bce);
	}

 done:
	splx(s);

	return (error);
}

void
mip6_bc_list_remove(mbc)
	struct mip6_bc_internal *mbc;
{
	int hash = MIP6_BC_HASH_ID(&mbc->mbc_hoa, &mbc->mbc_cnaddr);

	if (mip6_bc_hash[hash] == mbc) {
		struct mip6_bc_internal *next = LIST_NEXT(mbc, mbc_entry);
		if (next != NULL &&
		    hash == MIP6_BC_HASH_ID(&next->mbc_hoa, &mbc->mbc_cnaddr)) {
			mip6_bc_hash[hash] = next;
		} else {
			mip6_bc_hash[hash] = NULL;
		}
	}

	LIST_REMOVE(mbc, mbc_entry);
}

int
mip6_bce_remove_addr(cnaddr, hoa, coa, flags, bid)
	struct sockaddr_in6 *cnaddr, *hoa, *coa;
	u_int16_t flags, bid;
{
	struct mip6_bc_internal *mbc;

	mbc = mip6_bce_get(&hoa->sin6_addr, &cnaddr->sin6_addr, NULL, bid);
	if (!mbc)
		return (0);

	mbc->mbc_coa = coa->sin6_addr;
	return (mip6_bce_remove_bc(mbc));
}

int
mip6_bce_remove_bc(mbc)
	struct mip6_bc_internal *mbc;
{
	int s;
	int error = 0;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif
	mip6_bc_list_remove(mbc);

	if (MIP6_IS_HA && (mbc->mbc_flags & IP6_MH_BU_HOME)) {
		mip6_stop_dad(&mbc->mbc_hoa, -1);
		error = mip6_bc_proxy_control(&mbc->mbc_hoa,
					      &mbc->mbc_cnaddr, RTM_DELETE);
		mip6_bce_update_ipsecdb(mbc);
	}
	FREE(mbc, M_TEMP);

	splx(s);

	return (error);
}

static void
mip6_bce_update_ipsecdb(bce)
	struct mip6_bc_internal *bce;
{
#if (defined(IPSEC) || defined(FAST_IPSEC)) && !defined(__OpenBSD__)
/* racoon2 guys want us to update ipsecdb. (2004.10.8) */
	struct sockaddr_in6 hoa_sa, coa_sa, haaddr_sa;

	/* update the ipsecdb. */
	bzero(&hoa_sa, sizeof(struct sockaddr_in6));
	hoa_sa.sin6_len = sizeof(struct sockaddr_in6);
	hoa_sa.sin6_family = AF_INET6;
	hoa_sa.sin6_addr = bce->mbc_hoa;

	bzero(&coa_sa, sizeof(struct sockaddr_in6));
	coa_sa.sin6_len = sizeof(struct sockaddr_in6);
	coa_sa.sin6_family = AF_INET6;
	coa_sa.sin6_addr = bce->mbc_coa;

	bzero(&haaddr_sa, sizeof(struct sockaddr_in6));
	haaddr_sa.sin6_len = sizeof(struct sockaddr_in6);
	haaddr_sa.sin6_family = AF_INET6;
	haaddr_sa.sin6_addr = bce->mbc_cnaddr;

	key_mip6_update_home_agent_ipsecdb(&hoa_sa, NULL, &coa_sa,
	    &haaddr_sa);
#endif /* IPSEC && !__OpenBSD__ */
}

void
mip6_bce_remove_all(void)
{
	struct mip6_bc_internal *mbc, *mbcn = NULL;
	int s;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif
	for (mbc = LIST_FIRST(&mip6_bc_list); mbc; mbc = mbcn) {
		mbcn = LIST_NEXT(mbc, mbc_entry);

		mip6_bc_list_remove(mbc);
	}
	splx(s);
}

/* 
 * Create a Type 2 Routing Header for outgoing packets. The caller
 * must free the returned buffer (struct ip6_rthdr2 *) when it
 * finished.
 */
struct ip6_rthdr2 *
mip6_create_rthdr2(coa)
	struct in6_addr *coa;
{
	struct ip6_rthdr2 *rthdr2;
	size_t len;

	/*
	 * Mobile IPv6 uses a Type 2 Routing Header for route
	 * optimization. If the packet has a Type 1 Routing Header
	 * already, we must add the Type 2 Routing Header after the
	 * Type 1 one.
	 */

	len = sizeof(struct ip6_rthdr2)	+ sizeof(struct in6_addr);
	MALLOC(rthdr2, struct ip6_rthdr2 *, len, M_IP6OPT, M_NOWAIT);
	if (rthdr2 == NULL) {
		return (NULL);
	}
	bzero(rthdr2, len);

	/* The rthdr2->ip6r2_nxt field will be filled later in ip6_output */
	rthdr2->ip6r2_len = 2;
	rthdr2->ip6r2_type = 2;
	rthdr2->ip6r2_segleft = 1;
	rthdr2->ip6r2_reserved = 0;
	bcopy((void *)coa, (void *)(rthdr2 + 1), sizeof(struct in6_addr));

	mip6stat.mip6s_orthdr2++;

	return (rthdr2);
}

#if NMIP > 0
struct in6_ifaddr *
mip6_ifa_ifwithin6addr(in6)
	const struct in6_addr *in6;
{
	struct sockaddr_in6 sin6;

	/*
	 * in6 must have a scope information embedded in it,
	 * before being passed to this function.
	 */
	bzero(&sin6, sizeof(struct sockaddr_in6));
	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_family = AF_INET6;
	sin6.sin6_addr = *in6;
#ifndef __APPLE__
	if (sa6_recoverscope(&sin6) != 0)
		return (NULL);
#else
	/* It should support scope6.c */
	if (IN6_IS_SCOPE_LINKLOCAL(&sin6.sin6_addr)
	    /*||IN6_IS_ADDR_MC_INTFACELOCAL(&sin6.sin6_addr)*/) {
		sin6.sin6_scope_id = ntohs(sin6.sin6_addr.s6_addr16[1]);
		sin6.sin6_addr.s6_addr16[1] = 0;
	}
#endif

	return ((struct in6_ifaddr *)ifa_ifwithaddr((struct sockaddr *)&sin6));
}

static struct mip6_bul_internal *
mip6_bul_create(peeraddr, hoa, coa, flags, state, sc, bid)
	const struct in6_addr *peeraddr, *hoa, *coa;
	u_int16_t flags;
	u_int8_t state;
	struct mip_softc *sc;
	u_int16_t bid;
{
	struct mip6_bul_internal *mbul;

	MALLOC(mbul, struct mip6_bul_internal *,
	    sizeof(struct mip6_bul_internal), M_TEMP, M_NOWAIT);
	if (mbul == NULL)
		return (NULL);

	bzero(mbul, sizeof(*mbul));
	mbul->mbul_peeraddr = *peeraddr;
	mbul->mbul_hoa = *hoa;
	mbul->mbul_coa = *coa;
	mbul->mbul_mip = sc;
	mbul->mbul_flags = flags;
	mbul->mbul_state = state;
#ifdef MIP6_MCOA 
	mbul->mbul_bid = bid;
#endif /* MIP6_MCOA */

	return (mbul);
}

int
mip6_bul_add(peeraddr, hoa, coa, hoa_ifindex, flags, state, bid)
	const struct in6_addr *peeraddr, *hoa, *coa;
	u_short hoa_ifindex;
	u_int16_t flags;
	u_int8_t state;
	u_int16_t bid;
{
	int error = 0;
	struct in6_ifaddr *ia6_hoa;
	struct mip6_bul_internal *mbul;

	ia6_hoa = mip6_ifa_ifwithin6addr(hoa);
	if (ia6_hoa == NULL)
		return (-1);

	if (IN6_IS_ADDR_MULTICAST(peeraddr) ||
	    IN6_IS_ADDR_UNSPECIFIED(peeraddr))
		return (EINVAL);

	if (IN6_IS_ADDR_MULTICAST(coa) ||
	    IN6_IS_ADDR_UNSPECIFIED(coa))
		return (EINVAL);
		
	/* 
	 * If the target entry exists, the entry is removed
	 * first. Then, the requested bul will be added right after
	 * this deletion.  
	 */
	mbul = mip6_bul_get(hoa, peeraddr, bid);
	if (mbul) 
		mip6_bul_remove(mbul);

	/* binding update list is created here */
	mbul = mip6_bul_create(peeraddr, hoa, coa, flags, state,
	    (struct mip_softc *)ia6_hoa->ia_ifp, bid);
	if (mbul == NULL)
		return (-1);
	LIST_INSERT_HEAD(MBUL_LIST(ia6_hoa), mbul, mbul_entry);

	/* 
	 * tunnel setup only when the requested bul is for home
	 * registration. In NEMO case, the tunnel will be setup in
	 * the SHISA daemon.
	 */
	if ((mbul->mbul_flags & IP6_MH_BU_HOME) && 
	    (mbul->mbul_flags & IP6_MH_BU_ROUTER) == 0) {
#ifndef MIP_USE_PF
		mbul->mbul_encap = encap_attach_func(AF_INET6, IPPROTO_IPV6,
		    mip6_bul_encapcheck, (void *)&mip6_tunnel_protosw, mbul);
		if (error) {
			mip6log((LOG_ERR, "tunnel move failed.\n"));
			/* XXX notifiy to upper XXX */
			mip6_bul_remove(mbul); 
			
			return (error);
		}
#endif /* !MIP_USE_PF */
		mip6_bul_update_ipsecdb(mbul);
	}
	
	return (error);
}

void
mip6_bul_remove(mbul)
	struct mip6_bul_internal *mbul;
{
	int s;
#ifndef MIP_USE_PF
	int error;
#endif /* !MIP_USE_PF */

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif

	LIST_REMOVE(mbul, mbul_entry);

	/* Removing Tunnel encap */
	if ((mbul->mbul_flags & IP6_MH_BU_HOME) && 
	    (mbul->mbul_flags & IP6_MH_BU_ROUTER) == 0) {
#ifndef MIP_USE_PF
		error = encap_detach(mbul->mbul_encap);
		mbul->mbul_encap = NULL;
		if (error) {
			mip6log((LOG_ERR, "mip6_bul_remove: "
			    "tunnel deletione failed.\n"));
		}
#endif /* !MIP_USE_PF */
		mip6_bul_update_ipsecdb(mbul);
	}

	FREE(mbul, M_TEMP);

	splx(s);
}

static void
mip6_bul_update_ipsecdb(mbul)
	struct mip6_bul_internal *mbul;
{
#if (defined(IPSEC) || defined(FAST_IPSEC)) && !defined(__OpenBSD__)
/* racoon2 guys want us to update ipsecdb. (2004.10.8) */
	struct sockaddr_in6 hoa_sa, coa_sa, haaddr_sa;

	/* update ipsecdb. */
	bzero(&hoa_sa, sizeof(struct sockaddr_in6));
	hoa_sa.sin6_len = sizeof(struct sockaddr_in6);
	hoa_sa.sin6_family = AF_INET6;
	hoa_sa.sin6_addr = mbul->mbul_hoa;

	bzero(&coa_sa, sizeof(struct sockaddr_in6));
	coa_sa.sin6_len = sizeof(struct sockaddr_in6);
	coa_sa.sin6_family = AF_INET6;
	coa_sa.sin6_addr = mbul->mbul_coa;

	bzero(&haaddr_sa, sizeof(struct sockaddr_in6));
	haaddr_sa.sin6_len = sizeof(struct sockaddr_in6);
	haaddr_sa.sin6_family = AF_INET6;
	haaddr_sa.sin6_addr = mbul->mbul_peeraddr;

	key_mip6_update_mobile_node_ipsecdb(&hoa_sa, NULL, &coa_sa,
	    &haaddr_sa);
#endif /* IPSEC && !__OpenBSD__ */
}

void
mip6_bul_remove_all()
{
	int s;
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	struct in6_ifaddr *ia6;
	struct mip6_bul_internal *mbul, *nmbul;

#if defined(__NetBSD__) || defined(__OpenBSD__)
	s = splsoftnet();
#else
	s = splnet();
#endif

#if defined(__FreeBSD__) || defined(__APPLE__)
#ifdef __FreeBSD__
	IFNET_RLOCK();
#endif
	TAILQ_FOREACH(ifp, &ifnet, if_link)
#elif defined(__NetBSD__) || defined(__OpenBSD__)
	TAILQ_FOREACH(ifp, &ifnet, if_list)
#endif
	 {
#if defined(__FreeBSD__) || defined(__APPLE__)
		TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link)
#elif defined(__NetBSD__) || defined(__OpenBSD__)
		TAILQ_FOREACH(ifa, &ifp->if_addrlist, ifa_list) 
#endif
		{
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			ia6 = (struct in6_ifaddr *)ifa;
			
			if (LIST_EMPTY(MBUL_LIST(ia6)))
				continue;

			for (mbul = LIST_FIRST(MBUL_LIST(ia6)); mbul;
			     mbul = nmbul) {
				nmbul = LIST_NEXT(mbul, mbul_entry);
				mip6_bul_remove(mbul);
			}
		}
	}
#ifdef __FreeBSD__
	IFNET_RUNLOCK();
#endif /* __FreeBSD__ */

	splx(s);
}

struct mip6_bul_internal *
mip6_bul_get(src, dst, bid)
	const struct in6_addr *src, *dst;
	u_int16_t bid;
{
	struct in6_ifaddr *ia6_src;
	struct mip6_bul_internal *mbul;

	ia6_src = mip6_ifa_ifwithin6addr(src);
	if (ia6_src == NULL)
		return (NULL);
	
	for (mbul = LIST_FIRST(MBUL_LIST(ia6_src)); mbul;
	     mbul = LIST_NEXT(mbul, mbul_entry)) {
#ifdef MIP6_MCOA
		if (bid && (bid != mbul->mbul_bid))
			continue;
#endif /* MIP6_MCOA */

		if (IN6_ARE_ADDR_EQUAL(src, &mbul->mbul_hoa)
		    && IN6_ARE_ADDR_EQUAL(dst, &mbul->mbul_peeraddr))
			return (mbul);
	}

	/* not found. */
	return (NULL);
}


struct mip6_bul_internal *
mip6_bul_get_home_agent(src)
	const struct in6_addr *src;
{
	struct in6_ifaddr *ia6_src;
	struct mip6_bul_internal *mbul;

	ia6_src = mip6_ifa_ifwithin6addr(src);
	if (ia6_src == NULL)
		return (NULL);

	for (mbul = LIST_FIRST(MBUL_LIST(ia6_src)); mbul;
	     mbul = LIST_NEXT(mbul, mbul_entry)) {
		if (IN6_ARE_ADDR_EQUAL(src, &mbul->mbul_hoa)
		    && (mbul->mbul_flags & IP6_MH_BU_HOME) != 0)
			return (mbul);
	}

	/* not found. */
	return (NULL);
}

struct ip6_opt_home_address *
mip6_search_hoa_in_destopt(u_int8_t *optbuf)
{
	int optlen = 0;
	int destoptlen = (((struct ip6_dest *)optbuf)->ip6d_len + 1) << 3;
	optbuf += sizeof(struct ip6_dest);
	destoptlen -= sizeof(struct ip6_dest);

	/* search a HoA destination option */
	for (optlen = 0; destoptlen > 0; 
		destoptlen -= optlen, optbuf += optlen) {

		switch (*optbuf) {
		case IP6OPT_PAD1:
			optlen = 1;
			break;
		case IP6OPT_HOME_ADDRESS:
			return (struct ip6_opt_home_address *)optbuf;
		case IP6OPT_PADN:
		default:
			optlen = *(optbuf + 1) + 2;
			break;
		}
	}

	return (NULL);
}

/* 
 * Create a HoA Destination Option for outgoing packets. The caller
 * must free the returned buffer when it finished.
 */
u_int8_t *
mip6_create_hoa_opt(coa)
	struct in6_addr *coa;
{
	struct ip6_opt_home_address *ha_opt;
	struct ip6_dest *ip6dest;
	register char *optbuf;
	size_t pad, optlen, buflen;

	optlen = sizeof(struct ip6_dest);

	/*
	 * calculate the padding size for the HoA destination option
	 * (8n + 6).
	 */
	pad = MIP6_PADLEN(optlen, 8, 6);

	/* allocating a new buffer space for the HoA option. */
	buflen = optlen + pad + sizeof(struct ip6_opt_home_address);
	MALLOC(optbuf, char *, buflen, M_IP6OPT, M_NOWAIT);
	if (optbuf == NULL)
		return (NULL);
	bzero(optbuf, buflen);

	ip6dest = (struct ip6_dest *)optbuf;
		
	/* filling zero for padding. */
	MIP6_FILL_PADDING(optbuf + optlen, pad);
	optlen += pad;

	/* filing the HoA destination option fields. */
	ha_opt = (struct ip6_opt_home_address *)(optbuf + optlen);
	ha_opt->ip6oh_type = IP6OPT_HOME_ADDRESS;
	ha_opt->ip6oh_len = IP6OPT_HALEN;
	bcopy(coa, ha_opt->ip6oh_addr , sizeof(struct in6_addr));
	optlen += sizeof(struct ip6_opt_home_address);

	ip6dest->ip6d_nxt = 0;
	ip6dest->ip6d_len = ((optlen) >> 3) - 1;

	return (optbuf);
}

/* 
 * If the given prefix is equal to one of home prefixes, return
 * TRUE. Otherwise, it returns zero (FALSE)
 */
int
mip6_are_homeprefix(ndpr)
#ifndef __APPLE__
	struct nd_prefixctl *ndpr;
#else
	struct nd_prefix *ndpr;
#endif
{
	struct in6_ifaddr *ia6;
#ifdef __APPLE__
#define in6_ifaddr	in6_ifaddrs
#endif

	for (ia6 = in6_ifaddr; ia6; ia6 = ia6->ia_next) {
		if (ia6->ia_ifp == NULL)
			continue;

		if (ia6->ia_ifp->if_type != IFT_MOBILEIP)
			continue;

		if (in6_are_prefix_equal(&ndpr->ndpr_prefix.sin6_addr,
		    &ia6->ia_addr.sin6_addr, ndpr->ndpr_plen))
			return (TRUE);
	}
	return (FALSE);
#ifdef __APPLE__
#undef in6_ifaddr
#endif
}

int
mip6_ifa6_is_addr_valid_hoa(ifa6)
	struct in6_ifaddr *ifa6;
{
	struct mip6_bul_internal *mbul;

	if ((ifa6->ia6_flags & IN6_IFF_HOME) == 0)
		return (0);

	if (ifa6->ia_ifp->if_type != IFT_MOBILEIP) {
		/* The Mobile Node is attached to the home link */
		if ((ifa6->ia6_flags & IN6_IFF_DEREGISTERING) == 0)
			return (1);
	} else {
		mbul = mip6_bul_get_home_agent(&ifa6->ia_addr.sin6_addr);
		if (mbul != NULL)
			return (1);
	}

	return (0);
}

int
mip6_bul_encapcheck(m, off, proto, arg)
#if defined(__NetBSD__) && __NetBSD_Version__ >= 400000000
	struct mbuf *m;
#else
	const struct mbuf *m;
#endif /* __NetBSD__ && __NetBSD_Version__ >= 400000000 */
	int off;
	int proto;
	void *arg;
{
	struct mip6_bul_internal *mbul = (struct mip6_bul_internal *)arg;
	struct ip6_hdr *ip6;

	if (mbul == NULL)
		return (0);
	if (mbul->mbul_mip == NULL)
		return (0);

	ip6 = mtod(m, struct ip6_hdr*);

	/*
	 * Validation of source and destination address.  The sender
	 * MUST be Home Agent and the desitnation MUST be CoA of the
	 * mobile node.
	 */
	if (!IN6_ARE_ADDR_EQUAL(&ip6->ip6_src, &mbul->mbul_peeraddr)
	    || !(IN6_ARE_ADDR_EQUAL(&ip6->ip6_dst, &mbul->mbul_coa)))
		return (0);

	/*
	 * XXX: should we compare the ifid of the inner dstaddr of the
	 * incoming packet and the ifid of the mobile node's?  these
	 * check will be done in the ip6_input and later.
	 * Ryuji: No.
	 */

	return (128);
}
#endif /* NMIP > 0*/

int
mip6_bc_proxy_control(target, local, cmd)
	struct in6_addr *target;
	struct in6_addr *local;
	int cmd;
{
	struct sockaddr_in6 target_sa, local_sa, mask_sa;
	struct in6_addr daddr;
	const struct sockaddr_dl *sdl;
        struct rtentry *rt, *nrt;
	struct ifaddr *ifa;
	struct ifnet *ifp;
	int flags, error = 0;

	/* create a sockaddr_in6 structure for own address (local). */
	bzero(&local_sa, sizeof(local_sa));
	local_sa.sin6_len = sizeof(local_sa);
	local_sa.sin6_family = AF_INET6;
	local_sa.sin6_addr = *local;

	ifa = ifa_ifwithaddr((struct sockaddr *)&local_sa);
	if (ifa == NULL)
		return (EINVAL);
	ifp = ifa->ifa_ifp;

	bzero(&target_sa, sizeof(target_sa));
	target_sa.sin6_len = sizeof(target_sa);
	target_sa.sin6_family = AF_INET6;
	target_sa.sin6_addr = *target;

	switch (cmd) {
	case RTM_DELETE:
#if defined(__FreeBSD__) || defined(__APPLE__)
		rt = rtalloc1((struct sockaddr *)&target_sa, 0, 0UL);
#else /* __FreeBSD__ || __APPLE__ */
		rt = rtalloc1((struct sockaddr *)&target_sa, 0);
#endif /* __FreeBSD__  || __APPLE__ */
		if (rt) {
#ifdef __FreeBSD__
			RT_REMREF(rt);
			RT_UNLOCK(rt);
#else
			rt->rt_refcnt--;
#endif /* __FreeBSD__ */
		}
		if (rt == NULL)
			return (0);
		if ((rt->rt_flags & RTF_HOST) == 0 ||
		    (rt->rt_flags & RTF_ANNOUNCE) == 0) {
			/*
			 * there is a rtentry, but it is not a host nor
			 * a proxy entry.
			 */
			return (0);
		}
		error = rtrequest(RTM_DELETE, rt_getkey(rt),
		    (struct sockaddr *)0, rt_mask(rt), 0, (struct rtentry **)0);
		if (error) {
			mip6log((LOG_ERR,
				 "RTM_DELETE for %s returned error = %d\n",
				 ip6_sprintf(target), error));
		}
		rt = NULL;

		break;

	case RTM_ADD:
		/* Don't check exsiting routing entry */
#ifdef __NetBSD__
		sdl = ifp->if_sadl;
#else
		/* sdl search */
	{
		struct ifaddr *ifa_dl;

		for (ifa_dl = ifp->if_addrlist.tqh_first; ifa_dl;
		     ifa_dl = ifa_dl->ifa_list.tqe_next)
			if (ifa_dl->ifa_addr->sa_family == AF_LINK)
				break;

		if (!ifa_dl)
			return (EINVAL);

		sdl = (struct sockaddr_dl *)ifa_dl->ifa_addr;
	}
#endif /* __NetBSD__ */

		/* create a mask. */
		bzero(&mask_sa, sizeof(mask_sa));
		mask_sa.sin6_family = AF_INET6;
		mask_sa.sin6_len = sizeof(mask_sa);

		in6_prefixlen2mask(&mask_sa.sin6_addr, 128);
		flags = (RTF_STATIC | RTF_HOST | RTF_ANNOUNCE);

		error = rtrequest(RTM_ADD, (struct sockaddr *)&target_sa,
		    (const struct sockaddr *)sdl, (struct sockaddr *)&mask_sa, flags,
		    &nrt);

		if (error == 0) {
			/* Avoid expiration */
			if (nrt) {
#ifdef __FreeBSD__
				RT_LOCK(nrt);
				nrt->rt_rmx.rmx_expire = 0;
				RT_REMREF(nrt);
				RT_UNLOCK(nrt);
#else
				nrt->rt_rmx.rmx_expire = 0;
				nrt->rt_refcnt--;
#endif /* __FreeBSD__ */
			} else
				error = EINVAL;
		} else {
			mip6log((LOG_ERR,
			    "RTM_ADD for %s returned error = %d\n",
			    ip6_sprintf(target), error));
		}

		daddr = in6addr_linklocal_allnodes;
#ifndef __APPLE__
		if ((error = in6_setscope(&daddr, ifp, NULL)) != 0) {
			mip6log((LOG_ERR, "in6_setscope failed\n"));
			break;
		}
#else
		daddr.s6_addr16[1] = htons(ifp->if_index);
#endif 
		nd6_na_output(ifp, &daddr, &target_sa.sin6_addr,
			      ND_NA_FLAG_OVERRIDE, 1, (const struct sockaddr *)sdl);
		break;

	default:
		mip6log((LOG_ERR, "Invalid command %d in mip6control\n", cmd));
		error = -1;
		break;
	}

	return (error);
}

/*  validation of the Source Address in the outer IPv6 header. */
int
mip6_rev_encapcheck(m, off, proto, arg)
#if defined(__NetBSD__) && __NetBSD_Version__ >= 400000000
	struct mbuf *m;
#else
	const struct mbuf *m;
#endif /* __NetBSD__ && __NetBSD_Version__ >= 400000000 */
	int off;
	int proto;
	void *arg;
{
	struct ip6_hdr *oip6, *iip6;
	struct mip6_bc_internal *mbc;

	oip6 = mtod(m, struct ip6_hdr *);
	iip6 = oip6 + 1;

	mbc = mip6_bce_get(&iip6->ip6_src, &oip6->ip6_dst, &oip6->ip6_src, 0);
	if (mbc && IN6_ARE_ADDR_EQUAL(&mbc->mbc_coa, &oip6->ip6_src))
		return (128);

	return (0);
}

int
mip6_encapsulate(mm, osrc, odst)
	struct mbuf **mm;
	struct in6_addr *osrc, *odst;
{
	struct mbuf *m = *mm;
	struct ip6_hdr *ip6;
	struct ip6_mh mh;

	/* check whether this packet can be tunneled or not */
	ip6 = mtod(m, struct ip6_hdr *);
	/* If packet is Binding Error originated by this node, ignore it  */
	if (ip6->ip6_nxt == IPPROTO_MH) {
		m_copydata(m, sizeof(struct ip6_hdr),
			sizeof(struct ip6_mh), (void *)&mh);
		if (mh.ip6mh_type == IP6_MH_TYPE_BERROR) {
			printf("skip encapsulation for BE\n");
			goto done;
		}
	}

	M_PREPEND(m, sizeof(struct ip6_hdr), M_DONTWAIT);
	if (m && m->m_len < sizeof(struct ip6_hdr))
		m = m_pullup(m, sizeof(struct ip6_hdr));
	if (m == NULL)
		return (-1);

	ip6 = mtod(m, struct ip6_hdr *);
	ip6->ip6_flow = 0;
	ip6->ip6_vfc &= ~IPV6_VERSION_MASK;
	ip6->ip6_vfc |= IPV6_VERSION;
	ip6->ip6_plen = htons((u_short)m->m_pkthdr.len - sizeof(*ip6));
	ip6->ip6_nxt = IPPROTO_IPV6;
	ip6->ip6_hlim = ip6_defhlim;
	ip6->ip6_src = *osrc;
	ip6->ip6_dst = *odst;
	mip6stat.mip6s_orevtunnel++;

   done:
	return (ip6_output(m, 0, 0, 0, 0
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__APPLE__)
		, NULL, NULL
#else
		, NULL
#endif
		));
}

#if NMIP > 0
void
mip6_probe_routers(void)
{
        struct llinfo_nd6 *ln;
 
        ln = llinfo_nd6.ln_next;
        while (ln && ln != &llinfo_nd6) {
                if ((ln->ln_router) &&
                    ((ln->ln_state == ND6_LLINFO_REACHABLE) ||
                     (ln->ln_state == ND6_LLINFO_STALE))) {
                        ln->ln_asked = 0;
                        ln->ln_state = ND6_LLINFO_DELAY;
#ifndef __APPLE__
                        nd6_llinfo_settimer(ln, 0);
#endif
                }
                ln = ln->ln_next;
        }
}
#endif /* NMIP > 0 */

#if NMIP > 0
void
mip6_notify_rr_hint(dst, src)
	struct in6_addr *dst;
	struct in6_addr *src;
{
	struct sockaddr_in6 src_sin6, dst_sin6;

	if (mip6_rr_hint_ratelimit(dst, src)) {
		/* rate limited. */
		return;
	}

	bzero(&src_sin6, sizeof(struct sockaddr_in6));
	src_sin6.sin6_len = sizeof(struct sockaddr_in6);
	src_sin6.sin6_family = AF_INET6;
	src_sin6.sin6_addr = *src;

	bzero(&dst_sin6, sizeof(struct sockaddr_in6));
	dst_sin6.sin6_len = sizeof(struct sockaddr_in6);
	dst_sin6.sin6_family = AF_INET6;
	dst_sin6.sin6_addr = *dst;

	mips_notify_rr_hint((struct sockaddr *)&dst_sin6,
	    (struct sockaddr *)&src_sin6);
}

static int
mip6_rr_hint_ratelimit(dst, src)
	const struct in6_addr *dst;	/* not used at this moment */
	const struct in6_addr *src;	/* not used at this moment */
{
	int ret;

	ret = 0;	/* okay to send */

	/* PPS limit XXX 1 per 1 second. */
	if (!ppsratecheck(&mip6_rr_hint_ppslim_last, &mip6_rr_hint_pps_count,
	    mip6ctl_rr_hint_ppslim)) {
		/* The packet is subject to rate limit */
		ret++;
	}

	return ret;
}
#endif /* NMIP > 0 */

/* 
 * Retrieve IPv6 header information such as 
 *   - source and destination addresses
 *   - addresses stored in HoA option and RT header (if present)

 * When there are IPv6 options, the address stored in the options are
 * kept in hoa_addr and rt_addr with the logical flag set to
 * zero. When the logical flag is set, rt_addr and hoa_addr pointer
 * must not be NULL. On the other hand, if the logical flag is up,
 * this function gives logical source and destination addreses and do
 * not provide hoa_addr and rt_addr.
 */
int
mip6_get_ip6hdrinfo(m, src_addr, dst_addr, hoa_addr, rt_addr, logical, presence)
	struct mbuf *m;
	struct in6_addr *src_addr, *dst_addr, *hoa_addr, *rt_addr;
	u_int8_t logical;
	int *presence;
{
	struct ip6_hdr ip6;
	struct ip6_dest ip6d;
	u_int8_t *ip6dbuf;
	struct ip6_rthdr ip6r;
	u_int8_t *ip6o;
	int off, proto, nxt, ip6dlen, ip6olen;

	if (m == NULL || src_addr == NULL || dst_addr == NULL || presence == NULL) {
		mip6log((LOG_ERR, "%s: "
		    "invalid argument (m, src, dst, presence) "
		    "= (%p, %p, %p, %p).\n",
		    __FUNCTION__, m, src_addr, dst_addr, presence));
		return (-1);
	}

	if ((logical == 0) && (hoa_addr == NULL || rt_addr == NULL)) {
		mip6log((LOG_ERR, "%s: "
		    "invalid argument (hoa_addr, rt_addr) = (%p, %p).\n",
		    __FUNCTION__, hoa_addr, rt_addr));
		return (-1);
	}

	*presence = 0;

	/* IPv6 header may be safe to mtod(), but be conservative. */
	m_copydata(m, 0, sizeof(struct ip6_hdr), (void *)&ip6);
	*src_addr = ip6.ip6_src;
	*dst_addr = ip6.ip6_dst;

	off = 0;
	proto = IPPROTO_IPV6;

	/*
	 * extract src and dst addresses from HAO and type 2 routing
	 * header.  Note that we cannot directly access to mbuf
	 * (e.g. by using IP6_EXTHDR_CHECK/GET), since we use this
	 * function in both input/output pathes.  In a output path,
	 * the packet is not located on a contiguous memory.
	 */
	while ((off = ip6_nexthdr(m, off, proto, &nxt)) != -1) {
		proto = nxt;
		if (nxt == IPPROTO_DSTOPTS) {
			m_copydata(m, off, sizeof(struct ip6_dest),
			    (void *)&ip6d);
			ip6dlen = (ip6d.ip6d_len + 1) << 3;

			/* copy entire destopt header. */
			MALLOC(ip6dbuf, u_int8_t *, ip6dlen, M_IP6OPT,
			    M_NOWAIT);
			if (ip6dbuf == NULL) {
				mip6log((LOG_ERR, "%s: "
				    "failed to allocate memory to copy "
				    "destination option.\n", __FUNCTION__));
				return (-1);
			}
			m_copydata(m, off, ip6dlen, ip6dbuf);

			ip6dlen -= sizeof(struct ip6_dest);
			ip6o = ip6dbuf + sizeof(struct ip6_dest);
			for (ip6olen = 0; ip6dlen > 0;
			     ip6dlen -= ip6olen, ip6o += ip6olen) {
				if (*ip6o != IP6OPT_PAD1 &&
				    (ip6dlen < IP6OPT_MINLEN ||
					2 + *(ip6o + 1) > ip6dlen)) {
					mip6log((LOG_ERR,
					    "%s: destopt too small.\n", __FUNCTION__));
					FREE(ip6dbuf, M_IP6OPT);
					return (-1);
				}
				if (*ip6o == IP6OPT_PAD1) {
					/* special case. */
					ip6olen = 1;
				} else if (*ip6o == IP6OPT_HOME_ADDRESS) {
					ip6olen = 2 + *(ip6o + 1);
					if (ip6olen != sizeof(struct ip6_opt_home_address)) {
						mip6log((LOG_ERR,
						    "%s: invalid HAO length.\n", __FUNCTION__));
						FREE(ip6dbuf, M_IP6OPT);
						return (-1);
					}
					if (logical) {
						bcopy(ip6o + 2, src_addr,
						      sizeof(struct in6_addr));
					} else {
						bcopy(ip6o + 2, hoa_addr,
						      sizeof(struct in6_addr));
					}
					*presence |= HOA_PRESENT;
				} else {
					/* ignore other options. */
					ip6olen = 2 + *(ip6o + 1);
				}
			}
			FREE(ip6dbuf, M_IP6OPT);
		}
		if (nxt == IPPROTO_ROUTING) {
			m_copydata(m, off, sizeof(struct ip6_rthdr),
			    (void *)&ip6r);
			/*
			 * only type 2 routing header need to be
			 * checked.
			 */
			if (ip6r.ip6r_type != IPV6_RTHDR_TYPE_2)
				continue;
			if (ip6r.ip6r_len != 2)
				continue;
#if 0
			if (ip6r.ip6r_segleft != 1)
				continue;
#endif
			if (logical) {
				m_copydata(m, off + sizeof(struct ip6_rthdr2),
					   sizeof(struct in6_addr), (void *)dst_addr);
			} else {
				m_copydata(m, off + sizeof(struct ip6_rthdr2),
					   sizeof(struct in6_addr), (void *)rt_addr);
			}
			*presence |= RTHDR_PRESENT;
		}
	}
	return (0);
}


#if NMIP > 0
void
mip6_md_scan(u_int16_t ifindex)
{
        int s;
        struct nd_defrouter *dr;
#ifndef __FreeBSD__
#ifdef APPLE
	time_t time_second;
	struct timeval timenow;

	getmicrotime(&timenow);
	time_second = (time_t)timenow.tv_sec;
#else
#if defined(__NetBSD__) && __NetBSD_Version__ < 400000000
        long time_second = time.tv_sec;
#endif /* __NetBSD__ && __NetBSD_Version__ < 400000000 */
#endif /* APPLE */
#endif /* !__FreeBSD__ */

#if defined(__NetBSD__) || defined(__OpenBSD__)
        s = splsoftnet();
#else
        s = splnet();
#endif

#define MIP6_SCAN_TIME 2
        /* expire default router list */
        dr = TAILQ_FIRST(&nd_defrouter);
        while (dr) {
                if (dr->ifp && dr->ifp->if_index == ifindex) {
                        if (dr->expire > (time_second + MIP6_SCAN_TIME))
                                dr->expire = time_second + MIP6_SCAN_TIME;
                } 
                dr = TAILQ_NEXT(dr, dr_entry);
        }
        splx(s);

        return;
}
#endif /* NMIP > 0 */

struct ifaddr *nd6_dad_find_by_addr(struct in6_addr *);

void
mip6_do_dad(addr, ifidx)
	struct in6_addr *addr;
	int ifidx;
{
	struct ifnet *ifp;
	struct in6_ifaddr *mip6_ifa;	/* pesudo in6_ifaddr to usee nd6_dad_start() */

	if ((mip6_ifa = (struct in6_ifaddr *)nd6_dad_find_by_addr(addr)) != NULL) {
		mip6log((LOG_ERR,
			 "%s is in under DAD. The new request is rejected\n",
			 ip6_sprintf(addr)));
		return;
	}

#ifdef __FreeBSD__
	ifp = ifnet_byindex(ifidx);
#else
	ifp = ifindex2ifnet[ifidx];
#endif
	if (ifp == NULL) {
		mip6log((LOG_ERR,
			 "mip6_do_dad(): Couldn't get ifnet correspondent to if index %d. ", ifidx));
		return;
	}
	
	MALLOC(mip6_ifa, struct in6_ifaddr *, sizeof(*mip6_ifa), M_IFADDR, M_WAIT);
	bzero(mip6_ifa, sizeof(*mip6_ifa));
#ifdef __FreeBSD__
	IFA_LOCK_INIT(&mip6_ifa->ia_ifa);
#endif
	mip6_ifa->ia_ifp = ifp;
	mip6_ifa->ia6_flags |= IN6_IFF_PSEUDOIFA | IN6_IFF_TENTATIVE;
	mip6_ifa->ia_ifa.ifa_addr = (struct sockaddr *)&mip6_ifa->ia_addr;
	mip6_ifa->ia_addr.sin6_addr = *addr;
#ifndef __APPLE__
	if (in6_setscope(&mip6_ifa->ia_addr.sin6_addr, ifp, NULL) != 0) {
		mip6log((LOG_ERR, "in6_setscope failed\n"));
	}
#else
	mip6_ifa->ia_addr.sin6_addr.s6_addr16[1] = htons(ifp->if_index);
#endif 
	mip6_ifa->ia_addr.sin6_family = AF_INET6;
	mip6_ifa->ia_addr.sin6_len = sizeof(struct sockaddr_in6);
	/*
	 * start the DAD procedure.  delay isn't needed in this case,
	 * maybe.
	 */
	nd6_dad_start((struct ifaddr *)mip6_ifa, 0);
}

void
mip6_stop_dad(addr, ifidx)
	struct in6_addr *addr;
	int ifidx;	/* not used at this moment */
{
	struct ifaddr *ifa;
	
	ifa = nd6_dad_find_by_addr(addr);
	if (!ifa)
		nd6_dad_stop(ifa);
}

struct dadq *nd6_dad_find(struct ifaddr *);

/*
 * Running DAD for the link local address on an interface
 * corresponding to the ifindex
 */
void
mip6_do_dad_lladdr(ifidx)
	int ifidx;
{
	struct ifnet *ifp;
	struct ifaddr *ifa;

#ifdef __FreeBSD__
	ifp = ifnet_byindex(ifidx);
#else
	ifp = ifindex2ifnet[ifidx];
#endif
	if (ifp == NULL) {
		mip6log((LOG_ERR, "mip6_do_dad_lladdr(): Couldn't get ifnet"
			    " correspondent to if index %d. ", ifidx));
		return;
	}

	ifa = (struct ifaddr *)in6ifa_ifpforlinklocal(ifp, 0); /* 0 is ok? */

	if (nd6_dad_find(ifa) != NULL) {
		/* DAD already in progress */
		mip6log((LOG_ERR,
			 "mip6_do_dad_lladdr(): DAD already in progress."));
		return;
	}

	((struct in6_ifaddr *)ifa)->ia6_flags |= IN6_IFF_TENTATIVE;
	/*
	 * start the DAD procedure.  delay isn't needed in this case,
	 * maybe.
	 */
	nd6_dad_start((struct ifaddr *)ifa, 0);
}
