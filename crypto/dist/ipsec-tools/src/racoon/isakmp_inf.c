/*	$NetBSD: isakmp_inf.c,v 1.53 2018/05/19 19:47:47 maxv Exp $	*/

/* Id: isakmp_inf.c,v 1.44 2006/05/06 20:45:52 manubsd Exp */

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

#include <net/pfkeyv2.h>
#include <netinet/in.h>
#include <sys/queue.h>
#include PATH_IPSEC_H

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
#ifdef ENABLE_HYBRID
#include <resolv.h>
#endif

#include "libpfkey.h"

#include "var.h"
#include "vmbuf.h"
#include "schedule.h"
#include "str2val.h"
#include "misc.h"
#include "plog.h"
#include "debug.h"

#include "localconf.h"
#include "remoteconf.h"
#include "sockmisc.h"
#include "handler.h"
#include "policy.h"
#include "proposal.h"
#include "isakmp_var.h"
#include "evt.h"
#include "isakmp.h"
#ifdef ENABLE_HYBRID
#include "isakmp_xauth.h"
#include "isakmp_unity.h"
#include "isakmp_cfg.h" 
#endif
#include "isakmp_inf.h"
#include "oakley.h"
#include "ipsec_doi.h"
#include "crypto_openssl.h"
#include "pfkey.h"
#include "policy.h"
#include "algorithm.h"
#include "proposal.h"
#include "admin.h"
#include "strnames.h"
#ifdef ENABLE_NATT
#include "nattraversal.h"
#endif

/* information exchange */
static int isakmp_info_recv_n (struct ph1handle *, struct isakmp_pl_n *, u_int32_t, int);
static int isakmp_info_recv_d (struct ph1handle *, struct isakmp_pl_d *, u_int32_t, int);

#ifdef ENABLE_DPD
static int isakmp_info_recv_r_u __P((struct ph1handle *,
	struct isakmp_pl_ru *, u_int32_t));
static int isakmp_info_recv_r_u_ack __P((struct ph1handle *,
	struct isakmp_pl_ru *, u_int32_t));
static void isakmp_info_send_r_u __P((struct sched *));
#endif

/* %%%
 * Information Exchange
 */
/*
 * receive Information
 */
int
isakmp_info_recv(iph1, msg0)
	struct ph1handle *iph1;
	vchar_t *msg0;
{
	vchar_t *msg = NULL;
	vchar_t *pbuf = NULL;
	u_int32_t msgid = 0;
	int error = -1;
	struct isakmp *isakmp;
	struct isakmp_gen *gen;
	struct isakmp_parse_t *pa;
	void *p;
	vchar_t *hash, *payload;
	struct isakmp_gen *nd;
	u_int8_t np;
	int encrypted;

	plog(LLV_DEBUG, LOCATION, NULL, "receive Information.\n");

	encrypted = ISSET(((struct isakmp *)msg0->v)->flags, ISAKMP_FLAG_E);
	msgid = ((struct isakmp *)msg0->v)->msgid;

	/* Use new IV to decrypt Informational message. */
	if (encrypted) {
		struct isakmp_ivm *ivm;

		if (iph1->ivm == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, "iph1->ivm == NULL\n");
			return -1;
		}

		/* compute IV */
		ivm = oakley_newiv2(iph1, ((struct isakmp *)msg0->v)->msgid);
		if (ivm == NULL)
			return -1;

		msg = oakley_do_decrypt(iph1, msg0, ivm->iv, ivm->ive);
		oakley_delivm(ivm);
		if (msg == NULL)
			return -1;

	} else
		msg = vdup(msg0);

	/* Safety check */
	if (msg->l < sizeof(*isakmp) + sizeof(*gen)) {
		plog(LLV_ERROR, LOCATION, NULL, 
			"ignore information because the "
			"message is way too short - %zu byte(s).\n",
			msg->l);
		goto end;
	}

	isakmp = (struct isakmp *)msg->v;
	gen = (struct isakmp_gen *)((caddr_t)isakmp + sizeof(struct isakmp));
	np = gen->np;

	if (encrypted) {
		if (isakmp->np != ISAKMP_NPTYPE_HASH) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "ignore information because the "
			    "message has no hash payload.\n");
			goto end;
		}

		if (iph1->status != PHASE1ST_ESTABLISHED &&
		    iph1->status != PHASE1ST_DYING) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "ignore information because ISAKMP-SA "
			    "has not been established yet.\n");
			goto end;
		}
		
		/* Safety check */
		if (msg->l < sizeof(*isakmp) + ntohs(gen->len) + sizeof(*nd)) {
			plog(LLV_ERROR, LOCATION, NULL, 
				"ignore information because the "
				"message is too short - %zu byte(s).\n",
				msg->l);
			goto end;
		}

		p = (caddr_t) gen + sizeof(struct isakmp_gen);
		nd = (struct isakmp_gen *) ((caddr_t) gen + ntohs(gen->len));

		/* nd length check */
		if (ntohs(nd->len) > msg->l - (sizeof(struct isakmp) +
		    ntohs(gen->len))) {
			plog(LLV_ERROR, LOCATION, NULL,
				 "too long payload length (broken message?)\n");
			goto end;
		}

		if (ntohs(nd->len) < sizeof(*nd)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"too short payload length (broken message?)\n");
			goto end;
		}

		payload = vmalloc(ntohs(nd->len));
		if (payload == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "cannot allocate memory\n");
			goto end;
		}

		memcpy(payload->v, (caddr_t) nd, ntohs(nd->len));

		/* compute HASH */
		hash = oakley_compute_hash1(iph1, isakmp->msgid, payload);
		if (hash == NULL) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "cannot compute hash\n");

			vfree(payload);
			goto end;
		}
		
		if (ntohs(gen->len) - sizeof(struct isakmp_gen) != hash->l) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "ignore information due to hash length mismatch\n");

			vfree(hash);
			vfree(payload);
			goto end;
		}

		if (memcmp(p, hash->v, hash->l) != 0) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "ignore information due to hash mismatch\n");

			vfree(hash);
			vfree(payload);
			goto end;
		}

		plog(LLV_DEBUG, LOCATION, NULL, "hash validated.\n");

		vfree(hash);
		vfree(payload);
	} else {
		/* make sure the packet was encrypted after the beginning of phase 1. */
		switch (iph1->etype) {
		case ISAKMP_ETYPE_AGG:
		case ISAKMP_ETYPE_BASE:
		case ISAKMP_ETYPE_IDENT:
			if ((iph1->side == INITIATOR && iph1->status < PHASE1ST_MSG3SENT)
			 || (iph1->side == RESPONDER && iph1->status < PHASE1ST_MSG2SENT)) {
				break;
			}
			/*FALLTHRU*/
		default:
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"%s message must be encrypted\n",
				s_isakmp_nptype(np));
			error = 0;
			goto end;
		}
	}

	if (!(pbuf = isakmp_parse(msg))) {
		error = -1;
		goto end;
	}

	error = 0;
	for (pa = (struct isakmp_parse_t *)pbuf->v; pa->type; pa++) {
		switch (pa->type) {
		case ISAKMP_NPTYPE_HASH:
			/* Handled above */
			break;
		case ISAKMP_NPTYPE_N:
			error = isakmp_info_recv_n(iph1,
				(struct isakmp_pl_n *)pa->ptr,
				msgid, encrypted);
			break;
		case ISAKMP_NPTYPE_D:
			error = isakmp_info_recv_d(iph1,
				(struct isakmp_pl_d *)pa->ptr,
				msgid, encrypted);
			break;
		case ISAKMP_NPTYPE_NONCE:
			/* XXX to be 6.4.2 ike-01.txt */
			/* XXX IV is to be synchronized. */
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"ignore Acknowledged Informational\n");
			break;
		default:
			/* don't send information, see isakmp_ident_r1() */
			error = 0;
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"reject the packet, "
				"received unexpected payload type %s.\n",
				s_isakmp_nptype(gen->np));
		}
		if (error < 0)
			break;
	}
    end:
	if (msg != NULL)
		vfree(msg);
	if (pbuf != NULL)
		vfree(pbuf);
	return error;
}


