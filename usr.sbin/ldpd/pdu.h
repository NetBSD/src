/* $NetBSD: pdu.h,v 1.1.12.1 2013/02/25 00:30:43 tls Exp $ */

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

#ifndef _PDU_H_
#define _PDU_H_

#include <sys/types.h>
#include <netinet/in.h>

#include "ldp_peer.h"

#define	MIN_PDU_SIZE 10
#define	MAX_PDU_SIZE 1300

#define	PDU_VER_LENGTH (sizeof(uint16_t) + sizeof(uint16_t))
#define	PDU_PAYLOAD_LENGTH (sizeof(struct in_addr) + sizeof(uint16_t))

struct ldp_pdu {
	uint16_t version;
	uint16_t length;
	/* draft-ietf-mpls-ldp-ipv6-07 keeps this IPv4 only for now */
	struct in_addr  ldp_id;
	uint16_t label_space;
}               __packed;


uint	get_pdu(unsigned char *, struct ldp_pdu *);
int	check_recv_pdu(struct ldp_peer *, struct ldp_pdu *, int);


#endif	/* !_PDU_H_ */
