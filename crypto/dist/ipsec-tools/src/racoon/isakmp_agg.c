/*	$NetBSD: isakmp_agg.c,v 1.9 2006/09/30 21:49:37 manu Exp $	*/

/* Id: isakmp_agg.c,v 1.28 2006/04/06 16:46:08 manubsd Exp */

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

#include "config.h"

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

#ifdef ENABLE_HYBRID
#include <resolv.h>
#endif

#include "localconf.h"
#include "remoteconf.h"
#include "isakmp_var.h"
#include "isakmp.h"
#include "evt.h"
#include "oakley.h"
#include "handler.h"
#include "ipsec_doi.h"
#include "crypto_openssl.h"
#include "pfkey.h"
#include "isakmp_agg.h"
#include "isakmp_inf.h"
#ifdef ENABLE_HYBRID
#include "isakmp_xauth.h"
#include "isakmp_cfg.h"
#endif
#ifdef ENABLE_FRAG
#include "isakmp_frag.h"
#endif
#include "vendorid.h"
#include "strnames.h"

#ifdef ENABLE_NATT
#include "nattraversal.h"
#endif

#ifdef HAVE_GSSAPI
#include "gssapi.h"
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
	struct payload_list *plist = NULL;
	int need_cr = 0;
	vchar_t *cr = NULL; 
	int error = -1;
#ifdef ENABLE_NATT
	vchar_t *vid_natt[MAX_NATT_VID_COUNT] = { NULL };
	int i;
#endif
#ifdef ENABLE_HYBRID
	vchar_t *vid_xauth = NULL;
	vchar_t *vid_unity = NULL;
#endif
#ifdef ENABLE_FRAG
	vchar_t *vid_frag = NULL;
#endif
#ifdef HAVE_GSSAPI
	vchar_t *gsstoken = NULL;
	int len;
#endif
#ifdef ENABLE_DPD
	vchar_t *vid_dpd = NULL;
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

#ifdef ENABLE_HYBRID
	/* Do we need Xauth VID? */
	switch (RMAUTHMETHOD(iph1)) {
	case FICTIVE_AUTH_METHOD_XAUTH_PSKEY_I:
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I:
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I:
		if ((vid_xauth = set_vendorid(VENDORID_XAUTH)) == NULL)
			plog(LLV_ERROR, LOCATION, NULL, 
			     "Xauth vendor ID generation failed\n");
		if ((vid_unity = set_vendorid(VENDORID_UNITY)) == NULL)
			plog(LLV_ERROR, LOCATION, NULL, 
			     "Unity vendor ID generation failed\n");
		break;
	default:
		break;
	}
#endif

#ifdef ENABLE_FRAG
	if (iph1->rmconf->ike_frag) {
		vid_frag = set_vendorid(VENDORID_FRAG);
		if (vid_frag != NULL)
			vid_frag = isakmp_frag_addcap(vid_frag,
			    VENDORID_FRAG_AGG);
		if (vid_frag == NULL)
			plog(LLV_ERROR, LOCATION, NULL,
			    "Frag vendorID construction failed\n");
	}		
#endif

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

	plog(LLV_DEBUG, LOCATION, NULL, "authmethod is %s\n",
		s_oakley_attr_method(iph1->rmconf->proposal->authmethod));
#ifdef HAVE_GSSAPI
	if (RMAUTHMETHOD(iph1) == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB)
		gssapi_get_itoken(iph1, &len);
#endif

	/* set SA payload to propose */
	plist = isakmp_plist_append(plist, iph1->sa, ISAKMP_NPTYPE_SA);

	/* create isakmp KE payload */
	plist = isakmp_plist_append(plist, iph1->dhpub, ISAKMP_NPTYPE_KE);

	/* create isakmp NONCE payload */
	plist = isakmp_plist_append(plist, iph1->nonce, ISAKMP_NPTYPE_NONCE);

	/* create isakmp ID payload */
	plist = isakmp_plist_append(plist, iph1->id, ISAKMP_NPTYPE_ID);