/*
 * log unhandled / unallowed Notification payload
 */
int
isakmp_log_notify(iph1, notify, exchange)
	struct ph1handle *iph1;
	struct isakmp_pl_n *notify;
	const char *exchange;
{
	u_int type;
	char *nraw, *ndata, *nhex;
	size_t l;

	type = ntohs(notify->type);
	if (ntohs(notify->h.len) < sizeof(*notify) + notify->spi_size) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"invalid spi_size in %s notification in %s.\n",
			s_isakmp_notify_msg(type), exchange);
		return -1;
	}

	plog(LLV_ERROR, LOCATION, iph1->remote,
		"notification %s received in %s.\n",
		s_isakmp_notify_msg(type), exchange);

	nraw = ((char*) notify) + sizeof(*notify) + notify->spi_size;
	l = ntohs(notify->h.len) - sizeof(*notify) - notify->spi_size;
	if (l > 0) {
		if (type >= ISAKMP_NTYPE_MINERROR &&
		    type <= ISAKMP_NTYPE_MAXERROR) {
			ndata = binsanitize(nraw, l);
			if (ndata != NULL) {
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"error message: '%s'.\n",
					ndata);
				racoon_free(ndata);
			} else {
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"Cannot allocate memory\n");
			}
		} else {
			nhex = val2str(nraw, l);
			if (nhex != NULL) {
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"notification payload: %s.\n",
					nhex);
				racoon_free(nhex);
			} else {
				plog(LLV_ERROR, LOCATION, iph1->remote,
					"Cannot allocate memory\n");
			}
		}
	}

	return 0;
}


/*
 * handling of Notification payload
 */
static int
isakmp_info_recv_n(iph1, notify, msgid, encrypted)
	struct ph1handle *iph1;
	struct isakmp_pl_n *notify;
	u_int32_t msgid;
	int encrypted;
{
	u_int type;

	type = ntohs(notify->type);
	switch (type) {
	case ISAKMP_NTYPE_CONNECTED:
	case ISAKMP_NTYPE_RESPONDER_LIFETIME:
	case ISAKMP_NTYPE_REPLAY_STATUS:
#ifdef ENABLE_HYBRID
	case ISAKMP_NTYPE_UNITY_HEARTBEAT:
#endif
		/* do something */
		break;
	case ISAKMP_NTYPE_INITIAL_CONTACT:
		if (encrypted)
			return isakmp_info_recv_initialcontact(iph1, NULL);
		break;
#ifdef ENABLE_DPD
	case ISAKMP_NTYPE_R_U_THERE:
		if (encrypted)
			return isakmp_info_recv_r_u(iph1,
				(struct isakmp_pl_ru *)notify, msgid);
		break;
	case ISAKMP_NTYPE_R_U_THERE_ACK:
		if (encrypted)
			return isakmp_info_recv_r_u_ack(iph1,
				(struct isakmp_pl_ru *)notify, msgid);
		break;
#endif
	}

	/* If we receive a error notification we should delete the related
	 * phase1 / phase2 handle, and send an event to racoonctl.
	 * However, since phase1 error notifications are not encrypted and
	 * can not be authenticated, it would allow a DoS attack possibility
	 * to handle them.
	 * Phase2 error notifications should be encrypted, so we could handle
	 * those, but it needs implementing (the old code didn't implement
	 * that either).
	 * So we are good to just log the messages here.
	 */
	if (encrypted)
		isakmp_log_notify(iph1, notify, "informational exchange");
	else
		isakmp_log_notify(iph1, notify, "unencrypted informational exchange");

	return 0;
}

/*
 * handling of Deletion payload
 */
static int
isakmp_info_recv_d(iph1, delete, msgid, encrypted)
	struct ph1handle *iph1;
	struct isakmp_pl_d *delete;
	u_int32_t msgid;
	int encrypted;
{
	int tlen, num_spi;
	struct ph1handle *del_ph1;
	union {
		u_int32_t spi32;
		u_int16_t spi16[2];
	} spi;

	if (ntohl(delete->doi) != IPSEC_DOI) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"delete payload with invalid doi:%d.\n",
			ntohl(delete->doi));
#ifdef ENABLE_HYBRID
		/*
		 * At deconnexion time, Cisco VPN client does this
		 * with a zero DOI. Don't give up in that situation.
		 */
		if (((iph1->mode_cfg->flags &
		    ISAKMP_CFG_VENDORID_UNITY) == 0) || (delete->doi != 0))
			return 0;
