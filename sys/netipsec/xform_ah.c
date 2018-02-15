/*	$NetBSD: xform_ah.c,v 1.42.12.2 2018/02/15 08:05:01 martin Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: xform_ah.c,v 1.42.12.2 2018/02/15 08:05:01 martin Exp $");

#include "opt_inet.h"
#ifdef __FreeBSD__
#include "opt_inet6.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/sysctl.h>
#include <sys/socketvar.h> /* for softnet_lock */

#include <net/if.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_ecn.h>
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
#  ifdef __FreeBSD__
#  include <netinet6/ip6_ecn.h>
#  endif
#endif

#include <netipsec/key.h>
#include <netipsec/key_debug.h>
#include <netipsec/ipsec_osdep.h>

#include <opencrypto/cryptodev.h>

/*
 * Return header size in bytes.  The old protocol did not support
 * the replay counter; the new protocol always includes the counter.
 */
#define HDRSIZE(sav) \
	(((sav)->flags & SADB_X_EXT_OLD) ? \
		sizeof (struct ah) : sizeof (struct ah) + sizeof (u_int32_t))
/*
 * Return authenticator size in bytes.  The old protocol is known
 * to use a fixed 16-byte authenticator.  The new algorithm gets
 * this size from the xform but is (currently) always 12.
 */
#define	AUTHSIZE(sav) \
	((sav->flags & SADB_X_EXT_OLD) ? 16 : (sav)->tdb_authalgxform->authsize)

percpu_t *ahstat_percpu;

int	ah_enable = 1;			/* control flow of packets with AH */
int	ip4_ah_cleartos = 1;		/* clear ip_tos when doing AH calc */

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

static int ah_input_cb(struct cryptop*);
static int ah_output_cb(struct cryptop*);

/*
 * NB: this is public for use by the PF_KEY support.
 */
const struct auth_hash *
ah_algorithm_lookup(int alg)
{
	if (alg >= AH_ALG_MAX)
		return NULL;
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
		IPSEC_ASSERT(sav->tdb_authalgxform != NULL,
			("ah_hdrsiz: null xform"));
		/*XXX not right for null algorithm--does it matter??*/
		authsize = AUTHSIZE(sav);
		size = roundup(authsize, sizeof (u_int32_t)) + HDRSIZE(sav);
	} else {
		/* default guess */
		size = sizeof (struct ah) + sizeof (u_int32_t) + 16;
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
		DPRINTF(("ah_init: unsupported authentication algorithm %u\n",
			sav->alg_auth));
		return EINVAL;
	}
	/*
	 * Verify the replay state block allocation is consistent with
	 * the protocol type.  We check here so we can make assumptions
	 * later during protocol processing.
	 */
	/* NB: replay state is setup elsewhere (sigh) */
	if (((sav->flags&SADB_X_EXT_OLD) == 0) ^ (sav->replay != NULL)) {
		DPRINTF(("ah_init: replay state block inconsistency, "
			"%s algorithm %s replay state\n",
			(sav->flags & SADB_X_EXT_OLD) ? "old" : "new",
			sav->replay == NULL ? "without" : "with"));
		return EINVAL;
	}
	if (sav->key_auth == NULL) {
		DPRINTF(("ah_init: no authentication key for %s "
			"algorithm\n", thash->name));
		return EINVAL;
	}
	keylen = _KEYLEN(sav->key_auth);
	if (keylen != thash->keysize && thash->keysize != 0) {
		DPRINTF(("ah_init: invalid keylength %d, algorithm "
			 "%s requires keysize %d\n",
			 keylen, thash->name, thash->keysize));
		return EINVAL;
	}

	sav->tdb_xform = xsp;
	sav->tdb_authalgxform = thash;

	/* Initialize crypto session. */
	memset(cria, 0, sizeof (*cria));
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

	if (sav->key_auth)
		memset(_KEYBUF(sav->key_auth), 0, _KEYLEN(sav->key_auth));

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
#endif /* INET */

