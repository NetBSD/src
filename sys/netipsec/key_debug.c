/*	$NetBSD: key_debug.c,v 1.22 2018/04/19 08:27:38 maxv Exp $	*/
/*	$FreeBSD: key_debug.c,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$KAME: key_debug.c,v 1.26 2001/06/27 10:46:50 sakane Exp $	*/

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

#ifdef _KERNEL
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: key_debug.c,v 1.22 2018/04/19 08:27:38 maxv Exp $");
#endif

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif

#include <sys/types.h>
#include <sys/param.h>
#ifdef _KERNEL
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/queue.h>
#endif
#include <sys/socket.h>

#include <net/route.h>

#include <netipsec/key.h>
#include <netipsec/key_var.h>
#include <netipsec/key_debug.h>

#include <netinet/in.h>
#include <netipsec/ipsec.h>

#ifndef _KERNEL
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#endif /* !_KERNEL */

static void kdebug_sadb_prop(const struct sadb_ext *);
static void kdebug_sadb_identity(const struct sadb_ext *);
static void kdebug_sadb_supported(const struct sadb_ext *);
static void kdebug_sadb_lifetime(const struct sadb_ext *);
static void kdebug_sadb_sa(const struct sadb_ext *);
static void kdebug_sadb_address(const struct sadb_ext *);
static void kdebug_sadb_key(const struct sadb_ext *);
static void kdebug_sadb_x_sa2(const struct sadb_ext *);
static void kdebug_sadb_x_policy(const struct sadb_ext *);

static void kdebug__secpolicyindex(const struct secpolicyindex *);

static void kdebug_hexdump(const char *, const void *, size_t);
static void kdebug_sockaddr(const struct sockaddr *);
static void kdebug_secasindex(const struct secasindex *);
static void kdebug_mbufhdr(const struct mbuf *);

#ifdef _KERNEL
#if 0
static void kdebug_secasv(const struct secasvar *);
static void kdebug_secreplay(const struct secreplay *);
#endif
#endif

#ifndef _KERNEL
#define panic(...)	err(EXIT_FAILURE, __VA_ARGS__)
#endif

