/*	$NetBSD: tap.c,v 1.1.8.1 2009/05/13 19:20:19 jym Exp $	*/

/*-
 * Copyright (c) 2008 Iain Hibbert
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: tap.c,v 1.1.8.1 2009/05/13 19:20:19 jym Exp $");

#include <sys/ioctl.h>
#include <sys/uio.h>

#include <net/if_dl.h>
#include <net/if_tap.h>

#include <fcntl.h>
#include <unistd.h>
#include <util.h>

#include "btpand.h"

static void tap_exit(void);
static bool tap_send(channel_t *, packet_t *);
static bool tap_recv(packet_t *);
static void tap_down(channel_t *);

void
tap_init(void)
{
	channel_t *chan;
	struct sockaddr_dl *sdl;
	struct if_laddrreq iflr;
	struct ifreq ifr;
	int fd, s;

	fd = open(interface_name, O_RDWR);
	if (fd == -1) {
		log_err("Could not open \"%s\": %m", interface_name);
		exit(EXIT_FAILURE);
	}

	memset(&ifr, 0, sizeof(ifr));
	if (ioctl(fd, TAPGIFNAME, &ifr) == -1) {
		log_err("Could not get interface name: %m");
		exit(EXIT_FAILURE);
	}
	interface_name = strndup(ifr.ifr_name, IFNAMSIZ);
	atexit(tap_exit);

	s = socket(PF_LINK, SOCK_DGRAM, 0);
	if (s == -1) {
		log_err("Could not open PF_LINK socket: %m");
		exit(EXIT_FAILURE);
	}

	memset(&iflr, 0, sizeof(iflr));
	memcpy(iflr.iflr_name, ifr.ifr_name, IFNAMSIZ);
	iflr.flags = IFLR_ACTIVE;

	sdl = satosdl(sstosa(&iflr.addr));
	sdl->sdl_family = AF_LINK;
	sdl->sdl_len = sizeof(struct sockaddr_dl);
	sdl->sdl_alen = ETHER_ADDR_LEN;
	b2eaddr(LLADDR(sdl), &local_bdaddr);

	if (ioctl(s, SIOCALIFADDR, &iflr) == -1) {
		log_err("Could not add %s link address: %m", iflr.iflr_name);
		exit(EXIT_FAILURE);
	}

	if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) {
		log_err("Could not get interface flags: %m");
		exit(EXIT_FAILURE);
	}

	if ((ifr.ifr_flags & IFF_UP) == 0) {
		ifr.ifr_flags |= IFF_UP;

		if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
			log_err("Could not set IFF_UP: %m");
			exit(EXIT_FAILURE);
		}
	}

	close(s);

	log_info("Using interface %s with addr %s",
	    ifr.ifr_name, ether_ntoa((struct ether_addr *)LLADDR(sdl)));

	chan = channel_alloc();
	if (chan == NULL)
		exit(EXIT_FAILURE);

	chan->send = tap_send;
	chan->recv = tap_recv;
	chan->down = tap_down;
	chan->mru = ETHER_HDR_LEN + ETHER_MAX_LEN;
	memcpy(chan->raddr, LLADDR(sdl), ETHER_ADDR_LEN);
	memcpy(chan->laddr, LLADDR(sdl), ETHER_ADDR_LEN);
	chan->state = CHANNEL_OPEN;
	if (!channel_open(chan, fd))
		exit(EXIT_FAILURE);

	if (pidfile(ifr.ifr_name) == -1)
		log_err("pidfile not made");
}

static void
tap_exit(void)
{
	struct ifreq ifr;
	int s;

	s = socket(PF_LINK, SOCK_DGRAM, 0);
	if (s == -1) {
		log_err("Could not open PF_LINK socket: %m");
		return;
	}

	strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
	if (ioctl(s, SIOCGIFFLAGS, &ifr) == -1) {
		log_err("Could not get interface flags: %m");
		return;
	}

	if ((ifr.ifr_flags & IFF_UP)) {
		ifr.ifr_flags &= ~IFF_UP;
		if (ioctl(s, SIOCSIFFLAGS, &ifr) == -1) {
			log_err("Could not clear IFF_UP: %m");
			return;
		}
	}

	close(s);
}

static bool
tap_send(channel_t *chan, packet_t *pkt)
{
	struct iovec iov[4];
	ssize_t nw;

	iov[0].iov_base = pkt->dst;
	iov[0].iov_len = ETHER_ADDR_LEN;
	iov[1].iov_base = pkt->src;
	iov[1].iov_len = ETHER_ADDR_LEN;
	iov[2].iov_base = pkt->type;
	iov[2].iov_len = ETHER_TYPE_LEN;
	iov[3].iov_base = pkt->ptr;
	iov[3].iov_len = pkt->len;

	/* tap device write never fails */
	nw = writev(chan->fd, iov, __arraycount(iov));
	assert(nw > 0);

	return true;
}

static bool
tap_recv(packet_t *pkt)
{

	if (pkt->len < ETHER_HDR_LEN)
		return false;

	pkt->dst = pkt->ptr;
	packet_adj(pkt, ETHER_ADDR_LEN);
	pkt->src = pkt->ptr;
	packet_adj(pkt, ETHER_ADDR_LEN);
	pkt->type = pkt->ptr;
	packet_adj(pkt, ETHER_TYPE_LEN);

	return true;
}

static void
tap_down(channel_t *chan)
{

	/* we never close the tap channel */
}
