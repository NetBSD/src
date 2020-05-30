/*-
 * Copyright (c) 2009-2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This material is based upon work partially supported by The
 * NetBSD Foundation under a contract with Mindaugas Rasiukevicius.
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

#ifndef _NPF_CONN_H_
#define _NPF_CONN_H_

#if !defined(_KERNEL) && !defined(_NPF_STANDALONE)
#error "kernel-level header only"
#endif

#include <sys/types.h>

#include "npf_impl.h"

#if defined(__NPF_CONN_PRIVATE)

/*
 * The main connection tracking structure.
 */
struct npf_conn {
	/*
	 * Protocol, address length, the interface ID (if zero,
	 * then the state is global) and connection flags.
	 */
	uint16_t		c_proto;
	uint16_t		c_alen;
	unsigned		c_ifid;
	unsigned		c_flags;

	/* Matching rule flags and ID. */
	unsigned		c_retfl;
	uint64_t		c_rid;

	/*
	 * Entry in the connection database/list.  The entry is
	 * protected by npf_t::conn_lock.
	 */
	union {
		npf_conn_t *		c_next;
		LIST_ENTRY(npf_conn)	c_entry;
	};

	/* Associated rule procedure or NAT (if any). */
	npf_rproc_t *		c_rproc;
	npf_nat_t *		c_nat;

	/*
	 * The Reference count and the last activity time (used to
	 * calculate expiration time).  Note: *unsigned* 32-bit integer
	 * as a timestamp is sufficient for us.
	 */
	unsigned		c_refcnt;
	uint32_t		c_atime;

	/* The protocol state and lock. */
	kmutex_t		c_lock;
	npf_state_t		c_state;

	/*
	 * Connection "forwards" and "backwards" keys.  They are accessed
	 * as npf_connkey_t, see below and npf_conn_getkey().
	 */
	uint32_t		c_keys[];
};

typedef struct {
	int	connkey_interface;
	int	connkey_direction;
} npf_conn_params_t;

#endif

/*
 * Connection key interface.
 *
 * See the key layout description in the npf_connkey.c source file.
 */

#define	NPF_CONNKEY_V4WORDS	(2 + ((sizeof(struct in_addr) * 2) >> 2))
#define	NPF_CONNKEY_V6WORDS	(2 + ((sizeof(struct in6_addr) * 2) >> 2))
#define	NPF_CONNKEY_MAXWORDS	(NPF_CONNKEY_V6WORDS)

#define	NPF_CONNKEY_ALEN(key)	(((key)->ck_key[0] >> 28) << 2)
#define	NPF_CONNKEY_LEN(key)	(8 + (NPF_CONNKEY_ALEN(key) * 2))

typedef struct npf_connkey {
	/* Warning: ck_key has a variable length -- see above. */
	uint32_t		ck_key[NPF_CONNKEY_MAXWORDS];
} npf_connkey_t;

unsigned	npf_conn_conkey(const npf_cache_t *, npf_connkey_t *,
		    const unsigned, const npf_flow_t);
npf_connkey_t *	npf_conn_getforwkey(npf_conn_t *);
npf_connkey_t *	npf_conn_getbackkey(npf_conn_t *, unsigned);
void		npf_conn_adjkey(npf_connkey_t *, const npf_addr_t *,
		    const uint16_t, const unsigned);
unsigned	npf_connkey_setkey(npf_connkey_t *, unsigned, unsigned,
		    const void *, const uint16_t *, const npf_flow_t);
void		npf_connkey_getkey(const npf_connkey_t *, unsigned *,
		    unsigned *, npf_addr_t *, uint16_t *);
unsigned	npf_connkey_import(npf_t *, const nvlist_t *, npf_connkey_t *);
nvlist_t *	npf_connkey_export(npf_t *, const npf_connkey_t *);
void		npf_connkey_print(const npf_connkey_t *);

/*
 * Connection tracking interface.
 */
void		npf_conn_init(npf_t *);
void		npf_conn_fini(npf_t *);
void		npf_conn_tracking(npf_t *, bool);
void		npf_conn_load(npf_t *, npf_conndb_t *, bool);

npf_conn_t *	npf_conn_lookup(const npf_cache_t *, const unsigned, npf_flow_t *);
npf_conn_t *	npf_conn_inspect(npf_cache_t *, const unsigned, int *);
npf_conn_t *	npf_conn_establish(npf_cache_t *, const unsigned, bool);
void		npf_conn_release(npf_conn_t *);
void		npf_conn_destroy(npf_t *, npf_conn_t *);
void		npf_conn_expire(npf_conn_t *);
bool		npf_conn_pass(const npf_conn_t *, npf_match_info_t *,
		    npf_rproc_t **);
void		npf_conn_setpass(npf_conn_t *, const npf_match_info_t *,
		    npf_rproc_t *);
int		npf_conn_setnat(const npf_cache_t *, npf_conn_t *,
		    npf_nat_t *, unsigned);
npf_nat_t *	npf_conn_getnat(const npf_conn_t *);
bool		npf_conn_expired(npf_t *, const npf_conn_t *, uint64_t);
void		npf_conn_remove(npf_conndb_t *, npf_conn_t *);
void		npf_conn_worker(npf_t *);
int		npf_conn_import(npf_t *, npf_conndb_t *, const nvlist_t *,
		    npf_ruleset_t *);
int		npf_conn_find(npf_t *, const nvlist_t *, nvlist_t *);
void		npf_conn_print(npf_conn_t *);

/*
 * Connection database (aka state table) interface.
 */
void		npf_conndb_sysinit(npf_t *);
void		npf_conndb_sysfini(npf_t *);

npf_conndb_t *	npf_conndb_create(void);
void		npf_conndb_destroy(npf_conndb_t *);

npf_conn_t *	npf_conndb_lookup(npf_t *, const npf_connkey_t *, npf_flow_t *);
bool		npf_conndb_insert(npf_conndb_t *, const npf_connkey_t *,
		    npf_conn_t *, npf_flow_t);
npf_conn_t *	npf_conndb_remove(npf_conndb_t *, npf_connkey_t *);

void		npf_conndb_enqueue(npf_conndb_t *, npf_conn_t *);
npf_conn_t *	npf_conndb_getlist(npf_conndb_t *);
npf_conn_t *	npf_conndb_getnext(npf_conndb_t *, npf_conn_t *);
int		npf_conndb_export(npf_t *, nvlist_t *);
void		npf_conndb_gc(npf_t *, npf_conndb_t *, bool, bool);

#endif	/* _NPF_CONN_H_ */
