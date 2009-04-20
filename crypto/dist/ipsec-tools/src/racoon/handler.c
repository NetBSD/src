/*	$NetBSD: handler.c,v 1.9.6.8 2009/04/20 13:25:27 tteras Exp $	*/

/* Id: handler.c,v 1.28 2006/05/26 12:17:29 manubsd Exp */

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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#ifdef ENABLE_HYBRID
#include <resolv.h>
#endif

#include "schedule.h"
#include "grabmyaddr.h"
#include "algorithm.h"
#include "crypto_openssl.h"
#include "policy.h"
#include "proposal.h"
#include "isakmp_var.h"
#include "evt.h"
#include "isakmp.h"
#ifdef ENABLE_HYBRID
#include "isakmp_xauth.h"  
#include "isakmp_cfg.h"
#endif
#include "isakmp_inf.h"
#include "oakley.h"
#include "remoteconf.h"
#include "localconf.h"
#include "handler.h"
#include "gcmalloc.h"
#include "nattraversal.h"

#include "sainfo.h"

#ifdef HAVE_GSSAPI
#include "gssapi.h"
#endif

static LIST_HEAD(_ph1tree_, ph1handle) ph1tree;
static LIST_HEAD(_ph2tree_, ph2handle) ph2tree;
static LIST_HEAD(_ctdtree_, contacted) ctdtree;
static LIST_HEAD(_rcptree_, recvdpkt) rcptree;

static void del_recvdpkt __P((struct recvdpkt *));
static void rem_recvdpkt __P((struct recvdpkt *));
static void sweep_recvdpkt __P((void *));

/*
 * functions about management of the isakmp status table
 */
/* %%% management phase 1 handler */
/*
 * search for isakmpsa handler with isakmp index.
 */

extern caddr_t val2str(const char *, size_t);

struct ph1handle *
getph1byindex(index)
	isakmp_index *index;
{
	struct ph1handle *p;

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->status == PHASE1ST_EXPIRED)
			continue;
		if (memcmp(&p->index, index, sizeof(*index)) == 0)
			return p;
	}

	return NULL;
}


/*
 * search for isakmp handler by i_ck in index.
 */
struct ph1handle *
getph1byindex0(index)
	isakmp_index *index;
{
	struct ph1handle *p;

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->status == PHASE1ST_EXPIRED)
			continue;
		if (memcmp(&p->index, index, sizeof(cookie_t)) == 0)
			return p;
	}

	return NULL;
}

/*
 * search for isakmpsa handler by source and remote address.
 * don't use port number to search because this function search
 * with phase 2's destinaion.
 */
struct ph1handle *
getph1byaddr(local, remote, established)
	struct sockaddr *local, *remote;
	int established;
{
	struct ph1handle *p;

	plog(LLV_DEBUG2, LOCATION, NULL, "getph1byaddr: start\n");
	plog(LLV_DEBUG2, LOCATION, NULL, "local: %s\n", saddr2str(local));
	plog(LLV_DEBUG2, LOCATION, NULL, "remote: %s\n", saddr2str(remote));

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->status == PHASE1ST_EXPIRED)
			continue;
		plog(LLV_DEBUG2, LOCATION, NULL, "p->local: %s\n", saddr2str(p->local));
		plog(LLV_DEBUG2, LOCATION, NULL, "p->remote: %s\n", saddr2str(p->remote));

		if(established && p->status != PHASE1ST_ESTABLISHED){
			plog(LLV_DEBUG2, LOCATION, NULL, "status %d, skipping\n", p->status);
			continue;
		}
		if (CMPSADDR(local, p->local) == 0
			&& CMPSADDR(remote, p->remote) == 0){
			plog(LLV_DEBUG2, LOCATION, NULL, "matched\n");
			return p;
		}
	}

	plog(LLV_DEBUG2, LOCATION, NULL, "no match\n");

	return NULL;
}

struct ph1handle *
getph1byaddrwop(local, remote)
	struct sockaddr *local, *remote;
{
	struct ph1handle *p;

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->status == PHASE1ST_EXPIRED)
			continue;
		if (cmpsaddrwop(local, p->local) == 0
		 && cmpsaddrwop(remote, p->remote) == 0)
			return p;
	}

	return NULL;
}

/*
 * search for isakmpsa handler by remote address.
 * don't use port number to search because this function search
 * with phase 2's destinaion.
 */
struct ph1handle *
getph1bydstaddrwop(remote)
	struct sockaddr *remote;
{
	struct ph1handle *p;

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->status == PHASE1ST_EXPIRED)
			continue;
		if (cmpsaddrwop(remote, p->remote) == 0)
			return p;
	}

	return NULL;
}

/*
 * dump isakmp-sa
 */
