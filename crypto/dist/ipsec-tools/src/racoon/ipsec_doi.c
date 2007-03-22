/*	$NetBSD: ipsec_doi.c,v 1.23.4.4 2007/03/22 10:26:53 vanhu Exp $	*/

/* Id: ipsec_doi.c,v 1.55 2006/08/17 09:20:41 vanhu Exp */

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

#include <netinet/in.h>

#ifndef HAVE_NETINET6_IPSEC
#include <netinet/ipsec.h>
#else
#include <netinet6/ipsec.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "var.h"
#include "vmbuf.h"
#include "misc.h"
#include "plog.h"
#include "debug.h"

#include "cfparse_proto.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "ipsec_doi.h"
#include "oakley.h"
#include "remoteconf.h"
#include "localconf.h"
#include "sockmisc.h"
#include "handler.h"
#include "policy.h"
#include "algorithm.h"
#include "sainfo.h"
#include "proposal.h"
#include "crypto_openssl.h"
#include "strnames.h"
#include "gcmalloc.h"

#ifdef ENABLE_NATT
#include "nattraversal.h"
#endif
#ifdef ENABLE_HYBRID
static int switch_authmethod(int);
#endif

#ifdef HAVE_GSSAPI
#include <iconv.h>
#include "gssapi.h"
#ifdef HAVE_ICONV_2ND_CONST
#define __iconv_const const
#else
#define __iconv_const
#endif
#endif

int verbose_proposal_check = 1;

static vchar_t *get_ph1approval __P((struct ph1handle *, struct prop_pair **));
static struct isakmpsa *get_ph1approvalx __P((struct prop_pair *,
	struct isakmpsa *, struct isakmpsa *, int));
static void print_ph1mismatched __P((struct prop_pair *, struct isakmpsa *));
static int t2isakmpsa __P((struct isakmp_pl_t *, struct isakmpsa *));
static int cmp_aproppair_i __P((struct prop_pair *, struct prop_pair *));
static struct prop_pair *get_ph2approval __P((struct ph2handle *,
	struct prop_pair **));
static struct prop_pair *get_ph2approvalx __P((struct ph2handle *,
	struct prop_pair *));
static void free_proppair0 __P((struct prop_pair *));

static int get_transform
	__P((struct isakmp_pl_p *, struct prop_pair **, int *));
static u_int32_t ipsecdoi_set_ld __P((vchar_t *));

static int check_doi __P((u_int32_t));
static int check_situation __P((u_int32_t));

static int check_prot_main __P((int));
static int check_prot_quick __P((int));
static int (*check_protocol[]) __P((int)) = {
	check_prot_main,	/* IPSECDOI_TYPE_PH1 */
	check_prot_quick,	/* IPSECDOI_TYPE_PH2 */
};

static int check_spi_size __P((int, int));

static int check_trns_isakmp __P((int));
static int check_trns_ah __P((int));
static int check_trns_esp __P((int));
static int check_trns_ipcomp __P((int));
static int (*check_transform[]) __P((int)) = {
	0,
	check_trns_isakmp,	/* IPSECDOI_PROTO_ISAKMP */
	check_trns_ah,		/* IPSECDOI_PROTO_IPSEC_AH */
	check_trns_esp,		/* IPSECDOI_PROTO_IPSEC_ESP */
	check_trns_ipcomp,	/* IPSECDOI_PROTO_IPCOMP */
};

static int check_attr_isakmp __P((struct isakmp_pl_t *));
static int check_attr_ah __P((struct isakmp_pl_t *));
static int check_attr_esp __P((struct isakmp_pl_t *));
static int check_attr_ipsec __P((int, struct isakmp_pl_t *));
static int check_attr_ipcomp __P((struct isakmp_pl_t *));
static int (*check_attributes[]) __P((struct isakmp_pl_t *)) = {
	0,
	check_attr_isakmp,	/* IPSECDOI_PROTO_ISAKMP */
	check_attr_ah,		/* IPSECDOI_PROTO_IPSEC_AH */
	check_attr_esp,		/* IPSECDOI_PROTO_IPSEC_ESP */
	check_attr_ipcomp,	/* IPSECDOI_PROTO_IPCOMP */
};

static int setph1prop __P((struct isakmpsa *, caddr_t));
static int setph1trns __P((struct isakmpsa *, caddr_t));
static int setph1attr __P((struct isakmpsa *, caddr_t));
static vchar_t *setph2proposal0 __P((const struct ph2handle *,
	const struct saprop *, const struct saproto *));

static vchar_t *getidval __P((int, vchar_t *));

#ifdef HAVE_GSSAPI
static struct isakmpsa *fixup_initiator_sa __P((struct isakmpsa *,
	struct isakmpsa *));
#endif

/*%%%*/
/*
 * check phase 1 SA payload.
 * make new SA payload to be replyed not including general header.
 * the pointer to one of isakmpsa in proposal is set into iph1->approval.
 * OUT:
 *	positive: the pointer to new buffer of SA payload.
 *		  network byte order.
 *	NULL	: error occurd.
 */
int
ipsecdoi_checkph1proposal(sa, iph1)
	vchar_t *sa;
	struct ph1handle *iph1;
{
	vchar_t *newsa;		/* new SA payload approved. */
	struct prop_pair **pair;

	/* get proposal pair */
	pair = get_proppair(sa, IPSECDOI_TYPE_PH1);
	if (pair == NULL)
		return -1;

	/* check and get one SA for use */
	newsa = get_ph1approval(iph1, pair);
	
	free_proppair(pair);

	if (newsa == NULL)
		return -1;

	iph1->sa_ret = newsa;

	return 0;
}

/*
 * acceptable check for remote configuration.
 * return a new SA payload to be reply to peer.
 */
static vchar_t *
get_ph1approval(iph1, pair)
	struct ph1handle *iph1;
	struct prop_pair **pair;
{
	vchar_t *newsa;
	struct isakmpsa *sa, tsa;
	struct prop_pair *s, *p;
	int prophlen;
	int i;

	if (iph1->approval) {
		delisakmpsa(iph1->approval);
		iph1->approval = NULL;
	}

	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		if (pair[i] == NULL)
			continue;
		for (s = pair[i]; s; s = s->next) {
			prophlen = 
			    sizeof(struct isakmp_pl_p) + s->prop->spi_size;

			/* compare proposal and select one */
			for (p = s; p; p = p->tnext) {
				if ((sa = get_ph1approvalx(p, 
				    iph1->rmconf->proposal, &tsa, 
				    iph1->rmconf->pcheck_level)) != NULL)
					goto found;
			}
		}
	}

	/*
	 * if there is no suitable proposal, racoon complains about all of
	 * mismatched items in those proposal.
	 */
	if (verbose_proposal_check) {
		for (i = 0; i < MAXPROPPAIRLEN; i++) {
			if (pair[i] == NULL)
				continue;
			for (s = pair[i]; s; s = s->next) {
				prophlen = sizeof(struct isakmp_pl_p)
						+ s->prop->spi_size;
				for (p = s; p; p = p->tnext) {
					print_ph1mismatched(p,
						iph1->rmconf->proposal);
				}
			}
		}
	}
	plog(LLV_ERROR, LOCATION, NULL, "no suitable proposal found.\n");

	return NULL;

found:
	plog(LLV_DEBUG, LOCATION, NULL, "an acceptable proposal found.\n");

	/* check DH group settings */
	if (sa->dhgrp) {
		if (sa->dhgrp->prime && sa->dhgrp->gen1) {
			/* it's ok */
			goto saok;
		}
		plog(LLV_WARNING, LOCATION, NULL,
			"invalid DH parameter found, use default.\n");
		oakley_dhgrp_free(sa->dhgrp);
		sa->dhgrp=NULL;
	}

	if (oakley_setdhgroup(sa->dh_group, &sa->dhgrp) == -1) {
		sa->dhgrp = NULL;
		racoon_free(sa);
		return NULL;
	}

saok:
#ifdef HAVE_GSSAPI
	if (sa->gssid != NULL)
		plog(LLV_DEBUG, LOCATION, NULL, "gss id in new sa '%.*s'\n",
		    (int)sa->gssid->l, sa->gssid->v);
	if (iph1-> side == INITIATOR) {
		if (iph1->rmconf->proposal->gssid != NULL)
			iph1->gi_i = vdup(iph1->rmconf->proposal->gssid);
		if (tsa.gssid != NULL)
			iph1->gi_r = vdup(tsa.gssid);
		iph1->approval = fixup_initiator_sa(sa, &tsa);
	} else {
		if (tsa.gssid != NULL) {
			iph1->gi_r = vdup(tsa.gssid);
			iph1->gi_i = gssapi_get_id(iph1);
			if (sa->gssid == NULL && iph1->gi_i != NULL)
				sa->gssid = vdup(iph1->gi_i);
		}
		iph1->approval = sa;
	}
	if (iph1->gi_i != NULL)
		plog(LLV_DEBUG, LOCATION, NULL, "GIi is %.*s\n",
		    (int)iph1->gi_i->l, iph1->gi_i->v);
	if (iph1->gi_r != NULL)
		plog(LLV_DEBUG, LOCATION, NULL, "GIr is %.*s\n",
		    (int)iph1->gi_r->l, iph1->gi_r->v);
#else
	iph1->approval = sa;
#endif
	if(iph1->approval) {
		plog(LLV_DEBUG, LOCATION, NULL, "agreed on %s auth.\n",
		    s_oakley_attr_method(iph1->approval->authmethod));
	}

	newsa = get_sabyproppair(p, iph1);
	if (newsa == NULL){
		delisakmpsa(iph1->approval);
		iph1->approval = NULL;
	}

	return newsa;
}

/*
 * compare peer's single proposal and all of my proposal.
 * and select one if suiatable.
 * p       : one of peer's proposal.
 * proposal: my proposals.
 */
static struct isakmpsa *
get_ph1approvalx(p, proposal, sap, check_level)
	struct prop_pair *p;
	struct isakmpsa *proposal, *sap;
	int check_level;
{
	struct isakmp_pl_p *prop = p->prop;
	struct isakmp_pl_t *trns = p->trns;
	struct isakmpsa sa, *s, *tsap;
	int authmethod;

	plog(LLV_DEBUG, LOCATION, NULL,
       		"prop#=%d, prot-id=%s, spi-size=%d, #trns=%d\n",
		prop->p_no, s_ipsecdoi_proto(prop->proto_id),
		prop->spi_size, prop->num_t);

	plog(LLV_DEBUG, LOCATION, NULL,
		"trns#=%d, trns-id=%s\n",
		trns->t_no,
		s_ipsecdoi_trns(prop->proto_id, trns->t_id));

	tsap = sap != NULL ? sap : &sa;

	memset(tsap, 0, sizeof(*tsap));
	if (t2isakmpsa(trns, tsap) < 0)
		return NULL;
	for (s = proposal; s != NULL; s = s->next) {
#ifdef ENABLE_HYBRID
		authmethod = switch_authmethod(s->authmethod);
#else
		authmethod = s->authmethod;
#endif
		plog(LLV_DEBUG, LOCATION, NULL, "Compared: DB:Peer\n");
		plog(LLV_DEBUG, LOCATION, NULL, "(lifetime = %ld:%ld)\n",
			(long)s->lifetime, (long)tsap->lifetime);
		plog(LLV_DEBUG, LOCATION, NULL, "(lifebyte = %zu:%zu)\n",
			s->lifebyte, tsap->lifebyte);
		plog(LLV_DEBUG, LOCATION, NULL, "enctype = %s:%s\n",
			s_oakley_attr_v(OAKLEY_ATTR_ENC_ALG,
					s->enctype),
			s_oakley_attr_v(OAKLEY_ATTR_ENC_ALG,
					tsap->enctype));
		plog(LLV_DEBUG, LOCATION, NULL, "(encklen = %d:%d)\n",
			s->encklen, tsap->encklen);
		plog(LLV_DEBUG, LOCATION, NULL, "hashtype = %s:%s\n",
			s_oakley_attr_v(OAKLEY_ATTR_HASH_ALG,
					s->hashtype),
			s_oakley_attr_v(OAKLEY_ATTR_HASH_ALG,
					tsap->hashtype));
		plog(LLV_DEBUG, LOCATION, NULL, "authmethod = %s:%s\n",
			s_oakley_attr_v(OAKLEY_ATTR_AUTH_METHOD,
					s->authmethod),
			s_oakley_attr_v(OAKLEY_ATTR_AUTH_METHOD,
					tsap->authmethod));
		plog(LLV_DEBUG, LOCATION, NULL, "dh_group = %s:%s\n",
			s_oakley_attr_v(OAKLEY_ATTR_GRP_DESC,
					s->dh_group),
			s_oakley_attr_v(OAKLEY_ATTR_GRP_DESC,
					tsap->dh_group));
#if 0
		/* XXX to be considered ? */
		if (tsap->lifebyte > s->lifebyte) ;
#endif
		/*
		 * if responder side and peer's key length in proposal
		 * is bigger than mine, it might be accepted.
		 */
		if(tsap->enctype == s->enctype &&
		    tsap->authmethod == authmethod &&
		    tsap->hashtype == s->hashtype &&
		    tsap->dh_group == s->dh_group &&
		    tsap->encklen == s->encklen) {
			switch(check_level) {
			case PROP_CHECK_OBEY:
				goto found;
				break;

			case PROP_CHECK_STRICT:
				if ((tsap->lifetime > s->lifetime) ||
				    (tsap->lifebyte > s->lifebyte))
					continue;
				goto found;
				break;

			case PROP_CHECK_CLAIM:
				if (tsap->lifetime < s->lifetime)
					s->lifetime = tsap->lifetime;
				if (tsap->lifebyte < s->lifebyte)
					s->lifebyte = tsap->lifebyte;
				goto found;
				break;

			case PROP_CHECK_EXACT:
				if ((tsap->lifetime != s->lifetime) ||
				    (tsap->lifebyte != s->lifebyte))
					continue;
				goto found;
				break;

			default:
				plog(LLV_ERROR, LOCATION, NULL, 
				    "Unexpected proposal_check value\n");
				continue;
				break;
			}
		}
	}

found:
	if (tsap->dhgrp != NULL){
		oakley_dhgrp_free(tsap->dhgrp);
		tsap->dhgrp = NULL;
	}

	if ((s = dupisakmpsa(s)) != NULL) {
		switch(check_level) {
		case PROP_CHECK_OBEY:
			s->lifetime = tsap->lifetime;
			s->lifebyte = tsap->lifebyte;
			break;

		case PROP_CHECK_STRICT:
			s->lifetime = tsap->lifetime;
			s->lifebyte = tsap->lifebyte;
			break;

		case PROP_CHECK_CLAIM:
			if (tsap->lifetime < s->lifetime)
				s->lifetime = tsap->lifetime;
			if (tsap->lifebyte < s->lifebyte)
				s->lifebyte = tsap->lifebyte;
			break;

		default:
			break;
		}
	}
	return s;
}

/*
 * print all of items in peer's proposal which are mismatched to my proposal.
 * p       : one of peer's proposal.
 * proposal: my proposals.
 */
static void
print_ph1mismatched(p, proposal)
	struct prop_pair *p;
	struct isakmpsa *proposal;
{
	struct isakmpsa sa, *s;

	memset(&sa, 0, sizeof(sa));
	if (t2isakmpsa(p->trns, &sa) < 0)
		return;
	for (s = proposal; s ; s = s->next) {
		if (sa.enctype != s->enctype) {
			plog(LLV_ERROR, LOCATION, NULL,
				"rejected enctype: "
				"DB(prop#%d:trns#%d):Peer(prop#%d:trns#%d) = "
				"%s:%s\n",
				s->prop_no, s->trns_no,
				p->prop->p_no, p->trns->t_no,
				s_oakley_attr_v(OAKLEY_ATTR_ENC_ALG,
					s->enctype),
				s_oakley_attr_v(OAKLEY_ATTR_ENC_ALG,
					sa.enctype));
		}
		if (sa.authmethod != s->authmethod) {
			plog(LLV_ERROR, LOCATION, NULL,
				"rejected authmethod: "
				"DB(prop#%d:trns#%d):Peer(prop#%d:trns#%d) = "
				"%s:%s\n",
				s->prop_no, s->trns_no,
				p->prop->p_no, p->trns->t_no,
				s_oakley_attr_v(OAKLEY_ATTR_AUTH_METHOD,
					s->authmethod),
				s_oakley_attr_v(OAKLEY_ATTR_AUTH_METHOD,
					sa.authmethod));
		}
		if (sa.hashtype != s->hashtype) {
			plog(LLV_ERROR, LOCATION, NULL,
				"rejected hashtype: "
				"DB(prop#%d:trns#%d):Peer(prop#%d:trns#%d) = "
				"%s:%s\n",
				s->prop_no, s->trns_no,
				p->prop->p_no, p->trns->t_no,
				s_oakley_attr_v(OAKLEY_ATTR_HASH_ALG,
					s->hashtype),
				s_oakley_attr_v(OAKLEY_ATTR_HASH_ALG,
					sa.hashtype));
		}
		if (sa.dh_group != s->dh_group) {
			plog(LLV_ERROR, LOCATION, NULL,
				"rejected dh_group: "
				"DB(prop#%d:trns#%d):Peer(prop#%d:trns#%d) = "
				"%s:%s\n",
				s->prop_no, s->trns_no,
				p->prop->p_no, p->trns->t_no,
				s_oakley_attr_v(OAKLEY_ATTR_GRP_DESC,
					s->dh_group),
				s_oakley_attr_v(OAKLEY_ATTR_GRP_DESC,
					sa.dh_group));
		}
	}

	if (sa.dhgrp != NULL){
		oakley_dhgrp_free(sa.dhgrp);
		sa.dhgrp=NULL;
	}
}

/*
 * get ISAKMP data attributes
 */
static int
t2isakmpsa(trns, sa)
	struct isakmp_pl_t *trns;
	struct isakmpsa *sa;
{
	struct isakmp_data *d, *prev;
	int flag, type;
	int error = -1;
	int life_t;
	int keylen = 0;
	vchar_t *val = NULL;
	int len, tlen;
	u_char *p;

