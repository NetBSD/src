/*	$NetBSD: xform_ah.c,v 1.83 2018/02/15 09:17:37 ozaki-r Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_ah.c,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: xform_ah.c,v 1.83 2018/02/15 09:17:37 ozaki-r Exp $");

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

#ifdef __FreeBSD__
SYSCTL_DECL(_net_inet_ah);
SYSCTL_INT(_net_inet_ah, OID_AUTO,
	ah_enable,	CTLFLAG_RW,	&ah_enable,	0, "");
SYSCTL_INT(_net_inet_ah, OID_AUTO,
	ah_cleartos,	CTLFLAG_RW,	&ip4_ah_cleartos,	0, "");
SYSCTL_STRUCT(_net_inet_ah, IPSECCTL_STATS,
	stats,		CTLFLAG_RD,	&ahstat,	ahstat, "");
#endif /* __FreeBSD__ */

static unsigned char ipseczeroes[256];	/* larger than an ip6 extension hdr */

int ah_max_authsize;			/* max authsize over all algorithms */

static int ah_input_cb(struct cryptop *);
static int ah_output_cb(struct cryptop *);

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
ah_hdrsiz(const struct secasvar *sav)
{
	size_t size;

	if (sav != NULL) {
		int authsize;
		KASSERT(sav->tdb_authalgxform != NULL);
		/*XXX not right for null algorithm--does it matter??*/
		authsize = AUTHSIZE(sav);
		size = roundup(authsize, sizeof(uint32_t)) + HDRSIZE(sav);
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
		DPRINTF(("%s: unsupported authentication algorithm %u\n",
			__func__, sav->alg_auth));
		return EINVAL;
	}
	/*
	 * Verify the replay state block allocation is consistent with
	 * the protocol type.  We check here so we can make assumptions
	 * later during protocol processing.
	 */
	/* NB: replay state is setup elsewhere (sigh) */
	if (((sav->flags&SADB_X_EXT_OLD) == 0) ^ (sav->replay != NULL)) {
		DPRINTF(("%s: replay state block inconsistency, "
			"%s algorithm %s replay state\n", __func__,
			(sav->flags & SADB_X_EXT_OLD) ? "old" : "new",
			sav->replay == NULL ? "without" : "with"));
		return EINVAL;
	}
	if (sav->key_auth == NULL) {
		DPRINTF(("%s: no authentication key for %s algorithm\n",
			__func__, thash->name));
		return EINVAL;
	}
	keylen = _KEYLEN(sav->key_auth);
	if (keylen != thash->keysize && thash->keysize != 0) {
		DPRINTF(("%s: invalid keylength %d, algorithm %s requires "
			 "keysize %d\n", __func__,
			 keylen, thash->name, thash->keysize));
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
int
ah_zeroize(struct secasvar *sav)
{
	int err;

	if (sav->key_auth) {
		explicit_memset(_KEYBUF(sav->key_auth), 0,
		    _KEYLEN(sav->key_auth));
	}

	err = crypto_freesession(sav->tdb_cryptoid);
	sav->tdb_cryptoid = 0;
	sav->tdb_authalgxform = NULL;
	sav->tdb_xform = NULL;
	return err;
}

/*
 * Massage IPv4/IPv6 headers for AH processing.
 */
static int
ah_massage_headers(struct mbuf **m0, int proto, int skip, int alg, int out)
{
	struct mbuf *m = *m0;
	unsigned char *ptr;
	int off, count;
#ifdef INET
	struct ip *ip;
#endif
#ifdef INET6
	struct ip6_ext *ip6e;
	struct ip6_hdr ip6;
	struct ip6_rthdr *rh;
	int alloc, ad, nxt;
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
			DPRINTF(("%s: m_pullup failed\n", __func__));
			return ENOBUFS;
		}

		/* Fix the IP header */
		ip = mtod(m, struct ip *);
		if (ip4_ah_cleartos)
			ip->ip_tos = 0;
		ip->ip_ttl = 0;
		ip->ip_sum = 0;
		ip->ip_off = htons(ntohs(ip->ip_off) & ip4_ah_offsetmask);

		/*
		 * On FreeBSD, ip_off and ip_len assumed in host endian;
		 * they are converted (if necessary) by ip_input().
		 * On NetBSD, ip_off and ip_len are in network byte order.
		 * They must be massaged back to network byte order
		 * before verifying the  HMAC. Moreover, on FreeBSD,
		 * we should add `skip' back into the massaged ip_len
		 * (presumably ip_input() deducted it before we got here?)
		 * whereas on NetBSD, we should not.
		 */
		if (!out) {
			uint16_t inlen = ntohs(ip->ip_len);

			ip->ip_len = htons(inlen);

			if (alg == CRYPTO_MD5_KPDK || alg == CRYPTO_SHA1_KPDK)
				ip->ip_off  &= htons(IP_DF);
			else
				ip->ip_off = 0;
		} else {
			if (alg == CRYPTO_MD5_KPDK || alg == CRYPTO_SHA1_KPDK)
				ip->ip_off &= htons(IP_DF);
			else
				ip->ip_off = 0;
		}

		ptr = mtod(m, unsigned char *);

		/* IPv4 option processing */
		for (off = sizeof(struct ip); off < skip;) {
			if (ptr[off] == IPOPT_EOL || ptr[off] == IPOPT_NOP ||
			    off + 1 < skip)
				;
			else {
				DPRINTF(("%s: illegal IPv4 option length for "
				    "option %d\n", __func__, ptr[off]));

				m_freem(m);
				return EINVAL;
			}

			switch (ptr[off]) {
			case IPOPT_EOL:
				off = skip;  /* End the loop. */
				break;

			case IPOPT_NOP:
				off++;
				break;

			case IPOPT_SECURITY:	/* 0x82 */
			case 0x85:	/* Extended security. */
			case 0x86:	/* Commercial security. */
			case 0x94:	/* Router alert */
			case 0x95:	/* RFC1770 */
				/* Sanity check for option length. */
				if (ptr[off + 1] < 2) {
					DPRINTF(("%s: illegal IPv4 option "
					    "length for option %d\n", __func__,
					    ptr[off]));

					m_freem(m);
					return EINVAL;
				}

				off += ptr[off + 1];
				break;

			case IPOPT_LSRR:
			case IPOPT_SSRR:
				/* Sanity check for option length. */
				if (ptr[off + 1] < 2) {
					DPRINTF(("%s: illegal IPv4 option "
					    "length for option %d\n", __func__,
					    ptr[off]));

					m_freem(m);
					return EINVAL;
				}

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
					    ptr + off + ptr[off + 1] -
					    sizeof(struct in_addr),
					    sizeof(struct in_addr));

				/* Fall through */
			default:
				/* Sanity check for option length. */
				if (ptr[off + 1] < 2) {
					DPRINTF(("%s: illegal IPv4 option "
					    "length for option %d\n", __func__,
					    ptr[off]));
					m_freem(m);
					return EINVAL;
				}

				/* Zeroize all other options. */
				count = ptr[off + 1];
				memcpy(ptr + off, ipseczeroes, count);
				off += count;
				break;
			}

			/* Sanity check. */
			if (off > skip)	{
				DPRINTF(("%s: malformed IPv4 options header\n",
					__func__));
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
			DPRINTF(("%s: unsupported IPv6 jumbogram\n", __func__));
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

		/* Let's deal with the remaining headers (if any). */
		if (skip - sizeof(struct ip6_hdr) > 0) {
			if (m->m_len <= skip) {
				ptr = malloc(skip - sizeof(struct ip6_hdr),
				    M_XDATA, M_NOWAIT);
				if (ptr == NULL) {
					DPRINTF(("%s: failed to allocate "
					    "memory for IPv6 headers\n",
					    __func__));
					m_freem(m);
					return ENOBUFS;
				}

				/*
				 * Copy all the protocol headers after
				 * the IPv6 header.
				 */
				m_copydata(m, sizeof(struct ip6_hdr),
				    skip - sizeof(struct ip6_hdr), ptr);
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

		for (off = 0; off < skip - sizeof(struct ip6_hdr);) {
			int noff;

			switch (nxt) {
			case IPPROTO_HOPOPTS:
			case IPPROTO_DSTOPTS:
				ip6e = (struct ip6_ext *)(ptr + off);
				noff = off + ((ip6e->ip6e_len + 1) << 3);

				/* Sanity check. */
				if (noff > skip - sizeof(struct ip6_hdr)) {
					goto error6;
				}

				/*
				 * Zero out mutable options.
				 */
				for (count = off + sizeof(struct ip6_ext);
				     count < noff;) {
					if (ptr[count] == IP6OPT_PAD1) {
						count++;
						continue;
					}

					ad = ptr[count + 1] + 2;

					if (count + ad > noff) {
						goto error6;
					}

					if (ptr[count] & IP6OPT_MUTABLE) {
						memset(ptr + count, 0, ad);
					}

					count += ad;
				}

				if (count != noff) {
					goto error6;
				}

				/* Advance. */
				off += ((ip6e->ip6e_len + 1) << 3);
				nxt = ip6e->ip6e_nxt;
				break;

			case IPPROTO_ROUTING:
				/*
				 * Always include routing headers in
				 * computation.
				 */
				ip6e = (struct ip6_ext *) (ptr + off);
				rh = (struct ip6_rthdr *)(ptr + off);
				/*
				 * must adjust content to make it look like
				 * its final form (as seen at the final
				 * destination).
				 * we only know how to massage type 0 routing
				 * header.
				 */
				if (out && rh->ip6r_type == IPV6_RTHDR_TYPE_0) {
					struct ip6_rthdr0 *rh0;
					struct in6_addr *addr, finaldst;
					int i;

					rh0 = (struct ip6_rthdr0 *)rh;
					addr = (struct in6_addr *)(rh0 + 1);

					for (i = 0; i < rh0->ip6r0_segleft; i++)
						in6_clearscope(&addr[i]);

					finaldst = addr[rh0->ip6r0_segleft - 1];
					memmove(&addr[1], &addr[0],
						sizeof(struct in6_addr) *
						(rh0->ip6r0_segleft - 1));

					m_copydata(m, 0, sizeof(ip6), &ip6);
					addr[0] = ip6.ip6_dst;
					ip6.ip6_dst = finaldst;
					m_copyback(m, 0, sizeof(ip6), &ip6);

					rh0->ip6r0_segleft = 0;
				}

				/* advance */
				off += ((ip6e->ip6e_len + 1) << 3);
				nxt = ip6e->ip6e_nxt;
				break;

			default:
				DPRINTF(("%s: unexpected IPv6 header type %d\n",
				    __func__, off));
error6:
				if (alloc)
					free(ptr, M_XDATA);
				m_freem(m);
				return EINVAL;
			}
		}

		/* Copyback and free, if we allocated. */
		if (alloc) {
			m_copyback(m, sizeof(struct ip6_hdr),
			    skip - sizeof(struct ip6_hdr), ptr);
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
	int hl, rplen, authsize, error, stat = AH_STAT_HDROPS;
	struct cryptodesc *crda;
	struct cryptop *crp = NULL;
	bool pool_used;
	uint8_t nxt;

	IPSEC_SPLASSERT_SOFTNET(__func__);

	KASSERT(sav != NULL);
	KASSERT(sav->key_auth != NULL);
	KASSERT(sav->tdb_authalgxform != NULL);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);

	/* XXX don't pullup, just copy header */
	IP6_EXTHDR_GET(ah, struct newah *, m, skip, rplen);
	if (ah == NULL) {
		DPRINTF(("%s: cannot pullup header\n", __func__));
		error = ENOBUFS;
		stat = AH_STAT_HDROPS;	/*XXX*/
		goto bad;
	}

	nxt = ah->ah_nxt;

	/* Check replay window, if applicable. */
	if (sav->replay && !ipsec_chkreplay(ntohl(ah->ah_seq), sav)) {
		char buf[IPSEC_LOGSASTRLEN];
		DPRINTF(("%s: packet replay failure: %s\n", __func__,
		    ipsec_logsastr(sav, buf, sizeof(buf))));
		stat = AH_STAT_REPLAY;
		error = ENOBUFS;
		goto bad;
	}

	/* Verify AH header length. */
	hl = ah->ah_len * sizeof(uint32_t);
	ahx = sav->tdb_authalgxform;
	authsize = AUTHSIZE(sav);
	if (hl != authsize + rplen - sizeof(struct ah)) {
		char buf[IPSEC_ADDRSTRLEN];
		DPRINTF(("%s: bad authenticator length %u (expecting %lu)"
			" for packet in SA %s/%08lx\n", __func__,
			hl, (u_long) (authsize + rplen - sizeof(struct ah)),
			ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
			(u_long) ntohl(sav->spi)));
		stat = AH_STAT_BADAUTHL;
		error = EACCES;
		goto bad;
	}
	if (skip + authsize + rplen > m->m_pkthdr.len) {
		char buf[IPSEC_ADDRSTRLEN];
		DPRINTF(("%s: bad mbuf length %u (expecting >= %lu)"
			" for packet in SA %s/%08lx\n", __func__,
			m->m_pkthdr.len, (u_long)(skip + authsize + rplen),
			ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
			(u_long) ntohl(sav->spi)));
		stat = AH_STAT_BADAUTHL;
		error = EACCES;
		goto bad;
	}

	AH_STATADD(AH_STAT_IBYTES, m->m_pkthdr.len - skip - hl);

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		DPRINTF(("%s: failed to acquire crypto descriptor\n", __func__));
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
		DPRINTF(("%s: failed to allocate tdb_crypto\n", __func__));
		stat = AH_STAT_CRYPTO;
		error = ENOBUFS;
		goto bad;
	}

	error = m_makewritable(&m, 0, extra, M_NOWAIT);
	if (error) {
		DPRINTF(("%s: failed to m_makewritable\n", __func__));
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

	DPRINTF(("%s: hash over %d bytes, skip %d: "
		 "crda len %d skip %d inject %d\n", __func__,
		 crp->crp_ilen, tc->tc_skip,
		 crda->crd_len, crda->crd_skip, crda->crd_inject));

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
		error = ipsec6_common_input_cb(m, sav, skip, protoff);	     \
	} else {							     \
		error = ipsec4_common_input_cb(m, sav, skip, protoff);	     \
	}								     \
} while (0)
#else
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff)			     \
	(error = ipsec4_common_input_cb(m, sav, skip, protoff))
#endif

/*
 * AH input callback from the crypto driver.
 */
static int
ah_input_cb(struct cryptop *crp)
{
	char buf[IPSEC_ADDRSTRLEN];
	int rplen, error, skip, protoff;
	unsigned char calc[AH_ALEN_MAX];
	struct mbuf *m;
	struct tdb_crypto *tc;
	struct secasvar *sav;
	struct secasindex *saidx;
	uint8_t nxt;
	char *ptr;
	int authsize;
	uint16_t dport;
	uint16_t sport;
	bool pool_used;
	size_t size;
	IPSEC_DECLARE_LOCK_VARIABLE;

	KASSERT(crp->crp_opaque != NULL);
	tc = crp->crp_opaque;
	skip = tc->tc_skip;
	nxt = tc->tc_nxt;
	protoff = tc->tc_protoff;
	m = crp->crp_buf;


	/* find the source port for NAT-T */
	nat_t_ports_get(m, &dport, &sport);

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	sav = tc->tc_sav;
	saidx = &sav->sah->saidx;
	KASSERTMSG(saidx->dst.sa.sa_family == AF_INET ||
	    saidx->dst.sa.sa_family == AF_INET6,
	    "unexpected protocol family %u", saidx->dst.sa.sa_family);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);
	authsize = AUTHSIZE(sav);

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
			return crypto_dispatch(crp);
		}

		AH_STATINC(AH_STAT_NOXFORM);
		DPRINTF(("%s: crypto error %d\n", __func__, crp->crp_etype));
		error = crp->crp_etype;
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
		DPRINTF(("%s: authentication hash mismatch " \
		    "over %d bytes " \
		    "for packet in SA %s/%08lx:\n" \
	    "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x, " \
	    "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
		    __func__, authsize,
		    ipsec_address(&saidx->dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi),
			 calc[0], calc[1], calc[2], calc[3],
			 calc[4], calc[5], calc[6], calc[7],
			 calc[8], calc[9], calc[10], calc[11],
			 pppp[0], pppp[1], pppp[2], pppp[3],
			 pppp[4], pppp[5], pppp[6], pppp[7],
			 pppp[8], pppp[9], pppp[10], pppp[11]
			 ));
		AH_STATINC(AH_STAT_BADAUTH);
		error = EACCES;
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
	m->m_flags |= M_AUTHIPHDR|M_AUTHIPDGM;

	/*
	 * Update replay sequence number, if appropriate.
	 */
	if (sav->replay) {
		uint32_t seq;

		m_copydata(m, skip + offsetof(struct newah, ah_seq),
		    sizeof(seq), &seq);
		if (ipsec_updatereplay(ntohl(seq), sav)) {
			AH_STATINC(AH_STAT_REPLAY);
			error = ENOBUFS; /* XXX as above */
			goto bad;
		}
	}

	/*
	 * Remove the AH header and authenticator from the mbuf.
	 */
	error = m_striphdr(m, skip, rplen + authsize);
	if (error) {
		DPRINTF(("%s: mangled mbuf chain for SA %s/%08lx\n", __func__,
		    ipsec_address(&saidx->dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));

		AH_STATINC(AH_STAT_HDROPS);
		goto bad;
	}

	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff);

	KEY_SA_UNREF(&sav);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	return error;

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
	return error;
}

