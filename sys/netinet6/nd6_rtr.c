/*	$NetBSD: nd6_rtr.c,v 1.149 2020/06/12 11:04:45 roy Exp $	*/
/*	$KAME: nd6_rtr.c,v 1.95 2001/02/07 08:09:47 itojun Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: nd6_rtr.c,v 1.149 2020/06/12 11:04:45 roy Exp $");

#ifdef _KERNEL_OPT
#include "opt_net_mpsafe.h"
#endif

#include <sys/mbuf.h>
#include <sys/syslog.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/nd6.h>
#include <netinet/icmp6.h>
#include <netinet6/icmp6_private.h>

/*
 * Cache the source link layer address of Router Advertisement
 * and Solicition messages.
 */
void
nd6_rtr_cache(struct mbuf *m, int off, int icmp6len, int icmp6_type)
{
	struct ifnet *ifp;
	struct ip6_hdr *ip6 = mtod(m, struct ip6_hdr *);
	struct nd_router_solicit *nd_rs;
	struct nd_router_advert *nd_ra;
	struct in6_addr saddr6 = ip6->ip6_src;
	char *lladdr = NULL;
	int lladdrlen = 0;
	union nd_opts ndopts;
	struct psref psref;
	char ip6bufs[INET6_ADDRSTRLEN], ip6bufd[INET6_ADDRSTRLEN];

	ifp = m_get_rcvif_psref(m, &psref);
	if (ifp == NULL)
		goto freeit;

	/* Sanity checks */
	if (ip6->ip6_hlim != 255) {
		nd6log(LOG_ERR, "invalid hlim (%d) from %s to %s on %s\n",
		    ip6->ip6_hlim, IN6_PRINT(ip6bufs, &ip6->ip6_src),
		    IN6_PRINT(ip6bufd, &ip6->ip6_dst), if_name(ifp));
		goto bad;
	}

	switch (icmp6_type) {
	case ND_ROUTER_SOLICIT:
		/*
		 * Don't update the neighbor cache, if src = ::.
		 * This indicates that the src has no IP address assigned yet.
		 */
		if (IN6_IS_ADDR_UNSPECIFIED(&saddr6))
			goto freeit;

		IP6_EXTHDR_GET(nd_rs, struct nd_router_solicit *, m, off,
		    icmp6len);
		if (nd_rs == NULL) {
			ICMP6_STATINC(ICMP6_STAT_TOOSHORT);
			m_put_rcvif_psref(ifp, &psref);
			return;
		}

		icmp6len -= sizeof(*nd_rs);
		nd6_option_init(nd_rs + 1, icmp6len, &ndopts);
		break;
	case ND_ROUTER_ADVERT:
		if (!IN6_IS_ADDR_LINKLOCAL(&saddr6)) {
			nd6log(LOG_ERR, "src %s is not link-local\n",
			    IN6_PRINT(ip6bufs, &saddr6));
			goto bad;
		}

		IP6_EXTHDR_GET(nd_ra, struct nd_router_advert *, m, off,
		    icmp6len);
		if (nd_ra == NULL) {
			ICMP6_STATINC(ICMP6_STAT_TOOSHORT);
			m_put_rcvif_psref(ifp, &psref);
			return;
		}

		icmp6len -= sizeof(*nd_ra);
		nd6_option_init(nd_ra + 1, icmp6len, &ndopts);
		break;
	}

	if (nd6_options(&ndopts) < 0) {
		nd6log(LOG_INFO, "invalid ND option, ignored\n");
		/* nd6_options have incremented stats */
		goto freeit;
	}

	if (ndopts.nd_opts_src_lladdr) {
		lladdr = (char *)(ndopts.nd_opts_src_lladdr + 1);
		lladdrlen = ndopts.nd_opts_src_lladdr->nd_opt_len << 3;
	}

	if (lladdr && ((ifp->if_addrlen + 2 + 7) & ~7) != lladdrlen) {
		nd6log(LOG_INFO, "lladdrlen mismatch for %s "
		    "(if %d, %s packet %d)\n",
		    IN6_PRINT(ip6bufs, &saddr6),
		    ifp->if_addrlen,
		    icmp6_type == ND_ROUTER_SOLICIT ? "RS" : "RA",
		    lladdrlen - 2);
		goto bad;
	}

	nd6_cache_lladdr(ifp, &saddr6, lladdr, lladdrlen, icmp6_type, 0);

freeit:
	m_put_rcvif_psref(ifp, &psref);
	m_freem(m);
	return;

bad:
	ICMP6_STATINC(icmp6_type == ND_ROUTER_SOLICIT ?
	    ICMP6_STAT_BADRS : ICMP6_STAT_BADRA);
	m_put_rcvif_psref(ifp, &psref);
	m_freem(m);
}
