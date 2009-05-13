/* $NetBSD: nif.c,v 1.9.2.1 2009/05/13 17:18:16 jym Exp $ */

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

#include "globals.h"

struct nifdv {
	char *name;
	int (*match)(unsigned, void *);
	void *(*init)(unsigned, void *);
	int (*send)(void *, char *, unsigned);
	int (*recv)(void *, char *, unsigned, unsigned);
	int (*halt)(void *, int);
	void *priv;
};

static struct iodesc netdesc;

static struct nifdv vnifdv[] = {
	{ "fxp", fxp_match, fxp_init, fxp_send, fxp_recv },
	{ "tlp", tlp_match, tlp_init, tlp_send, tlp_recv },
	{ "nvt", nvt_match, nvt_init, nvt_send, nvt_recv },
	{ "sip", sip_match, sip_init, sip_send, sip_recv },
	{ "pcn", pcn_match, pcn_init, pcn_send, pcn_recv },
	{ "kse", kse_match, kse_init, kse_send, kse_recv },
	{ "sme", sme_match, sme_init, sme_send, sme_recv },
	{ "vge", vge_match, vge_init, vge_send, vge_recv },
	{ "rge", rge_match, rge_init, rge_send, rge_recv },
	{ "wm",  wm_match, wm_init,  wm_send,  wm_recv  }
};
static int nnifdv = sizeof(vnifdv)/sizeof(vnifdv[0]);

int
netif_init(unsigned tag)
{
	struct iodesc *s;
	struct nifdv *dv;
	int n;
	uint8_t enaddr[6];
	extern uint8_t en[6];
	extern char rootdev[4];

	for (n = 0; n < nnifdv; n++) {
		dv = &vnifdv[n];
		if ((*dv->match)(tag, NULL) > 0)
			goto found;
	}
	return 0;
  found:
	dv->priv = (*dv->init)(tag, enaddr);
	s = &netdesc;
	s->io_netif = dv;
	memcpy(s->myea, enaddr, sizeof(s->myea));
	memcpy(en, enaddr, sizeof(en));
	snprintf(rootdev, sizeof(rootdev), "%s", dv->name);
	return 1;
}

int
netif_open(void *cookie)
{
	/* single action */
	return 0;
}

int
netif_close(int sock)
{
	/* nothing to do for the HW */
	return 0;
}

/*
 * Send a packet.  The ether header is already there.
 * Return the length sent (or -1 on error).
 */
ssize_t
netif_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct nifdv *dv = desc->io_netif;

	return (*dv->send)(dv->priv, pkt, len);
}

/*
 * Receive a packet, including the ether header.
 * Return the total length received (or -1 on error).
 */
ssize_t
netif_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timo)
{
	struct nifdv *dv = desc->io_netif;
	int len;

	len = (*dv->recv)(dv->priv, pkt, maxlen, timo);
	if (len == -1)
		printf("timeout\n");
	return len;
}

struct iodesc *
socktodesc(int num)
{

	return (num == 0) ? &netdesc : NULL;
}