#ifdef HAVE_GSSAPI
	if (RMAUTHMETHOD(iph1) == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB) {
		gssapi_get_token_to_send(iph1, &gsstoken);
		plist = isakmp_plist_append(plist, gsstoken, ISAKMP_NPTYPE_GSS);
	}
#endif
	/* create isakmp CR payload */
	if (need_cr)
		plist = isakmp_plist_append(plist, cr, ISAKMP_NPTYPE_CR);

#ifdef ENABLE_FRAG
	if (vid_frag)
		plist = isakmp_plist_append(plist, vid_frag, ISAKMP_NPTYPE_VID);
#endif
#ifdef ENABLE_NATT
	/* 
	 * set VID payload for NAT-T if NAT-T 
	 * support allowed in the config file 
	 */
	if (iph1->rmconf->nat_traversal) 
		plist = isakmp_plist_append_natt_vids(plist, vid_natt);
#endif
#ifdef ENABLE_HYBRID
	if (vid_xauth)
		plist = isakmp_plist_append(plist, 
		    vid_xauth, ISAKMP_NPTYPE_VID);
	if (vid_unity)
		plist = isakmp_plist_append(plist, 
		    vid_unity, ISAKMP_NPTYPE_VID);
#endif
#ifdef ENABLE_DPD
	if(iph1->rmconf->dpd){
		vid_dpd = set_vendorid(VENDORID_DPD);
		if (vid_dpd != NULL)
			plist = isakmp_plist_append(plist, vid_dpd, ISAKMP_NPTYPE_VID);
	}
#endif

	iph1->sendbuf = isakmp_plist_set_all (&plist, iph1);

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
#ifdef HAVE_GSSAPI
	if (gsstoken)
		vfree(gsstoken);
#endif
#ifdef ENABLE_FRAG
	if (vid_frag)
		vfree(vid_frag);
#endif
#ifdef ENABLE_NATT
	for (i = 0; i < MAX_NATT_VID_COUNT && vid_natt[i] != NULL; i++)
		vfree(vid_natt[i]);
#endif
#ifdef ENABLE_HYBRID
	if (vid_xauth != NULL)
		vfree(vid_xauth);
	if (vid_unity != NULL)
		vfree(vid_unity);
#endif
#ifdef ENABLE_DPD
	if (vid_dpd != NULL)
		vfree(vid_dpd);
#endif

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
	int vid_numeric;
	int ptype;
#ifdef ENABLE_HYBRID
	vchar_t *unity_vid;
	vchar_t *xauth_vid;
#endif
#ifdef HAVE_GSSAPI
	vchar_t *gsstoken = NULL;
#endif

#ifdef ENABLE_NATT
	int natd_seq = 0;
	struct natd_payload {
		int seq;
		vchar_t *payload;
		TAILQ_ENTRY(natd_payload) chain;
	};
	TAILQ_HEAD(_natd_payload, natd_payload) natd_tree;
	TAILQ_INIT(&natd_tree);
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
		case ISAKMP_NPTYPE_VID:
			vid_numeric = check_vendorid(pa->ptr);
#ifdef ENABLE_NATT
			if (iph1->rmconf->nat_traversal && 
			    natt_vendorid(vid_numeric))
				natt_handle_vendorid(iph1, vid_numeric);
#endif
#ifdef ENABLE_HYBRID
			switch (vid_numeric) {
			case VENDORID_XAUTH:
				iph1->mode_cfg->flags |= 
				    ISAKMP_CFG_VENDORID_XAUTH;
				break;

			case VENDORID_UNITY:
				iph1->mode_cfg->flags |= 
				    ISAKMP_CFG_VENDORID_UNITY;
				break;
			default:
				break;
			}
#endif
#ifdef ENABLE_DPD
			if (vid_numeric == VENDORID_DPD && iph1->rmconf->dpd) {
				iph1->dpd_support=1;
				plog(LLV_DEBUG, LOCATION, NULL,
					 "remote supports DPD\n");
			}
#endif
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

