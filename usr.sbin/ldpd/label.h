/* $NetBSD: label.h,v 1.1.12.1 2013/02/25 00:30:43 tls Exp $ */

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

#ifndef _LABEL_H_
#define _LABEL_H_

#include <sys/queue.h>

#include "mpls_routes.h"
#include "ldp_peer.h"

#define	LDP_READD_NODEL		0
#define	LDP_READD_CHANGE	1
#define	LDP_READD_NOCHANGE	2

/*
 * MPLS label descriptor
 *
 * so_dest, so_pref and so_gate are the prefix identification and its GW
 * binding is the local label
 * label is the peer associated label
 */
struct label {
	union sockunion so_dest, so_pref, so_gate;
	int binding, label;
	struct ldp_peer *p;
	SLIST_ENTRY(label) labels;
};
SLIST_HEAD(,label) label_head;

void            label_init(void);
struct label *	label_add(union sockunion *, union sockunion *,
	  union sockunion *, uint32_t, struct ldp_peer *, uint32_t);
void            label_del(struct label *);
void            del_all_peer_labels(struct ldp_peer*, int);
void		label_reattach_all_peer_labels(struct ldp_peer*, int);
void            label_del_by_binding(uint32_t, int);
struct label *	label_get(union sockunion *sodest, union sockunion *sopref);
struct label *	label_get_by_prefix(const struct sockaddr *, int);
uint32_t	get_free_local_label(void);
void		change_local_label(struct label*, uint32_t);
void		label_reattach_route(struct label*, int);

#endif /* !_LABEL_H_ */
