/*-
 * Copyright (c) 2015-2020 Mindaugas Rasiukevicius <rmind at netbsd org>
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
	const char *	(*getname)(npf_t *, struct ifnet *);
	struct ifnet *	(*lookup)(npf_t *, const char *);
	void		(*flush)(npf_t *, void *);
	void *		(*getmeta)(npf_t *, const struct ifnet *);
	void		(*setmeta)(npf_t *, struct ifnet *, void *);
} npf_ifops_t;

typedef struct {
	struct mbuf *	(*alloc)(npf_t *, unsigned, size_t);
	void		(*free)(struct mbuf *);
	void *		(*getdata)(const struct mbuf *);
	struct mbuf *	(*getnext)(struct mbuf *);
	size_t		(*getlen)(const struct mbuf *);
	size_t		(*getchainlen)(const struct mbuf *);
	bool		(*ensure_contig)(struct mbuf **, size_t);
	bool		(*ensure_writable)(struct mbuf **, size_t);
	int		(*get_tag)(const struct mbuf *, uint32_t *);
	int		(*set_tag)(struct mbuf *, uint32_t);
} npf_mbufops_t;

int	npfk_sysinit(unsigned);
void	npfk_sysfini(void);

npf_t *	npfk_create(int, const npf_mbufops_t *, const npf_ifops_t *, void *);
int	npfk_load(npf_t *, const void *, npf_error_t *);
int	npfk_socket_load(npf_t *, int);
void	npfk_gc(npf_t *);
void	npfk_destroy(npf_t *);
void *	npfk_getarg(npf_t *);

void	npfk_thread_register(npf_t *);
void	npfk_thread_unregister(npf_t *);

int	npfk_packet_handler(npf_t *, struct mbuf **, struct ifnet *, int);

void	npfk_ifmap_attach(npf_t *, struct ifnet *);
void	npfk_ifmap_detach(npf_t *, struct ifnet *);

int	npfk_param_get(npf_t *, const char *, int *);
int	npfk_param_set(npf_t *, const char *, int);

void	npfk_stats(npf_t *, uint64_t *);
void	npfk_stats_clear(npf_t *);

/*
 * Extensions.
 */

int	npf_ext_log_init(npf_t *);
int	npf_ext_log_fini(npf_t *);

int	npf_ext_normalize_init(npf_t *);
int	npf_ext_normalize_fini(npf_t *);

int	npf_ext_rndblock_init(npf_t *);
int	npf_ext_rndblock_fini(npf_t *);

/*
 * ALGs.
 */

int	npf_alg_icmp_init(npf_t *);
int	npf_alg_icmp_fini(npf_t *);

int	npf_alg_pptp_init(npf_t *);
int	npf_alg_pptp_fini(npf_t *);

#endif
