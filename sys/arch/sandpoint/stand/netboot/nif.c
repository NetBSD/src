/* $NetBSD: nif.c,v 1.3.4.3 2008/01/09 01:48:37 matt Exp $ */

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

#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>

struct nifdv {
	char *name;
	void *(*init)(unsigned, void *);
	int (*send)(void *, char *, unsigned);
	int (*recv)(void *, char *, unsigned, unsigned);
	int (*halt)(void *, int);
	void *priv;
	int unit;
};

int netif_init(unsigned);
int netif_open(void *);
int netif_close(int);

static struct iodesc netdesc;

#define NIF_DECL(xxx) \
    void * xxx ## _init(unsigned, void *); \
    int xxx ## _send(void *, char *, unsigned); \
    int xxx ## _recv(void *, char *, unsigned, unsigned);

NIF_DECL(fxp);
NIF_DECL(tlp);
NIF_DECL(nvt);
NIF_DECL(sip);
NIF_DECL(pcn);
NIF_DECL(vge);
NIF_DECL(rge);
NIF_DECL(wm);

static struct nifdv vnifdv[] = {
	{ "fxp", fxp_init, fxp_send, fxp_recv },
	{ "tlp", tlp_init, tlp_send, tlp_recv },
	{ "nvt", nvt_init, nvt_send, nvt_recv },
	{ "sip", sip_init, sip_send, sip_recv },
	{ "pcn", pcn_init, pcn_send, pcn_recv },
	{ "vge", vge_init, vge_send, vge_recv },
	{ "rge", rge_init, rge_send, rge_recv },
	{ "wm",  wm_init,  wm_send,  wm_recv  }
};
static int nnifdv = sizeof(vnifdv)/sizeof(vnifdv[0]);
int nmatchednif = 0;

int
netif_init(tag)
	unsigned tag;
{
	struct iodesc *s;
	struct nifdv *dv;
	int n;
	void *l;
	uint8_t enaddr[6];
	extern uint8_t en[6];
	extern char rootdev[4];

	for (n = 0; n < nnifdv; n++) {
		l = (*vnifdv[n].init)(tag, enaddr);
		if (l != NULL)
			goto found;
	}
	return 0;
  found:
	dv = alloc(sizeof(struct nifdv));
	*dv = vnifdv[n];
	dv->priv = l;
	dv->unit = nmatchednif++;
	s = &netdesc;
	s->io_netif = dv;
	memcpy(s->myea, enaddr, sizeof(enaddr));
	memcpy(en, enaddr, sizeof(en));
	snprintf(rootdev, sizeof(rootdev), "%s", dv->name);
	return 1;
}

int
netif_open(cookie)
	void *cookie;
{
	/* single action */
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
	struct nifdv *dv = desc->io_netif;

	return (*dv->send)(dv->priv, pkt, len);
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
	struct nifdv *dv = desc->io_netif;
	int len;

	len = (*dv->recv)(dv->priv, pkt, maxlen, timo);
	if (len == -1)
		printf("timeout\n");
	return len;
}

struct iodesc *
socktodesc(num)
	int num;
{

	return (num == 0) ? &netdesc : NULL;
}