/* NOTE: host byte order */
/* %%%: about struct sadb_msg */
void
kdebug_sadb(const struct sadb_msg *base)
{
	const struct sadb_ext *ext;
	int tlen, extlen;

	/* sanity check */
	if (base == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf("sadb { version=%u type=%u errno=%u satype=%u",
	    base->sadb_msg_version, base->sadb_msg_type,
	    base->sadb_msg_errno, base->sadb_msg_satype);
	printf(" len=%u reserved=%u seq=%u pid=%u",
	    base->sadb_msg_len, base->sadb_msg_reserved,
	    base->sadb_msg_seq, base->sadb_msg_pid);

	tlen = PFKEY_UNUNIT64(base->sadb_msg_len) - sizeof(struct sadb_msg);
	ext = (const void *)(base + 1);

	while (tlen > 0) {
		if (ext->sadb_ext_len == 0 || ext->sadb_ext_len > tlen) {
			panic("%s: invalid ext_len=%d tlen=%d was passed",
			    __func__, ext->sadb_ext_len, tlen);
		}

		printf(" sadb_ext { len=%u type=%u }",
		    PFKEY_UNUNIT64(ext->sadb_ext_len), ext->sadb_ext_type);


		switch (ext->sadb_ext_type) {
		case SADB_EXT_SA:
			kdebug_sadb_sa(ext);
			break;
		case SADB_EXT_LIFETIME_CURRENT:
		case SADB_EXT_LIFETIME_HARD:
		case SADB_EXT_LIFETIME_SOFT:
			kdebug_sadb_lifetime(ext);
			break;
		case SADB_EXT_ADDRESS_SRC:
		case SADB_EXT_ADDRESS_DST:
		case SADB_EXT_ADDRESS_PROXY:
			kdebug_sadb_address(ext);
			break;
		case SADB_EXT_KEY_AUTH:
		case SADB_EXT_KEY_ENCRYPT:
			kdebug_sadb_key(ext);
			break;
		case SADB_EXT_IDENTITY_SRC:
		case SADB_EXT_IDENTITY_DST:
			kdebug_sadb_identity(ext);
			break;
		case SADB_EXT_SENSITIVITY:
			break;
		case SADB_EXT_PROPOSAL:
			kdebug_sadb_prop(ext);
			break;
		case SADB_EXT_SUPPORTED_AUTH:
		case SADB_EXT_SUPPORTED_ENCRYPT:
			kdebug_sadb_supported(ext);
			break;
		case SADB_EXT_SPIRANGE:
		case SADB_X_EXT_KMPRIVATE:
			break;
		case SADB_X_EXT_POLICY:
			kdebug_sadb_x_policy(ext);
			break;
		case SADB_X_EXT_SA2:
			kdebug_sadb_x_sa2(ext);
			break;
		default:
			panic("%s: invalid ext_type %u was passed",
			    __func__, ext->sadb_ext_type);
		}

		extlen = PFKEY_UNUNIT64(ext->sadb_ext_len);
		tlen -= extlen;
		ext = (const void *)((const char *)ext + extlen);
	}
	printf("\n");
}

static void
kdebug_sadb_prop(const struct sadb_ext *ext)
{
	const struct sadb_prop *prop = (const struct sadb_prop *)ext;
	const struct sadb_comb *comb;
	int len;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	len = (PFKEY_UNUNIT64(prop->sadb_prop_len) - sizeof(*prop))
		/ sizeof(*comb);
	comb = (const void *)(prop + 1);
	printf(" sadb_prop { replay=%u", prop->sadb_prop_replay);

	while (len--) {
		printf(" sadb_comb { auth=%u encrypt=%u"
		    "flags=%#04x reserved=%#08x ",
		    comb->sadb_comb_auth, comb->sadb_comb_encrypt,
		    comb->sadb_comb_flags, comb->sadb_comb_reserved);

		printf(" auth_minbits=%u auth_maxbits=%u"
		    "encrypt_minbits=%u encrypt_maxbits=%u",
		    comb->sadb_comb_auth_minbits,
		    comb->sadb_comb_auth_maxbits,
		    comb->sadb_comb_encrypt_minbits,
		    comb->sadb_comb_encrypt_maxbits);

		printf(" soft_alloc=%u hard_alloc=%u"
		    "soft_bytes=%lu hard_bytes=%lu",
		    comb->sadb_comb_soft_allocations,
		    comb->sadb_comb_hard_allocations,
		    (unsigned long)comb->sadb_comb_soft_bytes,
		    (unsigned long)comb->sadb_comb_hard_bytes);

		printf(" soft_alloc=%lu hard_alloc=%lu"
		    "soft_bytes=%lu hard_bytes=%lu }",
		    (unsigned long)comb->sadb_comb_soft_addtime,
		    (unsigned long)comb->sadb_comb_hard_addtime,
		    (unsigned long)comb->sadb_comb_soft_usetime,
		    (unsigned long)comb->sadb_comb_hard_usetime);
		comb++;
	}
	printf(" }");

	return;
}

static void
kdebug_sadb_identity(const struct sadb_ext *ext)
{
	const struct sadb_ident *id = (const struct sadb_ident *)ext;
	int len;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	len = PFKEY_UNUNIT64(id->sadb_ident_len) - sizeof(*id);
	printf(" sadb_ident_%s {",
	    id->sadb_ident_exttype == SADB_EXT_IDENTITY_SRC ? "src" : "dst");
	switch (id->sadb_ident_type) {
	default:
		printf(" type=%d id=%lu",
		    id->sadb_ident_type, (u_long)id->sadb_ident_id);
		if (len) {
			kdebug_hexdump("data", id + 1, len);
		}
		break;
	}

	printf(" }");
}

static void
kdebug_sadb_supported(const struct sadb_ext *ext)
{
	const struct sadb_supported *sup = (const struct sadb_supported *)ext;
	const struct sadb_alg *alg;
	int len;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	len = (PFKEY_UNUNIT64(sup->sadb_supported_len) - sizeof(*sup))
		/ sizeof(*alg);
	alg = (const void *)(sup + 1);
	printf(" sadb_sup {");
	while (len--) {
		printf(" { id=%d ivlen=%d min=%d max=%d }",
		    alg->sadb_alg_id, alg->sadb_alg_ivlen,
		    alg->sadb_alg_minbits, alg->sadb_alg_maxbits);
		alg++;
	}
	printf(" }");
}

static void
kdebug_sadb_lifetime(const struct sadb_ext *ext)
{
	const struct sadb_lifetime *lft = (const struct sadb_lifetime *)ext;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" sadb_lifetime { alloc=%u, bytes=%u",
	    lft->sadb_lifetime_allocations,
	    (u_int32_t)lft->sadb_lifetime_bytes);
	printf(" addtime=%u, usetime=%u }",
	    (u_int32_t)lft->sadb_lifetime_addtime,
	    (u_int32_t)lft->sadb_lifetime_usetime);
}