	tlen = ntohs(trns->h.len) - sizeof(*trns);
	prev = (struct isakmp_data *)NULL;
	d = (struct isakmp_data *)(trns + 1);

	/* default */
	life_t = OAKLEY_ATTR_SA_LD_TYPE_DEFAULT;
	sa->lifetime = OAKLEY_ATTR_SA_LD_SEC_DEFAULT;
	sa->lifebyte = 0;
	sa->dhgrp = racoon_calloc(1, sizeof(struct dhgroup));
	if (!sa->dhgrp)
		goto err;

	while (tlen > 0) {

		type = ntohs(d->type) & ~ISAKMP_GEN_MASK;
		flag = ntohs(d->type) & ISAKMP_GEN_MASK;

		plog(LLV_DEBUG, LOCATION, NULL,
			"type=%s, flag=0x%04x, lorv=%s\n",
			s_oakley_attr(type), flag,
			s_oakley_attr_v(type, ntohs(d->lorv)));

		/* get variable-sized item */
		switch (type) {
		case OAKLEY_ATTR_GRP_PI:
		case OAKLEY_ATTR_GRP_GEN_ONE:
		case OAKLEY_ATTR_GRP_GEN_TWO:
		case OAKLEY_ATTR_GRP_CURVE_A:
		case OAKLEY_ATTR_GRP_CURVE_B:
		case OAKLEY_ATTR_SA_LD:
		case OAKLEY_ATTR_GRP_ORDER:
			if (flag) {	/*TV*/
				len = 2;
				p = (u_char *)&d->lorv;
			} else {	/*TLV*/
				len = ntohs(d->lorv);
				p = (u_char *)(d + 1);
			}
			val = vmalloc(len);
			if (!val)
				return -1;
			memcpy(val->v, p, len);
			break;

		default:
			break;
		}

		switch (type) {
		case OAKLEY_ATTR_ENC_ALG:
			sa->enctype = (u_int16_t)ntohs(d->lorv);
			break;

		case OAKLEY_ATTR_HASH_ALG:
			sa->hashtype = (u_int16_t)ntohs(d->lorv);
			break;

		case OAKLEY_ATTR_AUTH_METHOD:
			sa->authmethod = ntohs(d->lorv);
			break;

		case OAKLEY_ATTR_GRP_DESC:
			sa->dh_group = (u_int16_t)ntohs(d->lorv);
			break;

		case OAKLEY_ATTR_GRP_TYPE:
		{
			int type = (int)ntohs(d->lorv);
			if (type == OAKLEY_ATTR_GRP_TYPE_MODP)
				sa->dhgrp->type = type;
			else
				return -1;
			break;
		}
		case OAKLEY_ATTR_GRP_PI:
			sa->dhgrp->prime = val;
			break;

		case OAKLEY_ATTR_GRP_GEN_ONE:
			vfree(val);
			if (!flag)
				sa->dhgrp->gen1 = ntohs(d->lorv);
			else {
				int len = ntohs(d->lorv);
				sa->dhgrp->gen1 = 0;
				if (len > 4)
					return -1;
				memcpy(&sa->dhgrp->gen1, d + 1, len);
				sa->dhgrp->gen1 = ntohl(sa->dhgrp->gen1);
			}
			break;

		case OAKLEY_ATTR_GRP_GEN_TWO:
			vfree(val);
			if (!flag)
				sa->dhgrp->gen2 = ntohs(d->lorv);
			else {
				int len = ntohs(d->lorv);
				sa->dhgrp->gen2 = 0;
				if (len > 4)
					return -1;
				memcpy(&sa->dhgrp->gen2, d + 1, len);
				sa->dhgrp->gen2 = ntohl(sa->dhgrp->gen2);
			}
			break;

		case OAKLEY_ATTR_GRP_CURVE_A:
			sa->dhgrp->curve_a = val;
			break;

		case OAKLEY_ATTR_GRP_CURVE_B:
			sa->dhgrp->curve_b = val;
			break;

		case OAKLEY_ATTR_SA_LD_TYPE:
		{
			int type = (int)ntohs(d->lorv);
			switch (type) {
			case OAKLEY_ATTR_SA_LD_TYPE_SEC:
			case OAKLEY_ATTR_SA_LD_TYPE_KB:
				life_t = type;
				break;
			default:
				life_t = OAKLEY_ATTR_SA_LD_TYPE_DEFAULT;
				break;
			}
			break;
		}
		case OAKLEY_ATTR_SA_LD:
			if (!prev
			 || (ntohs(prev->type) & ~ISAKMP_GEN_MASK) !=
					OAKLEY_ATTR_SA_LD_TYPE) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "life duration must follow ltype\n");
				break;
			}

			switch (life_t) {
			case IPSECDOI_ATTR_SA_LD_TYPE_SEC:
				sa->lifetime = ipsecdoi_set_ld(val);
				vfree(val);
				if (sa->lifetime == 0) {
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid life duration.\n");
					goto err;
				}
				break;
			case IPSECDOI_ATTR_SA_LD_TYPE_KB:
				sa->lifebyte = ipsecdoi_set_ld(val);
				vfree(val);
				if (sa->lifebyte == 0) {
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid life duration.\n");
					goto err;
				}
				break;
			default:
				vfree(val);
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid life type: %d\n", life_t);
				goto err;
			}
			break;

		case OAKLEY_ATTR_KEY_LEN:
		{
			int len = ntohs(d->lorv);
			if (len % 8 != 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"keylen %d: not multiple of 8\n",
					len);
				goto err;
			}
			sa->encklen = (u_int16_t)len;
			keylen++;
			break;
		}
		case OAKLEY_ATTR_PRF:
		case OAKLEY_ATTR_FIELD_SIZE:
			/* unsupported */
			break;

		case OAKLEY_ATTR_GRP_ORDER:
			sa->dhgrp->order = val;
			break;
#ifdef HAVE_GSSAPI
		case OAKLEY_ATTR_GSS_ID:
		{
			int error = -1;
			iconv_t cd = (iconv_t) -1;
			size_t srcleft, dstleft, rv;
			__iconv_const char *src;
			char *dst;
			int len = ntohs(d->lorv);

			/*
			 * Older verions of racoon just placed the
			 * ISO-Latin-1 string on the wire directly.
			 * Check to see if we are configured to be
			 * compatible with this behavior.
			 */
			if (lcconf->gss_id_enc == LC_GSSENC_LATIN1) {
				if ((sa->gssid = vmalloc(len)) == NULL) {
					plog(LLV_ERROR, LOCATION, NULL,
					    "failed to allocate memory\n");
					goto out;
				}
				memcpy(sa->gssid->v, d + 1, len);
				plog(LLV_DEBUG, LOCATION, NULL,
				    "received old-style gss "
				    "id '%.*s' (len %zu)\n",
				    (int)sa->gssid->l, sa->gssid->v, 
				    sa->gssid->l);
				error = 0;
				goto out;
			}

			/*
			 * For Windows 2000 compatibility, we expect
			 * the GSS ID attribute on the wire to be
			 * encoded in UTF-16LE.  Internally, we work
			 * in ISO-Latin-1.  Therefore, we should need
			 * 1/2 the specified length, which should always
			 * be a multiple of 2 octets.
			 */
			cd = iconv_open("latin1", "utf-16le");
			if (cd == (iconv_t) -1) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "unable to initialize utf-16le -> latin1 "
				    "conversion descriptor: %s\n",
				    strerror(errno));
				goto out;
			}

			if ((sa->gssid = vmalloc(len / 2)) == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "failed to allocate memory\n");
				goto out;
			}

			src = (__iconv_const char *)(d + 1);
			srcleft = len;

			dst = sa->gssid->v;
			dstleft = len / 2;

			rv = iconv(cd, (__iconv_const char **)&src, &srcleft, 
				   &dst, &dstleft);
			if (rv != 0) {
				if (rv == -1) {
					plog(LLV_ERROR, LOCATION, NULL,
					    "unable to convert GSS ID from "
					    "utf-16le -> latin1: %s\n",
					    strerror(errno));
				} else {
					plog(LLV_ERROR, LOCATION, NULL,
					    "%zd character%s in GSS ID cannot "
					    "be represented in latin1\n",
					    rv, rv == 1 ? "" : "s");
				}
				goto out;
			}

			/* XXX dstleft should always be 0; assert it? */
			sa->gssid->l = (len / 2) - dstleft;

			plog(LLV_DEBUG, LOCATION, NULL,
			    "received gss id '%.*s' (len %zu)\n",
			    (int)sa->gssid->l, sa->gssid->v, sa->gssid->l);

			error = 0;
out:
			if (cd != (iconv_t)-1)
				(void)iconv_close(cd);

			if ((error != 0) && (sa->gssid != NULL)) {
				vfree(sa->gssid);
				sa->gssid = NULL;
			}
			break;
		}
#endif /* HAVE_GSSAPI */

		default:
			break;
		}

		prev = d;
		if (flag) {
			tlen -= sizeof(*d);
			d = (struct isakmp_data *)((char *)d + sizeof(*d));
		} else {
			tlen -= (sizeof(*d) + ntohs(d->lorv));
			d = (struct isakmp_data *)((char *)d + sizeof(*d) + ntohs(d->lorv));
		}
	}

	/* key length must not be specified on some algorithms */
	if (keylen) {
		if (sa->enctype == OAKLEY_ATTR_ENC_ALG_DES
#ifdef HAVE_OPENSSL_IDEA_H
		 || sa->enctype == OAKLEY_ATTR_ENC_ALG_IDEA
#endif
		 || sa->enctype == OAKLEY_ATTR_ENC_ALG_3DES) {
			plog(LLV_ERROR, LOCATION, NULL,
				"keylen must not be specified "
				"for encryption algorithm %d\n",
				sa->enctype);
			return -1;
		}
	}

	return 0;
err:
	return error;
}

/*%%%*/
/*
 * check phase 2 SA payload and select single proposal.
 * make new SA payload to be replyed not including general header.
 * This function is called by responder only.
 * OUT:
 *	0: succeed.
 *	-1: error occured.
 */
int
ipsecdoi_selectph2proposal(iph2)
	struct ph2handle *iph2;
{
	struct prop_pair **pair;
	struct prop_pair *ret;

	/* get proposal pair */
	pair = get_proppair(iph2->sa, IPSECDOI_TYPE_PH2);
	if (pair == NULL)
		return -1;

	/* check and select a proposal. */
	ret = get_ph2approval(iph2, pair);
	free_proppair(pair);
	if (ret == NULL)
		return -1;

	/* make a SA to be replayed. */
	/* SPI must be updated later. */
	iph2->sa_ret = get_sabyproppair(ret, iph2->ph1);
	free_proppair0(ret);
	if (iph2->sa_ret == NULL)
		return -1;

	return 0;
}

/*
 * check phase 2 SA payload returned from responder.
 * This function is called by initiator only.
 * OUT:
 *	0: valid.
 *	-1: invalid.
 */
int
ipsecdoi_checkph2proposal(iph2)
	struct ph2handle *iph2;
{
	struct prop_pair **rpair = NULL, **spair = NULL;
	struct prop_pair *p;
	int i, n, num;
	int error = -1;
	vchar_t *sa_ret = NULL;

	/* get proposal pair of SA sent. */
	spair = get_proppair(iph2->sa, IPSECDOI_TYPE_PH2);
	if (spair == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get prop pair.\n");
		goto end;
	}

	/* XXX should check the number of transform */

	/* get proposal pair of SA replayed */
	rpair = get_proppair(iph2->sa_ret, IPSECDOI_TYPE_PH2);
	if (rpair == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get prop pair.\n");
		goto end;
	}

	/* check proposal is only one ? */
	n = 0;
	num = 0;
	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		if (rpair[i]) {
			n = i;
			num++;
		}
	}
	if (num == 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no proposal received.\n");
		goto end;
	}
	if (num != 1) {
		plog(LLV_ERROR, LOCATION, NULL,
			"some proposals received.\n");
		goto end;
	}

	if (spair[n] == NULL) {
		plog(LLV_WARNING, LOCATION, NULL,
			"invalid proposal number:%d received.\n", i);
	}
	

	if (rpair[n]->tnext != NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"multi transforms replyed.\n");
		goto end;
	}

	if (cmp_aproppair_i(rpair[n], spair[n])) {
		plog(LLV_ERROR, LOCATION, NULL,
			"proposal mismathed.\n");
		goto end;
	}

	/*
	 * check and select a proposal.
	 * ensure that there is no modification of the proposal by
	 * cmp_aproppair_i()
	 */
	p = get_ph2approval(iph2, rpair);
	if (p == NULL)
		goto end;

	/* make a SA to be replayed. */
	sa_ret = iph2->sa_ret;
	iph2->sa_ret = get_sabyproppair(p, iph2->ph1);
	free_proppair0(p);
	if (iph2->sa_ret == NULL)
		goto end;

	error = 0;

end:
	if (rpair)
		free_proppair(rpair);
	if (spair)
		free_proppair(spair);
	if (sa_ret)
		vfree(sa_ret);

	return error;
}

/*
 * compare two prop_pair which is assumed to have same proposal number.
 * the case of bundle or single SA, NOT multi transforms.
 * a: a proposal that is multi protocols and single transform, usually replyed.
 * b: a proposal that is multi protocols and multi transform, usually sent.
 * NOTE: this function is for initiator.
 * OUT
 *	0: equal
 *	1: not equal
 * XXX cannot understand the comment!
 */
static int
cmp_aproppair_i(a, b)
	struct prop_pair *a, *b;
{
	struct prop_pair *p, *q, *r;
	int len;

	for (p = a, q = b; p && q; p = p->next, q = q->next) {
		for (r = q; r; r = r->tnext) {
			/* compare trns */
			if (p->trns->t_no == r->trns->t_no)
				break;
		}
		if (!r) {
			/* no suitable transform found */
			plog(LLV_ERROR, LOCATION, NULL,
				"no suitable transform found.\n");
			return -1;
		}

		/* compare prop */
		if (p->prop->p_no != r->prop->p_no) {
			plog(LLV_WARNING, LOCATION, NULL,
				"proposal #%d mismatched, "
				"expected #%d.\n",
				r->prop->p_no, p->prop->p_no);
			/*FALLTHROUGH*/
		}

		if (p->prop->proto_id != r->prop->proto_id) {
			plog(LLV_ERROR, LOCATION, NULL,
				"proto_id mismathed: my:%d peer:%d\n",
				r->prop->proto_id, p->prop->proto_id);
			return -1;
		}

		if (p->prop->proto_id != r->prop->proto_id) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid spi size: %d.\n",
				p->prop->proto_id);
			return -1;
		}

		/* check #of transforms */
		if (p->prop->num_t != 1) {
			plog(LLV_WARNING, LOCATION, NULL,
				"#of transform is %d, "
				"but expected 1.\n", p->prop->num_t);
			/*FALLTHROUGH*/
		}

		if (p->trns->t_id != r->trns->t_id) {
			plog(LLV_WARNING, LOCATION, NULL,
				"transform number has been modified.\n");
			/*FALLTHROUGH*/
		}
		if (p->trns->reserved != r->trns->reserved) {
			plog(LLV_WARNING, LOCATION, NULL,
				"reserved field should be zero.\n");
			/*FALLTHROUGH*/
		}

		/* compare attribute */
		len = ntohs(r->trns->h.len) - sizeof(*p->trns);
		if (memcmp(p->trns + 1, r->trns + 1, len) != 0) {
			plog(LLV_WARNING, LOCATION, NULL,
				"attribute has been modified.\n");
			/*FALLTHROUGH*/
		}
	}
	if ((p && !q) || (!p && q)) {
		/* # of protocols mismatched */
		plog(LLV_ERROR, LOCATION, NULL,
			"#of protocols mismatched.\n");
		return -1;
	}

	return 0;
}

/*
 * acceptable check for policy configuration.
 * return a new SA payload to be reply to peer.
 */
static struct prop_pair *
get_ph2approval(iph2, pair)
	struct ph2handle *iph2;
	struct prop_pair **pair;
{
	struct prop_pair *ret;
	int i;

	iph2->approval = NULL;

	plog(LLV_DEBUG, LOCATION, NULL,
		"begin compare proposals.\n");

	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		if (pair[i] == NULL)
			continue;
		plog(LLV_DEBUG, LOCATION, NULL,
			"pair[%d]: %p\n", i, pair[i]);
		print_proppair(LLV_DEBUG, pair[i]);;

		/* compare proposal and select one */
		ret = get_ph2approvalx(iph2, pair[i]);
		if (ret != NULL) {
			/* found */
			return ret;
		}
	}

	plog(LLV_ERROR, LOCATION, NULL, "no suitable policy found.\n");

	return NULL;
}

/*
 * compare my proposal and peers just one proposal.
 * set a approval.
 */
static struct prop_pair *
get_ph2approvalx(iph2, pp)
	struct ph2handle *iph2;
	struct prop_pair *pp;
{
	struct prop_pair *ret = NULL;
	struct saprop *pr0, *pr = NULL;
	struct saprop *q1, *q2;

	pr0 = aproppair2saprop(pp);
	if (pr0 == NULL)
		return NULL;

	for (q1 = pr0; q1; q1 = q1->next) {
		for (q2 = iph2->proposal; q2; q2 = q2->next) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"peer's single bundle:\n");
			printsaprop0(LLV_DEBUG, q1);
			plog(LLV_DEBUG, LOCATION, NULL,
				"my single bundle:\n");
			printsaprop0(LLV_DEBUG, q2);

			pr = cmpsaprop_alloc(iph2->ph1, q1, q2, iph2->side);
			if (pr != NULL)
				goto found;

			plog(LLV_ERROR, LOCATION, NULL,
				"not matched\n");
		}
	}
	/* no proposal matching */
err:
	flushsaprop(pr0);
	return NULL;

