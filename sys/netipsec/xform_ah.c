/*	$NetBSD: xform_ah.c,v 1.111 2022/05/22 11:39:08 riastradh Exp $	*/
/*	$FreeBSD: xform_ah.c,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_ah.c,v 1.63 2001/06/26 06:18:58 angelos Exp $ */
/*
 * The authors of this code are John Ioannidis (ji@tla.org),
 * Angelos D. Keromytis (kermit@csd.uch.gr) and
 * Niels Provos (provos@physnet.uni-hamburg.de).
 *
 * The original version of this code was written by John Ioannidis
 * for BSD/OS in Athens, Greece, in November 1995.
 *
 * Ported to OpenBSD and NetBSD, with additional transforms, in December 1996,
 * by Angelos D. Keromytis.
 *
 * Additional transforms and features in 1997 and 1998 by Angelos D. Keromytis
 * and Niels Provos.
 *
 * Additional features in 1999 by Angelos D. Keromytis and Niklas Hallqvist.
 *
 * Copyright (c) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
 * Copyright (c) 1999 Niklas Hallqvist.
 * Copyright (c) 2001 Angelos D. Keromytis.
 *
 * Permission to use, copy, and modify this software with or without fee
 * is hereby granted, provided that this entire notice is included in
 * all copies of any software which is or includes a copy or
 * modification of this software.
 * You may use this code under the GNU public license if you so wish. Please
 * contribute changes back to the authors under this freer than GPL license
 * so that we may further the use of strong encryption without limitations to
 * all.
 *
 * THIS SOFTWARE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTY. IN PARTICULAR, NONE OF THE AUTHORS MAKES ANY
 * REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE
 * MERCHANTABILITY OF THIS SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR
 * PURPOSE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xform_ah.c,v 1.111 2022/05/22 11:39:08 riastradh Exp $");

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/pool.h>
#include <sys/pserialize.h>
#include <sys/kmem.h>

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_ecn.h>
#include <netinet/ip_var.h>
#include <netinet/ip6.h>

#include <net/route.h>
#include <netipsec/ipsec.h>
#include <netipsec/ipsec_private.h>
#include <netipsec/ah.h>
#include <netipsec/ah_var.h>
#include <netipsec/xform.h>

#ifdef INET6
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>
#include <netipsec/ipsec6.h>
#endif

#include <netipsec/key.h>
#include <netipsec/key_debug.h>

#include <opencrypto/cryptodev.h>

/*
 * Return header size in bytes.  The old protocol did not support
 * the replay counter; the new protocol always includes the counter.
 */
#define HDRSIZE(sav) \
	(((sav)->flags & SADB_X_EXT_OLD) ? \
		sizeof(struct ah) : sizeof(struct ah) + sizeof(uint32_t))
/*
 * Return authenticator size in bytes.  The old protocol is known
 * to use a fixed 16-byte authenticator.  The new algorithm gets
 * this size from the xform but is (currently) always 12.
 */
#define	AUTHSIZE(sav) \
	((sav->flags & SADB_X_EXT_OLD) ? 16 : (sav)->tdb_authalgxform->authsize)

percpu_t *ahstat_percpu;

int ah_enable = 1;			/* control flow of packets with AH */
int ip4_ah_cleartos = 1;		/* clear ip_tos when doing AH calc */

static const char ipseczeroes[256];

int ah_max_authsize;			/* max authsize over all algorithms */

static void ah_input_cb(struct cryptop *);
static void ah_output_cb(struct cryptop *);

const uint8_t ah_stats[256] = { SADB_AALG_STATS_INIT };

static pool_cache_t ah_tdb_crypto_pool_cache;
static size_t ah_pool_item_size;

/*
 * NB: this is public for use by the PF_KEY support.
 */
const struct auth_hash *
ah_algorithm_lookup(int alg)
{

	switch (alg) {
	case SADB_X_AALG_NULL:
		return &auth_hash_null;
	case SADB_AALG_MD5HMAC:
		return &auth_hash_hmac_md5_96;
	case SADB_AALG_SHA1HMAC:
		return &auth_hash_hmac_sha1_96;
	case SADB_X_AALG_RIPEMD160HMAC:
		return &auth_hash_hmac_ripemd_160_96;
	case SADB_X_AALG_MD5:
		return &auth_hash_key_md5;
	case SADB_X_AALG_SHA:
		return &auth_hash_key_sha1;
	case SADB_X_AALG_SHA2_256:
		return &auth_hash_hmac_sha2_256;
	case SADB_X_AALG_SHA2_384:
		return &auth_hash_hmac_sha2_384;
	case SADB_X_AALG_SHA2_512:
		return &auth_hash_hmac_sha2_512;
	case SADB_X_AALG_AES_XCBC_MAC:
		return &auth_hash_aes_xcbc_mac_96;
	}
	return NULL;
}

