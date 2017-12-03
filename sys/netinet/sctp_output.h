/*	$KAME: sctp_output.h,v 1.14 2005/03/06 16:04:18 itojun Exp $	*/
/*	$NetBSD: sctp_output.h,v 1.1.18.2 2017/12/03 11:39:04 jdolecek Exp $ */

#ifndef __SCTP_OUTPUT_H__
#define __SCTP_OUTPUT_H__

/*
 * Copyright (C) 2002, 2003, 2004 Cisco Systems Inc,
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



#include <netinet/sctp_header.h>
#if defined(_KERNEL)
void sctp_send_initiate(struct sctp_inpcb *, struct sctp_tcb *);

void sctp_send_initiate_ack(struct sctp_inpcb *, struct sctp_tcb *,
    struct mbuf *, int, int, struct sctphdr *, struct sctp_init_chunk *);

struct mbuf *sctp_arethere_unrecognized_parameters(struct mbuf *, int, int *,
    struct sctp_chunkhdr *);
void sctp_queue_op_err(struct sctp_tcb *, struct mbuf *);

int sctp_send_cookie_echo(struct mbuf *, int, struct sctp_tcb *,
    struct sctp_nets *);
int sctp_send_cookie_ack(struct sctp_tcb *);

void sctp_send_heartbeat_ack(struct sctp_tcb *, struct mbuf *, int, int,
    struct sctp_nets *);

int sctp_is_addr_restricted(struct sctp_tcb *, struct sockaddr *);

struct in_addr sctp_ipv4_source_address_selection(struct sctp_inpcb *,
    struct sctp_tcb *, struct route *, struct sctp_nets *, int);


struct in6_addr sctp_ipv6_source_address_selection(struct sctp_inpcb *,
	struct sctp_tcb *, struct route *, struct sctp_nets *, int);


int sctp_send_shutdown(struct sctp_tcb *, struct sctp_nets *);

int sctp_send_shutdown_ack(struct sctp_tcb *, struct sctp_nets *);

int sctp_send_shutdown_complete(struct sctp_tcb *, struct sctp_nets *);

int sctp_send_shutdown_complete2(struct mbuf *, int, struct sctphdr *);

int sctp_send_asconf(struct sctp_tcb *, struct sctp_nets *);

int sctp_send_asconf_ack(struct sctp_tcb *, uint32_t);

int sctp_get_frag_point(struct sctp_tcb *, struct sctp_association *);

void sctp_toss_old_cookies(struct sctp_association *);

void sctp_toss_old_asconf(struct sctp_tcb *);

void sctp_fix_ecn_echo(struct sctp_association *);

int sctp_output(struct sctp_inpcb *, struct mbuf *, struct sockaddr *,
    struct mbuf *, struct lwp *, int);

int sctp_chunk_output(struct sctp_inpcb *, struct sctp_tcb *, int);
void sctp_send_abort_tcb(struct sctp_tcb *, struct mbuf *);

void send_forward_tsn(struct sctp_tcb *, struct sctp_association *);

void sctp_send_sack(struct sctp_tcb *);

int sctp_send_hb(struct sctp_tcb *, int, struct sctp_nets *);

void sctp_send_ecn_echo(struct sctp_tcb *, struct sctp_nets *, uint32_t);

void
sctp_send_packet_dropped(struct sctp_tcb *, struct sctp_nets *, struct mbuf *,
    int, int);

void sctp_send_cwr(struct sctp_tcb *, struct sctp_nets *, uint32_t);

void
sctp_send_str_reset_ack(struct sctp_tcb *, struct sctp_stream_reset_request *);

void
sctp_send_str_reset_req(struct sctp_tcb *, int, uint16_t *, uint8_t, uint8_t);

void sctp_send_abort(struct mbuf *, int, struct sctphdr *, uint32_t,
    struct mbuf *);

void sctp_send_operr_to(struct mbuf *, int, struct mbuf *, uint32_t);

int
sctp_sosend(struct socket *, struct sockaddr *, struct uio *,
    struct mbuf *, struct mbuf *, int, struct lwp *);

#endif
#endif
