/*	$NetBSD: proposal.c,v 1.17 2008/09/19 11:14:49 tteras Exp $	*/

/* $Id: proposal.c,v 1.17 2008/09/19 11:14:49 tteras Exp $ */

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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include PATH_IPSEC_H

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
#include "pfkey.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "ipsec_doi.h"
#include "algorithm.h"
#include "proposal.h"
#include "sainfo.h"
#include "localconf.h"
#include "remoteconf.h"
#include "oakley.h"
#include "handler.h"
#include "strnames.h"
#include "gcmalloc.h"
#ifdef ENABLE_NATT
#include "nattraversal.h"
#endif

static uint g_nextreqid = 1;

/* %%%
 * modules for ipsec sa spec
 */
struct saprop *
newsaprop()
{
	struct saprop *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	return new;
}

struct saproto *
newsaproto()
{
	struct saproto *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	return new;
}

/* set saprop to last part of the prop tree */
void
inssaprop(head, new)
	struct saprop **head;
	struct saprop *new;
{
	struct saprop *p;

	if (*head == NULL) {
		*head = new;
		return;
	}

	for (p = *head; p->next; p = p->next)
		;
	p->next = new;

	return;
}

/* set saproto to the end of the proto tree in saprop */
void
inssaproto(pp, new)
	struct saprop *pp;
	struct saproto *new;
{
	struct saproto *p;

	for (p = pp->head; p && p->next; p = p->next)
		;
	if (p == NULL)
		pp->head = new;
	else
		p->next = new;

	return;
}

/* set saproto to the top of the proto tree in saprop */
void
inssaprotorev(pp, new)
      struct saprop *pp;
      struct saproto *new;
{
      new->next = pp->head;
      pp->head = new;

      return;
}

struct satrns *
newsatrns()
{
	struct satrns *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL)
		return NULL;

	return new;
}

/* set saproto to last part of the proto tree in saprop */
void
inssatrns(pr, new)
	struct saproto *pr;
	struct satrns *new;
{
	struct satrns *tr;

	for (tr = pr->head; tr && tr->next; tr = tr->next)
		;
	if (tr == NULL)
		pr->head = new;
	else
		tr->next = new;

	return;
}

/*
 * take a single match between saprop.  allocate a new proposal and return it
 * for future use (like picking single proposal from a bundle).
 *	pp1: peer's proposal.
 *	pp2: my proposal.
 * NOTE: In the case of initiator, must be ensured that there is no
 * modification of the proposal by calling cmp_aproppair_i() before
 * this function.
 * XXX cannot understand the comment!
 */
