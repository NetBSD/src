/*	$KAME: ip_encap.c,v 1.73 2001/10/02 08:30:58 itojun Exp $	*/

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
 * My grandfather said that there's a devil inside tunnelling technology...
 *
 * We have surprisingly many protocols that want packets with IP protocol
 * #4 or #41.  Here's a list of protocols that want protocol #41:
 *	RFC1933 configured tunnel
 *	RFC1933 automatic tunnel
 *	RFC2401 IPsec tunnel
 *	RFC2473 IPv6 generic packet tunnelling
 *	RFC2529 6over4 tunnel
 *	RFC3056 6to4 tunnel
 *	isatap tunnel
 *	mobile-ip6 (uses RFC2473)
 * Here's a list of protocol that want protocol #4:
 *	RFC1853 IPv4-in-IPv4 tunnelling
 *	RFC2003 IPv4 encapsulation within IPv4
 *	RFC2344 reverse tunnelling for mobile-ip4
 *	RFC2401 IPsec tunnel
 * Well, what can I say.  They impose different en/decapsulation mechanism
 * from each other, so they need separate protocol handler.  The only one
 * we can easily determine by protocol # is IPsec, which always has
 * AH/ESP/IPComp header right after outer IP header.
 *
 * So, clearly good old protosw does not work for protocol #4 and #41.
 * The code will let you match protocol via src/dst address pair.
 */
/* XXX is M_NETADDR correct? */

/*
 * With USE_RADIX the code will use radix table for tunnel lookup, for
 * tunnels registered with encap_attach() with a addr/mask pair.
 * Faster on machines with thousands of tunnel registerations (= interfaces).
 *
 * The code assumes that radix table code can handle non-continuous netmask,
 * as it will pass radix table memory region with (src + dst) sockaddr pair.
 *
 * FreeBSD is excluded here as they make max_keylen a static variable, and
 * thus forbid definition of radix table other than proper domains.
 */
#define USE_RADIX

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ip_encap.c,v 1.4.4.4 2002/04/01 07:48:32 nathanw Exp $");

#include "opt_mrouting.h"
#include "opt_inet.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/protosw.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_encap.h>
#ifdef MROUTING
#include <netinet/ip_mroute.h>
#endif /* MROUTING */

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6protosw.h>
#include <netinet6/in6_var.h>
#include <netinet6/in6_pcb.h>
#include <netinet/icmp6.h>
#endif

#include <machine/stdarg.h>

#ifdef MROUTING
#include <netinet/ip_mroute.h>
#endif

#include <net/net_osdep.h>

/* to lookup a pair of address using radix tree */
struct sockaddr_pack {
	u_int8_t sp_len;
	u_int8_t sp_family;	/* not really used */
	/* followed by variable-length data */
} __attribute__((__packed__));

struct pack4 {
	struct sockaddr_pack p;
	struct sockaddr_in mine;
	struct sockaddr_in yours;
} __attribute__((__packed__));
struct pack6 {
	struct sockaddr_pack p;
	struct sockaddr_in6 mine;
	struct sockaddr_in6 yours;
} __attribute__((__packed__));

enum direction { INBOUND, OUTBOUND };

#ifdef INET
static struct encaptab *encap4_lookup __P((struct mbuf *, int, int,
	enum direction));
#endif
#ifdef INET6
static struct encaptab *encap6_lookup __P((struct mbuf *, int, int,
	enum direction));
#endif
static int encap_add __P((struct encaptab *));
static int encap_remove __P((struct encaptab *));
static int encap_afcheck __P((int, const struct sockaddr *, const struct sockaddr *));
#ifdef USE_RADIX
static struct radix_node_head *encap_rnh __P((int));
static int mask_matchlen __P((const struct sockaddr *));
#endif
#ifndef USE_RADIX
static int mask_match __P((const struct encaptab *, const struct sockaddr *,
		const struct sockaddr *));
#endif
static void encap_fillarg __P((struct mbuf *, const struct encaptab *));

LIST_HEAD(, encaptab) encaptab = LIST_HEAD_INITIALIZER(&encaptab);

#ifdef USE_RADIX
extern int max_keylen;	/* radix.c */
struct radix_node_head *encap_head[2];	/* 0 for AF_INET, 1 for AF_INET6 */
#endif

