/* 
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2008 Roy Marples <roy@marples.name>
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
#include <syslog.h>
#include <unistd.h>

#include "arp.h"
#include "common.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "if-options.h"
#include "ipv4ll.h"
#include "net.h"

static struct dhcp_message*
make_ipv4ll_lease(uint32_t old_addr)
{
	uint32_t u32;
	struct dhcp_message *dhcp;
	uint8_t *p;

	dhcp = xzalloc(sizeof(*dhcp));
	/* Put some LL options in */
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

	for (;;) {
		dhcp->yiaddr = htonl(LINKLOCAL_ADDR |
		    (((uint32_t)abs((int)arc4random())
			% 0xFD00) + 0x0100));
		if (dhcp->yiaddr != old_addr &&
		    IN_LINKLOCAL(ntohl(dhcp->yiaddr)))
			break;
	}
	return dhcp;
}

void
start_ipv4ll(void *arg)
{
	struct interface *iface = arg;

	iface->state->probes = 0;
	iface->state->claims = 0;
	if (iface->addr.s_addr) {
		iface->state->conflicts = 0;
		if (IN_LINKLOCAL(htonl(iface->addr.s_addr))) {
			send_arp_announce(iface);
			return;
		}
	}

	/* We maybe rebooting an IPv4LL address. */
	if (!iface->state->offer ||
	    !IN_LINKLOCAL(htonl(iface->state->offer->yiaddr)))
	{
		syslog(LOG_INFO, "%s: probing for an IPv4LL address",
		    iface->name);
		delete_timeout(NULL, iface);
		free(iface->state->offer);
		iface->state->offer = make_ipv4ll_lease(0);
		iface->state->lease.frominfo = 0;
	}
	send_arp_probe(iface);
}

void
handle_ipv4ll_failure(void *arg)
{
	struct interface *iface = arg;
	time_t up;

	if (iface->state->fail.s_addr == iface->state->lease.addr.s_addr) {
		up = uptime();
		if (iface->state->defend + DEFEND_INTERVAL > up) {
			drop_config(iface, "EXPIRE");
			iface->state->conflicts = -1;
		} else {
			iface->state->defend = up;
			return;
		}
	}

	close_sockets(iface);
	free(iface->state->offer);
	iface->state->offer = NULL;
	if (++iface->state->conflicts > MAX_CONFLICTS) {
		syslog(LOG_ERR, "%s: failed to acquire an IPv4LL address",
		    iface->name);
		iface->state->interval = RATE_LIMIT_INTERVAL / 2;
		start_discover(iface);
	} else {
		add_timeout_sec(PROBE_WAIT, start_ipv4ll, iface);
	}
}