vchar_t *
dumpph1()
{
	struct ph1handle *iph1;
	struct ph1dump *pd;
	int cnt = 0;
	vchar_t *buf;

	/* get length of buffer */
	LIST_FOREACH(iph1, &ph1tree, chain)
		cnt++;

	buf = vmalloc(cnt * sizeof(struct ph1dump));
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer\n");
		return NULL;
	}
	pd = (struct ph1dump *)buf->v;

	LIST_FOREACH(iph1, &ph1tree, chain) {
		memcpy(&pd->index, &iph1->index, sizeof(iph1->index));
		pd->status = iph1->status;
		pd->side = iph1->side;
		memcpy(&pd->remote, iph1->remote, sysdep_sa_len(iph1->remote));
		memcpy(&pd->local, iph1->local, sysdep_sa_len(iph1->local));
		pd->version = iph1->version;
		pd->etype = iph1->etype;
		pd->created = iph1->created;
		pd->ph2cnt = iph1->ph2cnt;
		pd++;
	}

	return buf;
}

/*
 * create new isakmp Phase 1 status record to handle isakmp in Phase1
 */
struct ph1handle *
newph1()
{
	struct ph1handle *iph1;

	/* create new iph1 */
	iph1 = racoon_calloc(1, sizeof(*iph1));
	if (iph1 == NULL)
		return NULL;

	iph1->status = PHASE1ST_SPAWN;

#ifdef ENABLE_DPD
	iph1->dpd_support = 0;
	iph1->dpd_lastack = 0;
	iph1->dpd_seq = 0;
	iph1->dpd_fails = 0;
	iph1->dpd_r_u = NULL;
#endif

	return iph1;
}

/*
 * delete new isakmp Phase 1 status record to handle isakmp in Phase1
 */
void
delph1(iph1)
	struct ph1handle *iph1;
{
	if (iph1 == NULL)
		return;

	/* SA down shell script hook */
	script_hook(iph1, SCRIPT_PHASE1_DOWN);

	EVT_PUSH(iph1->local, iph1->remote, EVTT_PHASE1_DOWN, NULL);

#ifdef ENABLE_NATT
	if (iph1->natt_flags & NAT_KA_QUEUED)
		natt_keepalive_remove (iph1->local, iph1->remote);

	if (iph1->natt_options) {
		racoon_free(iph1->natt_options);
		iph1->natt_options = NULL;
	}
#endif

#ifdef ENABLE_HYBRID
	if (iph1->mode_cfg)
		isakmp_cfg_rmstate(iph1);
#endif

#ifdef ENABLE_DPD
	SCHED_KILL(iph1->dpd_r_u);
#endif

	if (iph1->remote) {
		racoon_free(iph1->remote);
		iph1->remote = NULL;
	}
	if (iph1->local) {
		racoon_free(iph1->local);
		iph1->local = NULL;
	}
	if (iph1->approval) {
		delisakmpsa(iph1->approval);
		iph1->approval = NULL;
	}

	VPTRINIT(iph1->authstr);

	sched_scrub_param(iph1);
	iph1->sce = NULL;
	iph1->scr = NULL;

	VPTRINIT(iph1->sendbuf);

	VPTRINIT(iph1->dhpriv);
	VPTRINIT(iph1->dhpub);
	VPTRINIT(iph1->dhpub_p);
	VPTRINIT(iph1->dhgxy);
	VPTRINIT(iph1->nonce);
	VPTRINIT(iph1->nonce_p);
	VPTRINIT(iph1->skeyid);
	VPTRINIT(iph1->skeyid_d);
	VPTRINIT(iph1->skeyid_a);
	VPTRINIT(iph1->skeyid_e);
	VPTRINIT(iph1->key);
	VPTRINIT(iph1->hash);
	VPTRINIT(iph1->sig);
	VPTRINIT(iph1->sig_p);
	oakley_delcert(iph1->cert);
	iph1->cert = NULL;
	oakley_delcert(iph1->cert_p);
	iph1->cert_p = NULL;
	oakley_delcert(iph1->crl_p);
	iph1->crl_p = NULL;
	oakley_delcert(iph1->cr_p);
	iph1->cr_p = NULL;
	VPTRINIT(iph1->id);
	VPTRINIT(iph1->id_p);

	if(iph1->approval != NULL)
		delisakmpsa(iph1->approval);

	if (iph1->ivm) {
		oakley_delivm(iph1->ivm);
		iph1->ivm = NULL;
	}

	VPTRINIT(iph1->sa);
	VPTRINIT(iph1->sa_ret);

#ifdef HAVE_GSSAPI
	VPTRINIT(iph1->gi_i);
	VPTRINIT(iph1->gi_r);

	gssapi_free_state(iph1);
#endif

	racoon_free(iph1);
}

/*
 * create new isakmp Phase 1 status record to handle isakmp in Phase1
 */