struct saprop *
cmpsaprop_alloc(ph1, pp1, pp2, side)
	struct ph1handle *ph1;
	const struct saprop *pp1, *pp2;
	int side;
{
	struct saprop *newpp = NULL;
	struct saproto *pr1, *pr2, *newpr = NULL;
	struct satrns *tr1, *tr2, *newtr;
	const int ordermatters = 0;
	int npr1, npr2;
	int spisizematch;

	newpp = newsaprop();
	if (newpp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate saprop.\n");
		return NULL;
	}
	newpp->prop_no = pp1->prop_no;

	/* see proposal.h about lifetime/key length and PFS selection. */

	/* check time/bytes lifetime and PFS */
	switch (ph1->rmconf->pcheck_level) {
	case PROP_CHECK_OBEY:
		newpp->lifetime = pp1->lifetime;
		newpp->lifebyte = pp1->lifebyte;
		newpp->pfs_group = pp1->pfs_group;
		break;

	case PROP_CHECK_STRICT:
		if (pp1->lifetime > pp2->lifetime) {
			plog(LLV_ERROR, LOCATION, NULL,
				"long lifetime proposed: "
				"my:%d peer:%d\n",
				(int)pp2->lifetime, (int)pp1->lifetime);
			goto err;
		}
		if (pp1->lifebyte > pp2->lifebyte) {
			plog(LLV_ERROR, LOCATION, NULL,
				"long lifebyte proposed: "
				"my:%d peer:%d\n",
				pp2->lifebyte, pp1->lifebyte);
			goto err;
		}
		newpp->lifetime = pp1->lifetime;
		newpp->lifebyte = pp1->lifebyte;

    prop_pfs_check:
		if (pp2->pfs_group != 0 && pp1->pfs_group != pp2->pfs_group) {
			plog(LLV_ERROR, LOCATION, NULL,
				"pfs group mismatched: "
				"my:%d peer:%d\n",
				pp2->pfs_group, pp1->pfs_group);
			goto err;
		}
		newpp->pfs_group = pp1->pfs_group;
		break;

	case PROP_CHECK_CLAIM:
		/* lifetime */
		if (pp1->lifetime <= pp2->lifetime) {
			newpp->lifetime = pp1->lifetime;
		} else {
			newpp->lifetime = pp2->lifetime;
			newpp->claim |= IPSECDOI_ATTR_SA_LD_TYPE_SEC;
			plog(LLV_NOTIFY, LOCATION, NULL,
				"use own lifetime: "
				"my:%d peer:%d\n",
				(int)pp2->lifetime, (int)pp1->lifetime);
		}

		/* lifebyte */
		if (pp1->lifebyte > pp2->lifebyte) {
			newpp->lifebyte = pp2->lifebyte;
			newpp->claim |= IPSECDOI_ATTR_SA_LD_TYPE_SEC;
			plog(LLV_NOTIFY, LOCATION, NULL,
				"use own lifebyte: "
				"my:%d peer:%d\n",
				pp2->lifebyte, pp1->lifebyte);
		}
		newpp->lifebyte = pp1->lifebyte;

    		goto prop_pfs_check;
		break;

	case PROP_CHECK_EXACT:
		if (pp1->lifetime != pp2->lifetime) {
			plog(LLV_ERROR, LOCATION, NULL,
				"lifetime mismatched: "
				"my:%d peer:%d\n",
				(int)pp2->lifetime, (int)pp1->lifetime);
			goto err;
		}

		if (pp1->lifebyte != pp2->lifebyte) {
			plog(LLV_ERROR, LOCATION, NULL,
				"lifebyte mismatched: "
				"my:%d peer:%d\n",
				pp2->lifebyte, pp1->lifebyte);
			goto err;
		}
		if (pp1->pfs_group != pp2->pfs_group) {
			plog(LLV_ERROR, LOCATION, NULL,
				"pfs group mismatched: "
				"my:%d peer:%d\n",
				pp2->pfs_group, pp1->pfs_group);
			goto err;
		}
		newpp->lifetime = pp1->lifetime;
		newpp->lifebyte = pp1->lifebyte;
		newpp->pfs_group = pp1->pfs_group;
		break;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid pcheck_level why?.\n");
		goto err;
	}

#ifdef HAVE_SECCTX
	/* check the security_context properties.
	 * It is possible for one side to have a security context
	 * and the other side doesn't. If so, this is an error.
	 */

	if (*pp1->sctx.ctx_str && !(*pp2->sctx.ctx_str)) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "My proposal missing security context\n");
		goto err;
	}
	if (!(*pp1->sctx.ctx_str) && *pp2->sctx.ctx_str) {
		plog(LLV_ERROR, LOCATION, NULL, 
		     "Peer is missing security context\n");
		goto err;
	}

	if (*pp1->sctx.ctx_str && *pp2->sctx.ctx_str) {
		if (pp1->sctx.ctx_doi == pp2->sctx.ctx_doi)
			newpp->sctx.ctx_doi = pp1->sctx.ctx_doi;
		else {
			plog(LLV_ERROR, LOCATION, NULL, 
			     "sec doi mismatched: my:%d peer:%d\n",
			     pp2->sctx.ctx_doi, pp1->sctx.ctx_doi);
			     goto err;
		}

		if (pp1->sctx.ctx_alg == pp2->sctx.ctx_alg)
			newpp->sctx.ctx_alg = pp1->sctx.ctx_alg;
		else {
			plog(LLV_ERROR, LOCATION, NULL,
			     "sec alg mismatched: my:%d peer:%d\n",
			     pp2->sctx.ctx_alg, pp1->sctx.ctx_alg);
			goto err;
		}

		if ((pp1->sctx.ctx_strlen != pp2->sctx.ctx_strlen) ||
		     memcmp(pp1->sctx.ctx_str, pp2->sctx.ctx_str,
		     pp1->sctx.ctx_strlen) != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
			     "sec ctx string mismatched: my:%s peer:%s\n",
			     pp2->sctx.ctx_str, pp1->sctx.ctx_str);
				goto err;
		} else {
			newpp->sctx.ctx_strlen = pp1->sctx.ctx_strlen;
			memcpy(newpp->sctx.ctx_str, pp1->sctx.ctx_str,
				pp1->sctx.ctx_strlen);
		}
	}