/*
 * AH output routine, called by ipsec[46]_process_packet().
 */
static int
ah_output(struct mbuf *m, const struct ipsecrequest *isr, struct secasvar *sav,
    struct mbuf **mp, int skip, int protoff)
{
	char buf[IPSEC_ADDRSTRLEN];
	const struct auth_hash *ahx;
	struct cryptodesc *crda;
	struct tdb_crypto *tc;
	struct mbuf *mi;
	struct cryptop *crp;
	uint16_t iplen;
	int error, rplen, authsize, maxpacketsize, roff;
	uint8_t prot;
	struct newah *ah;
	size_t ipoffs;

	IPSEC_SPLASSERT_SOFTNET(__func__);

	KASSERT(sav != NULL);
	KASSERT(sav->tdb_authalgxform != NULL);
	ahx = sav->tdb_authalgxform;

	AH_STATINC(AH_STAT_OUTPUT);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);

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
		DPRINTF(("%s: unknown/unsupported protocol "
		    "family %u, SA %s/%08lx\n", __func__,
		    sav->sah->saidx.dst.sa.sa_family,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_NOPF);
		error = EPFNOSUPPORT;
		goto bad;
	}
	authsize = AUTHSIZE(sav);
	if (rplen + authsize + m->m_pkthdr.len > maxpacketsize) {
		DPRINTF(("%s: packet in SA %s/%08lx got too big "
		    "(len %u, max len %u)\n", __func__,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi),
		    rplen + authsize + m->m_pkthdr.len, maxpacketsize));
		AH_STATINC(AH_STAT_TOOBIG);
		error = EMSGSIZE;
		goto bad;
	}

	/* Update the counters. */
	AH_STATADD(AH_STAT_OBYTES, m->m_pkthdr.len - skip);

	m = m_clone(m);
	if (m == NULL) {
		DPRINTF(("%s: cannot clone mbuf chain, SA %s/%08lx\n", __func__,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}

	/* Inject AH header. */
	mi = m_makespace(m, skip, rplen + authsize, &roff);
	if (mi == NULL) {
		DPRINTF(("%s: failed to inject %u byte AH header for SA "
		    "%s/%08lx\n", __func__,
		    rplen + authsize,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
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
	ah->ah_len = (rplen + authsize - sizeof(struct ah)) / sizeof(uint32_t);
	ah->ah_reserve = 0;
	ah->ah_spi = sav->spi;

	/* Zeroize authenticator. */
	m_copyback(m, skip + rplen, authsize, ipseczeroes);

	/* Insert packet replay counter, as requested.  */
	if (sav->replay) {
		if (sav->replay->count == ~0 &&
		    (sav->flags & SADB_X_EXT_CYCSEQ) == 0) {
			DPRINTF(("%s: replay counter wrapped for SA %s/%08lx\n",
			    __func__, ipsec_address(&sav->sah->saidx.dst, buf,
			    sizeof(buf)), (u_long) ntohl(sav->spi)));
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
		DPRINTF(("%s: failed to acquire crypto descriptors\n",
		    __func__));
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
	tc = pool_cache_get(ah_tdb_crypto_pool_cache, PR_NOWAIT);
	if (tc == NULL) {
		DPRINTF(("%s: failed to allocate tdb_crypto\n", __func__));
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
	iplen = htons(ntohs(iplen) + rplen + authsize);
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
	tc->tc_sav = sav;

	return crypto_dispatch(crp);
bad_tc:
	pool_cache_put(ah_tdb_crypto_pool_cache, tc);
bad_crp:
	crypto_freereq(crp);
bad:
	if (m)
		m_freem(m);
	return (error);
}

/*
 * AH output callback from the crypto driver.
 */
static int
ah_output_cb(struct cryptop *crp)
{
	int skip, error;
	struct tdb_crypto *tc;
	const struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m;
	void *ptr;
	int err;
	IPSEC_DECLARE_LOCK_VARIABLE;

	KASSERT(crp->crp_opaque != NULL);
	tc = crp->crp_opaque;
	skip = tc->tc_skip;
	ptr = (tc + 1);
	m = crp->crp_buf;

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	isr = tc->tc_isr;
	sav = tc->tc_sav;

	/* Check for crypto errors. */
	if (crp->crp_etype) {
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			IPSEC_RELEASE_GLOBAL_LOCKS();
			return crypto_dispatch(crp);
		}

		AH_STATINC(AH_STAT_NOXFORM);
		DPRINTF(("%s: crypto error %d\n", __func__, crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	AH_STATINC(AH_STAT_HIST + ah_stats[sav->alg_auth]);

	/*
	 * Copy original headers (with the new protocol number) back
	 * in place.
	 */
	m_copyback(m, 0, skip, ptr);

	/* No longer needed. */
	pool_cache_put(ah_tdb_crypto_pool_cache, tc);
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
	err = ipsec_process_done(m, isr, sav);
	KEY_SA_UNREF(&sav);
	KEY_SP_UNREF(&isr->sp);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	return err;
bad:
	if (sav)
		KEY_SA_UNREF(&sav);
	KEY_SP_UNREF(&isr->sp);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	if (m)
		m_freem(m);
	pool_cache_put(ah_tdb_crypto_pool_cache, tc);
	crypto_freereq(crp);
	return error;
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
