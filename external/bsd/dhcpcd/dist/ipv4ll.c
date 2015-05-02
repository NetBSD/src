#include <sys/cdefs.h>
 __RCSID("$NetBSD: ipv4ll.c,v 1.10 2015/05/02 15:18:37 roy Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2015 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ELOOP_QUEUE 6
#include "config.h"
#include "arp.h"
#include "common.h"
#include "dhcp.h"
#include "eloop.h"
#include "if.h"
#include "if-options.h"
#include "ipv4ll.h"

static struct dhcp_message *
ipv4ll_make_lease(uint32_t addr)
{
	uint32_t u32;
	struct dhcp_message *dhcp;
	uint8_t *p;

	dhcp = calloc(1, sizeof(*dhcp));
	if (dhcp == NULL)
		return NULL;
	/* Put some LL options in */
	dhcp->yiaddr = addr;
	p = dhcp->options;
	*p++ = DHO_SUBNETMASK;
	*p++ = sizeof(u32);
	u32 = htonl(LINKLOCAL_MASK);
	memcpy(p, &u32, sizeof(u32));
	p += sizeof(u32);
	*p++ = DHO_BROADCAST;
	*p++ = sizeof(u32);
	u32 = htonl(LINKLOCAL_BRDC);
	memcpy(p, &u32, sizeof(u32));
	p += sizeof(u32);
	*p++ = DHO_END;

	return dhcp;
}

static in_addr_t
ipv4ll_pick_addr(const struct arp_state *astate)
{
	in_addr_t addr;
	struct interface *ifp;
	const struct dhcp_state *state;

	for (;;) {
		/* RFC 3927 Section 2.1 states that the first 256 and
		 * last 256 addresses are reserved for future use.
		 * See ipv4ll_start for why we don't use arc4_random. */
		addr = ntohl(LINKLOCAL_ADDR |
		    ((uint32_t)(random() % 0xFD00) + 0x0100));

		/* No point using a failed address */
		if (addr == astate->failed.s_addr)
			continue;

		/* Ensure we don't have the address on another interface */
		TAILQ_FOREACH(ifp, astate->iface->ctx->ifaces, next) {
			state = D_CSTATE(ifp);
			if (state && state->addr.s_addr == addr)
				break;
		}

		/* Yay, this should be a unique and workable IPv4LL address */
		if (ifp == NULL)
			break;
	}
	return addr;
}

static void
ipv4ll_probed(struct arp_state *astate)
{
	struct dhcp_state *state = D_STATE(astate->iface);

	if (state->state == DHS_IPV4LL_BOUND) {
		ipv4_finaliseaddr(astate->iface);
		return;
	}

	if (state->state != DHS_BOUND) {
		struct dhcp_message *offer;

		/* A DHCP lease could have already been offered.
		 * Backup and replace once the IPv4LL address is bound */
		offer = state->offer;
		state->offer = ipv4ll_make_lease(astate->addr.s_addr);
		if (state->offer == NULL)
			logger(astate->iface->ctx, LOG_ERR, "%s: %m", __func__);
		else
			dhcp_bind(astate->iface, astate);
		state->offer = offer;
	}
}

static void
ipv4ll_announced(struct arp_state *astate)
{
	struct dhcp_state *state = D_STATE(astate->iface);

	state->conflicts = 0;
	/* Need to keep the arp state so we can defend our IP. */
}

static void
ipv4ll_probe(void *arg)
{

#ifdef IN_IFF_TENTATIVE
	ipv4ll_probed(arg);
#else
	arp_probe(arg);
#endif
}

static void
ipv4ll_conflicted(struct arp_state *astate, const struct arp_msg *amsg)
{
	struct dhcp_state *state = D_STATE(astate->iface);
	in_addr_t fail;

	fail = 0;
	/* RFC 3927 2.2.1, Probe Conflict Detection */
	if (amsg == NULL ||
	    (amsg->sip.s_addr == astate->addr.s_addr ||
	    (amsg->sip.s_addr == 0 && amsg->tip.s_addr == astate->addr.s_addr)))
		fail = astate->addr.s_addr;

	/* RFC 3927 2.5, Conflict Defense */
	if (IN_LINKLOCAL(htonl(state->addr.s_addr)) &&
	    amsg && amsg->sip.s_addr == state->addr.s_addr)
		fail = state->addr.s_addr;

	if (fail == 0)
		return;

	astate->failed.s_addr = fail;
	arp_report_conflicted(astate, amsg);

	if (astate->failed.s_addr == state->addr.s_addr) {
		time_t up;

		/* RFC 3927 Section 2.5 */
		up = uptime();
		if (state->defend + DEFEND_INTERVAL > up) {
			logger(astate->iface->ctx, LOG_WARNING,
			    "%s: IPv4LL %d second defence failed for %s",
			    astate->iface->name, DEFEND_INTERVAL,
			    inet_ntoa(state->addr));
			dhcp_drop(astate->iface, "EXPIRE");
		} else {
			logger(astate->iface->ctx, LOG_DEBUG,
			    "%s: defended IPv4LL address %s",
			    astate->iface->name, inet_ntoa(state->addr));
			state->defend = up;
			return;
		}
	}

	arp_cancel(astate);
	if (++state->conflicts == MAX_CONFLICTS)
		logger(astate->iface->ctx, LOG_ERR,
		    "%s: failed to acquire an IPv4LL address",
		    astate->iface->name);
	astate->addr.s_addr = ipv4ll_pick_addr(astate);
	eloop_timeout_add_sec(astate->iface->ctx->eloop,
		state->conflicts >= MAX_CONFLICTS ?
		RATE_LIMIT_INTERVAL : PROBE_WAIT,
		ipv4ll_probe, astate);
}

void
ipv4ll_start(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct arp_state *astate;
	struct ipv4_addr *ap;

	if (state->arp_ipv4ll)
		return;

	/* RFC 3927 Section 2.1 states that the random number generator
	 * SHOULD be seeded with a value derived from persistent information
	 * such as the IEEE 802 MAC address so that it usually picks
	 * the same address without persistent storage. */
	if (state->conflicts == 0) {
		unsigned int seed;

		if (sizeof(seed) > ifp->hwlen) {
			seed = 0;
			memcpy(&seed, ifp->hwaddr, ifp->hwlen);
		} else
			memcpy(&seed, ifp->hwaddr + ifp->hwlen - sizeof(seed),
			    sizeof(seed));
		initstate(seed, state->randomstate, sizeof(state->randomstate));
	}

	if ((astate = arp_new(ifp, NULL)) == NULL)
		return;

	state->arp_ipv4ll = astate;
	astate->probed_cb = ipv4ll_probed;
	astate->announced_cb = ipv4ll_announced;
	astate->conflicted_cb = ipv4ll_conflicted;

	if (IN_LINKLOCAL(htonl(state->addr.s_addr))) {
		astate->addr = state->addr;
		arp_announce(astate);
		return;
	}

	if (state->offer && IN_LINKLOCAL(ntohl(state->offer->yiaddr))) {
		astate->addr.s_addr = state->offer->yiaddr;
		free(state->offer);
		state->offer = NULL;
		ap = ipv4_iffindaddr(ifp, &astate->addr, NULL);
	} else
		ap = ipv4_iffindlladdr(ifp);
	if (ap) {
		astate->addr = ap->addr;
		ipv4ll_probed(astate);
		return;
	}

	setstate(state->randomstate);
	/* We maybe rebooting an IPv4LL address. */
	if (!IN_LINKLOCAL(htonl(astate->addr.s_addr))) {
		logger(ifp->ctx, LOG_INFO, "%s: probing for an IPv4LL address",
		    ifp->name);
		astate->addr.s_addr = INADDR_ANY;
	}
	if (astate->addr.s_addr == INADDR_ANY)
		astate->addr.s_addr = ipv4ll_pick_addr(astate);
#ifdef IN_IFF_TENTATIVE
	ipv4ll_probed(astate);
#else
	arp_probe(astate);
#endif
}

void
ipv4ll_stop(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	eloop_timeout_delete(ifp->ctx->eloop, NULL, state->arp_ipv4ll);
}