#else
		return 0;
#endif
	}

	num_spi = ntohs(delete->num_spi);
	tlen = ntohs(delete->h.len) - sizeof(struct isakmp_pl_d);

	if (tlen != num_spi * delete->spi_size) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"deletion payload with invalid length.\n");
		return 0;
	}

	plog(LLV_DEBUG, LOCATION, iph1->remote,
		"delete payload for protocol %s\n",
		s_ipsecdoi_proto(delete->proto_id));

	if((iph1 == NULL || !iph1->rmconf->weak_phase1_check) && !encrypted) {
		plog(LLV_WARNING, LOCATION, iph1->remote,
			"Ignoring unencrypted delete payload "
			"(check the weak_phase1_check option)\n");
		return 0;
	}

	switch (delete->proto_id) {
	case IPSECDOI_PROTO_ISAKMP:
		if (delete->spi_size != sizeof(isakmp_index)) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"delete payload with strange spi "
				"size %d(proto_id:%d)\n",
				delete->spi_size, delete->proto_id);
			return 0;
		}

		del_ph1=getph1byindex((isakmp_index *)(delete + 1));
		if(del_ph1 != NULL){

			evt_phase1(iph1, EVT_PHASE1_PEER_DELETED, NULL);
			sched_cancel(&del_ph1->scr);

			/*
			 * Delete also IPsec-SAs if rekeying is enabled.
			 */
			if (ph1_rekey_enabled(del_ph1))
				purge_remote(del_ph1);
			else
				isakmp_ph1expire(del_ph1);
		}
		break;

	case IPSECDOI_PROTO_IPSEC_AH:
	case IPSECDOI_PROTO_IPSEC_ESP:
		if (delete->spi_size != sizeof(u_int32_t)) {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"delete payload with strange spi "
				"size %d(proto_id:%d)\n",
				delete->spi_size, delete->proto_id);
			return 0;
		}
		purge_ipsec_spi(iph1->remote, delete->proto_id,
		    (u_int32_t *)(delete + 1), num_spi);
		break;

	case IPSECDOI_PROTO_IPCOMP:
		/* need to handle both 16bit/32bit SPI */
		memset(&spi, 0, sizeof(spi));
		if (delete->spi_size == sizeof(spi.spi16[1])) {
			memcpy(&spi.spi16[1], delete + 1,
			    sizeof(spi.spi16[1]));
		} else if (delete->spi_size == sizeof(spi.spi32))
			memcpy(&spi.spi32, delete + 1, sizeof(spi.spi32));
		else {
			plog(LLV_ERROR, LOCATION, iph1->remote,
				"delete payload with strange spi "
				"size %d(proto_id:%d)\n",
				delete->spi_size, delete->proto_id);
			return 0;
		}
		purge_ipsec_spi(iph1->remote, delete->proto_id,
		    &spi.spi32, num_spi);
		break;

	default:
		plog(LLV_ERROR, LOCATION, iph1->remote,
			"deletion message received, "
			"invalid proto_id: %d\n",
			delete->proto_id);
		return 0;
	}

	plog(LLV_DEBUG, LOCATION, NULL, "purged SAs.\n");

	return 0;
}

/*
 * send Delete payload (for ISAKMP SA) in Informational exchange.
 */
int
isakmp_info_send_d1(iph1)
	struct ph1handle *iph1;
{
	struct isakmp_pl_d *d;
	vchar_t *payload = NULL;
	int tlen;
	int error = 0;

	if (iph1->status != PHASE2ST_ESTABLISHED)
		return 0;

	/* create delete payload */

	/* send SPIs of inbound SAs. */
	/* XXX should send outbound SAs's ? */
	tlen = sizeof(*d) + sizeof(isakmp_index);
	payload = vmalloc(tlen);
	if (payload == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
			"failed to get buffer for payload.\n");
		return errno;
	}

	d = (struct isakmp_pl_d *)payload->v;
	d->h.np = ISAKMP_NPTYPE_NONE;
	d->h.len = htons(tlen);
	d->doi = htonl(IPSEC_DOI);
	d->proto_id = IPSECDOI_PROTO_ISAKMP;
	d->spi_size = sizeof(isakmp_index);
	d->num_spi = htons(1);
	memcpy(d + 1, &iph1->index, sizeof(isakmp_index));

	error = isakmp_info_send_common(iph1, payload,
					ISAKMP_NPTYPE_D, 0);
	vfree(payload);

	return error;
}

/*
 * send Delete payload (for IPsec SA) in Informational exchange, based on
 * pfkey msg.  It sends always single SPI.
 */
int
isakmp_info_send_d2(iph2)
	struct ph2handle *iph2;
{
	struct ph1handle *iph1;
	struct saproto *pr;
	struct isakmp_pl_d *d;
	vchar_t *payload = NULL;
	int tlen;
	int error = 0;
	u_int8_t *spi;

	if (iph2->status != PHASE2ST_ESTABLISHED)
		return 0;

	/*
	 * don't send delete information if there is no phase 1 handler.
	 * It's nonsensical to negotiate phase 1 to send the information.
	 */
	iph1 = getph1byaddr(iph2->src, iph2->dst, 0); 
	if (iph1 == NULL){
		plog(LLV_DEBUG2, LOCATION, NULL,
			 "No ph1 handler found, could not send DELETE_SA\n");
		return 0;
	}

	/* create delete payload */
	for (pr = iph2->approval->head; pr != NULL; pr = pr->next) {

		/* send SPIs of inbound SAs. */
		/*
		 * XXX should I send outbound SAs's ?
		 * I send inbound SAs's SPI only at the moment because I can't
		 * decode any more if peer send encoded packet without aware of
		 * deletion of SA.  Outbound SAs don't come under the situation.
		 */
		tlen = sizeof(*d) + pr->spisize;
		payload = vmalloc(tlen);
		if (payload == NULL) {
			plog(LLV_ERROR, LOCATION, NULL, 
				"failed to get buffer for payload.\n");
			return errno;
		}

		d = (struct isakmp_pl_d *)payload->v;
		d->h.np = ISAKMP_NPTYPE_NONE;
		d->h.len = htons(tlen);
		d->doi = htonl(IPSEC_DOI);
		d->proto_id = pr->proto_id;
		d->spi_size = pr->spisize;
		d->num_spi = htons(1);
		/*
		 * XXX SPI bits are left-filled, for use with IPComp.
		 * we should be switching to variable-length spi field...
		 */
		spi = (u_int8_t *)&pr->spi;
		spi += sizeof(pr->spi);
		spi -= pr->spisize;
		memcpy(d + 1, spi, pr->spisize);

		error = isakmp_info_send_common(iph1, payload,
						ISAKMP_NPTYPE_D, 0);
		vfree(payload);
	}

