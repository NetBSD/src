/*	$NetBSD: uipc_domain.c,v 1.38 2003/02/26 06:31:11 matt Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
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
 *	@(#)uipc_domain.c	8.3 (Berkeley) 2/14/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_domain.c,v 1.38 2003/02/26 06:31:11 matt Exp $");

#include "opt_inet.h"
#include "opt_ipsec.h"
#include "opt_atalk.h"
#include "opt_ccitt.h"
#include "opt_iso.h"
#include "opt_ns.h"
#include "opt_natm.h"
#include "arp.h"

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/protosw.h>
#include <sys/domain.h>
#include <sys/mbuf.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/proc.h>
#include <sys/sysctl.h>

void	pffasttimo __P((void *));
void	pfslowtimo __P((void *));

struct	domain	*domains;

struct callout pffasttimo_ch, pfslowtimo_ch;

/*
 * Current time values for fast and slow timeouts.  We can use u_int
 * relatively safely.  The fast timer will roll over in 27 years and
 * the slow timer in 68 years.
 */
u_int	pfslowtimo_now;
u_int	pffasttimo_now;

#define	ADDDOMAIN(x)	{ \
	extern struct domain __CONCAT(x,domain); \
	__CONCAT(x,domain.dom_next) = domains; \
	domains = &__CONCAT(x,domain); \
}

void
domaininit()
{
	struct domain *dp;
	struct protosw *pr;

#undef unix
	/*
	 * KAME NOTE: ADDDOMAIN(route) is moved to the last part so that
	 * it will be initialized as the *first* element.  confusing!
	 */
#ifndef lint
	ADDDOMAIN(unix);
#ifdef INET
	ADDDOMAIN(inet);
#endif
#ifdef INET6
	ADDDOMAIN(inet6);
#endif
#ifdef NS
	ADDDOMAIN(ns);
#endif
#ifdef ISO
	ADDDOMAIN(iso);
#endif
#ifdef CCITT
	ADDDOMAIN(ccitt);
#endif
#ifdef NATM
	ADDDOMAIN(natm);
#endif
#ifdef NETATALK
	ADDDOMAIN(atalk);
#endif
#ifdef IPSEC
	ADDDOMAIN(key);
#endif
#ifdef INET
#if NARP > 0
	ADDDOMAIN(arp);
#endif
#endif
	ADDDOMAIN(route);
#endif /* ! lint */

	for (dp = domains; dp; dp = dp->dom_next) {
		if (dp->dom_init)
			(*dp->dom_init)();
#ifdef MBUFTRACE
		if (dp->dom_mowner.mo_name[0] == '\0') {
			strncpy(dp->dom_mowner.mo_name, dp->dom_name,
			    sizeof(dp->dom_mowner.mo_name));
			MOWNER_ATTACH(&dp->dom_mowner);
		}
#endif
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_init)
				(*pr->pr_init)();
	}

	if (max_linkhdr < 16)		/* XXX */
		max_linkhdr = 16;
	max_hdr = max_linkhdr + max_protohdr;
	max_datalen = MHLEN - max_hdr;

	callout_init(&pffasttimo_ch);
	callout_init(&pfslowtimo_ch);

	callout_reset(&pffasttimo_ch, 1, pffasttimo, NULL);
	callout_reset(&pfslowtimo_ch, 1, pfslowtimo, NULL);
}

struct domain *
pffinddomain(family)
	int family;
{
	struct domain *dp;

	for (dp = domains; dp != NULL; dp = dp->dom_next)
		if (dp->dom_family == family)
			return (dp);
	return (NULL);
}

struct protosw *
pffindtype(family, type)
	int family, type;
{
	struct domain *dp;
	struct protosw *pr;

	dp = pffinddomain(family);
	if (dp == NULL)
		return (NULL);

	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_type && pr->pr_type == type)
			return (pr);

	return (NULL);
}

struct protosw *
pffindproto(family, protocol, type)
	int family, protocol, type;
{
	struct domain *dp;
	struct protosw *pr;
	struct protosw *maybe = NULL;

	if (family == 0)
		return (NULL);

	dp = pffinddomain(family);
	if (dp == NULL)
		return (NULL);

	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++) {
		if ((pr->pr_protocol == protocol) && (pr->pr_type == type))
			return (pr);

		if (type == SOCK_RAW && pr->pr_type == SOCK_RAW &&
		    pr->pr_protocol == 0 && maybe == NULL)
			maybe = pr;
	}
	return (maybe);
}

int
net_sysctl(name, namelen, oldp, oldlenp, newp, newlen, p)
	int *name;
	u_int namelen;
	void *oldp;
	size_t *oldlenp;
	void *newp;
	size_t newlen;
	struct proc *p;
{
	struct domain *dp;
	struct protosw *pr;
	int family, protocol;

	/*
	 * All sysctl names at this level are nonterminal.
	 * PF_KEY: next component is protocol family, and then at least one
	 *	additional component.
	 * usually: next two components are protocol family and protocol
	 *	number, then at least one addition component.
	 */
	if (namelen < 2)
		return (EISDIR);		/* overloaded */
	family = name[0];

	if (family == 0)
		return (0);

	dp = pffinddomain(family);
	if (dp == NULL)
		return (ENOPROTOOPT);

	switch (family) {
#ifdef IPSEC
	case PF_KEY:
		pr = dp->dom_protosw;
		if (pr->pr_sysctl)
			return ((*pr->pr_sysctl)(name + 1, namelen - 1,
				oldp, oldlenp, newp, newlen));
		return (ENOPROTOOPT);
#endif
	default:
		break;
	}
	if (namelen < 3)
		return (EISDIR);		/* overloaded */
	protocol = name[1];
	for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
		if (pr->pr_protocol == protocol && pr->pr_sysctl)
			return ((*pr->pr_sysctl)(name + 2, namelen - 2,
			    oldp, oldlenp, newp, newlen));
	return (ENOPROTOOPT);
}

void
pfctlinput(cmd, sa)
	int cmd;
	struct sockaddr *sa;
{
	struct domain *dp;
	struct protosw *pr;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_ctlinput)
				(*pr->pr_ctlinput)(cmd, sa, NULL);
}

void
pfctlinput2(cmd, sa, ctlparam)
	int cmd;
	struct sockaddr *sa;
	void *ctlparam;
{
	struct domain *dp;
	struct protosw *pr;

	if (!sa)
		return;
	for (dp = domains; dp; dp = dp->dom_next) {
		/*
		 * the check must be made by xx_ctlinput() anyways, to
		 * make sure we use data item pointed to by ctlparam in
		 * correct way.  the following check is made just for safety.
		 */
		if (dp->dom_family != sa->sa_family)
			continue;

		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_ctlinput)
				(*pr->pr_ctlinput)(cmd, sa, ctlparam);
	}
}

void
pfslowtimo(arg)
	void *arg;
{
	struct domain *dp;
	struct protosw *pr;

	pfslowtimo_now++;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_slowtimo)
				(*pr->pr_slowtimo)();
	callout_reset(&pfslowtimo_ch, hz / 2, pfslowtimo, NULL);
}

void
pffasttimo(arg)
	void *arg;
{
	struct domain *dp;
	struct protosw *pr;

	pffasttimo_now++;

	for (dp = domains; dp; dp = dp->dom_next)
		for (pr = dp->dom_protosw; pr < dp->dom_protoswNPROTOSW; pr++)
			if (pr->pr_fasttimo)
				(*pr->pr_fasttimo)();
	callout_reset(&pffasttimo_ch, hz / 5, pffasttimo, NULL);
}
