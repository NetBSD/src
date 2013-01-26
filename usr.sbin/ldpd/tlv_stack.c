/* $NetBSD: tlv_stack.c,v 1.6 2013/01/26 17:29:55 kefren Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include <arpa/inet.h>

#include <netmpls/mpls.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ldp.h"
#include "ldp_errors.h"
#include "ldp_peer.h"
#include "tlv.h"
#include "socketops.h"
#include "pdu.h"
#include "label.h"
#include "mpls_interface.h"
#include "tlv_stack.h"

uint8_t ldp_ceil8(int);

uint8_t 
ldp_ceil8(int x)
{
	if (x % 8 == 0)
		return x / 8;
	return x / 8 + 1;
}

int 
map_label(struct ldp_peer * p, struct fec_tlv * f, struct label_tlv * l)
{
	int             n;
	struct prefix_tlv *pref;
	union sockunion socktmp;

	if (ntohs(f->type) != TLV_FEC) {
		debugp("Invalid FEC TLV !\n");
		return LDP_E_BAD_FEC;
	}
	if (ntohs(l->type) != TLV_GENERIC_LABEL) {
		debugp("Invalid LABEL TLV! (0x%.4X)\n", ntohs(l->type));
		return LDP_E_BAD_LABEL;
	}
	/*
	 * Actually address field length is given only in length field in
	 * bits !
	 */

	n = ntohs(f->length);
	if (!n)
		return LDP_E_BAD_FEC;

	debugp("Label %u for:\n", ntohl(l->label));

	pref = (struct prefix_tlv *) (f + 1);
	memset (&socktmp, 0, sizeof(socktmp));

	/*
	 * Section 3.4.1
	 * Note that this version of LDP supports the use of multiple FEC
	 * Elements per FEC for the Label Mapping message only.  The use of
	 * multiple FEC Elements in other messages is not permitted in this
	 * version, and is a subject for future study.
	 */

	for (; n > 0; pref = (struct prefix_tlv *) ((unsigned char *) pref +
			ldp_ceil8(pref->prelen) + TLV_TYPE_LENGTH)) {
		n -= ldp_ceil8(pref->prelen) + TLV_TYPE_LENGTH;
		if (ntohs(pref->af) == LDP_AF_INET) {
			socktmp.sa.sa_family = AF_INET;
			socktmp.sa.sa_len = sizeof(socktmp.sin);
		} else if (ntohs(pref->af) == LDP_AF_INET6) {
			socktmp.sa.sa_family = AF_INET6;
			socktmp.sa.sa_len = sizeof(socktmp.sin6);
		} else {
			warnp("BAD ADDRESS FAMILY (%d) ! (prefix type %d, "
			    "length %d)\n", ntohs(pref->af), pref->type,
			    pref->prelen);
			return LDP_E_BAD_AF;
		}
		switch(pref->type) {
		    case FEC_PREFIX:
		    case FEC_HOST:
			if (socktmp.sa.sa_family == AF_INET)
				memcpy(&socktmp.sin.sin_addr, &pref->prefix,
				    ldp_ceil8(pref->prelen));
			else
				memcpy(&socktmp.sin6.sin6_addr, &pref->prefix,
				    ldp_ceil8(pref->prelen));
			debugp("Prefix/Host add: %s/%d\n", satos(&socktmp.sa),
			    pref->prelen);

			ldp_peer_add_mapping(p, &socktmp.sa, pref->prelen,
			    ntohl(l->label));

			/* Try to change RIB only if label is installed */
			if (label_get_by_prefix(&socktmp.sa, pref->prelen) != NULL)
				mpls_add_label(p, NULL, &socktmp.sa, pref->prelen,
				    ntohl(l->label), 1);
			break;
		    case FEC_WILDCARD:
			fatalp("LDP: Wildcard add from peer %s\n",
			    satos(p->address));
			return LDP_E_BAD_FEC;
		    default:
			fatalp("Unknown FEC type %d\n", pref->type);
			return LDP_E_BAD_FEC;
		}
	}

	return LDP_E_OK;
}