found:
	flushsaprop(pr0);
	plog(LLV_DEBUG, LOCATION, NULL, "matched\n");
	iph2->approval = pr;

    {
	struct saproto *sp;
	struct prop_pair *p, *x;
	struct prop_pair *n = NULL;

	ret = NULL;

	for (p = pp; p; p = p->next) {
		/*
		 * find a proposal with matching proto_id.
		 * we have analyzed validity already, in cmpsaprop_alloc().
		 */
		for (sp = pr->head; sp; sp = sp->next) {
			if (sp->proto_id == p->prop->proto_id)
				break;
		}
		if (!sp)
			goto err;
		if (sp->head->next)
			goto err;	/* XXX */

		for (x = p; x; x = x->tnext)
			if (sp->head->trns_no == x->trns->t_no)
				break;
		if (!x)
			goto err;	/* XXX */

		n = racoon_calloc(1, sizeof(struct prop_pair));
		if (n == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer.\n");
			goto err;
		}

		n->prop = x->prop;
		n->trns = x->trns;

		/* need to preserve the order */
		for (x = ret; x && x->next; x = x->next)
			;
		if (x && x->prop == n->prop) {
			for (/*nothing*/; x && x->tnext; x = x->tnext)
				;
			x->tnext = n;
		} else {
			if (x)
				x->next = n;
			else {
				ret = n;
			}
		}

		/* #of transforms should be updated ? */
	}
    }

	return ret;
}

void
free_proppair(pair)
	struct prop_pair **pair;
{
	int i;

	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		free_proppair0(pair[i]);
		pair[i] = NULL;
	}
	racoon_free(pair);
}

static void
free_proppair0(pair)
	struct prop_pair *pair;
{
	struct prop_pair *p, *q, *r, *s;

	p = pair;
	while (p) {
		q = p->next;
		r = p;
		while (r) {
			s = r->tnext;
			racoon_free(r);
			r = s;
		}
		p = q;
	}
}

/*
 * get proposal pairs from SA payload.
 * tiny check for proposal payload.
 */
struct prop_pair **
get_proppair(sa, mode)
	vchar_t *sa;
	int mode;
{
	struct prop_pair **pair = NULL;
	int num_p = 0;			/* number of proposal for use */
	int tlen;
	caddr_t bp;
	int i;
	struct ipsecdoi_sa_b *sab = (struct ipsecdoi_sa_b *)sa->v;

	plog(LLV_DEBUG, LOCATION, NULL, "total SA len=%zu\n", sa->l);
	plogdump(LLV_DEBUG, sa->v, sa->l);

	/* check SA payload size */
	if (sa->l < sizeof(*sab)) {
		plog(LLV_ERROR, LOCATION, NULL,
			"Invalid SA length = %zu.\n", sa->l);
		goto bad;
	}

	/* check DOI */
	if (check_doi(ntohl(sab->doi)) < 0)
		goto bad;

	/* check SITUATION */
	if (check_situation(ntohl(sab->sit)) < 0)
		goto bad;

	pair = racoon_calloc(1, MAXPROPPAIRLEN * sizeof(*pair));
	if (pair == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer.\n");
		goto bad;
	}
	memset(pair, 0, sizeof(pair));

	bp = (caddr_t)(sab + 1);
	tlen = sa->l - sizeof(*sab);

    {
	struct isakmp_pl_p *prop;
	int proplen;
	vchar_t *pbuf = NULL;
	struct isakmp_parse_t *pa;

	pbuf = isakmp_parsewoh(ISAKMP_NPTYPE_P, (struct isakmp_gen *)bp, tlen);
	if (pbuf == NULL)
		goto bad;

	for (pa = (struct isakmp_parse_t *)pbuf->v;
	     pa->type != ISAKMP_NPTYPE_NONE;
	     pa++) {
		/* check the value of next payload */
		if (pa->type != ISAKMP_NPTYPE_P) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Invalid payload type=%u\n", pa->type);
			vfree(pbuf);
			goto bad;
		}

		prop = (struct isakmp_pl_p *)pa->ptr;
		proplen = pa->len;

		plog(LLV_DEBUG, LOCATION, NULL,
			"proposal #%u len=%d\n", prop->p_no, proplen);

		if (proplen == 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid proposal with length %d\n", proplen);
			vfree(pbuf);
			goto bad;
		}

		/* check Protocol ID */
		if (!check_protocol[mode]) {
			plog(LLV_ERROR, LOCATION, NULL,
				"unsupported mode %d\n", mode);
			continue;
		}

		if (check_protocol[mode](prop->proto_id) < 0)
			continue;

		/* check SPI length when IKE. */
		if (check_spi_size(prop->proto_id, prop->spi_size) < 0)
			continue;

		/* get transform */
		if (get_transform(prop, pair, &num_p) < 0) {
			vfree(pbuf);
			goto bad;
		}
	}
	vfree(pbuf);
	pbuf = NULL;
    }

    {
	int notrans, nprop;
	struct prop_pair *p, *q;

	/* check for proposals with no transforms */
	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		if (!pair[i])
			continue;

		plog(LLV_DEBUG, LOCATION, NULL, "pair %d:\n", i);
		print_proppair(LLV_DEBUG, pair[i]);

		notrans = nprop = 0;
		for (p = pair[i]; p; p = p->next) {
			if (p->trns == NULL) {
				notrans++;
				break;
			}
			for (q = p; q; q = q->tnext)
				nprop++;
		}

#if 0
		/*
		 * XXX at this moment, we cannot accept proposal group
		 * with multiple proposals.  this should be fixed.
		 */
		if (pair[i]->next) {
			plog(LLV_WARNING, LOCATION, NULL,
				"proposal #%u ignored "
				"(multiple proposal not supported)\n",
				pair[i]->prop->p_no);
			notrans++;
		}
#endif

		if (notrans) {
			for (p = pair[i]; p; p = q) {
				q = p->next;
				racoon_free(p);
			}
			pair[i] = NULL;
			num_p--;
		} else {
			plog(LLV_DEBUG, LOCATION, NULL,
				"proposal #%u: %d transform\n",
				pair[i]->prop->p_no, nprop);
		}
	}
    }

	/* bark if no proposal is found. */
	if (num_p <= 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no Proposal found.\n");
		goto bad;
	}

	return pair;
bad:
	if (pair != NULL)
		racoon_free(pair);
	return NULL;
}

/*
 * check transform payload.
 * OUT:
 *	positive: return the pointer to the payload of valid transform.
 *	0	: No valid transform found.
 */
static int
get_transform(prop, pair, num_p)
	struct isakmp_pl_p *prop;
	struct prop_pair **pair;
	int *num_p;
{
	int tlen; /* total length of all transform in a proposal */
	caddr_t bp;
	struct isakmp_pl_t *trns;
	int trnslen;
	vchar_t *pbuf = NULL;
	struct isakmp_parse_t *pa;
	struct prop_pair *p = NULL, *q;
	int num_t;

	bp = (caddr_t)prop + sizeof(struct isakmp_pl_p) + prop->spi_size;
	tlen = ntohs(prop->h.len)
		- (sizeof(struct isakmp_pl_p) + prop->spi_size);
	pbuf = isakmp_parsewoh(ISAKMP_NPTYPE_T, (struct isakmp_gen *)bp, tlen);
	if (pbuf == NULL)
		return -1;

	/* check and get transform for use */
	num_t = 0;
	for (pa = (struct isakmp_parse_t *)pbuf->v;
	     pa->type != ISAKMP_NPTYPE_NONE;
	     pa++) {

		num_t++;

		/* check the value of next payload */
		if (pa->type != ISAKMP_NPTYPE_T) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Invalid payload type=%u\n", pa->type);
			break;
		}

		trns = (struct isakmp_pl_t *)pa->ptr;
		trnslen = pa->len;

		plog(LLV_DEBUG, LOCATION, NULL,
			"transform #%u len=%u\n", trns->t_no, trnslen);

		/* check transform ID */
		if (prop->proto_id >= ARRAYLEN(check_transform)) {
			plog(LLV_WARNING, LOCATION, NULL,
				"unsupported proto_id %u\n",
				prop->proto_id);
			continue;
		}
		if (prop->proto_id >= ARRAYLEN(check_attributes)) {
			plog(LLV_WARNING, LOCATION, NULL,
				"unsupported proto_id %u\n",
				prop->proto_id);
			continue;
		}

		if (!check_transform[prop->proto_id]
		 || !check_attributes[prop->proto_id]) {
			plog(LLV_WARNING, LOCATION, NULL,
				"unsupported proto_id %u\n",
				prop->proto_id);
			continue;
		}
		if (check_transform[prop->proto_id](trns->t_id) < 0)
			continue;

		/* check data attributes */
		if (check_attributes[prop->proto_id](trns) != 0)
			continue;

		p = racoon_calloc(1, sizeof(*p));
		if (p == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer.\n");
			vfree(pbuf);
			return -1;
		}
		p->prop = prop;
		p->trns = trns;

		/* need to preserve the order */
		for (q = pair[prop->p_no]; q && q->next; q = q->next)
			;
		if (q && q->prop == p->prop) {
			for (/*nothing*/; q && q->tnext; q = q->tnext)
				;
			q->tnext = p;
		} else {
			if (q)
				q->next = p;
			else {
				pair[prop->p_no] = p;
				(*num_p)++;
			}
		}
	}

	vfree(pbuf);

	return 0;
}

/*
 * make a new SA payload from prop_pair.
 * NOTE: this function make spi value clear.
 */
vchar_t *
get_sabyproppair(pair, iph1)
	struct prop_pair *pair;
	struct ph1handle *iph1;
{
	vchar_t *newsa;
	int newtlen;
	u_int8_t *np_p = NULL;
	struct prop_pair *p;
	int prophlen, trnslen;
	caddr_t bp;

	newtlen = sizeof(struct ipsecdoi_sa_b);
	for (p = pair; p; p = p->next) {
		newtlen += sizeof(struct isakmp_pl_p);
		newtlen += p->prop->spi_size;
		newtlen += ntohs(p->trns->h.len);
	}

	newsa = vmalloc(newtlen);
	if (newsa == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "failed to get newsa.\n");
		return NULL;
	}
	bp = newsa->v;

	((struct isakmp_gen *)bp)->len = htons(newtlen);

	/* update some of values in SA header */
	((struct ipsecdoi_sa_b *)bp)->doi = htonl(iph1->rmconf->doitype);
	((struct ipsecdoi_sa_b *)bp)->sit = htonl(iph1->rmconf->sittype);
	bp += sizeof(struct ipsecdoi_sa_b);

	/* create proposal payloads */
	for (p = pair; p; p = p->next) {
		prophlen = sizeof(struct isakmp_pl_p)
				+ p->prop->spi_size;
		trnslen = ntohs(p->trns->h.len);

		if (np_p)
			*np_p = ISAKMP_NPTYPE_P;

		/* create proposal */

		memcpy(bp, p->prop, prophlen);
		((struct isakmp_pl_p *)bp)->h.np = ISAKMP_NPTYPE_NONE;
		((struct isakmp_pl_p *)bp)->h.len = htons(prophlen + trnslen);
		((struct isakmp_pl_p *)bp)->num_t = 1;
		np_p = &((struct isakmp_pl_p *)bp)->h.np;
		memset(bp + sizeof(struct isakmp_pl_p), 0, p->prop->spi_size);
		bp += prophlen;

		/* create transform */
		memcpy(bp, p->trns, trnslen);
		((struct isakmp_pl_t *)bp)->h.np = ISAKMP_NPTYPE_NONE;
		((struct isakmp_pl_t *)bp)->h.len = htons(trnslen);
		bp += trnslen;
	}

	return newsa;
}

/*
 * update responder's spi
 */
int
ipsecdoi_updatespi(iph2)
	struct ph2handle *iph2;
{
	struct prop_pair **pair, *p;
	struct saprop *pp;
	struct saproto *pr;
	int i;
	int error = -1;
	u_int8_t *spi;

	pair = get_proppair(iph2->sa_ret, IPSECDOI_TYPE_PH2);
	if (pair == NULL)
		return -1;
	for (i = 0; i < MAXPROPPAIRLEN; i++) {
		if (pair[i])
			break;
	}
	if (i == MAXPROPPAIRLEN || pair[i]->tnext) {
		/* multiple transform must be filtered by selectph2proposal.*/
		goto end;
	}

	pp = iph2->approval;

	/* create proposal payloads */
	for (p = pair[i]; p; p = p->next) {
		/*
		 * find a proposal/transform with matching proto_id/t_id.
		 * we have analyzed validity already, in cmpsaprop_alloc().
		 */
		for (pr = pp->head; pr; pr = pr->next) {
			if (p->prop->proto_id == pr->proto_id &&
			    p->trns->t_id == pr->head->trns_id) {
				break;
			}
		}
		if (!pr)
			goto end;

		/*
		 * XXX SPI bits are left-filled, for use with IPComp.
		 * we should be switching to variable-length spi field...
		 */
		spi = (u_int8_t *)&pr->spi;
		spi += sizeof(pr->spi);
		spi -= pr->spisize;
		memcpy((caddr_t)p->prop + sizeof(*p->prop), spi, pr->spisize);
	}

	error = 0;
end:
	free_proppair(pair);
	return error;
}

/*
 * make a new SA payload from prop_pair.
 */
vchar_t *
get_sabysaprop(pp0, sa0)
	struct saprop *pp0;
	vchar_t *sa0;
{
	struct prop_pair **pair = NULL;
	vchar_t *newsa = NULL;
	int newtlen;
	u_int8_t *np_p = NULL;
	struct prop_pair *p = NULL;
	struct saprop *pp;
	struct saproto *pr;
	struct satrns *tr;
	int prophlen, trnslen;
	caddr_t bp;
	int error = -1;

	/* get proposal pair */
	pair = get_proppair(sa0, IPSECDOI_TYPE_PH2);
	if (pair == NULL)
		goto out;

	newtlen = sizeof(struct ipsecdoi_sa_b);
	for (pp = pp0; pp; pp = pp->next) {

		if (pair[pp->prop_no] == NULL)
			goto out;

		for (pr = pp->head; pr; pr = pr->next) {
			newtlen += (sizeof(struct isakmp_pl_p)
				+ pr->spisize);

			for (tr = pr->head; tr; tr = tr->next) {
				for (p = pair[pp->prop_no]; p; p = p->tnext) {
					if (tr->trns_no == p->trns->t_no)
						break;
				}
				if (p == NULL)
					goto out;

				newtlen += ntohs(p->trns->h.len);
			}
		}
	}

	newsa = vmalloc(newtlen);
	if (newsa == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, "failed to get newsa.\n");
		goto out;
	}
	bp = newsa->v;

	/* some of values of SA must be updated in the out of this function */
	((struct isakmp_gen *)bp)->len = htons(newtlen);
	bp += sizeof(struct ipsecdoi_sa_b);

	/* create proposal payloads */
	for (pp = pp0; pp; pp = pp->next) {

		for (pr = pp->head; pr; pr = pr->next) {
			prophlen = sizeof(struct isakmp_pl_p)
					+ p->prop->spi_size;

			for (tr = pr->head; tr; tr = tr->next) {
				for (p = pair[pp->prop_no]; p; p = p->tnext) {
					if (tr->trns_no == p->trns->t_no)
						break;
				}
				if (p == NULL)
					goto out;

				trnslen = ntohs(p->trns->h.len);

				if (np_p)
					*np_p = ISAKMP_NPTYPE_P;

				/* create proposal */

				memcpy(bp, p->prop, prophlen);
				((struct isakmp_pl_p *)bp)->h.np = ISAKMP_NPTYPE_NONE;
				((struct isakmp_pl_p *)bp)->h.len = htons(prophlen + trnslen);
				((struct isakmp_pl_p *)bp)->num_t = 1;
				np_p = &((struct isakmp_pl_p *)bp)->h.np;
				bp += prophlen;

				/* create transform */
				memcpy(bp, p->trns, trnslen);
				((struct isakmp_pl_t *)bp)->h.np = ISAKMP_NPTYPE_NONE;
				((struct isakmp_pl_t *)bp)->h.len = htons(trnslen);
				bp += trnslen;
			}
		}
	}

	error = 0;
out:
	if (pair != NULL)
		racoon_free(pair);

	if (error != 0) {
		if (newsa != NULL) {
			vfree(newsa);
			newsa = NULL;
		}
	}

	return newsa;
}

/*
 * If some error happens then return 0.  Although 0 means that lifetime is zero,
 * such a value should not be accepted.
 * Also 0 of lifebyte should not be included in a packet although 0 means not
 * to care of it.
 */
static u_int32_t
ipsecdoi_set_ld(buf)
	vchar_t *buf;
{
	u_int32_t ld;

	if (buf == 0)
		return 0;

	switch (buf->l) {
	case 2:
		ld = ntohs(*(u_int16_t *)buf->v);
		break;
	case 4:
		ld = ntohl(*(u_int32_t *)buf->v);
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"length %zu of life duration "
			"isn't supported.\n", buf->l);
		return 0;
	}

	return ld;
}

/*%%%*/
/*
 * check DOI
 */