static void
kdebug_sadb_sa(const struct sadb_ext *ext)
{
	const struct sadb_sa *sa = (const struct sadb_sa *)ext;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" sadb_sa { spi=%u replay=%u state=%u",
	    (u_int32_t)ntohl(sa->sadb_sa_spi), sa->sadb_sa_replay,
	    sa->sadb_sa_state);
	printf(" auth=%u encrypt=%u flags=%#08x }",
	    sa->sadb_sa_auth, sa->sadb_sa_encrypt, sa->sadb_sa_flags);
}

static void
kdebug_sadb_address(const struct sadb_ext *ext)
{
	const struct sadb_address *addr = (const struct sadb_address *)ext;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" sadb_address { proto=%u prefixlen=%u reserved=%#02x%02x }",
	    addr->sadb_address_proto, addr->sadb_address_prefixlen,
	    ((const u_char *)&addr->sadb_address_reserved)[0],
	    ((const u_char *)&addr->sadb_address_reserved)[1]);

	kdebug_sockaddr((const struct sockaddr *)
	    ((const char *)ext + sizeof(*addr)));
}

static void
kdebug_sadb_key(const struct sadb_ext *ext)
{
	const struct sadb_key *key = (const struct sadb_key *)ext;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	/* sanity check 2 */
	if ((key->sadb_key_bits >> 3) >
	    (PFKEY_UNUNIT64(key->sadb_key_len) - sizeof(struct sadb_key))) {
		panic("%s: key length mismatch, bit:%d len:%ld ", __func__,
		    key->sadb_key_bits >> 3,
		    (long)PFKEY_UNUNIT64(key->sadb_key_len)
		    - sizeof(struct sadb_key));
	}

	printf(" sadb_key { bits=%u reserved=%u",
	    key->sadb_key_bits, key->sadb_key_reserved);
	kdebug_hexdump("key", key + 1, key->sadb_key_bits >> 3);
	printf(" }");
}

static void
kdebug_sadb_x_sa2(const struct sadb_ext *ext)
{
	const struct sadb_x_sa2 *sa2 = (const struct sadb_x_sa2 *)ext;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" sadb_x_sa2 { mode=%u reqid=%u",
	    sa2->sadb_x_sa2_mode, sa2->sadb_x_sa2_reqid);
	printf(" reserved1=%u reserved2=%u sequence=%u }",
	    sa2->sadb_x_sa2_reserved1, sa2->sadb_x_sa2_reserved2,
	    sa2->sadb_x_sa2_sequence);
}

static void
kdebug_sadb_x_policy(const struct sadb_ext *ext)
{
	const struct sadb_x_policy *xpl = (const struct sadb_x_policy *)ext;
	const struct sockaddr *addr;

	/* sanity check */
	if (ext == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" sadb_x_policy { type=%u dir=%u id=%x }",
		xpl->sadb_x_policy_type, xpl->sadb_x_policy_dir,
		xpl->sadb_x_policy_id);

	if (xpl->sadb_x_policy_type == IPSEC_POLICY_IPSEC) {
		int tlen;
		const struct sadb_x_ipsecrequest *xisr;

		tlen = PFKEY_UNUNIT64(xpl->sadb_x_policy_len) - sizeof(*xpl);
		xisr = (const struct sadb_x_ipsecrequest *)(xpl + 1);

		while (tlen > 0) {
			printf(" { len=%u proto=%u mode=%u level=%u reqid=%u",
			    xisr->sadb_x_ipsecrequest_len,
			    xisr->sadb_x_ipsecrequest_proto,
			    xisr->sadb_x_ipsecrequest_mode,
			    xisr->sadb_x_ipsecrequest_level,
			    xisr->sadb_x_ipsecrequest_reqid);

			if (xisr->sadb_x_ipsecrequest_len > sizeof(*xisr)) {
				addr = (const void *)(xisr + 1);
				kdebug_sockaddr(addr);
				addr = (const void *)((const char *)addr
				    + addr->sa_len);
				kdebug_sockaddr(addr);
			}

			printf(" }");

			/* prevent infinite loop */
			if (xisr->sadb_x_ipsecrequest_len <= 0) {
				panic("%s: wrong policy struct", __func__);
			}
			/* prevent overflow */
			if (xisr->sadb_x_ipsecrequest_len > tlen) {
				panic("%s: invalid ipsec policy length",
				    __func__);
			}

			tlen -= xisr->sadb_x_ipsecrequest_len;

			xisr = (const struct sadb_x_ipsecrequest *)
			    ((const char *)xisr
			    + xisr->sadb_x_ipsecrequest_len);
		}

		if (tlen != 0)
			panic("%s: wrong policy struct", __func__);
	}
}