#endif /* HAVE_SECCTX */

	npr1 = npr2 = 0;
	for (pr1 = pp1->head; pr1; pr1 = pr1->next)
		npr1++;
	for (pr2 = pp2->head; pr2; pr2 = pr2->next)
		npr2++;
	if (npr1 != npr2)
		goto err;

	/* check protocol order */
	pr1 = pp1->head;
	pr2 = pp2->head;

	while (1) {
		if (!ordermatters) {
			/*
			 * XXX does not work if we have multiple proposals
			 * with the same proto_id
			 */
			switch (side) {
			case RESPONDER:
				if (!pr2)
					break;
				for (pr1 = pp1->head; pr1; pr1 = pr1->next) {
					if (pr1->proto_id == pr2->proto_id)
						break;
				}
				break;
			case INITIATOR:
				if (!pr1)
					break;
				for (pr2 = pp2->head; pr2; pr2 = pr2->next) {
					if (pr2->proto_id == pr1->proto_id)
						break;
				}
				break;
			}
		}
		if (!pr1 || !pr2)
			break;

		if (pr1->proto_id != pr2->proto_id) {
			plog(LLV_ERROR, LOCATION, NULL,
				"proto_id mismatched: "
				"my:%s peer:%s\n",
				s_ipsecdoi_proto(pr2->proto_id),
				s_ipsecdoi_proto(pr1->proto_id));
			goto err;
		}
		spisizematch = 0;
		if (pr1->spisize == pr2->spisize)
			spisizematch = 1;
		else if (pr1->proto_id == IPSECDOI_PROTO_IPCOMP) {
			/*
			 * draft-shacham-ippcp-rfc2393bis-05.txt:
			 * need to accept 16bit and 32bit SPI (CPI) for IPComp.
			 */
			if (pr1->spisize == sizeof(u_int16_t) &&
			    pr2->spisize == sizeof(u_int32_t)) {
				spisizematch = 1;
			} else if (pr2->spisize == sizeof(u_int16_t) &&
				 pr1->spisize == sizeof(u_int32_t)) {
				spisizematch = 1;
			}
			if (spisizematch) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "IPComp SPI size promoted "
				    "from 16bit to 32bit\n");
			}
		}
		if (!spisizematch) {
			plog(LLV_ERROR, LOCATION, NULL,
				"spisize mismatched: "
				"my:%d peer:%d\n",
				(int)pr2->spisize, (int)pr1->spisize);
			goto err;
		}

#ifdef ENABLE_NATT
		if ((ph1->natt_flags & NAT_DETECTED) && 
		    natt_udp_encap (pr2->encmode))
		{
			plog(LLV_INFO, LOCATION, NULL, "Adjusting my encmode %s->%s\n",
			     s_ipsecdoi_encmode(pr2->encmode),
			     s_ipsecdoi_encmode(pr2->encmode - ph1->natt_options->mode_udp_diff));
			pr2->encmode -= ph1->natt_options->mode_udp_diff;
			pr2->udp_encap = 1;
		}

		if ((ph1->natt_flags & NAT_DETECTED) &&
		    natt_udp_encap (pr1->encmode))
		{
			plog(LLV_INFO, LOCATION, NULL, "Adjusting peer's encmode %s(%d)->%s(%d)\n",
			     s_ipsecdoi_encmode(pr1->encmode),
			     pr1->encmode,
			     s_ipsecdoi_encmode(pr1->encmode - ph1->natt_options->mode_udp_diff),
			     pr1->encmode - ph1->natt_options->mode_udp_diff);
			pr1->encmode -= ph1->natt_options->mode_udp_diff;
			pr1->udp_encap = 1;
		}
#endif

		if (pr1->encmode != pr2->encmode) {
			plog(LLV_ERROR, LOCATION, NULL,
				"encmode mismatched: "
				"my:%s peer:%s\n",
				s_ipsecdoi_encmode(pr2->encmode),
				s_ipsecdoi_encmode(pr1->encmode));
			goto err;
		}

		for (tr1 = pr1->head; tr1; tr1 = tr1->next) {
			for (tr2 = pr2->head; tr2; tr2 = tr2->next) {
				if (cmpsatrns(pr1->proto_id, tr1, tr2, ph1->rmconf->pcheck_level) == 0)
					goto found;
			}
		}

		goto err;

	    found:
		newpr = newsaproto();
		if (newpr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to allocate saproto.\n");
			goto err;
		}
		newpr->proto_id = pr1->proto_id;
		newpr->spisize = pr1->spisize;
		newpr->encmode = pr1->encmode;
		newpr->spi = pr2->spi;		/* copy my SPI */
		newpr->spi_p = pr1->spi;	/* copy peer's SPI */
		newpr->reqid_in = pr2->reqid_in;
		newpr->reqid_out = pr2->reqid_out;
