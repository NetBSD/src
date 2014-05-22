/*	$NetBSD: tls_client.c,v 1.4.4.3 2014/05/22 14:08:03 yamt Exp $	*/

/*++
/* NAME
/*	tls_client
/* SUMMARY
/*	client-side TLS engine
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	TLS_APPL_STATE *tls_client_init(init_props)
/*	const TLS_CLIENT_INIT_PROPS *init_props;
/*
/*	TLS_SESS_STATE *tls_client_start(start_props)
/*	const TLS_CLIENT_START_PROPS *start_props;
/*
/*	void	tls_client_stop(app_ctx, stream, failure, TLScontext)
/*	TLS_APPL_STATE *app_ctx;
/*	VSTREAM	*stream;
/*	int	failure;
/*	TLS_SESS_STATE *TLScontext;
/* DESCRIPTION
/*	This module is the interface between Postfix TLS clients,
/*	the OpenSSL library and the TLS entropy and cache manager.
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
/*	tls_client_init() is called once when the SMTP client
/*	initializes.
/*	Certificate details are also decided during this phase,
/*	so peer-specific certificate selection is not possible.
/*
/*	tls_client_start() activates the TLS session over an established
/*	stream. We expect that network buffers are flushed and
/*	the TLS handshake can begin immediately.
/*
/*	tls_client_stop() sends the "close notify" alert via
/*	SSL_shutdown() to the peer and resets all connection specific
/*	TLS data. As RFC2487 does not specify a separate shutdown, it
/*	is assumed that the underlying TCP connection is shut down
/*	immediately afterwards. Any further writes to the channel will
/*	be discarded, and any further reads will report end-of-file.
/*	If the failure flag is set, no SSL_shutdown() handshake is performed.
/*
/*	Once the TLS connection is initiated, information about the TLS
/*	state is available via the TLScontext structure:
/* .IP TLScontext->protocol
/*	the protocol name (SSLv2, SSLv3, TLSv1),
/* .IP TLScontext->cipher_name
/*	the cipher name (e.g. RC4/MD5),
/* .IP TLScontext->cipher_usebits
/*	the number of bits actually used (e.g. 40),
/* .IP TLScontext->cipher_algbits
/*	the number of bits the algorithm is based on (e.g. 128).
/* .PP
/*	The last two values may differ from each other when export-strength
/*	encryption is used.
/*
/*	If the peer offered a certificate, part of the certificate data are
/*	available as:
/* .IP TLScontext->peer_status
/*	A bitmask field that records the status of the peer certificate
/*	verification. This consists of one or more of
/*	TLS_CERT_FLAG_PRESENT, TLS_CERT_FLAG_ALTNAME, TLS_CERT_FLAG_TRUSTED
/*	and TLS_CERT_FLAG_MATCHED.
/* .IP TLScontext->peer_CN
/*	Extracted CommonName of the peer, or zero-length string if the
/*	information could not be extracted.
/* .IP TLScontext->issuer_CN
/*	Extracted CommonName of the issuer, or zero-length string if the
/*	information could not be extracted.
/* .IP TLScontext->peer_fingerprint
/*	At the fingerprint security level, if the peer presented a certificate
/*	the fingerprint of the certificate.
/* .PP
/*	If no peer certificate is presented the peer_status is set to 0.
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

#ifdef USE_TLS
#include <string.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <argv.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <msg.h>
#include <iostuff.h>			/* non-blocking */

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#include <tls_mgr.h>
#define TLS_INTERNAL
#include <tls.h>

/* Application-specific. */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* load_clnt_session - load session from client cache (non-callback) */

static SSL_SESSION *load_clnt_session(TLS_SESS_STATE *TLScontext)
{
    const char *myname = "load_clnt_session";
    SSL_SESSION *session = 0;
    VSTRING *session_data = vstring_alloc(2048);

    /*
     * Prepare the query.
     */
    if (TLScontext->log_mask & TLS_LOG_CACHE)
	/* serverid already contains namaddrport information */
	msg_info("looking for session %s in %s cache",
		 TLScontext->serverid, TLScontext->cache_type);

    /*
     * We only get here if the cache_type is not empty. This code is not
     * called unless caching is enabled and the cache_type is stored in the
     * server SSL context.
     */
    if (TLScontext->cache_type == 0)
	msg_panic("%s: null client session cache type in session lookup",
		  myname);

    /*
     * Look up and activate the SSL_SESSION object. Errors are non-fatal,
     * since caching is only an optimization.
     */
    if (tls_mgr_lookup(TLScontext->cache_type, TLScontext->serverid,
		       session_data) == TLS_MGR_STAT_OK) {
	session = tls_session_activate(STR(session_data), LEN(session_data));
	if (session) {
	    if (TLScontext->log_mask & TLS_LOG_CACHE)
		/* serverid already contains namaddrport information */
		msg_info("reloaded session %s from %s cache",
			 TLScontext->serverid, TLScontext->cache_type);
	}
    }

    /*
     * Clean up.
     */
    vstring_free(session_data);

    return (session);
}

