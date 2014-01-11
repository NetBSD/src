/* $NetBSD: if_le.c,v 1.6 2014/01/11 15:51:02 tsutsui Exp $ */

/*
 * Copyright (c) 2013 Izumi Tsutsui.  All rights reserved.
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
/*
 * Copyright (c) 1982, 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: hp300/dev/if_le.c      7.16 (Berkeley) 3/11/93
 *
 *      @(#)if_le.c     8.1 (Berkeley) 6/10/93
 */

#include <sys/param.h>

#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>

#include <luna68k/stand/boot/samachdep.h>

/* libsa netif_driver glue functions */
static int  le_match(struct netif *, void *);
static int  le_probe(struct netif *, void *);
static void le_init(struct iodesc *, void *);
static int  le_get(struct iodesc *, void *, size_t, saseconds_t);
static int  le_put(struct iodesc *, void *, size_t);
static void le_end(struct netif *);

static void myetheraddr(uint8_t *);

/* libsa netif glue stuff */
struct netif_stats le_stats;
struct netif_dif le_ifs[] = {
	{ 0, 1, &le_stats, 0, 0, },
};

struct netif_driver le_netif_driver = {
	"le",
	le_match,
	le_probe,
	le_init,
	le_get,
	le_put,
	le_end,
	le_ifs,
	__arraycount(le_ifs),
};

#ifdef DEBUG
int debug;
#endif

int
leinit(int unit, void *addr)
{
	void *cookie;
	void *reg, *mem;
	uint8_t eaddr[6];

	reg = addr;
	mem = (void *)0x71010000;	/* XXX */

	myetheraddr(eaddr);

	cookie = lance_attach(unit, reg, mem, eaddr);
	if (cookie == NULL)
		return 0;

	printf("le%d: Am7990 LANCE Ethernet, mem at 0x%x\n",
	    unit, (uint32_t)mem);
	printf("le%d: Ethernet address = %s\n",
	    unit, ether_sprintf(eaddr));

	return 1;
}

static int
le_match(struct netif *nif, void *machdep_hint)
{
	void *cookie;
	uint8_t *eaddr;

	/* XXX should check nif_unit and unit number in machdep_hint path */

	cookie = lance_cookie(nif->nif_unit);
	if (cookie == NULL)
		return 0;

	eaddr = lance_eaddr(cookie);
	if (eaddr == NULL)
		return 0;

	return 1;
}

static int
le_probe(struct netif *nif, void *machdep_hint)
{

	/* XXX what should be checked? */

	return 0;
}

static void
le_init(struct iodesc *desc, void *machdep_hint)
{
	struct netif *nif = desc->io_netif;
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];
	void *cookie;
	uint8_t *eaddr;

#ifdef DEBUG
	printf("%s\n", __func__);
#endif

	cookie = lance_cookie(nif->nif_unit);
	eaddr = lance_eaddr(cookie);

	lance_init(cookie);

	/* fill glue stuff */
	dif->dif_private = cookie;
	memcpy(desc->myea, eaddr, 6);
}

static int
le_get(struct iodesc *desc, void *pkt, size_t maxlen, saseconds_t timeout)
{
	struct netif *nif = desc->io_netif;
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];
	void *cookie = dif->dif_private;
	int len = -1;
	saseconds_t t;

	t = getsecs() + timeout;
	while (getsecs() < t) {
		len = lance_get(cookie, pkt, len);
		if (len > 0)
			break;
	}

	return len;
}

static int
le_put(struct iodesc *desc, void *pkt, size_t len)
{
	struct netif *nif = desc->io_netif;
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];
	void *cookie = dif->dif_private;
#ifdef DEBUG
	struct ether_header *eh;

	eh = pkt;
	printf("dst:  %s\n", ether_sprintf(eh->ether_dhost));
	printf("src:  %s\n", ether_sprintf(eh->ether_shost));
	printf("type: 0x%x\n", eh->ether_type & 0xffff);
#endif

	return lance_put(cookie, pkt, len) ? len : -1;
}

static void
le_end(struct netif *nif)
{
	struct netif_dif *dif = &nif->nif_driver->netif_ifs[nif->nif_unit];
	void *cookie = dif->dif_private;

#ifdef DEBUG
	printf("%s\n", __func__);
#endif
	lance_end(cookie);
}

static void
myetheraddr(uint8_t *ether)
{
	unsigned int i, loc;
	uint8_t *ea;
	volatile struct { uint32_t ctl; } *ds1220;

	switch (machtype) {
	case LUNA_I:
		ea = (uint8_t *)0x4101FFE0;
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			int u, l;

			u = ea[0];
			u = (u < 'A') ? u & 0xf : u - 'A' + 10;
			l = ea[1];
			l = (l < 'A') ? l & 0xf : l - 'A' + 10;

			ether[i] = l | (u << 4);
			ea += 2;
		}
		break;
	case LUNA_II:
		ds1220 = (void *)0xF1000004;
		loc = 12;
		for (i = 0; i < ETHER_ADDR_LEN; i++) {
			unsigned int u, l, hex;

			ds1220->ctl = (loc) << 16;
			u = 0xf0 & (ds1220->ctl >> 12);
			ds1220->ctl = (loc + 1) << 16;
			l = 0x0f & (ds1220->ctl >> 16);
			hex = (u < '9') ? l : l + 9;

			ds1220->ctl = (loc + 2) << 16;
			u = 0xf0 & (ds1220->ctl >> 12);
			ds1220->ctl = (loc + 3) << 16;
			l = 0x0f & (ds1220->ctl >> 16);

			ether[i] = ((u < '9') ? l : l + 9) | (hex << 4);
			loc += 4;
		}
		break;
	default:
		ether[0] = 0x00; ether[1] = 0x00; ether[2] = 0x0a;
		ether[3] = 0xDE; ether[4] = 0xAD; ether[5] = 0x00;
		break;
	}
}
