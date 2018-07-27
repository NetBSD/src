/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
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

#include <arpa/inet.h>

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ELOOP_QUEUE 6
#include "config.h"
#include "arp.h"
#include "common.h"
#include "eloop.h"
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "logerr.h"
#include "sa.h"
#include "script.h"

#ifdef IPV4LL
static const struct in_addr inaddr_llmask = {
	.s_addr = HTONL(LINKLOCAL_MASK)
};
static const struct in_addr inaddr_llbcast = {
	.s_addr = HTONL(LINKLOCAL_BCAST)
};

static in_addr_t
ipv4ll_pickaddr(struct arp_state *astate)
{
	struct in_addr addr;
	struct ipv4ll_state *istate;

	istate = IPV4LL_STATE(astate->iface);
	setstate(istate->randomstate);

	do {
		long r;

		/* RFC 3927 Section 2.1 states that the first 256 and
		 * last 256 addresses are reserved for future use.
		 * See ipv4ll_start for why we don't use arc4random. */
		/* coverity[dont_call] */
		r = random();
		addr.s_addr = ntohl(LINKLOCAL_ADDR |
		    ((uint32_t)(r % 0xFD00) + 0x0100));

		/* No point using a failed address */
		if (addr.s_addr == astate->failed.s_addr)
			continue;
		/* Ensure we don't have the address on another interface */
	} while (ipv4_findaddr(astate->iface->ctx, &addr) != NULL);

	/* Restore the original random state */
	setstate(istate->arp->iface->ctx->randomstate);
	return addr.s_addr;
}

int
ipv4ll_subnetroute(struct rt_head *routes, struct interface *ifp)
{
	struct ipv4ll_state *state;
	struct rt *rt;
	struct in_addr in;

	assert(ifp != NULL);
	if ((state = IPV4LL_STATE(ifp)) == NULL ||
	    state->addr == NULL)
		return 0;

	if ((rt = rt_new(ifp)) == NULL)
		return -1;

	in.s_addr = state->addr->addr.s_addr & state->addr->mask.s_addr;
	sa_in_init(&rt->rt_dest, &in);
	in.s_addr = state->addr->mask.s_addr;
	sa_in_init(&rt->rt_netmask, &in);
	in.s_addr = INADDR_ANY;
	sa_in_init(&rt->rt_gateway, &in);
	sa_in_init(&rt->rt_ifa, &state->addr->addr);
	TAILQ_INSERT_TAIL(routes, rt, rt_next);
	return 1;
}

int
ipv4ll_defaultroute(struct rt_head *routes, struct interface *ifp)
{
	struct ipv4ll_state *state;
	struct rt *rt;
	struct in_addr in;

	assert(ifp != NULL);
	if ((state = IPV4LL_STATE(ifp)) == NULL ||
	    state->addr == NULL)
		return 0;

	if ((rt = rt_new(ifp)) == NULL)
		return -1;

	in.s_addr = INADDR_ANY;
	sa_in_init(&rt->rt_dest, &in);
	sa_in_init(&rt->rt_netmask, &in);
	sa_in_init(&rt->rt_gateway, &in);
	sa_in_init(&rt->rt_ifa, &state->addr->addr);
	TAILQ_INSERT_TAIL(routes, rt, rt_next);
	return 1;
}

ssize_t
ipv4ll_env(char **env, const char *prefix, const struct interface *ifp)
{
	const struct ipv4ll_state *state;
	const char *pf = prefix == NULL ? "" : "_";
	struct in_addr netnum;

	assert(ifp != NULL);
	if ((state = IPV4LL_CSTATE(ifp)) == NULL || state->addr == NULL)
		return 0;

	if (env == NULL)
		return 5;

	/* Emulate a DHCP environment */
	if (asprintf(&env[0], "%s%sip_address=%s",
	    prefix, pf, inet_ntoa(state->addr->addr)) == -1)
		return -1;
	if (asprintf(&env[1], "%s%ssubnet_mask=%s",
	    prefix, pf, inet_ntoa(state->addr->mask)) == -1)
		return -1;
	if (asprintf(&env[2], "%s%ssubnet_cidr=%d",
	    prefix, pf, inet_ntocidr(state->addr->mask)) == -1)
		return -1;
	if (asprintf(&env[3], "%s%sbroadcast_address=%s",
	    prefix, pf, inet_ntoa(state->addr->brd)) == -1)
		return -1;
	netnum.s_addr = state->addr->addr.s_addr & state->addr->mask.s_addr;
	if (asprintf(&env[4], "%s%snetwork_number=%s",
	    prefix, pf, inet_ntoa(netnum)) == -1)
		return -1;
	return 5;
}