/* new_client_session_cb - name new session and save it to client cache */

static int new_client_session_cb(SSL *ssl, SSL_SESSION *session)
{
    const char *myname = "new_client_session_cb";
    TLS_SESS_STATE *TLScontext;
    VSTRING *session_data;

    /*
     * The cache name (if caching is enabled in tlsmgr(8)) and the cache ID
     * string for this session are stored in the TLScontext. It cannot be
     * null at this point.
     */
    if ((TLScontext = SSL_get_ex_data(ssl, TLScontext_index)) == 0)
	msg_panic("%s: null TLScontext in new session callback", myname);

    /*
     * We only get here if the cache_type is not empty. This callback is not
     * set unless caching is enabled and the cache_type is stored in the
     * server SSL context.
     */
    if (TLScontext->cache_type == 0)
	msg_panic("%s: null session cache type in new session callback",
		  myname);

    if (TLScontext->log_mask & TLS_LOG_CACHE)
	/* serverid already contains namaddrport information */
	msg_info("save session %s to %s cache",
		 TLScontext->serverid, TLScontext->cache_type);

#if (OPENSSL_VERSION_NUMBER < 0x00906011L) || (OPENSSL_VERSION_NUMBER == 0x00907000L)

    /*
     * Ugly Hack: OpenSSL before 0.9.6a does not store the verify result in
     * sessions for the client side. We modify the session directly which is
     * version specific, but this bug is version specific, too.
     * 
     * READ: 0-09-06-01-1 = 0-9-6-a-beta1: all versions before beta1 have this
     * bug, it has been fixed during development of 0.9.6a. The development
     * version of 0.9.7 can have this bug, too. It has been fixed on
     * 2000/11/29.
     */
    session->verify_result = SSL_get_verify_result(TLScontext->con);
#endif

    /*
     * Passivate and save the session object. Errors are non-fatal, since
     * caching is only an optimization.
     */
    if ((session_data = tls_session_passivate(session)) != 0) {
	tls_mgr_update(TLScontext->cache_type, TLScontext->serverid,
		       STR(session_data), LEN(session_data));
	vstring_free(session_data);
    }

    /*
     * Clean up.
     */
    SSL_SESSION_free(session);			/* 200502 */

    return (1);
}

/* uncache_session - remove session from the external cache */

static void uncache_session(SSL_CTX *ctx, TLS_SESS_STATE *TLScontext)
{
    SSL_SESSION *session = SSL_get_session(TLScontext->con);

    SSL_CTX_remove_session(ctx, session);
    if (TLScontext->cache_type == 0 || TLScontext->serverid == 0)
	return;

    if (TLScontext->log_mask & TLS_LOG_CACHE)
	/* serverid already contains namaddrport information */
	msg_info("remove session %s from client cache", TLScontext->serverid);

    tls_mgr_delete(TLScontext->cache_type, TLScontext->serverid);
}

/* tls_client_init - initialize client-side TLS engine */

