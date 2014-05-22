/*-
 * Copyright (c) 2012 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Fleischer <paul@xpg.dk>
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
/*
 * This file is based on arch/sandpoint/stand/netboot/nif.c
 */

#include <sys/types.h>

#include <netinet/in.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/iodesc.h>

#include "dm9k.h"
#include <arch/evbarm/mini2440/mini2440_bootinfo.h>

extern struct btinfo_rootdevice bi_rdev;
extern struct btinfo_net	bi_net;

struct nifdv {
	char *name;
	int (*match)(unsigned, void *);
	void *(*init)(unsigned, void *);
	int (*send)(void *, char *, unsigned);
	int (*recv)(void *, char *, unsigned, unsigned);
	int (*halt)(void *, int);
	void *priv;
};


static struct nifdv vnifdv[] = {
	{"dme", dm9k_match, dm9k_init, dm9k_send, dm9k_recv}
};
static int nnifdv = sizeof(vnifdv)/sizeof(vnifdv[0]);
static bool nifmatch[sizeof(nnifdv)];
//static uint8_t nifaddr[sizeof(nnifdv)][6];

static struct iodesc netdesc;

void netif_match(unsigned int tag, uint8_t *macaddr);
int netif_init(unsigned int tag, uint8_t *macaddr);
int netif_open(void*);
int netif_close(int);

void
netif_match(unsigned int tag, uint8_t *macaddr)
{
	struct nifdv *dv;
	int n;

	for (n = 0; n < nnifdv; n++) {
		//memcpy(nifaddr[n], macaddr, sizeof(nifaddr[n]));
		dv = &vnifdv[n];
		if ((*dv->match)(tag, macaddr) > 0) {
			nifmatch[n] = 1;
			snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), "%s", dv->name);
			bi_rdev.cookie = tag;

			snprintf(bi_net.devname, sizeof(bi_net.devname), "%s", dv->name);
			bi_net.cookie = tag;
			memcpy(bi_net.mac_address, macaddr, sizeof(bi_net.mac_address));
			break;
		} else {
			nifmatch[n] = 0;
		}
	}
}

int
netif_init(unsigned int tag, uint8_t *macaddr)
{
	struct iodesc *s;
	struct nifdv *dv;
	int n;
	uint8_t enaddr[6];

	for (n = 0; n < nnifdv; n++) {
		if (nifmatch[n]) {
			dv = &vnifdv[n];
			goto found;
		}
	}
	return 0;
 found:
	dv->priv = (*dv->init)(tag, enaddr);
	s = &netdesc;
	s->io_netif = dv;
	memcpy(s->myea, enaddr, sizeof(s->myea));

	snprintf(bi_rdev.devname, sizeof(bi_rdev.devname), "%s", dv->name);
	bi_rdev.cookie = tag;

	snprintf(bi_net.devname, sizeof(bi_net.devname), "%s", dv->name);
	bi_net.cookie = tag;
	memcpy(bi_net.mac_address, enaddr, sizeof(bi_net.mac_address));
	return 1;
}

int
netif_open (void *cookie)
{
  return 0;
}

int
netif_close (int sock)
{
  return 0;
}


ssize_t
netif_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct nifdv *dv = desc->io_netif;

	return (*dv->send)(dv->priv, pkt, len);

}

ssize_t
netif_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timo)
{
	struct nifdv *dv = desc->io_netif;
	int len;

	len = (*dv->recv)(dv->priv, pkt, maxlen, timo);
	return len;
}

struct iodesc*
socktodesc (int i)
{
  return (i==0) ? &netdesc : NULL;
}
