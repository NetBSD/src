/*	$NetBSD: netisr.c,v 1.5.30.1 2013/07/23 21:07:37 riastradh Exp $	*/

/*
 * Copyright (c) 2008 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netisr.c,v 1.5.30.1 2013/07/23 21:07:37 riastradh Exp $");

#include <sys/param.h>
#include <sys/intr.h>

#include <netinet/in.h>
#include <netinet/ip_var.h>
#include <netinet/if_inarp.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netmpls/mpls_var.h>
#include <net/netisr.h>

#include <rump/rumpuser.h>

#include "rump_net_private.h"

static void *netisrs[NETISR_MAX];
void
schednetisr(int isr)
{

	softint_schedule(netisrs[isr]);
}

/*
 * Aliases are needed only for static linking (dlsym() is not supported).
 */
void __netisr_stub(void);
void
__netisr_stub(void)
{

	panic("netisr called but networking stack missing");
}
__weak_alias(ipintr,__netisr_stub);
__weak_alias(arpintr,__netisr_stub);
__weak_alias(ip6intr,__netisr_stub);
__weak_alias(mplsintr,__netisr_stub);

void
rump_netisr_init(void)
{
	void *iphand, *arphand, *ip6hand, *mplshand, *sym;

	iphand = ipintr;
	if ((sym = rumpuser_dl_globalsym("rumpns_ipintr")) != NULL)
		iphand = sym;

	arphand = arpintr;
	if ((sym = rumpuser_dl_globalsym("rumpns_arpintr")) != NULL)
		arphand = sym;

	ip6hand = ip6intr;
	if ((sym = rumpuser_dl_globalsym("rumpns_ip6intr")) != NULL)
		ip6hand = sym;

	mplshand = mplsintr;
	if ((sym = rumpuser_dl_globalsym("rumpns_mplsintr")) != NULL)
		mplshand = sym;
		
	netisrs[NETISR_IP] = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    (void (*)(void *))iphand, NULL);
	netisrs[NETISR_ARP] = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    (void (*)(void *))arphand, NULL);
	netisrs[NETISR_IPV6] = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    (void (*)(void *))ip6hand, NULL);
	netisrs[NETISR_MPLS] = softint_establish(SOFTINT_NET | SOFTINT_MPSAFE,
	    (void (*)(void *))mplshand, NULL);
}