static int
check_doi(doi)
	u_int32_t doi;
{
	switch (doi) {
	case IPSEC_DOI:
		return 0;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid value of DOI 0x%08x.\n", doi);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check situation
 */
static int
check_situation(sit)
	u_int32_t sit;
{
	switch (sit) {
	case IPSECDOI_SIT_IDENTITY_ONLY:
		return 0;

	case IPSECDOI_SIT_SECRECY:
	case IPSECDOI_SIT_INTEGRITY:
		plog(LLV_ERROR, LOCATION, NULL,
			"situation 0x%08x unsupported yet.\n", sit);
		return -1;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid situation 0x%08x.\n", sit);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check protocol id in main mode
 */
static int
check_prot_main(proto_id)
	int proto_id;
{
	switch (proto_id) {
	case IPSECDOI_PROTO_ISAKMP:
		return 0;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"Illegal protocol id=%u.\n", proto_id);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check protocol id in quick mode
 */
static int
check_prot_quick(proto_id)
	int proto_id;
{
	switch (proto_id) {
	case IPSECDOI_PROTO_IPSEC_AH:
	case IPSECDOI_PROTO_IPSEC_ESP:
		return 0;

	case IPSECDOI_PROTO_IPCOMP:
		return 0;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid protocol id %d.\n", proto_id);
		return -1;
	}
	/* NOT REACHED */
}

static int
check_spi_size(proto_id, size)
	int proto_id, size;
{
	switch (proto_id) {
	case IPSECDOI_PROTO_ISAKMP:
		if (size != 0) {
			/* WARNING */
			plog(LLV_WARNING, LOCATION, NULL,
				"SPI size isn't zero, but IKE proposal.\n");
		}
		return 0;

	case IPSECDOI_PROTO_IPSEC_AH:
	case IPSECDOI_PROTO_IPSEC_ESP:
		if (size != 4) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid SPI size=%d for IPSEC proposal.\n",
				size);
			return -1;
		}
		return 0;

	case IPSECDOI_PROTO_IPCOMP:
		if (size != 2 && size != 4) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid SPI size=%d for IPCOMP proposal.\n",
				size);
			return -1;
		}
		return 0;

	default:
		/* ??? */
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check transform ID in ISAKMP.
 */
static int
check_trns_isakmp(t_id)
	int t_id;
{
	switch (t_id) {
	case IPSECDOI_KEY_IKE:
		return 0;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid transform-id=%u in proto_id=%u.\n",
			t_id, IPSECDOI_KEY_IKE);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check transform ID in AH.
 */
static int
check_trns_ah(t_id)
	int t_id;
{
	switch (t_id) {
	case IPSECDOI_AH_MD5:
	case IPSECDOI_AH_SHA:
	case IPSECDOI_AH_SHA256:
	case IPSECDOI_AH_SHA384:
	case IPSECDOI_AH_SHA512:
		return 0;
	case IPSECDOI_AH_DES:
		plog(LLV_ERROR, LOCATION, NULL,
			"not support transform-id=%u in AH.\n", t_id);
		return -1;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid transform-id=%u in AH.\n", t_id);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check transform ID in ESP.
 */
static int
check_trns_esp(t_id)
	int t_id;
{
	switch (t_id) {
	case IPSECDOI_ESP_DES:
	case IPSECDOI_ESP_3DES:
	case IPSECDOI_ESP_NULL:
	case IPSECDOI_ESP_RC5:
	case IPSECDOI_ESP_CAST:
	case IPSECDOI_ESP_BLOWFISH:
	case IPSECDOI_ESP_AES:
	case IPSECDOI_ESP_TWOFISH:
	case IPSECDOI_ESP_CAMELLIA:
		return 0;
	case IPSECDOI_ESP_DES_IV32:
	case IPSECDOI_ESP_DES_IV64:
	case IPSECDOI_ESP_IDEA:
	case IPSECDOI_ESP_3IDEA:
	case IPSECDOI_ESP_RC4:
		plog(LLV_ERROR, LOCATION, NULL,
			"not support transform-id=%u in ESP.\n", t_id);
		return -1;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid transform-id=%u in ESP.\n", t_id);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check transform ID in IPCOMP.
 */
static int
check_trns_ipcomp(t_id)
	int t_id;
{
	switch (t_id) {
	case IPSECDOI_IPCOMP_OUI:
	case IPSECDOI_IPCOMP_DEFLATE:
	case IPSECDOI_IPCOMP_LZS:
		return 0;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid transform-id=%u in IPCOMP.\n", t_id);
		return -1;
	}
	/* NOT REACHED */
}

/*
 * check data attributes in IKE.
 */
static int
check_attr_isakmp(trns)
	struct isakmp_pl_t *trns;
{
	struct isakmp_data *d;
	int tlen;
	int flag, type;
	u_int16_t lorv;

	tlen = ntohs(trns->h.len) - sizeof(struct isakmp_pl_t);
	d = (struct isakmp_data *)((caddr_t)trns + sizeof(struct isakmp_pl_t));

	while (tlen > 0) {
		type = ntohs(d->type) & ~ISAKMP_GEN_MASK;
		flag = ntohs(d->type) & ISAKMP_GEN_MASK;
		lorv = ntohs(d->lorv);

		plog(LLV_DEBUG, LOCATION, NULL,
			"type=%s, flag=0x%04x, lorv=%s\n",
			s_oakley_attr(type), flag,
			s_oakley_attr_v(type, lorv));

		/*
		 * some of the attributes must be encoded in TV.
		 * see RFC2409 Appendix A "Attribute Classes".
		 */
		switch (type) {
		case OAKLEY_ATTR_ENC_ALG:
		case OAKLEY_ATTR_HASH_ALG:
		case OAKLEY_ATTR_AUTH_METHOD:
		case OAKLEY_ATTR_GRP_DESC:
		case OAKLEY_ATTR_GRP_TYPE:
		case OAKLEY_ATTR_SA_LD_TYPE:
		case OAKLEY_ATTR_PRF:
		case OAKLEY_ATTR_KEY_LEN:
		case OAKLEY_ATTR_FIELD_SIZE:
			if (!flag) {	/* TLV*/
				plog(LLV_ERROR, LOCATION, NULL,
					"oakley attribute %d must be TV.\n",
					type);
				return -1;
			}
			break;
		}

		/* sanity check for TLV.  length must be specified. */
		if (!flag && lorv == 0) {	/*TLV*/
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid length %d for TLV attribute %d.\n",
				lorv, type);
			return -1;
		}

		switch (type) {
		case OAKLEY_ATTR_ENC_ALG:
			if (!alg_oakley_encdef_ok(lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"invalied encryption algorithm=%d.\n",
					lorv);
				return -1;
			}
			break;

		case OAKLEY_ATTR_HASH_ALG:
			if (!alg_oakley_hashdef_ok(lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"invalied hash algorithm=%d.\n",
					lorv);
				return -1;
			}
			break;

		case OAKLEY_ATTR_AUTH_METHOD:
			switch (lorv) {
			case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
			case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
#ifdef ENABLE_HYBRID
			case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I:
#if 0 /* Clashes with OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB */
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_I:
#endif
#endif
			case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
				break;
			case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
#ifdef ENABLE_HYBRID
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_R:
			case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_R:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_R:
			case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I:
			case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_R:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_R:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_R:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I:
			case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_R:
#endif
			case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
			case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
				plog(LLV_ERROR, LOCATION, NULL,
					"auth method %s isn't supported.\n",
					s_oakley_attr_method(lorv));
				return -1;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid auth method %d.\n",
					lorv);
				return -1;
			}
			break;

		case OAKLEY_ATTR_GRP_DESC:
			if (!alg_oakley_dhdef_ok(lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid DH group %d.\n",
					lorv);
				return -1;
			}
			break;

		case OAKLEY_ATTR_GRP_TYPE:
			switch (lorv) {
			case OAKLEY_ATTR_GRP_TYPE_MODP:
				break;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"unsupported DH group type %d.\n",
					lorv);
				return -1;
			}
			break;

		case OAKLEY_ATTR_GRP_PI:
		case OAKLEY_ATTR_GRP_GEN_ONE:
			/* sanity checks? */
			break;

		case OAKLEY_ATTR_GRP_GEN_TWO:
		case OAKLEY_ATTR_GRP_CURVE_A:
		case OAKLEY_ATTR_GRP_CURVE_B:
			plog(LLV_ERROR, LOCATION, NULL,
				"attr type=%u isn't supported.\n", type);
			return -1;

		case OAKLEY_ATTR_SA_LD_TYPE:
			switch (lorv) {
			case OAKLEY_ATTR_SA_LD_TYPE_SEC:
			case OAKLEY_ATTR_SA_LD_TYPE_KB:
				break;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid life type %d.\n", lorv);
				return -1;
			}
			break;

		case OAKLEY_ATTR_SA_LD:
			/* should check the value */
			break;

		case OAKLEY_ATTR_PRF:
		case OAKLEY_ATTR_KEY_LEN:
			break;

		case OAKLEY_ATTR_FIELD_SIZE:
			plog(LLV_ERROR, LOCATION, NULL,
				"attr type=%u isn't supported.\n", type);
			return -1;

		case OAKLEY_ATTR_GRP_ORDER:
			break;

		case OAKLEY_ATTR_GSS_ID:
			break;

		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid attribute type %d.\n", type);
			return -1;
		}

		if (flag) {
			tlen -= sizeof(*d);
			d = (struct isakmp_data *)((char *)d
				+ sizeof(*d));
		} else {
			tlen -= (sizeof(*d) + lorv);
			d = (struct isakmp_data *)((char *)d
				+ sizeof(*d) + lorv);
		}
	}

	return 0;
}

/*
 * check data attributes in IPSEC AH/ESP.
 */
static int
check_attr_ah(trns)
	struct isakmp_pl_t *trns;
{
	return check_attr_ipsec(IPSECDOI_PROTO_IPSEC_AH, trns);
}

static int
check_attr_esp(trns)
	struct isakmp_pl_t *trns;
{
	return check_attr_ipsec(IPSECDOI_PROTO_IPSEC_ESP, trns);
}

static int
check_attr_ipsec(proto_id, trns)
	int proto_id;
	struct isakmp_pl_t *trns;
{
	struct isakmp_data *d;
	int tlen;
	int flag, type = 0;
	u_int16_t lorv;
	int attrseen[16];	/* XXX magic number */

	tlen = ntohs(trns->h.len) - sizeof(struct isakmp_pl_t);
	d = (struct isakmp_data *)((caddr_t)trns + sizeof(struct isakmp_pl_t));
	memset(attrseen, 0, sizeof(attrseen));

	while (tlen > 0) {
		type = ntohs(d->type) & ~ISAKMP_GEN_MASK;
		flag = ntohs(d->type) & ISAKMP_GEN_MASK;
		lorv = ntohs(d->lorv);

		plog(LLV_DEBUG, LOCATION, NULL,
			"type=%s, flag=0x%04x, lorv=%s\n",
			s_ipsecdoi_attr(type), flag,
			s_ipsecdoi_attr_v(type, lorv));

		if (type < sizeof(attrseen)/sizeof(attrseen[0]))
			attrseen[type]++;

		switch (type) {
		case IPSECDOI_ATTR_ENC_MODE:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when ENC_MODE.\n");
				return -1;
			}

			switch (lorv) {
			case IPSECDOI_ATTR_ENC_MODE_TUNNEL:
			case IPSECDOI_ATTR_ENC_MODE_TRNS:
				break;
#ifdef ENABLE_NATT
			case IPSECDOI_ATTR_ENC_MODE_UDPTUNNEL_RFC:
			case IPSECDOI_ATTR_ENC_MODE_UDPTRNS_RFC:
			case IPSECDOI_ATTR_ENC_MODE_UDPTUNNEL_DRAFT:
			case IPSECDOI_ATTR_ENC_MODE_UDPTRNS_DRAFT:
				plog(LLV_DEBUG, LOCATION, NULL,
				     "UDP encapsulation requested\n");
				break;
#endif
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid encryption mode=%u.\n",
					lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_AUTH:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when AUTH.\n");
				return -1;
			}

			switch (lorv) {
			case IPSECDOI_ATTR_AUTH_HMAC_MD5:
				if (proto_id == IPSECDOI_PROTO_IPSEC_AH &&
				    trns->t_id != IPSECDOI_AH_MD5) {
ahmismatch:
					plog(LLV_ERROR, LOCATION, NULL,
						"auth algorithm %u conflicts "
						"with transform %u.\n",
						lorv, trns->t_id);
					return -1;
				}
				break;
			case IPSECDOI_ATTR_AUTH_HMAC_SHA1:
				if (proto_id == IPSECDOI_PROTO_IPSEC_AH) {
					if (trns->t_id != IPSECDOI_AH_SHA)
						goto ahmismatch;
				}
				break;
 			case IPSECDOI_ATTR_AUTH_HMAC_SHA2_256:
 				if (proto_id == IPSECDOI_PROTO_IPSEC_AH) {
 					if (trns->t_id != IPSECDOI_AH_SHA256)
 						goto ahmismatch;
 				}	
 				break;
 			case IPSECDOI_ATTR_AUTH_HMAC_SHA2_384:
 				if (proto_id == IPSECDOI_PROTO_IPSEC_AH) {
 					if (trns->t_id != IPSECDOI_AH_SHA384)
 						goto ahmismatch;
 				}
 				break;
 			case IPSECDOI_ATTR_AUTH_HMAC_SHA2_512:
 				if (proto_id == IPSECDOI_PROTO_IPSEC_AH) {
 					if (trns->t_id != IPSECDOI_AH_SHA512)
 					goto ahmismatch;
 				}
 				break;
			case IPSECDOI_ATTR_AUTH_DES_MAC:
			case IPSECDOI_ATTR_AUTH_KPDK:
				plog(LLV_ERROR, LOCATION, NULL,
					"auth algorithm %u isn't supported.\n",
					lorv);
				return -1;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid auth algorithm=%u.\n",
					lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_SA_LD_TYPE:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when LD_TYPE.\n");
				return -1;
			}

			switch (lorv) {
			case IPSECDOI_ATTR_SA_LD_TYPE_SEC:
			case IPSECDOI_ATTR_SA_LD_TYPE_KB:
				break;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid life type %d.\n", lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_SA_LD:
			if (flag) {
				/* i.e. ISAKMP_GEN_TV */
				plog(LLV_DEBUG, LOCATION, NULL,
					"life duration was in TLV.\n");
			} else {
				/* i.e. ISAKMP_GEN_TLV */
				if (lorv == 0) {
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid length of LD\n");
					return -1;
				}
			}
			break;

		case IPSECDOI_ATTR_GRP_DESC:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when GRP_DESC.\n");
				return -1;
			}

			if (!alg_oakley_dhdef_ok(lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid group description=%u.\n",
					lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_KEY_LENGTH:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when KEY_LENGTH.\n");
				return -1;
			}
			break;

#ifdef HAVE_SECCTX
		case IPSECDOI_ATTR_SECCTX:
			if (flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"SECCTX must be in TLV.\n");
				return -1;
			}
		break;
#endif

		case IPSECDOI_ATTR_KEY_ROUNDS:
		case IPSECDOI_ATTR_COMP_DICT_SIZE:
		case IPSECDOI_ATTR_COMP_PRIVALG:
			plog(LLV_ERROR, LOCATION, NULL,
				"attr type=%u isn't supported.\n", type);
			return -1;

		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid attribute type %d.\n", type);
			return -1;
		}

		if (flag) {
			tlen -= sizeof(*d);
			d = (struct isakmp_data *)((char *)d
				+ sizeof(*d));
		} else {
			tlen -= (sizeof(*d) + lorv);
			d = (struct isakmp_data *)((caddr_t)d
				+ sizeof(*d) + lorv);
		}
	}

	if (proto_id == IPSECDOI_PROTO_IPSEC_AH &&
	    !attrseen[IPSECDOI_ATTR_AUTH]) {
		plog(LLV_ERROR, LOCATION, NULL,
			"attr AUTH must be present for AH.\n");
		return -1;
	}

	if (proto_id == IPSECDOI_PROTO_IPSEC_ESP &&
	    trns->t_id == IPSECDOI_ESP_NULL &&
	    !attrseen[IPSECDOI_ATTR_AUTH]) {
		plog(LLV_ERROR, LOCATION, NULL,
		    "attr AUTH must be present for ESP NULL encryption.\n");
		return -1;
	}

	return 0;
}

static int
check_attr_ipcomp(trns)
	struct isakmp_pl_t *trns;
{
	struct isakmp_data *d;
	int tlen;
	int flag, type = 0;
	u_int16_t lorv;
	int attrseen[16];	/* XXX magic number */

	tlen = ntohs(trns->h.len) - sizeof(struct isakmp_pl_t);
	d = (struct isakmp_data *)((caddr_t)trns + sizeof(struct isakmp_pl_t));
	memset(attrseen, 0, sizeof(attrseen));

	while (tlen > 0) {
		type = ntohs(d->type) & ~ISAKMP_GEN_MASK;
		flag = ntohs(d->type) & ISAKMP_GEN_MASK;
		lorv = ntohs(d->lorv);

		plog(LLV_DEBUG, LOCATION, NULL,
			"type=%d, flag=0x%04x, lorv=0x%04x\n",
			type, flag, lorv);

		if (type < sizeof(attrseen)/sizeof(attrseen[0]))
			attrseen[type]++;

		switch (type) {
		case IPSECDOI_ATTR_ENC_MODE:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when ENC_MODE.\n");
				return -1;
			}

			switch (lorv) {
			case IPSECDOI_ATTR_ENC_MODE_TUNNEL:
			case IPSECDOI_ATTR_ENC_MODE_TRNS:
				break;
#ifdef ENABLE_NATT
			case IPSECDOI_ATTR_ENC_MODE_UDPTUNNEL_RFC:
			case IPSECDOI_ATTR_ENC_MODE_UDPTRNS_RFC:
			case IPSECDOI_ATTR_ENC_MODE_UDPTUNNEL_DRAFT:
			case IPSECDOI_ATTR_ENC_MODE_UDPTRNS_DRAFT:
				plog(LLV_DEBUG, LOCATION, NULL,
				     "UDP encapsulation requested\n");
				break;
#endif
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid encryption mode=%u.\n",
					lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_SA_LD_TYPE:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when LD_TYPE.\n");
				return -1;
			}

			switch (lorv) {
			case IPSECDOI_ATTR_SA_LD_TYPE_SEC:
			case IPSECDOI_ATTR_SA_LD_TYPE_KB:
				break;
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid life type %d.\n", lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_SA_LD:
			if (flag) {
				/* i.e. ISAKMP_GEN_TV */
				plog(LLV_DEBUG, LOCATION, NULL,
					"life duration was in TLV.\n");
			} else {
				/* i.e. ISAKMP_GEN_TLV */
				if (lorv == 0) {
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid length of LD\n");
					return -1;
				}
			}
			break;

		case IPSECDOI_ATTR_GRP_DESC:
			if (! flag) {
				plog(LLV_ERROR, LOCATION, NULL,
					"must be TV when GRP_DESC.\n");
				return -1;
			}

			if (!alg_oakley_dhdef_ok(lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid group description=%u.\n",
					lorv);
				return -1;
			}
			break;

		case IPSECDOI_ATTR_AUTH:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid attr type=%u.\n", type);
			return -1;

		case IPSECDOI_ATTR_KEY_LENGTH:
		case IPSECDOI_ATTR_KEY_ROUNDS:
		case IPSECDOI_ATTR_COMP_DICT_SIZE:
		case IPSECDOI_ATTR_COMP_PRIVALG:
			plog(LLV_ERROR, LOCATION, NULL,
				"attr type=%u isn't supported.\n", type);
			return -1;

		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid attribute type %d.\n", type);
			return -1;
		}

		if (flag) {
			tlen -= sizeof(*d);
			d = (struct isakmp_data *)((char *)d
				+ sizeof(*d));
		} else {
			tlen -= (sizeof(*d) + lorv);
			d = (struct isakmp_data *)((caddr_t)d
				+ sizeof(*d) + lorv);
		}
	}

#if 0
	if (proto_id == IPSECDOI_PROTO_IPCOMP &&
	    !attrseen[IPSECDOI_ATTR_AUTH]) {
		plog(LLV_ERROR, LOCATION, NULL,
			"attr AUTH must be present for AH.\n", type);
		return -1;
	}
#endif

	return 0;
}

/* %%% */
/*
 * create phase1 proposal from remote configuration.
 * NOT INCLUDING isakmp general header of SA payload
 */
vchar_t *
ipsecdoi_setph1proposal(props)
	struct isakmpsa *props;
{
	vchar_t *mysa;
	int sablen;

	/* count total size of SA minus isakmp general header */
	/* not including isakmp general header of SA payload */
	sablen = sizeof(struct ipsecdoi_sa_b);
	sablen += setph1prop(props, NULL);

	mysa = vmalloc(sablen);
	if (mysa == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate my sa buffer\n");
		return NULL;
	}

	/* create SA payload */
	/* not including isakmp general header */
	((struct ipsecdoi_sa_b *)mysa->v)->doi = htonl(props->rmconf->doitype);
	((struct ipsecdoi_sa_b *)mysa->v)->sit = htonl(props->rmconf->sittype);

	(void)setph1prop(props, mysa->v + sizeof(struct ipsecdoi_sa_b));

	return mysa;
}

static int
setph1prop(props, buf)
	struct isakmpsa *props;
	caddr_t buf;
{
	struct isakmp_pl_p *prop = NULL;
	struct isakmpsa *s = NULL;
	int proplen, trnslen;
	u_int8_t *np_t; /* pointer next trns type in previous header */
	int trns_num;
	caddr_t p = buf;

	proplen = sizeof(*prop);
	if (buf) {
		/* create proposal */
		prop = (struct isakmp_pl_p *)p;
		prop->h.np = ISAKMP_NPTYPE_NONE;
		prop->p_no = props->prop_no;
		prop->proto_id = IPSECDOI_PROTO_ISAKMP;
		prop->spi_size = 0;
		p += sizeof(*prop);
	}

	np_t = NULL;
	trns_num = 0;

	for (s = props; s != NULL; s = s->next) {
		if (np_t)
			*np_t = ISAKMP_NPTYPE_T;

		trnslen = setph1trns(s, p);
		proplen += trnslen;
		if (buf) {
			/* save buffer to pre-next payload */
			np_t = &((struct isakmp_pl_t *)p)->h.np;
			p += trnslen;

			/* count up transform length */
			trns_num++;
		}
	}

	/* update proposal length */
	if (buf) {
		prop->h.len = htons(proplen);
		prop->num_t = trns_num;
	}

	return proplen;
}

static int
setph1trns(sa, buf)
	struct isakmpsa *sa;
	caddr_t buf;
{
	struct isakmp_pl_t *trns = NULL;
	int trnslen, attrlen;
	caddr_t p = buf;

	trnslen = sizeof(*trns);
	if (buf) {
		/* create transform */
		trns = (struct isakmp_pl_t *)p;
		trns->h.np  = ISAKMP_NPTYPE_NONE;
		trns->t_no  = sa->trns_no;
		trns->t_id  = IPSECDOI_KEY_IKE;
		p += sizeof(*trns);
	}

	attrlen = setph1attr(sa, p);
	trnslen += attrlen;
	if (buf)
		p += attrlen;

	if (buf)
		trns->h.len = htons(trnslen);

	return trnslen;
}

static int
setph1attr(sa, buf)
	struct isakmpsa *sa;
	caddr_t buf;
{
	caddr_t p = buf;
	int attrlen = 0;

	if (sa->lifetime) {
		u_int32_t lifetime = htonl((u_int32_t)sa->lifetime);

		attrlen += sizeof(struct isakmp_data)
			+ sizeof(struct isakmp_data);
		if (sa->lifetime > 0xffff)
			attrlen += sizeof(lifetime);
		if (buf) {
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_SA_LD_TYPE,
						OAKLEY_ATTR_SA_LD_TYPE_SEC);
			if (sa->lifetime > 0xffff) {
				p = isakmp_set_attr_v(p, OAKLEY_ATTR_SA_LD,
						(caddr_t)&lifetime, 
						sizeof(lifetime));
			} else {
				p = isakmp_set_attr_l(p, OAKLEY_ATTR_SA_LD,
							sa->lifetime);
			}
		}
	}

	if (sa->lifebyte) {
		u_int32_t lifebyte = htonl((u_int32_t)sa->lifebyte);
		
		attrlen += sizeof(struct isakmp_data)
			+ sizeof(struct isakmp_data);
		if (sa->lifebyte > 0xffff)
			attrlen += sizeof(lifebyte);
		if (buf) {
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_SA_LD_TYPE,
						OAKLEY_ATTR_SA_LD_TYPE_KB);
			if (sa->lifebyte > 0xffff) {
				p = isakmp_set_attr_v(p, OAKLEY_ATTR_SA_LD,
							(caddr_t)&lifebyte,
							sizeof(lifebyte));
			} else {
				p = isakmp_set_attr_l(p, OAKLEY_ATTR_SA_LD,
							sa->lifebyte);
			}
		}
	}

	if (sa->enctype) {
		attrlen += sizeof(struct isakmp_data);
		if (buf)
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_ENC_ALG, sa->enctype);
	}
	if (sa->encklen) {
		attrlen += sizeof(struct isakmp_data);
		if (buf)
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_KEY_LEN, sa->encklen);
	}
	if (sa->authmethod) {
		int authmethod;

#ifdef ENABLE_HYBRID
		authmethod = switch_authmethod(sa->authmethod);
#else
		authmethod = sa->authmethod;
#endif
		attrlen += sizeof(struct isakmp_data);
		if (buf)
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_AUTH_METHOD, authmethod);
	}
	if (sa->hashtype) {
		attrlen += sizeof(struct isakmp_data);
		if (buf)
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_HASH_ALG, sa->hashtype);
	}
	switch (sa->dh_group) {
	case OAKLEY_ATTR_GRP_DESC_MODP768:
	case OAKLEY_ATTR_GRP_DESC_MODP1024:
	case OAKLEY_ATTR_GRP_DESC_MODP1536:
	case OAKLEY_ATTR_GRP_DESC_MODP2048:
	case OAKLEY_ATTR_GRP_DESC_MODP3072:
	case OAKLEY_ATTR_GRP_DESC_MODP4096:
	case OAKLEY_ATTR_GRP_DESC_MODP6144:
	case OAKLEY_ATTR_GRP_DESC_MODP8192:
		/* don't attach group type for known groups */
		attrlen += sizeof(struct isakmp_data);
		if (buf) {
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_GRP_DESC,
				sa->dh_group);
		}
		break;
	case OAKLEY_ATTR_GRP_DESC_EC2N155:
	case OAKLEY_ATTR_GRP_DESC_EC2N185:
		/* don't attach group type for known groups */
		attrlen += sizeof(struct isakmp_data);
		if (buf) {
			p = isakmp_set_attr_l(p, OAKLEY_ATTR_GRP_TYPE,
				OAKLEY_ATTR_GRP_TYPE_EC2N);
		}
		break;
	case 0:
	default:
		break;
	}

