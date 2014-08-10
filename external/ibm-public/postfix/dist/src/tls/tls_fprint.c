/*	$NetBSD: tls_fprint.c,v 1.1.1.1.2.2 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	tls_fprint 3
/* SUMMARY
/*	Digests fingerprints and all that.
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	char	*tls_serverid_digest(props, protomask, ciphers)
/*	const TLS_CLIENT_START_PROPS *props;
/*	long	protomask;
/*	const char *ciphers;
/*
/*	char	*tls_digest_encode(md_buf, md_len)
/*	const unsigned char *md_buf;
/*	const char *md_len;
/*
/*	char	*tls_data_fprint(buf, len, mdalg)
/*	const char *buf;
/*	int	len;
/*	const char *mdalg;
/*
/*	char	*tls_cert_fprint(peercert, mdalg)
/*	X509	*peercert;
/*	const char *mdalg;
/*
/*	char	*tls_pkey_fprint(peercert, mdalg)
/*	X509	*peercert;
/*	const char *mdalg;
/* DESCRIPTION
/*	tls_digest_encode() converts a binary message digest to a hex ASCII
/*	format with ':' separators between each pair of hex digits.
/*	The return value is dynamically allocated with mymalloc(),
/*	and the caller must eventually free it with myfree().
/*
/*	tls_data_fprint() digests unstructured data, and encodes the digested
/*	result via tls_digest_encode().  The return value is dynamically
/*	allocated with mymalloc(), and the caller must eventually free it
/*	with myfree().
/*
/*	tls_cert_fprint() returns a fingerprint of the the given
/*	certificate using the requested message digest, formatted
/*	with tls_digest_encode(). Panics if the
/*	(previously verified) digest algorithm is not found. The return
/*	value is dynamically allocated with mymalloc(), and the caller
/*	must eventually free it with myfree().
/*
/*	tls_pkey_fprint() returns a public-key fingerprint; in all
/*	other respects the function behaves as tls_cert_fprint().
/*	The var_tls_bc_pkey_fprint variable enables an incorrect
/*	algorithm that was used in Postfix versions 2.9.[0-5].
/*	The return value is dynamically allocated with mymalloc(),
/*	and the caller must eventually free it with myfree().
/*
/*	tls_serverid_digest() suffixes props->serverid computed by the SMTP
/*	client with "&" plus a digest of additional parameters
/*	needed to ensure that re-used sessions are more likely to
/*	be reused and that they will satisfy all protocol and
/*	security requirements.
/*	The return value is dynamically allocated with mymalloc(),
/*	and the caller must eventually free it with myfree().
/*
/*	Arguments:
/* .IP peercert
/*	Server or client X.509 certificate.
/* .IP md_buf
/*	The raw binary digest.
/* .IP md_len
/*	The digest length in bytes.
/* .IP mdalg
/*	Name of a message digest algorithm suitable for computing secure
/*	(1st pre-image resistant) message digests of certificates. For now,
/*	md5, sha1, or member of SHA-2 family if supported by OpenSSL.
/* .IP buf
/*	Input data for the message digest algorithm mdalg.
/* .IP len
/*	The length of the input data.
/* .IP props
/*	The client start properties for the session, which contains the
/*	initial serverid from the SMTP client and the DANE verification
/*	parameters.
/* .IP protomask
/*	The mask of protocol exclusions.
/* .IP ciphers
/*	The SSL client cipherlist.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Viktor Dukhovni
/*--*/

/* System library. */

#include <sys_defs.h>
#include <ctype.h>

#ifdef USE_TLS
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

static const char hexcodes[] = "0123456789ABCDEF";

#define checkok(ret)	(ok &= ((ret) ? 1 : 0))
#define digest_data(p, l) checkok(EVP_DigestUpdate(mdctx, (char *)(p), (l)))
#define digest_object(p) digest_data((p), sizeof(*(p)))
#define digest_string(s) digest_data((s), strlen(s)+1)

