/*	$NetBSD: tls_verify.c,v 1.1.1.1.16.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	tls_verify 3
/* SUMMARY
/*	peer name and peer certificate verification
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	int	tls_verify_certificate_callback(ok, ctx)
/*	int	ok;
/*	X509_STORE_CTX *ctx;
/*
/*	int     tls_log_verify_error(TLScontext)
/*	TLS_SESS_STATE *TLScontext;
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
/* DESCRIPTION
/*	tls_verify_certificate_callback() is called several times (directly
/*	or indirectly) from crypto/x509/x509_vfy.c. It collects errors
/*	and trust information at each element of the trust chain.
/*	The last call at depth 0 sets the verification status based
/*	on the cumulative winner (lowest depth) of errors vs. trust.
/*	We always return 1 (continue the handshake) and handle trust
/*	and peer-name verification problems at the application level.
/*
/*	tls_log_verify_error() (called only when we care about the
/*	peer certificate, that is not when opportunistic) logs the
/*	reason why the certificate failed to be verified.
/*
/*	tls_peer_CN() returns the text CommonName for the peer
/*	certificate subject, or an empty string if no CommonName was
/*	found. The result is allocated with mymalloc() and must be
/*	freed by the caller; it contains UTF-8 without non-printable
/*	ASCII characters.
/*
/*	tls_issuer_CN() returns the text CommonName for the peer
/*	certificate issuer, or an empty string if no CommonName was
/*	found. The result is allocated with mymalloc() and must be
/*	freed by the caller; it contains UTF-8 without non-printable
/*	ASCII characters.
/*
/*	tls_dns_name() returns the string value of a GENERAL_NAME
/*	from a DNS subjectAltName extension. If non-printable characters
/*	are found, a null string is returned instead. Further sanity
/*	checks may be added if the need arises.
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

/* update_error_state - safely stash away error state */

static void update_error_state(TLS_SESS_STATE *TLScontext, int depth,
			               X509 *errorcert, int errorcode)
{
    /* No news is good news */
    if (TLScontext->errordepth >= 0 && TLScontext->errordepth <= depth)
	return;

    /*
     * The certificate pointer is stable during the verification callback,
     * but may be freed after the callback returns.  Since we delay error
     * reporting till later, we bump the refcount so we can rely on it still
     * being there until later.
     */
    if (TLScontext->errorcert != 0)
	X509_free(TLScontext->errorcert);
    if (errorcert != 0)
	CRYPTO_add(&errorcert->references, 1, CRYPTO_LOCK_X509);
    TLScontext->errorcert = errorcert;
    TLScontext->errorcode = errorcode;
    TLScontext->errordepth = depth;
}

/* tls_verify_certificate_callback - verify peer certificate info */

int     tls_verify_certificate_callback(int ok, X509_STORE_CTX *ctx)
{
    char    buf[CCERT_BUFSIZ];
    X509   *cert;
    int     err;
    int     depth;
    int     max_depth;
    SSL    *con;
    TLS_SESS_STATE *TLScontext;

    /* May be NULL as of OpenSSL 1.0, thanks for the API change! */
    cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    con = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    TLScontext = SSL_get_ex_data(con, TLScontext_index);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    /* Don't log the internal root CA unless there's an unexpected error. */
    if (ok && TLScontext->tadepth > 0 && depth > TLScontext->tadepth)
	return (1);

    /*
     * Certificate chain depth limit violations are mis-reported by the
     * OpenSSL library, from SSL_CTX_set_verify(3):
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
    max_depth = SSL_get_verify_depth(con) - 1;

    /*
     * We never terminate the SSL handshake in the verification callback,
     * rather we allow the TLS handshake to continue, but mark the session as
     * unverified. The application is responsible for closing any sessions
     * with unverified credentials.
     */
    if (max_depth >= 0 && depth > max_depth) {
	X509_STORE_CTX_set_error(ctx, err = X509_V_ERR_CERT_CHAIN_TOO_LONG);
	ok = 0;
    }
    if (ok == 0)
	update_error_state(TLScontext, depth, cert, err);

    if (TLScontext->log_mask & TLS_LOG_VERBOSE) {
	if (cert)
	    X509_NAME_oneline(X509_get_subject_name(cert), buf, sizeof(buf));
	else
	    strcpy(buf, "<unknown>");
	msg_info("%s: depth=%d verify=%d subject=%s",
		 TLScontext->namaddr, depth, ok, printable(buf, '?'));
    }
    return (1);
}

/* tls_log_verify_error - Report final verification error status */