size_t
ah_authsiz(const struct secasvar *sav)
{
	size_t size;

	if (sav == NULL) {
		return ah_max_authsize;
	}

	size = AUTHSIZE(sav);
	return roundup(size, sizeof(uint32_t));
}

size_t
ah_hdrsiz(const struct secasvar *sav)
{
	size_t size;

	if (sav != NULL) {
		int authsize, rplen, align;

		KASSERT(sav->tdb_authalgxform != NULL);
		/*XXX not right for null algorithm--does it matter??*/

		/* RFC4302: use the correct alignment. */
		align = sizeof(uint32_t);
#ifdef INET6
		if (sav->sah->saidx.dst.sa.sa_family == AF_INET6) {
			align = sizeof(uint64_t);
		}
#endif
		rplen = HDRSIZE(sav);
		authsize = AUTHSIZE(sav);
		size = roundup(rplen + authsize, align);
	} else {
		/* default guess */
		size = sizeof(struct ah) + sizeof(uint32_t) + ah_max_authsize;
	}
	return size;
}

/*
 * NB: public for use by esp_init.
 */
int
ah_init0(struct secasvar *sav, const struct xformsw *xsp,
	 struct cryptoini *cria)
{
	const struct auth_hash *thash;
	int keylen;

	thash = ah_algorithm_lookup(sav->alg_auth);
	if (thash == NULL) {
		DPRINTF("unsupported authentication algorithm %u\n",
		    sav->alg_auth);
		return EINVAL;
	}
	/*
	 * Verify the replay state block allocation is consistent with
	 * the protocol type.  We check here so we can make assumptions
	 * later during protocol processing.
	 */
	/* NB: replay state is setup elsewhere (sigh) */
	if (((sav->flags&SADB_X_EXT_OLD) == 0) ^ (sav->replay != NULL)) {
		DPRINTF("replay state block inconsistency, "
		    "%s algorithm %s replay state\n",
		    (sav->flags & SADB_X_EXT_OLD) ? "old" : "new",
		    sav->replay == NULL ? "without" : "with");
		return EINVAL;
	}
	if (sav->key_auth == NULL) {
		DPRINTF("no authentication key for %s algorithm\n",
		    thash->name);
		return EINVAL;
	}
	keylen = _KEYLEN(sav->key_auth);
	if (keylen != thash->keysize && thash->keysize != 0) {
		DPRINTF("invalid keylength %d, algorithm %s requires "
		    "keysize %d\n",
		    keylen, thash->name, thash->keysize);
		return EINVAL;
	}

	sav->tdb_xform = xsp;
	sav->tdb_authalgxform = thash;

	/* Initialize crypto session. */
	memset(cria, 0, sizeof(*cria));
	cria->cri_alg = sav->tdb_authalgxform->type;
	cria->cri_klen = _KEYBITS(sav->key_auth);
	cria->cri_key = _KEYBUF(sav->key_auth);

	return 0;
}

/*
 * ah_init() is called when an SPI is being set up.
 */
static int
ah_init(struct secasvar *sav, const struct xformsw *xsp)
{
	struct cryptoini cria;
	int error;

	error = ah_init0(sav, xsp, &cria);
	if (!error)
		error = crypto_newsession(&sav->tdb_cryptoid,
					   &cria, crypto_support);
	return error;
}

/*
 * Paranoia.
 *
 * NB: public for use by esp_zeroize (XXX).
 */
void
ah_zeroize(struct secasvar *sav)
{

	if (sav->key_auth) {
		explicit_memset(_KEYBUF(sav->key_auth), 0,
		    _KEYLEN(sav->key_auth));
	}

	(void)crypto_freesession(sav->tdb_cryptoid);
	sav->tdb_cryptoid = 0;
	sav->tdb_authalgxform = NULL;
	sav->tdb_xform = NULL;
}

/*
 * Massage IPv4/IPv6 headers for AH processing.
 */