int
insph1(iph1)
	struct ph1handle *iph1;
{
	/* validity check */
	if (iph1->remote == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid isakmp SA handler. no remote address.\n");
		return -1;
	}
	LIST_INSERT_HEAD(&ph1tree, iph1, chain);

	return 0;
}

void
remph1(iph1)
	struct ph1handle *iph1;
{
	LIST_REMOVE(iph1, chain);
}

/*
 * flush isakmp-sa
 */
void
flushph1()
{
	struct ph1handle *p, *next;

	for (p = LIST_FIRST(&ph1tree); p; p = next) {
		next = LIST_NEXT(p, chain);

		/* send delete information */
		if (p->status == PHASE1ST_ESTABLISHED) 
			isakmp_info_send_d1(p);

		remph1(p);
		delph1(p);
	}
}

void
initph1tree()
{
	LIST_INIT(&ph1tree);
}

/* %%% management phase 2 handler */
/*
 * search ph2handle with policy id.
 */
struct ph2handle *
getph2byspid(spid)
      u_int32_t spid;
{
	struct ph2handle *p;

	LIST_FOREACH(p, &ph2tree, chain) {
		/*
		 * there are ph2handle independent on policy
		 * such like informational exchange.
		 */
		if (p->spid == spid)
			return p;
	}

	return NULL;
}

/*
 * search ph2handle with sequence number.
 */
struct ph2handle *
getph2byseq(seq)
	u_int32_t seq;
{
	struct ph2handle *p;

	LIST_FOREACH(p, &ph2tree, chain) {
		if (p->seq == seq)
			return p;
	}

	return NULL;
}

/*
 * search ph2handle with message id.
 */
struct ph2handle *
getph2bymsgid(iph1, msgid)
	struct ph1handle *iph1;
	u_int32_t msgid;
{
	struct ph2handle *p;

	LIST_FOREACH(p, &ph2tree, chain) {
		if (p->msgid == msgid && p->ph1 == iph1)
			return p;
	}

	return NULL;
}

struct ph2handle *
getph2byid(src, dst, spid)
	struct sockaddr *src, *dst;
	u_int32_t spid;
{
	struct ph2handle *p;

	LIST_FOREACH(p, &ph2tree, chain) {
		if (spid == p->spid &&
		    CMPSADDR(src, p->src) == 0 &&
		    CMPSADDR(dst, p->dst) == 0){
			/* Sanity check to detect zombie handlers
			 * XXX Sould be done "somewhere" more interesting,
			 * because we have lots of getph2byxxxx(), but this one
			 * is called by pk_recvacquire(), so is the most important.
			 */
			if(p->status < PHASE2ST_ESTABLISHED &&
			   p->retry_counter == 0
			   && p->sce == NULL && p->scr == NULL){
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Zombie ph2 found, expiring it\n");
				isakmp_ph2expire(p);
			}else
				return p;
		}
	}

	return NULL;
}

struct ph2handle *
getph2bysaddr(src, dst)
	struct sockaddr *src, *dst;
{
	struct ph2handle *p;

	LIST_FOREACH(p, &ph2tree, chain) {
		if (cmpsaddrstrict(src, p->src) == 0 &&
		    cmpsaddrstrict(dst, p->dst) == 0)
			return p;
	}

	return NULL;
}

/*
 * call by pk_recvexpire().
 */
struct ph2handle *
getph2bysaidx(src, dst, proto_id, spi)
	struct sockaddr *src, *dst;
	u_int proto_id;
	u_int32_t spi;
{
	struct ph2handle *iph2;
	struct saproto *pr;

	LIST_FOREACH(iph2, &ph2tree, chain) {
		if (iph2->proposal == NULL && iph2->approval == NULL)
			continue;
		if (iph2->approval != NULL) {
			for (pr = iph2->approval->head; pr != NULL;
			     pr = pr->next) {
				if (proto_id != pr->proto_id)
					break;
				if (spi == pr->spi || spi == pr->spi_p)
					return iph2;
			}
		} else if (iph2->proposal != NULL) {
			for (pr = iph2->proposal->head; pr != NULL;
			     pr = pr->next) {
				if (proto_id != pr->proto_id)
					break;
				if (spi == pr->spi)
					return iph2;
			}
		}
	}

	return NULL;
}

/*
 * create new isakmp Phase 2 status record to handle isakmp in Phase2
 */
struct ph2handle *
newph2()
{
	struct ph2handle *iph2 = NULL;

	/* create new iph2 */
	iph2 = racoon_calloc(1, sizeof(*iph2));
	if (iph2 == NULL)
		return NULL;

	iph2->status = PHASE1ST_SPAWN;

	return iph2;
}

/*
 * initialize ph2handle
 * NOTE: don't initialize src/dst.
 *       SPI in the proposal is cleared.
 */