int 
withdraw_label(struct ldp_peer * p, struct fec_tlv * f)
{
	int             n;
	struct prefix_tlv *pref;
	union sockunion socktmp;
	struct label *lab;

	if (ntohs(f->type) != TLV_FEC) {
		debugp("Invalid FEC TLV !\n");
		return LDP_E_BAD_FEC;
	}
	n = ntohs(f->length);
	if (!n)
		return LDP_E_BAD_FEC;

	pref = (struct prefix_tlv *) & f[1];

	memset(&socktmp, 0, sizeof(socktmp));
	if (ntohs(pref->af) == LDP_AF_INET) {
		socktmp.sa.sa_family = AF_INET;
		socktmp.sa.sa_len = sizeof(socktmp.sin);
	} else if (ntohs(pref->af) != LDP_AF_INET6) {
		socktmp.sa.sa_family = AF_INET6;
		socktmp.sa.sa_len = sizeof(socktmp.sin6);
	} else {
		warnp("WITHDRAW: Bad AF (%d)! (prefix type %d, length %d)\n",
		    ntohs(pref->af), pref->type, pref->prelen);
		return LDP_E_BAD_AF;
	}
	switch(pref->type) {
	    case FEC_PREFIX:
	    case FEC_HOST:
		if (socktmp.sa.sa_family == AF_INET)
			memcpy(&socktmp.sin.sin_addr, &pref->prefix,
			    ldp_ceil8(pref->prelen));
		else
			memcpy(&socktmp.sin6.sin6_addr, &pref->prefix,
			    ldp_ceil8(pref->prelen));
		debugp("Prefix/Host withdraw: %s/%d\n", satos(&socktmp.sa),
		    pref->prelen);

		/* Delete mapping */
		ldp_peer_delete_mapping(p, &socktmp.sa, pref->prelen);

		/* Get label, see if we're pointing to this peer
		 * if so, send withdraw, reattach IP route and announce
		 * POP Label
		 */
		lab = label_get_by_prefix(&socktmp.sa, pref->prelen);
		if ((lab) && (lab->p == p)) {
			change_local_label(lab, MPLS_LABEL_IMPLNULL);
			label_reattach_route(lab, LDP_READD_CHANGE);
		}
		break;
	    case FEC_WILDCARD:
		fatalp("LDP neighbour %s: Wildcard withdraw !!!\n",
		    satos(p->address));
		ldp_peer_delete_mapping(p, NULL, 0);
		label_reattach_all_peer_labels(p, LDP_READD_CHANGE);
		break;
	    default:
		fatalp("Unknown FEC type %d\n", pref->type);
		return LDP_E_BAD_FEC;
	}

	return LDP_E_OK;
}


/*
 * In case of label redraw, reuse the same buffer to send label release
 * Simply replace type and message id
 */
void 
prepare_release(struct tlv * v)
{
	struct label_map_tlv *t;

	t = (struct label_map_tlv *) v;

	t->type = htons(LDP_LABEL_RELEASE);
	t->messageid = htonl(get_message_id());
}

/* Sends a label mapping */
void 
send_label_tlv(struct ldp_peer * peer, struct sockaddr * addr,
    uint8_t prefixlen, uint32_t label, struct label_request_tlv *lrt)
{
	struct label_map_tlv *lmt;
	struct fec_tlv *fec;
	struct prefix_tlv *p;
	struct label_tlv *l;

	/*
	 * Ok, so we have label map tlv that contains fec tlvs and label tlv
	 * but fec tlv contains prefix or host tlvs and prefix or host tlvs
	 * contains the network. After that we may have optional parameters
	 * Got it ?
	 */

	lmt = malloc(
		sizeof(struct label_map_tlv) +
		sizeof(struct fec_tlv) +
		sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
			ldp_ceil8(prefixlen) +
		sizeof(struct label_tlv) +
	 /* Label request optional parameter  */
	 	(lrt != NULL ? sizeof(struct label_request_tlv) : 0) );

	if (!lmt) {
		fatalp("send_label_tlv: malloc problem\n");
		return;
	}

	lmt->type = htons(LDP_LABEL_MAPPING);
	lmt->length = htons(sizeof(struct label_map_tlv) - TLV_TYPE_LENGTH
	    + sizeof(struct fec_tlv)
	    + sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
		ldp_ceil8(prefixlen)
	    + sizeof(struct label_tlv) +
	    + (lrt != NULL ? sizeof(struct label_request_tlv) : 0));
	lmt->messageid = htonl(get_message_id());

	/* FEC TLV */
	fec = (struct fec_tlv *) & lmt[1];
	fec->type = htons(TLV_FEC);
	fec->length = htons(sizeof(struct fec_tlv) - TLV_TYPE_LENGTH
	    + sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
		ldp_ceil8(prefixlen));

	/* Now let's do the even a dirtier job: PREFIX TLV */
	p = (struct prefix_tlv *) & fec[1];
	/*
	 * RFC5036 obsoletes FEC_HOST
	 *
	 * if (prefixlen == 32) p->type = FEC_HOST; else
	 */
	p->type = FEC_PREFIX;
	p->af = htons(LDP_AF_INET);
	p->prelen = prefixlen;
	memcpy(&p->prefix, addr, ldp_ceil8(prefixlen));

	/* LABEL TLV */
	l = (struct label_tlv *) ((unsigned char *) p +
		sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
		ldp_ceil8(prefixlen));
	l->type = htons(TLV_GENERIC_LABEL);
	l->length = htons(sizeof(l->label));
	l->label = htonl(label);

	/* Label request optional parameter */
	if (lrt)
		memcpy(((char*)l) + TLV_TYPE_LENGTH + ntohs(l->length),
			lrt, htons(lrt->length) + TLV_TYPE_LENGTH);

	/* Wow, seems we're ready */
	send_tlv(peer, (struct tlv *) lmt);
	free(lmt);
}

void 
send_label_tlv_to_all(struct sockaddr * addr, uint8_t prefixlen, uint32_t label)
{
	struct ldp_peer *p;
	SLIST_FOREACH(p, &ldp_peer_head, peers)
		send_label_tlv(p, addr, prefixlen, label, NULL);
}