#ifdef HAVE_GSSAPI
	if (sa->authmethod == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB &&
	    sa->gssid != NULL) {
		attrlen += sizeof(struct isakmp_data);
		/*
		 * Older versions of racoon just placed the ISO-Latin-1
		 * string on the wire directly.  Check to see if we are
		 * configured to be compatible with this behavior.  Otherwise,
		 * we encode the GSS ID as UTF-16LE for Windows 2000
		 * compatibility, which requires twice the number of octets.
		 */
		if (lcconf->gss_id_enc == LC_GSSENC_LATIN1)
			attrlen += sa->gssid->l;
		else
			attrlen += sa->gssid->l * 2;
		if (buf) {
			plog(LLV_DEBUG, LOCATION, NULL, "gss id attr: len %zu, "
			    "val '%.*s'\n", sa->gssid->l, (int)sa->gssid->l,
			    sa->gssid->v);
			if (lcconf->gss_id_enc == LC_GSSENC_LATIN1) {
				p = isakmp_set_attr_v(p, OAKLEY_ATTR_GSS_ID,
					(caddr_t)sa->gssid->v,
					sa->gssid->l);
			} else {
				size_t dstleft = sa->gssid->l * 2;
				size_t srcleft = sa->gssid->l;
				const char *src = (const char *)sa->gssid->v;
				char *odst, *dst = racoon_malloc(dstleft);
				iconv_t cd;
				size_t rv;

				cd = iconv_open("utf-16le", "latin1");
				if (cd == (iconv_t) -1) {
					plog(LLV_ERROR, LOCATION, NULL,
					    "unable to initialize "
					    "latin1 -> utf-16le "
					    "converstion descriptor: %s\n",
					    strerror(errno));
					attrlen -= sa->gssid->l * 2;
					goto gssid_done;
				}
				odst = dst;
				rv = iconv(cd, (__iconv_const char **)&src, 
				    &srcleft, &dst, &dstleft);
				if (rv != 0) {
					if (rv == -1) {
						plog(LLV_ERROR, LOCATION, NULL,
						    "unable to convert GSS ID "
						    "from latin1 -> utf-16le: "
						    "%s\n", strerror(errno));
					} else {
						/* should never happen */
						plog(LLV_ERROR, LOCATION, NULL,
						    "%zd character%s in GSS ID "
						    "cannot be represented "
						    "in utf-16le\n",
						    rv, rv == 1 ? "" : "s");
					}
					(void) iconv_close(cd);
					attrlen -= sa->gssid->l * 2;
					goto gssid_done;
				}
				(void) iconv_close(cd);

				/* XXX Check srcleft and dstleft? */

				p = isakmp_set_attr_v(p, OAKLEY_ATTR_GSS_ID,
					odst, sa->gssid->l * 2);

				racoon_free(odst);
			}
		}
	}
 gssid_done:
#endif /* HAVE_GSSAPI */

	return attrlen;
}

static vchar_t *
setph2proposal0(iph2, pp, pr)
	const struct ph2handle *iph2;
	const struct saprop *pp;
	const struct saproto *pr;
{
	vchar_t *p;
	struct isakmp_pl_p *prop;
	struct isakmp_pl_t *trns;
	struct satrns *tr;
	int attrlen;
	size_t trnsoff;
	caddr_t x0, x;
	u_int8_t *np_t; /* pointer next trns type in previous header */
	const u_int8_t *spi;
#ifdef HAVE_SECCTX
	int truectxlen = 0;
#endif

	p = vmalloc(sizeof(*prop) + sizeof(pr->spi));
	if (p == NULL)
		return NULL;

	/* create proposal */
	prop = (struct isakmp_pl_p *)p->v;
	prop->h.np = ISAKMP_NPTYPE_NONE;
	prop->p_no = pp->prop_no;
	prop->proto_id = pr->proto_id;
	prop->num_t = 1;

	spi = (const u_int8_t *)&pr->spi;
	switch (pr->proto_id) {
	case IPSECDOI_PROTO_IPCOMP:
		/*
		 * draft-shacham-ippcp-rfc2393bis-05.txt:
		 * construct 16bit SPI (CPI).
		 * XXX we may need to provide a configuration option to
		 * generate 32bit SPI.  otherwise we cannot interoeprate
		 * with nodes that uses 32bit SPI, in case we are initiator.
		 */
		prop->spi_size = sizeof(u_int16_t);
		spi += sizeof(pr->spi) - sizeof(u_int16_t);
		p->l -= sizeof(pr->spi);
		p->l += sizeof(u_int16_t);
		break;
	default:
		prop->spi_size = sizeof(pr->spi);
		break;
	}
	memcpy(prop + 1, spi, prop->spi_size);

	/* create transform */
	trnsoff = sizeof(*prop) + prop->spi_size;
	np_t = NULL;

	for (tr = pr->head; tr; tr = tr->next) {
	
		switch (pr->proto_id) {
		case IPSECDOI_PROTO_IPSEC_ESP:
			/*
			 * don't build a null encryption
			 * with no authentication transform.
			 */
			if (tr->trns_id == IPSECDOI_ESP_NULL &&
			    tr->authtype == IPSECDOI_ATTR_AUTH_NONE)
				continue;
			break;
		}

		if (np_t) {
			*np_t = ISAKMP_NPTYPE_T;
			prop->num_t++;
		}

		/* get attribute length */
		attrlen = 0;
		if (pp->lifetime) {
			attrlen += sizeof(struct isakmp_data)
				+ sizeof(struct isakmp_data);
			if (pp->lifetime > 0xffff)
				attrlen += sizeof(u_int32_t);
		}
		if (pp->lifebyte && pp->lifebyte != IPSECDOI_ATTR_SA_LD_KB_MAX) {
			attrlen += sizeof(struct isakmp_data)
				+ sizeof(struct isakmp_data);
			if (pp->lifebyte > 0xffff)
				attrlen += sizeof(u_int32_t);
		}
		attrlen += sizeof(struct isakmp_data);	/* enc mode */
		if (tr->encklen)
			attrlen += sizeof(struct isakmp_data);

		switch (pr->proto_id) {
		case IPSECDOI_PROTO_IPSEC_ESP:
			/* non authentication mode ? */
			if (tr->authtype != IPSECDOI_ATTR_AUTH_NONE)
				attrlen += sizeof(struct isakmp_data);
			break;
		case IPSECDOI_PROTO_IPSEC_AH:
			if (tr->authtype == IPSECDOI_ATTR_AUTH_NONE) {
				plog(LLV_ERROR, LOCATION, NULL,
					"no authentication algorithm found "
					"but protocol is AH.\n");
				vfree(p);
				return NULL;
			}
			attrlen += sizeof(struct isakmp_data);
			break;
		case IPSECDOI_PROTO_IPCOMP:
			break;
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid protocol: %d\n", pr->proto_id);
			vfree(p);
			return NULL;
		}

		if (alg_oakley_dhdef_ok(iph2->sainfo->pfs_group))
			attrlen += sizeof(struct isakmp_data);

#ifdef HAVE_SECCTX
		/* ctx_str is defined as char ctx_str[MAX_CTXSTR_SIZ].
		 * The string may be smaller than MAX_CTXSTR_SIZ.
		 */
		if (*pp->sctx.ctx_str) {
			truectxlen = sizeof(struct security_ctx) -
				     (MAX_CTXSTR_SIZE - pp->sctx.ctx_strlen);
			attrlen += sizeof(struct isakmp_data) + truectxlen;
		}
#endif /* HAVE_SECCTX */

		p = vrealloc(p, p->l + sizeof(*trns) + attrlen);
		if (p == NULL)
			return NULL;
		prop = (struct isakmp_pl_p *)p->v;

		/* set transform's values */
		trns = (struct isakmp_pl_t *)(p->v + trnsoff);
		trns->h.np  = ISAKMP_NPTYPE_NONE;
		trns->t_no  = tr->trns_no;
		trns->t_id  = tr->trns_id;

		/* set attributes */
		x = x0 = p->v + trnsoff + sizeof(*trns);

		if (pp->lifetime) {
			x = isakmp_set_attr_l(x, IPSECDOI_ATTR_SA_LD_TYPE,
						IPSECDOI_ATTR_SA_LD_TYPE_SEC);
			if (pp->lifetime > 0xffff) {
				u_int32_t v = htonl((u_int32_t)pp->lifetime);
				x = isakmp_set_attr_v(x, IPSECDOI_ATTR_SA_LD,
							(caddr_t)&v, sizeof(v));
			} else {
				x = isakmp_set_attr_l(x, IPSECDOI_ATTR_SA_LD,
							pp->lifetime);
			}
		}

		if (pp->lifebyte && pp->lifebyte != IPSECDOI_ATTR_SA_LD_KB_MAX) {
			x = isakmp_set_attr_l(x, IPSECDOI_ATTR_SA_LD_TYPE,
						IPSECDOI_ATTR_SA_LD_TYPE_KB);
			if (pp->lifebyte > 0xffff) {
				u_int32_t v = htonl((u_int32_t)pp->lifebyte);
				x = isakmp_set_attr_v(x, IPSECDOI_ATTR_SA_LD,
							(caddr_t)&v, sizeof(v));
			} else {
				x = isakmp_set_attr_l(x, IPSECDOI_ATTR_SA_LD,
							pp->lifebyte);
			}
		}

		x = isakmp_set_attr_l(x, IPSECDOI_ATTR_ENC_MODE, pr->encmode);

		if (tr->encklen)
			x = isakmp_set_attr_l(x, IPSECDOI_ATTR_KEY_LENGTH, tr->encklen);

		/* mandatory check has done above. */
		if ((pr->proto_id == IPSECDOI_PROTO_IPSEC_ESP && tr->authtype != IPSECDOI_ATTR_AUTH_NONE)
		 || pr->proto_id == IPSECDOI_PROTO_IPSEC_AH)
			x = isakmp_set_attr_l(x, IPSECDOI_ATTR_AUTH, tr->authtype);

		if (alg_oakley_dhdef_ok(iph2->sainfo->pfs_group))
			x = isakmp_set_attr_l(x, IPSECDOI_ATTR_GRP_DESC,
				iph2->sainfo->pfs_group);

#ifdef HAVE_SECCTX
		if (*pp->sctx.ctx_str) 
			x = isakmp_set_attr_v(x, IPSECDOI_ATTR_SECCTX,
					     (caddr_t)&pp->sctx, truectxlen);
#endif
		/* update length of this transform. */
		trns = (struct isakmp_pl_t *)(p->v + trnsoff);
		trns->h.len = htons(sizeof(*trns) + attrlen);

		/* save buffer to pre-next payload */
		np_t = &trns->h.np;

		trnsoff += (sizeof(*trns) + attrlen);
	}

	if (np_t == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no suitable proposal was created.\n");
		return NULL;
	}

	/* update length of this protocol. */
	prop->h.len = htons(p->l);

	return p;
}