void
initph2(iph2)
	struct ph2handle *iph2;
{
	sched_scrub_param(iph2);
	iph2->sce = NULL;
	iph2->scr = NULL;

	VPTRINIT(iph2->sendbuf);
	VPTRINIT(iph2->msg1);

	/* clear spi, keep variables in the proposal */
	if (iph2->proposal) {
		struct saproto *pr;
		for (pr = iph2->proposal->head; pr != NULL; pr = pr->next)
			pr->spi = 0;
	}

	/* clear approval */
	if (iph2->approval) {
		flushsaprop(iph2->approval);
		iph2->approval = NULL;
	}

	/* clear the generated policy */
	if (iph2->spidx_gen) {
		delsp_bothdir((struct policyindex *)iph2->spidx_gen);
		racoon_free(iph2->spidx_gen);
		iph2->spidx_gen = NULL;
	}

	if (iph2->pfsgrp) {
		oakley_dhgrp_free(iph2->pfsgrp);
		iph2->pfsgrp = NULL;
	}

	VPTRINIT(iph2->dhpriv);
	VPTRINIT(iph2->dhpub);
	VPTRINIT(iph2->dhpub_p);
	VPTRINIT(iph2->dhgxy);
	VPTRINIT(iph2->id);
	VPTRINIT(iph2->id_p);
	VPTRINIT(iph2->nonce);
	VPTRINIT(iph2->nonce_p);
	VPTRINIT(iph2->sa);
	VPTRINIT(iph2->sa_ret);

	if (iph2->ivm) {
		oakley_delivm(iph2->ivm);
		iph2->ivm = NULL;
	}
}

/*
 * delete new isakmp Phase 2 status record to handle isakmp in Phase2
 */
void
delph2(iph2)
	struct ph2handle *iph2;
{
	initph2(iph2);

	if (iph2->src) {
		racoon_free(iph2->src);
		iph2->src = NULL;
	}
	if (iph2->dst) {
		racoon_free(iph2->dst);
		iph2->dst = NULL;
	}
	if (iph2->src_id) {
	      racoon_free(iph2->src_id);
	      iph2->src_id = NULL;
	}
	if (iph2->dst_id) {
	      racoon_free(iph2->dst_id);
	      iph2->dst_id = NULL;
	}

	if (iph2->proposal) {
		flushsaprop(iph2->proposal);
		iph2->proposal = NULL;
	}

	racoon_free(iph2);
}

/*
 * create new isakmp Phase 2 status record to handle isakmp in Phase2
 */
int
insph2(iph2)
	struct ph2handle *iph2;
{
	LIST_INSERT_HEAD(&ph2tree, iph2, chain);

	return 0;
}

void
remph2(iph2)
	struct ph2handle *iph2;
{
	LIST_REMOVE(iph2, chain);
}

void
initph2tree()
{
	LIST_INIT(&ph2tree);
}

void
flushph2()
{
	struct ph2handle *p, *next;

	plog(LLV_DEBUG2, LOCATION, NULL,
		 "flushing all ph2 handlers...\n");

	for (p = LIST_FIRST(&ph2tree); p; p = next) {
		next = LIST_NEXT(p, chain);

		/* send delete information */
		if (p->status == PHASE2ST_ESTABLISHED){
			plog(LLV_DEBUG2, LOCATION, NULL,
				 "got a ph2 handler to flush...\n");
			isakmp_info_send_d2(p);
		}else{
			plog(LLV_DEBUG2, LOCATION, NULL,
				 "skipping ph2 handler (state %d)\n", p->status);
		}

		delete_spd(p, 0);
		unbindph12(p);
		remph2(p);
		delph2(p);
	}
}

/*
 * Delete all Phase 2 handlers for this src/dst/proto.  This
 * is used during INITIAL-CONTACT processing (so no need to
 * send a message to the peer).
 */
void
deleteallph2(src, dst, proto_id)
	struct sockaddr *src, *dst;
	u_int proto_id;
{
	struct ph2handle *iph2, *next;
	struct saproto *pr;

	for (iph2 = LIST_FIRST(&ph2tree); iph2 != NULL; iph2 = next) {
		next = LIST_NEXT(iph2, chain);
		if (iph2->proposal == NULL && iph2->approval == NULL)
			continue;
		if (iph2->approval != NULL) {
			for (pr = iph2->approval->head; pr != NULL;
			     pr = pr->next) {
				if (proto_id == pr->proto_id)
					goto zap_it;
			}
		} else if (iph2->proposal != NULL) {
			for (pr = iph2->proposal->head; pr != NULL;
			     pr = pr->next) {
				if (proto_id == pr->proto_id)
					goto zap_it;
			}
		}
		continue;
 zap_it:
		unbindph12(iph2);
		remph2(iph2);
		delph2(iph2);
	}
}

