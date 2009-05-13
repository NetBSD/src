/* 
 * dhcpcd - DHCP client daemon
 * Copyright 2006-2008 Roy Marples <roy@marples.name>
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
#include "bind.h"
#include "common.h"
#include "dhcpcd.h"
#include "eloop.h"
#include "if-options.h"
#include "ipv4ll.h"
#include "net.h"

#define ARP_LEN								      \
	(sizeof(struct arphdr) + (2 * sizeof(uint32_t)) + (2 * HWADDR_LEN))

static int
send_arp(const struct interface *iface, int op, in_addr_t sip, in_addr_t tip)
{
	uint8_t arp_buffer[ARP_LEN];
	struct arphdr ar;
	size_t len;
	uint8_t *p;
	int retval;

	ar.ar_hrd = htons(iface->family);
	ar.ar_pro = htons(ETHERTYPE_IP);
	ar.ar_hln = iface->hwlen;
	ar.ar_pln = sizeof(sip);
	ar.ar_op = htons(op);
	memcpy(arp_buffer, &ar, sizeof(ar));
	p = arp_buffer + sizeof(ar);
	memcpy(p, iface->hwaddr, iface->hwlen);
	p += iface->hwlen;
	memcpy(p, &sip, sizeof(sip));
	p += sizeof(sip);
	/* ARP requests should ignore this */
	retval = iface->hwlen;
	while (retval--)
		*p++ = '\0';
	memcpy(p, &tip, sizeof(tip));
	p += sizeof(tip);
	len = p - arp_buffer;
	retval = send_raw_packet(iface, ETHERTYPE_ARP, arp_buffer, len);
	return retval;
}

static void
handle_arp_failure(struct interface *iface)
{
	if (IN_LINKLOCAL(htonl(iface->state->fail.s_addr))) {
		handle_ipv4ll_failure(iface);
		return;
	}
	if (iface->state->lease.frominfo)
		unlink(iface->leasefile);
	else
		send_decline(iface);
	close_sockets(iface);
	delete_timeout(NULL, iface);
	if (iface->state->lease.frominfo)
		start_interface(iface);
	else
		add_timeout_sec(DHCP_ARP_FAIL, start_interface, iface);
}

static void
handle_arp_packet(void *arg)
{
	struct interface *iface = arg;
	uint8_t arp_buffer[ARP_LEN];
	struct arphdr ar;
	uint32_t reply_s;
	uint32_t reply_t;
	uint8_t *hw_s, *hw_t;
	ssize_t bytes;
	struct if_state *state = iface->state;
	struct if_options *opts = state->options;
	const char *hwaddr;
	struct in_addr ina;

	state->fail.s_addr = 0;
	for(;;) {
		bytes = get_raw_packet(iface, ETHERTYPE_ARP,
		    arp_buffer, sizeof(arp_buffer));
		if (bytes == 0 || bytes == -1)
			return;
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
		if (ar.ar_hln == iface->hwlen &&
		    memcmp(hw_s, iface->hwaddr, iface->hwlen) == 0)
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
			    (size_t)ar.ar_hln);
			syslog(LOG_INFO,
			    "%s: found %s on hardware address %s",
			    iface->name, inet_ntoa(ina), hwaddr);
			if (select_profile(iface, hwaddr) == -1 &&
			    errno == ENOENT)
				select_profile(iface, inet_ntoa(ina));
			close_sockets(iface);
			delete_timeout(NULL, iface);
			start_interface(iface);
			return;
		}

		/* Check for conflict */
		if (state->offer &&
		    (reply_s == state->offer->yiaddr ||
			(reply_s == 0 && reply_t == state->offer->yiaddr)))
			state->fail.s_addr = state->offer->yiaddr;

		/* Handle IPv4LL conflicts */
		if (IN_LINKLOCAL(htonl(iface->addr.s_addr)) &&
		    (reply_s == iface->addr.s_addr ||
			(reply_s == 0 && reply_t == iface->addr.s_addr)))
			state->fail.s_addr = iface->addr.s_addr;

		if (state->fail.s_addr) {
			syslog(LOG_ERR, "%s: hardware address %s claims %s",
			    iface->name,
			    hwaddr_ntoa((unsigned char *)hw_s,
				(size_t)ar.ar_hln),
			    inet_ntoa(state->fail));
			errno = EEXIST;
			handle_arp_failure(iface);
			return;
		}
	}
}