void
encap_init()
{
	static int initialized = 0;

	if (initialized)
		return;
	initialized++;
#if 0
	/*
	 * we cannot use LIST_INIT() here, since drivers may want to call
	 * encap_attach(), on driver attach.  encap_init() will be called
	 * on AF_INET{,6} initialization, which happens after driver
	 * initialization - using LIST_INIT() here can nuke encap_attach()
	 * from drivers.
	 */
	LIST_INIT(&encaptab);
#endif

#ifdef USE_RADIX
	/*
	 * initialize radix lookup table.  
	 * max_keylen initialization should happen before the call to rn_init().
	 */
	rn_inithead((void **)&encap_head[0], sizeof(struct sockaddr_pack) << 3);
	if (sizeof(struct pack4) > max_keylen)
		max_keylen = sizeof(struct pack4);
#ifdef INET6
	rn_inithead((void **)&encap_head[1], sizeof(struct sockaddr_pack) << 3);
	if (sizeof(struct pack6) > max_keylen)
		max_keylen = sizeof(struct pack6);
#endif
#endif
}

#ifdef INET
static struct encaptab *
encap4_lookup(m, off, proto, dir)
	struct mbuf *m;
	int off;
	int proto;
	enum direction dir;
{
	struct ip *ip;
	struct pack4 pack;
	struct encaptab *ep, *match;
	int prio, matchprio;
#ifdef USE_RADIX
	struct radix_node_head *rnh = encap_rnh(AF_INET);
	struct radix_node *rn;
#endif

#ifdef DIAGNOSTIC
	if (m->m_len < sizeof(*ip))
		panic("encap4_lookup");
#endif
	ip = mtod(m, struct ip *);

	bzero(&pack, sizeof(pack));
	pack.p.sp_len = sizeof(pack);
	pack.mine.sin_family = pack.yours.sin_family = AF_INET;
	pack.mine.sin_len = pack.yours.sin_len = sizeof(struct sockaddr_in);
	if (dir == INBOUND) {
		pack.mine.sin_addr = ip->ip_dst;
		pack.yours.sin_addr = ip->ip_src;
	} else {
		pack.mine.sin_addr = ip->ip_src;
		pack.yours.sin_addr = ip->ip_dst;
	}

	match = NULL;
	matchprio = 0;

#ifdef USE_RADIX
	rn = rnh->rnh_matchaddr((caddr_t)&pack, rnh);
	if (rn && (rn->rn_flags & RNF_ROOT) == 0) {
		match = (struct encaptab *)rn;
		matchprio = mask_matchlen(match->srcmask) +
		    mask_matchlen(match->dstmask);
	}
#endif

	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != AF_INET)
			continue;
		if (ep->proto >= 0 && ep->proto != proto)
			continue;
		if (ep->func)
			prio = (*ep->func)(m, off, proto, ep->arg);
		else {
#ifdef USE_RADIX
			continue;
#else
			prio = mask_match(ep, (struct sockaddr *)&pack.mine,
			    (struct sockaddr *)&pack.yours);
#endif
		}

		/*
		 * We prioritize the matches by using bit length of the
		 * matches.  mask_match() and user-supplied matching function
		 * should return the bit length of the matches (for example,
		 * if both src/dst are matched for IPv4, 64 should be returned).
		 * 0 or negative return value means "it did not match".
		 *
		 * The question is, since we have two "mask" portion, we
		 * cannot really define total order between entries.
		 * For example, which of these should be preferred?
		 * mask_match() returns 48 (32 + 16) for both of them.
		 *	src=3ffe::/16, dst=3ffe:501::/32
		 *	src=3ffe:501::/32, dst=3ffe::/16
		 *
		 * We need to loop through all the possible candidates
		 * to get the best match - the search takes O(n) for
		 * n attachments (i.e. interfaces).
		 *
		 * For radix-based lookup, I guess source takes precedence.
		 * See rn_{refines,lexobetter} for the correct answer.
		 */
		if (prio <= 0)
			continue;
		if (prio > matchprio) {
			matchprio = prio;
			match = ep;
		}
	}

	return match;
#undef s
#undef d
}

void
#if __STDC__
encap4_input(struct mbuf *m, ...)
#else
encap4_input(m, va_alist)
	struct mbuf *m;
	va_dcl
