/*	$NetBSD: xform_esp.c,v 1.45.6.1 2015/04/06 15:18:23 skrll Exp $	*/
/*	$FreeBSD: src/sys/netipsec/xform_esp.c,v 1.2.2.1 2003/01/24 05:11:36 sam Exp $	*/
/*	$OpenBSD: ip_esp.c,v 1.69 2001/06/26 06:18:59 angelos Exp $ */

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
 * Additional features in 1999 by Angelos D. Keromytis.
 *
 * Copyright (C) 1995, 1996, 1997, 1998, 1999 by John Ioannidis,
 * Angelos D. Keromytis and Niels Provos.
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
__KERNEL_RCSID(0, "$NetBSD: xform_esp.c,v 1.45.6.1 2015/04/06 15:18:23 skrll Exp $");

#include "opt_inet.h"
#ifdef __FreeBSD__
#include "opt_inet6.h"
#endif
#include "opt_ipsec.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
/*#include <sys/random.h>*/
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
#include <netipsec/esp.h>
#include <netipsec/esp_var.h>
#include <netipsec/xform.h>

#ifdef INET6
#include <netinet6/ip6_var.h>
#include <netipsec/ipsec6.h>
#  ifdef __FreeBSD__
#  include <netinet6/ip6_ecn.h>
#  endif
#endif

#include <netipsec/key.h>
#include <netipsec/key_debug.h>

#include <netipsec/ipsec_osdep.h>

#include <opencrypto/cryptodev.h>
#include <opencrypto/xform.h>

percpu_t *espstat_percpu;

int	esp_enable = 1;

#ifdef __FreeBSD__
SYSCTL_DECL(_net_inet_esp);
SYSCTL_INT(_net_inet_esp, OID_AUTO,
	esp_enable,	CTLFLAG_RW,	&esp_enable,	0, "");
SYSCTL_STRUCT(_net_inet_esp, IPSECCTL_STATS,
	stats,		CTLFLAG_RD,	&espstat,	espstat, "");
#endif /* __FreeBSD__ */

static	int esp_max_ivlen;		/* max iv length over all algorithms */

static int esp_input_cb(struct cryptop *op);
static int esp_output_cb(struct cryptop *crp);

/*
 * NB: this is public for use by the PF_KEY support.
 * NB: if you add support here; be sure to add code to esp_attach below!
 */
const struct enc_xform *
esp_algorithm_lookup(int alg)
{
	if (alg >= ESP_ALG_MAX)
		return NULL;
	switch (alg) {
	case SADB_EALG_DESCBC:
		return &enc_xform_des;
	case SADB_EALG_3DESCBC:
		return &enc_xform_3des;
	case SADB_X_EALG_AES:
		return &enc_xform_rijndael128;
	case SADB_X_EALG_BLOWFISHCBC:
		return &enc_xform_blf;
	case SADB_X_EALG_CAST128CBC:
		return &enc_xform_cast5;
	case SADB_X_EALG_SKIPJACK:
		return &enc_xform_skipjack;
	case SADB_X_EALG_CAMELLIACBC:
		return &enc_xform_camellia;
	case SADB_X_EALG_AESCTR:
		return &enc_xform_aes_ctr;
	case SADB_X_EALG_AESGCM16:
		return &enc_xform_aes_gcm;
	case SADB_X_EALG_AESGMAC:
		return &enc_xform_aes_gmac;
	case SADB_EALG_NULL:
		return &enc_xform_null;
	}
	return NULL;
}

size_t
esp_hdrsiz(const struct secasvar *sav)
{
	size_t size;

	if (sav != NULL) {
		/*XXX not right for null algorithm--does it matter??*/
		IPSEC_ASSERT(sav->tdb_encalgxform != NULL,
			("esp_hdrsiz: SA with null xform"));
		if (sav->flags & SADB_X_EXT_OLD)
			size = sizeof (struct esp);
		else
			size = sizeof (struct newesp);
		size += sav->tdb_encalgxform->ivsize + 9;
		/*XXX need alg check???*/
		if (sav->tdb_authalgxform != NULL && sav->replay)
			size += ah_hdrsiz(sav);
	} else {
		/*
		 *   base header size
		 * + max iv length for CBC mode
		 * + max pad length
		 * + sizeof (pad length field)
		 * + sizeof (next header field)
		 * + max icv supported.
		 */
		size = sizeof (struct newesp) + esp_max_ivlen + 9 + 16;
	}
	return size;
}