static int
ah_massage_headers(struct mbuf **m0, int proto, int skip, int alg, int out)
{
	struct mbuf *m = *m0;
	unsigned char *ptr;
	int off, optlen;
#ifdef INET
	struct ip *ip;
#endif
#ifdef INET6
	int count, ip6optlen;
	struct ip6_ext *ip6e;
	struct ip6_hdr ip6;
	int alloc, nxt;
#endif

	switch (proto) {
#ifdef INET
	case AF_INET:
		/*
		 * This is the least painful way of dealing with IPv4 header
		 * and option processing -- just make sure they're in
		 * contiguous memory.
		 */
		*m0 = m = m_pullup(m, skip);
		if (m == NULL) {
			DPRINTF("m_pullup failed\n");
			return ENOBUFS;
		}

		/* Fix the IP header */
		ip = mtod(m, struct ip *);
		if (ip4_ah_cleartos)
			ip->ip_tos = 0;
		ip->ip_ttl = 0;
		ip->ip_sum = 0;
		ip->ip_off = htons(ntohs(ip->ip_off) & ip4_ah_offsetmask);

		if (alg == CRYPTO_MD5_KPDK || alg == CRYPTO_SHA1_KPDK)
			ip->ip_off &= htons(IP_DF);
		else
			ip->ip_off = 0;

		ptr = mtod(m, unsigned char *);

		/* IPv4 option processing */
		for (off = sizeof(struct ip); off < skip;) {
			if (ptr[off] == IPOPT_EOL) {
				break;
			} else if (ptr[off] == IPOPT_NOP) {
				optlen = 1;
			} else if (off + 1 < skip) {
				optlen = ptr[off + 1];
				if (optlen < 2 || off + optlen > skip) {
					m_freem(m);
					return EINVAL;
				}
			} else {
				m_freem(m);
				return EINVAL;
			}

			switch (ptr[off]) {
			case IPOPT_NOP:
			case IPOPT_SECURITY:
			case 0x85:	/* Extended security. */
			case 0x86:	/* Commercial security. */
			case 0x94:	/* Router alert */
			case 0x95:	/* RFC1770 */
				break;

			case IPOPT_LSRR:
			case IPOPT_SSRR:
				/*
				 * On output, if we have either of the
				 * source routing options, we should
				 * swap the destination address of the
				 * IP header with the last address
				 * specified in the option, as that is
				 * what the destination's IP header
				 * will look like.
				 */
				if (out)
					memcpy(&ip->ip_dst,
					    ptr + off + optlen -
					    sizeof(struct in_addr),
					    sizeof(struct in_addr));
				/* FALLTHROUGH */

			default:
				/* Zeroize all other options. */
				memset(ptr + off, 0, optlen);
				break;
			}

			off += optlen;

			/* Sanity check. */
			if (off > skip)	{
				m_freem(m);
				return EINVAL;
			}
		}

		break;
#endif /* INET */

#ifdef INET6
	case AF_INET6:  /* Ugly... */
		/* Copy and "cook" the IPv6 header. */
		m_copydata(m, 0, sizeof(ip6), &ip6);

		/* We don't do IPv6 Jumbograms. */
		if (ip6.ip6_plen == 0) {
			DPRINTF("unsupported IPv6 jumbogram\n");
			m_freem(m);
			return EMSGSIZE;
		}

		ip6.ip6_flow = 0;
		ip6.ip6_hlim = 0;
		ip6.ip6_vfc &= ~IPV6_VERSION_MASK;
		ip6.ip6_vfc |= IPV6_VERSION;

		/* Scoped address handling. */
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6.ip6_src))
			ip6.ip6_src.s6_addr16[1] = 0;
		if (IN6_IS_SCOPE_LINKLOCAL(&ip6.ip6_dst))
			ip6.ip6_dst.s6_addr16[1] = 0;

		/* Done with IPv6 header. */
		m_copyback(m, 0, sizeof(struct ip6_hdr), &ip6);

		ip6optlen = skip - sizeof(struct ip6_hdr);

		/* Let's deal with the remaining headers (if any). */
		if (ip6optlen > 0) {
			if (m->m_len <= skip) {
				ptr = malloc(ip6optlen, M_XDATA, M_NOWAIT);
				if (ptr == NULL) {
					DPRINTF("failed to allocate "
					    "memory for IPv6 headers\n");
					m_freem(m);
					return ENOBUFS;
				}

				/*
				 * Copy all the protocol headers after
				 * the IPv6 header.
				 */
				m_copydata(m, sizeof(struct ip6_hdr),
				    ip6optlen, ptr);
				alloc = 1;
			} else {
				/* No need to allocate memory. */
				ptr = mtod(m, unsigned char *) +
				    sizeof(struct ip6_hdr);
				alloc = 0;
			}
		} else
			break;

		nxt = ip6.ip6_nxt & 0xff; /* Next header type. */

		for (off = 0; off < ip6optlen;) {
			int noff;

			if (off + sizeof(*ip6e) > ip6optlen) {
				goto error6;
			}
			ip6e = (struct ip6_ext *)(ptr + off);
			noff = off + ((ip6e->ip6e_len + 1) << 3);
			if (noff > ip6optlen) {
				goto error6;
			}

			switch (nxt) {
			case IPPROTO_HOPOPTS:
			case IPPROTO_DSTOPTS:
				/* Zero out mutable options. */
				for (count = off + sizeof(struct ip6_ext);
				     count < noff;) {
					if (ptr[count] == IP6OPT_PAD1) {
						count++;
						continue;
					}

					if (count + 1 >= noff) {
						goto error6;
					}
					optlen = ptr[count + 1] + 2;

					if (count + optlen > noff) {
						goto error6;
					}

					if (ptr[count] & IP6OPT_MUTABLE) {
						memset(ptr + count, 0, optlen);
					}

					count += optlen;
				}
				if (count != noff) {
					goto error6;
				}
				/* FALLTHROUGH */

			case IPPROTO_ROUTING:
				/* Advance. */
				off = noff;
				nxt = ip6e->ip6e_nxt;
				break;

			default:
error6:
				if (alloc)
					free(ptr, M_XDATA);
				m_freem(m);
				return EINVAL;
			}
		}

		/* Copyback and free, if we allocated. */
		if (alloc) {
			m_copyback(m, sizeof(struct ip6_hdr), ip6optlen, ptr);
			free(ptr, M_XDATA);
		}

		break;
