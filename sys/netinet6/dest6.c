/*	$NetBSD: dest6.c,v 1.15.44.1 2008/02/22 02:53:33 keiichi Exp $	*/
/*	$KAME: dest6.c,v 1.25 2001/02/22 01:39:16 itojun Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dest6.c,v 1.15.44.1 2008/02/22 02:53:33 keiichi Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet/icmp6.h>

#ifdef MOBILE_IPV6
#include <net/mipsock.h>
#include <netinet/ip6mh.h>
#include <netinet6/mip6.h>
#include <netinet6/mip6_var.h>

static int dest6_swap_hao(struct ip6_hdr *, struct ip6aux *,
    struct ip6_opt_home_address *);
static int dest6_nextopt(struct mbuf *, int, struct ip6_opt *);
static void mip6_notify_be_hint(struct in6_addr *, struct in6_addr *,
    struct in6_addr *, u_int8_t);
#endif /* MOBILE_IPV6 */

/*
 * Destination options header processing.
 */
int
dest6_input(struct mbuf **mp, int *offp, int proto)
{
	struct mbuf *m = *mp;
	int off = *offp, dstoptlen, optlen;
	struct ip6_dest *dstopts;
	u_int8_t *opt;
#ifdef MOBILE_IPV6
	struct m_tag *n;
	struct in6_addr home;
	struct ip6aux *ip6a = NULL;
	struct ip6_opt_home_address *haopt = NULL;
	struct ip6_hdr *ip6;
	struct mip6_bc_internal *bce;
	int verified = 0;

	ip6 = mtod(m, struct ip6_hdr *);
#endif /* MOBILE_IPV6 */
	/* validation of the length of the header */
	IP6_EXTHDR_GET(dstopts, struct ip6_dest *, m, off, sizeof(*dstopts));
	if (dstopts == NULL)
		return IPPROTO_DONE;
	dstoptlen = (dstopts->ip6d_len + 1) << 3;

	IP6_EXTHDR_GET(dstopts, struct ip6_dest *, m, off, dstoptlen);
	if (dstopts == NULL)
		return IPPROTO_DONE;
	off += dstoptlen;
	dstoptlen -= sizeof(struct ip6_dest);
	opt = (u_int8_t *)dstopts + sizeof(struct ip6_dest);

	/* search header for all options. */
	for (optlen = 0; dstoptlen > 0; dstoptlen -= optlen, opt += optlen) {
		if (*opt != IP6OPT_PAD1 &&
		    (dstoptlen < IP6OPT_MINLEN || *(opt + 1) + 2 > dstoptlen)) {
			ip6stat.ip6s_toosmall++;
			goto bad;
		}

		switch (*opt) {
		case IP6OPT_PAD1:
			optlen = 1;
			break;
		case IP6OPT_PADN:
			optlen = *(opt + 1) + 2;
			break;
#ifdef MOBILE_IPV6
		case IP6OPT_HOME_ADDRESS:
			/* HAO must appear only once */
			if ((mip6_nodetype &
			     (MIP6_NODETYPE_HOME_AGENT |
			      MIP6_NODETYPE_CORRESPONDENT_NODE)) == 0)
				goto nomip;
			n = ip6_addaux(m);
			if (!n) {
				/* not enough core */
				goto bad;
			}
			ip6a = (struct ip6aux *) (n + 1);
			if ((ip6a->ip6a_osrc_flags & IP6A_HASEEN) != 0) {
				/* XXX icmp6 paramprob? */
				goto bad;
			}

			haopt = (struct ip6_opt_home_address *)opt;
			optlen = haopt->ip6oh_len + 2;

			if (optlen != sizeof(*haopt)) {
				ip6stat.ip6s_toosmall++;
				goto bad;
			}

			/* XXX check header ordering */

			bcopy(haopt->ip6oh_addr, &home,
			    sizeof(struct in6_addr));

			bcopy(&home, &ip6a->ip6a_coa, sizeof(ip6a->ip6a_coa));
			ip6a->ip6a_osrc_flags |= IP6A_HASEEN;

#if 0
			mip6stat.mip6s_hao++;
#endif

			/* check whether this HAO is 'verified'. */
			if (
				(bce = mip6_bce_get(&home, &ip6->ip6_dst, &ip6->ip6_src, 0))
				&& IN6_ARE_ADDR_EQUAL(&bce->mbc_coa, &ip6->ip6_src)) {
				/*
				 * we have a corresponding binding
				 * cache entry for the home address
				 * includes in this HAO.
				 */
				verified = 1;
			}
			/*
			 * we have neither a corresponding binding
			 * cache nor ESP header. we have no clue to
			 * beleive this HAO is a correct one.
			 */
			break;
		nomip:
#endif /* MOBILE_IPV6 */
		default:		/* unknown option */
			optlen = ip6_unknown_opt(opt, m,
			    opt - mtod(m, u_int8_t *)
#ifdef __APPLE__
						 ,0
#endif					 
				);
			if (optlen == -1)
				return (IPPROTO_DONE);
			optlen += 2;
			break;
		}
	}

#ifdef MOBILE_IPV6
	/*
	 * if we are sure that the contents of the Home Address option
	 * is valid, swap the source address and the home address
	 * stored in the Home Address option.
	 */
	if (verified && dest6_swap_hao(ip6, ip6a, haopt) < 0)
		goto bad;
#endif /* MOBILE_IPV6 */
	*offp = off;
	return (dstopts->ip6d_nxt);

  bad:
	m_freem(m);
	return (IPPROTO_DONE);
}