#ifdef ENABLE_NATT
		case ISAKMP_NPTYPE_NATD_DRAFT:
		case ISAKMP_NPTYPE_NATD_RFC:
			if (NATT_AVAILABLE(iph1) && iph1->natt_options != NULL &&
			    pa->type == iph1->natt_options->payload_nat_d) {
				struct natd_payload *natd;
				natd = (struct natd_payload *)racoon_malloc(sizeof(*natd));
				if (!natd)
					goto end;

				natd->payload = NULL;

				if (isakmp_p2ph (&natd->payload, pa->ptr) < 0)
					goto end;

				natd->seq = natd_seq++;

				TAILQ_INSERT_TAIL(&natd_tree, natd, chain);
				break;
			}
			/* passthrough to default... */
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

#ifdef ENABLE_NATT
	if (NATT_AVAILABLE(iph1)) {
		struct natd_payload *natd = NULL;
		int natd_verified;
		
		plog(LLV_INFO, LOCATION, iph1->remote,
		     "Selected NAT-T version: %s\n",
		     vid_string_by_id(iph1->natt_options->version));

		/* set both bits first so that we can clear them
		   upon verifying hashes */
		iph1->natt_flags |= NAT_DETECTED;
                        
		while ((natd = TAILQ_FIRST(&natd_tree)) != NULL) {
			/* this function will clear appropriate bits bits 
			   from iph1->natt_flags */
			natd_verified = natt_compare_addr_hash (iph1,
				natd->payload, natd->seq);

			plog (LLV_INFO, LOCATION, NULL, "NAT-D payload #%d %s\n",
				natd->seq - 1,
				natd_verified ? "verified" : "doesn't match");
			
			vfree (natd->payload);

			TAILQ_REMOVE(&natd_tree, natd, chain);
			racoon_free (natd);
		}

		plog (LLV_INFO, LOCATION, NULL, "NAT %s %s%s\n",
		      iph1->natt_flags & NAT_DETECTED ? 
		      		"detected:" : "not detected",
		      iph1->natt_flags & NAT_DETECTED_ME ? "ME " : "",
		      iph1->natt_flags & NAT_DETECTED_PEER ? "PEER" : "");

		if (iph1->natt_flags & NAT_DETECTED)
			natt_float_ports (iph1);
	}
#endif

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
	ptype = oakley_validate_auth(iph1);
	if (ptype != 0) {
		if (ptype == -1) {
			/* message printed inner oakley_validate_auth() */
			goto end;
		}
		EVT_PUSH(iph1->local, iph1->remote, 
		    EVTT_PEERPH1AUTH_FAILED, NULL);
		isakmp_info_send_n1(iph1, ptype, NULL);
		goto end;
	}

	if (oakley_checkcr(iph1) < 0) {
		/* Ignore this error in order to be interoperability. */
		;
	}

	/* change status of isakmp status entry */
	iph1->status = PHASE1ST_MSG2RECEIVED;

	error = 0;

end:
#ifdef HAVE_GSSAPI
	if (gsstoken)
		vfree(gsstoken);
#endif
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
	struct payload_list *plist = NULL;
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
		if (gssapi_more_tokens(iph1) &&
#ifdef ENABLE_HYBRID
		    !iph1->rmconf->xauth &&
#endif
		    1)
			isakmp_info_send_n1(iph1,
			    ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE, NULL);
#endif
		goto end;
	}

	switch (AUTHMETHOD(iph1)) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
#ifdef ENABLE_HYBRID
	case FICTIVE_AUTH_METHOD_XAUTH_PSKEY_I:
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_I:
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_I:
#endif  
		/* set HASH payload */
		plist = isakmp_plist_append(plist, 
		    iph1->hash, ISAKMP_NPTYPE_HASH);
		break;

	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
