/* $NetBSD: tlv_stack.h,v 1.2 2013/01/26 17:29:55 kefren Exp $ */

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

#ifndef _TLV_STACK_H_
#define _TLV_STACK_H_

#include "ldp_peer.h"
#include "tlv.h"

#define	FEC_WILDCARD	0x01
#define	FEC_PREFIX	0x02
#define	FEC_HOST	0x03

int	map_label(struct ldp_peer *, struct fec_tlv *, struct label_tlv *);
int	withdraw_label(struct ldp_peer *, struct fec_tlv *);
void	prepare_release(struct tlv *);
void	send_label_tlv(struct ldp_peer *, struct sockaddr *, uint8_t,
		uint32_t, struct label_request_tlv *);
void	send_label_tlv_to_all(struct sockaddr *, uint8_t, uint32_t);
void	send_all_bindings(struct ldp_peer *);
void	send_withdraw_tlv(struct ldp_peer *, struct sockaddr *, uint8_t);
void	send_withdraw_tlv_to_all(struct sockaddr *, uint8_t);
int	request_respond(struct ldp_peer *, struct label_map_tlv *,
		struct fec_tlv *);

#endif	/* !_TLV_STACK_H_ */