#ifdef _KERNEL

void
kdebug_sadb_xpolicy(const char *msg, const struct sadb_ext *ext)
{
	printf("%s:", msg);
	kdebug_sadb_x_policy(ext);
	printf("\n");
}

/* %%%: about SPD and SAD */
void
kdebug_secpolicy(const struct secpolicy *sp)
{
	/* sanity check */
	if (sp == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" secpolicy { refcnt=%u state=%u policy=%u",
	    key_sp_refcnt(sp), sp->state, sp->policy);

	kdebug__secpolicyindex(&sp->spidx);

	printf(" type=");
	switch (sp->policy) {
	case IPSEC_POLICY_DISCARD:
		printf("discard");
		break;
	case IPSEC_POLICY_NONE:
		printf("none");
		break;
	case IPSEC_POLICY_IPSEC:
	    {
		printf("ipsec {");
		struct ipsecrequest *isr;
		for (isr = sp->req; isr != NULL; isr = isr->next) {
			printf(" level=%u", isr->level);
			kdebug_secasindex(&isr->saidx);
		}
		printf(" }");
	    }
		break;
	case IPSEC_POLICY_BYPASS:
		printf("bypass");
		break;
	case IPSEC_POLICY_ENTRUST:
		printf("entrust");
		break;
	default:
		panic("%s: Invalid policy found. %d", __func__, sp->policy);
	}
	printf(" }\n");
}

void
kdebug_secpolicyindex(const char *msg, const struct secpolicyindex *spidx)
{
	printf("%s:", msg);
	kdebug__secpolicyindex(spidx);
	printf("\n");
}


static void
kdebug__secpolicyindex(const struct secpolicyindex *spidx)
{
	/* sanity check */
	if (spidx == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" secpolicy { dir=%u prefs=%u prefd=%u ul_proto=%u",
		spidx->dir, spidx->prefs, spidx->prefd, spidx->ul_proto);

	kdebug_hexdump("src", &spidx->src, spidx->src.sa.sa_len);
	kdebug_hexdump("dst", &spidx->dst, spidx->dst.sa.sa_len);
	printf(" }");
}

static void
kdebug_secasindex(const struct secasindex *saidx)
{
	/* sanity check */
	if (saidx == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" secasindex { mode=%u proto=%u",
	    saidx->mode, saidx->proto);
	kdebug_hexdump("src", &saidx->src, saidx->src.sa.sa_len);
	kdebug_hexdump("dst", &saidx->dst, saidx->dst.sa.sa_len);
	printf(" }");
}

#if 0
static void
kdebug_secasv(const struct secasvar *sav)
{
	/* sanity check */
	if (sav == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" secasv {", );
	kdebug_secasindex(&sav->sah->saidx);

	printf(" refcnt=%u state=%u auth=%u enc=%u",
	    key_sa_refcnt(sav), sav->state, sav->alg_auth, sav->alg_enc);
	printf(" spi=%u flags=%u",
	    (u_int32_t)ntohl(sav->spi), sav->flags);

	if (sav->key_auth != NULL)
		kdebug_sadb_key((struct sadb_ext *)sav->key_auth);
	if (sav->key_enc != NULL)
		kdebug_sadb_key((struct sadb_ext *)sav->key_enc);

	if (sav->replay != NULL)
		kdebug_secreplay(sav->replay);
	if (sav->lft_c != NULL)
		kdebug_sadb_lifetime((struct sadb_ext *)sav->lft_c);
	if (sav->lft_h != NULL)
		kdebug_sadb_lifetime((struct sadb_ext *)sav->lft_h);
	if (sav->lft_s != NULL)
		kdebug_sadb_lifetime((struct sadb_ext *)sav->lft_s);

	/* XXX: misc[123] ? */
}

