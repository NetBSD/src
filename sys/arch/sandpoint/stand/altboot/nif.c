/* $NetBSD: nif.c,v 1.5.30.1 2014/08/10 06:54:07 tls Exp $ */

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

#include <machine/bootinfo.h>

#include "globals.h"

struct nifdv {
	char *name;
	int (*match)(unsigned, void *);
	void *(*init)(unsigned, void *);
	int (*send)(void *, char *, unsigned);
	int (*recv)(void *, char *, unsigned, unsigned);
	void (*shutdown)(void *);
	void *priv;
};

static struct iodesc netdesc;

static struct nifdv lnifdv[] = {
	{ "fxp",  fxp_match, fxp_init, fxp_send, fxp_recv },
	{ "tlp",  tlp_match, tlp_init, tlp_send, tlp_recv },
	{ "re",   rge_match, rge_init, rge_send, rge_recv },
	{ "sk",   skg_match, skg_init, skg_send, skg_recv },
	{ "stge", stg_match, stg_init, stg_send, stg_recv, stg_shutdown },
};
static int nnifdv = sizeof(lnifdv)/sizeof(lnifdv[0]);

int
netif_init(void *self)
{
	struct pcidev *pci = self;
	struct iodesc *s;
	struct nifdv *dv;
	unsigned tag;
	int n;
	uint8_t enaddr[6];
	extern struct btinfo_net bi_net;
	extern struct btinfo_rootdevice bi_rdev;

	tag = pci->bdf;
	for (n = 0; n < nnifdv; n++) {
		dv = &lnifdv[n];
		if ((*dv->match)(tag, NULL) > 0)
			goto found;
	}
	pci->drv = NULL;
	return 0;
  found:
	pci->drv = dv->priv = (*dv->init)(tag, enaddr);
	s = &netdesc;
	s->io_netif = dv;
	memcpy(s->myea, enaddr, sizeof(s->myea));
	/* build btinfo to identify NIF device */
	snprintf(bi_net.devname, sizeof(bi_net.devname), "%s", dv->name);
	memcpy(bi_net.mac_address, enaddr, sizeof(bi_net.mac_address));
	bi_net.cookie = tag;
	snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), "%s", dv->name);
	bi_rdev.cookie = tag;
	return 1;
}

void
netif_shutdown_all(void)
{
	struct nifdv *dv;

	/* currently there can be only one NIF */
	dv = netdesc.io_netif;
	if (dv != NULL)
		if (dv->shutdown != NULL)
			(*dv->shutdown)(dv->priv);
	memset(&netdesc, 0, sizeof(netdesc));
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

	return dv ? (*dv->send)(dv->priv, pkt, len) : -1;
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

	len = dv ? (*dv->recv)(dv->priv, pkt, maxlen, timo) : -1;
	if (len == -1)
		printf("timeout\n");
	return len;
}

struct iodesc *
socktodesc(int num)
{

	return (num == 0) ? &netdesc : NULL;
}