#ifdef ENABLE_HYBRID
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_I:
#endif
		/* XXX if there is CR or not ? */

		if (oakley_getmycert(iph1) < 0)
			goto end;

		if (oakley_getsign(iph1) < 0)
			goto end;

		if (iph1->cert != NULL && iph1->rmconf->send_cert)
			need_cert = 1;

		/* add CERT payload if there */
		if (need_cert)
			plist = isakmp_plist_append(plist, 
			    iph1->cert->pl, ISAKMP_NPTYPE_CERT);

		/* add SIG payload */
		plist = isakmp_plist_append(plist, 
		    iph1->sig, ISAKMP_NPTYPE_SIG);
		break;

	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
#ifdef ENABLE_HYBRID
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_I:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_I:
#endif
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

		plist = isakmp_plist_append(plist, 
		    gsshash, ISAKMP_NPTYPE_HASH);
		break;
#endif
	}

#ifdef ENABLE_NATT
	/* generate NAT-D payloads */
	if (NATT_AVAILABLE(iph1)) {
		vchar_t *natd[2] = { NULL, NULL };

		plog(LLV_INFO, LOCATION, 
		    NULL, "Adding remote and local NAT-D payloads.\n");

		if ((natd[0] = natt_hash_addr (iph1, iph1->remote)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "NAT-D hashing failed for %s\n", 
			    saddr2str(iph1->remote));
			goto end;
		}

		if ((natd[1] = natt_hash_addr (iph1, iph1->local)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "NAT-D hashing failed for %s\n", 
			    saddr2str(iph1->local));
			goto end;
		}

		plist = isakmp_plist_append(plist, 
		    natd[0], iph1->natt_options->payload_nat_d);
		plist = isakmp_plist_append(plist, 
		    natd[1], iph1->natt_options->payload_nat_d);
	}
#endif

	iph1->sendbuf = isakmp_plist_set_all (&plist, iph1);

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
	int vid_numeric;
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
			vid_numeric = check_vendorid(pa->ptr);

#ifdef ENABLE_NATT
			if (iph1->rmconf->nat_traversal &&
			    natt_vendorid(vid_numeric)) {
				natt_handle_vendorid(iph1, vid_numeric);
				break;
			}
#endif
#ifdef ENABLE_HYBRID
			switch (vid_numeric) {
			case VENDORID_XAUTH:
				iph1->mode_cfg->flags |= 
				    ISAKMP_CFG_VENDORID_XAUTH;
				break;

			case VENDORID_UNITY:
				iph1->mode_cfg->flags |= 
				    ISAKMP_CFG_VENDORID_UNITY;
				break;
			default:
				break;
			}
#endif
#ifdef ENABLE_DPD
			if (vid_numeric == VENDORID_DPD && iph1->rmconf->dpd) {
				iph1->dpd_support=1;
				plog(LLV_DEBUG, LOCATION, NULL,
					 "remote supports DPD\n");
			}
#endif
#ifdef ENABLE_FRAG
			if ((vid_numeric == VENDORID_FRAG) &&
			    (vendorid_frag_cap(pa->ptr) & VENDORID_FRAG_AGG))
				iph1->frag = 1;
#endif
			break;

		case ISAKMP_NPTYPE_CR:
			if (oakley_savecr(iph1, pa->ptr) < 0)
				goto end;
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

#ifdef ENABLE_NATT
	if (NATT_AVAILABLE(iph1))
		plog(LLV_INFO, LOCATION, iph1->remote,
		     "Selected NAT-T version: %s\n",
		     vid_string_by_id(iph1->natt_options->version));
#endif

	/* check SA payload and set approval SA for use */
	if (ipsecdoi_checkph1proposal(iph1->sa, iph1) < 0) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"failed to get valid proposal.\n");
		/* XXX send information */
		goto end;
	}

	if (oakley_checkcr(iph1) < 0) {
		/* Ignore this error in order to be interoperability. */
		;
	}

	iph1->status = PHASE1ST_MSG1RECEIVED;

	error = 0;

end:
#ifdef HAVE_GSSAPI
	if (gsstoken)
		vfree(gsstoken);
#endif
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
	struct payload_list *plist = NULL;
	int need_cr = 0;
	int need_cert = 0;
	vchar_t *cr = NULL;
	int error = -1;
