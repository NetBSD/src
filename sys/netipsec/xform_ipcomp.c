/*	$NetBSD: xform_ipcomp.c,v 1.54 2018/02/14 08:59:23 ozaki-r Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_ipcomp.c,v 1.1.4.1 2003/01/24 05:11:36 sam Exp $	*/
/* $OpenBSD: ip_ipcomp.c,v 1.1 2001/07/05 12:08:52 jjbg Exp $ */

/*
 * Copyright (c) 2001 Jean-Jacques Bernard-Gundol (jj@wabbitt.org)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: xform_ipcomp.c,v 1.54 2018/02/14 08:59:23 ozaki-r Exp $");

/* IP payload compression protocol (IPComp), see RFC 2393 */
#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/pool.h>
#include <sys/pserialize.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>

#include <net/route.h>
#include <netipsec/ipsec.h>
#include <netipsec/ipsec_private.h>
#include <netipsec/xform.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netipsec/ipsec6.h>
#endif

#include <netipsec/ipcomp.h>
#include <netipsec/ipcomp_var.h>

#include <netipsec/key.h>
#include <netipsec/key_debug.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/deflate.h>
#include <opencrypto/xform.h>

percpu_t *ipcompstat_percpu;

int	ipcomp_enable = 1;

#ifdef __FreeBSD__
SYSCTL_DECL(_net_inet_ipcomp);
SYSCTL_INT(_net_inet_ipcomp, OID_AUTO,
	ipcomp_enable,	CTLFLAG_RW,	&ipcomp_enable,	0, "");
SYSCTL_STRUCT(_net_inet_ipcomp, IPSECCTL_STATS,
	stats,		CTLFLAG_RD,	&ipcompstat,	ipcompstat, "");
#endif /* __FreeBSD__ */

static int ipcomp_input_cb(struct cryptop *crp);
static int ipcomp_output_cb(struct cryptop *crp);

const uint8_t ipcomp_stats[256] = { SADB_CALG_STATS_INIT };

static pool_cache_t ipcomp_tdb_crypto_pool_cache;

const struct comp_algo *
ipcomp_algorithm_lookup(int alg)
{
	switch (alg) {
	case SADB_X_CALG_DEFLATE:
		return &comp_algo_deflate_nogrow;
	}
	return NULL;
}

/*
 * ipcomp_init() is called when an CPI is being set up.
 */
static int
ipcomp_init(struct secasvar *sav, const struct xformsw *xsp)
{
	const struct comp_algo *tcomp;
	struct cryptoini cric;
	int ses;

	/* NB: algorithm really comes in alg_enc and not alg_comp! */
	tcomp = ipcomp_algorithm_lookup(sav->alg_enc);
	if (tcomp == NULL) {
		DPRINTF(("%s: unsupported compression algorithm %d\n",
		    __func__, sav->alg_comp));
		return EINVAL;
	}
	sav->alg_comp = sav->alg_enc;		/* set for doing histogram */
	sav->tdb_xform = xsp;
	sav->tdb_compalgxform = tcomp;

	/* Initialize crypto session */
	memset(&cric, 0, sizeof(cric));
	cric.cri_alg = sav->tdb_compalgxform->type;

	ses = crypto_newsession(&sav->tdb_cryptoid, &cric, crypto_support);
	return ses;
}

/*
 * ipcomp_zeroize() used when IPCA is deleted
 */
static int
ipcomp_zeroize(struct secasvar *sav)
{
	int err;

	err = crypto_freesession(sav->tdb_cryptoid);
	sav->tdb_cryptoid = 0;
	return err;
}

/*
 * ipcomp_input() gets called to uncompress an input packet
 */