/*
 * esp_init() is called when an SPI is being set up.
 */
static int
esp_init(struct secasvar *sav, const struct xformsw *xsp)
{
	const struct enc_xform *txform;
	struct cryptoini cria, crie;
	int keylen;
	int error;

	txform = esp_algorithm_lookup(sav->alg_enc);
	if (txform == NULL) {
		DPRINTF(("esp_init: unsupported encryption algorithm %d\n",
			sav->alg_enc));
		return EINVAL;
	}
	if (sav->key_enc == NULL) {
		DPRINTF(("esp_init: no encoding key for %s algorithm\n",
			 txform->name));
		return EINVAL;
	}
	if ((sav->flags&(SADB_X_EXT_OLD|SADB_X_EXT_IV4B)) == SADB_X_EXT_IV4B) {
		DPRINTF(("esp_init: 4-byte IV not supported with protocol\n"));
		return EINVAL;
	}
	keylen = _KEYLEN(sav->key_enc);
	if (txform->minkey > keylen || keylen > txform->maxkey) {
		DPRINTF(("esp_init: invalid key length %u, must be in "
			"the range [%u..%u] for algorithm %s\n",
			keylen, txform->minkey, txform->maxkey,
			txform->name));
		return EINVAL;
	}

	sav->ivlen = txform->ivsize;

	/*
	 * Setup AH-related state.
	 */
	if (sav->alg_auth != 0) {
		error = ah_init0(sav, xsp, &cria);
		if (error)
			return error;
	}

	/* NB: override anything set in ah_init0 */
	sav->tdb_xform = xsp;
	sav->tdb_encalgxform = txform;

	if (sav->alg_enc == SADB_X_EALG_AESGCM16 ||
	    sav->alg_enc == SADB_X_EALG_AESGMAC) {
		switch (keylen) {
		case 20:
			sav->alg_auth = SADB_X_AALG_AES128GMAC;
			sav->tdb_authalgxform = &auth_hash_gmac_aes_128;
			break;
		case 28:
			sav->alg_auth = SADB_X_AALG_AES192GMAC;
			sav->tdb_authalgxform = &auth_hash_gmac_aes_192;
			break;
		case 36:
			sav->alg_auth = SADB_X_AALG_AES256GMAC;
			sav->tdb_authalgxform = &auth_hash_gmac_aes_256;
			break;
		}
		memset(&cria, 0, sizeof(cria));
		cria.cri_alg = sav->tdb_authalgxform->type;
		cria.cri_klen = _KEYBITS(sav->key_enc);
		cria.cri_key = _KEYBUF(sav->key_enc);
	}

	/* Initialize crypto session. */
	memset(&crie, 0, sizeof (crie));
	crie.cri_alg = sav->tdb_encalgxform->type;
	crie.cri_klen = _KEYBITS(sav->key_enc);
	crie.cri_key = _KEYBUF(sav->key_enc);
	/* XXX Rounds ? */

	if (sav->tdb_authalgxform && sav->tdb_encalgxform) {
		/* init both auth & enc */
		crie.cri_next = &cria;
		error = crypto_newsession(&sav->tdb_cryptoid,
					  &crie, crypto_support);
	} else if (sav->tdb_encalgxform) {
		error = crypto_newsession(&sav->tdb_cryptoid,
					  &crie, crypto_support);
	} else if (sav->tdb_authalgxform) {
		error = crypto_newsession(&sav->tdb_cryptoid,
					  &cria, crypto_support);
	} else {
		/* XXX cannot happen? */
		DPRINTF(("esp_init: no encoding OR authentication xform!\n"));
		error = EINVAL;
	}
	return error;
}

/*
 * Paranoia.
 */
static int
esp_zeroize(struct secasvar *sav)
{
	/* NB: ah_zerorize free's the crypto session state */
	int error = ah_zeroize(sav);

	if (sav->key_enc)
		memset(_KEYBUF(sav->key_enc), 0, _KEYLEN(sav->key_enc));
	sav->tdb_encalgxform = NULL;
	sav->tdb_xform = NULL;
	return error;
}

/*
 * ESP input processing, called (eventually) through the protocol switch.
 */