#define digest_dane(dane, memb) do { \
	if ((dane)->memb != 0) \
	    checkok(digest_tlsa_usage(mdctx, (dane)->memb, #memb)); \
    } while (0)

#define digest_tlsa_argv(tlsa, memb) do { \
	if ((tlsa)->memb) { \
	    digest_string(#memb); \
	    for (dgst = (tlsa)->memb->argv; *dgst; ++dgst) \
		digest_string(*dgst); \
	} \
    } while (0)

/* digest_tlsa_usage - digest TA or EE match list sorted by alg and value */

static int digest_tlsa_usage(EVP_MD_CTX * mdctx, TLS_TLSA *tlsa,
			             const char *usage)
{
    char  **dgst;
    int     ok = 1;

    for (digest_string(usage); tlsa; tlsa = tlsa->next) {
	digest_string(tlsa->mdalg);
	digest_tlsa_argv(tlsa, pkeys);
	digest_tlsa_argv(tlsa, certs);
    }
    return (ok);
}

/* tls_serverid_digest - suffix props->serverid with parameter digest */

char   *tls_serverid_digest(const TLS_CLIENT_START_PROPS *props, long protomask,
			            const char *ciphers)
{
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    const char *mdalg;
    unsigned char md_buf[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    int     ok = 1;
    int     i;
    long    sslversion;
    VSTRING *result;

    /*
     * Try to use sha256: our serverid choice should be strong enough to
     * resist 2nd-preimage attacks with a difficulty comparable to that of
     * DANE TLSA digests.  Failing that, we compute serverid digests with the
     * default digest, but DANE requires sha256 and sha512, so if we must
     * fall back to our default digest, DANE support won't be available.  We
     * panic if the fallback algorithm is not available, as it was verified
     * available in tls_client_init() and must not simply vanish.
     */
    if ((md = EVP_get_digestbyname(mdalg = "sha256")) == 0
	&& (md = EVP_get_digestbyname(mdalg = props->mdalg)) == 0)
	msg_panic("digest algorithm \"%s\" not found", mdalg);

    /* Salt the session lookup key with the OpenSSL runtime version. */
    sslversion = SSLeay();

    mdctx = EVP_MD_CTX_create();
    checkok(EVP_DigestInit_ex(mdctx, md, NULL));
    digest_string(props->helo ? props->helo : "");
    digest_object(&sslversion);
    digest_object(&protomask);
    digest_string(ciphers);

    /*
     * All we get from the session cache is a single bit telling us whether
     * the certificate is trusted or not, but we need to know whether the
     * trust is CA-based (in that case we must do name checks) or whether it
     * is a direct end-point match.  We mustn't confuse the two, so it is
     * best to process only TA trust in the verify callback and check the EE
     * trust after. This works since re-used sessions always have access to
     * the leaf certificate, while only the original session has the leaf and
     * the full trust chain.
     * 
     * Only the trust anchor matchlist is hashed into the session key. The end
     * entity certs are not used to determine whether a certificate is
     * trusted or not, rather these are rechecked against the leaf cert
     * outside the verification callback, each time a session is created or
     * reused.
     * 
     * Therefore, the security context of the session does not depend on the EE
     * matching data, which is checked separately each time.  So we exclude
     * the EE part of the DANE structure from the serverid digest.
     * 
     * If the security level is "dane", we send SNI information to the peer.
     * This may cause it to respond with a non-default certificate.  Since
     * certificates for sessions with no or different SNI data may not match,
     * we must include the SNI name in the session id.
     */
    if (props->dane) {
	digest_dane(props->dane, ta);
#if 0
	digest_dane(props->dane, ee);		/* See above */
#endif
	digest_string(props->tls_level == TLS_LEV_DANE ? props->host : "");
    }
    checkok(EVP_DigestFinal_ex(mdctx, md_buf, &md_len));
    EVP_MD_CTX_destroy(mdctx);
    if (!ok)
	msg_fatal("error computing %s message digest", mdalg);

    /* Check for OpenSSL contract violation */
    if (md_len > EVP_MAX_MD_SIZE)
	msg_panic("unexpectedly large %s digest size: %u", mdalg, md_len);

    /*
     * Append the digest to the serverid.  We don't compare this digest to
     * any user-specified fingerprints.  Therefore, we don't need to use a
     * colon-separated format, which saves space in the TLS session cache and
     * makes logging of session cache lookup keys more readable.
     * 
     * This does however duplicate a few lines of code from the digest encoder
     * for colon-separated cert and pkey fingerprints. If that is a
     * compelling reason to consolidate, we could use that and append the
     * result.
     */
    result = vstring_alloc(strlen(props->serverid) + 1 + 2 * md_len);
    vstring_strcpy(result, props->serverid);
    VSTRING_ADDCH(result, '&');
    for (i = 0; i < md_len; i++) {
	VSTRING_ADDCH(result, hexcodes[(md_buf[i] & 0xf0) >> 4U]);
	VSTRING_ADDCH(result, hexcodes[(md_buf[i] & 0x0f)]);
    }
    VSTRING_TERMINATE(result);
    return (vstring_export(result));
}

/* tls_digest_encode - encode message digest binary blob as xx:xx:... */

char   *tls_digest_encode(const unsigned char *md_buf, int md_len)
{
    int     i;
    char   *result = mymalloc(md_len * 3);

    /* Check for contract violation */
    if (md_len > EVP_MAX_MD_SIZE || md_len >= INT_MAX / 3)
	msg_panic("unexpectedly large message digest size: %u", md_len);

    /* No risk of overrunes, len is bounded by OpenSSL digest length */
    for (i = 0; i < md_len; i++) {
	result[i * 3] = hexcodes[(md_buf[i] & 0xf0) >> 4U];
	result[(i * 3) + 1] = hexcodes[(md_buf[i] & 0x0f)];
	result[(i * 3) + 2] = (i + 1 != md_len) ? ':' : '\0';
    }
    return (result);
}

/* tls_data_fprint - compute and encode digest of binary object */

char   *tls_data_fprint(const char *buf, int len, const char *mdalg)
{
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char md_buf[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    int     ok = 1;

    /* Previously available in "init" routine. */
    if ((md = EVP_get_digestbyname(mdalg)) == 0)
	msg_panic("digest algorithm \"%s\" not found", mdalg);

    mdctx = EVP_MD_CTX_create();
    checkok(EVP_DigestInit_ex(mdctx, md, NULL));
    digest_data(buf, len);
    checkok(EVP_DigestFinal_ex(mdctx, md_buf, &md_len));
    EVP_MD_CTX_destroy(mdctx);
    if (!ok)
	msg_fatal("error computing %s message digest", mdalg);

    return (tls_digest_encode(md_buf, md_len));
}

/* tls_cert_fprint - extract certificate fingerprint */

char   *tls_cert_fprint(X509 *peercert, const char *mdalg)
{
    int     len;
    char   *buf;
    char   *buf2;
    char   *result;

    len = i2d_X509(peercert, NULL);
    buf2 = buf = mymalloc(len);
    i2d_X509(peercert, (unsigned char **) &buf2);
    if (buf2 - buf != len)
	msg_panic("i2d_X509 invalid result length");

    result = tls_data_fprint(buf, len, mdalg);
    myfree(buf);

    return (result);
}

/* tls_pkey_fprint - extract public key fingerprint from certificate */

char   *tls_pkey_fprint(X509 *peercert, const char *mdalg)
{
    if (var_tls_bc_pkey_fprint) {
	const char *myname = "tls_pkey_fprint";
	ASN1_BIT_STRING *key;
	char   *result;

	key = X509_get0_pubkey_bitstr(peercert);
	if (key == 0)
	    msg_fatal("%s: error extracting legacy public-key fingerprint: %m",
		      myname);

	result = tls_data_fprint((char *) key->data, key->length, mdalg);
	return (result);
    } else {
	int     len;
	char   *buf;
	char   *buf2;
	char   *result;

	len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(peercert), NULL);
	buf2 = buf = mymalloc(len);
	i2d_X509_PUBKEY(X509_get_X509_PUBKEY(peercert), (unsigned char **) &buf2);
	if (buf2 - buf != len)
	    msg_panic("i2d_X509_PUBKEY invalid result length");

	result = tls_data_fprint(buf, len, mdalg);
	myfree(buf);
	return (result);
    }
}

#endif
