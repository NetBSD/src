/* $NetBSD: nif_fxp.c,v 1.3.2.2 2007/10/27 11:28:25 yamt Exp $ */

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

struct iodesc sockets[2]; /* SOPEN_MAX */

struct iodesc *socktodesc(int);
void netif_init(void);
int netif_open(void *);
int netif_close(int);
ssize_t netif_put(struct iodesc *, void *, size_t);
ssize_t netif_get(struct iodesc *, void *, size_t, time_t);

void *fxp_init(void *);
int fxp_send(void *, char *, unsigned);
int fxp_recv(void *, char *, unsigned, unsigned);

struct iodesc *
socktodesc(sock)
	int sock;
{
	if (sock < 0 || sock >= 2)
		return NULL;
	return &sockets[sock];
}

void
netif_init()
{
	struct iodesc *s;
	extern uint8_t en[];

	s = &sockets[0];
	s->io_netif = fxp_init(s->myea); /* determine MAC address */

	memcpy(en, s->myea, sizeof(s->myea));
}

int
netif_open(cookie)
	void *cookie;
{

	return 0;
}

int
netif_close(sock)
	int sock;
{

	/* nothing to do for the HW */
	return 0;
}

/*
 * Send a packet.  The ether header is already there.
 * Return the length sent (or -1 on error).
 */
ssize_t
netif_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	size_t len;
{
	ssize_t rv;
	size_t sendlen;

	sendlen = len;
	if (sendlen < 60)
		sendlen = 60;

	rv = fxp_send(desc->io_netif, pkt, sendlen);

	return rv;
}

/*
 * Receive a packet, including the ether header.
 * Return the total length received (or -1 on error).
 */
ssize_t
netif_get(desc, pkt, maxlen, timo)
	struct iodesc *desc;
	void *pkt;
	size_t maxlen;
	time_t timo;
{
	int len;

	len = fxp_recv(desc->io_netif, pkt, maxlen, timo);
	if (len == -1) {
		printf("timeout\n");
		/* XXX errno = ... */
	}

	if (len < 12)
		return -1;
	return len;
}
