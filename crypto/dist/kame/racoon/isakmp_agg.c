/*	$KAME: isakmp_agg.c,v 1.59 2004/03/27 03:27:46 suz Exp $	*/

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

/* Aggressive Exchange (Aggressive Mode) */

#include <sys/cdefs.h>
__RCSID("$NetBSD: isakmp_agg.c,v 1.2.2.1.2.1 2006/01/19 21:39:38 tron Exp $");

#include <sys/types.h>
#include <sys/param.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
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
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "schedule.h"
#include "debug.h"

#include "localconf.h"
#include "remoteconf.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "oakley.h"
#include "handler.h"
#include "ipsec_doi.h"
#include "crypto_openssl.h"
#include "pfkey.h"
#include "isakmp_agg.h"
#include "isakmp_inf.h"
#include "vendorid.h"
#include "strnames.h"

#ifdef HAVE_GSSAPI
#include "auth_gssapi.h"
#endif

/*
 * begin Aggressive Mode as initiator.
 */
/*
 * send to responder
 * 	psk: HDR, SA, KE, Ni, IDi1
 * 	sig: HDR, SA, KE, Ni, IDi1 [, CR ]
 *   gssapi: HDR, SA, KE, Ni, IDi1, GSSi
 * 	rsa: HDR, SA, [ HASH(1),] KE, <IDi1_b>Pubkey_r, <Ni_b>Pubkey_r
 * 	rev: HDR, SA, [ HASH(1),] <Ni_b>Pubkey_r, <KE_b>Ke_i,
 * 	     <IDii_b>Ke_i [, <Cert-I_b>Ke_i ]
 */
int
agg_i1send(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg; /* must be null */
{
	struct isakmp_gen *gen;
	struct isakmp_construct p;
	int tlen;
	int need_cr = 0;
	vchar_t *cr = NULL, *gsstoken = NULL;
	int error = -1;
#ifdef HAVE_GSSAPI
	int len;
#endif

	/* validity check */
	if (msg != NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"msg has to be NULL in this function.\n");
		goto end;
	}
	if (iph1->status != PHASE1ST_START) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* create isakmp index */
	memset(&iph1->index, 0, sizeof(iph1->index));
	isakmp_newcookie((caddr_t)&iph1->index, iph1->remote, iph1->local);

	/* make ID payload into isakmp status */
	if (ipsecdoi_setid1(iph1) < 0)
		goto end;

	/* create SA payload for my proposal */
	iph1->sa = ipsecdoi_setph1proposal(iph1->rmconf->proposal);
	if (iph1->sa == NULL)
		goto end;

	/* consistency check of proposals */
	if (iph1->rmconf->dhgrp == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"configuration failure about DH group.\n");
		goto end;
	}

	/* generate DH public value */
	if (oakley_dh_generate(iph1->rmconf->dhgrp,
				&iph1->dhpub, &iph1->dhpriv) < 0)
		goto end;

	/* generate NONCE value */
	iph1->nonce = eay_set_random(iph1->rmconf->nonce_size);
	if (iph1->nonce == NULL)
		goto end;

#ifdef HAVE_SIGNING_C
	/* create CR if need */
	if (iph1->rmconf->send_cr
	 && oakley_needcr(iph1->rmconf->proposal->authmethod)
	 && iph1->rmconf->peerscertfile == NULL) {
		need_cr = 1;
		cr = oakley_getcr(iph1);
		if (cr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get cr buffer.\n");
			goto end;
		}
	}
#endif
	plog(LLV_DEBUG, LOCATION, NULL, "authmethod is %s\n",
		s_oakley_attr_method(iph1->rmconf->proposal->authmethod));
	/* create buffer to send isakmp payload */
	tlen = sizeof(struct isakmp)
		+ sizeof(*gen) + iph1->sa->l
		+ sizeof(*gen) + iph1->dhpub->l
		+ sizeof(*gen) + iph1->nonce->l
		+ sizeof(*gen) + iph1->id->l;
	if (need_cr)
		tlen += sizeof(*gen) + cr->l;
#ifdef HAVE_GSSAPI
	if (iph1->rmconf->proposal->authmethod ==
	    OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		gssapi_get_itoken(iph1, &len);
		tlen += sizeof (*gen) + len;
	}
