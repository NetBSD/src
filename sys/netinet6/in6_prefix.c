/*	$NetBSD: in6_prefix.c,v 1.3 1999/07/03 21:30:18 thorpej Exp $	*/

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
 * Copyright (c) 1982, 1986, 1991, 1993
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 *	@(#)in.c	8.2 (Berkeley) 11/15/93
 */

#include <sys/param.h>
#if !defined(__FreeBSD__) || __FreeBSD__ < 3
#include <sys/ioctl.h>
#endif
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/syslog.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet6/nd6.h>

extern int	in6_interfaces;

/*
 * Copy bits from src to tgt, from off bit for len bits.
 * Caller must specify collect tgtsize and srcsize.
 */
static void
bit_copy(char *tgt, u_int tgtsize, char *src, u_int srcsize,
	 u_int off, u_int len)
{
	char *sp, *tp;

	/* arg values check */
	if (srcsize < off || srcsize < (off + len) ||
	    tgtsize < off || tgtsize < (off + len)) {
		log(LOG_ERR,
		    "in6_prefix.c: bit_copy: invalid args: srcsize %d,\n"
		    "tgtsize %d, off %d, len %d\n", srcsize, tgtsize, off,
		    len);
		return;
	}

	/* search start point */
	for (sp = src, tp = tgt; off >= 8; sp++, tp++)
		off-=8;
	/* copy starting bits */
	if (off) {
		char setbit;
		int startbits;

		startbits = min((8 - off), len);

		for (setbit = (0x80 >> off); startbits;
		     setbit >>= 1, startbits--, len--)
				*tp  |= (setbit & *sp);
		tp++;
		sp++;
	}
	/* copy midium bits */
	for (; len >= 8; sp++, tp++) {
		*tp = *sp;
		len-=8;
	}
	/* copy ending bits */
	if (len) {
		char setbit;

		for (setbit = 0x80; len; setbit >>= 1, len--)
			*tp  |= (setbit & *sp);
	}
}

/*
 * Search prefix which matches arg prefix as specified in
 * draft-ietf-ipngwg-router-renum-08.txt
 */
static struct nd_prefix *
search_matched_prefix(struct ifnet *ifp, struct in6_prefixreq *ipr)
{
	struct ifprefix *ifpr;

	/* search matched prefix */
	for (ifpr = ifp->if_prefixlist; ifpr; ifpr = ifpr->ifpr_next) {
		if (ifpr->ifpr_prefix->sa_family != AF_INET6)
 			continue;
		if (ipr->ipr_plen <= in6_matchlen(&ipr->ipr_prefix.sin6_addr,
						  &IFPR_IN6(ifpr)))
			break;
	}
	/*
	 * search matched addr, and then search prefix
	 * which matches the addr
	 */
	if (ifpr == 0) {
		struct ifaddr *ifa;

#ifndef __NetBSD__
		for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
#else
		for (ifa = ifp->if_addrlist.tqh_first; ifa; ifa = ifa->ifa_list.tqe_next)
#endif
		{
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			if (ipr->ipr_plen <=
			    in6_matchlen(&ipr->ipr_prefix.sin6_addr,
					 &IFA_IN6(ifa)))
				break;
		}
		if (ifa) {
			for (ifpr = ifp->if_prefixlist; ifpr;
			     ifpr = ifpr->ifpr_next) {
				if (ifpr->ifpr_prefix->sa_family != AF_INET6)
					continue;
				if (ifpr->ifpr_plen <=
				    in6_matchlen(&IFA_IN6(ifa),
						 &IFPR_IN6(ifpr)))
					break;
			}
		}
	}
	return ifpr2ndpr(ifpr);
}

/*
 * Search prefix which matches arg prefix as specified in
 * draft-ietf-ipngwg-router-renum-08.txt, and mark it if exists.
 * Return 1 if anything matched, and 0 if nothing matched.
 */
static int
mark_matched_prefixes(u_long cmd, struct ifnet *ifp, struct in6_rrenumreq *irr)
{
	struct ifprefix *ifpr;
	struct ifaddr *ifa;
	int matchlen, matched = 0;

	/* search matched prefixes */
	for (ifpr = ifp->if_prefixlist; ifpr; ifpr = ifpr->ifpr_next) {
		if (ifpr->ifpr_prefix->sa_family != AF_INET6)
			continue;
		matchlen = in6_matchlen(&irr->irr_matchprefix.sin6_addr,
					&IFPR_IN6(ifpr));
		if (irr->irr_m_minlen > ifpr->ifpr_plen ||
		    irr->irr_m_maxlen < ifpr->ifpr_plen ||
		    irr->irr_m_len > matchlen)
 			continue;
		matched = 1;
		ifpr2ndpr(ifpr)->ndpr_statef_addmark = 1;
		if (cmd == SIOCCIFPREFIX_IN6)
			ifpr2ndpr(ifpr)->ndpr_statef_delmark = 1;
	}

	/*
	 * search matched addr, and then search prefixes
	 * which matche the addr
	 */
#ifndef __NetBSD__
	for (ifa = ifp->if_addrlist; ifa; ifa = ifa->ifa_next)
#else
	for (ifa = ifp->if_addrlist.tqh_first; ifa; ifa = ifa->ifa_list.tqe_next)
#endif
	{
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		matchlen = in6_matchlen(&irr->irr_matchprefix.sin6_addr,
					&IFA_IN6(ifa));
		if (irr->irr_m_minlen > matchlen ||
		    irr->irr_m_maxlen < matchlen || irr->irr_m_len > matchlen)
 			continue;
		for (ifpr = ifp->if_prefixlist; ifpr; ifpr = ifpr->ifpr_next) {
			if (ifpr->ifpr_prefix->sa_family != AF_INET6)
				continue;
			if (ifpr->ifpr_plen > in6_matchlen(&IFA_IN6(ifa),
							   &IFPR_IN6(ifpr)))
				continue;
			matched = 1;
			ifpr2ndpr(ifpr)->ndpr_statef_addmark = 1;
			if (cmd == SIOCCIFPREFIX_IN6)
				ifpr2ndpr(ifpr)->ndpr_statef_delmark = 1;
		}
	}
	return matched;
}

/*
 * Mark global prefixes as to be deleted.
 * Return 1 if anything matched, and 0 if nothing matched.
 */
static void
delmark_global_prefixes(struct ifnet *ifp, struct in6_rrenumreq *irr)
{
	struct ifprefix *ifpr;

	/* search matched prefixes */
	for (ifpr = ifp->if_prefixlist; ifpr; ifpr = ifpr->ifpr_next) {
		if (ifpr->ifpr_prefix->sa_family != AF_INET6)
 			continue;
		/* mark delete global prefix */
		if (in6_addrscope(&irr->irr_matchprefix.sin6_addr) ==
		    IPV6_ADDR_SCOPE_GLOBAL)
			ifpr2ndpr(ifpr)->ndpr_statef_delmark = 1;
	}
}

/* Unmark prefixes */
static void
unmark_prefixes(struct ifnet *ifp)
{
	struct ifprefix *ifpr;

	/* unmark all prefix */
	for (ifpr = ifp->if_prefixlist; ifpr; ifpr = ifpr->ifpr_next) {
		if (ifpr->ifpr_prefix->sa_family != AF_INET6)
 			continue;
		/* unmark prefix */
		ifpr2ndpr(ifpr)->ndpr_statef_addmark = 0;
		ifpr2ndpr(ifpr)->ndpr_statef_delmark = 0;
	}
}

static int
add_each_prefix(struct nd_prefix *ndpr)
{
	int error = 0;

	if ((error = in6_init_prefix_ltimes(ndpr)) != 0)
		return error;
	if ((error = prelist_update(ndpr, 0, NULL)) != 0)
		return error;
	if ((ndpr->ndpr_ifp->if_flags & IFF_LOOPBACK) == 0)
		in6_interfaces++;	/*XXX*/

	return error;
}

static void
add_useprefixes(struct ifnet *ifp, struct in6_rrenumreq *irr)
{
	struct ifprefix *ifpr, *nextifpr;

	/* add prefixes to each of marked prefix */
	for (ifpr = ifp->if_prefixlist; ifpr; ifpr = nextifpr) {
		nextifpr = ifpr->ifpr_next;
		if (ifpr->ifpr_prefix->sa_family != AF_INET6)
 			continue;
		if (ifpr2ndpr(ifpr)->ndpr_statef_addmark) {
			struct nd_prefix ndpr;

			/* init ndpr */
			bzero((caddr_t)&ndpr, sizeof(ndpr));
			ndpr.ndpr_ifp = ifp;
			ndpr.ndpr_plen = irr->irr_u_uselen +
					 irr->irr_u_keeplen;
			ndpr.ndpr_prefix.sin6_len = sizeof(ndpr.ndpr_prefix);
			ndpr.ndpr_prefix.sin6_family = AF_INET6;
			bit_copy((char *)&ndpr.ndpr_prefix.sin6_addr,
				  sizeof(ndpr.ndpr_prefix.sin6_addr) << 3,
				 (char *)&irr->irr_useprefix.sin6_addr,
				 sizeof(irr->irr_useprefix.sin6_addr) << 3,
				 0, irr->irr_u_uselen);
			bit_copy((char *)&ndpr.ndpr_prefix.sin6_addr,
				 sizeof(ndpr.ndpr_prefix.sin6_addr) << 3,
				 (char *)&
				 ifpr2ndpr(ifpr)->ndpr_prefix.sin6_addr,
				 sizeof(ifpr2ndpr(ifpr)->ndpr_prefix.sin6_addr)
				 << 3,
				 irr->irr_u_uselen, irr->irr_u_keeplen);
			ndpr.ndpr_vltime = irr->irr_vltime;
			ndpr.ndpr_pltime = irr->irr_pltime;
			ndpr.ndpr_raf_onlink = irr->irr_raf_mask_onlink ?
				irr->irr_raf_onlink :
				ifpr2ndpr(ifpr)->ndpr_raf_onlink;
			ndpr.ndpr_raf_auto = irr->irr_raf_mask_auto ?
				irr->irr_raf_auto :
				ifpr2ndpr(ifpr)->ndpr_raf_auto;
			/* Is some FlagMasks for rrf necessary? */
			ndpr.ndpr_rrf = irr->irr_rrf;
			ndpr.ndpr_origin = irr->irr_origin;

			(void)add_each_prefix(&ndpr);
		}
	}
}

static int
delete_each_prefix(struct nd_prefix *ndpr, u_char origin)
{
	int error = 0;

	if (ndpr->ndpr_origin > origin)
		return(EPERM);
	if (!IN6_IS_ADDR_UNSPECIFIED(&ndpr->ndpr_addr))
		in6_ifdel(ndpr->ndpr_ifp, &ndpr->ndpr_addr);
	/* xxx ND_OPT_PI_FLAG_ONLINK processing */

	prelist_remove(ndpr);

	return error;
}

static void
delete_prefixes(struct ifnet *ifp, u_char origin)
{
	struct ifprefix *ifpr, *nextifpr;

	/* delete prefixes marked as tobe deleted */
	for (ifpr = ifp->if_prefixlist; ifpr; ifpr = nextifpr) {
		nextifpr = ifpr->ifpr_next;
		if (ifpr->ifpr_prefix->sa_family != AF_INET6)
 			continue;
		if (ifpr2ndpr(ifpr)->ndpr_statef_delmark)
			(void)delete_each_prefix(ifpr2ndpr(ifpr), origin);
	}
}

int
in6_prefix_ioctl(u_long cmd, caddr_t data, struct ifnet *ifp)
{
	struct nd_prefix *ndpr, ndpr_tmp;
	struct in6_prefixreq *ipr = (struct in6_prefixreq *)data;
	struct in6_rrenumreq *irr = (struct in6_rrenumreq *)data;
	int error = 0;

	/*
	 * Failsafe for errneous address config program.
	 * Let's hope rrenumd don't make a mistakes.
	 */
	if (ipr->ipr_origin <= PR_ORIG_RA)
		ipr->ipr_origin = PR_ORIG_STATIC;

	switch (cmd) {
	case SIOCSGIFPREFIX_IN6:
		delmark_global_prefixes(ifp, irr);
		/* FALL THROUGH */
	case SIOCAIFPREFIX_IN6:
	case SIOCCIFPREFIX_IN6:
		if (mark_matched_prefixes(cmd, ifp, irr)) {
			if (irr->irr_u_uselen)
				add_useprefixes(ifp, irr);
			if (cmd != SIOCAIFPREFIX_IN6)
				delete_prefixes(ifp, irr->irr_origin);
		}
		else
			return (EADDRNOTAVAIL);

		unmark_prefixes(ifp);
		break;
	case SIOCGIFPREFIX_IN6:
		ndpr = search_matched_prefix(ifp, ipr);
		if (ndpr == 0 || ifp != ndpr->ndpr_ifp)
			return (EADDRNOTAVAIL);

		ipr->ipr_origin = ndpr->ndpr_origin;
		ipr->ipr_plen = ndpr->ndpr_plen;
		ipr->ipr_vltime = ndpr->ndpr_vltime;
		ipr->ipr_pltime = ndpr->ndpr_pltime;
		ipr->ipr_flags = ndpr->ndpr_flags;
		ipr->ipr_prefix = ndpr->ndpr_prefix;

		break;
	case SIOCSIFPREFIX_IN6:
		/* init ndpr_tmp */
		bzero((caddr_t)&ndpr_tmp, sizeof(ndpr_tmp));
		ndpr_tmp.ndpr_ifp = ifp;
		ndpr_tmp.ndpr_plen = ipr->ipr_plen;
		ndpr_tmp.ndpr_prefix = ipr->ipr_prefix;
		ndpr_tmp.ndpr_vltime = ipr->ipr_vltime;
		ndpr_tmp.ndpr_pltime = ipr->ipr_pltime;
		ndpr_tmp.ndpr_flags = ipr->ipr_flags;
		ndpr_tmp.ndpr_origin = ipr->ipr_origin;

		if ((error = add_each_prefix(&ndpr_tmp)) != 0)
			return error;
		break;
	case SIOCDIFPREFIX_IN6:
		ndpr = search_matched_prefix(ifp, ipr);
		if (ndpr == 0 || ifp != ndpr->ndpr_ifp)
			return (EADDRNOTAVAIL);

		error = delete_each_prefix(ndpr, ipr->ipr_origin);
		break;
	}
	return error;
}