/* %%% */
void
bindph12(iph1, iph2)
	struct ph1handle *iph1;
	struct ph2handle *iph2;
{
	iph2->ph1 = iph1;
	LIST_INSERT_HEAD(&iph1->ph2tree, iph2, ph1bind);
}

void
unbindph12(iph2)
	struct ph2handle *iph2;
{
	if (iph2->ph1 != NULL) {
		iph2->ph1 = NULL;
		LIST_REMOVE(iph2, ph1bind);
	}
}

/* %%% management contacted list */
/*
 * search contacted list.
 */
struct contacted *
getcontacted(remote)
	struct sockaddr *remote;
{
	struct contacted *p;

	LIST_FOREACH(p, &ctdtree, chain) {
		if (cmpsaddrstrict(remote, p->remote) == 0)
			return p;
	}

	return NULL;
}

/*
 * create new isakmp Phase 2 status record to handle isakmp in Phase2
 */
int
inscontacted(remote)
	struct sockaddr *remote;
{
	struct contacted *new;

	/* create new iph2 */
	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return -1;

	new->remote = dupsaddr(remote);
	if (new->remote == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		racoon_free(new);
		return -1;
	}

	LIST_INSERT_HEAD(&ctdtree, new, chain);

	return 0;
}

void
initctdtree()
{
	LIST_INIT(&ctdtree);
}

/*
 * check the response has been sent to the peer.  when not, simply reply
 * the buffered packet to the peer.
 * OUT:
 *	 0:	the packet is received at the first time.
 *	 1:	the packet was processed before.
 *	 2:	the packet was processed before, but the address mismatches.
 *	-1:	error happened.
 */
int
check_recvdpkt(remote, local, rbuf)
	struct sockaddr *remote, *local;
	vchar_t *rbuf;
{
	vchar_t *hash;
	struct recvdpkt *r;
	time_t t;
	int len, s;

	/* set current time */
	t = time(NULL);

	hash = eay_md5_one(rbuf);
	if (!hash) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		return -1;
	}

	LIST_FOREACH(r, &rcptree, chain) {
		if (memcmp(hash->v, r->hash->v, r->hash->l) == 0)
			break;
	}
	vfree(hash);

	/* this is the first time to receive the packet */
	if (r == NULL)
		return 0;

	/*
	 * the packet was processed before, but the remote address mismatches.
	 */
	if (cmpsaddrstrict(remote, r->remote) != 0)
		return 2;

	/*
	 * it should not check the local address because the packet
	 * may arrive at other interface.
	 */

	/* check the previous time to send */
	if (t - r->time_send < 1) {
		plog(LLV_WARNING, LOCATION, NULL,
			"the packet retransmitted in a short time from %s\n",
			saddr2str(remote));
		/*XXX should it be error ? */
	}

	/* select the socket to be sent */
	s = getsockmyaddr(r->local);
	if (s == -1)
		return -1;

	/* resend the packet if needed */
	len = sendfromto(s, r->sendbuf->v, r->sendbuf->l,
			r->local, r->remote, lcconf->count_persend);
	if (len == -1) {
		plog(LLV_ERROR, LOCATION, NULL, "sendfromto failed\n");
		return -1;
	}

	/* check the retry counter */
	r->retry_counter--;
	if (r->retry_counter <= 0) {
		rem_recvdpkt(r);
		del_recvdpkt(r);
		plog(LLV_DEBUG, LOCATION, NULL,
			"deleted the retransmission packet to %s.\n",
			saddr2str(remote));
	} else
		r->time_send = t;

	return 1;
}

/*
 * adding a hash of received packet into the received list.
 */
int
add_recvdpkt(remote, local, sbuf, rbuf)
	struct sockaddr *remote, *local;
	vchar_t *sbuf, *rbuf;
{
	struct recvdpkt *new = NULL;

	if (lcconf->retry_counter == 0) {
		/* no need to add it */
		return 0;
	}

	new = racoon_calloc(1, sizeof(*new));
	if (!new) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		return -1;
	}

	new->hash = eay_md5_one(rbuf);
	if (!new->hash) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		del_recvdpkt(new);
		return -1;
	}
	new->remote = dupsaddr(remote);
	if (new->remote == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		del_recvdpkt(new);
		return -1;
	}
	new->local = dupsaddr(local);
	if (new->local == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		del_recvdpkt(new);
		return -1;
	}
	new->sendbuf = vdup(sbuf);
	if (new->sendbuf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		del_recvdpkt(new);
		return -1;
	}

	new->retry_counter = lcconf->retry_counter;
	new->time_send = 0;
	new->created = time(NULL);

	LIST_INSERT_HEAD(&rcptree, new, chain);

	return 0;
}

