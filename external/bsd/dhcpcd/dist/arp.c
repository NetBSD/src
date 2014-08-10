#include <sys/cdefs.h>
 __RCSID("$NetBSD: arp.c,v 1.1.1.11.2.1 2014/08/10 07:06:59 tls Exp $");

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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

#include <sys/socket.h>
#include <sys/types.h>

#include <net/if.h>
#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "config.h"
#include "arp.h"
#include "ipv4.h"
#include "common.h"
#include "dhcp.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "if.h"
#include "if-options.h"
#include "ipv4ll.h"

#define ARP_LEN								      \
	(sizeof(struct arphdr) + (2 * sizeof(uint32_t)) + (2 * HWADDR_LEN))

static ssize_t
arp_send(const struct interface *ifp, int op, in_addr_t sip, in_addr_t tip)
{
	uint8_t arp_buffer[ARP_LEN];
	struct arphdr ar;
	size_t len;
	uint8_t *p;

	ar.ar_hrd = htons(ifp->family);
	ar.ar_pro = htons(ETHERTYPE_IP);
	ar.ar_hln = ifp->hwlen;
	ar.ar_pln = sizeof(sip);
	ar.ar_op = htons(op);

	p = arp_buffer;
	len = 0;

#define CHECK(fun, b, l)						\
	do {								\
		if (len + (l) > sizeof(arp_buffer))			\
			goto eexit;					\
		fun(p, (b), (l));					\
		p += (l);						\
		len += (l);						\
	} while (/* CONSTCOND */ 0)
#define APPEND(b, l)	CHECK(memcpy, b, l)
#define ZERO(l)		CHECK(memset, 0, l)

	APPEND(&ar, sizeof(ar));
	APPEND(ifp->hwaddr, ifp->hwlen);
	APPEND(&sip, sizeof(sip));
	ZERO(ifp->hwlen);
	APPEND(&tip, sizeof(tip));
	return if_sendrawpacket(ifp, ETHERTYPE_ARP, arp_buffer, len);

eexit:
	errno = ENOBUFS;
	return -1;
}

static void
arp_failure(struct interface *ifp)
{
	const struct dhcp_state *state = D_CSTATE(ifp);

	/* If we failed without a magic cookie then we need to try
	 * and defend our IPv4LL address. */
	if ((state->offer != NULL &&
	    state->offer->cookie != htonl(MAGIC_COOKIE)) ||
	    (state->new != NULL &&
	    state->new->cookie != htonl(MAGIC_COOKIE)))
	{
		ipv4ll_handle_failure(ifp);
		return;
	}

	unlink(state->leasefile);
	if (!state->lease.frominfo)
		dhcp_decline(ifp);
	eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
	if (state->lease.frominfo)
		dhcpcd_startinterface(ifp);
	else
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    DHCP_ARP_FAIL, dhcpcd_startinterface, ifp);
}

