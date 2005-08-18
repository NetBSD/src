/*	$NetBSD: tls_verify.c,v 1.1.1.1 2005/08/18 21:11:11 rpaulo Exp $	*/

/*++
/* NAME
/*	tls_verify 3
/* SUMMARY
/*	peer name and peer certificate verification
/* SYNOPSIS
/*	#define TLS_INTERNAL
/*	#include <tls.h>
/*
/*	int	tls_verify_certificate_callback(ok, ctx, int flags)
/*	int	ok;
/*	X509_STORE_CTX *ctx;
/*	int	flags;
/* DESCRIPTION
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
/*	The SMTP client will attempt to verify the server hostname
/*	against the names listed in the server certificate. When
/*	a hostname match is required, the verification fails
/*	on certificate verification or hostname mis-match errors.
/*	When no hostname match is required, hostname verification
/*	failures are logged but they do not affect the TLS handshake
/*	or the SMTP session.
/*
/*	The rules for peer name wild-card matching differ between
/*	RFC 2818 (HTTP over TLS) and RFC 2830 (LDAP over TLS), while
/*	RFC RFC3207 (SMTP over TLS) does not specify a rule at all.
/*	Postfix uses a restrictive match algorithm. One asterisk
/*	('*') is allowed as the left-most component of a wild-card
/*	certificate name; it matches the left-most component of
/*	the peer hostname.
/*
/*	Another area where RFCs aren't always explicit is the
/*	handling of dNSNames in peer certificates. RFC 3207 (SMTP
/*	over TLS) does not mention dNSNames. Postfix follows the
/*	strict rules in RFC 2818 (HTTP over TLS), section 3.1: The
/*	Subject Alternative Name/dNSName has precedence over
/*	CommonName.  If at least one dNSName is provided, Postfix
/*	verifies those against the peer hostname and ignores the
/*	CommonName, otherwise Postfix verifies the CommonName
/*	against the peer hostname.
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
/*	TLS client or server context. This also specifies the
/*	TLScontext with enforcement options.
/* .IP flags
/* .RS
/* .IP TLS_VERIFY_PEERNAME
/*	Verify the peer hostname against the names listed
/*	in the peer certificate. The peer hostname is specified
/*	via the ctx argument.
/* .RE
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
/*--*/

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>

/* TLS library. */

#define TLS_INTERNAL
#include <tls.h>

/* match_hostname -  match hostname against pattern */

static int match_hostname(const char *pattern, const char *hostname)
{
    char   *peername_left;

    return (strcasecmp(hostname, pattern) == 0
	    || (pattern[0] == '*' && pattern[1] == '.' && pattern[2] != 0
		&& (peername_left = strchr(hostname, '.')) != 0
		&& strcasecmp(peername_left + 1, pattern + 2) == 0));
}

/* tls_verify_certificate_callback - verify peer certificate info */

int     tls_verify_certificate_callback(int ok, X509_STORE_CTX *ctx, int flags)
{
    char    buf[1024];
    X509   *err_cert;
    int     err;
    int     depth;
    int     verify_depth;
    SSL    *con;
    TLScontext_t *TLScontext;

    /* Adapted from OpenSSL apps/s_cb.c */

    err_cert = X509_STORE_CTX_get_current_cert(ctx);
    err = X509_STORE_CTX_get_error(ctx);
    depth = X509_STORE_CTX_get_error_depth(ctx);

    con = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    TLScontext = SSL_get_ex_data(con, TLScontext_index);

    X509_NAME_oneline(X509_get_subject_name(err_cert), buf, sizeof(buf));
    if (TLScontext->log_level >= 2)
	msg_info("certificate verification depth=%d subject=%s", depth, buf);

    /*
     * Test for a too long certificate chain, because that error condition is
     * not handled by the OpenSSL library.
     */
    verify_depth = SSL_get_verify_depth(con);
    if (ok && (verify_depth >= 0) && (depth > verify_depth)) {
	ok = 0;
	err = X509_V_ERR_CERT_CHAIN_TOO_LONG;
	X509_STORE_CTX_set_error(ctx, err);
    }
    if (!ok) {
	msg_info("certificate verification failed for %s: num=%d:%s",
		 TLScontext->peername_save, err,
		 X509_verify_cert_error_string(err));
    }

    /*
     * Match the peer hostname against the names listed in the peer
     * certificate.
     */
    if (ok && (depth == 0) && (flags & TLS_VERIFY_PEERNAME)) {
	int     i,
	        r;
	int     hostname_matched;
	int     dNSName_found;

	STACK_OF(GENERAL_NAME) * gens;

	/*
	 * Verify the name(s) in the peer certificate against the peer
	 * hostname. Log peer hostname/certificate mis-matches. If a match is
	 * required but fails, bail out with a verification error.
	 */
	hostname_matched = dNSName_found = 0;

	gens = X509_get_ext_d2i(err_cert, NID_subject_alt_name, 0, 0);
	if (gens) {
	    for (i = 0, r = sk_GENERAL_NAME_num(gens); i < r; ++i) {
		const GENERAL_NAME *gn = sk_GENERAL_NAME_value(gens, i);

		if (gn->type == GEN_DNS) {
		    dNSName_found++;
		    if ((hostname_matched =
			 match_hostname((char *) gn->d.ia5->data,
					TLScontext->peername_save)))
			break;
		}
	    }
	    sk_GENERAL_NAME_free(gens);
	}
	if (dNSName_found) {
	    if (!hostname_matched)
		msg_info("certificate peer name verification failed for %s: "
		       "%d dNSNames in certificate found, but none matches",
			 TLScontext->peername_save, dNSName_found);
	} else {
	    buf[0] = '\0';
	    if (!X509_NAME_get_text_by_NID(X509_get_subject_name(err_cert),
					NID_commonName, buf, sizeof(buf))) {
		msg_info("certificate peer name verification failed for %s:"
			 "cannot parse subject CommonName",
			 TLScontext->peername_save);
		tls_print_errors();
	    } else {
		hostname_matched = match_hostname(buf,
						  TLScontext->peername_save);
		if (!hostname_matched)
		    msg_info("certificate peer name verification failed for %s:"
			     " CommonName mis-match: %s",
			     TLScontext->peername_save, buf);
	    }
	}

	if (!hostname_matched) {
	    if (TLScontext->enforce_verify_errors && TLScontext->enforce_CN) {
		err = X509_V_ERR_CERT_REJECTED;
		X509_STORE_CTX_set_error(ctx, err);
		msg_info("certificate peer name verification failed for %s:"
			 " hostname mismatch", TLScontext->peername_save);
		ok = 0;
	    }
	} else
	    TLScontext->hostname_matched = 1;
    }

    /*
     * Other causes for verification failure.
     */
    switch (ctx->error) {
    case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
	X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert),
			  buf, sizeof(buf));
	msg_info("certificate verification failed for %s:"
		 "issuer %s certificate unavailable",
		 TLScontext->peername_save, buf);
	break;
    case X509_V_ERR_CERT_NOT_YET_VALID:
    case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
	msg_info("certificate verification failed for %s:"
		 "certificate not yet valid",
		 TLScontext->peername_save);
	break;
    case X509_V_ERR_CERT_HAS_EXPIRED:
    case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
	msg_info("certificate verification failed for %s:"
		 "certificate has expired",
		 TLScontext->peername_save);
	break;
    }
    if (TLScontext->log_level >= 2)
	msg_info("verify return: %d", ok);

    /*
     * Never fail in case of opportunistic mode.
     */
    if (TLScontext->enforce_verify_errors)
	return (ok);
    else
	return (1);
}

#endif