void
del_recvdpkt(r)
	struct recvdpkt *r;
{
	if (r->remote)
		racoon_free(r->remote);
	if (r->local)
		racoon_free(r->local);
	if (r->hash)
		vfree(r->hash);
	if (r->sendbuf)
		vfree(r->sendbuf);
	racoon_free(r);
}

void
rem_recvdpkt(r)
	struct recvdpkt *r;
{
	LIST_REMOVE(r, chain);
}

void
sweep_recvdpkt(dummy)
	void *dummy;
{
	struct recvdpkt *r, *next;
	time_t t, lt;

	/* set current time */
	t = time(NULL);

	/* set the lifetime of the retransmission */
	lt = lcconf->retry_counter * lcconf->retry_interval;

	for (r = LIST_FIRST(&rcptree); r; r = next) {
		next = LIST_NEXT(r, chain);

		if (t - r->created > lt) {
			rem_recvdpkt(r);
			del_recvdpkt(r);
		}
	}

	sched_new(lt, sweep_recvdpkt, NULL);
}

void
init_recvdpkt()
{
	time_t lt = lcconf->retry_counter * lcconf->retry_interval;

	LIST_INIT(&rcptree);

	sched_new(lt, sweep_recvdpkt, NULL);
}

#ifdef ENABLE_HYBRID
/* 
 * Retruns 0 if the address was obtained by ISAKMP mode config, 1 otherwise
 * This should be in isakmp_cfg.c but ph1tree being private, it must be there
 */
int
exclude_cfg_addr(addr)
	const struct sockaddr *addr;
{
	struct ph1handle *p;
	struct sockaddr_in *sin;

	LIST_FOREACH(p, &ph1tree, chain) {
		if ((p->mode_cfg != NULL) &&
		    (p->mode_cfg->flags & ISAKMP_CFG_GOT_ADDR4) &&
		    (addr->sa_family == AF_INET)) {
			sin = (struct sockaddr_in *)addr;
			if (sin->sin_addr.s_addr == p->mode_cfg->addr4.s_addr)
				return 0;
		}
	}

	return 1;
}
#endif



/* 
 * Reload conf code
 */
static int revalidate_ph2(struct ph2handle *iph2){
	struct sainfoalg *alg;
	int found, check_level;
	struct sainfo *sainfo;
	struct saprop *approval;
	struct ph1handle *iph1;

	/* 
	 * Get the new sainfo using values of the old one
	 */
	if (iph2->sainfo != NULL) {
		iph2->sainfo = getsainfo(iph2->sainfo->idsrc, 
					  iph2->sainfo->iddst, iph2->sainfo->id_i,
					  iph2->sainfo->remoteid);
	}
	approval = iph2->approval;
	sainfo = iph2->sainfo;

	if (sainfo == NULL) {
		/* 
		 * Sainfo has been removed
		 */
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: No sainfo for ph2\n");
		return 0;
	}

	if (approval == NULL) {
		/*
		 * XXX why do we have a NULL approval sometimes ???
		 */
		plog(LLV_DEBUG, LOCATION, NULL,
			 "No approval found !\n");
		return 0;
	}	

	/*
	 * Don't care about proposals, should we do something ?
	 * We have to keep iph2->proposal valid at least for initiator,
	 * for pk_sendgetspi()
	 */

	plog(LLV_DEBUG, LOCATION, NULL, "active single bundle:\n");
	printsaprop0(LLV_DEBUG, approval);

	/*
	 * Validate approval against sainfo
	 * Note: we must have an updated ph1->rmconf before doing that,
	 * we'll set check_level to EXACT if we don't have a ph1
	 * XXX try tu find the new remote section to get the new check level ?
	 * XXX lifebyte
	 */
	if (iph2->ph1 != NULL)
		iph1=iph2->ph1;
	else
		iph1=getph1byaddr(iph2->src, iph2->dst, 0);

	if(iph1 != NULL && iph1->rmconf != NULL) {
		check_level = iph1->rmconf->pcheck_level;
	} else {
		if(iph1 != NULL)
			plog(LLV_DEBUG, LOCATION, NULL, "No phase1 rmconf found !\n");
		else
			plog(LLV_DEBUG, LOCATION, NULL, "No phase1 found !\n");
		check_level = PROP_CHECK_EXACT;
	}

	switch (check_level) {
	case PROP_CHECK_OBEY:
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: OBEY for ph2, ok\n");
		return 1;
		break;

	case PROP_CHECK_STRICT:
		/* FALLTHROUGH */
	case PROP_CHECK_CLAIM:
		if (sainfo->lifetime < approval->lifetime) {
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: lifetime mismatch\n");
			return 0;
		}

#if 0
		/* Lifebyte is deprecated, just ignore it
		 */
		if (sainfo->lifebyte < approval->lifebyte) {
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: lifebyte mismatch\n");
			return 0;
		}
#endif

		if (sainfo->pfs_group &&
		   sainfo->pfs_group != approval->pfs_group) {
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: PFS group mismatch\n");
			return 0;
		}
		break;

	case PROP_CHECK_EXACT:
		if (sainfo->lifetime != approval->lifetime ||
#if 0
			/* Lifebyte is deprecated, just ignore it
			 */
		    sainfo->lifebyte != approval->lifebyte ||
#endif
		    sainfo->pfs_group != iph2->approval->pfs_group) {
			plog(LLV_DEBUG, LOCATION, NULL,
			    "Reload: lifetime | pfs mismatch\n");
			return 0;
		}
		break;

	default:
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: Shouldn't be here !\n");
		return 0;
		break;
	}

	for (alg = sainfo->algs[algclass_ipsec_auth]; alg; alg = alg->next) {
		if (alg->alg == approval->head->head->authtype)
			break;
	}
	if (alg == NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: alg == NULL (auth)\n");
		return 0;
	}

	found = 0;
	for (alg = sainfo->algs[algclass_ipsec_enc]; 
	    (found == 0 && alg != NULL); alg = alg->next) {
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: next ph2 enc alg...\n");

		if (alg->alg != approval->head->head->trns_id){
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: encmode mismatch (%d / %d)\n",
				 alg->alg, approval->head->head->trns_id);
			continue;
		}

		switch (check_level){
		/* PROP_CHECK_STRICT cannot happen here */
		case PROP_CHECK_EXACT:
			if (alg->encklen != approval->head->head->encklen) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: enclen mismatch\n");
				continue;
			}
			break;

		case PROP_CHECK_CLAIM:
			/* FALLTHROUGH */
		case PROP_CHECK_STRICT:
			if (alg->encklen > approval->head->head->encklen) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: enclen mismatch\n");
				continue;
			}
			break;

		default:
			plog(LLV_ERROR, LOCATION, NULL, 
			    "unexpected check_level\n");
			continue;
			break;
		}
		found = 1;
	}

	if (!found){
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: No valid enc\n");
		return 0;
	}

	/*
	 * XXX comp
	 */
	plog(LLV_DEBUG, LOCATION, NULL,
		 "Reload: ph2 check ok\n");

	return 1;
}