static void
arp_packet(void *arg)
{
	struct interface *ifp = arg;
	uint8_t arp_buffer[ARP_LEN];
	struct arphdr ar;
	uint32_t reply_s;
	uint32_t reply_t;
	uint8_t *hw_s, *hw_t;
	ssize_t bytes;
	struct dhcp_state *state;
	struct if_options *opts = ifp->options;
	const char *hwaddr;
	struct in_addr ina;
	char hwbuf[HWADDR_LEN * 3];
	int flags;

	state = D_STATE(ifp);
	state->fail.s_addr = 0;
	flags = 0;
	while (!(flags & RAW_EOF)) {
		bytes = if_readrawpacket(ifp, ETHERTYPE_ARP,
		    arp_buffer, sizeof(arp_buffer), &flags);
		if (bytes == 0 || bytes == -1) {
			syslog(LOG_ERR, "%s: arp if_readrawpacket: %m",
			    ifp->name);
			dhcp_close(ifp);
			return;
		}
		/* We must have a full ARP header */
		if ((size_t)bytes < sizeof(ar))
			continue;
		memcpy(&ar, arp_buffer, sizeof(ar));
		/* Protocol must be IP. */
		if (ar.ar_pro != htons(ETHERTYPE_IP))
			continue;
		if (ar.ar_pln != sizeof(reply_s))
			continue;
		/* Only these types are recognised */
		if (ar.ar_op != htons(ARPOP_REPLY) &&
		    ar.ar_op != htons(ARPOP_REQUEST))
			continue;

		/* Get pointers to the hardware addreses */
		hw_s = arp_buffer + sizeof(ar);
		hw_t = hw_s + ar.ar_hln + ar.ar_pln;
		/* Ensure we got all the data */
		if ((hw_t + ar.ar_hln + ar.ar_pln) - arp_buffer > bytes)
			continue;
		/* Ignore messages from ourself */
		if (ar.ar_hln == ifp->hwlen &&
		    memcmp(hw_s, ifp->hwaddr, ifp->hwlen) == 0)
			continue;
		/* Copy out the IP addresses */
		memcpy(&reply_s, hw_s + ar.ar_hln, ar.ar_pln);
		memcpy(&reply_t, hw_t + ar.ar_hln, ar.ar_pln);

		/* Check for arping */
		if (state->arping_index &&
		    state->arping_index <= opts->arping_len &&
		    (reply_s == opts->arping[state->arping_index - 1] ||
		    (reply_s == 0 &&
		    reply_t == opts->arping[state->arping_index - 1])))
		{
			ina.s_addr = reply_s;
			hwaddr = hwaddr_ntoa((unsigned char *)hw_s,
			    (size_t)ar.ar_hln, hwbuf, sizeof(hwbuf));
			syslog(LOG_INFO,
			    "%s: found %s on hardware address %s",
			    ifp->name, inet_ntoa(ina), hwaddr);
			if (dhcpcd_selectprofile(ifp, hwaddr) == -1 &&
			    dhcpcd_selectprofile(ifp, inet_ntoa(ina)) == -1)
			{
				state->probes = 0;
				/* We didn't find a profile for this
				 * address or hwaddr, so move to the next
				 * arping profile */
				if (state->arping_index <
				    ifp->options->arping_len)
				{
					arp_probe(ifp);
					return;
				}
			}
			dhcp_close(ifp);
			eloop_timeout_delete(ifp->ctx->eloop, NULL, ifp);
			dhcpcd_startinterface(ifp);
			return;
		}

		/* RFC 2131 3.1.5, Client-server interaction
		 * RFC 3927 2.2.1, Probe Conflict Detection */
		if (state->offer &&
		    (reply_s == state->offer->yiaddr ||
		    (reply_s == 0 && reply_t == state->offer->yiaddr)))
			state->fail.s_addr = state->offer->yiaddr;

		/* RFC 3927 2.5, Conflict Defense */
		if (IN_LINKLOCAL(htonl(state->addr.s_addr)) &&
		    reply_s == state->addr.s_addr)
			state->fail.s_addr = state->addr.s_addr;

		if (state->fail.s_addr) {
			syslog(LOG_ERR, "%s: hardware address %s claims %s",
			    ifp->name,
			    hwaddr_ntoa((unsigned char *)hw_s,
				(size_t)ar.ar_hln, hwbuf, sizeof(hwbuf)),
			    inet_ntoa(state->fail));
			errno = EEXIST;
			arp_failure(ifp);
			return;
		}
	}
}