#ifdef ENABLE_NATT
		newpr->udp_encap = pr1->udp_encap | pr2->udp_encap;
#endif

		newtr = newsatrns();
		if (newtr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to allocate satrns.\n");
			racoon_free(newpr);
			goto err;
		}
		newtr->trns_no = tr1->trns_no;
		newtr->trns_id = tr1->trns_id;
		newtr->encklen = tr1->encklen;
		newtr->authtype = tr1->authtype;

		inssatrns(newpr, newtr);
		inssaproto(newpp, newpr);

		pr1 = pr1->next;
		pr2 = pr2->next;
	}

	/* XXX should check if we have visited all items or not */
	if (!ordermatters) {
		switch (side) {
		case RESPONDER:
			if (!pr2)
				pr1 = NULL;
			break;
		case INITIATOR:
			if (!pr1)
				pr2 = NULL;
			break;
		}
	}

	/* should be matched all protocols in a proposal */
	if (pr1 != NULL || pr2 != NULL)
		goto err;

	return newpp;

err:
	flushsaprop(newpp);
	return NULL;
}

/* take a single match between saprop.  returns 0 if pp1 equals to pp2. */
int
cmpsaprop(pp1, pp2)
	const struct saprop *pp1, *pp2;
{
	if (pp1->pfs_group != pp2->pfs_group) {
		plog(LLV_WARNING, LOCATION, NULL,
			"pfs_group mismatch. mine:%d peer:%d\n",
			pp1->pfs_group, pp2->pfs_group);
		/* FALLTHRU */
	}

	if (pp1->lifetime > pp2->lifetime) {
		plog(LLV_WARNING, LOCATION, NULL,
			"less lifetime proposed. mine:%d peer:%d\n",
			(int)pp1->lifetime, (int)pp2->lifetime);
		/* FALLTHRU */
	}
	if (pp1->lifebyte > pp2->lifebyte) {
		plog(LLV_WARNING, LOCATION, NULL,
			"less lifebyte proposed. mine:%d peer:%d\n",
			pp1->lifebyte, pp2->lifebyte);
		/* FALLTHRU */
	}

	return 0;
}

/*
 * take a single match between satrns.  returns 0 if tr1 equals to tr2.
 * tr1: peer's satrns
 * tr2: my satrns
 */
int
cmpsatrns(proto_id, tr1, tr2, check_level)
	int proto_id;
	const struct satrns *tr1, *tr2;
	int check_level;
{
	if (tr1->trns_id != tr2->trns_id) {
		plog(LLV_WARNING, LOCATION, NULL,
			"trns_id mismatched: "
			"my:%s peer:%s\n",
			s_ipsecdoi_trns(proto_id, tr2->trns_id),
			s_ipsecdoi_trns(proto_id, tr1->trns_id));
		return 1;
	}

	if (tr1->authtype != tr2->authtype) {
		plog(LLV_WARNING, LOCATION, NULL,
			"authtype mismatched: "
			"my:%s peer:%s\n",
			s_ipsecdoi_attr_v(IPSECDOI_ATTR_AUTH, tr2->authtype),
			s_ipsecdoi_attr_v(IPSECDOI_ATTR_AUTH, tr1->authtype));
		return 1;
	}

	/* Check key length regarding checkmode
	 * XXX Shall we send some kind of notify message when key length rejected ?
	 */
	switch(check_level){
	case PROP_CHECK_OBEY:
		return 0;
		break;

	case PROP_CHECK_STRICT:
		/* FALLTHROUGH */
	case PROP_CHECK_CLAIM:
		if (tr1->encklen < tr2->encklen) {
		plog(LLV_WARNING, LOCATION, NULL,
				 "low key length proposed, "
				 "mine:%d peer:%d.\n",
			tr2->encklen, tr1->encklen);
			return 1;
		}
		break;
	case PROP_CHECK_EXACT:
		if (tr1->encklen != tr2->encklen) {
			plog(LLV_WARNING, LOCATION, NULL,
				 "key length mismatched, "
				 "mine:%d peer:%d.\n",
				 tr2->encklen, tr1->encklen);
			return 1;
		}
		break;
	}

	return 0;
}