int
dest6_mip6_hao(m, mhoff, nxt)
	struct mbuf *m;
	int mhoff, nxt;
{
#ifdef MOBILE_IPV6
	struct ip6_hdr *ip6;
	struct ip6aux *ip6a;
	struct ip6_opt ip6o;
	struct m_tag *n;
	struct in6_addr home;
	struct ip6_opt_home_address haopt;
	int newoff, off, proto, swap;

	/* XXX should care about destopt1 and destopt2.  in destopt2,
           hao and src must be swapped. */
	if ((nxt == IPPROTO_HOPOPTS) || (nxt == IPPROTO_DSTOPTS)) {
		return (0);
	}

	n = ip6_findaux(m);
	if (!n)
		return (0);
	ip6a = (struct ip6aux *) (n + 1);
	if ((ip6a->ip6a_osrc_flags & (IP6A_HASEEN | IP6A_SWAP)) != IP6A_HASEEN)
		return (0);

	ip6 = mtod(m, struct ip6_hdr *);
	/* find home address */
	off = 0;
	proto = IPPROTO_IPV6;
	while (proto != IPPROTO_DSTOPTS) {
		int nxth;
		newoff = ip6_nexthdr(m, off, proto, &nxth);
		if (newoff < 0 || newoff < off)
			return (0);	/* XXX */
		off = newoff;
		proto = nxth;
	}
	ip6o.ip6o_type = IP6OPT_PADN;
	ip6o.ip6o_len = 0;
	while (ip6o.ip6o_type != IP6OPT_HOME_ADDRESS) {
		off = dest6_nextopt(m, off, &ip6o);
		if (off < 0)
			return (0);	/* XXX */
	}
	m_copydata(m, off, sizeof(struct ip6_opt_home_address),
	    (void *)&haopt);

	swap = 0;
	if (nxt == IPPROTO_AH || nxt == IPPROTO_ESP || nxt == IPPROTO_MH)
		swap = 1;

	home = *(struct in6_addr *)haopt.ip6oh_addr;
	/* reject invalid home addresses. */
	if (IN6_IS_ADDR_MULTICAST(&home) ||
	    IN6_IS_ADDR_LINKLOCAL(&home) ||
	    IN6_IS_ADDR_V4MAPPED(&home)  ||
	    IN6_IS_ADDR_UNSPECIFIED(&home) ||
	    IN6_IS_ADDR_LOOPBACK(&home)) {
		ip6stat.ip6s_badscope++;
		if (nxt == IPPROTO_MH) {
			/*
			 * a binding error message is sent only when
			 * the received packet is not a binding update
			 * message.
			 */
			return (0);
		}
		/* notify to send a binding error by the mobility socket. */
		(void)mip6_notify_be_hint(&ip6->ip6_src, &ip6->ip6_dst, &home,
		    IP6_MH_BES_UNKNOWN_HAO);
		return (-1);
	}

	if (swap) {
		int error;
		error = dest6_swap_hao(ip6, ip6a, &haopt);
		if (error)
			return (error);
		m_copyback(m, off, sizeof(struct ip6_opt_home_address),
		    (void *)&haopt);		/* XXX */
		return (0);
	}

	/* reject */
/*	mip6stat.mip6s_unverifiedhao++;*/

	/* notify to send a binding error by the mobility socket. */
	(void)mip6_notify_be_hint(&ip6->ip6_src, &ip6->ip6_dst, &home,
	    IP6_MH_BES_UNKNOWN_HAO);

	return (-1);
#else
	return (0);
#endif /* MOBILE_IPV6 */
}