static void 
remove_ph2(struct ph2handle *iph2)
{
	u_int32_t spis[2];

	if(iph2 == NULL)
		return;

	plog(LLV_DEBUG, LOCATION, NULL,
		 "Deleting a Ph2...\n");

	if (iph2->status == PHASE2ST_ESTABLISHED)
		isakmp_info_send_d2(iph2);

	if(iph2->approval != NULL && iph2->approval->head != NULL){
		spis[0]=iph2->approval->head->spi;
		spis[1]=iph2->approval->head->spi_p;

		/* purge_ipsec_spi() will do all the work:
		 * - delete SPIs in kernel
		 * - delete generated SPD
		 * - unbind / rem / del ph2
		 */
		purge_ipsec_spi(iph2->dst, iph2->approval->head->proto_id,
						spis, 2);
	}else{
		unbindph12(iph2);
		remph2(iph2);
		delph2(iph2);
	}
}

static void remove_ph1(struct ph1handle *iph1){
	struct ph2handle *iph2, *iph2_next;

	if(iph1 == NULL)
		return;

	plog(LLV_DEBUG, LOCATION, NULL,
		 "Removing PH1...\n");

	if (iph1->status == PHASE1ST_ESTABLISHED){
		for (iph2 = LIST_FIRST(&iph1->ph2tree); iph2; iph2 = iph2_next) {
			iph2_next = LIST_NEXT(iph2, chain);
			remove_ph2(iph2);
		}
		isakmp_info_send_d1(iph1);
	}
	iph1->status = PHASE1ST_EXPIRED;
	iph1->sce = sched_new(1, isakmp_ph1delete_stub, iph1);
}


static int revalidate_ph1tree_rmconf(void){
	struct ph1handle *p, *next;
	struct remoteconf *newrmconf;

	for (p = LIST_FIRST(&ph1tree); p; p = next) {
		next = LIST_NEXT(p, chain);

		if (p->status == PHASE1ST_EXPIRED)
			continue;

		newrmconf=getrmconf(p->remote);
		if(newrmconf == NULL){
			p->rmconf = NULL;
			remove_ph1(p);
		}else{
			/* Do not free old rmconf, it is just a pointer to an entry in rmtree
			 */
			p->rmconf=newrmconf;
			if(p->approval != NULL){
				struct isakmpsa *tmpsa;

				tmpsa=dupisakmpsa(p->approval);
				if(tmpsa != NULL){
					delisakmpsa(p->approval);
					p->approval=tmpsa;
					p->approval->rmconf=newrmconf;
				}
			}
		}
	}

	return 1;
}


/* rmconf is already updated here
 */