	return error;
}

/*
 * send Notification payload (for without ISAKMP SA) in Informational exchange
 */
int
isakmp_info_send_nx(isakmp, remote, local, type, data)
	struct isakmp *isakmp;
	struct sockaddr *remote, *local;
	int type;
	vchar_t *data;
{
	struct ph1handle *iph1 = NULL;
	vchar_t *payload = NULL;
	int tlen;
	int error = -1;
	struct isakmp_pl_n *n;
	int spisiz = 0;		/* see below */

	/* add new entry to isakmp status table. */
	iph1 = newph1();
	if (iph1 == NULL)
		return -1;

	memcpy(&iph1->index.i_ck, &isakmp->i_ck, sizeof(cookie_t));
	isakmp_newcookie((char *)&iph1->index.r_ck, remote, local);
	iph1->status = PHASE1ST_START;
	iph1->side = INITIATOR;
	iph1->version = isakmp->v;
	iph1->flags = 0;
	iph1->msgid = 0;	/* XXX */
#ifdef ENABLE_HYBRID
	if ((iph1->mode_cfg = isakmp_cfg_mkstate()) == NULL)
		goto end;
#endif
#ifdef ENABLE_FRAG
	iph1->frag = 0;
	iph1->frag_last_index = 0;
	iph1->frag_chain = NULL;
#endif

	/* copy remote address */
	if (copy_ph1addresses(iph1, NULL, remote, local) < 0)
		goto end;

	tlen = sizeof(*n) + spisiz;
	if (data)
		tlen += data->l;
	payload = vmalloc(tlen);
	if (payload == NULL) { 
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		goto end;
	}

	n = (struct isakmp_pl_n *)payload->v;
	n->h.np = ISAKMP_NPTYPE_NONE;
	n->h.len = htons(tlen);
	n->doi = htonl(IPSEC_DOI);
	n->proto_id = IPSECDOI_KEY_IKE;
	n->spi_size = spisiz;
	n->type = htons(type);
	if (spisiz)
		memset(n + 1, 0, spisiz);	/* XXX spisiz is always 0 */
	if (data)
		memcpy((caddr_t)(n + 1) + spisiz, data->v, data->l);

	error = isakmp_info_send_common(iph1, payload, ISAKMP_NPTYPE_N, 0);
	vfree(payload);

    end:
	if (iph1 != NULL)
		delph1(iph1);

	return error;
}

/*
 * send Notification payload (for ISAKMP SA) in Informational exchange
 */
int
isakmp_info_send_n1(iph1, type, data)
	struct ph1handle *iph1;
	int type;
	vchar_t *data;
{
	vchar_t *payload = NULL;
	int tlen;
	int error = 0;
	struct isakmp_pl_n *n;
	int spisiz;

	/*
	 * note on SPI size: which description is correct?  I have chosen
	 * this to be 0.
	 *
	 * RFC2408 3.1, 2nd paragraph says: ISAKMP SA is identified by
	 * Initiator/Responder cookie and SPI has no meaning, SPI size = 0.
	 * RFC2408 3.1, first paragraph on page 40: ISAKMP SA is identified
	 * by cookie and SPI has no meaning, 0 <= SPI size <= 16.
	 * RFC2407 4.6.3.3, INITIAL-CONTACT is required to set to 16.
	 */
	if (type == ISAKMP_NTYPE_INITIAL_CONTACT)
		spisiz = sizeof(isakmp_index);
	else
		spisiz = 0;

	tlen = sizeof(*n) + spisiz;
	if (data)
		tlen += data->l;
	payload = vmalloc(tlen);
	if (payload == NULL) { 
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		return errno;
	}

	n = (struct isakmp_pl_n *)payload->v;
	n->h.np = ISAKMP_NPTYPE_NONE;
	n->h.len = htons(tlen);
	n->doi = htonl(iph1->rmconf->doitype);
	n->proto_id = IPSECDOI_PROTO_ISAKMP; /* XXX to be configurable ? */
	n->spi_size = spisiz;
	n->type = htons(type);
	if (spisiz)
		memcpy(n + 1, &iph1->index, sizeof(isakmp_index));
	if (data)
		memcpy((caddr_t)(n + 1) + spisiz, data->v, data->l);

	error = isakmp_info_send_common(iph1, payload, ISAKMP_NPTYPE_N, iph1->flags);
	vfree(payload);

	return error;
}

/*
 * send Notification payload (for IPsec SA) in Informational exchange
 */
int
isakmp_info_send_n2(iph2, type, data)
	struct ph2handle *iph2;
	int type;
	vchar_t *data;
{
	struct ph1handle *iph1 = iph2->ph1;
	vchar_t *payload = NULL;
	int tlen;
	int error = 0;
	struct isakmp_pl_n *n;
	struct saproto *pr;

	if (!iph2->approval)
		return EINVAL;

	pr = iph2->approval->head;

	/* XXX must be get proper spi */
	tlen = sizeof(*n) + pr->spisize;
	if (data)
		tlen += data->l;
	payload = vmalloc(tlen);
	if (payload == NULL) { 
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		return errno;
	}

	n = (struct isakmp_pl_n *)payload->v;
	n->h.np = ISAKMP_NPTYPE_NONE;
	n->h.len = htons(tlen);
	n->doi = htonl(IPSEC_DOI);		/* IPSEC DOI (1) */
	n->proto_id = pr->proto_id;		/* IPSEC AH/ESP/whatever*/
	n->spi_size = pr->spisize;
	n->type = htons(type);
	*(u_int32_t *)(n + 1) = pr->spi;
	if (data)
		memcpy((caddr_t)(n + 1) + pr->spisize, data->v, data->l);

	iph2->flags |= ISAKMP_FLAG_E;	/* XXX Should we do FLAG_A ? */
	error = isakmp_info_send_common(iph1, payload, ISAKMP_NPTYPE_N, iph2->flags);
	vfree(payload);

	return error;
}