void
arp_announce(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct timeval tv;

	if (state->new == NULL)
		return;
	if (state->arp_fd == -1) {
		state->arp_fd = if_openrawsocket(ifp, ETHERTYPE_ARP);
		if (state->arp_fd == -1) {
			syslog(LOG_ERR, "%s: %s: %m", __func__, ifp->name);
			return;
		}
		eloop_event_add(ifp->ctx->eloop,
		    state->arp_fd, arp_packet, ifp);
	}
	if (++state->claims < ANNOUNCE_NUM)
		syslog(LOG_DEBUG,
		    "%s: sending ARP announce (%d of %d), "
		    "next in %d.0 seconds",
		    ifp->name, state->claims, ANNOUNCE_NUM, ANNOUNCE_WAIT);
	else
		syslog(LOG_DEBUG,
		    "%s: sending ARP announce (%d of %d)",
		    ifp->name, state->claims, ANNOUNCE_NUM);
	if (arp_send(ifp, ARPOP_REQUEST,
		state->new->yiaddr, state->new->yiaddr) == -1)
		syslog(LOG_ERR, "send_arp: %m");
	if (state->claims < ANNOUNCE_NUM) {
		eloop_timeout_add_sec(ifp->ctx->eloop,
		    ANNOUNCE_WAIT, arp_announce, ifp);
		return;
	}
	if (state->new->cookie != htonl(MAGIC_COOKIE)) {
		/* Check if doing DHCP */
		if (!(ifp->options->options & DHCPCD_DHCP))
			return;
		/* We should pretend to be at the end
		 * of the DHCP negotation cycle unless we rebooted */
		if (state->interval != 0)
			state->interval = 64;
		state->probes = 0;
		state->claims = 0;
		tv.tv_sec = state->interval - DHCP_RAND_MIN;
		tv.tv_usec = (suseconds_t)arc4random_uniform(
		    (DHCP_RAND_MAX - DHCP_RAND_MIN) * 1000000);
		timernorm(&tv);
		eloop_timeout_add_tv(ifp->ctx->eloop, &tv, dhcp_discover, ifp);
	} else {
		eloop_event_delete(ifp->ctx->eloop, state->arp_fd);
		close(state->arp_fd);
		state->arp_fd = -1;
	}
}

void
arp_probe(void *arg)
{
	struct interface *ifp = arg;
	struct dhcp_state *state = D_STATE(ifp);
	struct in_addr addr;
	struct timeval tv;
	int arping = 0;

	if (state->arp_fd == -1) {
		state->arp_fd = if_openrawsocket(ifp, ETHERTYPE_ARP);
		if (state->arp_fd == -1) {
			syslog(LOG_ERR, "%s: %s: %m", __func__, ifp->name);
			return;
		}
		eloop_event_add(ifp->ctx->eloop,
		    state->arp_fd, arp_packet, ifp);
	}

	if (state->arping_index < ifp->options->arping_len) {
		addr.s_addr = ifp->options->arping[state->arping_index];
		arping = 1;
	} else if (state->offer) {
		if (state->offer->yiaddr)
			addr.s_addr = state->offer->yiaddr;
		else
			addr.s_addr = state->offer->ciaddr;
	} else
		addr.s_addr = state->addr.s_addr;

	if (state->probes == 0) {
		if (arping)
			syslog(LOG_DEBUG, "%s: searching for %s",
			    ifp->name, inet_ntoa(addr));
		else
			syslog(LOG_DEBUG, "%s: checking for %s",
			    ifp->name, inet_ntoa(addr));
	}
	if (++state->probes < PROBE_NUM) {
		tv.tv_sec = PROBE_MIN;
		tv.tv_usec = (suseconds_t)arc4random_uniform(
		    (PROBE_MAX - PROBE_MIN) * 1000000);
		timernorm(&tv);
		eloop_timeout_add_tv(ifp->ctx->eloop, &tv, arp_probe, ifp);
	} else {
		tv.tv_sec = ANNOUNCE_WAIT;
		tv.tv_usec = 0;
		if (arping) {
			state->probes = 0;
			if (++state->arping_index < ifp->options->arping_len)
				eloop_timeout_add_tv(ifp->ctx->eloop,
				    &tv, arp_probe, ifp);
			else
				eloop_timeout_add_tv(ifp->ctx->eloop,
				    &tv, dhcpcd_startinterface, ifp);
		} else
			eloop_timeout_add_tv(ifp->ctx->eloop,
			    &tv, dhcp_bind, ifp);
	}
	syslog(LOG_DEBUG,
	    "%s: sending ARP probe (%d of %d), next in %0.1f seconds",
	    ifp->name, state->probes ? state->probes : PROBE_NUM, PROBE_NUM,
	    timeval_to_double(&tv));
	if (arp_send(ifp, ARPOP_REQUEST, 0, addr.s_addr) == -1)
		syslog(LOG_ERR, "send_arp: %m");
}

void
arp_start(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	state->probes = 0;
	state->arping_index = 0;
	arp_probe(ifp);
}

void
arp_close(struct interface *ifp)
{
	struct dhcp_state *state = D_STATE(ifp);

	if (state == NULL)
		return;

	if (state->arp_fd != -1) {
		eloop_event_delete(ifp->ctx->eloop, state->arp_fd);
		close(state->arp_fd);
		state->arp_fd = -1;
	}
}

