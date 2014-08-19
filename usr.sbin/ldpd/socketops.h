/* $NetBSD: socketops.h,v 1.2.8.3 2014/08/20 00:05:09 tls Exp $ */

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
void	send_initialize(const struct ldp_peer *);
void	keep_alive(const struct ldp_peer *);
void	recv_session_pdu(struct ldp_peer *);
int	send_message(const struct ldp_peer *, const struct ldp_pdu *,
	const struct tlv *);
int	send_tlv(const struct ldp_peer *, const struct tlv *);
int	send_addresses(const struct ldp_peer *);

struct	hello_info {
	union sockunion transport_address;
	struct in_addr ldp_id;
	int keepalive;
	SLIST_ENTRY(hello_info) infos;
};
SLIST_HEAD(,hello_info) hello_info_head;

struct	hello_socket {
	int type, socket;
	SLIST_ENTRY(hello_socket) listentry;
};
SLIST_HEAD(,hello_socket) hello_socket_head;

#endif	/* !_SOCKETOPS_H_ */
