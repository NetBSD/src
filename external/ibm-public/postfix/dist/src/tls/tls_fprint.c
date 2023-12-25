/*	$NetBSD: tls_fprint.c,v 1.3.2.1 2023/12/25 12:43:36 martin Exp $	*/

/*++
/* NAME
/*	tls_fprint 3
/* SUMMARY
/*	Digests fingerprints and all that.
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	EVP_MD *tls_digest_byname(const char *mdalg, EVP_MD_CTX **mdctxPtr)
/*	const char *mdalg;
/*	EVP_MD_CTX **mdctxPtr;
/*
/*	char	*tls_serverid_digest(TLScontext, props, ciphers)
/*	TLS_SESS_STATE *TLScontext;
/*	const TLS_CLIENT_START_PROPS *props;
/*	const char *ciphers;
/*
/*	char	*tls_digest_encode(md_buf, md_len)
/*	const unsigned char *md_buf;
/*	const char *md_len;
/*
/*	char	*tls_cert_fprint(peercert, mdalg)
/*	X509	*peercert;
/*	const char *mdalg;
/*
/*	char	*tls_pkey_fprint(peercert, mdalg)
/*	X509	*peercert;
/*	const char *mdalg;
/* DESCRIPTION
/*	tls_digest_byname() constructs, and optionally returns, an EVP_MD_CTX
/*	handle for performing digest operations with the algorithm named by the
/*	mdalg parameter.  The return value is non-null on success, and holds a
/*	digest algorithm handle.  If the mdctxPtr argument is non-null the
/*	created context is returned to the caller, who is then responsible for
/*	deleting it by calling EVP_MD_ctx_free() once it is no longer needed.
/*
/*	tls_digest_encode() converts a binary message digest to a hex ASCII
/*	format with ':' separators between each pair of hex digits.
/*	The return value is dynamically allocated with mymalloc(),
/*	and the caller must eventually free it with myfree().
/*
/*	tls_cert_fprint() returns a fingerprint of the given
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
/*	client with "&" plus a digest of additional parameters needed to ensure
/*	that re-used sessions are more likely to be reused and that they will
/*	satisfy all protocol and security requirements.  The return value is
/*	dynamically allocated with mymalloc(), and the caller must eventually
/*	free it with myfree().
/*
/*	Arguments:
/* .IP mdalg
/*	A digest algorithm name, such as "sha256".
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
/* .IP mdctxPtr
/*	Pointer to an (EVP_MD_CTX *) handle, or NULL if only probing for
/*	algorithm support without immediate use in mind.
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

#define CHECK_OK_AND(stillok) (ok = ok && (stillok))
#define CHECK_OK_AND_DIGEST_OBJECT(m, p) \
	CHECK_OK_AND_DIGEST_DATA((m), (unsigned char *)(p), sizeof(*(p)))
#define CHECK_OK_AND_DIGEST_DATA(m, p, l) CHECK_OK_AND(digest_bytes((m), (p), (l)))
#define CHECK_OK_AND_DIGEST_CHARS(m, s) CHECK_OK_AND(digest_chars((m), (s)))

/* digest_bytes - hash octet string of given length */

static int digest_bytes(EVP_MD_CTX *ctx, const unsigned char *buf, size_t len)
{
    return (EVP_DigestUpdate(ctx, buf, len));
}

/* digest_chars - hash string including trailing NUL */

static int digest_chars(EVP_MD_CTX *ctx, const char *s)
{
    return (EVP_DigestUpdate(ctx, s, strlen(s) + 1));
}

/* tlsa_cmp - compare TLSA RRs for sorting to canonical order */

static int tlsa_cmp(const void *a, const void *b)
{
    TLS_TLSA *p = *(TLS_TLSA **) a;
    TLS_TLSA *q = *(TLS_TLSA **) b;
    int     d;

    if ((d = (int) p->usage - (int) q->usage) != 0)
	return d;
    if ((d = (int) p->selector - (int) q->selector) != 0)
	return d;
    if ((d = (int) p->mtype - (int) q->mtype) != 0)
	return d;
    if ((d = (int) p->length - (int) q->length) != 0)
	return d;
    return (memcmp(p->data, q->data, p->length));
}