/*
 * create phase2 proposal from policy configuration.
 * NOT INCLUDING isakmp general header of SA payload.
 * This function is called by initiator only.
 */
int
ipsecdoi_setph2proposal(iph2)
	struct ph2handle *iph2;
{
	struct saprop *proposal, *a;
	struct saproto *b = NULL;
	vchar_t *q;
	struct ipsecdoi_sa_b *sab;
	struct isakmp_pl_p *prop;
	size_t propoff;	/* for previous field of type of next payload. */

	proposal = iph2->proposal;

	iph2->sa = vmalloc(sizeof(*sab));
	if (iph2->sa == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate my sa buffer\n");
		return -1;
	}

	/* create SA payload */
	sab = (struct ipsecdoi_sa_b *)iph2->sa->v;
	sab->doi = htonl(IPSEC_DOI);
	sab->sit = htonl(IPSECDOI_SIT_IDENTITY_ONLY);	/* XXX configurable ? */

	prop = NULL;
	propoff = 0;
	for (a = proposal; a; a = a->next) {
		for (b = a->head; b; b = b->next) {
#ifdef ENABLE_NATT
			if (iph2->ph1->natt_flags & NAT_DETECTED) {
			  int udp_diff = iph2->ph1->natt_options->mode_udp_diff;
			  plog (LLV_INFO, LOCATION, NULL,
				"NAT detected -> UDP encapsulation "
				"(ENC_MODE %d->%d).\n",
				b->encmode,
				b->encmode+udp_diff);
			  /* Tunnel -> UDP-Tunnel, Transport -> UDP_Transport */
			  b->encmode += udp_diff;
			  b->udp_encap = 1;
			}
#endif

			q = setph2proposal0(iph2, a, b);
			if (q == NULL) {
				VPTRINIT(iph2->sa);
				return -1;
			}

			iph2->sa = vrealloc(iph2->sa, iph2->sa->l + q->l);
			if (iph2->sa == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to allocate my sa buffer\n");
				if (q)
					vfree(q);
				return -1;
			}
			memcpy(iph2->sa->v + iph2->sa->l - q->l, q->v, q->l);
			if (propoff != 0) {
				prop = (struct isakmp_pl_p *)(iph2->sa->v +
					propoff);
				prop->h.np = ISAKMP_NPTYPE_P;
			}
			propoff = iph2->sa->l - q->l;

			vfree(q);
		}
	}

	return 0;
}

/*
 * return 1 if all of the given protocols are transport mode.
 */
int
ipsecdoi_transportmode(pp)
	struct saprop *pp;
{
	struct saproto *pr = NULL;

	for (; pp; pp = pp->next) {
		for (pr = pp->head; pr; pr = pr->next) {
			if (pr->encmode != IPSECDOI_ATTR_ENC_MODE_TRNS)
				return 0;
		}
	}

	return 1;
}

int
ipsecdoi_get_defaultlifetime()
{
	return IPSECDOI_ATTR_SA_LD_SEC_DEFAULT;
}

int
ipsecdoi_checkalgtypes(proto_id, enc, auth, comp)
	int proto_id, enc, auth, comp;
{
#define TMPALGTYPE2STR(n) s_algtype(algclass_ipsec_##n, n)
	switch (proto_id) {
	case IPSECDOI_PROTO_IPSEC_ESP:
		if (enc == 0 || comp != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"illegal algorithm defined "
				"ESP enc=%s auth=%s comp=%s.\n",
				TMPALGTYPE2STR(enc),
				TMPALGTYPE2STR(auth),
				TMPALGTYPE2STR(comp));
			return -1;
		}
		break;
	case IPSECDOI_PROTO_IPSEC_AH:
		if (enc != 0 || auth == 0 || comp != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"illegal algorithm defined "
				"AH enc=%s auth=%s comp=%s.\n",
				TMPALGTYPE2STR(enc),
				TMPALGTYPE2STR(auth),
				TMPALGTYPE2STR(comp));
			return -1;
		}
		break;
	case IPSECDOI_PROTO_IPCOMP:
		if (enc != 0 || auth != 0 || comp == 0) {
			plog(LLV_ERROR, LOCATION, NULL,
				"illegal algorithm defined "
				"IPcomp enc=%s auth=%s comp=%s.\n",
				TMPALGTYPE2STR(enc),
				TMPALGTYPE2STR(auth),
				TMPALGTYPE2STR(comp));
			return -1;
		}
		break;
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid ipsec protocol %d\n", proto_id);
		return -1;
	}
#undef TMPALGTYPE2STR
	return 0;
}

int
ipproto2doi(proto)
	int proto;
{
	switch (proto) {
	case IPPROTO_AH:
		return IPSECDOI_PROTO_IPSEC_AH;
	case IPPROTO_ESP:
		return IPSECDOI_PROTO_IPSEC_ESP;
	case IPPROTO_IPCOMP:
		return IPSECDOI_PROTO_IPCOMP;
	}
	return -1;	/* XXX */
}

int
doi2ipproto(proto)
	int proto;
{
	switch (proto) {
	case IPSECDOI_PROTO_IPSEC_AH:
		return IPPROTO_AH;
	case IPSECDOI_PROTO_IPSEC_ESP:
		return IPPROTO_ESP;
	case IPSECDOI_PROTO_IPCOMP:
		return IPPROTO_IPCOMP;
	}
	return -1;	/* XXX */
}

/*
 * Check if a subnet id is valid for comparison
 * with an address id ( address length mask )
 * and compare them
 * Return value
 * =  0 for match
 * =  1 for mismatch
 */

int
ipsecdoi_subnetisaddr_v4( subnet, address )
	const vchar_t *subnet;
	const vchar_t *address;
{
	struct in_addr *mask;

	if (address->l != sizeof(struct in_addr))
		return 1;

	if (subnet->l != (sizeof(struct in_addr)*2))
		return 1;

	mask = (struct in_addr*)(subnet->v + sizeof(struct in_addr));

	if (mask->s_addr!=0xffffffff)
		return 1;

	return memcmp(subnet->v,address->v,address->l);
}

#ifdef INET6

int
ipsecdoi_subnetisaddr_v6( subnet, address )
	const vchar_t *subnet;
	const vchar_t *address;
{
	struct in6_addr *mask;
	int i;

	if (address->l != sizeof(struct in6_addr))
		return 1;

	if (subnet->l != (sizeof(struct in6_addr)*2))
		return 1;

	mask = (struct in6_addr*)(subnet->v + sizeof(struct in6_addr));

	for (i=0; i<16; i++)
		if(mask->s6_addr[i]!=0xff)
			return 1;

	return memcmp(subnet->v,address->v,address->l);
}

#endif

/*
 * Check and Compare two IDs
 * - specify 0 for exact if wildcards are allowed
 * Return value
 * =  0 for match
 * =  1 for misatch
 * = -1 for integrity error
 */

int
ipsecdoi_chkcmpids( idt, ids, exact )
	const vchar_t *idt; /* id cmp target */
	const vchar_t *ids; /* id cmp source */
	int exact;
{
	struct ipsecdoi_id_b *id_bt;
	struct ipsecdoi_id_b *id_bs;
	vchar_t ident_t;
	vchar_t ident_s;
	int result;

	/* handle wildcard IDs */

	if (idt == NULL || ids == NULL)
	{
		if( !exact )
		{
			plog(LLV_DEBUG, LOCATION, NULL,
				"check and compare ids : values matched (ANONYMOUS)\n" );
			return 0;
		}
		else
		{
			plog(LLV_DEBUG, LOCATION, NULL,
				"check and compare ids : value mismatch (ANONYMOUS)\n" );
			return -1;
		}
	}

	/* make sure the ids are of the same type */

	id_bt = (struct ipsecdoi_id_b *) idt->v;
	id_bs = (struct ipsecdoi_id_b *) ids->v;

	ident_t.v = idt->v + sizeof(*id_bt);
	ident_t.l = idt->l - sizeof(*id_bt);
	ident_s.v = ids->v + sizeof(*id_bs);
	ident_s.l = ids->l - sizeof(*id_bs);

	if (id_bs->type != id_bt->type)
	{
		/*
		 * special exception for comparing
                 * address to subnet id types when
                 * the netmask is address length
                 */

		if ((id_bs->type == IPSECDOI_ID_IPV4_ADDR)&&
		    (id_bt->type == IPSECDOI_ID_IPV4_ADDR_SUBNET)) {
			result = ipsecdoi_subnetisaddr_v4(&ident_t,&ident_s);
			goto cmpid_result;
		}

		if ((id_bs->type == IPSECDOI_ID_IPV4_ADDR_SUBNET)&&
		    (id_bt->type == IPSECDOI_ID_IPV4_ADDR)) {
			result = ipsecdoi_subnetisaddr_v4(&ident_s,&ident_t);
			goto cmpid_result;
		}

#ifdef INET6
		if ((id_bs->type == IPSECDOI_ID_IPV6_ADDR)&&
		    (id_bt->type == IPSECDOI_ID_IPV6_ADDR_SUBNET)) {
			result = ipsecdoi_subnetisaddr_v6(&ident_t,&ident_s);
			goto cmpid_result;
		}

		if ((id_bs->type == IPSECDOI_ID_IPV6_ADDR_SUBNET)&&
		    (id_bt->type == IPSECDOI_ID_IPV6_ADDR)) {
			result = ipsecdoi_subnetisaddr_v6(&ident_s,&ident_t);
			goto cmpid_result;
		}
#endif
		plog(LLV_DEBUG, LOCATION, NULL,
			"check and compare ids : id type mismatch %s != %s\n",
			s_ipsecdoi_ident(id_bs->type),
			s_ipsecdoi_ident(id_bt->type));

		return 1;
	}

	/* compare the ID data. */

	switch (id_bt->type) {
	        case IPSECDOI_ID_DER_ASN1_DN:
        	case IPSECDOI_ID_DER_ASN1_GN:
			/* compare asn1 ids */
			result = eay_cmp_asn1dn(&ident_t, &ident_s);
			goto cmpid_result;

		case IPSECDOI_ID_IPV4_ADDR:
			/* validate lengths */
			if ((ident_t.l != sizeof(struct in_addr))||
			    (ident_s.l != sizeof(struct in_addr)))
				goto cmpid_invalid;
			break;

		case IPSECDOI_ID_IPV4_ADDR_SUBNET:
		case IPSECDOI_ID_IPV4_ADDR_RANGE:
			/* validate lengths */
			if ((ident_t.l != (sizeof(struct in_addr)*2))||
			    (ident_s.l != (sizeof(struct in_addr)*2)))
				goto cmpid_invalid;
			break;

#ifdef INET6
		case IPSECDOI_ID_IPV6_ADDR:
			/* validate lengths */
			if ((ident_t.l != sizeof(struct in6_addr))||
			    (ident_s.l != sizeof(struct in6_addr)))
				goto cmpid_invalid;
			break;

		case IPSECDOI_ID_IPV6_ADDR_SUBNET:
		case IPSECDOI_ID_IPV6_ADDR_RANGE:
			/* validate lengths */
			if ((ident_t.l != (sizeof(struct in6_addr)*2))||
			    (ident_s.l != (sizeof(struct in6_addr)*2)))
				goto cmpid_invalid;
			break;
#endif
		case IPSECDOI_ID_FQDN:
		case IPSECDOI_ID_USER_FQDN:
		case IPSECDOI_ID_KEY_ID:
			break;

		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"Unhandled id type %i specified for comparison\n",
				id_bt->type);
			return -1;
	}

	/* validate matching data and length */
	if (ident_t.l == ident_s.l)
		result = memcmp(ident_t.v,ident_s.v,ident_t.l);
	else
		result = 1;

cmpid_result:

	/* debug level output */
	if(loglevel >= LLV_DEBUG) {
		char *idstrt = ipsecdoi_id2str(idt);
		char *idstrs = ipsecdoi_id2str(ids);

		if (!result)
	 		plog(LLV_DEBUG, LOCATION, NULL,
				"check and compare ids : values matched (%s)\n",
				 s_ipsecdoi_ident(id_bs->type) );
		else
 			plog(LLV_DEBUG, LOCATION, NULL,
				"check and compare ids : value mismatch (%s)\n",
				 s_ipsecdoi_ident(id_bs->type));

		plog(LLV_DEBUG, LOCATION, NULL, "cmpid target: \'%s\'\n", idstrt );
		plog(LLV_DEBUG, LOCATION, NULL, "cmpid source: \'%s\'\n", idstrs );

		racoon_free(idstrs);
		racoon_free(idstrt);
	}

	/* return result */
	if( !result )
		return 0;
	else
		return 1;

cmpid_invalid:

	/* id integrity error */
	plog(LLV_DEBUG, LOCATION, NULL, "check and compare ids : %s integrity error\n",
		s_ipsecdoi_ident(id_bs->type));
	plog(LLV_DEBUG, LOCATION, NULL, "cmpid target: length = \'%zu\'\n", ident_t.l );
	plog(LLV_DEBUG, LOCATION, NULL, "cmpid source: length = \'%zu\'\n", ident_s.l );

	return -1;
}

/*
 * check the following:
 * - In main mode with pre-shared key, only address type can be used.
 * - if proper type for phase 1 ?
 * - if phase 1 ID payload conformed RFC2407 4.6.2.
 *   (proto, port) must be (0, 0), (udp, 500) or (udp, [specified]).
 * - if ID payload sent from peer is equal to the ID expected by me.
 *
 * both of "id" and "id_p" should be ID payload without general header,
 */
int
ipsecdoi_checkid1(iph1)
	struct ph1handle *iph1;
{
	struct ipsecdoi_id_b *id_b;
	struct sockaddr *sa;
	caddr_t sa1, sa2;

	if (iph1->id_p == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid iph1 passed id_p == NULL\n");
		return ISAKMP_INTERNAL_ERROR;
	}
	if (iph1->id_p->l < sizeof(*id_b)) {
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid value passed as \"ident\" (len=%lu)\n",
			(u_long)iph1->id_p->l);
		return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	}

	id_b = (struct ipsecdoi_id_b *)iph1->id_p->v;

	/* In main mode with pre-shared key, only address type can be used. */
	if (iph1->etype == ISAKMP_ETYPE_IDENT &&
	    iph1->approval->authmethod == OAKLEY_ATTR_AUTH_METHOD_PSKEY) {
		 if (id_b->type != IPSECDOI_ID_IPV4_ADDR
		  && id_b->type != IPSECDOI_ID_IPV6_ADDR) {
			plog(LLV_ERROR, LOCATION, NULL,
				"Expecting IP address type in main mode, "
				"but %s.\n", s_ipsecdoi_ident(id_b->type));
			return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
		}
	}

	/* if proper type for phase 1 ? */
	switch (id_b->type) {
	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
	case IPSECDOI_ID_IPV4_ADDR_RANGE:
	case IPSECDOI_ID_IPV6_ADDR_RANGE:
		plog(LLV_WARNING, LOCATION, NULL,
			"such ID type %s is not proper.\n",
			s_ipsecdoi_ident(id_b->type));
		/*FALLTHROUGH*/
	}

	/* if phase 1 ID payload conformed RFC2407 4.6.2. */
	if (id_b->type == IPSECDOI_ID_IPV4_ADDR ||
	    id_b->type == IPSECDOI_ID_IPV6_ADDR) {

		if (id_b->proto_id == 0 && ntohs(id_b->port) != 0) {
			plog(LLV_WARNING, LOCATION, NULL,
				"protocol ID and Port mismatched. "
				"proto_id:%d port:%d\n",
				id_b->proto_id, ntohs(id_b->port));
			/*FALLTHROUGH*/

		} else if (id_b->proto_id == IPPROTO_UDP) {
			/*
			 * copmaring with expecting port.
			 * always permit if port is equal to PORT_ISAKMP
			 */
			if (ntohs(id_b->port) != PORT_ISAKMP) {

				u_int16_t port;

				switch (iph1->remote->sa_family) {
				case AF_INET:
					port = ((struct sockaddr_in *)iph1->remote)->sin_port;
					break;
#ifdef INET6
				case AF_INET6:
					port = ((struct sockaddr_in6 *)iph1->remote)->sin6_port;
					break;
#endif
				default:
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid family: %d\n",
						iph1->remote->sa_family);
					return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
				}
				if (ntohs(id_b->port) != port) {
					plog(LLV_WARNING, LOCATION, NULL,
						"port %d expected, but %d\n",
						port, ntohs(id_b->port));
					/*FALLTHROUGH*/
				}
			}
		}
	}

	/* compare with the ID if specified. */
	if (genlist_next(iph1->rmconf->idvl_p, 0)) {
		vchar_t *ident0 = NULL;
		vchar_t ident;
		struct idspec *id;
		struct genlist_entry *gpb;

		for (id = genlist_next (iph1->rmconf->idvl_p, &gpb); id; id = genlist_next (0, &gpb)) {
			/* check the type of both IDs */
			if (id->idtype != doi2idtype(id_b->type))
				continue;  /* ID type mismatch */
			if (id->id == 0)
				goto matched;

			/* compare defined ID with the ID sent by peer. */
			if (ident0 != NULL)
				vfree(ident0);
			ident0 = getidval(id->idtype, id->id);

			switch (id->idtype) {
			case IDTYPE_ASN1DN:
				ident.v = iph1->id_p->v + sizeof(*id_b);
				ident.l = iph1->id_p->l - sizeof(*id_b);
				if (eay_cmp_asn1dn(ident0, &ident) == 0)
					goto matched;
				break;
			case IDTYPE_ADDRESS:
				sa = (struct sockaddr *)ident0->v;
				sa2 = (caddr_t)(id_b + 1);
				switch (sa->sa_family) {
				case AF_INET:
					if (iph1->id_p->l - sizeof(*id_b) != sizeof(struct in_addr))
						continue;  /* ID value mismatch */
					sa1 = (caddr_t)&((struct sockaddr_in *)sa)->sin_addr;
					if (memcmp(sa1, sa2, sizeof(struct in_addr)) == 0)
						goto matched;
					break;
#ifdef INET6
				case AF_INET6:
					if (iph1->id_p->l - sizeof(*id_b) != sizeof(struct in6_addr))
						continue;  /* ID value mismatch */
					sa1 = (caddr_t)&((struct sockaddr_in6 *)sa)->sin6_addr;
					if (memcmp(sa1, sa2, sizeof(struct in6_addr)) == 0)
						goto matched;
					break;
#endif
				default:
					break;
				}
				break;
			default:
				if (memcmp(ident0->v, id_b + 1, ident0->l) == 0)
					goto matched;
				break;
			}
		}
		if (ident0 != NULL) {
			vfree(ident0);
			ident0 = NULL;
		}
		plog(LLV_WARNING, LOCATION, NULL, "No ID match.\n");
		if (iph1->rmconf->verify_identifier)
			return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
matched: /* ID value match */
		if (ident0 != NULL)
			vfree(ident0);
	}

	return 0;
}

