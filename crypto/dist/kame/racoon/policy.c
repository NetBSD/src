/*	$KAME: policy.c,v 1.33 2000/12/15 13:43:57 sakane Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/queue.h>

#include <netkey/key_var.h>
#include <netinet/in.h>
#include <netinet6/ipsec.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#include "policy.h"
#include "localconf.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "handler.h"

static TAILQ_HEAD(_sptree, secpolicy) sptree;

/* perform exact match against security policy table. */
struct secpolicy *
getsp(spidx)
	struct policyindex *spidx;
{
	struct secpolicy *p;

	for (p = TAILQ_FIRST(&sptree); p; p = TAILQ_NEXT(p, chain)) {
		if (!cmpspidx(spidx, &p->spidx))
			return p;
	}

	return NULL;
}

/*
 * perform non-exact match against security policy table, only if this is
 * transport mode SA negotiation.  for example, 0.0.0.0/0 -> 0.0.0.0/0
 * entry in policy.txt can be returned when we're negotiating transport
 * mode SA.  this is how the kernel works.
 */
#if 1
struct secpolicy *
getsp_r(spidx)
	struct policyindex *spidx;	/* from peer */
{
	struct secpolicy *p;

	for (p = TAILQ_FIRST(&sptree); p; p = TAILQ_NEXT(p, chain)) {
		if (!cmpspidx_wild(spidx, &p->spidx))
			return p;
	}

	return NULL;
}
#else
struct secpolicy *
getsp_r(spidx, iph2)
	struct policyindex *spidx;	/* from peer */
	struct ph2handle *iph2;
{
	struct secpolicy *p;
	u_int8_t prefixlen;

	plog(LLV_DEBUG, LOCATION, NULL, "checking for transport mode\n");

	if (spidx->src.ss_family != spidx->dst.ss_family) {
		plog(LLV_ERROR, LOCATION, NULL,
			"address family mismatch, src:%d dst:%d\n",
				spidx->src.ss_family,
				spidx->dst.ss_family);
		return NULL;
	}
	switch (spidx->src.ss_family) {
	case AF_INET:
		prefixlen = sizeof(struct in_addr) << 3;
		break;
#ifdef INET6
	case AF_INET6:
		prefixlen = sizeof(struct in6_addr) << 3;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", spidx->src.ss_family);
		return NULL;
	}

	/* is it transport mode SA negotiation? */
	plog(LLV_DEBUG, LOCATION, NULL, "src1: %s\n",
		saddr2str(iph2->src));
	plog(LLV_DEBUG, LOCATION, NULL, "src2: %s\n",
		saddr2str((struct sockaddr *)&spidx->src));
	if (cmpsaddrwop(iph2->src, (struct sockaddr *)&spidx->src)
	 || spidx->prefs != prefixlen)
		return NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "dst1: %s\n",
		saddr2str(iph2->dst));
	plog(LLV_DEBUG, LOCATION, NULL, "dst2: %s\n",
		saddr2str((struct sockaddr *)&spidx->dst));
	if (cmpsaddrwop(iph2->dst, (struct sockaddr *)&spidx->dst)
	 || spidx->prefd != prefixlen)
		return NULL;

	plog(LLV_DEBUG, LOCATION, NULL, "looks to be transport mode\n");

	for (p = TAILQ_FIRST(&sptree); p; p = TAILQ_NEXT(p, chain)) {
		if (!cmpspidx_wild(spidx, &p->spidx))
			return p;
	}

	return NULL;
}
#endif

struct secpolicy *
getspbyspid(spid)
	u_int32_t spid;
{
	struct secpolicy *p;

	for (p = TAILQ_FIRST(&sptree); p; p = TAILQ_NEXT(p, chain)) {
		if (p->id == spid)
			return p;
	}

	return NULL;
}

/*
 * compare policyindex.
 * a: subject b: db
 * OUT:	0:	equal
 *	1:	not equal
 */
int
cmpspidx(a, b)
	struct policyindex *a, *b;
{
	plog(LLV_DEBUG, LOCATION, NULL, "sub:%p: %s\n", a, spidx2str(a));
	plog(LLV_DEBUG, LOCATION, NULL, "db :%p: %s\n", b, spidx2str(b));

	/* XXX don't check direction now, but it's to be checked carefully. */
	if (a->dir != b->dir
	 || a->prefs != b->prefs
	 || a->prefd != b->prefd
	 || a->ul_proto != b->ul_proto)
		return 1;

	if (cmpsaddr((struct sockaddr *)&a->src, (struct sockaddr *)&b->src))
		return 1;
	if (cmpsaddr((struct sockaddr *)&a->dst, (struct sockaddr *)&b->dst))
		return 1;

	return 0;
}

/*
 * compare policyindex, with wildcard address/protocol match.
 * a: subject b: db, can contain wildcard things.
 * OUT:	0:	equal
 *	1:	not equal
 */
int
cmpspidx_wild(a, b)
	struct policyindex *a, *b;
{
	struct sockaddr_storage sa1, sa2;