#ifdef ENABLE_HYBRID
	vchar_t *xauth_vid = NULL;
	vchar_t *unity_vid = NULL;
#endif
#ifdef ENABLE_NATT
	vchar_t *vid_natt = NULL;
	vchar_t *natd[2] = { NULL, NULL };
#endif
#ifdef ENABLE_DPD
	vchar_t *vid_dpd = NULL;
#endif
#ifdef ENABLE_FRAG
	vchar_t *vid_frag = NULL;
#endif

#ifdef HAVE_GSSAPI
	int gsslen;
	vchar_t *gsstoken = NULL, *gsshash = NULL;
	vchar_t *gss_sa = NULL;
	int free_gss_sa = 0;
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
	if (RMAUTHMETHOD(iph1) == OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB)
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

#ifdef ENABLE_NATT
	/* Has the peer announced NAT-T? */
	if (NATT_AVAILABLE(iph1)) {
	  	/* set chosen VID */
		vid_natt = set_vendorid(iph1->natt_options->version);

		/* generate NAT-D payloads */
		plog (LLV_INFO, LOCATION, NULL, "Adding remote and local NAT-D payloads.\n");
		if ((natd[0] = natt_hash_addr (iph1, iph1->remote)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"NAT-D hashing failed for %s\n", saddr2str(iph1->remote));
			goto end;
		}

		if ((natd[1] = natt_hash_addr (iph1, iph1->local)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
				"NAT-D hashing failed for %s\n", saddr2str(iph1->local));
			goto end;
		}
	}
#endif
#ifdef ENABLE_DPD
	/* Only send DPD support if remote announced DPD and if DPD support is active */
	if (iph1->dpd_support && iph1->rmconf->dpd)
		vid_dpd = set_vendorid(VENDORID_DPD);
#endif
#ifdef ENABLE_FRAG
	if (iph1->frag) {
		vid_frag = set_vendorid(VENDORID_FRAG);
		if (vid_frag != NULL)
			vid_frag = isakmp_frag_addcap(vid_frag,
			    VENDORID_FRAG_AGG);
		if (vid_frag == NULL)
			plog(LLV_ERROR, LOCATION, NULL,
			    "Frag vendorID construction failed\n");
	}