void    tls_log_verify_error(TLS_SESS_STATE *TLScontext)
{
    char    buf[CCERT_BUFSIZ];
    int     err = TLScontext->errorcode;
    X509   *cert = TLScontext->errorcert;
    int     depth = TLScontext->errordepth;

#define PURPOSE ((depth>0) ? "CA": TLScontext->am_server ? "client": "server")

    if (err == X509_V_OK)
	return;

    /*
     * Specific causes for verification failure.
     */
    switch (err) {
    case X509_V_ERR_CERT_UNTRUSTED:

	/*
	 * We expect the error cert to be the leaf, but it is likely
	 * sufficient to omit it from the log, even less user confusion.
	 */
	msg_info("certificate verification failed for %s: "
		 "not trusted by local or TLSA policy", TLScontext->namaddr);
	break;
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
	if (cert)
	    X509_NAME_oneline(X509_get_issuer_name(cert), buf, sizeof(buf));
	else
	    strcpy(buf, "<unknown>");
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
		 TLScontext->namaddr, depth - 1);
	break;
    default:
	msg_info("%s certificate verification failed for %s: num=%d:%s",
		 PURPOSE, TLScontext->namaddr, err,
		 X509_verify_cert_error_string(err));
	break;
    }
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
    int     asn1_type;
    int     utf8_length;
    unsigned char *utf8_value;
    int     ch;
    unsigned char *cp;

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
     * XXX Convert everything into UTF-8. This is a super-set of ASCII, so we
     * don't have to bother with separate code paths for ASCII-like content.
     * If the payload is ASCII then we won't waste lots of CPU cycles
     * converting it into UTF-8. It's up to OpenSSL to do something
     * reasonable when converting ASCII formats that contain non-ASCII
     * content.
     * 
     * XXX Don't bother optimizing the string length error check. It is not
     * worth the complexity.
     */
    asn1_type = ASN1_STRING_type(entry_str);
    if ((utf8_length = ASN1_STRING_to_UTF8(&utf8_value, entry_str)) < 0) {
	msg_warn("%s: %s: error decoding peer %s of ASN.1 type=%d",
		 myname, TLScontext->namaddr, label, asn1_type);
	tls_print_errors();
	return (0);
    }

    /*
     * No returns without cleaning up. A good optimizer will replace multiple
     * blocks of identical code by jumps to just one such block.
     */
#define TLS_TEXT_NAME_RETURN(x) do { \
	char *__tls_text_name_temp = (x); \
	OPENSSL_free(utf8_value); \
	return (__tls_text_name_temp); \
    } while (0)

    /*
     * Remove trailing null characters. They would give false alarms with the
     * length check and with the embedded null check.
     */
#define TRIM0(s, l) do { while ((l) > 0 && (s)[(l)-1] == 0) --(l); } while (0)

    TRIM0(utf8_value, utf8_length);

    /*
     * Enforce the length limit, because the caller will copy the result into
     * a fixed-length buffer.
     */
    if (utf8_length >= CCERT_BUFSIZ) {
	msg_warn("%s: %s: peer %s too long: %d",
		 myname, TLScontext->namaddr, label, utf8_length);
	TLS_TEXT_NAME_RETURN(0);
    }

    /*
     * Reject embedded nulls in ASCII or UTF-8 names. OpenSSL is responsible
     * for producing properly-formatted UTF-8.
     */
    if (utf8_length != strlen((char *) utf8_value)) {
	msg_warn("%s: %s: NULL character in peer %s",
		 myname, TLScontext->namaddr, label);
	TLS_TEXT_NAME_RETURN(0);
    }

    /*
     * Reject non-printable ASCII characters in UTF-8 content.
     * 
     * Note: the code below does not find control characters in illegal UTF-8
     * sequences. It's OpenSSL's job to produce valid UTF-8, and reportedly,
     * it does validation.
     */
    for (cp = utf8_value; (ch = *cp) != 0; cp++) {
	if (ISASCII(ch) && !ISPRINT(ch)) {
	    msg_warn("%s: %s: non-printable content in peer %s",
		     myname, TLScontext->namaddr, label);
	    TLS_TEXT_NAME_RETURN(0);
	}
    }
    TLS_TEXT_NAME_RETURN(mystrdup((char *) utf8_value));
}

/* tls_dns_name - Extract valid DNS name from subjectAltName value */

const char *tls_dns_name(const GENERAL_NAME * gn,
			         const TLS_SESS_STATE *TLScontext)
{
    const char *myname = "tls_dns_name";
    char   *cp;
    const char *dnsname;
    int     len;

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
    len = ASN1_STRING_length(gn->d.ia5);
    TRIM0(dnsname, len);

    /*
     * Per Dr. Steven Henson of the OpenSSL development team, ASN1_IA5STRING
     * values can have internal ASCII NUL values in this context because
     * their length is taken from the decoded ASN1 buffer, a trailing NUL is
     * always appended to make sure that the string is terminated, but the
     * ASN.1 length may differ from strlen().
     */
    if (len != strlen(dnsname)) {
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
    if (*dnsname && !allprint(dnsname)) {
	cp = mystrdup(dnsname);
	msg_warn("%s: %s: non-printable characters in subjectAltName: %.100s",
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
		       "subject CN", TLScontext, DONT_GRIPE);
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
			   "issuer Organization", TLScontext, DONT_GRIPE);
    return (cn ? cn : mystrdup(""));
}

#endif
