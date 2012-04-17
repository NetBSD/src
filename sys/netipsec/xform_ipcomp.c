/*	$NetBSD: xform_ipcomp.c,v 1.28.4.1 2012/04/17 00:08:46 yamt Exp $	*/
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
__KERNEL_RCSID(0, "$NetBSD: xform_ipcomp.c,v 1.28.4.1 2012/04/17 00:08:46 yamt Exp $");

/* IP payload compression protocol (IPComp), see RFC 2393 */
#include "opt_inet.h"
#ifdef __FreeBSD__
#include "opt_inet6.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/kernel.h>
#include <sys/protosw.h>
#include <sys/sysctl.h>
#include <sys/socketvar.h> /* for softnet_lock */

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

#include <netipsec/ipsec_osdep.h>

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

const struct comp_algo *
ipcomp_algorithm_lookup(int alg)
{
	if (alg >= IPCOMP_ALG_MAX)
		return NULL;
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
		DPRINTF(("ipcomp_init: unsupported compression algorithm %d\n",
			 sav->alg_comp));
		return EINVAL;
	}
	sav->alg_comp = sav->alg_enc;		/* set for doing histogram */
	sav->tdb_xform = xsp;
	sav->tdb_compalgxform = tcomp;

	/* Initialize crypto session */
	memset(&cric, 0, sizeof (cric));
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
ipcomp_input(struct mbuf *m, const struct secasvar *sav, int skip, int protoff)
{
	struct tdb_crypto *tc;
	struct cryptodesc *crdc;
	struct cryptop *crp;
	int error, hlen = IPCOMP_HLENGTH;

	IPSEC_SPLASSERT_SOFTNET("ipcomp_input");

	/* Get crypto descriptors */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		m_freem(m);
		DPRINTF(("ipcomp_input: no crypto descriptors\n"));
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		return ENOBUFS;
	}
	/* Get IPsec-specific opaque pointer */
	tc = (struct tdb_crypto *) malloc(sizeof (*tc), M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		m_freem(m);
		crypto_freereq(crp);
		DPRINTF(("ipcomp_input: cannot allocate tdb_crypto\n"));
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		return ENOBUFS;
	}

	error = m_makewritable(&m, 0, m->m_pkthdr.len, M_NOWAIT);
	if (error) {
		DPRINTF(("ipcomp_input: m_makewritable failed\n"));
		m_freem(m);
		free(tc, M_XDATA);
		crypto_freereq(crp);
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		return error;
	}

	crdc = crp->crp_desc;

	crdc->crd_skip = skip + hlen;
	crdc->crd_len = m->m_pkthdr.len - (skip + hlen);
	crdc->crd_inject = 0; /* unused */

	tc->tc_ptr = 0;

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

	return crypto_dispatch(crp);
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
 * IPComp input callback from the crypto driver.
 */