#endif
{
	int off, proto;
	va_list ap;
	const struct protosw *psw;
	struct encaptab *match;

	va_start(ap, m);
	off = va_arg(ap, int);
	proto = va_arg(ap, int);
	va_end(ap);

	match = encap4_lookup(m, off, proto, INBOUND);

	if (match) {
		/* found a match, "match" has the best one */
		psw = match->psw;
		if (psw && psw->pr_input) {
			encap_fillarg(m, match);
			(*psw->pr_input)(m, off, proto);
		} else
			m_freem(m);
		return;
	}

	/* last resort: inject to raw socket */
	rip_input(m, off, proto);
}
#endif

#ifdef INET6
static struct encaptab *
encap6_lookup(m, off, proto, dir)
	struct mbuf *m;
	int off;
	int proto;
	enum direction dir;
{
	struct ip6_hdr *ip6;
	struct pack6 pack;
	int prio, matchprio;
	struct encaptab *ep, *match;
#ifdef USE_RADIX
	struct radix_node_head *rnh = encap_rnh(AF_INET6);
	struct radix_node *rn;
#endif

#ifdef DIAGNOSTIC
	if (m->m_len < sizeof(*ip6))
		panic("encap6_lookup");
#endif
	ip6 = mtod(m, struct ip6_hdr *);

	bzero(&pack, sizeof(pack));
	pack.p.sp_len = sizeof(pack);
	pack.mine.sin6_family = pack.yours.sin6_family = AF_INET6;
	pack.mine.sin6_len = pack.yours.sin6_len = sizeof(struct sockaddr_in6);
	if (dir == INBOUND) {
		pack.mine.sin6_addr = ip6->ip6_dst;
		pack.yours.sin6_addr = ip6->ip6_src;
	} else {
		pack.mine.sin6_addr = ip6->ip6_src;
		pack.yours.sin6_addr = ip6->ip6_dst;
	}

	match = NULL;
	matchprio = 0;

#ifdef USE_RADIX
	rn = rnh->rnh_matchaddr((caddr_t)&pack, rnh);
	if (rn && (rn->rn_flags & RNF_ROOT) == 0) {
		match = (struct encaptab *)rn;
		matchprio = mask_matchlen(match->srcmask) +
		    mask_matchlen(match->dstmask);
	}
#endif

	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != AF_INET6)
			continue;
		if (ep->proto >= 0 && ep->proto != proto)
			continue;
		if (ep->func)
			prio = (*ep->func)(m, off, proto, ep->arg);
		else {
#ifdef USE_RADIX
			continue;
#else
			prio = mask_match(ep, (struct sockaddr *)&pack.mine,
			    (struct sockaddr *)&pack.yours);
#endif
		}

		/* see encap4_lookup() for issues here */
		if (prio <= 0)
			continue;
		if (prio > matchprio) {
			matchprio = prio;
			match = ep;
		}
	}

	return match;
#undef s
#undef d
}

int
encap6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp;
	int proto;
{
	struct mbuf *m = *mp;
	const struct ip6protosw *psw;
	struct encaptab *match;

	match = encap6_lookup(m, *offp, proto, INBOUND);

	if (match) {
		/* found a match */
		psw = (const struct ip6protosw *)match->psw;
		if (psw && psw->pr_input) {
			encap_fillarg(m, match);
			return (*psw->pr_input)(mp, offp, proto);
		} else {
			m_freem(m);
			return IPPROTO_DONE;
		}
	}

	/* last resort: inject to raw socket */
	return rip6_input(mp, offp, proto);
}
#endif

static int
encap_add(ep)
	struct encaptab *ep;
{
#ifdef USE_RADIX
	struct radix_node_head *rnh = encap_rnh(ep->af);
#endif
	int error = 0;

	LIST_INSERT_HEAD(&encaptab, ep, chain);
#ifdef USE_RADIX
	if (!ep->func && rnh) {
		if (!rnh->rnh_addaddr((caddr_t)ep->addrpack,
		    (caddr_t)ep->maskpack, rnh, ep->nodes)) {
			error = EEXIST;
			goto fail;
		}
	}
#endif
	return error;

 fail:
	LIST_REMOVE(ep, chain);
	return error;
}

static int
encap_remove(ep)
	struct encaptab *ep;
{
#ifdef USE_RADIX
	struct radix_node_head *rnh = encap_rnh(ep->af);
#endif
	int error = 0;

	LIST_REMOVE(ep, chain);
#ifdef USE_RADIX
	if (!ep->func && rnh) {
		if (!rnh->rnh_deladdr((caddr_t)ep->addrpack,
		    (caddr_t)ep->maskpack, rnh))
			error = ESRCH;
	}
#endif
	return error;
}