#endif /* INET6 */
	}

	return 0;
}

/*
 * ah_input() gets called to verify that an input packet
 * passes authentication.
 */
static int
ah_input(struct mbuf *m, struct secasvar *sav, int skip, int protoff)
{
	const struct auth_hash *ahx;
	struct tdb_crypto *tc = NULL;
	struct newah *ah;
	int hl, rplen, authsize, ahsize, error, stat = AH_STAT_HDROPS;
	struct cryptodesc *crda;
	struct cryptop *crp = NULL;
	bool pool_used;
	uint8_t nxt;

	KASSERT(sav != NULL);
	KASSERT(sav->key_auth != NULL);
	KASSERT(sav->tdb_authalgxform != NULL);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);

	/* XXX don't pullup, just copy header */
	M_REGION_GET(ah, struct newah *, m, skip, rplen);
	if (ah == NULL) {
		/* m already freed */
		return ENOBUFS;
	}

	nxt = ah->ah_nxt;

	/* Check replay window, if applicable. */
	if (sav->replay && !ipsec_chkreplay(ntohl(ah->ah_seq), sav)) {
		char buf[IPSEC_LOGSASTRLEN];
		DPRINTF("packet replay failure: %s\n",
		    ipsec_logsastr(sav, buf, sizeof(buf)));
		stat = AH_STAT_REPLAY;
		error = EACCES;
		goto bad;
	}

	/* Verify AH header length. */
	hl = sizeof(struct ah) + (ah->ah_len * sizeof(uint32_t));
	ahx = sav->tdb_authalgxform;
	authsize = AUTHSIZE(sav);
	ahsize = ah_hdrsiz(sav);
	if (hl != ahsize) {
		char buf[IPSEC_ADDRSTRLEN];
		DPRINTF("bad authenticator length %u (expecting %lu)"
		    " for packet in SA %s/%08lx\n",
		    hl, (u_long)ahsize,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi));
		stat = AH_STAT_BADAUTHL;
		error = EACCES;
		goto bad;
	}
	if (skip + ahsize > m->m_pkthdr.len) {
		char buf[IPSEC_ADDRSTRLEN];
		DPRINTF("bad mbuf length %u (expecting >= %lu)"
		    " for packet in SA %s/%08lx\n",
		    m->m_pkthdr.len, (u_long)(skip + ahsize),
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi));
		stat = AH_STAT_BADAUTHL;
		error = EACCES;
		goto bad;
	}

	AH_STATADD(AH_STAT_IBYTES, m->m_pkthdr.len - skip - hl);

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		DPRINTF("failed to acquire crypto descriptor\n");
		stat = AH_STAT_CRYPTO;
		error = ENOBUFS;
		goto bad;
	}

	crda = crp->crp_desc;
	KASSERT(crda != NULL);

	crda->crd_skip = 0;
	crda->crd_len = m->m_pkthdr.len;
	crda->crd_inject = skip + rplen;

	/* Authentication operation. */
	crda->crd_alg = ahx->type;
	crda->crd_key = _KEYBUF(sav->key_auth);
	crda->crd_klen = _KEYBITS(sav->key_auth);

	/* Allocate IPsec-specific opaque crypto info. */
	size_t size = sizeof(*tc);
	size_t extra = skip + rplen + authsize;
	size += extra;

	if (__predict_true(size <= ah_pool_item_size)) {
		tc = pool_cache_get(ah_tdb_crypto_pool_cache, PR_NOWAIT);
		pool_used = true;
	} else {
		/* size can exceed on IPv6 packets with large options.  */
		tc = kmem_intr_zalloc(size, KM_NOSLEEP);
		pool_used = false;
	}
	if (tc == NULL) {
		DPRINTF("failed to allocate tdb_crypto\n");
		stat = AH_STAT_CRYPTO;
		error = ENOBUFS;
		goto bad;
	}

	error = m_makewritable(&m, 0, extra, M_NOWAIT);
	if (error) {
		DPRINTF("failed to m_makewritable\n");
		goto bad;
	}

	/*
	 * Save the authenticator, the skipped portion of the packet,
	 * and the AH header.
	 */
	m_copydata(m, 0, extra, (tc + 1));
	/* Zeroize the authenticator on the packet. */
	m_copyback(m, skip + rplen, authsize, ipseczeroes);

	/* "Massage" the packet headers for crypto processing. */
	error = ah_massage_headers(&m, sav->sah->saidx.dst.sa.sa_family,
	    skip, ahx->type, 0);
	if (error != 0) {
		/* NB: mbuf is free'd by ah_massage_headers */
		m = NULL;
		goto bad;
	}

    {
	int s = pserialize_read_enter();

	/*
	 * Take another reference to the SA for opencrypto callback.
	 */
	if (__predict_false(sav->state == SADB_SASTATE_DEAD)) {
		pserialize_read_exit(s);
		stat = AH_STAT_NOTDB;
		error = ENOENT;
		goto bad;
	}
	KEY_SA_REF(sav);
	pserialize_read_exit(s);
    }

	/* Crypto operation descriptor. */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length. */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = m;
	crp->crp_callback = ah_input_cb;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = tc;

	/* These are passed as-is to the callback. */
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_nxt = nxt;
	tc->tc_protoff = protoff;
	tc->tc_skip = skip;
	tc->tc_sav = sav;

	DPRINTF("hash over %d bytes, skip %d: "
	     "crda len %d skip %d inject %d\n",
	     crp->crp_ilen, tc->tc_skip,
	     crda->crd_len, crda->crd_skip, crda->crd_inject);

	return crypto_dispatch(crp);