#endif

	switch (AUTHMETHOD(iph1)) {
	case OAKLEY_ATTR_AUTH_METHOD_PSKEY:
#ifdef ENABLE_HYBRID
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_PSKEY_R:
#endif
		/* set SA payload to reply */
		plist = isakmp_plist_append(plist, 
		    iph1->sa_ret, ISAKMP_NPTYPE_SA);

		/* create isakmp KE payload */
		plist = isakmp_plist_append(plist, 
		    iph1->dhpub, ISAKMP_NPTYPE_KE);

		/* create isakmp NONCE payload */
		plist = isakmp_plist_append(plist, 
		    iph1->nonce, ISAKMP_NPTYPE_NONCE);

		/* create isakmp ID payload */
		plist = isakmp_plist_append(plist, 
		    iph1->id, ISAKMP_NPTYPE_ID);

		/* create isakmp HASH payload */
		plist = isakmp_plist_append(plist, 
		    iph1->hash, ISAKMP_NPTYPE_HASH);

		/* create isakmp CR payload if needed */
		if (need_cr)
			plist = isakmp_plist_append(plist, 
			    cr, ISAKMP_NPTYPE_CR);
		break;
	case OAKLEY_ATTR_AUTH_METHOD_DSSSIG:
	case OAKLEY_ATTR_AUTH_METHOD_RSASIG:
#ifdef ENABLE_HYBRID
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_RSA_R:
	case OAKLEY_ATTR_AUTH_METHOD_HYBRID_DSS_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSASIG_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_DSSSIG_R:
#endif
		/* XXX if there is CR or not ? */

		if (oakley_getmycert(iph1) < 0)
			goto end;

		if (oakley_getsign(iph1) < 0)
			goto end;

		if (iph1->cert != NULL && iph1->rmconf->send_cert)
			need_cert = 1;

		/* set SA payload to reply */
		plist = isakmp_plist_append(plist, 
		    iph1->sa_ret, ISAKMP_NPTYPE_SA);

		/* create isakmp KE payload */
		plist = isakmp_plist_append(plist, 
		    iph1->dhpub, ISAKMP_NPTYPE_KE);

		/* create isakmp NONCE payload */
		plist = isakmp_plist_append(plist, 
		    iph1->nonce, ISAKMP_NPTYPE_NONCE);

		/* add ID payload */
		plist = isakmp_plist_append(plist, 
		    iph1->id, ISAKMP_NPTYPE_ID);

		/* add CERT payload if there */
		if (need_cert)
			plist = isakmp_plist_append(plist, 
			    iph1->cert->pl, ISAKMP_NPTYPE_CERT);

		/* add SIG payload */
		plist = isakmp_plist_append(plist, 
		    iph1->sig, ISAKMP_NPTYPE_SIG);

		/* create isakmp CR payload if needed */
		if (need_cr)
			plist = isakmp_plist_append(plist, 
			    cr, ISAKMP_NPTYPE_CR);
		break;

	case OAKLEY_ATTR_AUTH_METHOD_RSAENC:
	case OAKLEY_ATTR_AUTH_METHOD_RSAREV:
#ifdef ENABLE_HYBRID
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAENC_R:
	case OAKLEY_ATTR_AUTH_METHOD_XAUTH_RSAREV_R:
#endif
		break;
#ifdef HAVE_GSSAPI
	case OAKLEY_ATTR_AUTH_METHOD_GSSAPI_KRB:
			/* create buffer to send isakmp payload */
			gsshash = gssapi_wraphash(iph1);
			if (gsshash == NULL) {
				plog(LLV_ERROR, LOCATION, NULL,
					"failed to wrap hash\n");
				/*
				 * This is probably due to the GSS 
				 * roundtrips not being finished yet. 
				 * Return this error in the hope that 
				 * a fallback to main mode will be done.
				 */
				isakmp_info_send_n1(iph1,
				    ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE, NULL);
				goto end;
			}
			if (iph1->approval->gssid != NULL)
				gss_sa = 
				    ipsecdoi_setph1proposal(iph1->approval);  
			else
				gss_sa = iph1->sa_ret;

			if (gss_sa != iph1->sa_ret)
				free_gss_sa = 1;

			/* set SA payload to reply */
			plist = isakmp_plist_append(plist, 
			    gss_sa, ISAKMP_NPTYPE_SA);

			/* create isakmp KE payload */
			plist = isakmp_plist_append(plist, 
			    iph1->dhpub, ISAKMP_NPTYPE_KE);

			/* create isakmp NONCE payload */
			plist = isakmp_plist_append(plist, 
			    iph1->nonce, ISAKMP_NPTYPE_NONCE);

			/* create isakmp ID payload */
			plist = isakmp_plist_append(plist, 
			    iph1->id, ISAKMP_NPTYPE_ID);

			/* create GSS payload */
			gssapi_get_token_to_send(iph1, &gsstoken);
			plist = isakmp_plist_append(plist, 
			    gsstoken, ISAKMP_NPTYPE_GSS);

			/* create isakmp HASH payload */
			plist = isakmp_plist_append(plist, 
			    gsshash, ISAKMP_NPTYPE_HASH);

			/* append vendor id, if needed */
			break;
#endif
	}

#ifdef ENABLE_HYBRID
	if (iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_XAUTH) {
		plog (LLV_INFO, LOCATION, NULL, "Adding xauth VID payload.\n");
		if ((xauth_vid = set_vendorid(VENDORID_XAUTH)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "Cannot create Xauth vendor ID\n");
			goto end;
		}
		plist = isakmp_plist_append(plist, 
		    xauth_vid, ISAKMP_NPTYPE_VID);
	}

	if (iph1->mode_cfg->flags & ISAKMP_CFG_VENDORID_UNITY) {
		if ((unity_vid = set_vendorid(VENDORID_UNITY)) == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "Cannot create Unity vendor ID\n");
			goto end;
		}
		plist = isakmp_plist_append(plist, 
		    unity_vid, ISAKMP_NPTYPE_VID);
	}