static int
encap_afcheck(af, sp, dp)
	int af;
	const struct sockaddr *sp;
	const struct sockaddr *dp;
{
	if (sp && dp) {
		if (sp->sa_len != dp->sa_len)
			return EINVAL;
		if (af != sp->sa_family || af != dp->sa_family)
			return EINVAL;
	} else if (!sp && !dp)
		;
	else
		return EINVAL;

	switch (af) {
	case AF_INET:
		if (sp && sp->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		if (dp && dp->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		break;
#ifdef INET6
	case AF_INET6:
		if (sp && sp->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;
		if (dp && dp->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;
		break;
#endif
	default:
		return EAFNOSUPPORT;
	}

	return 0;
}

/*
 * sp (src ptr) is always my side, and dp (dst ptr) is always remote side.
 * length of mask (sm and dm) is assumed to be same as sp/dp.
 * Return value will be necessary as input (cookie) for encap_detach().
 */
const struct encaptab *
encap_attach(af, proto, sp, sm, dp, dm, psw, arg)
	int af;
	int proto;
	const struct sockaddr *sp, *sm;
	const struct sockaddr *dp, *dm;
	const struct protosw *psw;
	void *arg;
{
	struct encaptab *ep;
	int error;
	int s;
	size_t l;
	struct pack4 *pack4;
#ifdef INET6
	struct pack6 *pack6;
#endif

	s = splsoftnet();
	/* sanity check on args */
	error = encap_afcheck(af, sp, dp);
	if (error)
		goto fail;

	/* check if anyone have already attached with exactly same config */
	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != af)
			continue;
		if (ep->proto != proto)
			continue;
		if (ep->func)
			continue;
#ifdef DIAGNOSTIC
		if (!ep->src || !ep->dst || !ep->srcmask || !ep->dstmask)
			panic("null pointers in encaptab");
#endif
		if (ep->src->sa_len != sp->sa_len ||
		    bcmp(ep->src, sp, sp->sa_len) != 0 ||
		    bcmp(ep->srcmask, sm, sp->sa_len) != 0)
			continue;
		if (ep->dst->sa_len != dp->sa_len ||
		    bcmp(ep->dst, dp, dp->sa_len) != 0 ||
		    bcmp(ep->dstmask, dm, dp->sa_len) != 0)
			continue;

		error = EEXIST;
		goto fail;
	}

	switch (af) {
	case AF_INET:
		l = sizeof(*pack4);
		break;
#ifdef INET6
	case AF_INET6:
		l = sizeof(*pack6);
		break;
#endif
	default:
		goto fail;
	}

#ifdef DIAGNOSTIC
	/* if l exceeds the value sa_len can possibly express, it's wrong. */
	if (l > (1 << (8 * sizeof(ep->addrpack->sa_len)))) {
		error = EINVAL;
		goto fail;
	}
#endif

	ep = malloc(sizeof(*ep), M_NETADDR, M_NOWAIT);	/* M_NETADDR ok? */
	if (ep == NULL) {
		error = ENOBUFS;
		goto fail;
	}
	bzero(ep, sizeof(*ep));
	ep->addrpack = malloc(l, M_NETADDR, M_NOWAIT);
	if (ep->addrpack == NULL) {
		error = ENOBUFS;
		goto gc;
	}
	ep->maskpack = malloc(l, M_NETADDR, M_NOWAIT);
	if (ep->maskpack == NULL) {
		error = ENOBUFS;
		goto gc;
	}

	ep->af = af;
	ep->proto = proto;
	ep->addrpack->sa_len = l & 0xff;
	ep->maskpack->sa_len = l & 0xff;
	switch (af) {
	case AF_INET:
		pack4 = (struct pack4 *)ep->addrpack;
		ep->src = (struct sockaddr *)&pack4->mine;
		ep->dst = (struct sockaddr *)&pack4->yours;
		pack4 = (struct pack4 *)ep->maskpack;
		ep->srcmask = (struct sockaddr *)&pack4->mine;
		ep->dstmask = (struct sockaddr *)&pack4->yours;
		break;
#ifdef INET6
	case AF_INET6:
		pack6 = (struct pack6 *)ep->addrpack;
		ep->src = (struct sockaddr *)&pack6->mine;
		ep->dst = (struct sockaddr *)&pack6->yours;
		pack6 = (struct pack6 *)ep->maskpack;
		ep->srcmask = (struct sockaddr *)&pack6->mine;
		ep->dstmask = (struct sockaddr *)&pack6->yours;
		break;
#endif
	}

	bcopy(sp, ep->src, sp->sa_len);
	bcopy(sm, ep->srcmask, sp->sa_len);
	bcopy(dp, ep->dst, dp->sa_len);
	bcopy(dm, ep->dstmask, dp->sa_len);
	ep->psw = psw;
	ep->arg = arg;

	error = encap_add(ep);
	if (error)
		goto gc;

	error = 0;
	splx(s);
	return ep;

gc:
	if (ep->addrpack)
		free(ep->addrpack, M_NETADDR);
	if (ep->maskpack)
		free(ep->maskpack, M_NETADDR);
	if (ep)
		free(ep, M_NETADDR);
fail:
	splx(s);
	return NULL;
}

const struct encaptab *
encap_attach_func(af, proto, func, psw, arg)
	int af;
	int proto;
	int (*func) __P((const struct mbuf *, int, int, void *));
	const struct protosw *psw;
	void *arg;
{
	struct encaptab *ep;
	int error;
	int s;

	s = splsoftnet();
	/* sanity check on args */
	if (!func) {
		error = EINVAL;
		goto fail;
	}

	error = encap_afcheck(af, NULL, NULL);
	if (error)
		goto fail;

	ep = malloc(sizeof(*ep), M_NETADDR, M_NOWAIT);	/*XXX*/
	if (ep == NULL) {
		error = ENOBUFS;
		goto fail;
	}
	bzero(ep, sizeof(*ep));

	ep->af = af;
	ep->proto = proto;
	ep->func = func;
	ep->psw = psw;
	ep->arg = arg;

	error = encap_add(ep);
	if (error)
		goto fail;

	error = 0;
	splx(s);
	return ep;

fail:
	splx(s);
	return NULL;
}

/* XXX encap4_ctlinput() is necessary if we set DF=1 on outer IPv4 header */

#ifdef INET6
void
encap6_ctlinput(cmd, sa, d0)
	int cmd;
	struct sockaddr *sa;
	void *d0;
{
	void *d = d0;
	struct ip6_hdr *ip6;
	struct mbuf *m;
	int off;
	struct ip6ctlparam *ip6cp = NULL;
	const struct sockaddr_in6 *sa6_src = NULL;
	void *cmdarg;
	int nxt;
	struct encaptab *ep;
	const struct ip6protosw *psw;

	if (sa->sa_family != AF_INET6 ||
	    sa->sa_len != sizeof(struct sockaddr_in6))
		return;

	if ((unsigned)cmd >= PRC_NCMDS)
		return;
	if (cmd == PRC_HOSTDEAD)
		d = NULL;
	else if (cmd == PRC_MSGSIZE)
		; /* special code is present, see below */
	else if (inet6ctlerrmap[cmd] == 0)
		return;

	/* if the parameter is from icmp6, decode it. */
	if (d != NULL) {
		ip6cp = (struct ip6ctlparam *)d;
		m = ip6cp->ip6c_m;
		ip6 = ip6cp->ip6c_ip6;
		off = ip6cp->ip6c_off;
		cmdarg = ip6cp->ip6c_cmdarg;
		sa6_src = ip6cp->ip6c_src;
		nxt = ip6cp->ip6c_nxt;
	} else {
		m = NULL;
		ip6 = NULL;
		cmdarg = NULL;
		sa6_src = &sa6_any;
		nxt = -1;
	}

	if (ip6 && cmd == PRC_MSGSIZE) {
		int valid = 0;
		struct encaptab *match;

		/*
		 * Check to see if we have a valid encap configuration.
		 */
		match = encap6_lookup(m, off, nxt, OUTBOUND);

		if (match)
			valid++;

		/*
		 * Depending on the value of "valid" and routing table
		 * size (mtudisc_{hi,lo}wat), we will:
		 * - recalcurate the new MTU and create the
		 *   corresponding routing entry, or
		 * - ignore the MTU change notification.
		 */
		icmp6_mtudisc_update((struct ip6ctlparam *)d, valid);
	}

	/* inform all listeners */
	for (ep = LIST_FIRST(&encaptab); ep; ep = LIST_NEXT(ep, chain)) {
		if (ep->af != AF_INET6)
			continue;
		if (ep->proto >= 0 && ep->proto != nxt)
			continue;

		/* should optimize by looking at address pairs */

		/* XXX need to pass ep->arg or ep itself to listeners */
		psw = (const struct ip6protosw *)ep->psw;
		if (psw && psw->pr_ctlinput)
			(*psw->pr_ctlinput)(cmd, sa, d);
	}

	rip6_ctlinput(cmd, sa, d0);
}
#endif

int
encap_detach(cookie)
	const struct encaptab *cookie;
{
	const struct encaptab *ep = cookie;
	struct encaptab *p;
	int error;

	for (p = LIST_FIRST(&encaptab); p; p = LIST_NEXT(p, chain)) {
		if (p == ep) {
			error = encap_remove(p);
			if (error)
				return error;
			if (!ep->func) {
				free(p->addrpack, M_NETADDR);
				free(p->maskpack, M_NETADDR);
			}
			free(p, M_NETADDR);	/*XXX*/
			return 0;
		}
	}

	return ENOENT;
}

#ifdef USE_RADIX
static struct radix_node_head *
encap_rnh(af)
	int af;
{

	switch (af) {
	case AF_INET:
		return encap_head[0];
#ifdef INET6
	case AF_INET6:
		return encap_head[1];
#endif
	default:
		return NULL;
	}
}

static int
mask_matchlen(sa)
	const struct sockaddr *sa;
{
	const char *p, *ep;
	int l;

	p = (const char *)sa;
	ep = p + sa->sa_len;
	p += 2;	/* sa_len + sa_family */

	l = 0;
	while (p < ep) {
		l += (*p ? 8 : 0);	/* estimate */
		p++;
	}
	return l;
}
#endif

#ifndef USE_RADIX
static int
mask_match(ep, sp, dp)
	const struct encaptab *ep;
	const struct sockaddr *sp;
	const struct sockaddr *dp;
{
	struct sockaddr_storage s;
	struct sockaddr_storage d;
	int i;
	const u_int8_t *p, *q;
	u_int8_t *r;
	int matchlen;

#ifdef DIAGNOSTIC
	if (ep->func)
		panic("wrong encaptab passed to mask_match");
#endif
	if (sp->sa_len > sizeof(s) || dp->sa_len > sizeof(d))
		return 0;
	if (sp->sa_family != ep->af || dp->sa_family != ep->af)
		return 0;
	if (sp->sa_len != ep->src->sa_len || dp->sa_len != ep->dst->sa_len)
		return 0;

	matchlen = 0;

	p = (const u_int8_t *)sp;
	q = (const u_int8_t *)ep->srcmask;
	r = (u_int8_t *)&s;
	for (i = 0 ; i < sp->sa_len; i++) {
		r[i] = p[i] & q[i];
		/* XXX estimate */
		matchlen += (q[i] ? 8 : 0);
	}

	p = (const u_int8_t *)dp;
	q = (const u_int8_t *)ep->dstmask;
	r = (u_int8_t *)&d;
	for (i = 0 ; i < dp->sa_len; i++) {
		r[i] = p[i] & q[i];
		/* XXX rough estimate */
		matchlen += (q[i] ? 8 : 0);
	}

	/* need to overwrite len/family portion as we don't compare them */
	s.ss_len = sp->sa_len;
	s.ss_family = sp->sa_family;
	d.ss_len = dp->sa_len;
	d.ss_family = dp->sa_family;

	if (bcmp(&s, ep->src, ep->src->sa_len) == 0 &&
	    bcmp(&d, ep->dst, ep->dst->sa_len) == 0) {
		return matchlen;
	} else
		return 0;
}
#endif

static void
encap_fillarg(m, ep)
	struct mbuf *m;
	const struct encaptab *ep;
{
	struct mbuf *n;

	n = m_aux_add(m, AF_INET, IPPROTO_IPV4);
	if (n) {
		*mtod(n, void **) = ep->arg;
		n->m_len = sizeof(void *);
	}
}

void *
encap_getarg(m)
	struct mbuf *m;
{
	void *p;
	struct mbuf *n;

	p = NULL;
	n = m_aux_find(m, AF_INET, IPPROTO_IPV4);
	if (n) {
		if (n->m_len == sizeof(void *))
			p = *mtod(n, void **);
		m_aux_delete(m, n);
	}
	return p;
}