	plog(LLV_DEBUG, LOCATION, NULL, "sub:%p: %s\n", a, spidx2str(a));
	plog(LLV_DEBUG, LOCATION, NULL, "db: %p: %s\n", b, spidx2str(b));

	if (!(b->dir == IPSEC_DIR_ANY || a->dir == b->dir))
		return 1;

	if (!(a->ul_proto == IPSEC_PROTO_ANY ||
	      b->ul_proto == IPSEC_PROTO_ANY ||
	      a->ul_proto == b->ul_proto))
		return 1;

	if (a->src.ss_family != b->src.ss_family)
		return 1;
	if (a->dst.ss_family != b->dst.ss_family)
		return 1;

	/* compare src address */
	if (sizeof(sa1) < a->src.ss_len || sizeof(sa2) < b->src.ss_len) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unexpected error: "
			"src.ss_len:%d dst.ss_len:%d\n",
			a->src.ss_len, b->src.ss_len);
		return 1;
	}
	mask_sockaddr((struct sockaddr *)&sa1, (struct sockaddr *)&a->src,
		b->prefs);
	mask_sockaddr((struct sockaddr *)&sa2, (struct sockaddr *)&b->src,
		b->prefs);
	plog(LLV_DEBUG, LOCATION, NULL, "%p masked with /%d: %s\n",
		a, b->prefs, saddr2str((struct sockaddr *)&sa1));
	plog(LLV_DEBUG, LOCATION, NULL, "%p masked with /%d: %s\n",
		b, b->prefs, saddr2str((struct sockaddr *)&sa2));
	if (cmpsaddr((struct sockaddr *)&sa1, (struct sockaddr *)&sa2))
		return 1;

	/* compare dst address */
	if (sizeof(sa1) < a->dst.ss_len || sizeof(sa2) < b->dst.ss_len) {
		plog(LLV_ERROR, LOCATION, NULL, "unexpected error\n");
		exit(1);
	}
	mask_sockaddr((struct sockaddr *)&sa1, (struct sockaddr *)&a->dst,
		b->prefd);
	mask_sockaddr((struct sockaddr *)&sa2, (struct sockaddr *)&b->dst,
		b->prefd);
	plog(LLV_DEBUG, LOCATION, NULL, "%p masked with /%d: %s\n",
		a, b->prefd, saddr2str((struct sockaddr *)&sa1));
	plog(LLV_DEBUG, LOCATION, NULL, "%p masked with /%d: %s\n",
		b, b->prefd, saddr2str((struct sockaddr *)&sa2));
	if (cmpsaddr((struct sockaddr *)&sa1, (struct sockaddr *)&sa2))
		return 1;

	return 0;
}

struct secpolicy *
newsp()
{
	struct secpolicy *new;

	new = CALLOC(sizeof(*new), struct secpolicy *);
	if (new == NULL)
		return NULL;

	return new;
}

void
delsp(sp)
	struct secpolicy *sp;
{
	struct ipsecrequest *req = NULL, *next;

	for (req = sp->req; req; req = next) {
		next = req->next;
		free(req);
	}
	
	free(sp);
}

void
inssp(new)
	struct secpolicy *new;
{
	TAILQ_INSERT_TAIL(&sptree, new, chain);
}

void
remsp(sp)
	struct secpolicy *sp;
{
	TAILQ_REMOVE(&sptree, sp, chain);
}

void
flushsp()
{
	struct secpolicy *p, *next;

	for (p = TAILQ_FIRST(&sptree); p; p = next) {
		next = TAILQ_NEXT(p, chain);
		remsp(p);
		delsp(p);
	}
}

void
initsp()
{
	TAILQ_INIT(&sptree);
}

struct ipsecrequest *
newipsecreq()
{
	struct ipsecrequest *new;

	new = CALLOC(sizeof(*new), struct ipsecrequest *);
	if (new == NULL)
		return NULL;

	return new;
}

const char *
spidx2str(spidx)
	const struct policyindex *spidx;
{
	/* addr/pref[port] addr/pref[port] ul dir act */
	static char buf[256];
	char *p, *a, *b;
	int blen, i;

	blen = sizeof(buf) - 1;
	p = buf;

	a = saddr2str((struct sockaddr *)&spidx->src);
	for (b = a; *b != '\0'; b++)
		if (*b == '[') {
			*b = '\0';
			b++;
			break;
		}
	i = snprintf(p, blen, "%s/%d[%s ", a, spidx->prefs, b);
	p += i;
	blen -= i;

	a = saddr2str((struct sockaddr *)&spidx->dst);
	for (b = a; *b != '\0'; b++)
		if (*b == '[') {
			*b = '\0';
			b++;
			break;
		}
	i = snprintf(p, blen, "%s/%d[%s ", a, spidx->prefd, b);
	p += i;
	blen -= i;

	snprintf(p, blen, "proto=%d dir=%d",
		spidx->ul_proto, spidx->dir);

	return buf;
}
