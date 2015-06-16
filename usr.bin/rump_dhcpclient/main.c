/*	$NetBSD: main.c,v 1.4 2015/06/16 22:54:10 christos Exp $	*/

/*-
 * Copyright (c) 2011 Antti Kantee.  All Rights Reserved.
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <net/if.h>
#include <net/if_dl.h>

#include <err.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>

#include <rump/rump_syscalls.h>
#include <rump/rumpclient.h>

#include "configure.h"
#include "dhcp.h"
#include "net.h"

struct interface *ifaces;

__dead static void
usage(void)
{

	fprintf(stderr, "Usage: %s ifname\n", getprogname());
	exit(1);
}

int
get_hwaddr(struct interface *ifp)
{
	struct if_laddrreq iflr;
	struct sockaddr_dl *sdl;
	int s, sverrno;

	memset(&iflr, 0, sizeof(iflr));
	strlcpy(iflr.iflr_name, ifp->name, sizeof(iflr.iflr_name));
	iflr.addr.ss_family = AF_LINK;

	sdl = satosdl(&iflr.addr);
	sdl->sdl_alen = ETHER_ADDR_LEN;

	if ((s = rump_sys_socket(AF_LINK, SOCK_DGRAM, 0)) == -1)
		return -1;

	if (rump_sys_ioctl(s, SIOCGLIFADDR, &iflr) == -1) {
		sverrno = errno;
		rump_sys_close(s);
		errno = sverrno;
		return -1;
	}

	/* XXX: is that the right way to copy the link address? */
	memcpy(ifp->hwaddr, sdl->sdl_data+strlen(ifp->name), ETHER_ADDR_LEN);
	ifp->hwlen = ETHER_ADDR_LEN;
	ifp->family = ARPHRD_ETHER;

	rump_sys_close(s);
	return 0;
}

static void
send_discover(struct interface *ifp)
{
	struct dhcp_message *dhcp;
	uint8_t *udp;
	ssize_t mlen, ulen;
	struct in_addr ia;

	memset(&ia, 0, sizeof(ia));

	mlen = make_message(&dhcp, ifp, DHCP_DISCOVER);
	ulen = make_udp_packet(&udp, (void *)dhcp, mlen, ia, ia);
	if (send_raw_packet(ifp, ETHERTYPE_IP, udp, ulen) == -1)
		err(EXIT_FAILURE, "sending discover failed");
}

static void
send_request(struct interface *ifp)
{
	struct dhcp_message *dhcp;
	uint8_t *udp;
	ssize_t mlen, ulen;
	struct in_addr ia;

	memset(&ia, 0, sizeof(ia));

	mlen = make_message(&dhcp, ifp, DHCP_REQUEST);
	ulen = make_udp_packet(&udp, (void *)dhcp, mlen, ia, ia);
	if (send_raw_packet(ifp, ETHERTYPE_IP, udp, ulen) == -1)
		err(EXIT_FAILURE, "sending discover failed");
}

/* wait for 5s by default */
#define RESPWAIT 5000
static void
get_network(struct interface *ifp, uint8_t **rawp,
	const struct dhcp_message **dhcpp)
{
	struct pollfd pfd;
	const struct dhcp_message *dhcp;
	const uint8_t *data;
	uint8_t *raw;
	ssize_t n;

	pfd.fd = ifp->raw_fd;
	pfd.events = POLLIN;

	raw = xmalloc(udp_dhcp_len);
	for (;;) {
		switch (rump_sys_poll(&pfd, 1, RESPWAIT)) {
		case 0:
			errx(EXIT_FAILURE, "timed out waiting for response");
		case -1:
			err(EXIT_FAILURE, "poll failed");
		default:
			break;
		}

		if ((n = get_raw_packet(ifp, ETHERTYPE_IP,
		    raw, udp_dhcp_len)) < 1)
			continue;

		if (valid_udp_packet(raw, n, NULL) == -1) {
			fprintf(stderr, "invalid packet received.  retrying\n");
			continue;
		}

		n = get_udp_data(&data, raw);
		if ((size_t)n > sizeof(*dhcp)) {
			fprintf(stderr, "invalid packet size.  retrying\n");
			continue;
		}
		dhcp = (const void *)data;

		/* XXX: what if packet is too small? */

		/* some sanity checks */
		if (dhcp->cookie != htonl(MAGIC_COOKIE)) {
			/* ignore */
			continue;
		}

		if (ifp->state->xid != dhcp->xid) {
			fprintf(stderr, "invalid transaction.  retrying\n");
			continue;
		}

		break;
	}

	*rawp = raw;
	*dhcpp = dhcp;
}

static void
get_offer(struct interface *ifp)
{
	const struct dhcp_message *dhcp;
	uint8_t *raw;
	uint8_t type;

	get_network(ifp, &raw, &dhcp);

	get_option_uint8(&type, dhcp, DHO_MESSAGETYPE);
	switch (type) {
	case DHCP_OFFER:
		break;
	case DHCP_NAK:
		errx(EXIT_FAILURE, "got NAK from dhcp server");
	default:
		errx(EXIT_FAILURE, "didn't receive offer");
	}

	ifp->state->offer = xzalloc(sizeof(*ifp->state->offer));
	memcpy(ifp->state->offer, dhcp, sizeof(*ifp->state->offer));
	ifp->state->lease.addr.s_addr = dhcp->yiaddr;
	ifp->state->lease.cookie = dhcp->cookie;
	free(raw);
}

static void
get_ack(struct interface *ifp)
{
	const struct dhcp_message *dhcp;
	uint8_t *raw;
	uint8_t type;

	get_network(ifp, &raw, &dhcp);
	get_option_uint8(&type, dhcp, DHO_MESSAGETYPE);
	if (type != DHCP_ACK)
		errx(EXIT_FAILURE, "didn't receive ack");

	ifp->state->new = ifp->state->offer;
	get_lease(&ifp->state->lease, ifp->state->new);
}

int
main(int argc, char *argv[])
{
	struct interface *ifp;
	struct if_options *ifo;
	const int mib[] = { CTL_KERN, KERN_HOSTNAME };
	size_t hlen;

	setprogname(argv[0]);
	if (argc != 2)
		usage();

	if (rumpclient_init() == -1)
		err(EXIT_FAILURE, "init failed");

	if (init_sockets() == -1)
		err(EXIT_FAILURE, "failed to init sockets");
	if ((ifp = init_interface(argv[1])) == NULL)
		err(EXIT_FAILURE, "cannot init %s", argv[1]);
	ifaces = ifp;
	if (open_socket(ifp, ETHERTYPE_IP) == -1)
		err(EXIT_FAILURE, "bpf");
	up_interface(ifp);

	ifp->state = xzalloc(sizeof(*ifp->state));
	ifp->state->options = ifo = xzalloc(sizeof(*ifp->state->options));
	ifp->state->xid = arc4random();

	hlen = sizeof(ifo->hostname);
	if (rump_sys___sysctl(mib, __arraycount(mib), ifo->hostname, &hlen,
	    NULL, 0) == -1)
		snprintf(ifo->hostname, sizeof(ifo->hostname),
		    "unknown.hostname");
	ifo->options = DHCPCD_GATEWAY | DHCPCD_HOSTNAME;

	if (get_hwaddr(ifp) == -1)
		err(EXIT_FAILURE, "failed to get hwaddr for %s", ifp->name);

	send_discover(ifp);
	get_offer(ifp);
	send_request(ifp);
	get_ack(ifp);

	configure(ifp);

	return 0;
}