static int
esp_input(struct mbuf *m, const struct secasvar *sav, int skip, int protoff)
{
	const struct auth_hash *esph;
	const struct enc_xform *espx;
	struct tdb_ident *tdbi;
	struct tdb_crypto *tc;
	int plen, alen, hlen, error;
	struct m_tag *mtag;
	struct newesp *esp;

	struct cryptodesc *crde;
	struct cryptop *crp;

	IPSEC_SPLASSERT_SOFTNET("esp_input");

	IPSEC_ASSERT(sav != NULL, ("esp_input: null SA"));
	IPSEC_ASSERT(sav->tdb_encalgxform != NULL,
		("esp_input: null encoding xform"));
	IPSEC_ASSERT((skip&3) == 0 && (m->m_pkthdr.len&3) == 0,
		("esp_input: misaligned packet, skip %u pkt len %u",
			skip, m->m_pkthdr.len));

	/* XXX don't pullup, just copy header */
	IP6_EXTHDR_GET(esp, struct newesp *, m, skip, sizeof (struct newesp));

	esph = sav->tdb_authalgxform;
	espx = sav->tdb_encalgxform;

	/* Determine the ESP header length */
	if (sav->flags & SADB_X_EXT_OLD)
		hlen = sizeof (struct esp) + sav->ivlen;
	else
		hlen = sizeof (struct newesp) + sav->ivlen;
	/* Authenticator hash size */
	alen = esph ? esph->authsize : 0;

	/*
	 * Verify payload length is multiple of encryption algorithm
	 * block size.
	 *
	 * NB: This works for the null algorithm because the blocksize
	 *     is 4 and all packets must be 4-byte aligned regardless
	 *     of the algorithm.
	 */
	plen = m->m_pkthdr.len - (skip + hlen + alen);
	if ((plen & (espx->blocksize - 1)) || (plen <= 0)) {
		DPRINTF(("esp_input: "
		    "payload of %d octets not a multiple of %d octets,"
		    "  SA %s/%08lx\n",
		    plen, espx->blocksize,
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		ESP_STATINC(ESP_STAT_BADILEN);
		m_freem(m);
		return EINVAL;
	}

	/*
	 * Check sequence number.
	 */
	if (esph && sav->replay && !ipsec_chkreplay(ntohl(esp->esp_seq), sav)) {
		DPRINTF(("esp_input: packet replay check for %s\n",
		    ipsec_logsastr(sav)));	/*XXX*/
		ESP_STATINC(ESP_STAT_REPLAY);
		m_freem(m);
		return ENOBUFS;		/*XXX*/
	}

	/* Update the counters */
	ESP_STATADD(ESP_STAT_IBYTES, m->m_pkthdr.len - skip - hlen - alen);

	/* Find out if we've already done crypto */
	for (mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_CRYPTO_DONE, NULL);
	     mtag != NULL;
	     mtag = m_tag_find(m, PACKET_TAG_IPSEC_IN_CRYPTO_DONE, mtag)) {
		tdbi = (struct tdb_ident *) (mtag + 1);
		if (tdbi->proto == sav->sah->saidx.proto &&
		    tdbi->spi == sav->spi &&
		    !memcmp(&tdbi->dst, &sav->sah->saidx.dst,
			  sizeof(union sockaddr_union)))
			break;
	}

	/* Get crypto descriptors */
	crp = crypto_getreq(esph && espx ? 2 : 1);
	if (crp == NULL) {
		DPRINTF(("esp_input: failed to acquire crypto descriptors\n"));
		ESP_STATINC(ESP_STAT_CRYPTO);
		m_freem(m);
		return ENOBUFS;
	}

	/* Get IPsec-specific opaque pointer */
	if (esph == NULL || mtag != NULL)
		tc = (struct tdb_crypto *) malloc(sizeof(struct tdb_crypto),
		    M_XDATA, M_NOWAIT|M_ZERO);
	else
		tc = (struct tdb_crypto *) malloc(sizeof(struct tdb_crypto) + alen,
		    M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		crypto_freereq(crp);
		DPRINTF(("esp_input: failed to allocate tdb_crypto\n"));
		ESP_STATINC(ESP_STAT_CRYPTO);
		m_freem(m);
		return ENOBUFS;
	}

	error = m_makewritable(&m, 0, m->m_pkthdr.len, M_NOWAIT);
	if (error) {
		m_freem(m);
		free(tc, M_XDATA);
		crypto_freereq(crp);
		DPRINTF(("esp_input: m_makewritable failed\n"));
		ESP_STATINC(ESP_STAT_CRYPTO);
		return error;
	}

	tc->tc_ptr = mtag;

	if (esph) {
		struct cryptodesc *crda = crp->crp_desc;

		IPSEC_ASSERT(crda != NULL, ("esp_input: null ah crypto descriptor"));

		/* Authentication descriptor */
		crda->crd_skip = skip;
		if (espx && espx->type == CRYPTO_AES_GCM_16)
			crda->crd_len = hlen - sav->ivlen;
		else
			crda->crd_len = m->m_pkthdr.len - (skip + alen);
		crda->crd_inject = m->m_pkthdr.len - alen;

		crda->crd_alg = esph->type;
		if (espx && (espx->type == CRYPTO_AES_GCM_16 ||
			     espx->type == CRYPTO_AES_GMAC)) {
			crda->crd_key = _KEYBUF(sav->key_enc);
			crda->crd_klen = _KEYBITS(sav->key_enc);
		} else {
			crda->crd_key = _KEYBUF(sav->key_auth);
			crda->crd_klen = _KEYBITS(sav->key_auth);
		}

		/* Copy the authenticator */
		if (mtag == NULL)
			m_copydata(m, m->m_pkthdr.len - alen, alen,
				      (tc + 1));

		/* Chain authentication request */
		crde = crda->crd_next;
	} else {
		crde = crp->crp_desc;
	}

	/* Crypto operation descriptor */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = m;
	crp->crp_callback = esp_input_cb;
	crp->crp_sid = sav->tdb_cryptoid;
	crp->crp_opaque = tc;

	/* These are passed as-is to the callback */
	tc->tc_spi = sav->spi;
	tc->tc_dst = sav->sah->saidx.dst;
	tc->tc_proto = sav->sah->saidx.proto;
	tc->tc_protoff = protoff;
	tc->tc_skip = skip;

	/* Decryption descriptor */
	if (espx) {
		IPSEC_ASSERT(crde != NULL, ("esp_input: null esp crypto descriptor"));
		crde->crd_skip = skip + hlen;
		if (espx->type == CRYPTO_AES_GMAC)
			crde->crd_len = 0;
		else
			crde->crd_len = m->m_pkthdr.len - (skip + hlen + alen);
		crde->crd_inject = skip + hlen - sav->ivlen;

		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
		/* XXX Rounds ? */
	}

	if (mtag == NULL)
		return crypto_dispatch(crp);
	else
		return esp_input_cb(crp);
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
 * ESP input callback from the crypto driver.
 */
static int
esp_input_cb(struct cryptop *crp)
{
	u_int8_t lastthree[3], aalg[AH_ALEN_MAX];
	int s, hlen, skip, protoff, error;
	struct mbuf *m;
	struct cryptodesc *crd __diagused;
	const struct auth_hash *esph;
	struct tdb_crypto *tc;
	struct m_tag *mtag;
	struct secasvar *sav;
	struct secasindex *saidx;
	void *ptr;
	u_int16_t dport;
	u_int16_t sport;

	crd = crp->crp_desc;
	IPSEC_ASSERT(crd != NULL, ("esp_input_cb: null crypto descriptor!"));

	tc = (struct tdb_crypto *) crp->crp_opaque;
	IPSEC_ASSERT(tc != NULL, ("esp_input_cb: null opaque crypto data area!"));
	skip = tc->tc_skip;
	protoff = tc->tc_protoff;
	mtag = (struct m_tag *) tc->tc_ptr;
	m = (struct mbuf *) crp->crp_buf;

	/* find the source port for NAT-T */
	nat_t_ports_get(m, &dport, &sport);

	s = splsoftnet();
	mutex_enter(softnet_lock);

	sav = KEY_ALLOCSA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, sport, dport);
	if (sav == NULL) {
		ESP_STATINC(ESP_STAT_NOTDB);
		DPRINTF(("esp_input_cb: SA expired while in crypto "
		    "(SA %s/%08lx proto %u)\n", ipsec_address(&tc->tc_dst),
		    (u_long) ntohl(tc->tc_spi), tc->tc_proto));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}

	saidx = &sav->sah->saidx;
	IPSEC_ASSERT(saidx->dst.sa.sa_family == AF_INET ||
		saidx->dst.sa.sa_family == AF_INET6,
		("esp_input_cb: unexpected protocol family %u",
		 saidx->dst.sa.sa_family));

	esph = sav->tdb_authalgxform;

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

		ESP_STATINC(ESP_STAT_NOXFORM);
		DPRINTF(("esp_input_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ESP_STATINC(ESP_STAT_CRYPTO);
		DPRINTF(("esp_input_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}
	ESP_STATINC(ESP_STAT_HIST + sav->alg_enc);

	/* If authentication was performed, check now. */
	if (esph != NULL) {
		/*
		 * If we have a tag, it means an IPsec-aware NIC did
		 * the verification for us.  Otherwise we need to
		 * check the authentication calculation.
		 */
		AH_STATINC(AH_STAT_HIST + sav->alg_auth);
		if (mtag == NULL) {
			/* Copy the authenticator from the packet */
			m_copydata(m, m->m_pkthdr.len - esph->authsize,
				esph->authsize, aalg);

			ptr = (tc + 1);

			/* Verify authenticator */
			if (!consttime_memequal(ptr, aalg, esph->authsize)) {
				DPRINTF(("esp_input_cb: "
		    "authentication hash mismatch for packet in SA %s/%08lx\n",
				    ipsec_address(&saidx->dst),
				    (u_long) ntohl(sav->spi)));
				ESP_STATINC(ESP_STAT_BADAUTH);
				error = EACCES;
				goto bad;
			}
		}

		/* Remove trailing authenticator */
		m_adj(m, -(esph->authsize));
	}

	/* Release the crypto descriptors */
	free(tc, M_XDATA), tc = NULL;
	crypto_freereq(crp), crp = NULL;

	/*
	 * Packet is now decrypted.
	 */
	m->m_flags |= M_DECRYPTED;

	/*
	 * Update replay sequence number, if appropriate.
	 */
	if (sav->replay) {
		u_int32_t seq;

		m_copydata(m, skip + offsetof(struct newesp, esp_seq),
		    sizeof (seq), &seq);
		if (ipsec_updatereplay(ntohl(seq), sav)) {
			DPRINTF(("%s: packet replay check for %s\n", __func__,
			    ipsec_logsastr(sav)));
			ESP_STATINC(ESP_STAT_REPLAY);
			error = ENOBUFS;
			goto bad;
		}
	}

	/* Determine the ESP header length */
	if (sav->flags & SADB_X_EXT_OLD)
		hlen = sizeof (struct esp) + sav->ivlen;
	else
		hlen = sizeof (struct newesp) + sav->ivlen;

	/* Remove the ESP header and IV from the mbuf. */
	error = m_striphdr(m, skip, hlen);
	if (error) {
		ESP_STATINC(ESP_STAT_HDROPS);
		DPRINTF(("esp_input_cb: bad mbuf chain, SA %s/%08lx\n",
		    ipsec_address(&sav->sah->saidx.dst),
		    (u_long) ntohl(sav->spi)));
		goto bad;
	}

	/* Save the last three bytes of decrypted data */
	m_copydata(m, m->m_pkthdr.len - 3, 3, lastthree);

	/* Verify pad length */
	if (lastthree[1] + 2 > m->m_pkthdr.len - skip) {
		ESP_STATINC(ESP_STAT_BADILEN);
		DPRINTF(("esp_input_cb: invalid padding length %d "
			 "for %u byte packet in SA %s/%08lx\n",
			 lastthree[1], m->m_pkthdr.len - skip,
			 ipsec_address(&sav->sah->saidx.dst),
			 (u_long) ntohl(sav->spi)));
		error = EINVAL;
		goto bad;
	}

	/* Verify correct decryption by checking the last padding bytes */
	if ((sav->flags & SADB_X_EXT_PMASK) != SADB_X_EXT_PRAND) {
		if (lastthree[1] != lastthree[0] && lastthree[1] != 0) {
			ESP_STATINC(ESP_STAT_BADENC);
			DPRINTF(("esp_input_cb: decryption failed "
				"for packet in SA %s/%08lx\n",
				ipsec_address(&sav->sah->saidx.dst),
				(u_long) ntohl(sav->spi)));
DPRINTF(("esp_input_cb: %x %x\n", lastthree[0], lastthree[1]));
			error = EINVAL;
			goto bad;
		}
	}

	/* Trim the mbuf chain to remove trailing authenticator and padding */
	m_adj(m, -(lastthree[1] + 2));

	/* Restore the Next Protocol field */
	m_copyback(m, protoff, sizeof (u_int8_t), lastthree + 2);

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
 * ESP output routine, called by ipsec[46]_process_packet().
 */
static int
esp_output(
    struct mbuf *m,
    struct ipsecrequest *isr,
    struct mbuf **mp,
    int skip,
    int protoff
)
{
	const struct enc_xform *espx;
	const struct auth_hash *esph;
	int hlen, rlen, padding, blks, alen, i, roff;
	struct mbuf *mo = NULL;
	struct tdb_crypto *tc;
	const struct secasvar *sav;
	struct secasindex *saidx;
	unsigned char *pad;
	u_int8_t prot;
	int error, maxpacketsize;

	struct cryptodesc *crde = NULL, *crda = NULL;
	struct cryptop *crp;

	IPSEC_SPLASSERT_SOFTNET("esp_output");

	sav = isr->sav;
	IPSEC_ASSERT(sav != NULL, ("esp_output: null SA"));
	esph = sav->tdb_authalgxform;
	espx = sav->tdb_encalgxform;
	IPSEC_ASSERT(espx != NULL, ("esp_output: null encoding xform"));

	if (sav->flags & SADB_X_EXT_OLD)
		hlen = sizeof (struct esp) + sav->ivlen;
	else
		hlen = sizeof (struct newesp) + sav->ivlen;

	rlen = m->m_pkthdr.len - skip;	/* Raw payload length. */
	/*
	 * NB: The null encoding transform has a blocksize of 4
	 *     so that headers are properly aligned.
	 */
	blks = espx->blocksize;		/* IV blocksize */

	/* XXX clamp padding length a la KAME??? */
	padding = ((blks - ((rlen + 2) % blks)) % blks) + 2;

	if (esph)
		alen = esph->authsize;
	else
		alen = 0;

	ESP_STATINC(ESP_STAT_OUTPUT);

	saidx = &sav->sah->saidx;
	/* Check for maximum packet size violations. */
	switch (saidx->dst.sa.sa_family) {
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
		DPRINTF(("esp_output: unknown/unsupported protocol "
		    "family %d, SA %s/%08lx\n",
		    saidx->dst.sa.sa_family, ipsec_address(&saidx->dst),
		    (u_long) ntohl(sav->spi)));
		ESP_STATINC(ESP_STAT_NOPF);
		error = EPFNOSUPPORT;
		goto bad;
	}
	if (skip + hlen + rlen + padding + alen > maxpacketsize) {
		DPRINTF(("esp_output: packet in SA %s/%08lx got too big "
		    "(len %u, max len %u)\n",
		    ipsec_address(&saidx->dst), (u_long) ntohl(sav->spi),
		    skip + hlen + rlen + padding + alen, maxpacketsize));
		ESP_STATINC(ESP_STAT_TOOBIG);
		error = EMSGSIZE;
		goto bad;
	}

	/* Update the counters. */
	ESP_STATADD(ESP_STAT_OBYTES, m->m_pkthdr.len - skip);

	m = m_clone(m);
	if (m == NULL) {
		DPRINTF(("esp_output: cannot clone mbuf chain, SA %s/%08lx\n",
		    ipsec_address(&saidx->dst), (u_long) ntohl(sav->spi)));
		ESP_STATINC(ESP_STAT_HDROPS);
		error = ENOBUFS;
		goto bad;
	}

	/* Inject ESP header. */
	mo = m_makespace(m, skip, hlen, &roff);
	if (mo == NULL) {
		DPRINTF(("esp_output: failed to inject %u byte ESP hdr for SA "
		    "%s/%08lx\n",
		    hlen, ipsec_address(&saidx->dst),
		    (u_long) ntohl(sav->spi)));
		ESP_STATINC(ESP_STAT_HDROPS);	/* XXX diffs from openbsd */
		error = ENOBUFS;
		goto bad;
	}

	/* Initialize ESP header. */
	memcpy(mtod(mo, char *) + roff, &sav->spi, sizeof(u_int32_t));
	if (sav->replay) {
		u_int32_t replay;

#ifdef IPSEC_DEBUG
		/* Emulate replay attack when ipsec_replay is TRUE. */
		if (!ipsec_replay)
#endif
			sav->replay->count++;

		replay = htonl(sav->replay->count);
		bcopy(&replay,
		    mtod(mo,char *) + roff + sizeof(u_int32_t),
		    sizeof(u_int32_t));
	}

	/*
	 * Add padding -- better to do it ourselves than use the crypto engine,
	 * although if/when we support compression, we'd have to do that.
	 */
	pad = (u_char *) m_pad(m, padding + alen);
	if (pad == NULL) {
		DPRINTF(("esp_output: m_pad failed for SA %s/%08lx\n",
		    ipsec_address(&saidx->dst), (u_long) ntohl(sav->spi)));
		m = NULL;		/* NB: free'd by m_pad */
		error = ENOBUFS;
		goto bad;
	}

	/*
	 * Add padding: random, zero, or self-describing.
	 * XXX catch unexpected setting
	 */
	switch (sav->flags & SADB_X_EXT_PMASK) {
	case SADB_X_EXT_PRAND:
		(void) read_random(pad, padding - 2);
		break;
	case SADB_X_EXT_PZERO:
		memset(pad, 0, padding - 2);
		break;
	case SADB_X_EXT_PSEQ:
		for (i = 0; i < padding - 2; i++)
			pad[i] = i+1;
		break;
	}

	/* Fix padding length and Next Protocol in padding itself. */
	pad[padding - 2] = padding - 2;
	m_copydata(m, protoff, sizeof(u_int8_t), pad + padding - 1);

	/* Fix Next Protocol in IPv4/IPv6 header. */
	prot = IPPROTO_ESP;
	m_copyback(m, protoff, sizeof(u_int8_t), (u_char *) &prot);

	/* Get crypto descriptors. */
	crp = crypto_getreq(esph && espx ? 2 : 1);
	if (crp == NULL) {
		DPRINTF(("esp_output: failed to acquire crypto descriptors\n"));
		ESP_STATINC(ESP_STAT_CRYPTO);
		error = ENOBUFS;
		goto bad;
	}

	if (espx) {
		crde = crp->crp_desc;
		crda = crde->crd_next;

		/* Encryption descriptor. */
		crde->crd_skip = skip + hlen;
		if (espx->type == CRYPTO_AES_GMAC)
			crde->crd_len = 0;
		else
			crde->crd_len = m->m_pkthdr.len - (skip + hlen + alen);
		crde->crd_flags = CRD_F_ENCRYPT;
		crde->crd_inject = skip + hlen - sav->ivlen;

		/* Encryption operation. */
		crde->crd_alg = espx->type;
		crde->crd_key = _KEYBUF(sav->key_enc);
		crde->crd_klen = _KEYBITS(sav->key_enc);
		/* XXX Rounds ? */
	} else
		crda = crp->crp_desc;

	/* IPsec-specific opaque crypto info. */
	tc = (struct tdb_crypto *) malloc(sizeof(struct tdb_crypto),
		M_XDATA, M_NOWAIT|M_ZERO);
	if (tc == NULL) {
		crypto_freereq(crp);
		DPRINTF(("esp_output: failed to allocate tdb_crypto\n"));
		ESP_STATINC(ESP_STAT_CRYPTO);
		error = ENOBUFS;
		goto bad;
	}

	/* Callback parameters */
	tc->tc_isr = isr;
	tc->tc_spi = sav->spi;
	tc->tc_dst = saidx->dst;
	tc->tc_proto = saidx->proto;

	/* Crypto operation descriptor. */
	crp->crp_ilen = m->m_pkthdr.len; /* Total input length. */
	crp->crp_flags = CRYPTO_F_IMBUF;
	crp->crp_buf = m;
	crp->crp_callback = esp_output_cb;
	crp->crp_opaque = tc;
	crp->crp_sid = sav->tdb_cryptoid;

	if (esph) {
		/* Authentication descriptor. */
		crda->crd_skip = skip;
		if (espx && espx->type == CRYPTO_AES_GCM_16)
			crda->crd_len = hlen - sav->ivlen;
		else
			crda->crd_len = m->m_pkthdr.len - (skip + alen);
		crda->crd_inject = m->m_pkthdr.len - alen;

		/* Authentication operation. */
		crda->crd_alg = esph->type;
		if (espx && (espx->type == CRYPTO_AES_GCM_16 ||
			     espx->type == CRYPTO_AES_GMAC)) {
			crda->crd_key = _KEYBUF(sav->key_enc);
			crda->crd_klen = _KEYBITS(sav->key_enc);
		} else {
			crda->crd_key = _KEYBUF(sav->key_auth);
			crda->crd_klen = _KEYBITS(sav->key_auth);
		}
	}

	return crypto_dispatch(crp);
bad:
	if (m)
		m_freem(m);
	return (error);
}

/*
 * ESP output callback from the crypto driver.
 */
static int
esp_output_cb(struct cryptop *crp)
{
	struct tdb_crypto *tc;
	struct ipsecrequest *isr;
	struct secasvar *sav;
	struct mbuf *m;
	int s, err, error;

	tc = (struct tdb_crypto *) crp->crp_opaque;
	IPSEC_ASSERT(tc != NULL, ("esp_output_cb: null opaque data area!"));
	m = (struct mbuf *) crp->crp_buf;

	s = splsoftnet();
	mutex_enter(softnet_lock);

	isr = tc->tc_isr;
	sav = KEY_ALLOCSA(&tc->tc_dst, tc->tc_proto, tc->tc_spi, 0, 0);
	if (sav == NULL) {
		ESP_STATINC(ESP_STAT_NOTDB);
		DPRINTF(("esp_output_cb: SA expired while in crypto "
		    "(SA %s/%08lx proto %u)\n", ipsec_address(&tc->tc_dst),
		    (u_long) ntohl(tc->tc_spi), tc->tc_proto));
		error = ENOBUFS;		/*XXX*/
		goto bad;
	}
	IPSEC_ASSERT(isr->sav == sav,
		("esp_output_cb: SA changed was %p now %p\n", isr->sav, sav));

	/* Check for crypto errors. */
	if (crp->crp_etype) {
		/* Reset session ID. */
		if (sav->tdb_cryptoid != 0)
			sav->tdb_cryptoid = crp->crp_sid;

		if (crp->crp_etype == EAGAIN) {
			KEY_FREESAV(&sav);
			mutex_exit(softnet_lock);
			splx(s);
			return crypto_dispatch(crp);
		}

		ESP_STATINC(ESP_STAT_NOXFORM);
		DPRINTF(("esp_output_cb: crypto error %d\n", crp->crp_etype));
		error = crp->crp_etype;
		goto bad;
	}

	/* Shouldn't happen... */
	if (m == NULL) {
		ESP_STATINC(ESP_STAT_CRYPTO);
		DPRINTF(("esp_output_cb: bogus returned buffer from crypto\n"));
		error = EINVAL;
		goto bad;
	}
	ESP_STATINC(ESP_STAT_HIST + sav->alg_enc);
	if (sav->tdb_authalgxform != NULL)
		AH_STATINC(AH_STAT_HIST + sav->alg_auth);

	/* Release crypto descriptors. */
	free(tc, M_XDATA);
	crypto_freereq(crp);

#ifdef IPSEC_DEBUG
	/* Emulate man-in-the-middle attack when ipsec_integrity is TRUE. */
	if (ipsec_integrity) {
		static unsigned char ipseczeroes[AH_ALEN_MAX];
		const struct auth_hash *esph;

		/*
		 * Corrupt HMAC if we want to test integrity verification of
		 * the other side.
		 */
		esph = sav->tdb_authalgxform;
		if (esph !=  NULL) {
			m_copyback(m, m->m_pkthdr.len - esph->authsize,
			    esph->authsize, ipseczeroes);
		}
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

static struct xformsw esp_xformsw = {
	XF_ESP,		XFT_CONF|XFT_AUTH,	"IPsec ESP",
	esp_init,	esp_zeroize,		esp_input,
	esp_output,
	NULL,
};

INITFN void
esp_attach(void)
{

	espstat_percpu = percpu_alloc(sizeof(uint64_t) * ESP_NSTATS);

#define	MAXIV(xform)					\
	if (xform.ivsize > esp_max_ivlen)		\
		esp_max_ivlen = xform.ivsize		\

	esp_max_ivlen = 0;
	MAXIV(enc_xform_des);		/* SADB_EALG_DESCBC */
	MAXIV(enc_xform_3des);		/* SADB_EALG_3DESCBC */
	MAXIV(enc_xform_rijndael128);	/* SADB_X_EALG_AES */
	MAXIV(enc_xform_blf);		/* SADB_X_EALG_BLOWFISHCBC */
	MAXIV(enc_xform_cast5);		/* SADB_X_EALG_CAST128CBC */
	MAXIV(enc_xform_skipjack);	/* SADB_X_EALG_SKIPJACK */
	MAXIV(enc_xform_camellia);	/* SADB_X_EALG_CAMELLIACBC */
	MAXIV(enc_xform_aes_ctr);	/* SADB_X_EALG_AESCTR */
	MAXIV(enc_xform_null);		/* SADB_EALG_NULL */

	xform_register(&esp_xformsw);
#undef MAXIV
}
#ifdef __FreeBSD__
SYSINIT(esp_xform_init, SI_SUB_DRIVERS, SI_ORDER_FIRST, esp_attach, NULL)
#else
#endif