/*
 * create ID payload for phase 1 and set into iph1->id.
 * NOT INCLUDING isakmp general header.
 * see, RFC2407 4.6.2.1
 */
int
ipsecdoi_setid1(iph1)
	struct ph1handle *iph1;
{
	vchar_t *ret = NULL;
	struct ipsecdoi_id_b id_b;
	vchar_t *ident = NULL;
	struct sockaddr *ipid = NULL;

	/* init */
	id_b.proto_id = 0;
	id_b.port = 0;
	ident = NULL;

	switch (iph1->rmconf->idvtype) {
	case IDTYPE_FQDN:
		id_b.type = IPSECDOI_ID_FQDN;
		ident = getidval(iph1->rmconf->idvtype, iph1->rmconf->idv);
		break;
	case IDTYPE_USERFQDN:
		id_b.type = IPSECDOI_ID_USER_FQDN;
		ident = getidval(iph1->rmconf->idvtype, iph1->rmconf->idv);
		break;
	case IDTYPE_KEYID:
		id_b.type = IPSECDOI_ID_KEY_ID;
		ident = getidval(iph1->rmconf->idvtype, iph1->rmconf->idv);
		break;
	case IDTYPE_ASN1DN:
		id_b.type = IPSECDOI_ID_DER_ASN1_DN;
		if (iph1->rmconf->idv) {
			/* XXX it must be encoded to asn1dn. */
			ident = vdup(iph1->rmconf->idv);
		} else {
			if (oakley_getmycert(iph1) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to get own CERT.\n");
				goto err;
			}
			ident = eay_get_x509asn1subjectname(&iph1->cert->cert);
		}
		break;
	case IDTYPE_ADDRESS:
		/*
		 * if the value of the id type was set by the configuration
		 * file, then use it.  otherwise the value is get from local
		 * ip address by using ike negotiation.
		 */
		if (iph1->rmconf->idv)
			ipid = (struct sockaddr *)iph1->rmconf->idv->v;
		/*FALLTHROUGH*/
	default:
	    {
		int l;
		caddr_t p;

		if (ipid == NULL)
			ipid = iph1->local;

		/* use IP address */
		switch (ipid->sa_family) {
		case AF_INET:
			id_b.type = IPSECDOI_ID_IPV4_ADDR;
			l = sizeof(struct in_addr);
			p = (caddr_t)&((struct sockaddr_in *)ipid)->sin_addr;
			break;
#ifdef INET6
		case AF_INET6:
			id_b.type = IPSECDOI_ID_IPV6_ADDR;
			l = sizeof(struct in6_addr);
			p = (caddr_t)&((struct sockaddr_in6 *)ipid)->sin6_addr;
			break;
#endif
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid address family.\n");
			goto err;
		}
		id_b.proto_id = IPPROTO_UDP;
		id_b.port = htons(PORT_ISAKMP);
		ident = vmalloc(l);
		if (!ident) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get ID buffer.\n");
			return 0;
		}
		memcpy(ident->v, p, ident->l);
	    }
	}
	if (!ident) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get ID buffer.\n");
		return 0;
	}

	ret = vmalloc(sizeof(id_b) + ident->l);
	if (ret == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get ID buffer.\n");
		goto err;
	}

	memcpy(ret->v, &id_b, sizeof(id_b));
	memcpy(ret->v + sizeof(id_b), ident->v, ident->l);

	iph1->id = ret;

	plog(LLV_DEBUG, LOCATION, NULL,
		"use ID type of %s\n", s_ipsecdoi_ident(id_b.type));
	if (ident)
		vfree(ident);
	return 0;

err:
	if (ident)
		vfree(ident);
	plog(LLV_ERROR, LOCATION, NULL, "failed get my ID\n");
	return -1;
}

static vchar_t *
getidval(type, val)
	int type;
	vchar_t *val;
{
	vchar_t *new = NULL;

	if (val)
		new = vdup(val);
	else if (lcconf->ident[type])
		new = vdup(lcconf->ident[type]);

	return new;
}

/* it's only called by cfparse.y. */
int
set_identifier(vpp, type, value)
	vchar_t **vpp, *value;
	int type;
{
	return set_identifier_qual(vpp, type, value, IDQUAL_UNSPEC);
}

int
set_identifier_qual(vpp, type, value, qual)
	vchar_t **vpp, *value;
	int type;
	int qual;
{
	vchar_t *new = NULL;

	/* simply return if value is null. */
	if (!value){
		if( type == IDTYPE_FQDN || type == IDTYPE_USERFQDN){
			plog(LLV_ERROR, LOCATION, NULL,
				 "No %s\n", type == IDTYPE_FQDN ? "fqdn":"user fqdn");
			return -1;
		}
		return 0;
	}

	switch (type) {
	case IDTYPE_FQDN:
	case IDTYPE_USERFQDN:
		if(value->l <= 1){
			plog(LLV_ERROR, LOCATION, NULL,
				 "Empty %s\n", type == IDTYPE_FQDN ? "fqdn":"user fqdn");
			return -1;
		}
		/* length is adjusted since QUOTEDSTRING teminates NULL. */
		new = vmalloc(value->l - 1);
		if (new == NULL)
			return -1;
		memcpy(new->v, value->v, new->l);
		break;
	case IDTYPE_KEYID:
		/* 
		 * If no qualifier is specified: IDQUAL_UNSPEC. It means
		 * to use a file for backward compatibility sake. 
		 */
		switch(qual) {
		case IDQUAL_FILE:
		case IDQUAL_UNSPEC: {
			FILE *fp;
			char b[512];
			int tlen, len;

			fp = fopen(value->v, "r");
			if (fp == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"can not open %s\n", value->v);
				return -1;
			}
			tlen = 0;
			while ((len = fread(b, 1, sizeof(b), fp)) != 0) {
				new = vrealloc(new, tlen + len);
				if (!new) {
					fclose(fp);
					return -1;
				}
				memcpy(new->v + tlen, b, len);
				tlen += len;
			}
			break;
		}

		case IDQUAL_TAG:
			new = vmalloc(value->l - 1);
			if (new == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"can not allocate memory");
				return -1;
			}
			memcpy(new->v, value->v, new->l);
			break;

		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"unknown qualifier");
			return -1;
		}
		break;
	
	case IDTYPE_ADDRESS: {
		struct sockaddr *sa;

		/* length is adjusted since QUOTEDSTRING teminates NULL. */
		if (value->l == 0)
			break;

		sa = str2saddr(value->v, NULL);
		if (sa == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"invalid ip address %s\n", value->v);
			return -1;
		}

		new = vmalloc(sysdep_sa_len(sa));
		if (new == NULL) {
			racoon_free(sa);
			return -1;
		}
		memcpy(new->v, sa, new->l);
		racoon_free(sa);
		break;
	}
	case IDTYPE_ASN1DN:
		if (value->v[0] == '~')
			/* Hex-encoded ASN1 strings */
			new = eay_hex2asn1dn(value->v + 1, - 1);
		else
			/* DN encoded strings */
			new = eay_str2asn1dn(value->v, value->l - 1);

		if (new == NULL)
			return -1;

		if (loglevel >= LLV_DEBUG) {
			X509_NAME *xn;
			BIO *bio;
			unsigned char *ptr = (unsigned char *) new->v, *buf;
			size_t len;
			char save;

			xn = d2i_X509_NAME(NULL, (void *)&ptr, new->l);
			bio = BIO_new(BIO_s_mem());
			
			X509_NAME_print_ex(bio, xn, 0, 0);
			len = BIO_get_mem_data(bio, &ptr);
			save = ptr[len];
			ptr[len] = 0;
			plog(LLV_DEBUG, LOCATION, NULL, "Parsed DN: %s\n", ptr);
			ptr[len] = save;
			X509_NAME_free(xn);
			BIO_free(bio);
		}

		break;
	}

	*vpp = new;

	return 0;
}

/*
 * create ID payload for phase 2, and set into iph2->id and id_p.  There are
 * NOT INCLUDING isakmp general header.
 * this function is for initiator.  responder will get to copy from payload.
 * responder ID type is always address type.
 * see, RFC2407 4.6.2.1
 */
int
ipsecdoi_setid2(iph2)
	struct ph2handle *iph2;
{
	struct secpolicy *sp;

	/* check there is phase 2 handler ? */
	sp = getspbyspid(iph2->spid);
	if (sp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"no policy found for spid:%u.\n", iph2->spid);
		return -1;
	}

	iph2->id = ipsecdoi_sockaddr2id((struct sockaddr *)&sp->spidx.src,
					sp->spidx.prefs, sp->spidx.ul_proto);
	if (iph2->id == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get ID for %s\n",
			spidx2str(&sp->spidx));
		return -1;
	}
	plog(LLV_DEBUG, LOCATION, NULL, "use local ID type %s\n",
		s_ipsecdoi_ident(((struct ipsecdoi_id_b *)iph2->id->v)->type));

	/* remote side */
	iph2->id_p = ipsecdoi_sockaddr2id((struct sockaddr *)&sp->spidx.dst,
				sp->spidx.prefd, sp->spidx.ul_proto);
	if (iph2->id_p == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get ID for %s\n",
			spidx2str(&sp->spidx));
		VPTRINIT(iph2->id);
		return -1;
	}
	plog(LLV_DEBUG, LOCATION, NULL,
		"use remote ID type %s\n",
		s_ipsecdoi_ident(((struct ipsecdoi_id_b *)iph2->id_p->v)->type));

	return 0;
}

/*
 * set address type of ID.
 * NOT INCLUDING general header.
 */
vchar_t *
ipsecdoi_sockaddr2id(saddr, prefixlen, ul_proto)
	struct sockaddr *saddr;
	u_int prefixlen;
	u_int ul_proto;
{
	vchar_t *new;
	int type, len1, len2;
	caddr_t sa;
	u_short port;

	/*
	 * Q. When type is SUBNET, is it allowed to be ::1/128.
	 * A. Yes. (consensus at bake-off)
	 */
	switch (saddr->sa_family) {
	case AF_INET:
		len1 = sizeof(struct in_addr);
		if (prefixlen == (sizeof(struct in_addr) << 3)) {
			type = IPSECDOI_ID_IPV4_ADDR;
			len2 = 0;
		} else {
			type = IPSECDOI_ID_IPV4_ADDR_SUBNET;
			len2 = sizeof(struct in_addr);
		}
		sa = (caddr_t)&((struct sockaddr_in *)(saddr))->sin_addr;
		port = ((struct sockaddr_in *)(saddr))->sin_port;
		break;
#ifdef INET6
	case AF_INET6:
		len1 = sizeof(struct in6_addr);
		if (prefixlen == (sizeof(struct in6_addr) << 3)) {
			type = IPSECDOI_ID_IPV6_ADDR;
			len2 = 0;
		} else {
			type = IPSECDOI_ID_IPV6_ADDR_SUBNET;
			len2 = sizeof(struct in6_addr);
		}
		sa = (caddr_t)&((struct sockaddr_in6 *)(saddr))->sin6_addr;
		port = ((struct sockaddr_in6 *)(saddr))->sin6_port;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d.\n", saddr->sa_family);
		return NULL;
	}

	/* get ID buffer */
	new = vmalloc(sizeof(struct ipsecdoi_id_b) + len1 + len2);
	if (new == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get ID buffer.\n");
		return NULL;
	}

	memset(new->v, 0, new->l);

	/* set the part of header. */
	((struct ipsecdoi_id_b *)new->v)->type = type;

	/* set ul_proto and port */
	/*
	 * NOTE: we use both IPSEC_ULPROTO_ANY and IPSEC_PORT_ANY as wild card
	 * because 0 means port number of 0.  Instead of 0, we use IPSEC_*_ANY.
	 */
	((struct ipsecdoi_id_b *)new->v)->proto_id =
		ul_proto == IPSEC_ULPROTO_ANY ? 0 : ul_proto;
	((struct ipsecdoi_id_b *)new->v)->port =
		port == IPSEC_PORT_ANY ? 0 : port;
	memcpy(new->v + sizeof(struct ipsecdoi_id_b), sa, len1);

	/* set address */

	/* set prefix */
	if (len2) {
		u_char *p = (unsigned char *) new->v + 
			sizeof(struct ipsecdoi_id_b) + len1;
		u_int bits = prefixlen;

		while (bits >= 8) {
			*p++ = 0xff;
			bits -= 8;
		}

		if (bits > 0)
			*p = ~((1 << (8 - bits)) - 1);
	}

	return new;
}

vchar_t *
ipsecdoi_sockrange2id(laddr, haddr, ul_proto)
	struct sockaddr *laddr, *haddr;
	u_int ul_proto;
{
	vchar_t *new;
	int type, len1, len2;
	u_short port;

	if (laddr->sa_family != haddr->sa_family) {
	    plog(LLV_ERROR, LOCATION, NULL, "Address family mismatch\n");
	    return NULL;
	}

	switch (laddr->sa_family) {
	case AF_INET:
	    type = IPSECDOI_ID_IPV4_ADDR_RANGE;
	    len1 = sizeof(struct in_addr);
	    len2 = sizeof(struct in_addr);
	    break;
#ifdef INET6
	case AF_INET6:
		type = IPSECDOI_ID_IPV6_ADDR_RANGE;
		len1 = sizeof(struct in6_addr);
		len2 = sizeof(struct in6_addr);
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d.\n", laddr->sa_family);
		return NULL;
	}

	/* get ID buffer */
	new = vmalloc(sizeof(struct ipsecdoi_id_b) + len1 + len2);
	if (new == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get ID buffer.\n");
		return NULL;
	}

	memset(new->v, 0, new->l);
	/* set the part of header. */
	((struct ipsecdoi_id_b *)new->v)->type = type;

	/* set ul_proto and port */
	/*
	 * NOTE: we use both IPSEC_ULPROTO_ANY and IPSEC_PORT_ANY as wild card
	 * because 0 means port number of 0.  Instead of 0, we use IPSEC_*_ANY.
	 */
	((struct ipsecdoi_id_b *)new->v)->proto_id =
		ul_proto == IPSEC_ULPROTO_ANY ? 0 : ul_proto;
	port = ((struct sockaddr_in *)(laddr))->sin_port;
	((struct ipsecdoi_id_b *)new->v)->port =
		port == IPSEC_PORT_ANY ? 0 : port;
	memcpy(new->v + sizeof(struct ipsecdoi_id_b), 
	       (caddr_t)&((struct sockaddr_in *)(laddr))->sin_addr, 
	       len1);
	memcpy(new->v + sizeof(struct ipsecdoi_id_b) + len1, 
	       (caddr_t)&((struct sockaddr_in *)haddr)->sin_addr,
	       len2);
	return new;
}


/*
 * create sockaddr structure from ID payload (buf).
 * buffers (saddr, prefixlen, ul_proto) must be allocated.
 * see, RFC2407 4.6.2.1
 */
int
ipsecdoi_id2sockaddr(buf, saddr, prefixlen, ul_proto)
	vchar_t *buf;
	struct sockaddr *saddr;
	u_int8_t *prefixlen;
	u_int16_t *ul_proto;
{
	struct ipsecdoi_id_b *id_b = (struct ipsecdoi_id_b *)buf->v;
	u_int plen = 0;

	/*
	 * When a ID payload of subnet type with a IP address of full bit
	 * masked, it has to be processed as host address.
	 * e.g. below 2 type are same.
	 *      type = ipv6 subnet, data = 2001::1/128
	 *      type = ipv6 address, data = 2001::1
	 */
	switch (id_b->type) {
	case IPSECDOI_ID_IPV4_ADDR:
	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
#ifndef __linux__
		saddr->sa_len = sizeof(struct sockaddr_in);
#endif
		saddr->sa_family = AF_INET;
		((struct sockaddr_in *)saddr)->sin_port =
			(id_b->port == 0
				? IPSEC_PORT_ANY
				: id_b->port);		/* see sockaddr2id() */
		memcpy(&((struct sockaddr_in *)saddr)->sin_addr,
			buf->v + sizeof(*id_b), sizeof(struct in_addr));
		break;
#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR:
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
#ifndef __linux__
		saddr->sa_len = sizeof(struct sockaddr_in6);
#endif
		saddr->sa_family = AF_INET6;
		((struct sockaddr_in6 *)saddr)->sin6_port =
			(id_b->port == 0
				? IPSEC_PORT_ANY
				: id_b->port);		/* see sockaddr2id() */
		memcpy(&((struct sockaddr_in6 *)saddr)->sin6_addr,
			buf->v + sizeof(*id_b), sizeof(struct in6_addr));
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"unsupported ID type %d\n", id_b->type);
		return ISAKMP_NTYPE_INVALID_ID_INFORMATION;
	}

	/* get prefix length */
	switch (id_b->type) {
	case IPSECDOI_ID_IPV4_ADDR:
		plen = sizeof(struct in_addr) << 3;
		break;
#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR:
		plen = sizeof(struct in6_addr) << 3;
		break;
#endif
	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
#endif
	    {
		u_char *p;
		u_int max;
		int alen = sizeof(struct in_addr);

		switch (id_b->type) {
		case IPSECDOI_ID_IPV4_ADDR_SUBNET:
			alen = sizeof(struct in_addr);
			break;
#ifdef INET6
		case IPSECDOI_ID_IPV6_ADDR_SUBNET:
			alen = sizeof(struct in6_addr);
			break;
#endif
		}

		/* sanity check */
		if (buf->l < alen)
			return ISAKMP_INTERNAL_ERROR;

		/* get subnet mask length */
		plen = 0;
		max = alen <<3;

		p = (unsigned char *) buf->v
			+ sizeof(struct ipsecdoi_id_b)
			+ alen;

		for (; *p == 0xff; p++) {
			plen += 8;
			if (plen >= max)
				break;
		}

		if (plen < max) {
			u_int l = 0;
			u_char b = ~(*p);

			while (b) {
				b >>= 1;
				l++;
			}

			l = 8 - l;
			plen += l;
		}
	    }
		break;
	}

	*prefixlen = plen;
	*ul_proto = id_b->proto_id == 0
				? IPSEC_ULPROTO_ANY
				: id_b->proto_id;	/* see sockaddr2id() */

	return 0;
}