TLS_APPL_STATE *tls_client_init(const TLS_CLIENT_INIT_PROPS *props)
{
    long    off = 0;
    int     cachable;
    SSL_CTX *client_ctx;
    TLS_APPL_STATE *app_ctx;
    const EVP_MD *md_alg;
    unsigned int md_len;
    int     log_mask;

    /*
     * Convert user loglevel to internal logmask.
     */
    log_mask = tls_log_mask(props->log_param, props->log_level);

    if (log_mask & TLS_LOG_VERBOSE)
	msg_info("initializing the client-side TLS engine");

    /*
     * Load (mostly cipher related) TLS-library internal main.cf parameters.
     */
    tls_param_init();

    /*
     * Detect mismatch between compile-time headers and run-time library.
     */
    tls_check_version();

    /*
     * Initialize the OpenSSL library by the book! To start with, we must
     * initialize the algorithms. We want cleartext error messages instead of
     * just error codes, so we load the error_strings.
     */
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    /*
     * Create an application data index for SSL objects, so that we can
     * attach TLScontext information; this information is needed inside
     * tls_verify_certificate_callback().
     */
    if (TLScontext_index < 0) {
	if ((TLScontext_index = SSL_get_ex_new_index(0, 0, 0, 0, 0)) < 0) {
	    msg_warn("Cannot allocate SSL application data index: "
		     "disabling TLS support");
	    return (0);
	}
    }

    /*
     * Register SHA-2 digests, if implemented and not already registered.
     * Improves interoperability with clients and servers that prematurely
     * deploy SHA-2 certificates.
     */
#if defined(LN_sha256) && defined(NID_sha256) && !defined(OPENSSL_NO_SHA256)
    if (!EVP_get_digestbyname(LN_sha224))
	EVP_add_digest(EVP_sha224());
    if (!EVP_get_digestbyname(LN_sha256))
	EVP_add_digest(EVP_sha256());
#endif
#if defined(LN_sha512) && defined(NID_sha512) && !defined(OPENSSL_NO_SHA512)
    if (!EVP_get_digestbyname(LN_sha384))
	EVP_add_digest(EVP_sha384());
    if (!EVP_get_digestbyname(LN_sha512))
	EVP_add_digest(EVP_sha512());
#endif

    /*
     * If the administrator specifies an unsupported digest algorithm, fail
     * now, rather than in the middle of a TLS handshake.
     */
    if ((md_alg = EVP_get_digestbyname(props->fpt_dgst)) == 0) {
	msg_warn("Digest algorithm \"%s\" not found: disabling TLS support",
		 props->fpt_dgst);
	return (0);
    }

    /*
     * Sanity check: Newer shared libraries may use larger digests.
     */
    if ((md_len = EVP_MD_size(md_alg)) > EVP_MAX_MD_SIZE) {
	msg_warn("Digest algorithm \"%s\" output size %u too large:"
		 " disabling TLS support", props->fpt_dgst, md_len);
	return (0);
    }

    /*
     * Initialize the PRNG (Pseudo Random Number Generator) with some seed
     * from external and internal sources. Don't enable TLS without some real
     * entropy.
     */
    if (tls_ext_seed(var_tls_daemon_rand_bytes) < 0) {
	msg_warn("no entropy for TLS key generation: disabling TLS support");
	return (0);
    }
    tls_int_seed();

    /*
     * The SSL/TLS specifications require the client to send a message in the
     * oldest specification it understands with the highest level it
     * understands in the message. RFC2487 is only specified for TLSv1, but
     * we want to be as compatible as possible, so we will start off with a
     * SSLv2 greeting allowing the best we can offer: TLSv1. We can restrict
     * this with the options setting later, anyhow.
     */
    ERR_clear_error();
    if ((client_ctx = SSL_CTX_new(SSLv23_client_method())) == 0) {
	msg_warn("cannot allocate client SSL_CTX: disabling TLS support");
	tls_print_errors();
	return (0);
    }

    /*
     * See the verify callback in tls_verify.c
     */
    SSL_CTX_set_verify_depth(client_ctx, props->verifydepth + 1);

    /*
     * Protocol selection is destination dependent, so we delay the protocol
     * selection options to the per-session SSL object.
     */
    off |= tls_bug_bits();
    SSL_CTX_set_options(client_ctx, off);

    /*
     * Set the call-back routine for verbose logging.
     */
    if (log_mask & TLS_LOG_DEBUG)
	SSL_CTX_set_info_callback(client_ctx, tls_info_callback);

    /*
     * Load the CA public key certificates for both the client cert and for
     * the verification of server certificates. As provided by OpenSSL we
     * support two types of CA certificate handling: One possibility is to
     * add all CA certificates to one large CAfile, the other possibility is
     * a directory pointed to by CApath, containing separate files for each
     * CA with softlinks named after the hash values of the certificate. The
     * first alternative has the advantage that the file is opened and read
     * at startup time, so that you don't have the hassle to maintain another
     * copy of the CApath directory for chroot-jail.
     */
    if (tls_set_ca_certificate_info(client_ctx,
				    props->CAfile, props->CApath) < 0) {
	/* tls_set_ca_certificate_info() already logs a warning. */
	SSL_CTX_free(client_ctx);		/* 200411 */
	return (0);
    }

    /*
     * We do not need a client certificate, so the certificates are only
     * loaded (and checked) if supplied. A clever client would handle
     * multiple client certificates and decide based on the list of
     * acceptable CAs, sent by the server, which certificate to submit.
     * OpenSSL does however not do this and also has no call-back hooks to
     * easily implement it.
     * 
     * Load the client public key certificate and private key from file and
     * check whether the cert matches the key. We can use RSA certificates
     * ("cert") DSA certificates ("dcert") or ECDSA certificates ("eccert").
     * All three can be made available at the same time. The CA certificates
     * for all three are handled in the same setup already finished. Which
     * one is used depends on the cipher negotiated (that is: the first
     * cipher listed by the client which does match the server). The client
     * certificate is presented after the server chooses the session cipher,
     * so we will just present the right cert for the chosen cipher (if it
     * uses certificates).
     */
    if (tls_set_my_certificate_key_info(client_ctx,
					props->cert_file,
					props->key_file,
					props->dcert_file,
					props->dkey_file,
					props->eccert_file,
					props->eckey_file) < 0) {
	/* tls_set_my_certificate_key_info() already logs a warning. */
	SSL_CTX_free(client_ctx);		/* 200411 */
	return (0);
    }

    /*
     * According to the OpenSSL documentation, temporary RSA key is needed
     * export ciphers are in use. We have to provide one, so well, we just do
     * it.
     */
    SSL_CTX_set_tmp_rsa_callback(client_ctx, tls_tmp_rsa_cb);

    /*
     * Finally, the setup for the server certificate checking, done "by the
     * book".
     */
    SSL_CTX_set_verify(client_ctx, SSL_VERIFY_NONE,
		       tls_verify_certificate_callback);

    /*
     * Initialize the session cache.
     * 
     * Since the client does not search an internal cache, we simply disable it.
     * It is only useful for expiring old sessions, but we do that in the
     * tlsmgr(8).
     * 
     * This makes SSL_CTX_remove_session() not useful for flushing broken
     * sessions from the external cache, so we must delete them directly (not
     * via a callback).
     */
    if (tls_mgr_policy(props->cache_type, &cachable) != TLS_MGR_STAT_OK)
	cachable = 0;

    /*
     * Allocate an application context, and populate with mandatory protocol
     * and cipher data.
     */
    app_ctx = tls_alloc_app_context(client_ctx, log_mask);

    /*
     * The external session cache is implemented by the tlsmgr(8) process.
     */
    if (cachable) {

	app_ctx->cache_type = mystrdup(props->cache_type);

	/*
	 * OpenSSL does not use callbacks to load sessions from a client
	 * cache, so we must invoke that function directly. Apparently,
	 * OpenSSL does not provide a way to pass session names from here to
	 * call-back routines that do session lookup.
	 * 
	 * OpenSSL can, however, automatically save newly created sessions for
	 * us by callback (we create the session name in the call-back
	 * function).
	 * 
	 * XXX gcc 2.95 can't compile #ifdef .. #endif in the expansion of
	 * SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_INTERNAL_STORE |
	 * SSL_SESS_CACHE_NO_AUTO_CLEAR.
	 */
#ifndef SSL_SESS_CACHE_NO_INTERNAL_STORE
#define SSL_SESS_CACHE_NO_INTERNAL_STORE 0
#endif

	SSL_CTX_set_session_cache_mode(client_ctx,
				       SSL_SESS_CACHE_CLIENT |
				       SSL_SESS_CACHE_NO_INTERNAL_STORE |
				       SSL_SESS_CACHE_NO_AUTO_CLEAR);
	SSL_CTX_sess_set_new_cb(client_ctx, new_client_session_cb);
    }
    return (app_ctx);
}