/* tls_digest_tlsa - fold in digest of TLSA records */

static int tls_digest_tlsa(EVP_MD_CTX *mdctx, TLS_TLSA *tlsa)
{
    TLS_TLSA *p;
    TLS_TLSA **arr;
    int     ok = 1;
    int     n;
    int     i;

    for (n = 0, p = tlsa; p != 0; p = p->next)
	++n;
    arr = (TLS_TLSA **) mymalloc(n * sizeof(*arr));
    for (i = 0, p = tlsa; p; p = p->next)
	arr[i++] = (void *) p;
    qsort(arr, n, sizeof(arr[0]), tlsa_cmp);

    CHECK_OK_AND_DIGEST_OBJECT(mdctx, &n);
    for (i = 0; i < n; ++i) {
	CHECK_OK_AND_DIGEST_OBJECT(mdctx, &arr[i]->usage);
	CHECK_OK_AND_DIGEST_OBJECT(mdctx, &arr[i]->selector);
	CHECK_OK_AND_DIGEST_OBJECT(mdctx, &arr[i]->mtype);
	CHECK_OK_AND_DIGEST_OBJECT(mdctx, &arr[i]->length);
	CHECK_OK_AND_DIGEST_DATA(mdctx, arr[i]->data, arr[i]->length);
    }
    myfree((void *) arr);
    return (ok);
}

/* tls_digest_byname - test availability or prepare to use digest */

const EVP_MD *tls_digest_byname(const char *mdalg, EVP_MD_CTX **mdctxPtr)
{
    const EVP_MD *md;
    EVP_MD_CTX *mdctx = NULL;
    int     ok = 1;

    /*
     * In OpenSSL 3.0, because of dynamically variable algorithm providers,
     * there is a time-of-check/time-of-use issue that means that abstract
     * algorithm handles returned by EVP_get_digestbyname() can (and not
     * infrequently do) return ultimately unusable algorithms, to check for
     * actual availability, one needs to use the new EVP_MD_fetch() API, or
     * indirectly check usability by creating a concrete context. We take the
     * latter approach here (works for 1.1.1 without #ifdef).
     * 
     * Note that EVP_MD_CTX_{create,destroy} were renamed to, respectively,
     * EVP_MD_CTX_{new,free} in OpenSSL 1.1.0.
     */
    CHECK_OK_AND(md = EVP_get_digestbyname(mdalg));

    /*
     * Sanity check: Newer shared libraries could (hypothetical ABI break)
     * allow larger digests, we avoid such poison algorithms.
     */
    CHECK_OK_AND(EVP_MD_size(md) <= EVP_MAX_MD_SIZE);
    CHECK_OK_AND(mdctx = EVP_MD_CTX_new());
    CHECK_OK_AND(EVP_DigestInit_ex(mdctx, md, NULL));


    if (ok && mdctxPtr != 0)
	*mdctxPtr = mdctx;
    else
	EVP_MD_CTX_free(mdctx);
    return (ok ? md : 0);
}

/* tls_serverid_digest - suffix props->serverid with parameter digest */