int
set_satrnsbysainfo(pr, sainfo)
	struct saproto *pr;
	struct sainfo *sainfo;
{
	struct sainfoalg *a, *b;
	struct satrns *newtr;
	int t;

	switch (pr->proto_id) {
	case IPSECDOI_PROTO_IPSEC_AH:
		if (sainfo->algs[algclass_ipsec_auth] == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"no auth algorithm found\n");
			goto err;
		}
		t = 1;
		for (a = sainfo->algs[algclass_ipsec_auth]; a; a = a->next) {

			if (a->alg == IPSECDOI_ATTR_AUTH_NONE)
				continue;
				
			/* allocate satrns */
			newtr = newsatrns();
			if (newtr == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate satrns.\n");
				goto err;
			}

			newtr->trns_no = t++;
			newtr->trns_id = ipsecdoi_authalg2trnsid(a->alg);
			newtr->authtype = a->alg;

			inssatrns(pr, newtr);
		}
		break;
	case IPSECDOI_PROTO_IPSEC_ESP:
		if (sainfo->algs[algclass_ipsec_enc] == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"no encryption algorithm found\n");
			goto err;
		}
		t = 1;
		for (a = sainfo->algs[algclass_ipsec_enc]; a; a = a->next) {
			for (b = sainfo->algs[algclass_ipsec_auth]; b; b = b->next) {
				/* allocate satrns */
				newtr = newsatrns();
				if (newtr == NULL) {
					plog(LLV_ERROR, LOCATION, NULL,
						"failed to allocate satrns.\n");
					goto err;
				}

				newtr->trns_no = t++;
				newtr->trns_id = a->alg;
				newtr->encklen = a->encklen;
				newtr->authtype = b->alg;

				inssatrns(pr, newtr);
			}
		}
		break;
	case IPSECDOI_PROTO_IPCOMP:
		if (sainfo->algs[algclass_ipsec_comp] == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"no ipcomp algorithm found\n");
			goto err;
		}
		t = 1;
		for (a = sainfo->algs[algclass_ipsec_comp]; a; a = a->next) {

			/* allocate satrns */
			newtr = newsatrns();
			if (newtr == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate satrns.\n");
				goto err;
			}

			newtr->trns_no = t++;
			newtr->trns_id = a->alg;
			newtr->authtype = IPSECDOI_ATTR_AUTH_NONE; /*no auth*/

			inssatrns(pr, newtr);
		}
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"unknown proto_id (%d).\n", pr->proto_id);
		goto err;
	}

	/* no proposal found */
	if (pr->head == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "no algorithms found.\n");
		return -1;
	}

	return 0;

err:
	flushsatrns(pr->head);
	return -1;
}

struct saprop *
aproppair2saprop(p0)
	struct prop_pair *p0;
{
	struct prop_pair *p, *t;
	struct saprop *newpp;
	struct saproto *newpr;
	struct satrns *newtr;
	u_int8_t *spi;

	if (p0 == NULL)
		return NULL;

	/* allocate ipsec a sa proposal */
	newpp = newsaprop();
	if (newpp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate saprop.\n");
		return NULL;
	}
	newpp->prop_no = p0->prop->p_no;
	/* lifetime & lifebyte must be updated later */

	for (p = p0; p; p = p->next) {

		/* allocate ipsec sa protocol */
		newpr = newsaproto();
		if (newpr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to allocate saproto.\n");
			goto err;
		}

		/* check spi size */
		/* XXX should be handled isakmp cookie */
		if (sizeof(newpr->spi) < p->prop->spi_size) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid spi size %d.\n", p->prop->spi_size);
			racoon_free(newpr);
			goto err;
		}

		/*
		 * XXX SPI bits are left-filled, for use with IPComp.
		 * we should be switching to variable-length spi field...
		 */
		newpr->proto_id = p->prop->proto_id;
		newpr->spisize = p->prop->spi_size;
		memset(&newpr->spi, 0, sizeof(newpr->spi));
		spi = (u_int8_t *)&newpr->spi;
		spi += sizeof(newpr->spi);
		spi -= p->prop->spi_size;
		memcpy(spi, p->prop + 1, p->prop->spi_size);
		newpr->reqid_in = 0;
		newpr->reqid_out = 0;

