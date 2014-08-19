/* $NetBSD: notifications.c,v 1.2.12.1 2014/08/20 00:05:09 tls Exp $ */

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

#include <stdlib.h>

#include "ldp.h"
#include "socketops.h"
#include "tlv.h"
#include "ldp_errors.h"
#include "notifications.h"

/* builds a notification TLV. Do not forget to free the result */
struct notification_tlv *
build_notification(uint32_t msg, uint32_t n)
{
	struct notification_tlv *t;
	t = malloc(sizeof(*t));

	if (!t) {
		fatalp("build_notification: malloc problem\n");
		return NULL;
	}

	t->type = htons(LDP_NOTIFICATION);
	t->length = htons(sizeof(struct notification_tlv) - TLV_TYPE_LENGTH);
	t->status = htons(TLV_STATUS);
	t->st_len = sizeof(struct notification_tlv) - 2 * TLV_TYPE_LENGTH;
	t->st_code = htonl(n);
	t->msg_id = htonl(msg);
	t->msg_type = 0;
	return t;

}

int 
send_notification(const struct ldp_peer * p, uint32_t msg, uint32_t code)
{
	struct notification_tlv *nt;
	int             rv;

	if (p->state != LDP_PEER_CONNECTED && p->state != LDP_PEER_ESTABLISHED)
		return -1;

	nt = build_notification(msg, code);

	rv = send_tlv(p, (struct tlv *) nt);
	free(nt);
	return rv;
}