static void
kdebug_secreplay(const struct secreplay *rpl)
{
	int len, l;

	/* sanity check */
	if (rpl == NULL)
		panic("%s: NULL pointer was passed", __func__);

	printf(" secreplay { count=%u wsize=%u seq=%u lastseq=%u",
	    rpl->count, rpl->wsize, rpl->seq, rpl->lastseq);

	if (rpl->bitmap == NULL) {
		printf(" }");
		return;
	}

	printf(" bitmap {");

	for (len = 0; len < rpl->wsize; len++) {
		for (l = 7; l >= 0; l--)
			printf(" %u", (((rpl->bitmap)[len] >> l) & 1) ? 1 : 0);
	}
	printf(" } }");
}
#endif

static void
kdebug_mbufhdr(const struct mbuf *m)
{
	/* sanity check */
	if (m == NULL)
		return;

	printf(" mbuf(%p) { m_next:%p m_nextpkt:%p m_data:%p "
	       "m_len:%d m_type:%#02x m_flags:%#02x }",
		m, m->m_next, m->m_nextpkt, m->m_data,
		m->m_len, m->m_type, m->m_flags);

	if (m->m_flags & M_PKTHDR) {
		printf(" m_pkthdr { len:%d rcvif:%p }",
		    m->m_pkthdr.len, m_get_rcvif_NOMPSAFE(m));
	}

	if (m->m_flags & M_EXT) {
		printf(" m_ext { ext_buf:%p ext_free:%p "
		   "ext_size:%zu ext_refcnt:%u }",
		    m->m_ext.ext_buf, m->m_ext.ext_free,
		    m->m_ext.ext_size, m->m_ext.ext_refcnt);
	}
}

void
kdebug_mbuf(const char *msg, const struct mbuf *m0)
{
	const struct mbuf *m = m0;
	int i, j;

	printf("%s:", msg);
	for (j = 0; m; m = m->m_next) {
		kdebug_mbufhdr(m);
		printf(" m_data:");
		for (i = 0; i < m->m_len; i++) {
			if (i % 4 == 0)
				printf(" ");
			printf("%02x", mtod(m, u_char *)[i]);
			j++;
		}
	}
	printf("\n");
}
#endif /* _KERNEL */

static void
kdebug_sockaddr(const struct sockaddr *addr)
{
	const struct sockaddr_in *sin4;
#ifdef INET6
	const struct sockaddr_in6 *sin6;
#endif

	/* sanity check */
	if (addr == NULL)
		panic("%s: NULL pointer was passed", __func__);

	/* NOTE: We deal with port number as host byte order. */
	printf(" sockaddr { len=%u family=%u",
	    addr->sa_len, addr->sa_family);

	switch (addr->sa_family) {
	case AF_INET:
		sin4 = (const struct sockaddr_in *)addr;
		printf(" port=%u", ntohs(sin4->sin_port));
		kdebug_hexdump("addr", &sin4->sin_addr, sizeof(sin4->sin_addr));
		break;
#ifdef INET6
	case AF_INET6:
		sin6 = (const struct sockaddr_in6 *)addr;
		printf(" port=%u", ntohs(sin6->sin6_port));
		printf(" flowinfo=%#08x, scope_id=%#08x",
		    sin6->sin6_flowinfo, sin6->sin6_scope_id);
		kdebug_hexdump("addr", &sin6->sin6_addr, sizeof(sin6->sin6_addr));
		break;
#endif
	}

	printf(" }");
}

static void
kdebug_hexdump(const char *tag, const void *v, size_t len)
{
	size_t i;
	const unsigned char *buf = v;

	if (len)
		printf(" %s=", tag);

	for (i = 0; i < len; i++) {
		if (i && i % 4 == 0) printf(" ");
		printf("%02x", buf[i]);
	}
}