		for (t = p; t; t = t->tnext) {

			plog(LLV_DEBUG, LOCATION, NULL,
				"prop#=%d prot-id=%s spi-size=%d "
				"#trns=%d trns#=%d trns-id=%s\n",
				t->prop->p_no,
				s_ipsecdoi_proto(t->prop->proto_id),
				t->prop->spi_size, t->prop->num_t,
				t->trns->t_no,
				s_ipsecdoi_trns(t->prop->proto_id,
				t->trns->t_id));

			/* allocate ipsec sa transform */
			newtr = newsatrns();
			if (newtr == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate satrns.\n");
				racoon_free(newpr);
				goto err;
			}

			if (ipsecdoi_t2satrns(t->trns, 
			    newpp, newpr, newtr) < 0) {
				flushsaprop(newpp);
				racoon_free(newtr);
				racoon_free(newpr);
				return NULL;
			}

			inssatrns(newpr, newtr);
		}

		/*
		 * If the peer does not specify encryption mode, use 
		 * transport mode by default.  This is to conform to
		 * draft-shacham-ippcp-rfc2393bis-08.txt (explicitly specifies
		 * that unspecified == transport), as well as RFC2407
		 * (unspecified == implementation dependent default).
		 */
		if (newpr->encmode == 0)
			newpr->encmode = IPSECDOI_ATTR_ENC_MODE_TRNS;

		inssaproto(newpp, newpr);
	}

	return newpp;

err:
	flushsaprop(newpp);
	return NULL;
}

void
flushsaprop(head)
	struct saprop *head;
{
	struct saprop *p, *save;

	for (p = head; p != NULL; p = save) {
		save = p->next;
		flushsaproto(p->head);
		racoon_free(p);
	}

	return;
}

void
flushsaproto(head)
	struct saproto *head;
{
	struct saproto *p, *save;

	for (p = head; p != NULL; p = save) {
		save = p->next;
		flushsatrns(p->head);
		vfree(p->keymat);
		vfree(p->keymat_p);
		racoon_free(p);
	}

	return;
}

void
flushsatrns(head)
	struct satrns *head;
{
	struct satrns *p, *save;

	for (p = head; p != NULL; p = save) {
		save = p->next;
		racoon_free(p);
	}

	return;
}

/*
 * print multiple proposals
 */
void
printsaprop(pri, pp)
	const int pri;
	const struct saprop *pp;
{
	const struct saprop *p;

	if (pp == NULL) {
		plog(pri, LOCATION, NULL, "(null)");
		return;
	}

	for (p = pp; p; p = p->next) {
		printsaprop0(pri, p);
	}

	return;
}

/*
 * print one proposal.
 */
void
printsaprop0(pri, pp)
	int pri;
	const struct saprop *pp;
{
	const struct saproto *p;

	if (pp == NULL)
		return;

	for (p = pp->head; p; p = p->next) {
		printsaproto(pri, p);
	}

	return;
}

void
printsaproto(pri, pr)
	const int pri;
	const struct saproto *pr;
{
	struct satrns *tr;

	if (pr == NULL)
		return;

	plog(pri, LOCATION, NULL,
		" (proto_id=%s spisize=%d spi=%08lx spi_p=%08lx "
		"encmode=%s reqid=%d:%d)\n",
		s_ipsecdoi_proto(pr->proto_id),
		(int)pr->spisize,
		(unsigned long)ntohl(pr->spi),
		(unsigned long)ntohl(pr->spi_p),
		s_ipsecdoi_attr_v(IPSECDOI_ATTR_ENC_MODE, pr->encmode),
		(int)pr->reqid_in, (int)pr->reqid_out);

	for (tr = pr->head; tr; tr = tr->next) {
		printsatrns(pri, pr->proto_id, tr);
	}

	return;
}

void
printsatrns(pri, proto_id, tr)
	const int pri;
	const int proto_id;
	const struct satrns *tr;
{
	if (tr == NULL)
		return;

	switch (proto_id) {
	case IPSECDOI_PROTO_IPSEC_AH:
		plog(pri, LOCATION, NULL,
			"  (trns_id=%s authtype=%s)\n",
			s_ipsecdoi_trns(proto_id, tr->trns_id),
			s_ipsecdoi_attr_v(IPSECDOI_ATTR_AUTH, tr->authtype));
		break;
	case IPSECDOI_PROTO_IPSEC_ESP:
		plog(pri, LOCATION, NULL,
			"  (trns_id=%s encklen=%d authtype=%s)\n",
			s_ipsecdoi_trns(proto_id, tr->trns_id),
			tr->encklen,
			s_ipsecdoi_attr_v(IPSECDOI_ATTR_AUTH, tr->authtype));
		break;
	case IPSECDOI_PROTO_IPCOMP:
		plog(pri, LOCATION, NULL,
			"  (trns_id=%s)\n",
			s_ipsecdoi_trns(proto_id, tr->trns_id));
		break;
	default:
		plog(pri, LOCATION, NULL,
			"(unknown proto_id %d)\n", proto_id);
	}

	return;
}