#endif

#ifdef ENABLE_NATT
	/* append NAT-T payloads */
	if (vid_natt) {
		/* chosen VID */
		plist = isakmp_plist_append(plist, vid_natt, ISAKMP_NPTYPE_VID);
		/* NAT-D */
		plist = isakmp_plist_append(plist, natd[0], iph1->natt_options->payload_nat_d);
		plist = isakmp_plist_append(plist, natd[1], iph1->natt_options->payload_nat_d);
	}
#endif

#ifdef ENABLE_FRAG
	if (vid_frag)
		plist = isakmp_plist_append(plist, vid_frag, ISAKMP_NPTYPE_VID);
#endif

#ifdef ENABLE_DPD
	if (vid_dpd)
		plist = isakmp_plist_append(plist, vid_dpd, ISAKMP_NPTYPE_VID);
#endif

	iph1->sendbuf = isakmp_plist_set_all (&plist, iph1);

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
#ifdef ENABLE_HYBRID
	if (xauth_vid)
		vfree(xauth_vid);
	if (unity_vid)
		vfree(unity_vid);
#endif
#ifdef HAVE_GSSAPI
	if (gsstoken)
		vfree(gsstoken);
	if (gsshash)
		vfree(gsshash);
	if (free_gss_sa)
		vfree(gss_sa);
#endif
#ifdef ENABLE_DPD
	if (vid_dpd)
		vfree(vid_dpd);
#endif
#ifdef ENABLE_FRAG
	if (vid_frag)
		vfree(vid_frag);
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
	int ptype;

#ifdef ENABLE_NATT
	int natd_seq = 0;
#endif

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
		case ISAKMP_NPTYPE_CERT:
			if (oakley_savecert(iph1, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_SIG:
			if (isakmp_p2ph(&iph1->sig_p, pa->ptr) < 0)
				goto end;
			break;
		case ISAKMP_NPTYPE_N:
			isakmp_check_notify(pa->ptr, iph1);
			break;

#ifdef ENABLE_NATT
		case ISAKMP_NPTYPE_NATD_DRAFT:
		case ISAKMP_NPTYPE_NATD_RFC:
			if (NATT_AVAILABLE(iph1) && iph1->natt_options != NULL &&
				pa->type == iph1->natt_options->payload_nat_d)
			{
				vchar_t *natd_received = NULL;
				int natd_verified;
				
				if (isakmp_p2ph (&natd_received, pa->ptr) < 0)
					goto end;
				
				if (natd_seq == 0)
					iph1->natt_flags |= NAT_DETECTED;
				
				natd_verified = natt_compare_addr_hash (iph1,
					natd_received, natd_seq++);
				
				plog (LLV_INFO, LOCATION, NULL, "NAT-D payload #%d %s\n",
					natd_seq - 1,
					natd_verified ? "verified" : "doesn't match");
				
				vfree (natd_received);
				break;
			}
			/* passthrough to default... */
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

#ifdef ENABLE_NATT
	if (NATT_AVAILABLE(iph1))
		plog (LLV_INFO, LOCATION, NULL, "NAT %s %s%s\n",
		      iph1->natt_flags & NAT_DETECTED ? 
		      		"detected:" : "not detected",
		      iph1->natt_flags & NAT_DETECTED_ME ? "ME " : "",
		      iph1->natt_flags & NAT_DETECTED_PEER ? "PEER" : "");
#endif

	/* validate authentication value */
	ptype = oakley_validate_auth(iph1);
	if (ptype != 0) {
		if (ptype == -1) {
			/* message printed inner oakley_validate_auth() */
			goto end;
		}
		EVT_PUSH(iph1->local, iph1->remote, 
		    EVTT_PEERPH1AUTH_FAILED, NULL);
		isakmp_info_send_n1(iph1, ptype, NULL);
		goto end;
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