/*
 * make printable string from ID payload except of general header.
 */
char *
ipsecdoi_id2str(id)
	const vchar_t *id;
{
#define BUFLEN 512
	char * ret = NULL;
	int len = 0;
	char *dat;
	static char buf[BUFLEN];
	struct ipsecdoi_id_b *id_b = (struct ipsecdoi_id_b *)id->v;
	struct sockaddr saddr;
	u_int plen = 0;

	switch (id_b->type) {
	case IPSECDOI_ID_IPV4_ADDR:
	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
	case IPSECDOI_ID_IPV4_ADDR_RANGE:

#ifndef __linux__
		saddr.sa_len = sizeof(struct sockaddr_in);
#endif
		saddr.sa_family = AF_INET;
		((struct sockaddr_in *)&saddr)->sin_port = IPSEC_PORT_ANY;
		memcpy(&((struct sockaddr_in *)&saddr)->sin_addr,
			id->v + sizeof(*id_b), sizeof(struct in_addr));
		break;
#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR:
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
	case IPSECDOI_ID_IPV6_ADDR_RANGE:

#ifndef __linux__
		saddr.sa_len = sizeof(struct sockaddr_in6);
#endif
		saddr.sa_family = AF_INET6;
		((struct sockaddr_in6 *)&saddr)->sin6_port = IPSEC_PORT_ANY;
		memcpy(&((struct sockaddr_in6 *)&saddr)->sin6_addr,
			id->v + sizeof(*id_b), sizeof(struct in6_addr));
		break;
#endif
	}

	switch (id_b->type) {
	case IPSECDOI_ID_IPV4_ADDR:
#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR:
#endif
		len = snprintf( buf, BUFLEN, "%s", saddrwop2str(&saddr));
		break;

	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
#endif
	    {
		u_char *p;
		u_int max;
		int alen = sizeof(struct in_addr);

		switch (id_b->type) {
		case IPSECDOI_ID_IPV4_ADDR_SUBNET:
			alen = sizeof(struct in_addr);
			break;
#ifdef INET6
		case IPSECDOI_ID_IPV6_ADDR_SUBNET:
			alen = sizeof(struct in6_addr);
			break;
#endif
		}

		/* sanity check */
		if (id->l < alen) {
			len = 0;
			break;
		}

		/* get subnet mask length */
		plen = 0;
		max = alen <<3;

		p = (unsigned char *) id->v
			+ sizeof(struct ipsecdoi_id_b)
			+ alen;

		for (; *p == 0xff; p++) {
			plen += 8;
			if (plen >= max)
				break;
		}

		if (plen < max) {
			u_int l = 0;
			u_char b = ~(*p);

			while (b) {
				b >>= 1;
				l++;
			}

			l = 8 - l;
			plen += l;
		}

		len = snprintf( buf, BUFLEN, "%s/%i", saddrwop2str(&saddr), plen);
	    }
		break;

	case IPSECDOI_ID_IPV4_ADDR_RANGE:

		len = snprintf( buf, BUFLEN, "%s-", saddrwop2str(&saddr));

#ifndef __linux__
		saddr.sa_len = sizeof(struct sockaddr_in);
#endif
		saddr.sa_family = AF_INET;
		((struct sockaddr_in *)&saddr)->sin_port = IPSEC_PORT_ANY;
		memcpy(&((struct sockaddr_in *)&saddr)->sin_addr,
			id->v + sizeof(*id_b) + sizeof(struct in_addr),
			sizeof(struct in_addr));

		len += snprintf( buf + len, BUFLEN - len, "%s", saddrwop2str(&saddr));

		break;

#ifdef INET6
	case IPSECDOI_ID_IPV6_ADDR_RANGE:

		len = snprintf( buf, BUFLEN, "%s-", saddrwop2str(&saddr));

#ifndef __linux__
		saddr.sa_len = sizeof(struct sockaddr_in6);
#endif
		saddr.sa_family = AF_INET6;
		((struct sockaddr_in6 *)&saddr)->sin6_port = IPSEC_PORT_ANY;
		memcpy(&((struct sockaddr_in6 *)&saddr)->sin6_addr,
			id->v + sizeof(*id_b) + sizeof(struct in6_addr),
			sizeof(struct in6_addr));

		len += snprintf( buf + len, BUFLEN - len, "%s", saddrwop2str(&saddr));

		break;
#endif

	case IPSECDOI_ID_FQDN:
	case IPSECDOI_ID_USER_FQDN:
		len = id->l - sizeof(*id_b);
		if (len > BUFLEN)
			len = BUFLEN;
		memcpy(buf, id->v + sizeof(*id_b), len);
		break;

	case IPSECDOI_ID_DER_ASN1_DN:
	case IPSECDOI_ID_DER_ASN1_GN:
	{
		X509_NAME *xn = NULL;

		dat = id->v + sizeof(*id_b);
		len = id->l - sizeof(*id_b);

		if (d2i_X509_NAME(&xn, (void*) &dat, len) != NULL) {
			BIO *bio = BIO_new(BIO_s_mem());
			X509_NAME_print_ex(bio, xn, 0, 0);
			len = BIO_get_mem_data(bio, &dat);
			if (len > BUFLEN)
				len = BUFLEN;
			memcpy(buf,dat,len);
			BIO_free(bio);
			X509_NAME_free(xn);
		} else {
			plog(LLV_ERROR, LOCATION, NULL,
				"unable to extract asn1dn from id\n");

			len = sprintf(buf, "<ASN1-DN>");
		}

		break;
	}

	/* currently unhandled id types */
	case IPSECDOI_ID_KEY_ID:
		len = sprintf( buf, "<KEY-ID>");
		break;

	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"unknown ID type %d\n", id_b->type);
	}

	if (!len)
		len = sprintf( buf, "<?>");

	ret = racoon_malloc(len+1);
	if (ret != NULL) {
		memcpy(ret,buf,len);
		ret[len]=0;
	}

	return ret;
}

/*
 * set IPsec data attributes into a proposal.
 * NOTE: MUST called per a transform.
 */
int
ipsecdoi_t2satrns(t, pp, pr, tr)
	struct isakmp_pl_t *t;
	struct saprop *pp;
	struct saproto *pr;
	struct satrns *tr;
{
	struct isakmp_data *d, *prev;
	int flag, type;
	int error = -1;
	int life_t;
	int tlen;

	tr->trns_no = t->t_no;
	tr->trns_id = t->t_id;

	tlen = ntohs(t->h.len) - sizeof(*t);
	prev = (struct isakmp_data *)NULL;
	d = (struct isakmp_data *)(t + 1);

	/* default */
	life_t = IPSECDOI_ATTR_SA_LD_TYPE_DEFAULT;
	pp->lifetime = IPSECDOI_ATTR_SA_LD_SEC_DEFAULT;
	pp->lifebyte = 0;
	tr->authtype = IPSECDOI_ATTR_AUTH_NONE;

	while (tlen > 0) {

		type = ntohs(d->type) & ~ISAKMP_GEN_MASK;
		flag = ntohs(d->type) & ISAKMP_GEN_MASK;

		plog(LLV_DEBUG, LOCATION, NULL,
			"type=%s, flag=0x%04x, lorv=%s\n",
			s_ipsecdoi_attr(type), flag,
			s_ipsecdoi_attr_v(type, ntohs(d->lorv)));

		switch (type) {
		case IPSECDOI_ATTR_SA_LD_TYPE:
		{
			int type = ntohs(d->lorv);
			switch (type) {
			case IPSECDOI_ATTR_SA_LD_TYPE_SEC:
			case IPSECDOI_ATTR_SA_LD_TYPE_KB:
				life_t = type;
				break;
			default:
				plog(LLV_WARNING, LOCATION, NULL,
					"invalid life duration type. "
					"use default\n");
				life_t = IPSECDOI_ATTR_SA_LD_TYPE_DEFAULT;
				break;
			}
			break;
		}
		case IPSECDOI_ATTR_SA_LD:
			if (prev == NULL
			 || (ntohs(prev->type) & ~ISAKMP_GEN_MASK) !=
					IPSECDOI_ATTR_SA_LD_TYPE) {
				plog(LLV_ERROR, LOCATION, NULL,
				    "life duration must follow ltype\n");
				break;
			}

		    {
			u_int32_t t;
			vchar_t *ld_buf = NULL;

			if (flag) {
				/* i.e. ISAKMP_GEN_TV */
				ld_buf = vmalloc(sizeof(d->lorv));
				if (ld_buf == NULL) {
					plog(LLV_ERROR, LOCATION, NULL,
					    "failed to get LD buffer.\n");
					goto end;
				}
				memcpy(ld_buf->v, &d->lorv, sizeof(d->lorv));
			} else {
				int len = ntohs(d->lorv);
				/* i.e. ISAKMP_GEN_TLV */
				ld_buf = vmalloc(len);
				if (ld_buf == NULL) {
					plog(LLV_ERROR, LOCATION, NULL,
					    "failed to get LD buffer.\n");
					goto end;
				}
				memcpy(ld_buf->v, d + 1, len);
			}
			switch (life_t) {
			case IPSECDOI_ATTR_SA_LD_TYPE_SEC:
				t = ipsecdoi_set_ld(ld_buf);
				vfree(ld_buf);
				if (t == 0) {
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid life duration.\n");
					goto end;
				}
				/* lifetime must be equal in a proposal. */
				if (pp->lifetime == IPSECDOI_ATTR_SA_LD_SEC_DEFAULT)
					pp->lifetime = t;
				else if (pp->lifetime != t) {
					plog(LLV_ERROR, LOCATION, NULL,
						"lifetime mismatched "
						"in a proposal, "
						"prev:%ld curr:%u.\n",
						(long)pp->lifetime, t);
					goto end;
				}
				break;
			case IPSECDOI_ATTR_SA_LD_TYPE_KB:
				t = ipsecdoi_set_ld(ld_buf);
				vfree(ld_buf);
				if (t == 0) {
					plog(LLV_ERROR, LOCATION, NULL,
						"invalid life duration.\n");
					goto end;
				}
				/* lifebyte must be equal in a proposal. */
				if (pp->lifebyte == 0)
					pp->lifebyte = t;
				else if (pp->lifebyte != t) {
					plog(LLV_ERROR, LOCATION, NULL,
						"lifebyte mismatched "
						"in a proposal, "
						"prev:%d curr:%u.\n",
						pp->lifebyte, t);
					goto end;
				}
				break;
			default:
				vfree(ld_buf);
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid life type: %d\n", life_t);
				goto end;
			}
		    }
			break;

		case IPSECDOI_ATTR_GRP_DESC:
			/*
			 * RFC2407: 4.5 IPSEC Security Association Attributes
			 *   Specifies the Oakley Group to be used in a PFS QM
			 *   negotiation.  For a list of supported values, see
			 *   Appendix A of [IKE].
			 */
			if (pp->pfs_group == 0)
				pp->pfs_group = (u_int16_t)ntohs(d->lorv);
			else if (pp->pfs_group != (u_int16_t)ntohs(d->lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"pfs_group mismatched "
					"in a proposal.\n");
				goto end;
			}
			break;

		case IPSECDOI_ATTR_ENC_MODE:
			if (pr->encmode &&
			    pr->encmode != (u_int16_t)ntohs(d->lorv)) {
				plog(LLV_ERROR, LOCATION, NULL,
					"multiple encmode exist "
					"in a transform.\n");
				goto end;
			}
			pr->encmode = (u_int16_t)ntohs(d->lorv);
			break;

		case IPSECDOI_ATTR_AUTH:
			if (tr->authtype != IPSECDOI_ATTR_AUTH_NONE) {
				plog(LLV_ERROR, LOCATION, NULL,
					"multiple authtype exist "
					"in a transform.\n");
				goto end;
			}
			tr->authtype = (u_int16_t)ntohs(d->lorv);
			break;

		case IPSECDOI_ATTR_KEY_LENGTH:
			if (pr->proto_id != IPSECDOI_PROTO_IPSEC_ESP) {
				plog(LLV_ERROR, LOCATION, NULL,
					"key length defined but not ESP");
				goto end;
			}
			tr->encklen = ntohs(d->lorv);
			break;
#ifdef HAVE_SECCTX
		case IPSECDOI_ATTR_SECCTX:
		{
			int len = ntohs(d->lorv);
			memcpy(&pp->sctx, d + 1, len);
			break;
		}
#endif /* HAVE_SECCTX */
		case IPSECDOI_ATTR_KEY_ROUNDS:
		case IPSECDOI_ATTR_COMP_DICT_SIZE:
		case IPSECDOI_ATTR_COMP_PRIVALG:
		default:
			break;
		}

		prev = d;
		if (flag) {
			tlen -= sizeof(*d);
			d = (struct isakmp_data *)((char *)d + sizeof(*d));
		} else {
			tlen -= (sizeof(*d) + ntohs(d->lorv));
			d = (struct isakmp_data *)((caddr_t)d + sizeof(*d) + ntohs(d->lorv));
		}
	}

	error = 0;
end:
	return error;
}

int
ipsecdoi_authalg2trnsid(alg)
	int alg;
{
	switch (alg) {
        case IPSECDOI_ATTR_AUTH_HMAC_MD5:
		return IPSECDOI_AH_MD5;
        case IPSECDOI_ATTR_AUTH_HMAC_SHA1:
		return IPSECDOI_AH_SHA;
	case IPSECDOI_ATTR_AUTH_HMAC_SHA2_256:
		return IPSECDOI_AH_SHA256;
	case IPSECDOI_ATTR_AUTH_HMAC_SHA2_384:
		return IPSECDOI_AH_SHA384;
	case IPSECDOI_ATTR_AUTH_HMAC_SHA2_512:
		return IPSECDOI_AH_SHA512;
        case IPSECDOI_ATTR_AUTH_DES_MAC:
		return IPSECDOI_AH_DES;
	case IPSECDOI_ATTR_AUTH_KPDK:
		return IPSECDOI_AH_MD5;	/* XXX */
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid authentication algorithm:%d\n", alg);
	}
	return -1;
}

#ifdef HAVE_GSSAPI
struct isakmpsa *
fixup_initiator_sa(match, received)
	struct isakmpsa *match, *received;
{
	if (received->gssid != NULL)
		match->gssid = vdup(received->gssid);

	return match;
}
#endif

static int rm_idtype2doi[] = {
	255,				/* IDTYPE_UNDEFINED, 0 */
	IPSECDOI_ID_FQDN,		/* IDTYPE_FQDN, 1 */
	IPSECDOI_ID_USER_FQDN,		/* IDTYPE_USERFQDN, 2 */
	IPSECDOI_ID_KEY_ID,		/* IDTYPE_KEYID, 3 */
	255,    /*			   IDTYPE_ADDRESS, 4 
		 * it expands into 4 types by another function. */
	IPSECDOI_ID_DER_ASN1_DN,	/* IDTYPE_ASN1DN, 5 */
};

/*
 * convert idtype to DOI value.
 * OUT	255  : NG
 *	other: converted.
 */
int
idtype2doi(idtype)
	int idtype;
{
	if (ARRAYLEN(rm_idtype2doi) > idtype)
		return rm_idtype2doi[idtype];
	return 255;
}

int
doi2idtype(doi)
	int doi;
{
	switch(doi) {
	case IPSECDOI_ID_FQDN:
		return(IDTYPE_FQDN);
	case IPSECDOI_ID_USER_FQDN:
		return(IDTYPE_USERFQDN);
	case IPSECDOI_ID_KEY_ID:
		return(IDTYPE_KEYID);
	case IPSECDOI_ID_DER_ASN1_DN:
		return(IDTYPE_ASN1DN);
	case IPSECDOI_ID_IPV4_ADDR:
	case IPSECDOI_ID_IPV4_ADDR_SUBNET:
	case IPSECDOI_ID_IPV6_ADDR:
	case IPSECDOI_ID_IPV6_ADDR_SUBNET:
		return(IDTYPE_ADDRESS);
	default:
		plog(LLV_WARNING, LOCATION, NULL,
			"Inproper idtype:%s in this function.\n",
			s_ipsecdoi_ident(doi));
		return(IDTYPE_ADDRESS);	/* XXX */
	}
	/*NOTREACHED*/
}

#ifdef ENABLE_HYBRID
static int
switch_authmethod(authmethod)
	int authmethod;
{
	switch(authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I;
		break;
	/* Those are not implemented */
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I;
		break;
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_R:
		authmethod = OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I;
		break;
	default:
		break;
	}

	return authmethod;
}
#endif