/*
 * send Information
 * When ph1->skeyid_a == NULL, send message without encoding.
 */
int
isakmp_info_send_common(iph1, payload, np, flags)
	struct ph1handle *iph1;
	vchar_t *payload;
	u_int32_t np;
	int flags;
{
	struct ph2handle *iph2 = NULL;
	vchar_t *hash = NULL;
	struct isakmp *isakmp;
	struct isakmp_gen *gen;
	char *p;
	int tlen;
	int error = -1;

	/* add new entry to isakmp status table */
	iph2 = newph2();
	if (iph2 == NULL)
		goto end;

	iph2->dst = dupsaddr(iph1->remote);
	if (iph2->dst == NULL) {
		delph2(iph2);
		goto end;
	}
	iph2->src = dupsaddr(iph1->local);
	if (iph2->src == NULL) {
		delph2(iph2);
		goto end;
	}
	iph2->side = INITIATOR;
	iph2->status = PHASE2ST_START;
	iph2->msgid = isakmp_newmsgid2(iph1);

	/* get IV and HASH(1) if skeyid_a was generated. */
	if (iph1->skeyid_a != NULL) {
		iph2->ivm = oakley_newiv2(iph1, iph2->msgid);
		if (iph2->ivm == NULL) {
			delph2(iph2);
			goto end;
		}

		/* generate HASH(1) */
		hash = oakley_compute_hash1(iph1, iph2->msgid, payload);
		if (hash == NULL) {
			delph2(iph2);
			goto end;
		}

		/* initialized total buffer length */
		tlen = hash->l;
		tlen += sizeof(*gen);
	} else {
		/* IKE-SA is not established */
		hash = NULL;

		/* initialized total buffer length */
		tlen = 0;
	}
	if ((flags & ISAKMP_FLAG_A) == 0)
		iph2->flags = (hash == NULL ? 0 : ISAKMP_FLAG_E);
	else
		iph2->flags = (hash == NULL ? 0 : ISAKMP_FLAG_A);

	insph2(iph2);
	bindph12(iph1, iph2);

	tlen += sizeof(*isakmp) + payload->l;

	/* create buffer for isakmp payload */
	iph2->sendbuf = vmalloc(tlen);
	if (iph2->sendbuf == NULL) { 
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		goto err;
	}

	/* create isakmp header */
	isakmp = (struct isakmp *)iph2->sendbuf->v;
	memcpy(&isakmp->i_ck, &iph1->index.i_ck, sizeof(cookie_t));
	memcpy(&isakmp->r_ck, &iph1->index.r_ck, sizeof(cookie_t));
	isakmp->np = hash == NULL ? (np & 0xff) : ISAKMP_NPTYPE_HASH;
	isakmp->v = iph1->version;
	isakmp->etype = ISAKMP_ETYPE_INFO;
	isakmp->flags = iph2->flags;
	memcpy(&isakmp->msgid, &iph2->msgid, sizeof(isakmp->msgid));
	isakmp->len   = htonl(tlen);
	p = (char *)(isakmp + 1);

	/* create HASH payload */
	if (hash != NULL) {
		gen = (struct isakmp_gen *)p;
		gen->np = np & 0xff;
		gen->len = htons(sizeof(*gen) + hash->l);
		p += sizeof(*gen);
		memcpy(p, hash->v, hash->l);
		p += hash->l;
	}

	/* add payload */
	memcpy(p, payload->v, payload->l);
	p += payload->l;

#ifdef HAVE_PRINT_ISAKMP_C
	isakmp_printpacket(iph2->sendbuf, iph1->local, iph1->remote, 1);
#endif

	/* encoding */
	if (ISSET(isakmp->flags, ISAKMP_FLAG_E)) {
		vchar_t *tmp;

		tmp = oakley_do_encrypt(iph2->ph1, iph2->sendbuf, iph2->ivm->ive,
				iph2->ivm->iv);
		VPTRINIT(iph2->sendbuf);
		if (tmp == NULL)
			goto err;
		iph2->sendbuf = tmp;
	}

	/* HDR*, HASH(1), N */
	if (isakmp_send(iph2->ph1, iph2->sendbuf) < 0) {
		VPTRINIT(iph2->sendbuf);
		goto err;
	}

	plog(LLV_DEBUG, LOCATION, NULL,
		"sendto Information %s.\n", s_isakmp_nptype(np));

	/*
	 * don't resend notify message because peer can use Acknowledged
	 * Informational if peer requires the reply of the notify message.
	 */

	/* XXX If Acknowledged Informational required, don't delete ph2handle */
	error = 0;
	VPTRINIT(iph2->sendbuf);
	goto err;	/* XXX */

end:
	if (hash)
		vfree(hash);
	return error;

err:
	remph2(iph2);
	delph2(iph2);
	goto end;
}

/*
 * add a notify payload to buffer by reallocating buffer.
 * If buf == NULL, the function only create a notify payload.
 *
 * XXX Which is SPI to be included, inbound or outbound ?
 */
vchar_t *
isakmp_add_pl_n(buf0, np_p, type, pr, data)
	vchar_t *buf0;
	u_int8_t **np_p;
	int type;
	struct saproto *pr;
	vchar_t *data;
{
	vchar_t *buf = NULL;
	struct isakmp_pl_n *n;
	int tlen;
	int oldlen = 0;

	if (*np_p)
		**np_p = ISAKMP_NPTYPE_N;

	tlen = sizeof(*n) + pr->spisize;

	if (data)
		tlen += data->l;
	if (buf0) {
		oldlen = buf0->l;
		buf = vrealloc(buf0, buf0->l + tlen);
	} else
		buf = vmalloc(tlen);
	if (!buf) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get a payload buffer.\n");
		return NULL;
	}

	n = (struct isakmp_pl_n *)(buf->v + oldlen);
	n->h.np = ISAKMP_NPTYPE_NONE;
	n->h.len = htons(tlen);
	n->doi = htonl(IPSEC_DOI);		/* IPSEC DOI (1) */
	n->proto_id = pr->proto_id;		/* IPSEC AH/ESP/whatever*/
	n->spi_size = pr->spisize;
	n->type = htons(type);
	*(u_int32_t *)(n + 1) = pr->spi;	/* XXX */
	if (data)
		memcpy((caddr_t)(n + 1) + pr->spisize, data->v, data->l);

	/* save the pointer of next payload type */
	*np_p = &n->h.np;

	return buf;
}

