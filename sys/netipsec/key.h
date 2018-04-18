/*	$NetBSD: key.h,v 1.35 2018/04/18 07:32:44 maxv Exp $	*/
/*	$FreeBSD: src/sys/netipsec/key.h,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$KAME: key.h,v 1.21 2001/07/27 03:51:30 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#ifndef _NETIPSEC_KEY_H_
#define _NETIPSEC_KEY_H_

#ifdef _KERNEL

struct secpolicy;
struct secpolicyindex;
struct ipsecrequest;
struct secasvar;
struct sockaddr;
struct socket;
struct secasindex;
union sockaddr_union;

#include <net/pfkeyv2.h>

struct sadb_msghdr {
	struct sadb_msg *msg;
	void *ext[SADB_EXT_MAX + 1];
	int extoff[SADB_EXT_MAX + 1];
	int extlen[SADB_EXT_MAX + 1];
};

int key_havesp(u_int dir);
struct secpolicy *key_lookup_sp_byspidx(const struct secpolicyindex *, u_int,
    const char *, int);
struct secpolicy *key_newsp(const char*, int);
struct secpolicy *key_gettunnel(const struct sockaddr *,
    const struct sockaddr *, const struct sockaddr *,
    const struct sockaddr *, const char *, int);
/* NB: prepend with _ for KAME IPv6 compatbility */
void key_init_sp(struct secpolicy *);
void key_free_sp(struct secpolicy *);
u_int key_sp_refcnt(const struct secpolicy *);
void key_sp_ref(struct secpolicy *, const char *, int);
void key_sp_unref(struct secpolicy *, const char *, int);
void key_sa_ref(struct secasvar *, const char *, int);
void key_sa_unref(struct secasvar *, const char *, int);
u_int key_sa_refcnt(const struct secasvar *);

void key_socksplist_add(struct secpolicy *);

/*
 * Access to the SADB are interlocked with splsoftnet.  In particular,
 * holders of SA's use this to block accesses by protocol processing
 * that can happen either by network swi's or by continuations that
 * occur on crypto callbacks.  Much of this could go away if
 * key_checkrequest were redone.
 */
#define	KEY_LOOKUP_SP_BYSPIDX(spidx, dir)			\
	key_lookup_sp_byspidx(spidx, dir, __func__, __LINE__)
#define	KEY_NEWSP()						\
	key_newsp(__func__, __LINE__)
#define	KEY_GETTUNNEL(osrc, odst, isrc, idst)			\
	key_gettunnel(osrc, odst, isrc, idst, __func__, __LINE__)
#define	KEY_SP_UNREF(spp)					\
	key_sp_unref(*(spp), __func__, __LINE__)
#define	KEY_SP_REF(sp)						\
	key_sp_ref(sp, __func__, __LINE__)
#define KEY_SA_REF(sav)						\
	key_sa_ref(sav, __func__, __LINE__)
#define	KEY_SA_UNREF(psav)					\
	key_sa_unref(*(psav), __func__, __LINE__)

struct secasvar *key_lookup_sa(const union sockaddr_union *,
    u_int, u_int32_t, u_int16_t, u_int16_t, const char *, int);
void key_freesav(struct secasvar **, const char *, int);
struct secasvar *key_lookup_sa_bysaidx(const struct secasindex *);

#define	KEY_LOOKUP_SA(dst, proto, spi, sport, dport)		\
	key_lookup_sa(dst, proto, spi, sport, dport,  __func__, __LINE__)

int key_checktunnelsanity(struct secasvar *, u_int, void *, void *);
int key_checkrequest(const struct ipsecrequest *, const struct secasindex *,
    struct secasvar **);

struct secpolicy *key_msg2sp(const struct sadb_x_policy *, size_t, int *);
struct mbuf *key_sp2msg(const struct secpolicy *, int);
int key_ismyaddr(const struct sockaddr *);
int key_spdacquire(const struct secpolicy *);
u_long key_random(void);
void key_randomfill(void *, size_t);
void key_freereg(struct socket *);
int key_parse(struct mbuf *, struct socket *);
void key_init(void);
void key_sa_recordxfer(struct secasvar *, struct mbuf *);
void key_sa_routechange(struct sockaddr *);
void key_update_used(void);
int key_get_used(void);

u_int16_t key_portfromsaddr(const union sockaddr_union *);

/* for ipsec(4) */
struct secpolicy *key_kpi_spdadd(struct mbuf *);
int key_kpi_spddelete2(struct mbuf *);
u_int16_t key_newreqid(void);

#endif /* defined(_KERNEL) */
#endif /* !_NETIPSEC_KEY_H_ */
