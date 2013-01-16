/* $NetBSD: socketops.h,v 1.2.2.1 2013/01/16 05:34:09 yamt Exp $ */

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

#ifndef _SOCKETOPS_H_
#define _SOCKETOPS_H_

#include "mpls_routes.h"
#include "ldp_peer.h"
#include "pdu.h"
#include "tlv.h"

/* Address families from RFC1700 */
#define	LDP_AF_INET	1
#define	LDP_AF_INET6	2

int	set_ttl(int);
int	create_hello_sockets(void);
int	create_listening_socket(void);
void	send_hello(void);
int	get_message_id(void);
int	the_big_loop(void);
void	new_peer_connection(void);
void	send_initialize(struct ldp_peer *);
void	keep_alive(struct ldp_peer *);
void	recv_session_pdu(struct ldp_peer *);
int	send_message(struct ldp_peer *, struct ldp_pdu *, struct tlv *);
int	send_tlv(struct ldp_peer *, struct tlv *);
int	send_addresses(struct ldp_peer *);

struct	hello_info {
	struct in_addr address, transport_address, ldp_id;
	int keepalive;
	SLIST_ENTRY(hello_info) infos;
};
SLIST_HEAD(,hello_info) hello_info_head;

#endif	/* !_SOCKETOPS_H_ */
