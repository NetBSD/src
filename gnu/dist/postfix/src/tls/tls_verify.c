/*++
/* NAME
/*	tls_verify 3
/* SUMMARY
/*	peer name and peer certificate verification
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	char *tls_peer_CN(peercert, TLScontext)
/*	X509   *peercert;
/*	TLS_SESS_STATE *TLScontext;
/*
/*	char *tls_issuer_CN(peercert, TLScontext)
/*	X509   *peercert;
/*	TLS_SESS_STATE *TLScontext;
/*
/*	const char *tls_dns_name(gn, TLScontext)
/*	const GENERAL_NAME *gn;
/*	TLS_SESS_STATE *TLScontext;
/*
/*	char *tls_fingerprint(peercert, dgst)
/*	X509   *peercert;
/*	const char *dgst;
/*
/*	int	tls_verify_certificate_callback(ok, ctx)
/*	int	ok;
/*	X509_STORE_CTX *ctx;
/* DESCRIPTION
/*	tls_peer_CN() returns the text CommonName for the peer
/*	certificate subject, or an empty string if no CommonName was
/*	found. The result is allocated with mymalloc() and must be
/*	freed by the caller.
/*
/*	tls_issuer_CN() returns the text CommonName for the peer
/*	certificate issuer, or an empty string if no CommonName was
/*	found. The result is allocated with mymalloc() and must be
/*	freed by the caller.
/*
/*	tls_dns_name() returns the string value of a GENERAL_NAME
/*	from a DNS subjectAltName extension. If non-printable characters
/*	are found, a null string is returned instead. Further sanity
/*	checks may be added if the need arises.
/*
/* 	tls_fingerprint() returns a fingerprint of the the given
/*	certificate using the requested message digest. Panics if the
/*	(previously verified) digest algorithm is not found. The return
/*	value is dynamically allocated with mymalloc(), and the caller
/*	must eventually free it with myfree().
/*
/*	tls_verify_callback() is called several times (directly or
/*	indirectly) from crypto/x509/x509_vfy.c. It is called as
/*	a final check, and if it returns "0", the handshake is
/*	immediately shut down and the connection fails.
/*
/*	Postfix/TLS has two modes, the "opportunistic" mode and
/*	the "enforce" mode:
/*
/*	In the "opportunistic" mode we never want the connection
/*	to fail just because there is something wrong with the
/*	peer's certificate. After all, we would have sent or received
/*	the mail even if TLS weren't available.  Therefore the
/*	return value is always "1".
/*
/*	The SMTP client or server may require TLS (e.g. to protect
/*	passwords), while peer certificates are optional.  In this
/*	case we must return "1" even when we are unhappy with the
/*	peer certificate.  Only when peer certificates are required,
/*      certificate verification failure will result in immediate
/*	termination (return 0).
/*
/*	The only error condition not handled inside the OpenSSL
/*	library is the case of a too-long certificate chain. We
/*	test for this condition only if "ok = 1", that is, if
/*	verification didn't fail because of some earlier problem.
/*
/*	Arguments:
/* .IP ok
/*	Result of prior verification: non-zero means success.  In
/*	order to reduce the noise level, some tests or error reports
/*	are disabled when verification failed because of some
/*	earlier problem.
/* .IP ctx
/*	SSL application context. This links to the Postfix TLScontext
/*	with enforcement and logging options.
/* .IP gn
/*	An OpenSSL GENERAL_NAME structure holding a DNS subjectAltName
/*	to be decoded and checked for validity.
/* .IP peercert
/*	Server or client X.509 certificate.
/* .IP dgst
/*	Name of a message digest algorithm suitable for computing secure
/*	(1st pre-image resistant) message digests of certificates. For now,
/*	md5, sha1, or member of SHA-2 family if supported by OpenSSL.
/* .IP TLScontext
/*	Server or client context for warning messages.
/* DIAGNOSTICS
/*	tls_peer_CN(), tls_issuer_CN() and tls_dns_name() log a warning
/*	when 1) the requested information is not available in the specified
/*	certificate, 2) the result exceeds a fixed limit, 3) the result
/*	contains NUL characters or the result contains non-printable or
/*	non-ASCII characters.
/* LICENSE
/* .ad
/* .fi
/*	This software is free. You can do with it whatever you want.
/*	The original author kindly requests that you acknowledge
/*	the use of his software.
/* AUTHOR(S)
/*	Originally written by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
/*
/*	Updated by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Victor Duchovni
/*	Morgan Stanley
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

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

static const char hexcodes[] = "0123456789ABCDEF";

/* tls_verify_certificate_callback - verify peer certificate info */