#ifdef MOBILE_IPV6
static int
dest6_swap_hao(ip6, ip6a, haopt)
	struct ip6_hdr *ip6;
	struct ip6aux *ip6a;
	struct ip6_opt_home_address *haopt;
{

	if ((ip6a->ip6a_osrc_flags & (IP6A_HASEEN | IP6A_SWAP)) != IP6A_HASEEN)
		return (EINVAL);

	/* XXX should we do this at all?  do it now or later? */
	/* XXX interaction with RFC3542 IPV6_RECVDSTOPT */
	/* XXX interaction with ipsec - should be okay */
	/* XXX icmp6 responses is modified - which is bad */
	bcopy(&ip6->ip6_src, &ip6a->ip6a_coa, sizeof(ip6a->ip6a_coa));
	bcopy(haopt->ip6oh_addr, &ip6->ip6_src, sizeof(ip6->ip6_src));
	bcopy(&ip6a->ip6a_coa, haopt->ip6oh_addr, sizeof(haopt->ip6oh_addr));
#if 0
	/* XXX linklocal address is (currently) not supported */
	if ((error = in6_setscope(&ip6->ip6_src, m->m_pkthdr.rcvif,
	    NULL)) != 0)
		return (error);
#endif
	ip6a->ip6a_osrc_flags |= IP6A_SWAP;

	return (0);
}

static int
dest6_nextopt(m, off, ip6o)
	struct mbuf *m;
	int off;
	struct ip6_opt *ip6o;
{
	u_int8_t type;

	if (ip6o->ip6o_type != IP6OPT_PAD1)
		off += 2 + ip6o->ip6o_len;
	else
		off += 1;
	if (m->m_pkthdr.len < off + 1)
		return -1;
	m_copydata(m, off, sizeof(type), (void *)&type);

	switch (type) {
	case IP6OPT_PAD1:
		ip6o->ip6o_type = type;
		ip6o->ip6o_len = 0;
		return off;
	default:
		if (m->m_pkthdr.len < off + 2)
			return -1;
		m_copydata(m, off, sizeof(ip6o), (void *)ip6o);
		if (m->m_pkthdr.len < off + 2 + ip6o->ip6o_len)
			return -1;
		return off;
	}
}

static void
mip6_notify_be_hint(src, coa, hoa, status)
	struct in6_addr *src;
	struct in6_addr *coa;
	struct in6_addr *hoa;
	u_int8_t status;
{
	struct sockaddr_in6 src_sin6, coa_sin6, hoa_sin6;

	bzero(&src_sin6, sizeof(struct sockaddr_in6));
	bzero(&coa_sin6, sizeof(struct sockaddr_in6));
	bzero(&hoa_sin6, sizeof(struct sockaddr_in6));

	src_sin6.sin6_len = sizeof(struct sockaddr_in6);
	src_sin6.sin6_family = AF_INET6;
	src_sin6.sin6_addr = *src;

	coa_sin6.sin6_len = sizeof(struct sockaddr_in6);
	coa_sin6.sin6_family = AF_INET6;
	coa_sin6.sin6_addr = *coa;

	hoa_sin6.sin6_len = sizeof(struct sockaddr_in6);
	hoa_sin6.sin6_family = AF_INET6;
	hoa_sin6.sin6_addr = *hoa;

	mips_notify_be_hint((struct sockaddr *)&src_sin6,
	    (struct sockaddr *)&coa_sin6,
	    (struct sockaddr *)&hoa_sin6, status);
}
#endif /* MOBILE_IPV6 */
