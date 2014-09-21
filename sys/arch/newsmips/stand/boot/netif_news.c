/*	$NetBSD: netif_news.c,v 1.9 2014/09/21 16:35:44 christos Exp $	*/

/*
 * Copyright (c) 1995 Gordon W. Ross
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

/*
 * The Sun PROM has a fairly general set of network drivers,
 * so it is easiest to just replace the netif module with
 * this adaptation to the PROM network interface.
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <net/if.h>
#include <net/if_ether.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libkern/libkern.h>

#include <machine/apcall.h>
#include <promdev.h>

#include "netif_news.h"

#ifdef NETIF_DEBUG
int netif_debug;
#endif

static struct iodesc sdesc;

struct iodesc *
socktodesc(int sock)
{
	if (sock != 0) {
		return NULL;
	}
	return &sdesc;
}

int
netif_news_open(struct romdev *pd)
{
	struct iodesc *io;

	/* find a free socket */
	io = &sdesc;
	if (io->io_netif) {
#ifdef	DEBUG
		printf("netif_open: device busy\n");
#endif
		errno = ENFILE;
		return -1;
	}
	memset(io, 0, sizeof(*io));

	io->io_netif = pd;

	/* Put our ethernet address in io->myea */
	prom_getether(pd, io->myea);

	return 0;
}

void
netif_news_close(int fd)
{
	struct iodesc *io;

	io = &sdesc;
	io->io_netif = NULL;
}

/*
 * Send a packet.  The ether header is already there.
 * Return the length sent (or -1 on error).
 */
ssize_t
netif_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct romdev *pd;
	ssize_t rv;
	size_t sendlen;

	pd = (struct romdev *)desc->io_netif;

#ifdef NETIF_DEBUG
	if (netif_debug) {
		struct ether_header *eh;

		printf("netif_put: desc=0x%x pkt=0x%x len=%d\n",
			   desc, pkt, len);
		eh = pkt;
		printf("dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("src: %s ", ether_sprintf(eh->ether_shost));
		printf("type: 0x%x\n", eh->ether_type & 0xFFFF);
	}
#endif

	sendlen = len;
	if (sendlen < 60) {
		sendlen = 60;
#ifdef NETIF_DEBUG
		printf("netif_put: length padded to %d\n", sendlen);
#endif
	}

	rv = apcall_write(pd->fd, pkt, sendlen);

#ifdef NETIF_DEBUG
	if (netif_debug)
		printf("netif_put: xmit returned %d\n", rv);
#endif

	return rv;
}

/*
 * Receive a packet, including the ether header.
 * Return the total length received (or -1 on error).
 */
ssize_t
netif_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timo)
{
	struct romdev *pd;
	satime_t tick0;
	ssize_t len;

	pd = (struct romdev *)desc->io_netif;

#ifdef NETIF_DEBUG
	if (netif_debug)
		printf("netif_get: pkt=0x%x, maxlen=%d, tmo=%d\n",
			   pkt, maxlen, timo);
#endif

	tick0 = getsecs();

	do {
		len = apcall_read(pd->fd, pkt, maxlen);
	} while ((len == 0) && ((getsecs() - tick0) < timo));

#ifdef NETIF_DEBUG
	if (netif_debug)
		printf("netif_get: received len=%d\n", len);
#endif

	if (len < 12)
		return -1;

#ifdef NETIF_DEBUG
	if (netif_debug) {
		struct ether_header *eh = pkt;

		printf("dst: %s ", ether_sprintf(eh->ether_dhost));
		printf("src: %s ", ether_sprintf(eh->ether_shost));
		printf("type: 0x%x\n", eh->ether_type & 0xFFFF);
	}
#endif

	return len;
}

int
prom_getether(struct romdev *pd, u_char *ea)
{

	if (apcall_ioctl(pd->fd, APIOCGIFHWADDR, ea))
		return -1;

#ifdef BOOT_DEBUG
	printf("hardware address %s\n", ether_sprintf(ea));
#endif

	return 0;
}

satime_t
getsecs(void)
{
	u_int t[2];

	apcall_gettimeofday(t);		/* time = t[0](s) + t[1](ns) */
	return t[0];
}
