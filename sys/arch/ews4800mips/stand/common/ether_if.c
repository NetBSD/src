/*	$NetBSD: ether_if.c,v 1.1 2005/12/29 15:20:09 tsutsui Exp $	*/

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

#include <lib/libsa/stand.h>
#include <lib/libkern/libkern.h>

#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>

#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libsa/bootp.h>
#include <lib/libsa/dev_net.h>

#include <machine/sbd.h>
#define	_SBD_TR2A_PRIVATE
#include <machine/sbd_tr2a.h>	/* getsecs. */

#include "local.h"

struct devsw netdevsw = {
	"net", net_strategy, net_open, net_close, net_ioctl
};

int ether_match(struct netif *, void *);
int ether_probe(struct netif *, void *);
void ether_init(struct iodesc *, void *);
int ether_get(struct iodesc *, void *, size_t, time_t);
int ether_put(struct iodesc *, void *, size_t);
void ether_end(struct netif *);

extern boolean_t lance_init(void);
extern void lance_eaddr(uint8_t *);
extern boolean_t lance_get(void *, size_t);
extern boolean_t lance_put(void *, size_t);

struct netif_stats ether_stats[1];

struct netif_dif ether_ifs[] = {
	{ 0, 1, &ether_stats[0], 0, },
};

struct netif_driver __netif_driver = {
	"ether",
	ether_match,
	ether_probe,
	ether_init,
	ether_get,
	ether_put,
	ether_end,
	ether_ifs,
	1,
};

int debug = 1;
int n_netif_drivers = 1;
struct netif_driver *netif_drivers[1] = { &__netif_driver };

int
ether_match(struct netif *netif, void *hint)
{

	return SBD_INFO->machine == MACHINE_TR2A;
}

int
ether_probe(struct netif *netif, void *hint)
{

	return 0;
}

void
ether_init(struct iodesc *iodesc, void *hint)
{

	lance_init();
	lance_eaddr(iodesc->myea);
}

int
ether_get(struct iodesc *iodesc, void *pkt, size_t len, time_t timeout)
{

	return lance_get(pkt, len) ? len : -1;
}

int
ether_put(struct iodesc *iodesc, void *pkt, size_t len)
{

	return lance_put(pkt, len) ? len : -1;
}

void
ether_end(struct netif *netif)
{

}

void
_rtt(void)
{

	while (/*CONSTCOND*/1)
		;
	/* NOTREACHED */
}

time_t
getsecs(void)
{

	return (time_t)*(RTC_MK48T18_ADDR + 4);
}