void
purge_ipsec_spi(dst0, proto, spi, n)
	struct sockaddr *dst0;
	int proto;
	u_int32_t *spi;	/*network byteorder*/
	size_t n;
{
	vchar_t *buf = NULL;
	struct sadb_msg *msg, *next, *end;
	struct sadb_sa *sa;
	struct sadb_lifetime *lt;
	struct sockaddr *src, *dst;
	struct ph2handle *iph2;
	u_int64_t created;
	size_t i;
	caddr_t mhp[SADB_EXT_MAX + 1];
	unsigned num_purged = 0;

	plog(LLV_DEBUG2, LOCATION, NULL,
		 "purge_ipsec_spi:\n");
	plog(LLV_DEBUG2, LOCATION, NULL, "dst0: %s\n", saddr2str(dst0));
	plog(LLV_DEBUG2, LOCATION, NULL, "SPI: %08X\n", ntohl(spi[0]));

	buf = pfkey_dump_sadb(ipsecdoi2pfkey_proto(proto));
	if (buf == NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"pfkey_dump_sadb returned nothing.\n");
		return;
	}

	msg = (struct sadb_msg *)buf->v;
	end = (struct sadb_msg *)(buf->v + buf->l);

	while (msg < end) {
		if ((msg->sadb_msg_len << 3) < sizeof(*msg))
			break;
		next = (struct sadb_msg *)((caddr_t)msg + (msg->sadb_msg_len << 3));
		if (msg->sadb_msg_type != SADB_DUMP) {
			msg = next;
			continue;
		}

		if (pfkey_align(msg, mhp) || pfkey_check(mhp)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"pfkey_check (%s)\n", ipsec_strerror());
			msg = next;
			continue;
		}

		sa = (struct sadb_sa *)(mhp[SADB_EXT_SA]);
		if (!sa
		 || !mhp[SADB_EXT_ADDRESS_SRC]
		 || !mhp[SADB_EXT_ADDRESS_DST]) {
			msg = next;
			continue;
		}
		pk_fixup_sa_addresses(mhp);
		src = PFKEY_ADDR_SADDR(mhp[SADB_EXT_ADDRESS_SRC]);
		dst = PFKEY_ADDR_SADDR(mhp[SADB_EXT_ADDRESS_DST]);
		lt = (struct sadb_lifetime*)mhp[SADB_EXT_LIFETIME_HARD];
		if(lt != NULL)
			created = lt->sadb_lifetime_addtime;
		else
			created = 0;

		if (sa->sadb_sa_state != SADB_SASTATE_MATURE
		 && sa->sadb_sa_state != SADB_SASTATE_DYING) {
			msg = next;
			continue;
		}

		plog(LLV_DEBUG2, LOCATION, NULL, "src: %s\n", saddr2str(src));
		plog(LLV_DEBUG2, LOCATION, NULL, "dst: %s\n", saddr2str(dst));
		plog(LLV_DEBUG2, LOCATION, NULL, "spi: %u\n", ntohl(sa->sadb_sa_spi));

		/* XXX n^2 algorithm, inefficient */

		/* don't delete inbound SAs at the moment */
		/* XXX should we remove SAs with opposite direction as well? */
		if (cmpsaddr(dst0, dst) != CMPSADDR_MATCH) {
			msg = next;
			continue;
		}

		for (i = 0; i < n; i++) {
			plog(LLV_DEBUG, LOCATION, NULL,
				"check spi(packet)=%u spi(db)=%u.\n",
				ntohl(spi[i]), ntohl(sa->sadb_sa_spi));
			if (spi[i] != sa->sadb_sa_spi)
				continue;

			pfkey_send_delete(lcconf->sock_pfkey,
				msg->sadb_msg_satype,
				IPSEC_MODE_ANY,
				src, dst, sa->sadb_sa_spi);

			/*
			 * delete a relative phase 2 handler.
			 * continue to process if no relative phase 2 handler
			 * exists.
			 */
			iph2 = getph2bysaidx(src, dst, proto, spi[i]);
			if(iph2 != NULL){
				delete_spd(iph2, created);
				remph2(iph2);
				delph2(iph2);
			}

			plog(LLV_INFO, LOCATION, NULL,
				"purged IPsec-SA proto_id=%s spi=%u.\n",
				s_ipsecdoi_proto(proto),
				ntohl(spi[i]));
			num_purged++;
		}

		msg = next;
	}

	if (buf)
		vfree(buf);

	plog(LLV_DEBUG, LOCATION, NULL, "purged %u SAs.\n", num_purged);
}

/*
 * delete all phase2 sa relatived to the destination address
 * (except the phase2 within which the INITIAL-CONTACT was received).
 * Don't delete Phase 1 handlers on INITIAL-CONTACT, and don't ignore
 * an INITIAL-CONTACT if we have contacted the peer.  This matches the
 * Sun IKE behavior, and makes rekeying work much better when the peer
 * restarts.
 */
int
isakmp_info_recv_initialcontact(iph1, protectedph2)
	struct ph1handle *iph1;
	struct ph2handle *protectedph2;
{
	vchar_t *buf = NULL;
	struct sadb_msg *msg, *next, *end;
	struct sadb_sa *sa;
	struct sockaddr *src, *dst;
	caddr_t mhp[SADB_EXT_MAX + 1];
	int proto_id, i;
	struct ph2handle *iph2;
#if 0
	char *loc, *rem;
#endif

	plog(LLV_INFO, LOCATION, iph1->remote, "received INITIAL-CONTACT\n");

	if (f_local)
		return 0;

#if 0
	loc = racoon_strdup(saddrwop2str(iph1->local));
	rem = racoon_strdup(saddrwop2str(iph1->remote));
	STRDUP_FATAL(loc);
	STRDUP_FATAL(rem);