static int
ipcomp_input(struct mbuf *m, struct secasvar *sav, int skip, int protoff)
{
	struct tdb_crypto *tc;
	struct cryptodesc *crdc;
	struct cryptop *crp;
	int error, hlen = IPCOMP_HLENGTH;

	IPSEC_SPLASSERT_SOFTNET(__func__);

	/* Get crypto descriptors */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		m_freem(m);
		DPRINTF(("%s: no crypto descriptors\n", __func__));
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		return ENOBUFS;
	}
	/* Get IPsec-specific opaque pointer */
	tc = pool_cache_get(ipcomp_tdb_crypto_pool_cache, PR_NOWAIT);
	if (tc == NULL) {
		m_freem(m);
		crypto_freereq(crp);
		DPRINTF(("%s: cannot allocate tdb_crypto\n", __func__));
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		return ENOBUFS;
	}

	error = m_makewritable(&m, 0, m->m_pkthdr.len, M_NOWAIT);
	if (error) {
		DPRINTF(("%s: m_makewritable failed\n", __func__));
		m_freem(m);
		pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
		crypto_freereq(crp);
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		return error;
	}

    {
	int s = pserialize_read_enter();

	/*
	 * Take another reference to the SA for opencrypto callback.
	 */
	if (__predict_false(sav->state == SADB_SASTATE_DEAD)) {
		pserialize_read_exit(s);
		m_freem(m);
		pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
		crypto_freereq(crp);
		IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
		return ENOENT;
	}
	KEY_SA_REF(sav);
	pserialize_read_exit(s);
    }

	crdc = crp->crp_desc;

	crdc->crd_skip = skip + hlen;
	crdc->crd_len = m->m_pkthdr.len - (skip + hlen);
	crdc->crd_inject = 0; /* unused */

	/* Decompression operation */
	crdc->crd_alg = sav->tdb_compalgxform->type;

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len - (skip + hlen);
	crp->crp_olen = MCLBYTES; /* hint to decompression code */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = m;
	crp->crp_callback = ipcomp_input_cb;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = tc;

	/* These are passed as-is to the callback */
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_protoff = protoff;
	tc->tc_skip = skip;
	tc->tc_sav = sav;

	return crypto_dispatch(crp);
}

#ifdef INET6
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff) do {		     \
	if (saidx->dst.sa.sa_family == AF_INET6) {			     \
		error = ipsec6_common_input_cb(m, sav, skip, protoff);	     \
	} else {							     \
		error = ipsec4_common_input_cb(m, sav, skip, protoff);       \
	}								     \
} while (0)
#else
#define	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff)			     \
	(error = ipsec4_common_input_cb(m, sav, skip, protoff))
#endif

/*
 * IPComp input callback from the crypto driver.
 */
