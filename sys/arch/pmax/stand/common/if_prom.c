/*      $NetBSD: if_prom.c,v 1.1.2.2 2002/02/28 04:11:24 nathanw Exp $ */

/* Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Gregory McGarry.
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
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>

#include <net/if_ether.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <lib/libsa/stand.h>
#include <lib/libsa/net.h>
#include <lib/libsa/netif.h>
#include <lib/libkern/libkern.h>

#include <machine/dec_prom.h>
#include <stand/common/common.h>

#ifdef NET_DEBUG
void dump_packet_info __P((void *, int));
#endif

/*
 *  For some reason the proms won't pass arp responses back to us.  I
 *  have checked if the first parameter to bootread/bootwrite do anything
 *  but it doesn't appear so.  Therefore, we stop the upper layers from
 *  sending arp requests in the first place, by monitoring packets which
 *  come in and filling the arp cache ourselves.  - gmcgarry
 */
#ifdef FILL_ARPCACHE
struct arp_list {
	struct in_addr	addr;
	u_char		ea[6];
};
extern struct arp_list arp_list[8];
extern int arp_num;
void fill_arpcache __P((void *, int));
#endif

/* forward declarations */
int prom_probe __P((struct netif *, void *));
int prom_match __P((struct netif *, void *));
void prom_init __P((struct iodesc *, void *));
int prom_get __P((struct iodesc *, void *, size_t, time_t));
int prom_put __P((struct iodesc *, void *, size_t));
void prom_end __P((struct netif *));

extern int try_bootp;

extern struct netif_stats       prom_stats[];
struct netif_dif prom_ifs[] = {
/*	dif_unit	dif_nsel	dif_stats	dif_private	*/
{	0,		1,		&prom_stats[0],	0,		},
};
struct netif_stats prom_stats[NENTS(prom_ifs)];

struct netif_driver prom_netif_driver = {
	"prom",				/* netif_bname */
	prom_match,			/* netif_match */
	prom_probe,			/* netif_probe */
	prom_init,			/* netif_init */
	prom_get,			/* netif_get */
	prom_put,			/* netif_put */
	prom_end,			/* netif_end */
	prom_ifs,			/* netif_ifs */
	NENTS(prom_ifs)			/* netif_nifs */
};

static int sc_fd;				/* PROM file id */

int
prom_match(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{

#ifdef NET_DEBUG
	printf("prom_match: called\n");
#endif
	return (1);
}


int
prom_probe(nif, machdep_hint)
	struct netif *nif;
	void *machdep_hint;
{

#ifdef NET_DEBUG
	printf("prom_probe: called\n");
#endif
	return 0;
}


void
prom_init(desc, machdep_hint)
	struct iodesc *desc;
	void *machdep_hint;
{
	char *device = desc->io_netif->nif_driver->netif_bname;
	char *c, *enet;
	int i, j, num;

#ifdef NET_DEBUG
	printf("prom_init: called\n");
#endif

	try_bootp = 1;

	/*
	 * Get our hardware address (this prom call is one of the rare ones
         * which is the same for new and old proms)
         */
	enet = (*callv->_getenv)("enet");

#ifdef NET_DEBUG
	if (debug)
		printf("enet=%s\n", enet);
#endif

	i=0;
	c = enet;
	for (i=0; i<6; i++) {
		j = *c - '0';
		num = (j<10?j:j-39);
		num <<= 4;
		c++;
		j = *c - '0';
		num += (j<10?j:j-39);
		desc->myea[i] = num;
		c++;
		c++; /* skip '-' */
	}

	desc->xid = 0x66d30000;

	if (callv == &callvec)
		sc_fd = prom_open(device, 0);
	else
		sc_fd = (*callv->_bootinit)(device);

	if (sc_fd < 0)
		printf("problem initialising device\n");
}


int
prom_put(desc, pkt, len)
	struct iodesc *desc;
	void *pkt;
	size_t len;
{
	int s;

#ifdef NET_DEBUG
	printf("prom_put: called\n");
#endif

#ifdef NET_DEBUG
	if (debug)
		dump_packet_info(pkt,len);
#endif

	if (callv == &callvec)
		s = prom_write(sc_fd, pkt, len);
	else {
		s = (*callv->_bootwrite)(0, pkt, len);
		(*callv->_wbflush)(); /* didn't really make a difference */
	}
	if (s < 0)
		return (EIO);
	return s;
}


int
prom_get(desc, pkt, len, timeout)
	struct iodesc *desc;
	void *pkt;
	size_t len;
	time_t timeout;
{
	int s;
	time_t t;

#ifdef NET_DEBUG
	printf("prom_get: called\n");
#endif

	t = getsecs();
	s = 0;
	while (((getsecs() - t) < timeout) && !s) {
		if (callv == &callvec)
			s = prom_read(sc_fd, pkt, len);
		else
			s = (*callv->_bootread)(0, pkt, len);
	}

#ifdef FILL_ARPCACHE
	if (s > 0)
		fill_arpcache(pkt,s);
#endif

	return s;

}


void
prom_end(nif)
	struct netif *nif;
{

#ifdef NET_DEBUG
	printf("prom_end: called\n");
#endif

	if (callv == &callvec)
		prom_close(sc_fd);
}


#ifdef FILL_ARPCACHE
void fill_arpcache (pkt, len)
	void *pkt;
	int len;
{
	int i;
	struct arp_list *al;
	struct ether_header *eh = (struct ether_header *)pkt;
	struct ip *ih = (struct ip *)(eh + 1);

#ifdef NET_DEBUG
	if (debug)
		dump_packet_info(pkt, len);
#endif

	if (ntohs(eh->ether_type) == 0x0800) {

		/* check arp cache */
		for (i=0, al=arp_list; i<arp_num; ++i, ++al) {
			if (al->addr.s_addr == ih->ip_src.s_addr) {
				/* already in cache */
				return;
			}
		}
        	if (arp_num > 7)
               		arp_num = 1;    /* recycle */
		al->addr.s_addr = ih->ip_src.s_addr;
		for (i=0; i<6; i++)
			al->ea[i] = eh->ether_shost[i];
		++arp_num;
	}

}
#endif

#ifdef NET_DEBUG
void dump_packet_info(pkt, len)
	void *pkt;
	int len;
{
	struct ether_header *eh = (struct ether_header *)pkt;
	struct ip *ih = (struct ip *)(eh + 1);

	printf("ether_dhost = %s\n", ether_sprintf(eh->ether_dhost));
	printf("ether_shost = %s\n", ether_sprintf(eh->ether_shost));
	printf("ether_type = 0x%x\n", ntohs(eh->ether_type));

	if (ntohs(eh->ether_type) == 0x0800) {
		printf("ip packet version %d\n", ih->ip_v);
		printf("source ip: 0x%x\n", ih->ip_src.s_addr);
		printf("dest ip: 0x%x\n", ih->ip_dst.s_addr);

	}

}
#endif