void
print_proppair0(pri, p, level)
	int pri; 
	struct prop_pair *p;
	int level;
{
	char spc[21];

	memset(spc, ' ', sizeof(spc));
	spc[sizeof(spc) - 1] = '\0';
	if (level < 20) {
		spc[level] = '\0';
	}

	plog(pri, LOCATION, NULL,
		"%s%p: next=%p tnext=%p\n", spc, p, p->next, p->tnext);
	if (p->next)
		print_proppair0(pri, p->next, level + 1);
	if (p->tnext)
		print_proppair0(pri, p->tnext, level + 1);
}

void
print_proppair(pri, p)
	int pri;
	struct prop_pair *p;
{
	print_proppair0(pri, p, 1);
}

int
set_proposal_from_policy(iph2, sp_main, sp_sub)
	struct ph2handle *iph2;
	struct secpolicy *sp_main, *sp_sub;
{
	struct saprop *newpp;
	struct ipsecrequest *req;
	int encmodesv = IPSECDOI_ATTR_ENC_MODE_TRNS; /* use only when complex_bundle */

	newpp = newsaprop();
	if (newpp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate saprop.\n");
		goto err;
	}
	newpp->prop_no = 1;
	newpp->lifetime = iph2->sainfo->lifetime;
	newpp->lifebyte = iph2->sainfo->lifebyte;
	newpp->pfs_group = iph2->sainfo->pfs_group;

	if (lcconf->complex_bundle)
		goto skip1;

	/*
	 * decide the encryption mode of this SA bundle.
	 * the mode becomes tunnel mode when there is even one policy
	 * of tunnel mode in the SPD.  otherwise the mode becomes
	 * transport mode.
	 */
	for (req = sp_main->req; req; req = req->next) {
		if (req->saidx.mode == IPSEC_MODE_TUNNEL) {
			encmodesv = pfkey2ipsecdoi_mode(req->saidx.mode);
#ifdef ENABLE_NATT
			if (iph2->ph1 && (iph2->ph1->natt_flags & NAT_DETECTED))
				encmodesv += iph2->ph1->natt_options->mode_udp_diff;
#endif
			break;
		}
	}

    skip1:
	for (req = sp_main->req; req; req = req->next) {
		struct saproto *newpr;
		caddr_t paddr = NULL;

		/*
		 * check if SA bundle ?
		 * nested SAs negotiation is NOT supported.
		 *       me +--- SA1 ---+ peer1
		 *       me +--- SA2 --------------+ peer2
		 */
#ifdef __linux__
		if (req->saidx.src.ss_family && req->saidx.dst.ss_family) {
#else
		if (req->saidx.src.ss_len && req->saidx.dst.ss_len) {
#endif
			/* check the end of ip addresses of SA */
			if (iph2->side == INITIATOR)
				paddr = (caddr_t)&req->saidx.dst;
			else
				paddr = (caddr_t)&req->saidx.src;
		}

		/* allocate ipsec sa protocol */
		newpr = newsaproto();
		if (newpr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to allocate saproto.\n");
			goto err;
		}

		newpr->proto_id = ipproto2doi(req->saidx.proto);
		if (newpr->proto_id == IPSECDOI_PROTO_IPCOMP)
			newpr->spisize = 2;
		else
			newpr->spisize = 4;
		if (lcconf->complex_bundle) {
			newpr->encmode = pfkey2ipsecdoi_mode(req->saidx.mode);
#ifdef ENABLE_NATT
			if (iph2->ph1 && (iph2->ph1->natt_flags & NAT_DETECTED))
				newpr->encmode += 
				    iph2->ph1->natt_options->mode_udp_diff;
#endif
		}
		else
			newpr->encmode = encmodesv;

		if (iph2->side == INITIATOR)
			newpr->reqid_out = req->saidx.reqid;
		else
			newpr->reqid_in = req->saidx.reqid;

		if (set_satrnsbysainfo(newpr, iph2->sainfo) < 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get algorithms.\n");
			racoon_free(newpr);
			goto err;
		}

		/* set new saproto */
		inssaprotorev(newpp, newpr);
	}

	/* get reqid_in from inbound policy */
	if (sp_sub) {
		struct saproto *pr;

		req = sp_sub->req;
		pr = newpp->head;
		while (req && pr) {
			if (iph2->side == INITIATOR)
				pr->reqid_in = req->saidx.reqid;
			else
				pr->reqid_out = req->saidx.reqid;
			pr = pr->next;
			req = req->next;
		}
		if (pr || req) {
			plog(LLV_NOTIFY, LOCATION, NULL,
				"There is a difference "
				"between the in/out bound policies in SPD.\n");
		}
	}

	iph2->proposal = newpp;

	printsaprop0(LLV_DEBUG, newpp);

	return 0;
err:
	flushsaprop(newpp);
	return -1;
}

/*
 * generate a policy from peer's proposal.
 * this function unconditionally choices first proposal in SA payload
 * passed by peer.
 */
int
set_proposal_from_proposal(iph2)
	struct ph2handle *iph2;
{
        struct saprop *newpp = NULL, *pp0, *pp_peer = NULL;
	struct saproto *newpr = NULL, *pr;
	struct prop_pair **pair;
	int error = -1;
	int i;

