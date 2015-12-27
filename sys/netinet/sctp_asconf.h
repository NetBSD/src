/*	$KAME: sctp_asconf.h,v 1.8 2005/03/06 16:04:16 itojun Exp $	*/
/*	$NetBSD: sctp_asconf.h,v 1.1.2.2 2015/12/27 12:10:07 skrll Exp $ */

#ifndef _NETINET_SCTP_ASCONF_H_
#define _NETINET_SCTP_ASCONF_H_

/*
 * Copyright (c) 2001, 2002, 2003, 2004 Cisco Systems, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY CISCO SYSTEMS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CISCO SYSTEMS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <sys/malloc.h>

#if defined(_KERNEL)

extern void sctp_asconf_cleanup(struct sctp_tcb *, struct sctp_nets *);

extern struct mbuf *sctp_compose_asconf(struct sctp_tcb *);

extern void sctp_handle_asconf(struct mbuf *, unsigned int, struct sctp_asconf_chunk *,
	struct sctp_tcb *, struct sctp_nets *);

extern void sctp_handle_asconf_ack(struct mbuf *, int,
	struct sctp_asconf_ack_chunk *, struct sctp_tcb *, struct sctp_nets *);

extern uint32_t sctp_addr_mgmt_ep_sa(struct sctp_inpcb *, struct sockaddr *,
	uint16_t);

extern void sctp_add_ip_address(struct ifaddr *);

extern void sctp_delete_ip_address(struct ifaddr *);

extern int32_t sctp_set_primary_ip_address_sa(struct sctp_tcb *,
	struct sockaddr *);

extern void sctp_set_primary_ip_address(struct ifaddr *);

extern void sctp_check_address_list(struct sctp_tcb *, struct mbuf *, int, int,
	struct sockaddr *, uint16_t, uint16_t, uint16_t, uint16_t);

#endif /* _KERNEL */

#endif /* !_NETINET_SCTP_ASCONF_H_ */