int     tls_verify_certificate_callback(int ok, X509_STORE_CTX *ctx)
{
    char    buf[CCERT_BUFSIZ];
    X509   *cert;
    int     err;
    int     depth;
    SSL    *con;
    TLS_SESS_STATE *TLScontext;

    depth = X509_STORE_CTX_get_error_depth(ctx);
    cert = X509_STORE_CTX_get_current_cert(ctx);
    con = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    TLScontext = SSL_get_ex_data(con, TLScontext_index);

    /*
     * The callback function is called repeatedly, first with the root
     * certificate, and then with each intermediate certificate ending with
     * the peer certificate.
     * 
     * With each call, the validity of the current certificate (usage bits,
     * attributes, expiration, ... checked by the OpenSSL library) is
     * available in the "ok" argument. Error details are available via
     * X509_STORE_CTX API.
     * 
     * We never terminate the SSL handshake in the verification callback, rather
     * we allow the TLS handshake to continue, but mark the session as
     * unverified. The application is responsible for closing any sessions
     * with unverified credentials.
     * 
     * Certificate chain depth limit violations are mis-reported by the OpenSSL
     * library, from SSL_CTX_set_verify(3):
     * 
     * The certificate verification depth set with SSL[_CTX]_verify_depth()
     * stops the verification at a certain depth. The error message produced
     * will be that of an incomplete certificate chain and not
     * X509_V_ERR_CERT_CHAIN_TOO_LONG as may be expected.
     * 
     * We set a limit that is one higher than the user requested limit. If this
     * higher limit is reached, we raise an error even a trusted root CA is
     * present at this depth. This disambiguates trust chain truncation from
     * an incomplete trust chain.
     */
    if (depth >= SSL_get_verify_depth(con)) {
	ok = 0;
	X509_STORE_CTX_set_error(ctx, X509_V_ERR_CERT_CHAIN_TOO_LONG);
    }
    if (TLScontext->log_level >= 2) {
	X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
	msg_info("%s: certificate verification depth=%d verify=%d subject=%s",
		 TLScontext->namaddr, depth, ok, printable(buf, '?'));
    }

    /*
     * If no errors, or we are not logging verification errors, we are done.
     */
    if (ok || (TLScontext->peer_status & TLS_CERT_FLAG_LOGGED) != 0)
	return (1);

    /*
     * One counter-example is enough.
     */
    TLScontext->peer_status |= TLS_CERT_FLAG_LOGGED;

#define PURPOSE ((depth>0) ? "CA": TLScontext->am_server ? "client": "server")

    /*
     * Specific causes for verification failure.
     */
    switch (err = X509_STORE_CTX_get_error(ctx)) {
    case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	msg_info("certificate verification failed for %s: "
		 "self-signed certificate", TLScontext->namaddr);
	break;
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
    case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:

	/*
	 * There is no difference between issuing cert not provided and
	 * provided, but not found in CAfile/CApath. Either way, we don't
	 * trust it.
	 */
	X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert),
			  buf, sizeof(buf));
	msg_info("certificate verification failed for %s: untrusted issuer %s",
		 TLScontext->namaddr, printable(buf, '?'));
	break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	msg_info("%s certificate verification failed for %s: certificate not"
		 " yet valid", PURPOSE, TLScontext->namaddr);
	break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
	msg_info("%s certificate verification failed for %s: certificate has"
		 " expired", PURPOSE, TLScontext->namaddr);
	break;
    case X509_V_ERR_INVALID_PURPOSE:
	msg_info("certificate verification failed for %s: not designated for "
		 "use as a %s certificate", TLScontext->namaddr, PURPOSE);
	break;
    case X509_V_ERR_CERT_CHAIN_TOO_LONG:
	msg_info("certificate verification failed for %s: "
		 "certificate chain longer than limit(%d)",
		 TLScontext->namaddr, SSL_get_verify_depth(con) - 1);
	break;
    default:
	msg_info("%s certificate verification failed for %s: num=%d:%s",
		 PURPOSE, TLScontext->namaddr, err,
		 X509_verify_cert_error_string(err));
	break;
    }

    return (1);
}

#ifndef DONT_GRIPE
#define DONT_GRIPE 0
#define DO_GRIPE 1
#endif

/* tls_text_name - extract certificate property value by name */

