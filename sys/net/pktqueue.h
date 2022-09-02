/*	$NetBSD: pktqueue.h,v 1.8 2022/09/02 05:50:36 thorpej Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mindaugas Rasiukevicius.
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

#ifndef _NET_PKTQUEUE_H_
#define _NET_PKTQUEUE_H_

#if !defined(_KERNEL)
#error "not supposed to be exposed to userland."
#endif

#include <sys/sysctl.h>

struct mbuf;

typedef struct pktqueue pktqueue_t;

typedef enum { PKTQ_MAXLEN, PKTQ_NITEMS, PKTQ_DROPS } pktq_count_t;

typedef uint32_t (*pktq_rps_hash_func_t)(const struct mbuf *);
#define PKTQ_RPS_HASH_NAME_LEN 32

pktqueue_t *	pktq_create(size_t, void (*)(void *), void *);
void		pktq_destroy(pktqueue_t *);

bool		pktq_enqueue(pktqueue_t *, struct mbuf *, const u_int);
struct mbuf *	pktq_dequeue(pktqueue_t *);
void		pktq_barrier(pktqueue_t *);
void		pktq_ifdetach(void);
void		pktq_flush(pktqueue_t *);
int		pktq_set_maxlen(pktqueue_t *, size_t);

uint32_t	pktq_rps_hash(const pktq_rps_hash_func_t *,
		    const struct mbuf *);
extern const pktq_rps_hash_func_t	pktq_rps_hash_default;

void		pktq_sysctl_setup(pktqueue_t *, struct sysctllog **,
		    const struct sysctlnode *, const int);

int		sysctl_pktq_rps_hash_handler(SYSCTLFN_PROTO);

#endif
