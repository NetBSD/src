/* $NetBSD: if_ne.c,v 1.1.2.3 2013/01/16 05:33:08 yamt Exp $ */

/*
 * Copyright (c) 2003 Tetsuya Isaki. All rights reserved.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>

#include "ne.h"
#include "dp8390.h"

extern int badbaddr(void *);

static int  ne_match(struct netif *, void *);
static int  ne_probe(struct netif *, void *);
static void ne_init(struct iodesc *, void *);
static int  ne_get(struct iodesc *, void *, size_t, saseconds_t);
static int  ne_put(struct iodesc *, void *, size_t);
static void ne_end(struct netif *);

uint neaddr_conf[] = {
	0xece200,	/* Neptune-X at ISA addr 0x300 */
	0xece300,	/* Nereid #1 */
	0xeceb00,	/* Nereid #2 */
};
uint neaddr;


static int
ne_match(struct netif *nif, void *machdep_hint)
{
	int i;

	for (i = 0; i < sizeof(neaddr_conf) / sizeof(neaddr_conf[0]); i++) {
		if (!badbaddr((void *)neaddr_conf[i])) {
			neaddr = neaddr_conf[i];
			return 1;
		}
	}
	printf("ne_match: no match\n");
	return 0;
}

static int
ne_probe(struct netif *nif, void *machdep_hint)
{

	return 0;
}

static void
ne_init(struct iodesc *desc, void *machdep_hint)
{

#ifdef DEBUG
	printf("ne_init\n");
#endif
	if (EtherInit(desc->myea) == 0) {
		printf("EtherInit failed?\n");
		exit(1);
	}

	printf("ethernet address = %s\n", ether_sprintf(desc->myea));
}

static int
ne_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timeout)
{
	int len = 0;
	saseconds_t t;

	t = getsecs() + timeout;
	while (getsecs() < t) {
		len = EtherReceive(pkt, maxlen);
		if (len)
			break;
	}

	return len;
}

static int
ne_put(struct iodesc *desc, void *pkt, size_t len)
{
#ifdef DEBUG
 	struct ether_header *eh;
 
 	eh = pkt;
 	printf("dst:  %s\n", ether_sprintf(eh->ether_dhost));
 	printf("src:  %s\n", ether_sprintf(eh->ether_shost));
 	printf("type: 0x%x\n", eh->ether_type & 0xffff);
#endif
 
	return EtherSend(pkt, len);
}

static void
ne_end(struct netif *nif)
{

#ifdef DEBUG
	printf("ne_end\n");
#endif

	EtherStop();
}


struct netif_stats ne_stats;
struct netif_dif ne_ifs[] = {
	{ 0, 1, &ne_stats, 0, 0, },
};

struct netif_driver ne_netif_driver = {
	"ne",
	ne_match,
	ne_probe,
	ne_init,
	ne_get,
	ne_put,
	ne_end,
	ne_ifs,
	sizeof(ne_ifs) / sizeof(ne_ifs[0]),
};