static char *tls_text_name(X509_NAME *name, int nid, const char *label,
			        const TLS_SESS_STATE *TLScontext, int gripe)
{
    const char *myname = "tls_text_name";
    int     pos;
    X509_NAME_ENTRY *entry;
    ASN1_STRING *entry_str;
    int     typ;
    int     len;
    unsigned char *val;
    unsigned char *utf;
    char   *cp;

    if (name == 0 || (pos = X509_NAME_get_index_by_NID(name, nid, -1)) < 0) {
	if (gripe != DONT_GRIPE) {
	    msg_warn("%s: %s: peer certificate has no %s",
		     myname, TLScontext->namaddr, label);
	    tls_print_errors();
	}
	return (0);
    }
#if 0

    /*
     * If the match is required unambiguous, insist that that no other values
     * be present.
     */
    if (X509_NAME_get_index_by_NID(name, nid, pos) >= 0) {
	msg_warn("%s: %s: multiple %ss in peer certificate",
		 myname, TLScontext->namaddr, label);
	return (0);
    }
#endif

    if ((entry = X509_NAME_get_entry(name, pos)) == 0) {
	/* This should not happen */
	msg_warn("%s: %s: error reading peer certificate %s entry",
		 myname, TLScontext->namaddr, label);
	tls_print_errors();
	return (0);
    }
    if ((entry_str = X509_NAME_ENTRY_get_data(entry)) == 0) {
	/* This should not happen */
	msg_warn("%s: %s: error reading peer certificate %s data",
		 myname, TLScontext->namaddr, label);
	tls_print_errors();
	return (0);
    }

    /*
     * Peername checks are security sensitive, carefully scrutinize the
     * input!
     */
    typ = ASN1_STRING_type(entry_str);
    len = ASN1_STRING_length(entry_str);
    val = ASN1_STRING_data(entry_str);

    /*
     * http://www.apps.ietf.org/rfc/rfc3280.html#sec-4.1.2.4 Quick Summary:
     * 
     * The DirectoryString type is defined as a choice of PrintableString,
     * TeletexString, BMPString, UTF8String, and UniversalString.  The
     * UTF8String encoding is the preferred encoding, and all certificates
     * issued after December 31, 2003 MUST use the UTF8String encoding of
     * DirectoryString (except as noted below).
     * 
     * XXX: 2007, the above has not happened yet (of course), and we continue to
     * see new certificates with T61STRING (Teletex) attribute values.
     * 
     * XXX: 2007, at this time there are only two ASN.1 fixed width multi-byte
     * string encodings, BMPSTRING (16 bit Unicode) and UniversalString
     * (32-bit Unicode). The only variable width ASN.1 string encoding is
     * UTF8 with all the other encodings being 1 byte wide subsets or subsets
     * of ASCII.
     * 
     * Relying on this could simplify the code, because we would never convert
     * unexpected single-byte encodings, but is involves too many cases to be
     * sure that we have a complete set and the assumptions may become false.
     * So, we pessimistically convert encodings not blessed by RFC 2459, and
     * filter out all types that are not string types as a side-effect of
     * UTF8 conversion (the ASN.1 library knows which types are string types
     * and how wide they are...).
     * 
     * XXX: Two possible states after switch, either "utf == val" and it MUST
     * NOT be freed with OPENSSL_free(), or "utf != val" and it MUST be freed
     * with OPENSSL_free().
     */
    switch (typ) {
    case V_ASN1_PRINTABLESTRING:		/* X.500 portable ASCII
						 * printables */
    case V_ASN1_IA5STRING:			/* ISO 646 ~ ASCII */
    case V_ASN1_T61STRING:			/* Essentially ISO-Latin */
    case V_ASN1_UTF8STRING:			/* UTF8 */
	utf = val;
	break;

    default:

	/*
	 * May shrink in wash, but BMPSTRING only shrinks by 50%. Others may
	 * shrink by up to 75%. We Sanity check the length before bothering
	 * to copy any large strings to convert to UTF8, only to find out
	 * they don't fit. So long as no new MB types are introduced, and
	 * weird string encodings unsanctioned by RFC 3280, are used in the
	 * issuer or subject DN, this "conservative" estimate will be exact.
	 */
	len >>= (typ == V_ASN1_BMPSTRING) ? 1 : 2;
	if (len >= CCERT_BUFSIZ) {
	    msg_warn("%s: %s: peer %s too long: %d",
		     myname, TLScontext->namaddr, label, len);
	    return (0);
	}
	if ((len = ASN1_STRING_to_UTF8(&utf, entry_str)) < 0) {
	    msg_warn("%s: %s: error decoding peer %s of ASN.1 type=%d",
		     myname, TLScontext->namaddr, label, typ);
	    tls_print_errors();
	    return (0);
	}
    }
#define RETURN(x) do { if (utf!=val) OPENSSL_free(utf); return (x); } while (0)

    if (len >= CCERT_BUFSIZ) {
	msg_warn("%s: %s: peer %s too long: %d",
		 myname, TLScontext->namaddr, label, len);
	RETURN(0);
    }
    if (len != strlen((char *) utf)) {
	msg_warn("%s: %s: internal NUL in peer %s",
		 myname, TLScontext->namaddr, label);
	RETURN(0);
    }
    for (cp = (char *) utf; *cp; cp++) {
	if (!ISASCII(*cp) || !ISPRINT(*cp)) {
	    msg_warn("%s: %s: non-printable characters in peer %s",
		     myname, TLScontext->namaddr, label);
	    RETURN(0);
	}
    }
    cp = mystrdup((char *) utf);
    RETURN(cp);
}

