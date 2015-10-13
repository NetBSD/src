/*	$KAME: sctp_indata.h,v 1.9 2005/03/06 16:04:17 itojun Exp $	*/
/*	$NetBSD: sctp_indata.h,v 1.1 2015/10/13 21:28:35 rjs Exp $ */

#ifndef __SCTP_INDATA_H__
#define __SCTP_INDATA_H__

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


#if defined(_KERNEL)
int sctp_deliver_data(struct sctp_tcb *, struct sctp_association *,
    struct sctp_tmit_chunk *, int);

void sctp_set_rwnd(struct sctp_tcb *, struct sctp_association *);

void sctp_handle_sack(struct sctp_sack_chunk *, struct sctp_tcb *,
	struct sctp_nets *, int *);

/* draft-ietf-tsvwg-usctp */
void sctp_handle_forward_tsn(struct sctp_tcb *,
	struct sctp_forward_tsn_chunk *, int *);

struct sctp_tmit_chunk *
sctp_try_advance_peer_ack_point(struct sctp_tcb *, struct sctp_association *);

void sctp_service_queues(struct sctp_tcb *, struct sctp_association *, int have_lock);

void sctp_update_acked(struct sctp_tcb *, struct sctp_shutdown_chunk *,
	struct sctp_nets *, int *);

int sctp_process_data(struct mbuf **, int, int *, int, struct sctphdr *,
    struct sctp_inpcb *, struct sctp_tcb *, struct sctp_nets *, u_int32_t *);

void sctp_sack_check(struct sctp_tcb *, int, int, int *);
#endif
#endif