#endif

	iph1->sendbuf = vmalloc(tlen);
	if (iph1->sendbuf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		goto end;
	}

	/* set isakmp header */
	p = set_isakmp_header(iph1->sendbuf, iph1);
	if (p.buff == NULL)
		goto end;

	/* set SA payload to propose */
	p = set_isakmp_payload_c(p, iph1->sa, ISAKMP_NPTYPE_SA);

	/* create isakmp KE payload */
	p = set_isakmp_payload_c(p, iph1->dhpub, ISAKMP_NPTYPE_KE);

	/* create isakmp NONCE payload */
	p = set_isakmp_payload_c(p, iph1->nonce, ISAKMP_NPTYPE_NONCE);

	/* create isakmp ID payload */
	p = set_isakmp_payload_c(p, iph1->id, ISAKMP_NPTYPE_ID);

#ifdef HAVE_GSSAPI
	if (iph1->rmconf->proposal->authmethod ==
	    OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		gssapi_get_token_to_send(iph1, &gsstoken);
		p = set_isakmp_payload_c(p, gsstoken, ISAKMP_NPTYPE_GSS);
	} else
#endif
	if (need_cr)
		/* create isakmp CR payload */
		p = set_isakmp_payload_c(p, cr, ISAKMP_NPTYPE_CR);

#ifdef HAVE_PRINT_ISAKMP_C
	isakmp_printpacket(iph1->sendbuf, iph1->local, iph1->remote, 0);
#endif

	/* send the packet, add to the schedule to resend */
	iph1->retry_counter = iph1->rmconf->retry_counter;
	if (isakmp_ph1resend(iph1) == -1)
		goto end;

	iph1->status = PHASE1ST_MSG1SENT;

	error = 0;

end:
	if (cr)
		vfree(cr);
	if (gsstoken)
		vfree(gsstoken);

	return error;
}

/*
 * receive from responder
 * 	psk: HDR, SA, KE, Nr, IDr1, HASH_R
 * 	sig: HDR, SA, KE, Nr, IDr1, [ CR, ] [ CERT, ] SIG_R
 *   gssapi: HDR, SA, KE, Nr, IDr1, GSSr, HASH_R
 * 	rsa: HDR, SA, KE, <IDr1_b>PubKey_i, <Nr_b>PubKey_i, HASH_R
 * 	rev: HDR, SA, <Nr_b>PubKey_i, <KE_b>Ke_r, <IDir_b>Ke_r, HASH_R
 */