char   *tls_serverid_digest(TLS_SESS_STATE *TLScontext,
			            const TLS_CLIENT_START_PROPS *props,
			            const char *ciphers)
{
    EVP_MD_CTX *mdctx;
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
     * available in tls_client_init() and must not simply vanish.  Our
     * provider set is not expected to change once the OpenSSL library is
     * initialized.
     */
    if (tls_digest_byname(mdalg = LN_sha256, &mdctx) == 0
	&& tls_digest_byname(mdalg = props->mdalg, &mdctx) == 0)
	msg_panic("digest algorithm \"%s\" not found", props->mdalg);

    /* Salt the session lookup key with the OpenSSL runtime version. */
    sslversion = OpenSSL_version_num();

    CHECK_OK_AND_DIGEST_CHARS(mdctx, props->helo ? props->helo : "");
    CHECK_OK_AND_DIGEST_OBJECT(mdctx, &sslversion);
    CHECK_OK_AND_DIGEST_CHARS(mdctx, props->protocols);
    CHECK_OK_AND_DIGEST_CHARS(mdctx, ciphers);

    /*
     * Ensure separation of caches for sessions where DANE trust
     * configuration succeeded from those where it did not.  The latter
     * should always see a certificate validation failure, both on initial
     * handshake and on resumption.
     */
    CHECK_OK_AND_DIGEST_OBJECT(mdctx, &TLScontext->must_fail);

    /*
     * DNS-based or synthetic DANE trust settings are potentially used at all
     * levels above "encrypt".
     */
    if (TLScontext->level > TLS_LEV_ENCRYPT
	&& props->dane && props->dane->tlsa) {
	CHECK_OK_AND(tls_digest_tlsa(mdctx, props->dane->tlsa));
    } else {
	int     none = 0;		/* Record a TLSA RR count of zero */

	CHECK_OK_AND_DIGEST_OBJECT(mdctx, &none);
    }

    /*
     * Include the chosen SNI name, which can affect server certificate
     * selection.
     */
    if (TLScontext->level > TLS_LEV_ENCRYPT && TLScontext->peer_sni)
	CHECK_OK_AND_DIGEST_CHARS(mdctx, TLScontext->peer_sni);
    else
	CHECK_OK_AND_DIGEST_CHARS(mdctx, "");

    CHECK_OK_AND(EVP_DigestFinal_ex(mdctx, md_buf, &md_len));
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

    /* No risk of overruns, len is bounded by OpenSSL digest length */
    for (i = 0; i < md_len; i++) {
	result[i * 3] = hexcodes[(md_buf[i] & 0xf0) >> 4U];
	result[(i * 3) + 1] = hexcodes[(md_buf[i] & 0x0f)];
	result[(i * 3) + 2] = (i + 1 != md_len) ? ':' : '\0';
    }
    return (result);
}

/* tls_data_fprint - compute and encode digest of binary object */

static char *tls_data_fprint(const unsigned char *buf, int len, const char *mdalg)
{
    EVP_MD_CTX *mdctx = NULL;
    unsigned char md_buf[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    int     ok = 1;

    /* Previously available in "init" routine. */
    if (tls_digest_byname(mdalg, &mdctx) == 0)
	msg_panic("digest algorithm \"%s\" not found", mdalg);

    CHECK_OK_AND_DIGEST_DATA(mdctx, buf, len);
    CHECK_OK_AND(EVP_DigestFinal_ex(mdctx, md_buf, &md_len));
    EVP_MD_CTX_destroy(mdctx);
    if (!ok)
	msg_fatal("error computing %s message digest", mdalg);

    return (tls_digest_encode(md_buf, md_len));
}

/* tls_cert_fprint - extract certificate fingerprint */

char   *tls_cert_fprint(X509 *peercert, const char *mdalg)
{
    int     len;
    unsigned char *buf;
    unsigned char *buf2;
    char   *result;

    len = i2d_X509(peercert, NULL);
    buf2 = buf = mymalloc(len);
    i2d_X509(peercert, &buf2);
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

	result = tls_data_fprint(key->data, key->length, mdalg);
	return (result);
    } else {
	int     len;
	unsigned char *buf;
	unsigned char *buf2;
	char   *result;

	len = i2d_X509_PUBKEY(X509_get_X509_PUBKEY(peercert), NULL);
	buf2 = buf = mymalloc(len);
	i2d_X509_PUBKEY(X509_get_X509_PUBKEY(peercert), &buf2);
	if (buf2 - buf != len)
	    msg_panic("i2d_X509_PUBKEY invalid result length");

	result = tls_data_fprint(buf, len, mdalg);
	myfree(buf);
	return (result);
    }
}

#endif