#ifdef INET6
	struct ip6_ext *ip6e;
	struct ip6_hdr ip6;
	int alloc, ad, nxt;
#endif /* INET6 */

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
			DPRINTF(("ah_massage_headers: m_pullup failed\n"));
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
#ifdef __FreeBSD__
  #define TOHOST(x) (x)
#else
  #define TOHOST(x) (ntohs(x))
#endif
		if (!out) {
			u_int16_t inlen = TOHOST(ip->ip_len);

#ifdef __FreeBSD__
			ip->ip_len = htons(inlen + skip);
#else  /*!__FreeBSD__ */
			ip->ip_len = htons(inlen);
#endif /*!__FreeBSD__ */

			if (alg == CRYPTO_MD5_KPDK || alg == CRYPTO_SHA1_KPDK)
				ip->ip_off  &= IP_OFF_CONVERT(IP_DF);
			else
				ip->ip_off = 0;
		} else {
			if (alg == CRYPTO_MD5_KPDK || alg == CRYPTO_SHA1_KPDK)
				ip->ip_off &= IP_OFF_CONVERT(IP_DF);
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
				DPRINTF(("ah_massage_headers: illegal IPv4 "
				    "option length for option %d\n",
				    ptr[off]));

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
					DPRINTF(("ah_massage_headers: "
					    "illegal IPv4 option length for "
					    "option %d\n", ptr[off]));

					m_freem(m);
					return EINVAL;
				}

				off += ptr[off + 1];
				break;

			case IPOPT_LSRR:
			case IPOPT_SSRR:
				/* Sanity check for option length. */
				if (ptr[off + 1] < 2) {
					DPRINTF(("ah_massage_headers: "
					    "illegal IPv4 option length for "
					    "option %d\n", ptr[off]));

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
					bcopy(ptr + off + ptr[off + 1] -
					    sizeof(struct in_addr),
					    &(ip->ip_dst), sizeof(struct in_addr));

				/* Fall through */
			default:
				/* Sanity check for option length. */
				if (ptr[off + 1] < 2) {
					DPRINTF(("ah_massage_headers: "
					    "illegal IPv4 option length for "
					    "option %d\n", ptr[off]));
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
				DPRINTF(("ah_massage_headers(): malformed "
				    "IPv4 options header\n"));

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
			DPRINTF(("ah_massage_headers: unsupported IPv6 jumbogram\n"));
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
				ptr = (unsigned char *) malloc(
				    skip - sizeof(struct ip6_hdr),
				    M_XDATA, M_NOWAIT);
				if (ptr == NULL) {
					DPRINTF(("ah_massage_headers: failed "
					    "to allocate memory for IPv6 "
					    "headers\n"));
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

		for (off = 0; off < skip - sizeof(struct ip6_hdr);)
			switch (nxt) {
			case IPPROTO_HOPOPTS:
			case IPPROTO_DSTOPTS:
				ip6e = (struct ip6_ext *) (ptr + off);

				/*
				 * Process the mutable/immutable
				 * options -- borrows heavily from the
				 * KAME code.
				 */
				for (count = off + sizeof(struct ip6_ext);
				     count < off + ((ip6e->ip6e_len + 1) << 3);) {
					if (ptr[count] == IP6OPT_PAD1) {
						count++;
						continue; /* Skip padding. */
					}

					/* Sanity check. */
					if (count > off +
					    ((ip6e->ip6e_len + 1) << 3)) {
						m_freem(m);

						/* Free, if we allocated. */
						if (alloc)
							free(ptr, M_XDATA);
						return EINVAL;
					}

					ad = ptr[count + 1] + 2;

					/* If mutable option, zeroize. */
					if (ptr[count] & IP6OPT_MUTABLE)
						memcpy(ptr + count, ipseczeroes,
						    ad);

					count += ad;

					/* Sanity check. */
					if (count >
					    skip - sizeof(struct ip6_hdr)) {
						m_freem(m);

						/* Free, if we allocated. */
						if (alloc)
							free(ptr, M_XDATA);
						return EINVAL;
					}
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
				{
					struct ip6_rthdr *rh;

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
				}

			default:
				DPRINTF(("ah_massage_headers: unexpected "
				    "IPv6 header type %d", off));
				if (alloc)
					free(ptr, M_XDATA);
				m_freem(m);
				return EINVAL;
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
ah_input(struct mbuf *m, const struct secasvar *sav, int skip, int protoff)
{
	const struct auth_hash *ahx;
	struct tdb_ident *tdbi;
	struct tdb_crypto *tc;
	struct m_tag *mtag;
	struct newah *ah;
	int hl, rplen, authsize, error;
	uint8_t nxt;

	struct cryptodesc *crda;
	struct cryptop *crp;

	IPSEC_SPLASSERT_SOFTNET("ah_input");

	IPSEC_ASSERT(sav != NULL, ("ah_input: null SA"));
	IPSEC_ASSERT(sav->key_auth != NULL,
		("ah_input: null authentication key"));
	IPSEC_ASSERT(sav->tdb_authalgxform != NULL,
		("ah_input: null authentication xform"));

	/* Figure out header size. */
	rplen = HDRSIZE(sav);

	/* XXX don't pullup, just copy header */
	IP6_EXTHDR_GET(ah, struct newah *, m, skip, rplen);
	if (ah == NULL) {
		DPRINTF(("ah_input: cannot pullup header\n"));
		AH_STATINC(AH_STAT_HDROPS);	/*XXX*/
		m_freem(m);
		return ENOBUFS;
	}

	nxt = ah->ah_nxt;

	/* Check replay window, if applicable. */
	if (sav->replay && !ipsec_chkreplay(ntohl(ah->ah_seq), sav)) {
		AH_STATINC(AH_STAT_REPLAY);
		DPRINTF(("ah_input: packet replay failure: %s\n",
			  ipsec_logsastr(sav)));
		m_freem(m);
		return ENOBUFS;
	}

	/* Verify AH header length. */
	hl = ah->ah_len * sizeof (u_int32_t);
	ahx = sav->tdb_authalgxform;
	authsize = AUTHSIZE(sav);
	if (hl != authsize + rplen - sizeof (struct ah)) {
		DPRINTF(("ah_input: bad authenticator length %u (expecting %lu)"
			" for packet in SA %s/%08lx\n",
			hl, (u_long) (authsize + rplen - sizeof (struct ah)),
			ipsec_address(&sav->sah->saidx.dst),
			(u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_BADAUTHL);
		m_freem(m);
		return EACCES;
	}
	if (skip + authsize + rplen > m->m_pkthdr.len) {
		char buf[IPSEC_ADDRSTRLEN];
		DPRINTF(("%s: bad mbuf length %u (expecting >= %lu)"
			" for packet in SA %s/%08lx\n", __func__,
			m->m_pkthdr.len, (u_long)(skip + authsize + rplen),
			ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
			(u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_BADAUTHL);
		m_freem(m);
		return EACCES;
	}

	AH_STATADD(AH_STAT_IBYTES, m->m_pkthdr.len - skip - hl);

	/* Get crypto descriptors. */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		DPRINTF(("ah_input: failed to acquire crypto descriptor\n"));
		AH_STATINC(AH_STAT_CRYPTO);
		m_freem(m);
		return ENOBUFS;
	}

	crda = crp->crp_desc;
	IPSEC_ASSERT(crda != NULL, ("ah_input: null crypto descriptor"));

	crda->crd_skip = 0;
	crda->crd_len = m->m_pkthdr.len;
	crda->crd_inject = skip + rplen;

	/* Authentication operation. */
	crda->crd_alg = ahx->type;
	crda->crd_key = _KEYBUF(sav->key_auth);
	crda->crd_klen = _KEYBITS(sav->key_auth);

	/* Find out if we've already done crypto. */
	for (mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_CRYPTO_DONE, NULL);
	     mtag != NULL;
	     mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_CRYPTO_DONE, mtag)) {
		tdbi = (struct tdb_ident *) (mtag + 1);
		if (tdbi->proto == sav->sah->saidx.proto &&
		    tdbi->spi == sav->spi &&
		    !memcmp(&tdbi->dst, &sav->sah->saidx.dst,
			  sizeof (union sockaddr_union)))
			break;
	}

	/* Allocate IPsec-specific opaque crypto info. */
	if (mtag == NULL) {
		tc = (struct tdb_crypto *) malloc(sizeof (struct tdb_crypto) +
			skip + rplen + authsize, M_XDATA, M_NOWAIT|M_ZERO);
	} else {
		/* Hash verification has already been done successfully. */
		tc = (struct tdb_crypto *) malloc(sizeof (struct tdb_crypto),
						    M_XDATA, M_NOWAIT|M_ZERO);
	}
	if (tc == NULL) {
		DPRINTF(("ah_input: failed to allocate tdb_crypto\n"));
		AH_STATINC(AH_STAT_CRYPTO);
		crypto_freereq(crp);
		m_freem(m);
		return ENOBUFS;
	}

	error = m_makewritable(&m, 0, skip + rplen + authsize, M_NOWAIT);
	if (error) {
		m_freem(m);
		DPRINTF(("ah_input: failed to copyback_cow\n"));
		AH_STATINC(AH_STAT_HDROPS);
		free(tc, M_XDATA);
		crypto_freereq(crp);
		return error;
	}

	/* Only save information if crypto processing is needed. */
	if (mtag == NULL) {
		/*
		 * Save the authenticator, the skipped portion of the packet,
		 * and the AH header.
		 */
		m_copydata(m, 0, skip + rplen + authsize, (tc + 1));

		/* Zeroize the authenticator on the packet. */
		m_copyback(m, skip + rplen, authsize, ipseczeroes);

		/* "Massage" the packet headers for crypto processing. */
		error = ah_massage_headers(&m, sav->sah->saidx.dst.sa.sa_family,
		    skip, ahx->type, 0);
		if (error != 0) {
			/* NB: mbuf is free'd by ah_massage_headers */
			AH_STATINC(AH_STAT_HDROPS);
			free(tc, M_XDATA);
			crypto_freereq(crp);
			return error;
		}
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
	tc->tc_ptr = mtag; /* Save the mtag we've identified. */

	DPRINTF(("ah: hash over %d bytes, skip %d: "
		 "crda len %d skip %d inject %d\n",
		 crp->crp_ilen, tc->tc_skip,
		 crda->crd_len, crda->crd_skip, crda->crd_inject));

	if (mtag == NULL)
		return crypto_dispatch(crp);
	else
		return ah_input_cb(crp);
}

#ifdef INET6
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff, mtag) do {		     \
	if (saidx->dst.sa.sa_family == AF_INET6) {			     \
		error = ipsec6_common_input_cb(m, sav, skip, protoff, mtag); \
	} else {							     \
		error = ipsec4_common_input_cb(m, sav, skip, protoff, mtag); \
	}								     \
} while (0)
#else
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff, mtag)		     \
	(error = ipsec4_common_input_cb(m, sav, skip, protoff, mtag))
#endif

/*
 * AH input callback from the crypto driver.
 */
static int
ah_input_cb(struct cryptop *crp)
{
	int rplen, error, skip, protoff;
	unsigned char calc[AH_ALEN_MAX];
	struct mbuf *m;
	struct tdb_crypto *tc;
	struct m_tag *mtag;
	struct secasvar *sav;
	struct secasindex *saidx;
	u_int8_t nxt;
	char *ptr;
	int s, authsize;
	u_int16_t dport;
	u_int16_t sport;

	tc = (struct tdb_crypto *) crp->crp_opaque;
	IPSEC_ASSERT(tc != NULL, ("ah_input_cb: null opaque crypto data area!"));
	skip = tc->tc_skip;
	nxt = tc->tc_nxt;
	protoff = tc->tc_protoff;
	mtag = (struct m_tag *) tc->tc_ptr;
	m = (struct mbuf *) crp->crp_buf;


	/* find the source port for NAT-T */
	nat_t_ports_get(m, &dport, &sport);

	s = splsoftnet();
	mutex_enter(softnet_lock);

	sav = KEY_ALLOCSA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, sport, dport);
	if (sav == NULL) {
		AH_STATINC(AH_STAT_NOTDB);
		DPRINTF(("ah_input_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	saidx = &sav->sah->saidx;
	IPSEC_ASSERT(saidx->dst.sa.sa_family == AF_INET ||
		saidx->dst.sa.sa_family == AF_INET6,
		("ah_input_cb: unexpected protocol family %u",
		 saidx->dst.sa.sa_family));

	/* Check for crypto errors. */
	if (crp->crp_etype) {
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			mutex_exit(softnet_lock);
			splx(s);
			return crypto_dispatch(crp);
		}

		AH_STATINC(AH_STAT_NOXFORM);
		DPRINTF(("ah_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	} else {
		AH_STATINC(AH_STAT_HIST + sav->alg_auth);
		crypto_freereq(crp);		/* No longer needed. */
		crp = NULL;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		AH_STATINC(AH_STAT_CRYPTO);
		DPRINTF(("ah_input_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}

	/* Figure out header size. */
	rplen = HDRSIZE(sav);
	authsize = AUTHSIZE(sav);

	if (ipsec_debug)
	  memset(calc, 0, sizeof(calc));

	/* Copy authenticator off the packet. */
	m_copydata(m, skip + rplen, authsize, calc);

	/*
	 * If we have an mtag, we don't need to verify the authenticator --
	 * it has been verified by an IPsec-aware NIC.
	 */
	if (mtag == NULL) {
		ptr = (char *) (tc + 1);

		/* Verify authenticator. */
		if (!consttime_memequal(ptr + skip + rplen, calc, authsize)) {
			u_int8_t *pppp = ptr + skip+rplen;
			DPRINTF(("ah_input: authentication hash mismatch " \
			    "over %d bytes " \
			    "for packet in SA %s/%08lx:\n" \
		    "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x, " \
		    "%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n",
			    authsize,
			    ipsec_address(&saidx->dst),
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
		((u_int8_t *) ptr)[protoff] = nxt;

		/* Copyback the saved (uncooked) network headers. */
		m_copyback(m, 0, skip, ptr);
	} else {
		/* Fix the Next Protocol field. */
		m_copyback(m, protoff, sizeof(u_int8_t), &nxt);
	}

	free(tc, M_XDATA), tc = NULL;			/* No longer needed */

	/*
	 * Header is now authenticated.
	 */
	m->m_flags |= M_AUTHIPHDR|M_AUTHIPDGM;

	/*
	 * Update replay sequence number, if appropriate.
	 */
	if (sav->replay) {
		u_int32_t seq;

		m_copydata(m, skip + offsetof(struct newah, ah_seq),
			   sizeof (seq), &seq);
		if (ipsec_updatereplay(ntohl(seq), sav)) {
			AH_STATINC(AH_STAT_REPLAY);
			error = ENOBUFS;			/*XXX as above*/
			goto bad;
		}
	}

	/*
	 * Remove the AH header and authenticator from the mbuf.
	 */
	error = m_striphdr(m, skip, rplen + authsize);
	if (error) {
		DPRINTF(("ah_input_cb: mangled mbuf chain for SA %s/%08lx\n",
		    ipsec_address(&saidx->dst), (u_long) ntohl(sav->spi)));

		AH_STATINC(AH_STAT_HDROPS);
		goto bad;
	}

	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff, mtag);

	KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	return error;
bad:
	if (sav)
		KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	if (m != NULL)
		m_freem(m);
	if (tc != NULL)
		free(tc, M_XDATA);
	if (crp != NULL)
		crypto_freereq(crp);
	return error;
}

/*
 * AH output routine, called by ipsec[46]_process_packet().
 */
static int
ah_output(
    struct mbuf *m,
    struct ipsecrequest *isr,
    struct mbuf **mp,
    int skip,
    int protoff
)
{
	const struct secasvar *sav;
	const struct auth_hash *ahx;
	struct cryptodesc *crda;
	struct tdb_crypto *tc;
	struct mbuf *mi;
	struct cryptop *crp;
	u_int16_t iplen;
	int error, rplen, authsize, maxpacketsize, roff;
	u_int8_t prot;
	struct newah *ah;

	IPSEC_SPLASSERT_SOFTNET("ah_output");

	sav = isr->sav;
	IPSEC_ASSERT(sav != NULL, ("ah_output: null SA"));
	ahx = sav->tdb_authalgxform;
	IPSEC_ASSERT(ahx != NULL, ("ah_output: null authentication xform"));

	AH_STATINC(AH_STAT_OUTPUT);

	/* Figure out header size. */
	rplen = HDRSIZE(sav);

	/* Check for maximum packet size violations. */
	switch (sav->sah->saidx.dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		maxpacketsize = IP_MAXPACKET;
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		maxpacketsize = IPV6_MAXPACKET;
		break;
#endif /* INET6 */
	default:
		DPRINTF(("ah_output: unknown/unsupported protocol "
		    "family %u, SA %s/%08lx\n",
		    sav->sah->saidx.dst.sa.sa_family,
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_NOPF);
		error = EPFNOSUPPORT;
		goto bad;
	}
	authsize = AUTHSIZE(sav);
	if (rplen + authsize + m->m_pkthdr.len > maxpacketsize) {
		DPRINTF(("ah_output: packet in SA %s/%08lx got too big "
		    "(len %u, max len %u)\n",
		    ipsec_address(&sav->sah->saidx.dst),
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
		DPRINTF(("ah_output: cannot clone mbuf chain, SA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}

	/* Inject AH header. */
	mi = m_makespace(m, skip, rplen + authsize, &roff);
	if (mi == NULL) {
		DPRINTF(("ah_output: failed to inject %u byte AH header for SA "
		    "%s/%08lx\n",
		    rplen + authsize,
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		AH_STATINC(AH_STAT_HDROPS);	/*XXX differs from openbsd */
		error = ENOBUFS;
		goto bad;
	}

	/*
	 * The AH header is guaranteed by m_makespace() to be in
	 * contiguous memory, at roff bytes offset into the returned mbuf.
	 */
	ah = (struct newah *)(mtod(mi, char *) + roff);

	/* Initialize the AH header. */
	m_copydata(m, protoff, sizeof(u_int8_t), &ah->ah_nxt);
	ah->ah_len = (rplen + authsize - sizeof(struct ah)) / sizeof(u_int32_t);
	ah->ah_reserve = 0;
	ah->ah_spi = sav->spi;

	/* Zeroize authenticator. */
	m_copyback(m, skip + rplen, authsize, ipseczeroes);

	/* Insert packet replay counter, as requested.  */
	if (sav->replay) {
		if (sav->replay->count == ~0 &&
		    (sav->flags & SADB_X_EXT_CYCSEQ) == 0) {
			DPRINTF(("ah_output: replay counter wrapped for SA "
				"%s/%08lx\n",
				ipsec_address(&sav->sah->saidx.dst),
				(u_long) ntohl(sav->spi)));
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
		DPRINTF(("ah_output: failed to acquire crypto descriptors\n"));
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
	tc = (struct tdb_crypto *) malloc(
		sizeof(struct tdb_crypto) + skip, M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		crypto_freereq(crp);
		DPRINTF(("ah_output: failed to allocate tdb_crypto\n"));
		AH_STATINC(AH_STAT_CRYPTO);
		error = ENOBUFS;
		goto bad;
	}

	/* Save the skipped portion of the packet. */
	m_copydata(m, 0, skip, (tc + 1));

	/*
	 * Fix IP header length on the header used for
	 * authentication. We don't need to fix the original
	 * header length as it will be fixed by our caller.
	 */
	switch (sav->sah->saidx.dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		bcopy(((char *)(tc + 1)) +
		    offsetof(struct ip, ip_len),
		    &iplen, sizeof(u_int16_t));
		iplen = htons(ntohs(iplen) + rplen + authsize);
		m_copyback(m, offsetof(struct ip, ip_len),
		    sizeof(u_int16_t), &iplen);
		break;
#endif /* INET */

#ifdef INET6
	case AF_INET6:
		bcopy(((char *)(tc + 1)) +
		    offsetof(struct ip6_hdr, ip6_plen),
		    &iplen, sizeof(u_int16_t));
		iplen = htons(ntohs(iplen) + rplen + authsize);
		m_copyback(m, offsetof(struct ip6_hdr, ip6_plen),
		    sizeof(u_int16_t), &iplen);
		break;
#endif /* INET6 */
	}

	/* Fix the Next Header field in saved header. */
	((u_int8_t *) (tc + 1))[protoff] = IPPROTO_AH;

	/* Update the Next Protocol field in the IP header. */
	prot = IPPROTO_AH;
	m_copyback(m, protoff, sizeof(u_int8_t), &prot);

	/* "Massage" the packet headers for crypto processing. */
	error = ah_massage_headers(&m, sav->sah->saidx.dst.sa.sa_family,
			skip, ahx->type, 1);
	if (error != 0) {
		m = NULL;	/* mbuf was free'd by ah_massage_headers. */
		free(tc, M_XDATA);
		crypto_freereq(crp);
		goto bad;
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

	return crypto_dispatch(crp);
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
	struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m;
	void *ptr;
	int s, err;

	tc = (struct tdb_crypto *) crp->crp_opaque;
	IPSEC_ASSERT(tc != NULL, ("ah_output_cb: null opaque data area!"));
	skip = tc->tc_skip;
	ptr = (tc + 1);
	m = (struct mbuf *) crp->crp_buf;

	s = splsoftnet();
	mutex_enter(softnet_lock);

	isr = tc->tc_isr;
	sav = KEY_ALLOCSA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, 0, 0);
	if (sav == NULL) {
		AH_STATINC(AH_STAT_NOTDB);
		DPRINTF(("ah_output_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}
	IPSEC_ASSERT(isr->sav == sav, ("ah_output_cb: SA changed\n"));

	/* Check for crypto errors. */
	if (crp->crp_etype) {
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			KEY_FREESAV(&sav);
			mutex_exit(softnet_lock);
			splx(s);
			return crypto_dispatch(crp);
		}

		AH_STATINC(AH_STAT_NOXFORM);
		DPRINTF(("ah_output_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		AH_STATINC(AH_STAT_CRYPTO);
		DPRINTF(("ah_output_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}
	AH_STATINC(AH_STAT_HIST + sav->alg_auth);

	/*
	 * Copy original headers (with the new protocol number) back
	 * in place.
	 */
	m_copyback(m, 0, skip, ptr);

	/* No longer needed. */
	free(tc, M_XDATA);
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
	err = ipsec_process_done(m, isr);
	KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	return err;
bad:
	if (sav)
		KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	if (m)
		m_freem(m);
	free(tc, M_XDATA);
	crypto_freereq(crp);
	return error;
}

static struct xformsw ah_xformsw = {
	XF_AH,		XFT_AUTH,	"IPsec AH",
	ah_init,	ah_zeroize,	ah_input,	ah_output,
	NULL,
};

INITFN void
ah_attach(void)
{
	ahstat_percpu = percpu_alloc(sizeof(uint64_t) * AH_NSTATS);
	xform_register(&ah_xformsw);
}

#ifdef __FreeBSD__
SYSINIT(ah_xform_init, SI_SUB_PROTO_DOMAIN, SI_ORDER_MIDDLE, ah_attach, NULL);
#endif
