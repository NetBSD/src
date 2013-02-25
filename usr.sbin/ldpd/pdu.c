/* $NetBSD: pdu.c,v 1.1.12.1 2013/02/25 00:30:43 tls Exp $ */

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

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>

#include "socketops.h"
#include "ldp_errors.h"
#include "ldp.h"
#include "ldp_peer.h"
#include "notifications.h"
#include "pdu.h"

uint
get_pdu(unsigned char *s, struct ldp_pdu * p)
{
	struct ldp_pdu *p1;
	p1 = (struct ldp_pdu *) s;

	p->version = ntohs(p1->version);
	p->length = ntohs(p1->length);
	memcpy(&p->ldp_id, &p1->ldp_id, sizeof(struct in_addr));
	p->label_space = ntohs(p1->label_space);

	return MIN_PDU_SIZE;
}

/* Checks an incoming PDU for size and version */
int 
check_recv_pdu(struct ldp_peer * p, struct ldp_pdu * rpdu, int c)
{
	struct notification_tlv *notiftlv;

	/* Avoid underflow */
	if (c < MIN_PDU_SIZE)
		return LDP_E_BAD_LENGTH;

	if (p->ldp_id.s_addr != rpdu->ldp_id.s_addr) {
		fatalp("Invalid LDP ID %s received from ",
		    inet_ntoa(rpdu->ldp_id));
		fatalp("%s\n", inet_ntoa(p->ldp_id));
		notiftlv = build_notification(0,
		    NOTIF_FATAL | NOTIF_BAD_LDP_ID);
		send_tlv(p, (struct tlv *) notiftlv);
		free(notiftlv);
		return LDP_E_BAD_ID;
	}

	/* Check PDU for right LDP version */
	if (ntohs(rpdu->version) != LDP_VERSION) {
		fatalp("Invalid PDU version received from %s (%d)\n",
		       satos(p->address), ntohs(rpdu->version));
		notiftlv = build_notification(0,
		    NOTIF_FATAL | NOTIF_BAD_LDP_VER);
		send_tlv(p, (struct tlv *) notiftlv);
		free(notiftlv);
		return LDP_E_BAD_VERSION;
	}
	/* Check PDU for length validity */
	if (ntohs(rpdu->length) > c - PDU_VER_LENGTH) {
		fatalp("Invalid PDU length received from %s (announced %d, "
		    "received %d)\n", satos(p->address),
		    ntohs(rpdu->length), (int) (c - PDU_VER_LENGTH));
		notiftlv = build_notification(0,
		    NOTIF_FATAL | NOTIF_BAD_PDU_LEN);
		send_tlv(p, (struct tlv *) notiftlv);
		free(notiftlv);
		return LDP_E_BAD_LENGTH;
	}
	return LDP_E_OK;
}