static void
ipv4ll_probed(struct arp_state *astate)
{
	struct interface *ifp;
	struct ipv4ll_state *state;
	struct ipv4_addr *ia;

	assert(astate != NULL);
	assert(astate->iface != NULL);

	ifp = astate->iface;
	state = IPV4LL_STATE(ifp);
	assert(state != NULL);

	ia = ipv4_iffindaddr(ifp, &astate->addr, &inaddr_llmask);
#ifdef IN_IFF_NOTREADY
	if (ia == NULL || ia->addr_flags & IN_IFF_NOTREADY)
#endif
		loginfox("%s: using IPv4LL address %s",
		  ifp->name, inet_ntoa(astate->addr));
	if (ia == NULL) {
		if (ifp->ctx->options & DHCPCD_TEST)
			goto test;
		ia = ipv4_addaddr(ifp, &astate->addr,
		    &inaddr_llmask, &inaddr_llbcast);
	}
	if (ia == NULL)
		return;
#ifdef IN_IFF_NOTREADY
	if (ia->addr_flags & IN_IFF_NOTREADY)
		return;
	logdebugx("%s: DAD completed for %s",
	    ifp->name, inet_ntoa(astate->addr));
#endif
test:
	state->addr = ia;
	if (ifp->ctx->options & DHCPCD_TEST) {
		script_runreason(ifp, "TEST");
		eloop_exit(ifp->ctx->eloop, EXIT_SUCCESS);
		return;
	}
	timespecclear(&state->defend);
	if_initrt(ifp->ctx, AF_INET);
	rt_build(ifp->ctx, AF_INET);
	arp_announce(astate);
	script_runreason(ifp, "IPV4LL");
	dhcpcd_daemonise(ifp->ctx);
}

static void
ipv4ll_announced(struct arp_state *astate)
{
	struct ipv4ll_state *state = IPV4LL_STATE(astate->iface);

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
	struct interface *ifp;
	struct ipv4ll_state *state;
#ifdef IN_IFF_DUPLICATED
	struct ipv4_addr *ia;
#endif

	assert(astate != NULL);
	assert(astate->iface != NULL);
	ifp = astate->iface;
	state = IPV4LL_STATE(ifp);
	assert(state != NULL);

	/*
	 * NULL amsg means kernel detected DAD.
	 * We always fail on matching sip.
	 * We only fail on matching tip and we haven't added that address yet.
	 */
	if (amsg == NULL ||
	    amsg->sip.s_addr == astate->addr.s_addr ||
	    (amsg->sip.s_addr == 0 && amsg->tip.s_addr == astate->addr.s_addr
	     && ipv4_iffindaddr(ifp, &amsg->tip, NULL) == NULL))
		astate->failed = astate->addr;
	else
		return;

	arp_report_conflicted(astate, amsg);

	if (state->addr != NULL &&
	    astate->failed.s_addr == state->addr->addr.s_addr)
	{
#ifdef KERNEL_RFC5227
		logwarnx("%s: IPv4LL defence failed for %s",
		    ifp->name, state->addr->saddr);
#else
		struct timespec now, defend;

		/* RFC 3927 Section 2.5 says a defence should
		 * broadcast an ARP announcement.
		 * Because the kernel will also unicast a reply to the
		 * hardware address which requested the IP address
		 * the other IPv4LL client will receieve two ARP
		 * messages.
		 * If another conflict happens within DEFEND_INTERVAL
		 * then we must drop our address and negotiate a new one. */
		defend.tv_sec = state->defend.tv_sec + DEFEND_INTERVAL;
		defend.tv_nsec = state->defend.tv_nsec;
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (timespeccmp(&defend, &now, >))
			logwarnx("%s: IPv4LL %d second defence failed for %s",
			    ifp->name, DEFEND_INTERVAL, state->addr->saddr);
		else if (arp_request(ifp,
		    state->addr->addr.s_addr, state->addr->addr.s_addr) == -1)
			logerr(__func__);
		else {
			logdebugx("%s: defended IPv4LL address %s",
			    ifp->name, state->addr->saddr);
			state->defend = now;
			return;
		}
#endif
		ipv4_deladdr(state->addr, 1);
		state->down = 1;
		state->addr = NULL;
		script_runreason(ifp, "IPV4LL");
	}

#ifdef IN_IFF_DUPLICATED
	ia = ipv4_iffindaddr(ifp, &astate->addr, NULL);
	if (ia != NULL && ia->addr_flags & IN_IFF_DUPLICATED)
		ipv4_deladdr(ia, 1);
#endif

	arp_cancel(astate);
	if (++state->conflicts == MAX_CONFLICTS)
		logerr("%s: failed to acquire an IPv4LL address",
		    ifp->name);
	astate->addr.s_addr = ipv4ll_pickaddr(astate);
	eloop_timeout_add_sec(ifp->ctx->eloop,
		state->conflicts >= MAX_CONFLICTS ?
		RATE_LIMIT_INTERVAL : PROBE_WAIT,
		ipv4ll_probe, astate);
}

static void
ipv4ll_arpfree(struct arp_state *astate)
{
	struct ipv4ll_state *state;

	state = IPV4LL_STATE(astate->iface);
	if (state->arp == astate)
		state->arp = NULL;
}

