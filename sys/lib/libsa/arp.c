/*	$NetBSD: arp.c,v 1.4 1995/02/20 11:04:00 mycroft Exp $	*/

/*
 * Copyright (c) 1992 Regents of the University of California.
 * All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
 * @(#) Header: arp.c,v 1.5 93/07/15 05:52:26 leres Exp  (LBL)
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>

#include <netinet/if_ether.h>
#include <netinet/in_systm.h>

#include <string.h>

#include "stand.h"
#include "net.h"

/* Cache stuff */
#define ARP_NUM 8			/* need at most 3 arp entries */

static struct arp_list {
	n_long	addr;
	u_char	ea[6];
} arp_list[ARP_NUM] = {
	{ INADDR_BROADCAST, BA }
};
static	int arp_num = 1;

/* Local forwards */
static	size_t arpsend __P((struct iodesc *, void *, size_t));
static	size_t arprecv __P((struct iodesc *, void *, size_t, time_t));

/* Broadcast an ARP packet, asking who has addr on interface d */
u_char *
arpwhohas(d, addr)
	register struct iodesc *d;
	n_long addr;
{
	register int i;
	register struct ether_arp *ah;
	register struct arp_list *al;
	struct {
		u_char	header[ETHER_SIZE];
		struct	ether_arp warp;
	} wbuf;
	struct {
		u_char	header[ETHER_SIZE];
		struct	ether_arp rarp;
	} rbuf;

#ifdef ARP_DEBUG
 	if (debug)
 	    printf("arpwhohas: called for %s\n", intoa(addr));
#endif
	/* Try for cached answer first */
	for (i = 0, al = arp_list; i < arp_num; ++i, ++al)
		if (addr == al->addr)
			return (al->ea);

	/* Don't overflow cache */
	if (arp_num > ARP_NUM - 1)
		panic("arpwhohas: overflowed arp_list!");

#ifdef ARP_DEBUG
 	if (debug)
		printf("arpwhohas: not cached\n");
#endif

	ah = &wbuf.warp;
	bzero(ah, sizeof(*ah));

	ah->arp_hrd = htons(ARPHRD_ETHER);
	ah->arp_pro = htons(ETHERTYPE_IP);
	ah->arp_hln = sizeof(ah->arp_sha); /* hardware address length */
	ah->arp_pln = sizeof(ah->arp_spa); /* protocol address length */
	ah->arp_op = htons(ARPOP_REQUEST);
	MACPY(d->myea, ah->arp_sha);
	bcopy(&d->myip, ah->arp_spa, sizeof(ah->arp_spa));
	bcopy(&addr, ah->arp_tpa, sizeof(ah->arp_tpa));

	/* Store ip address in cache */
	al->addr = addr;

	(void)sendrecv(d,
	    arpsend, ah, sizeof(*ah),
	    arprecv, &rbuf.rarp, sizeof(rbuf.rarp));

	/* Store ethernet address in cache */
	MACPY(rbuf.rarp.arp_sha, al->ea);
	++arp_num;

#ifdef ARP_DEBUG
 	if (debug)
		printf("arpwhohas: cacheing %s --> %s\n", intoa(addr), ether_sprintf(al->ea));
#endif

	return (al->ea);
}

static size_t
arpsend(d, pkt, len)
	register struct iodesc *d;
	register void *pkt;
	register size_t len;
{

#ifdef ARP_DEBUG
 	if (debug)
		printf("arpsend: called\n");
#endif

	return (sendether(d, pkt, len, bcea, ETHERTYPE_ARP));
}

/* Returns 0 if this is the packet we're waiting for else -1 (and errno == 0) */
static size_t
arprecv(d, pkt, len, tleft)
	register struct iodesc *d;
	register void *pkt;
	register size_t len;
	time_t tleft;
{
	register struct ether_header *eh;
	register struct ether_arp *ah;

#ifdef ARP_DEBUG
 	if (debug)
 		printf("arprecv: called\n");
#endif

	len = readether(d, pkt, len, tleft);
	if (len == -1 || len < sizeof(struct ether_arp))
		goto bad;

	eh = (struct ether_header *)pkt - 1;
	/* Must be to us */
	if (bcmp(d->myea, eh->ether_dhost, 6) != 0 &&
	    bcmp(bcea, eh->ether_dhost, 6) != 0) {
#ifdef ARP_DEBUG
		if (debug)
			printf("arprecv: not ours %s\n", ether_sprintf(eh->ether_dhost));
#endif
		goto bad;
	}
	if (ntohs(eh->ether_type) != ETHERTYPE_ARP) {
#ifdef ARP_DEBUG
		if (debug)
			printf("arprecv: not arp %d\n", ntohs(eh->ether_type));
#endif
		goto bad;
	}

	ah = (struct ether_arp *)pkt;
	HTONS(ah->arp_hrd);
	HTONS(ah->arp_pro);
	HTONS(ah->arp_op);
	if (ah->arp_hrd != ARPHRD_ETHER || ah->arp_pro != ETHERTYPE_IP ||
	    ah->arp_hln != sizeof(ah->arp_sha) ||
	    ah->arp_pln != sizeof(ah->arp_spa) || ah->arp_op != ARPOP_REPLY) {
#ifdef ARP_DEBUG
		if (debug)
			printf("arprecv: not valid reply\n");
#endif
		goto bad;
	}
	if (bcmp(&arp_list[arp_num].addr, ah->arp_spa, sizeof(long)) != 0 ||
	    bcmp(&d->myip, ah->arp_tpa, sizeof(d->myip)) != 0) {
#ifdef ARP_DEBUG
		if (debug)
			printf("arprecv: already cached??\n");
#endif
		goto bad;
	}

	return (0);

bad:
	errno = 0;
	return (-1);
}