bad:
	if (tc != NULL) {
		if (__predict_true(pool_used))
			pool_cache_put(ah_tdb_crypto_pool_cache, tc);
		else
			kmem_intr_free(tc, size);
	}
	if (crp != NULL)
		crypto_freereq(crp);
	if (m != NULL)
		m_freem(m);
	AH_STATINC(stat);
	return error;
}

#ifdef INET6
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff) do {		     \
	if (saidx->dst.sa.sa_family == AF_INET6) {			     \
		(void)ipsec6_common_input_cb(m, sav, skip, protoff);	     \
	} else {							     \
		(void)ipsec4_common_input_cb(m, sav, skip, protoff);	     \
	}								     \
} while (0)
#else
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff)			     \
	((void)ipsec4_common_input_cb(m, sav, skip, protoff))
#endif

/*
 * AH input callback from the crypto driver.
 */
static void
ah_input_cb(struct cryptop *crp)
{
	char buf[IPSEC_ADDRSTRLEN];
	int rplen, ahsize, skip, protoff;
	unsigned char calc[AH_ALEN_MAX];
	struct mbuf *m;
	struct tdb_crypto *tc;
	struct secasvar *sav;
	struct secasindex *saidx;
	uint8_t nxt;
	char *ptr;
	int authsize;
	bool pool_used;
	size_t size;
	IPSEC_DECLARE_LOCK_VARIABLE;

	KASSERT(crp->crp_opaque != NULL);
	tc = crp->crp_opaque;
	skip = tc->tc_skip;
	nxt = tc->tc_nxt;
	protoff = tc->tc_protoff;
	m = crp->crp_buf;

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	sav = tc->tc_sav;
	saidx = &sav->sah->saidx;
	KASSERTMSG(saidx->dst.sa.sa_family == AF_INET ||
	    saidx->dst.sa.sa_family == AF_INET6,
	    "unexpected protocol family %u", saidx->dst.sa.sa_family);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);
	authsize = AUTHSIZE(sav);
	ahsize = ah_hdrsiz(sav);

	size = sizeof(*tc) + skip + rplen + authsize;
	if (__predict_true(size <= ah_pool_item_size))
		pool_used = true;
	else
		pool_used = false;

	/* Check for crypto errors. */
	if (crp->crp_etype) {
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			IPSEC_RELEASE_GLOBAL_LOCKS();
			(void)crypto_dispatch(crp);
			return;
		}

		AH_STATINC(AH_STAT_NOXFORM);
		DPRINTF("crypto error %d\n", crp->crp_etype);
		goto bad;
	} else {
		AH_STATINC(AH_STAT_HIST + ah_stats[sav->alg_auth]);
		crypto_freereq(crp);		/* No longer needed. */
		crp = NULL;
	}

	if (ipsec_debug)
		memset(calc, 0, sizeof(calc));

	/* Copy authenticator off the packet. */
	m_copydata(m, skip + rplen, authsize, calc);

	ptr = (char *)(tc + 1);
	const uint8_t *pppp = ptr + skip + rplen;

	/* Verify authenticator. */
	if (!consttime_memequal(pppp, calc, authsize)) {
		DPRINTF("authentication hash mismatch " \
		    "over %d bytes " \
		    "for packet in SA %s/%08lx:\n" \
		    "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x, " \
		    "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
		    authsize, ipsec_address(&saidx->dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi),
		     calc[0], calc[1], calc[2], calc[3],
		     calc[4], calc[5], calc[6], calc[7],
		     calc[8], calc[9], calc[10], calc[11],
		     pppp[0], pppp[1], pppp[2], pppp[3],
		     pppp[4], pppp[5], pppp[6], pppp[7],
		     pppp[8], pppp[9], pppp[10], pppp[11]);
		AH_STATINC(AH_STAT_BADAUTH);
		goto bad;
	}

	/* Fix the Next Protocol field. */
	ptr[protoff] = nxt;

	/* Copyback the saved (uncooked) network headers. */
	m_copyback(m, 0, skip, ptr);

	if (__predict_true(pool_used))
		pool_cache_put(ah_tdb_crypto_pool_cache, tc);
	else
		kmem_intr_free(tc, size);
	tc = NULL;

	/*
	 * Header is now authenticated.
	 */
	m->m_flags |= M_AUTHIPHDR;

	/*
	 * Update replay sequence number, if appropriate.
	 */
	if (sav->replay) {
		uint32_t seq;

		m_copydata(m, skip + offsetof(struct newah, ah_seq),
		    sizeof(seq), &seq);
		if (ipsec_updatereplay(ntohl(seq), sav)) {
			AH_STATINC(AH_STAT_REPLAY);
			goto bad;
		}
	}

	/*
	 * Remove the AH header and authenticator from the mbuf.
	 */
	if (m_striphdr(m, skip, ahsize) != 0) {
		DPRINTF("mangled mbuf chain for SA %s/%08lx\n",
		    ipsec_address(&saidx->dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi));

		AH_STATINC(AH_STAT_HDROPS);
		goto bad;
	}

	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff);

	KEY_SA_UNREF(&sav);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	return;