/* match_hostname -  match hostname against pattern */

static int match_hostname(const char *peerid,
			          const TLS_CLIENT_START_PROPS *props)
{
    const ARGV *cmatch_argv;
    const char *nexthop = props->nexthop;
    const char *hname = props->host;
    const char *pattern;
    const char *pattern_left;
    int     sub;
    int     i;
    int     idlen;
    int     patlen;

    if ((cmatch_argv = props->matchargv) == 0)
	return 0;

    /*
     * Match the peerid against each pattern until we find a match.
     */
    for (i = 0; i < cmatch_argv->argc; ++i) {
	sub = 0;
	if (!strcasecmp(cmatch_argv->argv[i], "nexthop"))
	    pattern = nexthop;
	else if (!strcasecmp(cmatch_argv->argv[i], "hostname"))
	    pattern = hname;
	else if (!strcasecmp(cmatch_argv->argv[i], "dot-nexthop")) {
	    pattern = nexthop;
	    sub = 1;
	} else {
	    pattern = cmatch_argv->argv[i];
	    if (*pattern == '.' && pattern[1] != '\0') {
		++pattern;
		sub = 1;
	    }
	}

	/*
	 * Sub-domain match: peerid is any sub-domain of pattern.
	 */
	if (sub) {
	    if ((idlen = strlen(peerid)) > (patlen = strlen(pattern)) + 1
		&& peerid[idlen - patlen - 1] == '.'
		&& !strcasecmp(peerid + (idlen - patlen), pattern))
		return (1);
	    else
		continue;
	}

	/*
	 * Exact match and initial "*" match. The initial "*" in a peerid
	 * matches exactly one hostname component, under the condition that
	 * the peerid contains multiple hostname components.
	 */
	if (!strcasecmp(peerid, pattern)
	    || (peerid[0] == '*' && peerid[1] == '.' && peerid[2] != 0
		&& (pattern_left = strchr(pattern, '.')) != 0
		&& strcasecmp(pattern_left + 1, peerid + 2) == 0))
	    return (1);
    }
    return (0);
}