void
ipv4ll_start(void *arg)
{
	struct interface *ifp;
	struct ipv4ll_state *state;
	struct arp_state *astate;
	struct ipv4_addr *ia;

	assert(arg != NULL);
	ifp = arg;
	if ((state = IPV4LL_STATE(ifp)) == NULL) {
		ifp->if_data[IF_DATA_IPV4LL] = calloc(1, sizeof(*state));
		if ((state = IPV4LL_STATE(ifp)) == NULL) {
			logerr(__func__);
			return;
		}
	}

	if (state->arp != NULL)
		return;

	/* RFC 3927 Section 2.1 states that the random number generator
	 * SHOULD be seeded with a value derived from persistent information
	 * such as the IEEE 802 MAC address so that it usually picks
	 * the same address without persistent storage. */
	if (state->conflicts == 0) {
		unsigned int seed;
		char *orig;

		if (sizeof(seed) > ifp->hwlen) {
			seed = 0;
			memcpy(&seed, ifp->hwaddr, ifp->hwlen);
		} else
			memcpy(&seed, ifp->hwaddr + ifp->hwlen - sizeof(seed),
			    sizeof(seed));
		/* coverity[dont_call] */
		orig = initstate(seed,
		    state->randomstate, sizeof(state->randomstate));

		/* Save the original state. */
		if (ifp->ctx->randomstate == NULL)
			ifp->ctx->randomstate = orig;

		/* Set back the original state until we need the seeded one. */
		setstate(ifp->ctx->randomstate);
	}

	if ((astate = arp_new(ifp, NULL)) == NULL)
		return;

	state->arp = astate;
	astate->probed_cb = ipv4ll_probed;
	astate->announced_cb = ipv4ll_announced;
	astate->conflicted_cb = ipv4ll_conflicted;
	astate->free_cb = ipv4ll_arpfree;

	/* Find an existing IPv4LL address and ensure we can work with it. */
	ia = ipv4_iffindlladdr(ifp);

#ifdef IN_IFF_TENTATIVE
	if (ia != NULL && ia->addr_flags & IN_IFF_DUPLICATED) {
		ipv4_deladdr(ia, 0);
		ia = NULL;
	}
#endif

	if (ia != NULL) {
		astate->addr = ia->addr;
#ifdef IN_IFF_TENTATIVE
		if (ia->addr_flags & (IN_IFF_TENTATIVE | IN_IFF_DETACHED)) {
			loginfox("%s: waiting for DAD to complete on %s",
			    ifp->name, inet_ntoa(ia->addr));
			return;
		}
		loginfox("%s: using IPv4LL address %s", ifp->name, ia->saddr);
#endif
		ipv4ll_probed(astate);
		return;
	}

	loginfox("%s: probing for an IPv4LL address", ifp->name);
	astate->addr.s_addr = ipv4ll_pickaddr(astate);
#ifdef IN_IFF_TENTATIVE
	ipv4ll_probed(astate);
#else
	arp_probe(astate);
#endif
}

void
ipv4ll_freedrop(struct interface *ifp, int drop)
{
	struct ipv4ll_state *state;
	int dropped;

	assert(ifp != NULL);
	state = IPV4LL_STATE(ifp);
	dropped = 0;

	/* Free ARP state first because ipv4_deladdr might also ... */
	if (state && state->arp) {
		eloop_timeout_delete(ifp->ctx->eloop, NULL, state->arp);
		arp_free(state->arp);
		state->arp = NULL;
	}

	if (drop && (ifp->options->options & DHCPCD_NODROP) != DHCPCD_NODROP) {
		struct ipv4_state *istate;

		if (state && state->addr != NULL) {
			ipv4_deladdr(state->addr, 1);
			state->addr = NULL;
			dropped = 1;
		}

		/* Free any other link local addresses that might exist. */
		if ((istate = IPV4_STATE(ifp)) != NULL) {
			struct ipv4_addr *ia, *ian;

			TAILQ_FOREACH_SAFE(ia, &istate->addrs, next, ian) {
				if (IN_LINKLOCAL(ntohl(ia->addr.s_addr))) {
					ipv4_deladdr(ia, 0);
					dropped = 1;
				}
			}
		}
	}

	if (state) {
		free(state);
		ifp->if_data[IF_DATA_IPV4LL] = NULL;

		if (dropped) {
			rt_build(ifp->ctx, AF_INET);
			script_runreason(ifp, "IPV4LL");
		}
	}
}

/* This may cause issues in BSD systems, where running as a single dhcpcd
 * daemon would solve this issue easily. */
#ifdef HAVE_ROUTE_METRIC
int
ipv4ll_recvrt(__unused int cmd, const struct rt *rt)
{
	struct dhcpcd_ctx *ctx;
	struct interface *ifp;

	/* Ignore route init. */
	if (rt->rt_dflags & RTDF_INIT)
		return 0;

	/* Only interested in default route changes. */
	if (sa_is_unspecified(&rt->rt_dest))
		return 0;

	/* If any interface is running IPv4LL, rebuild our routing table. */
	ctx = rt->rt_ifp->ctx;
	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (IPV4LL_STATE_RUNNING(ifp)) {
			if_initrt(ctx, AF_INET);
			rt_build(ctx, AF_INET);
			break;
		}
	}

	return 0;
}
#endif
#endif