static int
ipcomp_input_cb(struct cryptop *crp)
{
	struct cryptodesc *crd;
	struct tdb_crypto *tc;
	int skip, protoff;
	struct mtag *mtag;
	struct mbuf *m;
	struct secasvar *sav;
	struct secasindex *saidx;
	int s, hlen = IPCOMP_HLENGTH, error, clen;
	u_int8_t nproto;
	void *addr;
	u_int16_t dport = 0;
	u_int16_t sport = 0;
#ifdef IPSEC_NAT_T
	struct m_tag * tag = NULL;
#endif

	crd = crp->crp_desc;

	tc = (struct tdb_crypto *) crp->crp_opaque;
	IPSEC_ASSERT(tc != NULL, ("ipcomp_input_cb: null opaque crypto data area!"));
	skip = tc->tc_skip;
	protoff = tc->tc_protoff;
	mtag = (struct mtag *) tc->tc_ptr;
	m = (struct mbuf *) crp->crp_buf;

#ifdef IPSEC_NAT_T
	/* find the source port for NAT-T */
	if ((tag = m_tag_find(m, PACKET_TAG_IPSEC_NAT_T_PORTS, NULL))) {
		sport = ((u_int16_t *)(tag + 1))[0];
		dport = ((u_int16_t *)(tag + 1))[1];
	}
#endif

	s = splsoftnet();
	mutex_enter(softnet_lock);

	sav = KEY_ALLOCSA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, sport, dport);
	if (sav == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
		DPRINTF(("ipcomp_input_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	saidx = &sav->sah->saidx;
	IPSEC_ASSERT(saidx->dst.sa.sa_family == AF_INET ||
		saidx->dst.sa.sa_family == AF_INET6,
		("ipcomp_input_cb: unexpected protocol family %u",
		 saidx->dst.sa.sa_family));

	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset the session ID */
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			KEY_FREESAV(&sav);
			mutex_exit(softnet_lock);
			splx(s);
			return crypto_dispatch(crp);
		}

		IPCOMP_STATINC(IPCOMP_STAT_NOXFORM);
		DPRINTF(("ipcomp_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}
	/* Shouldn't happen... */
	if (m == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		DPRINTF(("ipcomp_input_cb: null mbuf returned from crypto\n"));
		error = EINVAL;
		goto bad;
	}
	IPCOMP_STATINC(IPCOMP_STAT_HIST + sav->alg_comp);

	/* Update the counters */
	IPCOMP_STATADD(IPCOMP_STAT_IBYTES, m->m_pkthdr.len - skip - hlen);


	clen = crp->crp_olen;		/* Length of data after processing */

	/* Release the crypto descriptors */
	free(tc, M_XDATA), tc = NULL;
	crypto_freereq(crp), crp = NULL;

	/* In case it's not done already, adjust the size of the mbuf chain */
	m->m_pkthdr.len = clen + hlen + skip;

	if (m->m_len < skip + hlen && (m = m_pullup(m, skip + hlen)) == 0) {
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);	/*XXX*/
		DPRINTF(("ipcomp_input_cb: m_pullup failed\n"));
		error = EINVAL;				/*XXX*/
		goto bad;
	}

	/* Keep the next protocol field */
	addr = (uint8_t*) mtod(m, struct ip *) + skip;
	nproto = ((struct ipcomp *) addr)->comp_nxt;
	if (nproto == IPPROTO_IPCOMP || nproto == IPPROTO_AH || nproto == IPPROTO_ESP) {
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);
		DPRINTF(("ipcomp_input_cb: nested ipcomp, IPCA %s/%08lx\n",
			 ipsec_address(&sav->sah->saidx.dst),
			 (u_long) ntohl(sav->spi)));
		error = EINVAL;
		goto bad;
	}

	/* Remove the IPCOMP header */
	error = m_striphdr(m, skip, hlen);
	if (error) {
		IPCOMP_STATINC(IPCOMP_STAT_HDROPS);
		DPRINTF(("ipcomp_input_cb: bad mbuf chain, IPCA %s/%08lx\n",
			 ipsec_address(&sav->sah->saidx.dst),
			 (u_long) ntohl(sav->spi)));
		goto bad;
	}

	/* Restore the Next Protocol field */
	m_copyback(m, protoff, sizeof (u_int8_t), (u_int8_t *) &nproto);

	IPSEC_COMMON_INPUT_CB(m, sav, skip, protoff, NULL);

	KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	return error;
bad:
	if (sav)
		KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	if (m)
		m_freem(m);
	if (tc != NULL)
		free(tc, M_XDATA);
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
    struct ipsecrequest *isr,
    struct mbuf **mp,
    int skip,
    int protoff
)
{
	const struct secasvar *sav;
	const struct comp_algo *ipcompx;
	int error, ralen, hlen, maxpacketsize;
	struct cryptodesc *crdc;
	struct cryptop *crp;
	struct tdb_crypto *tc;

	IPSEC_SPLASSERT_SOFTNET("ipcomp_output");
	sav = isr->sav;
	IPSEC_ASSERT(sav != NULL, ("ipcomp_output: null SA"));
	ipcompx = sav->tdb_compalgxform;
	IPSEC_ASSERT(ipcompx != NULL, ("ipcomp_output: null compression xform"));

	ralen = m->m_pkthdr.len - skip;	/* Raw payload length before comp. */
    
    /* Don't process the packet if it is too short */
	if (ralen < ipcompx->minlen) {
		IPCOMP_STATINC(IPCOMP_STAT_MINLEN);
		return ipsec_process_done(m,isr);
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
		DPRINTF(("ipcomp_output: unknown/unsupported protocol family %d"
		    ", IPCA %s/%08lx\n",
		    sav->sah->saidx.dst.sa.sa_family,
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		error = EPFNOSUPPORT;
		goto bad;
	}
	if (skip + hlen + ralen > maxpacketsize) {
		IPCOMP_STATINC(IPCOMP_STAT_TOOBIG);
		DPRINTF(("ipcomp_output: packet in IPCA %s/%08lx got too big "
		    "(len %u, max len %u)\n",
		    ipsec_address(&sav->sah->saidx.dst),
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
		DPRINTF(("ipcomp_output: cannot clone mbuf chain, IPCA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		error = ENOBUFS;
		goto bad;
	}

	/* Ok now, we can pass to the crypto processing */

	/* Get crypto descriptors */
	crp = crypto_getreq(1);
	if (crp == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		DPRINTF(("ipcomp_output: failed to acquire crypto descriptor\n"));
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
	tc = (struct tdb_crypto *) malloc(sizeof(struct tdb_crypto),
		M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		DPRINTF(("ipcomp_output: failed to allocate tdb_crypto\n"));
		crypto_freereq(crp);
		error = ENOBUFS;
		goto bad;
	}

	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_skip = skip;
	tc->tc_protoff = protoff;

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
	struct tdb_crypto *tc;
	struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m, *mo;
	int s, error, skip, rlen, roff;
	u_int8_t prot;
	u_int16_t cpi;
	struct ipcomp * ipcomp;


	tc = (struct tdb_crypto *) crp->crp_opaque;
	IPSEC_ASSERT(tc != NULL, ("ipcomp_output_cb: null opaque data area!"));
	m = (struct mbuf *) crp->crp_buf;
	skip = tc->tc_skip;
	rlen = crp->crp_ilen - skip;

	s = splsoftnet();
	mutex_enter(softnet_lock);

	isr = tc->tc_isr;
	sav = KEY_ALLOCSA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, 0, 0);
	if (sav == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_NOTDB);
		DPRINTF(("ipcomp_output_cb: SA expired while in crypto\n"));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}
	IPSEC_ASSERT(isr->sav == sav, ("ipcomp_output_cb: SA changed\n"));

	/* Check for crypto errors */
	if (crp->crp_etype) {
		/* Reset session ID */
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			KEY_FREESAV(&sav);
			mutex_exit(softnet_lock);
			splx(s);
			return crypto_dispatch(crp);
		}
		IPCOMP_STATINC(IPCOMP_STAT_NOXFORM);
		DPRINTF(("ipcomp_output_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}
	/* Shouldn't happen... */
	if (m == NULL) {
		IPCOMP_STATINC(IPCOMP_STAT_CRYPTO);
		DPRINTF(("ipcomp_output_cb: bogus return buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}
	IPCOMP_STATINC(IPCOMP_STAT_HIST + sav->alg_comp);

	if (rlen > crp->crp_olen) {
		/* Inject IPCOMP header */
		mo = m_makespace(m, skip, IPCOMP_HLENGTH, &roff);
		if (mo == NULL) {
			IPCOMP_STATINC(IPCOMP_STAT_WRAP);
			DPRINTF(("ipcomp_output: failed to inject IPCOMP header for "
					 "IPCA %s/%08lx\n",
						ipsec_address(&sav->sah->saidx.dst),
						(u_long) ntohl(sav->spi)));
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
		m_copyback(m, tc->tc_protoff, sizeof(u_int8_t), (u_char *)&prot);

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
			    ipsec_address(&sav->sah->saidx.dst),
			    (u_long) ntohl(sav->spi)));
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
	free(tc, M_XDATA);
	crypto_freereq(crp);

	/* NB: m is reclaimed by ipsec_process_done. */
	error = ipsec_process_done(m, isr);
	KEY_FREESAV(&sav);
	mutex_exit(softnet_lock);
	splx(s);
	return error;
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

static struct xformsw ipcomp_xformsw = {
	XF_IPCOMP,		XFT_COMP,		"IPcomp",
	ipcomp_init,		ipcomp_zeroize,		ipcomp_input,
	ipcomp_output,
	NULL,
};

INITFN void
ipcomp_attach(void)
{
	ipcompstat_percpu = percpu_alloc(sizeof(uint64_t) * IPCOMP_NSTATS);
	xform_register(&ipcomp_xformsw);
}

#ifdef __FreeBSD__
SYSINIT(ipcomp_xform_init, SI_SUB_DRIVERS, SI_ORDER_FIRST, ipcomp_attach, NULL)
#endif /* __FreeBSD__ */