	/*
	 * Purge all IPSEC-SAs for the peer.  We can do this
	 * the easy way (using a PF_KEY SADB_DELETE extension)
	 * or we can do it the hard way.
	 */
	for (i = 0; i < pfkey_nsatypes; i++) {
		proto_id = pfkey2ipsecdoi_proto(pfkey_satypes[i].ps_satype);

		plog(LLV_INFO, LOCATION, NULL,
		    "purging %s SAs for %s -> %s\n",
		    pfkey_satypes[i].ps_name, loc, rem);
		if (pfkey_send_delete_all(lcconf->sock_pfkey,
		    pfkey_satypes[i].ps_satype, IPSEC_MODE_ANY,
		    iph1->local, iph1->remote) == -1) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "delete_all %s -> %s failed for %s (%s)\n",
			    loc, rem,
			    pfkey_satypes[i].ps_name, ipsec_strerror());
			goto the_hard_way;
		}

		deleteallph2(iph1->local, iph1->remote, proto_id);

		plog(LLV_INFO, LOCATION, NULL,
		    "purging %s SAs for %s -> %s\n",
		    pfkey_satypes[i].ps_name, rem, loc);
		if (pfkey_send_delete_all(lcconf->sock_pfkey,
		    pfkey_satypes[i].ps_satype, IPSEC_MODE_ANY,
		    iph1->remote, iph1->local) == -1) {
			plog(LLV_ERROR, LOCATION, NULL,
			    "delete_all %s -> %s failed for %s (%s)\n",
			    rem, loc,
			    pfkey_satypes[i].ps_name, ipsec_strerror());
			goto the_hard_way;
		}

		deleteallph2(iph1->remote, iph1->local, proto_id);
	}

	racoon_free(loc);
	racoon_free(rem);
	return 0;

 the_hard_way:
	racoon_free(loc);
	racoon_free(rem);
#endif

	buf = pfkey_dump_sadb(SADB_SATYPE_UNSPEC);
	if (buf == NULL) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"pfkey_dump_sadb returned nothing.\n");
		return 0;
	}

	msg = (struct sadb_msg *)buf->v;
	end = (struct sadb_msg *)(buf->v + buf->l);

	for (; msg < end; msg = next) {
		if ((msg->sadb_msg_len << 3) < sizeof(*msg))
			break;

		next = (struct sadb_msg *)((caddr_t)msg + (msg->sadb_msg_len << 3));
		if (msg->sadb_msg_type != SADB_DUMP)
			continue;

		if (pfkey_align(msg, mhp) || pfkey_check(mhp)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"pfkey_check (%s)\n", ipsec_strerror());
			continue;
		}

		if (mhp[SADB_EXT_SA] == NULL
		 || mhp[SADB_EXT_ADDRESS_SRC] == NULL
		 || mhp[SADB_EXT_ADDRESS_DST] == NULL)
			continue;

		sa = (struct sadb_sa *)mhp[SADB_EXT_SA];
		pk_fixup_sa_addresses(mhp);
		src = PFKEY_ADDR_SADDR(mhp[SADB_EXT_ADDRESS_SRC]);
		dst = PFKEY_ADDR_SADDR(mhp[SADB_EXT_ADDRESS_DST]);

		if (sa->sadb_sa_state != SADB_SASTATE_MATURE
		 && sa->sadb_sa_state != SADB_SASTATE_DYING)
			continue;

		/*
		 * RFC2407 4.6.3.3 INITIAL-CONTACT is the message that
		 * announces the sender of the message was rebooted.
		 * it is interpreted to delete all SAs which source address
		 * is the sender of the message.
		 * racoon only deletes SA which is matched both the
		 * source address and the destination accress.
		 */

		/*
		 * Check that the IP and port match. But this is not optimal,
		 * since NAT-T can make the peer have multiple different
		 * ports. Correct thing to do is delete all entries with
                 * same identity. -TT
                 */
		if ((cmpsaddr(iph1->local, src) != CMPSADDR_MATCH ||
		     cmpsaddr(iph1->remote, dst) != CMPSADDR_MATCH) &&
		    (cmpsaddr(iph1->local, dst) != CMPSADDR_MATCH ||
		     cmpsaddr(iph1->remote, src) != CMPSADDR_MATCH))
			continue;

		/*
		 * Make sure this is an SATYPE that we manage.
		 * This is gross; too bad we couldn't do it the
		 * easy way.
		 */
		for (i = 0; i < pfkey_nsatypes; i++) {
			if (pfkey_satypes[i].ps_satype ==
			    msg->sadb_msg_satype)
				break;
		}
		if (i == pfkey_nsatypes)
			continue;

		plog(LLV_INFO, LOCATION, NULL,
			"purging spi=%u.\n", ntohl(sa->sadb_sa_spi));
		pfkey_send_delete(lcconf->sock_pfkey,
			msg->sadb_msg_satype,
			IPSEC_MODE_ANY, src, dst, sa->sadb_sa_spi);

		/*
		 * delete a relative phase 2 handler.
		 * continue to process if no relative phase 2 handler
		 * exists.
		 */
		proto_id = pfkey2ipsecdoi_proto(msg->sadb_msg_satype);
		iph2 = getph2bysaidx(src, dst, proto_id, sa->sadb_sa_spi);
		if (iph2 && iph2 != protectedph2) {
			delete_spd(iph2, 0);
			remph2(iph2);
			delph2(iph2);
		}
	}

	vfree(buf);
	return 0;
}


#ifdef ENABLE_DPD
static int
isakmp_info_recv_r_u (iph1, ru, msgid)
	struct ph1handle *iph1;
	struct isakmp_pl_ru *ru;
	u_int32_t msgid;
{
	struct isakmp_pl_ru *ru_ack;
	vchar_t *payload = NULL;
	int tlen;
	int error = 0;

	plog(LLV_DEBUG, LOCATION, iph1->remote,
		 "DPD R-U-There received\n");

	/* XXX should compare cookies with iph1->index?
	   Or is this already done by calling function?  */
	tlen = sizeof(*ru_ack);
	payload = vmalloc(tlen);
	if (payload == NULL) { 
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get buffer to send.\n");
		return errno;
	}

	ru_ack = (struct isakmp_pl_ru *)payload->v;
	ru_ack->h.np = ISAKMP_NPTYPE_NONE;
	ru_ack->h.len = htons(tlen);
	ru_ack->doi = htonl(IPSEC_DOI);
	ru_ack->type = htons(ISAKMP_NTYPE_R_U_THERE_ACK);
	ru_ack->proto_id = IPSECDOI_PROTO_ISAKMP; /* XXX ? */
	ru_ack->spi_size = sizeof(isakmp_index);
	memcpy(ru_ack->i_ck, ru->i_ck, sizeof(cookie_t));
	memcpy(ru_ack->r_ck, ru->r_ck, sizeof(cookie_t));	
	ru_ack->data = ru->data;

	/* XXX Should we do FLAG_A ?  */
	error = isakmp_info_send_common(iph1, payload, ISAKMP_NPTYPE_N,
					ISAKMP_FLAG_E);
	vfree(payload);

	plog(LLV_DEBUG, LOCATION, NULL, "received a valid R-U-THERE, ACK sent\n");

	/* Should we mark tunnel as active ? */
	return error;
}