static int
ipcomp_input_cb(struct cryptop *crp)
{
	char buf[IPSEC_ADDRSTRLEN];
	struct tdb_crypto *tc;
	int skip, protoff;
	struct mbuf *m;
	struct secasvar *sav;
	struct secasindex *saidx __diagused;
	int hlen = IPCOMP_HLENGTH, error, clen;
	uint8_t nproto;
	void *addr;
	uint16_t dport;
	uint16_t sport;
	IPSEC_DECLARE_LOCK_VARIABLE;

	KASSERT(crp->crp_opaque != NULL);
	tc = crp->crp_opaque;
	skip = tc->tc_skip;
	protoff = tc->tc_protoff;
	m = crp->crp_buf;

	/* find the source port for NAT-T */
	nat_t_ports_get(m, &dport, &sport);

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	sav = tc->tc_sav;
	if (__predict_false(!SADB_SASTATE_USABLE_P(sav))) {
		KEY_SA_UNREF(&sav);
		sav = KEY_LOOKUP_SA(&tc->tc_dst, tc->tc_proto, tc->tc_spi,
		    sport, dport);
		if (sav == NULL) {
			IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
			DPRINTF(("%s: SA expired while in crypto\n", __func__));
			error = ENOBUFS;		/*XXX*/
			goto bad;
		}
	}

	saidx = &sav->sah->saidx;
	KASSERTMSG(saidx->dst.sa.sa_family == AF_INET ||
	    saidx->dst.sa.sa_family == AF_INET6,
	    "unexpected protocol family %u", saidx->dst.sa.sa_family);

	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset the session ID */
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			KEY_SA_UNREF(&sav);
			IPSEC_RELEASE_GLOBAL_LOCKS();
			return crypto_dispatch(crp);
		}

		IPCOMP_STATINC(IPCOMP_STAT_NOXFORM);
		DPRINTF(("%s: crypto error %d\n", __func__, crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	IPCOMP_STATINC(IPCOMP_STAT_HIST + ipcomp_stats[sav->alg_comp]);

	/* Update the counters */
	IPCOMP_STATADD(IPCOMP_STAT_IBYTES, m->m_pkthdr.len - skip - hlen);


	clen = crp->crp_olen;		/* Length of data after processing */

	/* Release the crypto descriptors */
	pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
	tc = NULL;
	crypto_freereq(crp), crp = NULL;

	/* In case it's not done already, adjust the size of the mbuf chain */
	m->m_pkthdr.len = clen + hlen + skip;

	if (m->m_len < skip + hlen && (m = m_pullup(m, skip + hlen)) == 0) {
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);	/*XXX*/
		DPRINTF(("%s: m_pullup failed\n", __func__));
		error = EINVAL;				/*XXX*/
		goto bad;
	}

	/* Keep the next protocol field */
	addr = (uint8_t*) mtod(m, struct ip *) + skip;
	nproto = ((struct ipcomp *) addr)->comp_nxt;
	switch (nproto) {
	case IPPROTO_IPCOMP:
	case IPPROTO_AH:
	case IPPROTO_ESP:
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);
		DPRINTF(("%s: nested ipcomp, IPCA %s/%08lx\n", __func__,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
		error = EINVAL;
		goto bad;
	default:
		break;
	}

	/* Remove the IPCOMP header */
	error = m_striphdr(m, skip, hlen);
	if (error) {
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);
		DPRINTF(("%s: bad mbuf chain, IPCA %s/%08lx\n", __func__,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
		goto bad;
	}

	/* Restore the Next Protocol field */
	m_copyback(m, protoff, sizeof(uint8_t), (uint8_t *) &nproto);

	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff);

	KEY_SA_UNREF(&sav);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	return error;
bad:
	if (sav)
		KEY_SA_UNREF(&sav);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	if (m)
		m_freem(m);
	if (tc != NULL)
		pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
	if (crp)
		crypto_freereq(crp);
	return error;
}

/*
 * IPComp output routine, called by ipsec[46]_process_packet()
 */
static int
ipcomp_output(
    struct mbuf *m,
    const struct ipsecrequest *isr,
    struct secasvar *sav,
    struct mbuf **mp,
    int skip,
    int protoff
)
{
	char buf[IPSEC_ADDRSTRLEN];
	const struct comp_algo *ipcompx;
	int error, ralen, hlen, maxpacketsize;
	struct cryptodesc *crdc;
	struct cryptop *crp;
	struct tdb_crypto *tc;

	IPSEC_SPLASSERT_SOFTNET(__func__);
	KASSERT(sav != NULL);
	KASSERT(sav->tdb_compalgxform != NULL);
	ipcompx = sav->tdb_compalgxform;

	ralen = m->m_pkthdr.len - skip;	/* Raw payload length before comp. */
    
    /* Don't process the packet if it is too short */
	if (ralen < ipcompx->minlen) {
		IPCOMP_STATINC(IPCOMP_STAT_MINLEN);
		return ipsec_process_done(m, isr, sav);
	}

	hlen = IPCOMP_HLENGTH;

	IPCOMP_STATINC(IPCOMP_STAT_OUTPUT);

	/* Check for maximum packet size violations. */
	switch (sav->sah->saidx.dst.sa.sa_family) {
#ifdef INET
	case AF_INET:
		maxpacketsize =  IP_MAXPACKET;
		break;
#endif /* INET */
#ifdef INET6
	case AF_INET6:
		maxpacketsize =  IPV6_MAXPACKET;
		break;
#endif /* INET6 */
	default:
		IPCOMP_STATINC(IPCOMP_STAT_NOPF);
		DPRINTF(("%s: unknown/unsupported protocol family %d"
		    ", IPCA %s/%08lx\n", __func__,
		    sav->sah->saidx.dst.sa.sa_family,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
		error = EPFNOSUPPORT;
		goto bad;
	}
	if (skip + hlen + ralen > maxpacketsize) {
		IPCOMP_STATINC(IPCOMP_STAT_TOOBIG);
		DPRINTF(("%s: packet in IPCA %s/%08lx got too big "
		    "(len %u, max len %u)\n", __func__,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi),
		    skip + hlen + ralen, maxpacketsize));
		error = EMSGSIZE;
		goto bad;
	}

	/* Update the counters */
	IPCOMP_STATADD(IPCOMP_STAT_OBYTES, m->m_pkthdr.len - skip);

	m = m_clone(m);
	if (m == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);
		DPRINTF(("%s: cannot clone mbuf chain, IPCA %s/%08lx\n",
		    __func__,
		    ipsec_address(&sav->sah->saidx.dst, buf, sizeof(buf)),
		    (u_long) ntohl(sav->spi)));
		error = ENOBUFS;
		goto bad;
	}

	/* Ok now, we can pass to the crypto processing */

	/* Get crypto descriptors */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		DPRINTF(("%s: failed to acquire crypto descriptor\n",
		    __func__));
		error = ENOBUFS;
		goto bad;
	}
	crdc = crp->crp_desc;

	/* Compression descriptor */
	crdc->crd_skip = skip;
	crdc->crd_len = m->m_pkthdr.len - skip;
	crdc->crd_flags = CRD_F_COMP;
	crdc->crd_inject = skip;

	/* Compression operation */
	crdc->crd_alg = ipcompx->type;

	/* IPsec-specific opaque crypto info */
	tc = pool_cache_get(ipcomp_tdb_crypto_pool_cache, PR_NOWAIT);
	if (tc == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		DPRINTF(("%s: failed to allocate tdb_crypto\n", __func__));
		crypto_freereq(crp);
		error = ENOBUFS;
		goto bad;
	}

    {
	int s = pserialize_read_enter();

	/*
	 * Take another reference to the SP and the SA for opencrypto callback.
	 */
	if (__predict_false(isr->sp->state == IPSEC_SPSTATE_DEAD ||
	    sav->state == SADB_SASTATE_DEAD)) {
		pserialize_read_exit(s);
		pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
		crypto_freereq(crp);
		IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
		error = ENOENT;
		goto bad;
	}
	KEY_SP_REF(isr->sp);
	KEY_SA_REF(sav);
	pserialize_read_exit(s);
    }

	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;
	tc->tc_sav = sav;

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len;	/* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = m;
	crp->crp_callback = ipcomp_output_cb;
	crp->crp_opaque = tc;
	crp->crp_sid = sav->tdb_cryptoid;

	return crypto_dispatch(crp);
bad:
	if (m)
		m_freem(m);
	return (error);
}

/*
 * IPComp output callback from the crypto driver.
 */
static int
ipcomp_output_cb(struct cryptop *crp)
{
	char buf[IPSEC_ADDRSTRLEN];
	struct tdb_crypto *tc;
	const struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m, *mo;
	int error, skip, rlen, roff;
	uint8_t prot;
	uint16_t cpi;
	struct ipcomp * ipcomp;
	IPSEC_DECLARE_LOCK_VARIABLE;

	KASSERT(crp->crp_opaque != NULL);
	tc = crp->crp_opaque;
	m = crp->crp_buf;
	skip = tc->tc_skip;
	rlen = crp->crp_ilen - skip;

	IPSEC_ACQUIRE_GLOBAL_LOCKS();

	isr = tc->tc_isr;
	sav = tc->tc_sav;
	if (__predict_false(isr->sp->state == IPSEC_SPSTATE_DEAD)) {
		IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
		IPSECLOG(LOG_DEBUG,
		    "SP is being destroyed while in crypto (id=%u)\n",
		    isr->sp->id);
		error = ENOENT;
		goto bad;
	}
	if (__predict_false(!SADB_SASTATE_USABLE_P(sav))) {
		KEY_SA_UNREF(&sav);
		sav = KEY_LOOKUP_SA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, 0, 0);
		if (sav == NULL) {
			IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
			DPRINTF(("%s: SA expired while in crypto\n", __func__));
			error = ENOBUFS;		/*XXX*/
			goto bad;
		}
	}

	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset session ID */
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			IPSEC_RELEASE_GLOBAL_LOCKS();
			return crypto_dispatch(crp);
		}
		IPCOMP_STATINC(IPCOMP_STAT_NOXFORM);
		DPRINTF(("%s: crypto error %d\n", __func__, crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	IPCOMP_STATINC(IPCOMP_STAT_HIST + ipcomp_stats[sav->alg_comp]);

	if (rlen > crp->crp_olen) {
		/* Inject IPCOMP header */
		mo = m_makespace(m, skip, IPCOMP_HLENGTH, &roff);
		if (mo == NULL) {
			IPCOMP_STATINC(IPCOMP_STAT_WRAP);
			DPRINTF(("%s: failed to inject IPCOMP header for "
			    "IPCA %s/%08lx\n", __func__,
			    ipsec_address(&sav->sah->saidx.dst, buf,
			    sizeof(buf)), (u_long) ntohl(sav->spi)));
			error = ENOBUFS;
			goto bad;
		}
		ipcomp = (struct ipcomp *)(mtod(mo, char *) + roff);

		/* Initialize the IPCOMP header */
		/* XXX alignment always correct? */
		switch (sav->sah->saidx.dst.sa.sa_family) {
#ifdef INET
		case AF_INET:
			ipcomp->comp_nxt = mtod(m, struct ip *)->ip_p;
			 break;
#endif /* INET */
#ifdef INET6
		case AF_INET6:
			ipcomp->comp_nxt = mtod(m, struct ip6_hdr *)->ip6_nxt;
		break;
#endif
		}
		ipcomp->comp_flags = 0;

		if ((sav->flags & SADB_X_EXT_RAWCPI) == 0)
			 cpi = sav->alg_enc;
		else
			cpi = ntohl(sav->spi) & 0xffff;
		ipcomp->comp_cpi = htons(cpi);

		/* Fix Next Protocol in IPv4/IPv6 header */
		prot = IPPROTO_IPCOMP;
		m_copyback(m, tc->tc_protoff, sizeof(uint8_t), (u_char *)&prot);

		/* Adjust the length in the IP header */
		switch (sav->sah->saidx.dst.sa.sa_family) {
#ifdef INET
		case AF_INET:
			mtod(m, struct ip *)->ip_len = htons(m->m_pkthdr.len);
			break;
#endif /* INET */
#ifdef INET6
		case AF_INET6:
			mtod(m, struct ip6_hdr *)->ip6_plen =
				htons(m->m_pkthdr.len) - sizeof(struct ip6_hdr);
			break;
#endif /* INET6 */
		default:
			IPCOMP_STATINC(IPCOMP_STAT_NOPF);
			DPRINTF(("ipcomp_output: unknown/unsupported protocol "
			    "family %d, IPCA %s/%08lx\n",
			    sav->sah->saidx.dst.sa.sa_family,
			    ipsec_address(&sav->sah->saidx.dst, buf,
			    sizeof(buf)), (u_long) ntohl(sav->spi)));
			error = EPFNOSUPPORT;
			goto bad;
		}
	} else {
		/* compression was useless, we have lost time */
		IPCOMP_STATINC(IPCOMP_STAT_USELESS);
		DPRINTF(("ipcomp_output_cb: compression was useless : initial size was %d"
				   	"and compressed size is %d\n", rlen, crp->crp_olen));
	}


	/* Release the crypto descriptor */
	pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
	crypto_freereq(crp);

	/* NB: m is reclaimed by ipsec_process_done. */
	error = ipsec_process_done(m, isr, sav);
	KEY_SA_UNREF(&sav);
	KEY_SP_UNREF(&isr->sp);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	return error;
bad:
	if (sav)
		KEY_SA_UNREF(&sav);
	KEY_SP_UNREF(&isr->sp);
	IPSEC_RELEASE_GLOBAL_LOCKS();
	if (m)
		m_freem(m);
	pool_cache_put(ipcomp_tdb_crypto_pool_cache, tc);
	crypto_freereq(crp);
	return error;
}

static struct xformsw ipcomp_xformsw = {
	.xf_type	= XF_IPCOMP,
	.xf_flags	= XFT_COMP,
	.xf_name	= "IPcomp",
	.xf_init	= ipcomp_init,
	.xf_zeroize	= ipcomp_zeroize,
	.xf_input	= ipcomp_input,
	.xf_output	= ipcomp_output,
	.xf_next	= NULL,
};

void
ipcomp_attach(void)
{
	ipcompstat_percpu = percpu_alloc(sizeof(uint64_t) * IPCOMP_NSTATS);
	ipcomp_tdb_crypto_pool_cache = pool_cache_init(sizeof(struct tdb_crypto),
	    coherency_unit, 0, 0, "ipcomp_tdb_crypto", NULL, IPL_SOFTNET,
	    NULL, NULL, NULL);
	xform_register(&ipcomp_xformsw);
}