/* verify_extract_name - verify peer name and extract peer information */

static void verify_extract_name(TLS_SESS_STATE *TLScontext, X509 *peercert,
				        const TLS_CLIENT_START_PROPS *props)
{
    int     i;
    int     r;
    int     matched = 0;
    int     dnsname_match;
    int     verify_peername = 0;
    int     log_certmatch;
    int     verbose;
    const char *dnsname;
    const GENERAL_NAME *gn;

    STACK_OF(GENERAL_NAME) * gens;

    /*
     * On exit both peer_CN and issuer_CN should be set.
     */
    TLScontext->issuer_CN = tls_issuer_CN(peercert, TLScontext);

    /*
     * Is the certificate trust chain valid and trusted?
     */
    if (SSL_get_verify_result(TLScontext->con) == X509_V_OK)
	TLScontext->peer_status |= TLS_CERT_FLAG_TRUSTED;

    if (TLS_CERT_IS_TRUSTED(TLScontext) && props->tls_level >= TLS_LEV_VERIFY)
	verify_peername = 1;

    /* Force cert processing so we can log the data? */
    log_certmatch = TLScontext->log_mask & TLS_LOG_CERTMATCH;

    /* Log cert details when processing? */
    verbose = log_certmatch || (TLScontext->log_mask & TLS_LOG_VERBOSE);

    if (verify_peername || log_certmatch) {

	/*
	 * Verify the dNSName(s) in the peer certificate against the nexthop
	 * and hostname.
	 * 
	 * If DNS names are present, we use the first matching (or else simply
	 * the first) DNS name as the subject CN. The CommonName in the
	 * issuer DN is obsolete when SubjectAltName is available. This
	 * yields much less surprising logs, because we log the name we
	 * verified or a name we checked and failed to match.
	 * 
	 * XXX: The nexthop and host name may both be the same network address
	 * rather than a DNS name. In this case we really should be looking
	 * for GEN_IPADD entries, not GEN_DNS entries.
	 * 
	 * XXX: In ideal world the caller who used the address to build the
	 * connection would tell us that the nexthop is the connection
	 * address, but if that is not practical, we can parse the nexthop
	 * again here.
	 */
	gens = X509_get_ext_d2i(peercert, NID_subject_alt_name, 0, 0);
	if (gens) {
	    r = sk_GENERAL_NAME_num(gens);
	    for (i = 0; i < r; ++i) {
		gn = sk_GENERAL_NAME_value(gens, i);
		if (gn->type != GEN_DNS)
		    continue;

		/*
		 * Even if we have an invalid DNS name, we still ultimately
		 * ignore the CommonName, because subjectAltName:DNS is
		 * present (though malformed). Replace any previous peer_CN
		 * if empty or we get a match.
		 * 
		 * We always set at least an empty peer_CN if the ALTNAME cert
		 * flag is set. If not, we set peer_CN from the cert
		 * CommonName below, so peer_CN is always non-null on return.
		 */
		TLScontext->peer_status |= TLS_CERT_FLAG_ALTNAME;
		dnsname = tls_dns_name(gn, TLScontext);
		if (dnsname && *dnsname) {
		    if ((dnsname_match = match_hostname(dnsname, props)) != 0)
			matched++;
		    /* Keep the first matched name. */
		    if (TLScontext->peer_CN
			&& ((dnsname_match && matched == 1)
			    || *TLScontext->peer_CN == 0)) {
			myfree(TLScontext->peer_CN);
			TLScontext->peer_CN = 0;
		    }
		    if (verbose)
			msg_info("%s: %ssubjectAltName: %s", props->namaddr,
				 dnsname_match ? "Matched " : "", dnsname);
		}
		if (TLScontext->peer_CN == 0)
		    TLScontext->peer_CN = mystrdup(dnsname ? dnsname : "");
		if (matched && !log_certmatch)
		    break;
	    }
	    if (verify_peername && matched)
		TLScontext->peer_status |= TLS_CERT_FLAG_MATCHED;

	    /*
	     * (Sam Rushing, Ironport) Free stack *and* member GENERAL_NAME
	     * objects
	     */
	    sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
	}

	/*
	 * No subjectAltNames, peer_CN is taken from CommonName.
	 */
	if (TLScontext->peer_CN == 0) {
	    TLScontext->peer_CN = tls_peer_CN(peercert, TLScontext);
	    if (*TLScontext->peer_CN)
		matched = match_hostname(TLScontext->peer_CN, props);
	    if (verify_peername && matched)
		TLScontext->peer_status |= TLS_CERT_FLAG_MATCHED;
	    if (verbose)
		msg_info("%s %sCommonName %s", props->namaddr,
			 matched ? "Matched " : "", TLScontext->peer_CN);
	} else if (verbose) {
	    char   *tmpcn = tls_peer_CN(peercert, TLScontext);

	    /*
	     * Though the CommonName was superceded by a subjectAltName, log
	     * it when certificate match debugging was requested.
	     */
	    msg_info("%s CommonName %s", TLScontext->namaddr, tmpcn);
	    myfree(tmpcn);
	}
    } else
	TLScontext->peer_CN = tls_peer_CN(peercert, TLScontext);

    /*
     * Give them a clue. Problems with trust chain verification were logged
     * when the session was first negotiated, before the session was stored
     * into the cache. We don't want mystery failures, so log the fact the
     * real problem is to be found in the past.
     */
    if (TLScontext->session_reused
	&& !TLS_CERT_IS_TRUSTED(TLScontext)
	&& (TLScontext->log_mask & TLS_LOG_UNTRUSTED))
	msg_info("%s: re-using session with untrusted certificate, "
		 "look for details earlier in the log", props->namaddr);
}