static int
isakmp_info_recv_r_u_ack (iph1, ru, msgid)
	struct ph1handle *iph1;
	struct isakmp_pl_ru *ru;
	u_int32_t msgid;
{
	u_int32_t seq;

	plog(LLV_DEBUG, LOCATION, iph1->remote,
		 "DPD R-U-There-Ack received\n");

	seq = ntohl(ru->data);
	if (seq <= iph1->dpd_last_ack || seq > iph1->dpd_seq) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			 "Wrong DPD sequence number (%d; last_ack=%d, seq=%d).\n", 
			 seq, iph1->dpd_last_ack, iph1->dpd_seq);
		return 0;
	}

	/* accept cookies in original or reversed order */
	if ((memcmp(ru->i_ck, iph1->index.i_ck, sizeof(cookie_t)) ||
	     memcmp(ru->r_ck, iph1->index.r_ck, sizeof(cookie_t))) &&
	    (memcmp(ru->r_ck, iph1->index.i_ck, sizeof(cookie_t)) ||
	     memcmp(ru->i_ck, iph1->index.r_ck, sizeof(cookie_t)))) {
		plog(LLV_ERROR, LOCATION, iph1->remote,
			 "Cookie mismatch in DPD ACK!.\n");
		return 0;
	}

	iph1->dpd_fails = 0;
	iph1->dpd_last_ack = seq;
	sched_cancel(&iph1->dpd_r_u);
	isakmp_sched_r_u(iph1, 0);

	plog(LLV_DEBUG, LOCATION, iph1->remote, "received an R-U-THERE-ACK\n");

	return 0;
}




/*
 * send DPD R-U-THERE payload in Informational exchange.
 */
static void
isakmp_info_send_r_u(sc)
	struct sched *sc;
{
	struct ph1handle *iph1 = container_of(sc, struct ph1handle, dpd_r_u);

	/* create R-U-THERE payload */
	struct isakmp_pl_ru *ru;
	vchar_t *payload = NULL;
	int tlen;
	int error = 0;

	plog(LLV_DEBUG, LOCATION, iph1->remote, "DPD monitoring....\n");

	if (iph1->status == PHASE1ST_EXPIRED) {
		/* This can happen after removing tunnels from the
		 * config file and then reloading.
		 * Such iph1 have rmconf=NULL, so return before the if
		 * block below.
		 */
		return;
	}

	if (iph1->dpd_fails >= iph1->rmconf->dpd_maxfails) {

		plog(LLV_INFO, LOCATION, iph1->remote,
			"DPD: remote (ISAKMP-SA spi=%s) seems to be dead.\n",
			isakmp_pindex(&iph1->index, 0));

		script_hook(iph1, SCRIPT_PHASE1_DEAD);
		evt_phase1(iph1, EVT_PHASE1_DPD_TIMEOUT, NULL);
		purge_remote(iph1);

		/* Do not reschedule here: phase1 is deleted,
		 * DPD will be reactivated when a new ph1 will be negociated
		 */
		return;
	}

	/* TODO: check recent activity to avoid useless sends... */

	tlen = sizeof(*ru);
	payload = vmalloc(tlen);
	if (payload == NULL) {
		plog(LLV_ERROR, LOCATION, NULL, 
			 "failed to get buffer for payload.\n");
		return;
	}
	ru = (struct isakmp_pl_ru *)payload->v;
	ru->h.np = ISAKMP_NPTYPE_NONE;
	ru->h.len = htons(tlen);
	ru->doi = htonl(IPSEC_DOI);
	ru->type = htons(ISAKMP_NTYPE_R_U_THERE);
	ru->proto_id = IPSECDOI_PROTO_ISAKMP; /* XXX ?*/
	ru->spi_size = sizeof(isakmp_index);

	memcpy(ru->i_ck, iph1->index.i_ck, sizeof(cookie_t));
	memcpy(ru->r_ck, iph1->index.r_ck, sizeof(cookie_t));

	if (iph1->dpd_seq == 0) {
		/* generate a random seq which is not too big */
		iph1->dpd_seq = iph1->dpd_last_ack = rand() & 0x0fff;
	}

	iph1->dpd_seq++;
	iph1->dpd_fails++;
	ru->data = htonl(iph1->dpd_seq);

	error = isakmp_info_send_common(iph1, payload, ISAKMP_NPTYPE_N, 0);
	vfree(payload);

	plog(LLV_DEBUG, LOCATION, iph1->remote,
		 "DPD R-U-There sent (%d)\n", error);

	/* Reschedule the r_u_there with a short delay,
	 * will be deleted/rescheduled if ACK received before */
	isakmp_sched_r_u(iph1, 1);

	plog(LLV_DEBUG, LOCATION, iph1->remote,
		 "rescheduling send_r_u (%d).\n", iph1->rmconf->dpd_retry);
}

/* Schedule a new R-U-THERE */
int
isakmp_sched_r_u(iph1, retry)
	struct ph1handle *iph1;
	int retry;
{
	if(iph1 == NULL ||
	   iph1->rmconf == NULL)
		return 1;


	if(iph1->dpd_support == 0 ||
	   iph1->rmconf->dpd_interval == 0)
		return 0;

	if(retry)
		sched_schedule(&iph1->dpd_r_u, iph1->rmconf->dpd_retry,
			       isakmp_info_send_r_u);
	else
		sched_schedule(&iph1->dpd_r_u, iph1->rmconf->dpd_interval,
			       isakmp_info_send_r_u);

	return 0;
}
#endif