bad:
	if (sav)
		KEY_SA_UNREF(&sav);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	if (m != NULL)
		m_freem(m);
	if (tc != NULL) {
		if (pool_used)
			pool_cache_put(ah_tdb_crypto_pool_cache, tc);
		else
			kmem_intr_free(tc, size);
	}
	if (crp != NULL)
		crypto_freereq(crp);
	return;
}

/*
 * AH output routine, called by ipsec[46]_process_packet().
 */
static int
ah_output(struct mbuf *m, const struct ipsecrequest *isr, struct secasvar *sav,
    int skip, int protoff, int flags)
{
	char buf[IPSEC_ADDRSTRLEN];
	const struct auth_hash *ahx;
	struct cryptodesc *crda;
	struct tdb_crypto *tc;
	struct mbuf *mi;
	struct cryptop *crp;
	uint16_t iplen;
	int error, rplen, authsize, ahsize, maxpacketsize, roff;
	uint8_t prot;
	struct newah *ah;
	size_t ipoffs;
	bool pool_used;

	KASSERT(sav != NULL);
	KASSERT(sav->tdb_authalgxform != NULL);
	ahx = sav->tdb_authalgxform;

	AH_STATINC(AH_STAT_OUTPUT);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);
	authsize = AUTHSIZE(sav);
	ahsize = ah_hdrsiz(sav);

	/* Check for maximum packet size violations. */
	switch (sav->sah->saidx.dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		maxpacketsize = IP_MAXPACKET;
		ipoffs = offsetof(struct ip, ip_len);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		maxpacketsize = IPV6_MAXPACKET;
		ipoffs = offsetof(struct ip6_hdr, ip6_plen);
		break;
#endif
	default:
		DPRINTF("unknown/unsupported protocol "
		    "family %u, SA %s/%08lx\n",
		    sav->sah->saidx.dst.sa.sa_family,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi));
		AH_STATINC(AH_STAT_NOPF);
		error = EPFNOSUPPORT;
		goto bad;
	}
	if (ahsize + m->m_pkthdr.len > maxpacketsize) {
		DPRINTF("packet in SA %s/%08lx got too big "
		    "(len %u, max len %u)\n",
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi),
		    ahsize + m->m_pkthdr.len, maxpacketsize);
		AH_STATINC(AH_STAT_TOOBIG);
		error = EMSGSIZE;
		goto bad;
	}

	/* Update the counters. */
	AH_STATADD(AH_STAT_OBYTES, m->m_pkthdr.len - skip);

	m = m_clone(m);
	if (m == NULL) {
		DPRINTF("cannot clone mbuf chain, SA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi));
		AH_STATINC(AH_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}

	/* Inject AH header. */
	mi = m_makespace(m, skip, ahsize, &roff);
	if (mi == NULL) {
		DPRINTF("failed to inject %u byte AH header for SA "
		    "%s/%08lx\n", ahsize,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi));
		AH_STATINC(AH_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}

	/*
	 * The AH header is guaranteed by m_makespace() to be in
	 * contiguous memory, at roff bytes offset into the returned mbuf.
	 */
	ah = (struct newah *)(mtod(mi, char *) + roff);

	/* Initialize the AH header. */
	m_copydata(m, protoff, sizeof(uint8_t), &ah->ah_nxt);
	ah->ah_len = (ahsize - sizeof(struct ah)) / sizeof(uint32_t);
	ah->ah_reserve = 0;
	ah->ah_spi = sav->spi;

	/* Zeroize authenticator. */
	m_copyback(m, skip + rplen, authsize, ipseczeroes);

	/* Zeroize padding. */
	m_copyback(m, skip + rplen + authsize, ahsize - (rplen + authsize),
	    ipseczeroes);

	/* Insert packet replay counter, as requested.  */
	if (sav->replay) {
		if (sav->replay->count == ~0 &&
		    (sav->flags & SADB_X_EXT_CYCSEQ) == 0) {
			DPRINTF("replay counter wrapped for SA %s/%08lx\n",
			    ipsec_address(&sav->sah->saidx.dst, buf,
			    sizeof(buf)), (u_long) ntohl(sav->spi));
			AH_STATINC(AH_STAT_WRAP);
			error = EINVAL;
			goto bad;
		}
#ifdef IPSEC_DEBUG
		/* Emulate replay attack when ipsec_replay is TRUE. */
		if (!ipsec_replay)
#endif
			sav->replay->count++;
		ah->ah_seq = htonl(sav->replay->count);
	}

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		DPRINTF("failed to acquire crypto descriptors\n");
		AH_STATINC(AH_STAT_CRYPTO);
		error = ENOBUFS;
		goto bad;
	}

	crda = crp->crp_desc;

	crda->crd_skip = 0;
	crda->crd_inject = skip + rplen;
	crda->crd_len = m->m_pkthdr.len;

	/* Authentication operation. */
	crda->crd_alg = ahx->type;
	crda->crd_key = _KEYBUF(sav->key_auth);
	crda->crd_klen = _KEYBITS(sav->key_auth);

	/* Allocate IPsec-specific opaque crypto info. */
	size_t size = sizeof(*tc) + skip;

	if (__predict_true(size <= ah_pool_item_size)) {
		tc = pool_cache_get(ah_tdb_crypto_pool_cache, PR_NOWAIT);
		pool_used = true;
	} else {
		/* size can exceed on IPv6 packets with large options.  */
		tc = kmem_intr_zalloc(size, KM_NOSLEEP);
		pool_used = false;
	}
	if (tc == NULL) {
		DPRINTF("failed to allocate tdb_crypto\n");
		AH_STATINC(AH_STAT_CRYPTO);
		error = ENOBUFS;
		goto bad_crp;
	}

	uint8_t *pext = (char *)(tc + 1);
	/* Save the skipped portion of the packet. */
	m_copydata(m, 0, skip, pext);

	/*
	 * Fix IP header length on the header used for
	 * authentication. We don't need to fix the original
	 * header length as it will be fixed by our caller.
	 */
	memcpy(&iplen, pext + ipoffs, sizeof(iplen));
	iplen = htons(ntohs(iplen) + ahsize);
	m_copyback(m, ipoffs, sizeof(iplen), &iplen);

	/* Fix the Next Header field in saved header. */
	pext[protoff] = IPPROTO_AH;

	/* Update the Next Protocol field in the IP header. */
	prot = IPPROTO_AH;
	m_copyback(m, protoff, sizeof(prot), &prot);

	/* "Massage" the packet headers for crypto processing. */
	error = ah_massage_headers(&m, sav->sah->saidx.dst.sa.sa_family,
	    skip, ahx->type, 1);
	if (error != 0) {
		m = NULL;	/* mbuf was free'd by ah_massage_headers. */
		goto bad_tc;
	}

    {
	int s = pserialize_read_enter();

	/*
	 * Take another reference to the SP and the SA for opencrypto callback.
	 */
	if (__predict_false(isr->sp->state == IPSEC_SPSTATE_DEAD ||
	    sav->state == SADB_SASTATE_DEAD)) {
		pserialize_read_exit(s);
		AH_STATINC(AH_STAT_NOTDB);
		error = ENOENT;
		goto bad_tc;
	}
	KEY_SP_REF(isr->sp);
	KEY_SA_REF(sav);
	pserialize_read_exit(s);
    }

	/* Crypto operation descriptor. */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length. */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = m;
	crp->crp_callback = ah_output_cb;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = tc;

	/* These are passed as-is to the callback. */
	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;
	tc->tc_flags = flags;
	tc->tc_sav = sav;

	return crypto_dispatch(crp);

bad_tc:
	if (__predict_true(pool_used))
		pool_cache_put(ah_tdb_crypto_pool_cache, tc);
	else
		kmem_intr_free(tc, size);
bad_crp:
	crypto_freereq(crp);
bad:
	if (m)
		m_freem(m);
	return error;
}