	/* get proposal pair */
	pair = get_proppair(iph2->sa, IPSECDOI_TYPE_PH2);
	if (pair == NULL)
		goto end;

	/*
	 * make my proposal according as the client proposal.
	 * XXX assumed there is only one proposal even if it's the SA bundle.
	 */
	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		if (pair[i] == NULL)
			continue;
		
		if (pp_peer != NULL)
			flushsaprop(pp_peer);

		pp_peer = aproppair2saprop(pair[i]);
		if (pp_peer == NULL)
			goto end;

		pp0 = newsaprop();
		if (pp0 == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to allocate saprop.\n");
			goto end;
		}
		pp0->prop_no = 1;
		pp0->lifetime = iph2->sainfo->lifetime;
		pp0->lifebyte = iph2->sainfo->lifebyte;
		pp0->pfs_group = iph2->sainfo->pfs_group;

#ifdef HAVE_SECCTX
		if (*pp_peer->sctx.ctx_str) {
			pp0->sctx.ctx_doi = pp_peer->sctx.ctx_doi;
			pp0->sctx.ctx_alg = pp_peer->sctx.ctx_alg;
			pp0->sctx.ctx_strlen = pp_peer->sctx.ctx_strlen;
			memcpy(pp0->sctx.ctx_str, pp_peer->sctx.ctx_str,
			       pp_peer->sctx.ctx_strlen);
		}
#endif /* HAVE_SECCTX */

		if (pp_peer->next != NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"pp_peer is inconsistency, ignore it.\n");
			/*FALLTHROUGH*/
		}

		for (pr = pp_peer->head; pr; pr = pr->next)
		{
			newpr = newsaproto();
			if (newpr == NULL)
			{
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate saproto.\n");
				racoon_free(pp0);
				goto end;
			}
			newpr->proto_id = pr->proto_id;
			newpr->spisize = pr->spisize;
			newpr->encmode = pr->encmode;
			newpr->spi = 0;
			newpr->spi_p = pr->spi;     /* copy peer's SPI */
			newpr->reqid_in = 0;
			newpr->reqid_out = 0;

			if (iph2->ph1->rmconf->gen_policy == GENERATE_POLICY_UNIQUE){
				newpr->reqid_in = g_nextreqid ;
				newpr->reqid_out = g_nextreqid ++;
				/* 
				 * XXX there is a (very limited) 
				 * risk of reusing the same reqid
				 * as another SP entry for the same peer
				 */
				if(g_nextreqid >= IPSEC_MANUAL_REQID_MAX)
					g_nextreqid = 1;
			}else{
				newpr->reqid_in = 0;
				newpr->reqid_out = 0;
			}
 
			if (set_satrnsbysainfo(newpr, iph2->sainfo) < 0)
			{
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to get algorithms.\n");
				racoon_free(newpr);
				racoon_free(pp0);
				goto end;
			}
			inssaproto(pp0, newpr);
		}

		inssaprop(&newpp, pp0);
        }

	plog(LLV_DEBUG, LOCATION, NULL, "make a proposal from peer's:\n");
	printsaprop0(LLV_DEBUG, newpp);  

	iph2->proposal = newpp;

	error = 0;

end:
	if (error && newpp)
		flushsaprop(newpp);

	if (pp_peer)
		flushsaprop(pp_peer);
	if (pair)
		free_proppair(pair);
	return error;
}