void
send_arp_announce(void *arg)
{
	struct interface *iface = arg;
	struct if_state *state = iface->state;
	struct timeval tv;

	if (iface->arp_fd == -1) {
		open_socket(iface, ETHERTYPE_ARP);
		add_event(iface->arp_fd, handle_arp_packet, iface);
	}
	if (++state->claims < ANNOUNCE_NUM)	
		syslog(LOG_DEBUG,
		    "%s: sending ARP announce (%d of %d), "
		    "next in %d.00 seconds",
		    iface->name, state->claims, ANNOUNCE_NUM, ANNOUNCE_WAIT);
	else
		syslog(LOG_DEBUG,
		    "%s: sending ARP announce (%d of %d)",
		    iface->name, state->claims, ANNOUNCE_NUM);
	if (send_arp(iface, ARPOP_REQUEST,
		state->new->yiaddr, state->new->yiaddr) == -1)
		syslog(LOG_ERR, "send_arp: %m");
	if (state->claims < ANNOUNCE_NUM) {
		add_timeout_sec(ANNOUNCE_WAIT, send_arp_announce, iface);
		return;
	}
	if (IN_LINKLOCAL(htonl(state->new->yiaddr))) {
		/* We should pretend to be at the end
		 * of the DHCP negotation cycle unless we rebooted */
		if (state->interval != 0)
			state->interval = 64;
		state->probes = 0;
		state->claims = 0;
		tv.tv_sec = state->interval - DHCP_RAND_MIN;
		tv.tv_usec = arc4random() % (DHCP_RAND_MAX_U - DHCP_RAND_MIN_U);
		timernorm(&tv);
		add_timeout_tv(&tv, start_discover, iface);
	} else {
		delete_event(iface->arp_fd);
		close(iface->arp_fd);
		iface->arp_fd = -1;
	}
}

void
send_arp_probe(void *arg)
{
	struct interface *iface = arg;
	struct if_state *state = iface->state;
	struct in_addr addr;
	struct timeval tv;
	int arping = 0;

	if (state->arping_index < state->options->arping_len) {
		addr.s_addr = state->options->arping[state->arping_index];
		arping = 1;
	} else if (state->offer) {
		if (state->offer->yiaddr)
			addr.s_addr = state->offer->yiaddr;
		else
			addr.s_addr = state->offer->ciaddr;
	} else
		addr.s_addr = iface->addr.s_addr;

	if (iface->arp_fd == -1) {
		open_socket(iface, ETHERTYPE_ARP);
		add_event(iface->arp_fd, handle_arp_packet, iface);
	}
	if (state->probes == 0) {
		if (arping)
			syslog(LOG_INFO, "%s: searching for %s",
			    iface->name, inet_ntoa(addr));
		else
			syslog(LOG_INFO, "%s: checking for %s",
			    iface->name, inet_ntoa(addr));
	}
	if (++state->probes < PROBE_NUM) {
		tv.tv_sec = PROBE_MIN;
		tv.tv_usec = arc4random() % (PROBE_MAX_U - PROBE_MIN_U);
		timernorm(&tv);
		add_timeout_tv(&tv, send_arp_probe, iface);
	} else {
		tv.tv_sec = ANNOUNCE_WAIT;
		tv.tv_usec = 0;
		if (arping) {
			state->probes = 0;
			if (++state->arping_index < state->options->arping_len)
				add_timeout_tv(&tv, send_arp_probe, iface);
			else
				add_timeout_tv(&tv, start_interface, iface);
		} else
			add_timeout_tv(&tv, bind_interface, iface);
	}
	syslog(LOG_DEBUG,
	    "%s: sending ARP probe (%d of %d), next in %0.2f seconds",
	    iface->name, state->probes ? state->probes : PROBE_NUM, PROBE_NUM,
	    timeval_to_double(&tv));
	if (send_arp(iface, ARPOP_REQUEST, 0, addr.s_addr) == -1)
		syslog(LOG_ERR, "send_arp: %m");
}

void
start_arping(struct interface *iface)
{
	iface->state->probes = 0;
	iface->state->arping_index = 0;
	send_arp_probe(iface);
}