/*
 * AH output callback from the crypto driver.
 */
static void
ah_output_cb(struct cryptop *crp)
{
	int skip;
	struct tdb_crypto *tc;
	const struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m;
	void *ptr;
	int flags;
	size_t size;
	bool pool_used;
	IPSEC_DECLARE_LOCK_VARIABLE;

	KASSERT(crp->crp_opaque != NULL);
	tc = crp->crp_opaque;
	skip = tc->tc_skip;
	ptr = (tc + 1);
	m = crp->crp_buf;
	size = sizeof(*tc) + skip;
	pool_used = size <= ah_pool_item_size;

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	isr = tc->tc_isr;
	sav = tc->tc_sav;

	/* Check for crypto errors. */
	if (crp->crp_etype) {
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			IPSEC_RELEASE_GLOBAL_LOCKS();
			(void)crypto_dispatch(crp);
			return;
		}

		AH_STATINC(AH_STAT_NOXFORM);
		DPRINTF("crypto error %d\n", crp->crp_etype);
		goto bad;
	}

	AH_STATINC(AH_STAT_HIST + ah_stats[sav->alg_auth]);

	/*
	 * Copy original headers (with the new protocol number) back
	 * in place.
	 */
	m_copyback(m, 0, skip, ptr);

	flags = tc->tc_flags;
	/* No longer needed. */
	if (__predict_true(pool_used))
		pool_cache_put(ah_tdb_crypto_pool_cache, tc);
	else
		kmem_intr_free(tc, size);
	crypto_freereq(crp);