static int revalidate_ph1(struct ph1handle *iph1){
	struct isakmpsa *p, *approval;
	struct etypes *e;

	if(iph1 == NULL ||
	   iph1->approval == NULL ||
		iph1->rmconf == NULL)
		return 0;

	approval=iph1->approval;

	for (e = iph1->rmconf->etypes; e != NULL; e = e->next){
		if (iph1->etype == e->type)
			break;
	}

	if (e == NULL){
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: Exchange type mismatch\n");
		return 0;
	}

	if (iph1->etype == ISAKMP_ETYPE_AGG &&
	   approval->dh_group != iph1->rmconf->dh_group){
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: DH mismatch\n");
		return 0;
	}

	for (p=iph1->rmconf->proposal; p != NULL; p=p->next){
		plog(LLV_DEBUG, LOCATION, NULL,
			 "Reload: Trying next proposal...\n");

		if(approval->authmethod != p->authmethod){
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: Authmethod mismatch\n");
			continue;
		}

		if(approval->enctype != p->enctype){
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: enctype mismatch\n");
			continue;
		}

		switch (iph1->rmconf->pcheck_level) {
		case PROP_CHECK_OBEY:
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: OBEY pcheck level, ok...\n");
			return 1;
			break;

		case PROP_CHECK_CLAIM:
			/* FALLTHROUGH */
		case PROP_CHECK_STRICT:
			if (approval->encklen < p->encklen) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: encklen mismatch\n");
				continue;
			}

			if (approval->lifetime > p->lifetime) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: lifetime mismatch\n");
				continue;
			}

#if 0
			/* Lifebyte is deprecated, just ignore it
			 */
			if (approval->lifebyte > p->lifebyte) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: lifebyte mismatch\n");
				continue;
			}
#endif
			break;

		case PROP_CHECK_EXACT:
			if (approval->encklen != p->encklen) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: encklen mismatch\n");
				continue;
			}

			if (approval->lifetime != p->lifetime) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: lifetime mismatch\n");
				continue;
			}

#if 0
			/* Lifebyte is deprecated, just ignore it
			 */
			if (approval->lifebyte != p->lifebyte) {
				plog(LLV_DEBUG, LOCATION, NULL,
					 "Reload: lifebyte mismatch\n");
				continue;
			}
#endif
			break;

		default:
			plog(LLV_ERROR, LOCATION, NULL, 
			    "unexpected check_level\n");
			continue;
			break;
		}

		if (approval->hashtype != p->hashtype) {
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: hashtype mismatch\n");
			continue;
		}

		if (iph1->etype != ISAKMP_ETYPE_AGG &&
		    approval->dh_group != p->dh_group) {
			plog(LLV_DEBUG, LOCATION, NULL,
				 "Reload: dhgroup mismatch\n");
			continue;
		}

		plog(LLV_DEBUG, LOCATION, NULL, "Reload: Conf ok\n");
		return 1;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "Reload: No valid conf found\n");
	return 0;
}


static int revalidate_ph1tree(void){
	struct ph1handle *p, *next;

	for (p = LIST_FIRST(&ph1tree); p; p = next) {
		next = LIST_NEXT(p, chain);

		if (p->status == PHASE1ST_EXPIRED)
			continue;

		if(!revalidate_ph1(p))
			remove_ph1(p);
	}

	return 1;
}

static int revalidate_ph2tree(void){
	struct ph2handle *p, *next;

	for (p = LIST_FIRST(&ph2tree); p; p = next) {
		next = LIST_NEXT(p, chain);

		if (p->status == PHASE2ST_EXPIRED)
			continue;

		if(!revalidate_ph2(p)){
			plog(LLV_DEBUG, LOCATION, NULL,
				 "PH2 not validated, removing it\n");
			remove_ph2(p);
		}
	}

	return 1;
}

int 
revalidate_ph12(void)
{

	revalidate_ph1tree_rmconf();

	revalidate_ph2tree();
	revalidate_ph1tree();

	return 1;
}

#ifdef ENABLE_HYBRID
struct ph1handle *
getph1bylogin(login)
	char *login;
{
	struct ph1handle *p;

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->mode_cfg == NULL)
			continue;
		if (strncmp(p->mode_cfg->login, login, LOGINLEN) == 0)
			return p;
	}

	return NULL;
}

int
purgeph1bylogin(login)
	char *login;
{
	struct ph1handle *p;
	int found = 0;

	LIST_FOREACH(p, &ph1tree, chain) {
		if (p->mode_cfg == NULL)
			continue;
		if (strncmp(p->mode_cfg->login, login, LOGINLEN) == 0) {
			if (p->status == PHASE1ST_ESTABLISHED)
				isakmp_info_send_d1(p);
			purge_remote(p);
			found++;
		}
	}

	return found;
}
#endif