/* verify_extract_print - extract and verify peer fingerprint */

static void verify_extract_print(TLS_SESS_STATE *TLScontext, X509 *peercert,
				         const TLS_CLIENT_START_PROPS *props)
{
    char  **cpp;

    /* Non-null by contract */
    TLScontext->peer_fingerprint = tls_fingerprint(peercert, props->fpt_dgst);
    TLScontext->peer_pkey_fprint = tls_pkey_fprint(peercert, props->fpt_dgst);

    /*
     * Compare the fingerprint against each acceptable value, ignoring
     * upper/lower case differences.
     */
    if (props->tls_level == TLS_LEV_FPRINT) {
	for (cpp = props->matchargv->argv; *cpp; ++cpp) {
	    if (strcasecmp(TLScontext->peer_fingerprint, *cpp) == 0
		|| strcasecmp(TLScontext->peer_pkey_fprint, *cpp) == 0) {
		TLScontext->peer_status |= TLS_CERT_FLAG_MATCHED;
		break;
	    }
	}
    }
}

 /*
  * This is the actual startup routine for the connection. We expect that the
  * buffers are flushed and the "220 Ready to start TLS" was received by us,
  * so that we can immediately start the TLS handshake process.
  */
TLS_SESS_STATE *tls_client_start(const TLS_CLIENT_START_PROPS *props)
{
    int     sts;
    int     protomask;
    const char *cipher_list;
    SSL_SESSION *session;
    const SSL_CIPHER *cipher;
    X509   *peercert;
    TLS_SESS_STATE *TLScontext;
    TLS_APPL_STATE *app_ctx = props->ctx;
    VSTRING *myserverid;
    int     log_mask = app_ctx->log_mask;

    /*
     * When certificate verification is required, log trust chain validation
     * errors even when disabled by default for opportunistic sessions.
     */
    if (props->tls_level >= TLS_LEV_VERIFY)
	log_mask |= TLS_LOG_UNTRUSTED;

    if (log_mask & TLS_LOG_VERBOSE)
	msg_info("setting up TLS connection to %s", props->namaddr);

    /*
     * First make sure we have valid protocol and cipher parameters
     * 
     * The cipherlist will be applied to the global SSL context, where it can be
     * repeatedly reset if necessary, but the protocol restrictions will be
     * is applied to the SSL connection, because protocol restrictions in the
     * global context cannot be cleared.
     */

    /*
     * OpenSSL will ignore cached sessions that use the wrong protocol. So we
     * do not need to filter out cached sessions with the "wrong" protocol,
     * rather OpenSSL will simply negotiate a new session.
     * 
     * Still, we salt the session lookup key with the protocol list, so that
     * sessions found in the cache are always acceptable.
     */
    protomask = tls_protocol_mask(props->protocols);
    if (protomask == TLS_PROTOCOL_INVALID) {
	/* tls_protocol_mask() logs no warning. */
	msg_warn("%s: Invalid TLS protocol list \"%s\": aborting TLS session",
		 props->namaddr, props->protocols);
	return (0);
    }
    myserverid = vstring_alloc(100);
    vstring_sprintf_append(myserverid, "%s&p=%d", props->serverid, protomask);

    /*
     * Per session cipher selection for sessions with mandatory encryption
     * 
     * By the time a TLS client is negotiating ciphers it has already offered to
     * re-use a session, it is too late to renege on the offer. So we must
     * not attempt to re-use sessions whose ciphers are too weak. We salt the
     * session lookup key with the cipher list, so that sessions found in the
     * cache are always acceptable.
     */
    cipher_list = tls_set_ciphers(app_ctx, "TLS", props->cipher_grade,
				  props->cipher_exclusions);
    if (cipher_list == 0) {
	msg_warn("%s: %s: aborting TLS session",
		 props->namaddr, vstring_str(app_ctx->why));
	vstring_free(myserverid);
	return (0);
    }
    if (log_mask & TLS_LOG_VERBOSE)
	msg_info("%s: TLS cipher list \"%s\"", props->namaddr, cipher_list);
    vstring_sprintf_append(myserverid, "&c=%s", cipher_list);

    /*
     * Finally, salt the session key with the OpenSSL library version,
     * (run-time, rather than compile-time, just in case that matters).
     */
    vstring_sprintf_append(myserverid, "&l=%ld", (long) SSLeay());

    /*
     * Allocate a new TLScontext for the new connection and get an SSL
     * structure. Add the location of TLScontext to the SSL to later retrieve
     * the information inside the tls_verify_certificate_callback().
     * 
     * If session caching was enabled when TLS was initialized, the cache type
     * is stored in the client SSL context.
     */
    TLScontext = tls_alloc_sess_context(log_mask, props->namaddr);
    TLScontext->cache_type = app_ctx->cache_type;

    TLScontext->serverid = vstring_export(myserverid);
    TLScontext->stream = props->stream;

    if ((TLScontext->con = SSL_new(app_ctx->ssl_ctx)) == NULL) {
	msg_warn("Could not allocate 'TLScontext->con' with SSL_new()");
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }
    if (!SSL_set_ex_data(TLScontext->con, TLScontext_index, TLScontext)) {
	msg_warn("Could not set application data for 'TLScontext->con'");
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * Apply session protocol restrictions.
     */
    if (protomask != 0)
	SSL_set_options(TLScontext->con,
		   ((protomask & TLS_PROTOCOL_TLSv1) ? SSL_OP_NO_TLSv1 : 0L)
	     | ((protomask & TLS_PROTOCOL_TLSv1_1) ? SSL_OP_NO_TLSv1_1 : 0L)
	     | ((protomask & TLS_PROTOCOL_TLSv1_2) ? SSL_OP_NO_TLSv1_2 : 0L)
		 | ((protomask & TLS_PROTOCOL_SSLv3) ? SSL_OP_NO_SSLv3 : 0L)
	       | ((protomask & TLS_PROTOCOL_SSLv2) ? SSL_OP_NO_SSLv2 : 0L));

    /*
     * XXX To avoid memory leaks we must always call SSL_SESSION_free() after
     * calling SSL_set_session(), regardless of whether or not the session
     * will be reused.
     */
    if (TLScontext->cache_type) {
	session = load_clnt_session(TLScontext);
	if (session) {
	    SSL_set_session(TLScontext->con, session);
	    SSL_SESSION_free(session);		/* 200411 */
#if (OPENSSL_VERSION_NUMBER < 0x00906011L) || (OPENSSL_VERSION_NUMBER == 0x00907000L)

	    /*
	     * Ugly Hack: OpenSSL before 0.9.6a does not store the verify
	     * result in sessions for the client side. We modify the session
	     * directly which is version specific, but this bug is version
	     * specific, too.
	     * 
	     * READ: 0-09-06-01-1 = 0-9-6-a-beta1: all versions before beta1
	     * have this bug, it has been fixed during development of 0.9.6a.
	     * The development version of 0.9.7 can have this bug, too. It
	     * has been fixed on 2000/11/29.
	     */
	    SSL_set_verify_result(TLScontext->con, session->verify_result);
#endif

	}
    }

    /*
     * Before really starting anything, try to seed the PRNG a little bit
     * more.
     */
    tls_int_seed();
    (void) tls_ext_seed(var_tls_daemon_rand_bytes);

    /*
     * Initialize the SSL connection to connect state. This should not be
     * necessary anymore since 0.9.3, but the call is still in the library
     * and maintaining compatibility never hurts.
     */
    SSL_set_connect_state(TLScontext->con);

    /*
     * Connect the SSL connection with the network socket.
     */
    if (SSL_set_fd(TLScontext->con, vstream_fileno(props->stream)) != 1) {
	msg_info("SSL_set_fd error to %s", props->namaddr);
	tls_print_errors();
	uncache_session(app_ctx->ssl_ctx, TLScontext);
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * Turn on non-blocking I/O so that we can enforce timeouts on network
     * I/O.
     */
    non_blocking(vstream_fileno(props->stream), NON_BLOCKING);

    /*
     * If the debug level selected is high enough, all of the data is dumped:
     * TLS_LOG_TLSPKTS will dump the SSL negotiation, TLS_LOG_ALLPKTS will
     * dump everything.
     * 
     * We do have an SSL_set_fd() and now suddenly a BIO_ routine is called?
     * Well there is a BIO below the SSL routines that is automatically
     * created for us, so we can use it for debugging purposes.
     */
    if (log_mask & TLS_LOG_TLSPKTS)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), tls_bio_dump_cb);

    /*
     * Start TLS negotiations. This process is a black box that invokes our
     * call-backs for certificate verification.
     * 
     * Error handling: If the SSL handhake fails, we print out an error message
     * and remove all TLS state concerning this session.
     */
    sts = tls_bio_connect(vstream_fileno(props->stream), props->timeout,
			  TLScontext);
    if (sts <= 0) {
	if (ERR_peek_error() != 0) {
	    msg_info("SSL_connect error to %s: %d", props->namaddr, sts);
	    tls_print_errors();
	} else if (errno != 0) {
	    msg_info("SSL_connect error to %s: %m", props->namaddr);
	} else {
	    msg_info("SSL_connect error to %s: lost connection",
		     props->namaddr);
	}
	uncache_session(app_ctx->ssl_ctx, TLScontext);
	tls_free_context(TLScontext);
	return (0);
    }
    /* Turn off packet dump if only dumping the handshake */
    if ((log_mask & TLS_LOG_ALLPKTS) == 0)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), 0);

    /*
     * The caller may want to know if this session was reused or if a new
     * session was negotiated.
     */
    TLScontext->session_reused = SSL_session_reused(TLScontext->con);
    if ((log_mask & TLS_LOG_CACHE) && TLScontext->session_reused)
	msg_info("%s: Reusing old session", TLScontext->namaddr);

    /*
     * Do peername verification if requested and extract useful information
     * from the certificate for later use.
     */
    if ((peercert = SSL_get_peer_certificate(TLScontext->con)) != 0) {
	TLScontext->peer_status |= TLS_CERT_FLAG_PRESENT;

	/*
	 * Peer name or fingerprint verification as requested.
	 * Unconditionally set peer_CN, issuer_CN and peer_fingerprint.
	 */
	verify_extract_name(TLScontext, peercert, props);
	verify_extract_print(TLScontext, peercert, props);

	if (TLScontext->log_mask &
	    (TLS_LOG_CERTMATCH | TLS_LOG_VERBOSE | TLS_LOG_PEERCERT))
	    msg_info("%s: subject_CN=%s, issuer_CN=%s, "
		     "fingerprint=%s, pkey_fingerprint=%s", props->namaddr,
		     TLScontext->peer_CN, TLScontext->issuer_CN,
		     TLScontext->peer_fingerprint,
		     TLScontext->peer_pkey_fprint);
	X509_free(peercert);
    } else {
	TLScontext->issuer_CN = mystrdup("");
	TLScontext->peer_CN = mystrdup("");
	TLScontext->peer_fingerprint = mystrdup("");
	TLScontext->peer_pkey_fprint = mystrdup("");
    }

    /*
     * Finally, collect information about protocol and cipher for logging
     */
    TLScontext->protocol = SSL_get_version(TLScontext->con);
    cipher = SSL_get_current_cipher(TLScontext->con);
    TLScontext->cipher_name = SSL_CIPHER_get_name(cipher);
    TLScontext->cipher_usebits = SSL_CIPHER_get_bits(cipher,
					     &(TLScontext->cipher_algbits));

    /*
     * The TLS engine is active. Switch to the tls_timed_read/write()
     * functions and make the TLScontext available to those functions.
     */
    tls_stream_start(props->stream, TLScontext);

    /*
     * All the key facts in a single log entry.
     */
    if (log_mask & TLS_LOG_SUMMARY)
	msg_info("%s TLS connection established to %s: %s with cipher %s "
	      "(%d/%d bits)", TLS_CERT_IS_MATCHED(TLScontext) ? "Verified" :
		 TLS_CERT_IS_TRUSTED(TLScontext) ? "Trusted" : "Untrusted",
	      props->namaddr, TLScontext->protocol, TLScontext->cipher_name,
		 TLScontext->cipher_usebits, TLScontext->cipher_algbits);

    tls_int_seed();

    return (TLScontext);
}

#endif					/* USE_TLS */