/*
 * Send all local labels to a peer
 */
void 
send_all_bindings(struct ldp_peer * peer)
{
	struct label *l;

	SLIST_FOREACH(l, &label_head, labels)
	   send_label_tlv(peer, &l->so_dest.sa,
		from_union_to_cidr(&l->so_pref), l->binding, NULL);

}

/* Sends a label WITHDRAW */
void 
send_withdraw_tlv(struct ldp_peer * peer, struct sockaddr * addr,
    uint8_t prefixlen)
{
	struct label_map_tlv *lmt;
	struct fec_tlv *fec;
	struct prefix_tlv *p;

	/*
	 * Ok, so we have label map tlv that contains fec tlvs but fec tlv
	 * contains prefix or host tlvs and prefix or host tlvs contains the
	 * network. Yes, we don't have to announce label here
	 */

	lmt = malloc(sizeof(struct label_map_tlv)
	      + sizeof(struct fec_tlv)
	      + sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
			ldp_ceil8(prefixlen));

	if (!lmt) {
		fatalp("send_withdraw_tlv: malloc problem\n");
		return;
		}

	lmt->type = htons(LDP_LABEL_WITHDRAW);
	lmt->length = htons(sizeof(struct label_map_tlv) - TLV_TYPE_LENGTH
	    + sizeof(struct fec_tlv)
	    + sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
		ldp_ceil8(prefixlen));
	lmt->messageid = htonl(get_message_id());

	/* FEC TLV */
	fec = (struct fec_tlv *) & lmt[1];
	fec->type = htons(TLV_FEC);
	fec->length = htons(sizeof(struct fec_tlv) - TLV_TYPE_LENGTH
	    + sizeof(struct prefix_tlv) - sizeof(struct in_addr) +
		ldp_ceil8(prefixlen));

	/* Now the even dirtier job: PREFIX TLV */
	p = (struct prefix_tlv *) & fec[1];
	/*
	 * RFC5036 obsoletes FEC_HOST
	 *
	 * if (prefixlen == 32) p->type = FEC_HOST; else
	 */
	p->type = FEC_PREFIX;
	p->af = htons(LDP_AF_INET);
	p->prelen = prefixlen;
	memcpy(&p->prefix, addr, ldp_ceil8(prefixlen));

	/* Wow, seems we're ready */
	send_tlv(peer, (struct tlv *) lmt);
	free(lmt);
}

void 
send_withdraw_tlv_to_all(struct sockaddr * addr, uint8_t prefixlen)
{
	struct ldp_peer *p;
	SLIST_FOREACH(p, &ldp_peer_head, peers)
		send_withdraw_tlv(p, addr, prefixlen);
}

int
request_respond(struct ldp_peer *p, struct label_map_tlv *lmt,
    struct fec_tlv *fec)
{
	struct prefix_tlv *pref;
	union sockunion socktmp;
	struct label *lab;
	struct label_request_tlv lrm;

	if (ntohs(fec->type) != TLV_FEC || fec->length == 0) {
		debugp("Invalid FEC TLV !\n");
		return LDP_E_BAD_FEC;
	}
	pref = (struct prefix_tlv *) (fec + 1);

	memset(&socktmp, 0, sizeof(socktmp));
	if (ntohs(pref->af) == LDP_AF_INET) {
		socktmp.sa.sa_family = AF_INET;
		socktmp.sa.sa_len = sizeof(socktmp.sin);
	} else if (ntohs(pref->af) == LDP_AF_INET6) {
		socktmp.sa.sa_family = AF_INET6;
		socktmp.sa.sa_len = sizeof(socktmp.sin6);
	} else {
		debugp("request_respond: Bad address family\n");
		return LDP_E_BAD_AF;
	}

	switch (pref->type) {
		case FEC_PREFIX:
		case FEC_HOST:

		if (socktmp.sa.sa_family == AF_INET)
			memcpy(&socktmp.sin.sin_addr, &pref->prefix,
			    ldp_ceil8(pref->prelen));
		else /* AF_INET6 */
			memcpy(&socktmp.sin6.sin6_addr, &pref->prefix,
			    ldp_ceil8(pref->prelen));
		debugp("Prefix/Host request: %s/%d\n", satos(&socktmp.sa),
			pref->prelen);

		lab = label_get_by_prefix(&socktmp.sa, pref->prelen);
		if (!lab)
			return LDP_E_NO_SUCH_ROUTE;
		lrm.type = htons(TLV_LABEL_REQUEST);
		/* XXX - use sizeof */
		lrm.length = htons(socktmp.sa.sa_family == AF_INET ? 4 : 16);
		lrm.messageid = lmt->messageid;
		send_label_tlv(p, &socktmp.sa, pref->prelen, lab->binding, &lrm);
		break;

		case FEC_WILDCARD:
		/*
		 * Section 3.4.1
		 * To be used only in the Label Withdraw and Label Release
		 */
		/* Fallthrough */
		default:

		fatalp("Invalid FEC type\n");
		return LDP_E_BAD_FEC;
	}
	return LDP_E_OK;
}