int
agg_i2recv(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	vchar_t *pbuf = NULL;
	struct isakmp_parse_t *pa;
	vchar_t *satmp = NULL;
	int error = -1;
#ifdef HAVE_GSSAPI
	vchar_t *gsstoken = NULL;
#endif

	/* validity check */
	if (iph1->status != PHASE1ST_MSG1SENT) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* validate the type of next payload */
	pbuf = isakmp_parse(msg);
	if (pbuf == NULL)
		goto end;
	pa = (struct isakmp_parse_t *)pbuf->v;

	iph1->pl_hash = NULL;

	/* SA payload is fixed postion */
	if (pa->type != ISAKMP_NPTYPE_SA) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"received invalid next payload type %d, "
			"expecting %d.\n",
			pa->type, ISAKMP_NPTYPE_SA);
		goto end;
	}
	if (isakmp_p2ph(&satmp, pa->ptr) < 0)
		goto end;
	pa++;

	for (/*nothing*/;
	     pa->type != ISAKMP_NPTYPE_NONE;
	     pa++) {

		switch (pa->type) {
		case ISAKMP_NPTYPE_KE:
			if (isakmp_p2ph(&iph1->dhpub_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_NONCE:
			if (isakmp_p2ph(&iph1->nonce_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_ID:
			if (isakmp_p2ph(&iph1->id_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_HASH:
			iph1->pl_hash = (struct isakmp_pl_hash *)pa->ptr;
			break;
#ifdef HAVE_SIGNING_C
		case ISAKMP_NPTYPE_CR:
			if (oakley_savecr(iph1, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_CERT:
			if (oakley_savecert(iph1, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_SIG:
			if (isakmp_p2ph(&iph1->sig_p, pa->ptr) < 0)
				goto end;
			break;
#endif
		case ISAKMP_NPTYPE_VID:
			(void)check_vendorid(pa->ptr);
			break;
		case ISAKMP_NPTYPE_N:
			isakmp_check_notify(pa->ptr, iph1);
			break;
#ifdef HAVE_GSSAPI
		case ISAKMP_NPTYPE_GSS:
			if (isakmp_p2ph(&gsstoken, pa->ptr) < 0)
				goto end;
			gssapi_save_received_token(iph1, gsstoken);
			break;
#endif
		default:
			/* don't send information, see isakmp_ident_r1() */
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"ignore the packet, "
				"received unexpecting payload type %d.\n",
				pa->type);
			goto end;
		}
	}

	/* payload existency check */
	if (iph1->dhpub_p == NULL || iph1->nonce_p == NULL) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"few isakmp message received.\n");
		goto end;
	}	

	/* verify identifier */
	if (ipsecdoi_checkid1(iph1) != 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"invalid ID payload.\n");
		goto end;
	}

	/* check SA payload and set approval SA for use */
	if (ipsecdoi_checkph1proposal(satmp, iph1) < 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to get valid proposal.\n");
		/* XXX send information */
		goto end;
	}
	VPTRINIT(iph1->sa_ret);

	/* fix isakmp index */
	memcpy(&iph1->index.r_ck, &((struct isakmp *)msg->v)->r_ck,
		sizeof(cookie_t));

	/* compute sharing secret of DH */
	if (oakley_dh_compute(iph1->rmconf->dhgrp, iph1->dhpub,
				iph1->dhpriv, iph1->dhpub_p, &iph1->dhgxy) < 0)
		goto end;

	/* generate SKEYIDs & IV & final cipher key */
	if (oakley_skeyid(iph1) < 0)
		goto end;
	if (oakley_skeyid_dae(iph1) < 0)
		goto end;
	if (oakley_compute_enckey(iph1) < 0)
		goto end;
	if (oakley_newiv(iph1) < 0)
		goto end;

	/* validate authentication value */
    {
	int type;
	type = oakley_validate_auth(iph1);
	if (type != 0) {
		if (type == -1) {
			/* message printed inner oakley_validate_auth() */
			goto end;
		}
		isakmp_info_send_n1(iph1, type, NULL);
		goto end;
	}
    }

#ifdef HAVE_SIGNING_C
	if (oakley_checkcr(iph1) < 0) {
		/* Ignore this error in order to be interoperability. */
		;
	}
#endif

	/* change status of isakmp status entry */
	iph1->status = PHASE1ST_MSG2RECEIVED;

	error = 0;

end:
	if (pbuf)
		vfree(pbuf);
	if (satmp)
		vfree(satmp);
	if (error) {
		VPTRINIT(iph1->dhpub_p);
		VPTRINIT(iph1->nonce_p);
		VPTRINIT(iph1->id_p);
		oakley_delcert(iph1->cert_p);
		iph1->cert_p = NULL;
		oakley_delcert(iph1->crl_p);
		iph1->crl_p = NULL;
		VPTRINIT(iph1->sig_p);
		oakley_delcert(iph1->cr_p);
		iph1->cr_p = NULL;
	}

	return error;
}

/*
 * send to responder
 * 	psk: HDR, HASH_I
 *   gssapi: HDR, HASH_I
 * 	sig: HDR, [ CERT, ] SIG_I
 * 	rsa: HDR, HASH_I
 * 	rev: HDR, HASH_I
 */
int
agg_i2send(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	struct isakmp_gen *gen;
	struct isakmp_construct p;
	int tlen;
	int need_cert = 0;
	int error = -1;
	vchar_t *gsshash = NULL;

	/* validity check */
	if (iph1->status != PHASE1ST_MSG2RECEIVED) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* generate HASH to send */
	plog(LLV_DEBUG, LOCATION, NULL, "generate HASH_I\n");
	iph1->hash = oakley_ph1hash_common(iph1, GENERATE);
	if (iph1->hash == NULL) {
#ifdef HAVE_GSSAPI
		if (gssapi_more_tokens(iph1))
			isakmp_info_send_n1(iph1,
			    ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE, NULL);
#endif
		goto end;
	}

	tlen = sizeof(struct isakmp);

	switch (iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
		tlen += sizeof(*gen) + iph1->hash->l;

		iph1->sendbuf = vmalloc(tlen);
		if (iph1->sendbuf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer to send.\n");
			goto end;
		}

		/* set isakmp header */
		p = set_isakmp_header(iph1->sendbuf, iph1);
		if (p.buff == NULL)
			goto end;

		/* set HASH payload */
		p = set_isakmp_payload_c(p, iph1->hash, ISAKMP_NPTYPE_HASH);
		break;
#ifdef HAVE_SIGNING_C
	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
		/* XXX if there is CR or not ? */

		if (oakley_getmycert(iph1) < 0)
			goto end;

		if (oakley_getsign(iph1) < 0)
			goto end;

		if (iph1->cert != NULL && iph1->rmconf->send_cert)
			need_cert = 1;

		tlen += sizeof(*gen) + iph1->sig->l;
		if (need_cert)
			tlen += sizeof(*gen) + iph1->cert->pl->l;

		iph1->sendbuf = vmalloc(tlen);
		if (iph1->sendbuf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer to send.\n");
			goto end;
		}

		/* set isakmp header */
		p = set_isakmp_header(iph1->sendbuf, iph1);
		if (p.buff == NULL)
			goto end;

		/* add CERT payload if there */
		if (need_cert)
			p = set_isakmp_payload_c(p, iph1->cert->pl, ISAKMP_NPTYPE_CERT);
		/* add SIG payload */
		p = set_isakmp_payload_c(p, iph1->sig, ISAKMP_NPTYPE_SIG);
		break;
#endif
	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
		tlen += sizeof(*gen) + iph1->hash->l;
		break;
#ifdef HAVE_GSSAPI
	case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
		gsshash = gssapi_wraphash(iph1);
		if (gsshash == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to wrap hash\n");
			isakmp_info_send_n1(iph1,
				ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE, NULL);
			goto end;
		}
		tlen += sizeof(*gen) + gsshash->l;

		iph1->sendbuf = vmalloc(tlen);
		if (iph1->sendbuf == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer to send.\n");
			goto end;
		}
		/* set isakmp header */
		p = set_isakmp_header(iph1->sendbuf, iph1);
		p = set_isakmp_payload_c(p, gsshash, ISAKMP_NPTYPE_HASH);
		break;
#endif
	}

#ifdef HAVE_PRINT_ISAKMP_C
	isakmp_printpacket(iph1->sendbuf, iph1->local, iph1->remote, 0);
#endif

	/* send to responder */
	if (isakmp_send(iph1, iph1->sendbuf) < 0)
		goto end;

	/* the sending message is added to the received-list. */
	if (add_recvdpkt(iph1->remote, iph1->local, iph1->sendbuf, msg) == -1) {
		plog(LLV_ERROR , LOCATION, NULL,
			"failed to add a response packet to the tree.\n");
		goto end;
	}

	/* set encryption flag */
	iph1->flags |= ISAKMP_FLAG_E;

	iph1->status = PHASE1ST_ESTABLISHED;

	error = 0;

end:
	if (gsshash)
		vfree(gsshash);
	return error;
}

/*
 * receive from initiator
 * 	psk: HDR, SA, KE, Ni, IDi1
 * 	sig: HDR, SA, KE, Ni, IDi1 [, CR ]
 *   gssapi: HDR, SA, KE, Ni, IDi1 , GSSi
 * 	rsa: HDR, SA, [ HASH(1),] KE, <IDi1_b>Pubkey_r, <Ni_b>Pubkey_r
 * 	rev: HDR, SA, [ HASH(1),] <Ni_b>Pubkey_r, <KE_b>Ke_i,
 * 	     <IDii_b>Ke_i [, <Cert-I_b>Ke_i ]
 */
int
agg_r1recv(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	int error = -1;
	vchar_t *pbuf = NULL;
	struct isakmp_parse_t *pa;
#ifdef HAVE_GSSAPI
	vchar_t *gsstoken = NULL;
#endif

	/* validity check */
	if (iph1->status != PHASE1ST_START) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* validate the type of next payload */
	pbuf = isakmp_parse(msg);
	if (pbuf == NULL)
		goto end;
	pa = (struct isakmp_parse_t *)pbuf->v;

	/* SA payload is fixed postion */
	if (pa->type != ISAKMP_NPTYPE_SA) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"received invalid next payload type %d, "
			"expecting %d.\n",
			pa->type, ISAKMP_NPTYPE_SA);
		goto end;
	}
	if (isakmp_p2ph(&iph1->sa, pa->ptr) < 0)
		goto end;
	pa++;

	for (/*nothing*/;
	     pa->type != ISAKMP_NPTYPE_NONE;
	     pa++) {

		plog(LLV_DEBUG, LOCATION, NULL,
			"received payload of type %s\n",
			s_isakmp_nptype(pa->type));

		switch (pa->type) {
		case ISAKMP_NPTYPE_KE:
			if (isakmp_p2ph(&iph1->dhpub_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_NONCE:
			if (isakmp_p2ph(&iph1->nonce_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_ID:
			if (isakmp_p2ph(&iph1->id_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_VID:
			(void)check_vendorid(pa->ptr);
			break;
#ifdef HAVE_SIGNING_C
		case ISAKMP_NPTYPE_CR:
			if (oakley_savecr(iph1, pa->ptr) < 0)
				goto end;
			break;
#endif
#ifdef HAVE_GSSAPI
		case ISAKMP_NPTYPE_GSS:
			if (isakmp_p2ph(&gsstoken, pa->ptr) < 0)
				goto end;
			gssapi_save_received_token(iph1, gsstoken);
			break;
#endif
		default:
			/* don't send information, see isakmp_ident_r1() */
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"ignore the packet, "
				"received unexpecting payload type %d.\n",
				pa->type);
			goto end;
		}
	}

	/* payload existency check */
	if (iph1->dhpub_p == NULL || iph1->nonce_p == NULL) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"few isakmp message received.\n");
		goto end;
	}

	/* verify identifier */
	if (ipsecdoi_checkid1(iph1) != 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"invalid ID payload.\n");
		goto end;
	}

	/* check SA payload and set approval SA for use */
	if (ipsecdoi_checkph1proposal(iph1->sa, iph1) < 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to get valid proposal.\n");
		/* XXX send information */
		goto end;
	}

#ifdef HAVE_SIGNING_C
	if (oakley_checkcr(iph1) < 0) {
		/* Ignore this error in order to be interoperability. */
		;
	}
#endif

	iph1->status = PHASE1ST_MSG1RECEIVED;

	error = 0;

end:
	if (pbuf)
		vfree(pbuf);
	if (error) {
		VPTRINIT(iph1->sa);
		VPTRINIT(iph1->dhpub_p);
		VPTRINIT(iph1->nonce_p);
		VPTRINIT(iph1->id_p);
		oakley_delcert(iph1->cr_p);
		iph1->cr_p = NULL;
	}

	return error;
}

/*
 * send to initiator
 * 	psk: HDR, SA, KE, Nr, IDr1, HASH_R
 * 	sig: HDR, SA, KE, Nr, IDr1, [ CR, ] [ CERT, ] SIG_R
 *   gssapi: HDR, SA, KE, Nr, IDr1, GSSr, HASH_R
 * 	rsa: HDR, SA, KE, <IDr1_b>PubKey_i, <Nr_b>PubKey_i, HASH_R
 * 	rev: HDR, SA, <Nr_b>PubKey_i, <KE_b>Ke_r, <IDir_b>Ke_r, HASH_R
 */
int
agg_r1send(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	struct isakmp_gen *gen;
	struct isakmp_construct p;
	int tlen;
	int need_cr = 0;
	int need_cert = 0;
	vchar_t *cr = NULL;
	vchar_t *vid = NULL;
	int error = -1;
#ifdef HAVE_GSSAPI
	int gsslen;
	vchar_t *gsstoken = NULL, *gsshash = NULL;
	vchar_t *gss_sa = NULL;
#endif

	/* validity check */
	if (iph1->status != PHASE1ST_MSG1RECEIVED) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* set responder's cookie */
	isakmp_newcookie((caddr_t)&iph1->index.r_ck, iph1->remote, iph1->local);

	/* make ID payload into isakmp status */
	if (ipsecdoi_setid1(iph1) < 0)
		goto end;

	/* generate DH public value */
	if (oakley_dh_generate(iph1->rmconf->dhgrp,
				&iph1->dhpub, &iph1->dhpriv) < 0)
		goto end;

	/* generate NONCE value */
	iph1->nonce = eay_set_random(iph1->rmconf->nonce_size);
	if (iph1->nonce == NULL)
		goto end;

	/* compute sharing secret of DH */
	if (oakley_dh_compute(iph1->approval->dhgrp, iph1->dhpub,
				iph1->dhpriv, iph1->dhpub_p, &iph1->dhgxy) < 0)
		goto end;

	/* generate SKEYIDs & IV & final cipher key */
	if (oakley_skeyid(iph1) < 0)
		goto end;
	if (oakley_skeyid_dae(iph1) < 0)
		goto end;
	if (oakley_compute_enckey(iph1) < 0)
		goto end;
	if (oakley_newiv(iph1) < 0)
		goto end;

#ifdef HAVE_GSSAPI
	if (iph1->rmconf->proposal->authmethod ==	
	    OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB)
		gssapi_get_rtoken(iph1, &gsslen);
#endif

	/* generate HASH to send */
	plog(LLV_DEBUG, LOCATION, NULL, "generate HASH_R\n");
	iph1->hash = oakley_ph1hash_common(iph1, GENERATE);
	if (iph1->hash == NULL) {
#ifdef HAVE_GSSAPI
		if (gssapi_more_tokens(iph1))
			isakmp_info_send_n1(iph1,
			    ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE, NULL);
#endif
		goto end;
	}

#ifdef HAVE_SIGNING_C
	/* create CR if need */
	if (iph1->rmconf->send_cr
	 && oakley_needcr(iph1->approval->authmethod)
	 && iph1->rmconf->peerscertfile == NULL) {
		need_cr = 1;
		cr = oakley_getcr(iph1);
		if (cr == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get cr buffer.\n");
			goto end;
		}
	}
#endif

	tlen = sizeof(struct isakmp);

	switch (iph1->approval->authmethod) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
		/* create buffer to send isakmp payload */
		tlen += sizeof(*gen) + iph1->sa_ret->l
			+ sizeof(*gen) + iph1->dhpub->l
			+ sizeof(*gen) + iph1->nonce->l
			+ sizeof(*gen) + iph1->id->l
			+ sizeof(*gen) + iph1->hash->l;
		if ((vid = set_vendorid(iph1->approval->vendorid)) != NULL)
			tlen += sizeof(*gen) + vid->l;
		if (need_cr)
			tlen += sizeof(*gen) + cr->l;

		iph1->sendbuf = vmalloc(tlen);
		if (iph1->sendbuf == NULL) { 
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer to send\n");
			goto end;
		}

		/* set isakmp header */
		p = set_isakmp_header(iph1->sendbuf, iph1);
		if (p.buff == NULL)
			goto end;

		/* set SA payload to reply */
		p = set_isakmp_payload_c(p, iph1->sa_ret, ISAKMP_NPTYPE_SA);

		/* create isakmp KE payload */
		p = set_isakmp_payload_c(p, iph1->dhpub, ISAKMP_NPTYPE_KE);

		/* create isakmp NONCE payload */
		p = set_isakmp_payload_c(p, iph1->nonce, ISAKMP_NPTYPE_NONCE);

		/* create isakmp ID payload */
		p = set_isakmp_payload_c(p, iph1->id, ISAKMP_NPTYPE_ID);

		/* create isakmp HASH payload */
		p = set_isakmp_payload_c(p, iph1->hash, ISAKMP_NPTYPE_HASH);

		/* append vendor id, if needed */
		if (vid)
			p = set_isakmp_payload_c(p, vid, ISAKMP_NPTYPE_VID);

		/* create isakmp CR payload if needed */
		if (need_cr)
			p = set_isakmp_payload_c(p, cr, ISAKMP_NPTYPE_CR);
		break;
#ifdef HAVE_SIGNING_C
	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
		/* XXX if there is CR or not ? */

		if (oakley_getmycert(iph1) < 0)
			goto end;

		if (oakley_getsign(iph1) < 0)
			goto end;

		if (iph1->cert != NULL && iph1->rmconf->send_cert)
			need_cert = 1;

		tlen += sizeof(*gen) + iph1->sa_ret->l
			+ sizeof(*gen) + iph1->dhpub->l
			+ sizeof(*gen) + iph1->nonce->l
			+ sizeof(*gen) + iph1->id->l
			+ sizeof(*gen) + iph1->sig->l;
		if (need_cert)
			tlen += sizeof(*gen) + iph1->cert->pl->l;
		if ((vid = set_vendorid(iph1->approval->vendorid)) != NULL)
			tlen += sizeof(*gen) + vid->l;
		if (need_cr)
			tlen += sizeof(*gen) + cr->l;

		iph1->sendbuf = vmalloc(tlen);
		if (iph1->sendbuf == NULL) { 
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer to send.\n");
			goto end;
		}

		/* set isakmp header */
		p = set_isakmp_header(iph1->sendbuf, iph1);
		if (p.buff == NULL)
			goto end;

		/* set SA payload to reply */
		p = set_isakmp_payload_c(p, iph1->sa_ret, ISAKMP_NPTYPE_SA);

		/* create isakmp KE payload */
		p = set_isakmp_payload_c(p, iph1->dhpub, ISAKMP_NPTYPE_KE);

		/* create isakmp NONCE payload */
		p = set_isakmp_payload_c(p, iph1->nonce, ISAKMP_NPTYPE_NONCE);

		/* add ID payload */
		p = set_isakmp_payload_c(p, iph1->id, ISAKMP_NPTYPE_ID);

		/* add CERT payload if there */
		if (need_cert)
			p = set_isakmp_payload_c(p, iph1->cert->pl, ISAKMP_NPTYPE_CERT);
		/* add SIG payload */
		p = set_isakmp_payload_c(p, iph1->sig, ISAKMP_NPTYPE_SIG);

		/* append vendor id, if needed */
		if (vid)
			p = set_isakmp_payload_c(p, vid, ISAKMP_NPTYPE_VID);

		/* create isakmp CR payload if needed */
		if (need_cr)
			p = set_isakmp_payload_c(p, cr, ISAKMP_NPTYPE_CR);
		break;
#endif
	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
		tlen += sizeof(*gen) + iph1->hash->l;
		break;
#ifdef HAVE_GSSAPI
	case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
		/* create buffer to send isakmp payload */
		gsshash = gssapi_wraphash(iph1);
		if (gsshash == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to wrap hash\n");
			/*
			 * This is probably due to the GSS roundtrips not
			 * being finished yet. Return this error in
			 * the hope that a fallback to main mode will
			 * be done.
			 */
			isakmp_info_send_n1(iph1,
			    ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE, NULL);
			goto end;
		}
		if (iph1->approval->gssid != NULL)
			gss_sa = ipsecdoi_setph1proposal(iph1->approval);  
		else
			gss_sa = iph1->sa_ret;

		tlen += sizeof(*gen) + gss_sa->l
			+ sizeof(*gen) + iph1->dhpub->l
			+ sizeof(*gen) + iph1->nonce->l
			+ sizeof(*gen) + iph1->id->l
			+ sizeof(*gen) + gsslen
			+ sizeof(*gen) + gsshash->l;
		if ((vid = set_vendorid(iph1->approval->vendorid)) != NULL)
			tlen += sizeof(*gen) + vid->l;
		iph1->sendbuf = vmalloc(tlen);
		if (iph1->sendbuf == NULL) { 
			plog(LLV_ERROR, LOCATION, NULL,
				"failed to get buffer to send\n");
			goto end;
		}

		/* set isakmp header */
		p = set_isakmp_header(iph1->sendbuf, iph1);
		if (p.buff == NULL)
			goto end;

		/* set SA payload to reply */
		p = set_isakmp_payload_c(p, gss_sa, ISAKMP_NPTYPE_SA);

		/* create isakmp KE payload */
		p = set_isakmp_payload_c(p, iph1->dhpub, ISAKMP_NPTYPE_KE);

		/* create isakmp NONCE payload */
		p = set_isakmp_payload_c(p, iph1->nonce, ISAKMP_NPTYPE_NONCE);

		/* create isakmp ID payload */
		p = set_isakmp_payload_c(p, iph1->id, ISAKMP_NPTYPE_ID);

		/* create GSS payload */
		gssapi_get_token_to_send(iph1, &gsstoken);
		p = set_isakmp_payload_c(p, gsstoken, ISAKMP_NPTYPE_GSS);

		/* create isakmp HASH payload */
		p = set_isakmp_payload_c(p, gsshash, ISAKMP_NPTYPE_HASH);

		/* append vendor id, if needed */
		if (vid)
			p = set_isakmp_payload_c(p, vid, ISAKMP_NPTYPE_VID);
		break;
#endif
	}


#ifdef HAVE_PRINT_ISAKMP_C
	isakmp_printpacket(iph1->sendbuf, iph1->local, iph1->remote, 1);
#endif

	/* send the packet, add to the schedule to resend */
	iph1->retry_counter = iph1->rmconf->retry_counter;
	if (isakmp_ph1resend(iph1) == -1)
		goto end;

	/* the sending message is added to the received-list. */
	if (add_recvdpkt(iph1->remote, iph1->local, iph1->sendbuf, msg) == -1) {
		plog(LLV_ERROR , LOCATION, NULL,
			"failed to add a response packet to the tree.\n");
		goto end;
	}

	iph1->status = PHASE1ST_MSG1SENT;

	error = 0;

end:
	if (cr)
		vfree(cr);
	if (vid)
		vfree(vid);
#ifdef HAVE_GSSAPI
	if (gsstoken)
		vfree(gsstoken);
	if (gsshash)
		vfree(gsshash);
	if (gss_sa != iph1->sa_ret)
		vfree(gss_sa);
#endif

	return error;
}

/*
 * receive from initiator
 * 	psk: HDR, HASH_I
 *   gssapi: HDR, HASH_I
 * 	sig: HDR, [ CERT, ] SIG_I
 * 	rsa: HDR, HASH_I
 * 	rev: HDR, HASH_I
 */
int
agg_r2recv(iph1, msg0)
	struct ph1handle *iph1;
	vchar_t *msg0;
{
	vchar_t *msg = NULL;
	vchar_t *pbuf = NULL;
	struct isakmp_parse_t *pa;
	int error = -1;

	/* validity check */
	if (iph1->status != PHASE1ST_MSG1SENT) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* decrypting if need. */
	/* XXX configurable ? */
	if (ISSET(((struct isakmp *)msg0->v)->flags, ISAKMP_FLAG_E)) {
		msg = oakley_do_decrypt(iph1, msg0,
					iph1->ivm->iv, iph1->ivm->ive);
		if (msg == NULL)
			goto end;
	} else
		msg = vdup(msg0);

	/* validate the type of next payload */
	pbuf = isakmp_parse(msg);
	if (pbuf == NULL)
		goto end;

	iph1->pl_hash = NULL;

	for (pa = (struct isakmp_parse_t *)pbuf->v;
	     pa->type != ISAKMP_NPTYPE_NONE;
	     pa++) {

		switch (pa->type) {
		case ISAKMP_NPTYPE_HASH:
			iph1->pl_hash = (struct isakmp_pl_hash *)pa->ptr;
			break;
		case ISAKMP_NPTYPE_VID:
			(void)check_vendorid(pa->ptr);
			break;
#ifdef HAVE_SIGNING_C
		case ISAKMP_NPTYPE_CERT:
			if (oakley_savecert(iph1, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_SIG:
			if (isakmp_p2ph(&iph1->sig_p, pa->ptr) < 0)
				goto end;
			break;
#endif
		case ISAKMP_NPTYPE_N:
			isakmp_check_notify(pa->ptr, iph1);
			break;
		default:
			/* don't send information, see isakmp_ident_r1() */
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"ignore the packet, "
				"received unexpecting payload type %d.\n",
				pa->type);
			goto end;
		}
	}

	/* validate authentication value */
    {
	int type;
	type = oakley_validate_auth(iph1);
	if (type != 0) {
		if (type == -1) {
			/* message printed inner oakley_validate_auth() */
			goto end;
		}
		isakmp_info_send_n1(iph1, type, NULL);
		goto end;
	}
    }

	iph1->status = PHASE1ST_MSG2RECEIVED;

	error = 0;

end:
	if (pbuf)
		vfree(pbuf);
	if (msg)
		vfree(msg);
	if (error) {
		oakley_delcert(iph1->cert_p);
		iph1->cert_p = NULL;
		oakley_delcert(iph1->crl_p);
		iph1->crl_p = NULL;
		VPTRINIT(iph1->sig_p);
	}

	return error;
}

/*
 * status update and establish isakmp sa.
 */
int
agg_r2send(iph1, msg)
	struct ph1handle *iph1;
	vchar_t *msg;
{
	int error = -1;

	/* validity check */
	if (iph1->status != PHASE1ST_MSG2RECEIVED) {
		plog(LLV_ERROR, LOCATION, NULL,
			"status mismatched %d.\n", iph1->status);
		goto end;
	}

	/* IV synchronized when packet encrypted. */
	/* see handler.h about IV synchronization. */
	if (ISSET(((struct isakmp *)msg->v)->flags, ISAKMP_FLAG_E))
		memcpy(iph1->ivm->iv->v, iph1->ivm->ive->v, iph1->ivm->iv->l);

	/* set encryption flag */
	iph1->flags |= ISAKMP_FLAG_E;

	iph1->status = PHASE1ST_ESTABLISHED;

	error = 0;

end:
	return error;
}
