/*-
 * Copyright (c) 2015 Mindaugas Rasiukevicius <rmind at netbsd org>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _NPFKERN_H_
#define _NPFKERN_H_

#ifndef _KERNEL
#include <stdbool.h>
#include <inttypes.h>
#endif

struct mbuf;
struct ifnet;

#if defined(_NPF_STANDALONE) || !defined(__NetBSD__)
#define PFIL_IN		0x00000001	// incoming packet
#define PFIL_OUT	0x00000002	// outgoing packet
#endif

#define	NPF_NO_GC	0x01

typedef struct {
	const char *	(*getname)(struct ifnet *);
	struct ifnet *	(*lookup)(const char *);
	void		(*flush)(void *);
	void *		(*getmeta)(const struct ifnet *);
	void		(*setmeta)(struct ifnet *, void *);
} npf_ifops_t;

typedef struct {
	struct mbuf *	(*alloc)(int, int);
	void		(*free)(struct mbuf *);
	void *		(*getdata)(const struct mbuf *);
	struct mbuf *	(*getnext)(struct mbuf *);
	size_t		(*getlen)(const struct mbuf *);
	size_t		(*getchainlen)(const struct mbuf *);
	bool		(*ensure_contig)(struct mbuf **, size_t);
	bool		(*ensure_writable)(struct mbuf **, size_t);
} npf_mbufops_t;

int	npf_sysinit(unsigned);
void	npf_sysfini(void);

npf_t *	npf_create(int, const npf_mbufops_t *, const npf_ifops_t *);
int	npf_load(npf_t *, void *, npf_error_t *);
void	npf_gc(npf_t *);
void	npf_destroy(npf_t *);

void	npf_thread_register(npf_t *);
int	npf_packet_handler(npf_t *, struct mbuf **, struct ifnet *, int);
void	npf_ifmap_attach(npf_t *, struct ifnet *);
void	npf_ifmap_detach(npf_t *, struct ifnet *);
void	npf_stats(npf_t *, uint64_t *);

#endif