#ifdef IPSEC_DEBUG
	/* Emulate man-in-the-middle attack when ipsec_integrity is TRUE. */
	if (ipsec_integrity) {
		int alen;

		/*
		 * Corrupt HMAC if we want to test integrity verification of
		 * the other side.
		 */
		alen = AUTHSIZE(sav);
		m_copyback(m, m->m_pkthdr.len - alen, alen, ipseczeroes);
	}
#endif

	/* NB: m is reclaimed by ipsec_process_done. */
	(void)ipsec_process_done(m, isr, sav, flags);
	KEY_SA_UNREF(&sav);
	KEY_SP_UNREF(&isr->sp);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	return;
bad:
	if (sav)
		KEY_SA_UNREF(&sav);
	KEY_SP_UNREF(&isr->sp);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	if (m)
		m_freem(m);
	if (__predict_true(pool_used))
		pool_cache_put(ah_tdb_crypto_pool_cache, tc);
	else
		kmem_intr_free(tc, size);
	crypto_freereq(crp);
}

static struct xformsw ah_xformsw = {
	.xf_type	= XF_AH,
	.xf_flags	= XFT_AUTH,
	.xf_name	= "IPsec AH",
	.xf_init	= ah_init,
	.xf_zeroize	= ah_zeroize,
	.xf_input	= ah_input,
	.xf_output	= ah_output,
	.xf_next	= NULL,
};

void
ah_attach(void)
{
	ahstat_percpu = percpu_alloc(sizeof(uint64_t) * AH_NSTATS);

#define MAXAUTHSIZE(name)						\
	if ((auth_hash_ ## name).authsize > ah_max_authsize)		\
		ah_max_authsize = (auth_hash_ ## name).authsize

	ah_max_authsize = 0;
	MAXAUTHSIZE(null);
	MAXAUTHSIZE(md5);
	MAXAUTHSIZE(sha1);
	MAXAUTHSIZE(key_md5);
	MAXAUTHSIZE(key_sha1);
	MAXAUTHSIZE(hmac_md5);
	MAXAUTHSIZE(hmac_sha1);
	MAXAUTHSIZE(hmac_ripemd_160);
	MAXAUTHSIZE(hmac_md5_96);
	MAXAUTHSIZE(hmac_sha1_96);
	MAXAUTHSIZE(hmac_ripemd_160_96);
	MAXAUTHSIZE(hmac_sha2_256);
	MAXAUTHSIZE(hmac_sha2_384);
	MAXAUTHSIZE(hmac_sha2_512);
	MAXAUTHSIZE(aes_xcbc_mac_96);
	MAXAUTHSIZE(gmac_aes_128);
	MAXAUTHSIZE(gmac_aes_192);
	MAXAUTHSIZE(gmac_aes_256);
	IPSECLOG(LOG_DEBUG, "ah_max_authsize=%d\n", ah_max_authsize);

#undef MAXAUTHSIZE

	ah_pool_item_size = sizeof(struct tdb_crypto) +
	    sizeof(struct ip) + MAX_IPOPTLEN +
	    sizeof(struct ah) + sizeof(uint32_t) + ah_max_authsize;
	ah_tdb_crypto_pool_cache = pool_cache_init(ah_pool_item_size,
	    coherency_unit, 0, 0, "ah_tdb_crypto", NULL, IPL_SOFTNET,
	    NULL, NULL, NULL);

	xform_register(&ah_xformsw);
}