/* tls_dns_name - Extract valid DNS name from subjectAltName value */

const char *tls_dns_name(const GENERAL_NAME * gn,
			         const TLS_SESS_STATE *TLScontext)
{
    const char *myname = "tls_dns_name";
    char   *cp;
    const char *dnsname;

    /*
     * Peername checks are security sensitive, carefully scrutinize the
     * input!
     */
    if (gn->type != GEN_DNS)
	msg_panic("%s: Non DNS input argument", myname);

    /*
     * We expect the OpenSSL library to construct GEN_DNS extesion objects as
     * ASN1_IA5STRING values. Check we got the right union member.
     */
    if (ASN1_STRING_type(gn->d.ia5) != V_ASN1_IA5STRING) {
	msg_warn("%s: %s: invalid ASN1 value type in subjectAltName",
		 myname, TLScontext->namaddr);
	return (0);
    }

    /*
     * Safe to treat as an ASCII string possibly holding a DNS name
     */
    dnsname = (char *) ASN1_STRING_data(gn->d.ia5);

    /*
     * Per Dr. Steven Henson of the OpenSSL development team, ASN1_IA5STRING
     * values can have internal ASCII NUL values in this context because
     * their length is taken from the decoded ASN1 buffer, a trailing NUL is
     * always appended to make sure that the string is terminated, but the
     * ASN.1 length may differ from strlen().
     */
    if (ASN1_STRING_length(gn->d.ia5) != strlen(dnsname)) {
	msg_warn("%s: %s: internal NUL in subjectAltName",
		 myname, TLScontext->namaddr);
	return 0;
    }

    /*
     * XXX: Should we be more strict and call valid_hostname()? So long as
     * the name is safe to handle, if it is not a valid hostname, it will not
     * compare equal to the expected peername, so being more strict than
     * "printable" is likely excessive...
     */
    for (cp = (char *) dnsname; cp && *cp; cp++)
	if (!ISASCII(*cp) || !ISPRINT(*cp)) {
	    cp = mystrdup(dnsname);
	    msg_warn("%s: %s: non-printable characters in subjectAltName: %s",
		     myname, TLScontext->namaddr, printable(cp, '?'));
	    myfree(cp);
	    return 0;
	}
    return (dnsname);
}

/* tls_peer_CN - extract peer common name from certificate */

char   *tls_peer_CN(X509 *peercert, const TLS_SESS_STATE *TLScontext)
{
    char   *cn;

    cn = tls_text_name(X509_get_subject_name(peercert), NID_commonName,
		       "subject CN", TLScontext, DO_GRIPE);
    return (cn ? cn : mystrdup(""));
}

/* tls_issuer_CN - extract issuer common name from certificate */

char   *tls_issuer_CN(X509 *peer, const TLS_SESS_STATE *TLScontext)
{
    X509_NAME *name;
    char   *cn;

    name = X509_get_issuer_name(peer);

    /*
     * If no issuer CN field, use Organization instead. CA certs without a CN
     * are common, so we only complain if the organization is also missing.
     */
    if ((cn = tls_text_name(name, NID_commonName,
			    "issuer CN", TLScontext, DONT_GRIPE)) == 0)
	cn = tls_text_name(name, NID_organizationName,
			   "issuer Organization", TLScontext, DO_GRIPE);
    return (cn ? cn : mystrdup(""));
}

/* tls_fingerprint - extract fingerprint from certificate */

char   *tls_fingerprint(X509 *peercert, const char *dgst)
{
    const char *myname = "tls_fingerprint";
    const EVP_MD *md_alg;
    unsigned char md_buf[EVP_MAX_MD_SIZE];
    unsigned int md_len;
    int     i;
    char   *result = 0;

    /* Previously available in "init" routine. */
    if ((md_alg = EVP_get_digestbyname(dgst)) == 0)
	msg_panic("%s: digest algorithm \"%s\" not found", myname, dgst);

    /* Fails when serialization to ASN.1 runs out of memory */
    if (X509_digest(peercert, md_alg, md_buf, &md_len) == 0)
	msg_fatal("%s: error computing certificate %s digest (out of memory?)",
		  myname, dgst);

    /* Check for OpenSSL contract violation */
    if (md_len > EVP_MAX_MD_SIZE || md_len >= INT_MAX / 3)
	msg_panic("%s: unexpectedly large %s digest size: %u",
		  myname, dgst, md_len);

    result = mymalloc(md_len * 3);
    for (i = 0; i < md_len; i++) {
	result[i * 3] = hexcodes[(md_buf[i] & 0xf0) >> 4U];
	result[(i * 3) + 1] = hexcodes[(md_buf[i] & 0x0f)];
	result[(i * 3) + 2] = (i + 1 != md_len) ? ':' : '\0';
    }
    return (result);
}

#endif
