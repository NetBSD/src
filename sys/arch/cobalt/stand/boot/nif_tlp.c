/*	$NetBSD: nif_tlp.c,v 1.1.24.2 2008/01/06 05:00:53 wrstuden Exp $	*/

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libsa/dev_net.h>

#include "boot.h"

static int tlp_match(struct netif *, void *);
static int tlp_probe(struct netif *, void *);
static void tlp_attach(struct iodesc *, void *);
static int tlp_get(struct iodesc *, void *, size_t, time_t);
static int tlp_put(struct iodesc *, void *, size_t);
static void tlp_end(struct netif *);

#define MIN_LEN		60	/* ETHER_MIN_LEN - ETHER_CRC_LEN */

static struct netif_stats tlp_stats[1];

static struct netif_dif tlp_ifs[] = {
	{ 0, 1, &tlp_stats[0], NULL, 0 },
};

struct netif_driver ether_tlp_driver = {
	"tlp",
	tlp_match,
	tlp_probe,
	tlp_attach,
	tlp_get,
	tlp_put,
	tlp_end,
	tlp_ifs,
	1,
};

#ifdef DEBUG
int debug = 1;		/* referred in various libsa net sources */
#endif

int
tlp_match(struct netif *netif, void *hint)
{

	/* always match for onboard tlp */
	return 1;
}

int
tlp_probe(struct netif *netif, void *hint)
{

	/* XXX */
	return 0;
}

void
tlp_attach(struct iodesc *desc, void *hint)
{
	struct netif *nif = desc->io_netif;
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];

	dif->dif_private = tlp_init(&desc->myea);
}

int
tlp_get(struct iodesc *desc, void *pkt, size_t maxlen, time_t timeout)
{
	int len;
	struct netif *nif = desc->io_netif;
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];
	void *l = dif->dif_private;

	len = tlp_recv(l, pkt, maxlen, timeout);
	if (len == -1) {
		printf("tlp: receive timeout\n");
		/* XXX */
	}

	if (len < MIN_LEN)
		len = -1;

	return len;
}

int
tlp_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct netif *nif = desc->io_netif;
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];
	void *l = dif->dif_private;
	int rv;
	size_t sendlen;

	sendlen = len;
	if (sendlen < MIN_LEN)
		sendlen = MIN_LEN;	/* XXX */

	rv = tlp_send(l, pkt, sendlen);

	return rv;
}

void
tlp_end(struct netif *netif)
{
}
