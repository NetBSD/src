/*	$NetBSD: tls_client.c,v 1.1.1.4 2007/05/19 16:28:30 heas Exp $	*/

/*++
/* NAME
/*	tls
/* SUMMARY
/*	client-side TLS engine
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	SSL_CTX	*tls_client_init(init_props)
/*	const tls_client_init_props *init_props;
/*
/*	TLScontext_t *tls_client_start(start_props)
/*	const tls_client_start_props *start_props;
/*
/*	void	tls_client_stop(client_ctx, stream, failure, TLScontext)
/*	SSL_CTX	*client_ctx;
/*	VSTREAM	*stream;
/*	int	failure;
/*	TLScontext_t *TLScontext;
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
/*	so that peer-specific behavior is not possible.
/*
/*	tls_client_start() activates the TLS session over an established
/*	stream. We expect that network buffers are flushed and
/*	the TLS handshake can begin immediately.
/*
/*	tls_client_stop() sends the "close notify" alert via
/*	SSL_shutdown() to the peer and resets all connection specific
/*	TLS data. As RFC2487 does not specify a separate shutdown, it
/*	is assumed that the underlying TCP connection is shut down
/*	immediately afterwards, so we don't care about additional data
/*	coming through the channel.
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
/*	The status of the peer certificate verification is available in
/*	TLScontext->peer_verified. It is set to 1 when the certificate could
/*	be verified.
/*	If the peer offered a certificate, part of the certificate data are
/*	available as:
/* .IP TLScontext->peer_CN
/*	Extracted CommonName of the peer, or zero-length string if the
/*	information could not be extracted.
/* .IP TLScontext->issuer_CN
/*	extracted CommonName of the issuer, or zero-length string if the
/*	information could not be extracted.
/* .PP
/*	Otherwise these fields are set to null pointers.
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

#include <argv.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <msg.h>

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

static SSL_SESSION *load_clnt_session(TLScontext_t *TLScontext)
{
    const char *myname = "load_clnt_session";
    SSL_SESSION *session = 0;
    VSTRING *session_data = vstring_alloc(2048);

    /*
     * Prepare the query.
     */
    if (TLScontext->log_level >= 2)
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
	    if (TLScontext->log_level >= 2)
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
    TLScontext_t *TLScontext;
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

    if (TLScontext->log_level >= 2)
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

static void uncache_session(TLScontext_t *TLScontext)
{
    if (TLScontext->cache_type == 0 || TLScontext->serverid == 0)
	return;

    if (TLScontext->log_level >= 2)
	msg_info("remove session %s from client cache", TLScontext->serverid);

    tls_mgr_delete(TLScontext->cache_type, TLScontext->serverid);
}

/* tls_client_init - initialize client-side TLS engine */

SSL_CTX *tls_client_init(const tls_client_init_props *props)
{
    long    off = 0;
    SSL_CTX *client_ctx;
    int     cachable;

    /* See skeleton in OpenSSL apps/s_client.c. */

    if (props->log_level >= 2)
	msg_info("initializing the client-side TLS engine");

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
    client_ctx = SSL_CTX_new(SSLv23_client_method());
    if (client_ctx == NULL) {
	tls_print_errors();
	return (0);
    }

    /*
     * Protocol selection is destination dependent, so we delay the protocol
     * selection options to the per-session SSL object.
     */
    off |= tls_bug_bits();
    SSL_CTX_set_options(client_ctx, off);

    /*
     * Set the call-back routine for verbose logging.
     */
    if (props->log_level >= 2)
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
     * ("cert") and DSA certificates ("dcert"), both can be made available at
     * the same time. The CA certificates for both are handled in the same
     * setup already finished. Which one is used depends on the cipher
     * negotiated (that is: the first cipher listed by the client which does
     * match the server). A client with RSA only (e.g. Netscape) will use the
     * RSA certificate only. A client with openssl-library will use RSA first
     * if not especially changed in the cipher setup.
     */
    if ((*props->cert_file != 0 || *props->dcert_file != 0)
	&& tls_set_my_certificate_key_info(client_ctx, props->cert_file,
					 props->key_file, props->dcert_file,
					   props->dkey_file) < 0) {
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

    if (TLSscache_index < 0)
	TLSscache_index =
	    SSL_CTX_get_ex_new_index(0, "TLScontext ex_data index",
				     NULL, NULL, NULL);

    if (tls_mgr_policy(props->cache_type, &cachable) != TLS_MGR_STAT_OK)
	cachable = 0;

    if (!SSL_CTX_set_ex_data(client_ctx, TLSscache_index,
			     (void *) props->cache_type)) {
	msg_warn("Session cache off: error saving cache type in SSL context.");
	tls_print_errors();
	cachable = 0;
    }

    /*
     * The external session cache is implemented by the tlsmgr(8) process.
     */
    if (cachable) {

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

    /*
     * Create a global index so that we can attach TLScontext information to
     * SSL objects; this information is needed inside
     * tls_verify_certificate_callback().
     */
    if (TLScontext_index < 0)
	TLScontext_index = SSL_get_ex_new_index(0, "TLScontext ex_data index",
						NULL, NULL, NULL);
    return (client_ctx);
}

/* match_hostname -  match hostname against pattern */

static int match_hostname(const char *peerid, ARGV *cmatch_argv,
			          const char *nexthop, const char *hname)
{
    const char *pattern;
    const char *pattern_left;
    int     sub;
    int     i;
    int     idlen;
    int     patlen;

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

/* verify_extract_peer - verify peer name and extract peer information */

static void verify_extract_peer(const char *nexthop, const char *hname,
			              const char *certmatch, X509 *peercert,
				        TLScontext_t *TLScontext)
{
    int     i;
    int     r;
    int     hostname_matched = 0;
    int     dNSName_found = 0;
    int     verify_peername;
    ARGV   *cmatch_argv = 0;

    STACK_OF(GENERAL_NAME) * gens;

    TLScontext->peer_verified =
	(SSL_get_verify_result(TLScontext->con) == X509_V_OK);

    verify_peername =
	(TLScontext->enforce_CN != 0 && TLScontext->peer_verified != 0);

    if (verify_peername) {
	cmatch_argv = argv_split(certmatch, "\t\n\r, :");

	/*
	 * Verify the dNSName(s) in the peer certificate against the
	 * peername.
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
	    for (i = 0, r = sk_GENERAL_NAME_num(gens); i < r; ++i) {
		const GENERAL_NAME *gn = sk_GENERAL_NAME_value(gens, i);

		if (gn->type == GEN_DNS) {
		    dNSName_found++;
		    if ((hostname_matched =
		       match_hostname((char *) gn->d.ia5->data, cmatch_argv,
				      nexthop, hname)) != 0)
			break;
		}
	    }
	    sk_GENERAL_NAME_pop_free(gens, GENERAL_NAME_free);
	}
    }
    if (dNSName_found) {
	if (!hostname_matched)
	    msg_info("certificate peer name verification failed for "
		     "%s: %d dNSNames in certificate found, "
		     "but none match", hname, dNSName_found);
    }
    if ((TLScontext->peer_CN = tls_peer_CN(peercert)) == 0)
	TLScontext->peer_CN = mystrdup("");

    if ((TLScontext->issuer_CN = tls_issuer_CN(peercert)) == 0)
	TLScontext->issuer_CN = mystrdup("");

    if (!dNSName_found && verify_peername) {

	/*
	 * Verify the CommonName in the peer certificate against the
	 * peername.
	 */
	if (TLScontext->peer_CN[0] != '\0') {
	    hostname_matched = match_hostname(TLScontext->peer_CN, cmatch_argv,
					      nexthop, hname);
	    if (!hostname_matched)
		msg_info("certificate peer name verification failed "
			 "for nexthop=%s, host=%s: CommonName mis-match: %s",
			 nexthop, hname, TLScontext->peer_CN);
	}
    }
    if (cmatch_argv)
	cmatch_argv = argv_free(cmatch_argv);
    TLScontext->hostname_matched = hostname_matched;

    if (TLScontext->log_level >= 1) {
	if (TLScontext->peer_verified
	    && (!TLScontext->enforce_CN || TLScontext->hostname_matched))
	    msg_info("Verified: subject_CN=%s, issuer=%s",
		     TLScontext->peer_CN, TLScontext->issuer_CN);
	else
	    msg_info("Unverified: subject_CN=%s, issuer=%s",
		     TLScontext->peer_CN, TLScontext->issuer_CN);
    }
}

 /*
  * This is the actual startup routine for the connection. We expect that the
  * buffers are flushed and the "220 Ready to start TLS" was received by us,
  * so that we can immediately start the TLS handshake process.
  */
TLScontext_t *tls_client_start(const tls_client_start_props *props)
{
    int     sts;
    SSL_SESSION *session;
    SSL_CIPHER *cipher;
    X509   *peercert;
    TLScontext_t *TLScontext;

    if (props->log_level >= 1)
	msg_info("setting up TLS connection to %s", props->host);

    /*
     * Before we create an SSL, update the SSL_CTX cipherlist if necessary.
     */
    if (tls_set_cipher_list(props->ctx, props->cipherlist) == 0) {
	msg_warn("Invalid cipherlist \"%s\": aborting TLS session",
		 props->cipherlist);
	return (0);
    }

    /*
     * Allocate a new TLScontext for the new connection and get an SSL
     * structure. Add the location of TLScontext to the SSL to later retrieve
     * the information inside the tls_verify_certificate_callback().
     * 
     * If session caching was enabled when TLS was initialized, the cache type
     * is stored in the client SSL context.
     */
    TLScontext = tls_alloc_context(props->log_level, props->host);
    TLScontext->serverid = mystrdup(props->serverid);
    TLScontext->cache_type = SSL_CTX_get_ex_data(props->ctx, TLSscache_index);

    if ((TLScontext->con = (SSL *) SSL_new(props->ctx)) == NULL) {
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
     * Set the verification parameters to be checked in
     * tls_verify_certificate_callback().
     */
    if (props->tls_level >= TLS_LEV_VERIFY) {
	TLScontext->enforce_verify_errors = 1;
	TLScontext->enforce_CN = 1;
	SSL_set_verify(TLScontext->con, SSL_VERIFY_PEER,
		       tls_verify_certificate_callback);
    } else {
	TLScontext->enforce_verify_errors = 0;
	TLScontext->enforce_CN = 0;
    }
    TLScontext->hostname_matched = 0;

    /*
     * The TLS connection is realized by a BIO_pair, so obtain the pair.
     * 
     * XXX There is no need to make internal_bio a member of the TLScontext
     * structure. It will be attached to TLScontext->con, and destroyed along
     * with it. The network_bio, however, needs to be freed explicitly.
     */
    if (!BIO_new_bio_pair(&TLScontext->internal_bio, TLS_BIO_BUFSIZE,
			  &TLScontext->network_bio, TLS_BIO_BUFSIZE)) {
	msg_warn("Could not obtain BIO_pair");
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }
#define DISABLE_ALL (SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1)

    /*
     * Per session protocol selection for sessions with mandatory encryption
     * 
     * OpenSSL will ignore cached sessions that use the wrong protocol. So we do
     * not need to filter out cached sessions with the "wrong" protocol,
     * rather OpenSSL will simply negotiate a new session.
     * 
     * Still, we expect the caller to salt the session lookup key with the
     * protocol list, so that sessions found in the cache are always
     * acceptable.
     * 
     * Not enabling any protocols explicitly, enables all.
     */
    if (props->tls_level >= TLS_LEV_ENCRYPT
	&& props->protocols != 0 && props->protocols != TLS_ALL_PROTOCOLS) {
	long    disable = DISABLE_ALL;

	if (props->protocols & TLS_PROTOCOL_TLSv1)
	    disable &= ~SSL_OP_NO_TLSv1;
	if (props->protocols & TLS_PROTOCOL_SSLv3)
	    disable &= ~SSL_OP_NO_SSLv3;
	if (props->protocols & TLS_PROTOCOL_SSLv2)
	    disable &= ~SSL_OP_NO_SSLv2;

	SSL_set_options(TLScontext->con, disable);
    }

    /*
     * Try to load an existing session from the TLS session cache.
     * 
     * By the time a TLS client is negotiating ciphers it has already offered to
     * re-use a session, it is too late to renege on the offer. So we must
     * not attempt to re-use sessions whose ciphers are too weak. We expect
     * the caller to salt the session lookup key with the cipher list, so
     * that sessions found in the cache are always acceptable.
     * 
     * XXX To avoid memory leaks we must always call SSL_SESSION_free() after
     * calling SSL_set_session(), regardless of whether or not the session
     * will be reused.
     * 
     * XXX: We rely on the client including any non-default cipherlist in the
     * serverid cache lookup key so as to avoid fetching sessions with
     * incompatible ciphers. We could save the cipher name into the cache
     * together with the serialized session, and compare it against the
     * cipherlist here, but this is unlikely to be worth the trouble. A sane
     * administrator should use at most a handful of cipherlists, especially
     * when setting policy for domains served by a common MX host.
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
     * Connect the SSL connection with the Postfix side of the BIO-pair for
     * reading and writing.
     */
    SSL_set_bio(TLScontext->con, TLScontext->internal_bio,
		TLScontext->internal_bio);

    /*
     * If the debug level selected is high enough, all of the data is dumped:
     * 3 will dump the SSL negotiation, 4 will dump everything.
     * 
     * We do have an SSL_set_fd() and now suddenly a BIO_ routine is called?
     * Well there is a BIO below the SSL routines that is automatically
     * created for us, so we can use it for debugging purposes.
     */
    if (props->log_level >= 3)
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
	msg_info("SSL_connect error to %s: %d", props->host, sts);
	tls_print_errors();
	uncache_session(TLScontext);
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * The TLS engine is active. Switch to the tls_timed_read/write()
     * functions and make the TLScontext available to those functions.
     */
    tls_stream_start(props->stream, TLScontext);

    /* Only log_level==4 dumps everything */
    if (props->log_level < 4)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), 0);

    /*
     * The caller may want to know if this session was reused or if a new
     * session was negotiated.
     */
    TLScontext->session_reused = SSL_session_reused(TLScontext->con);
    if (props->log_level >= 2 && TLScontext->session_reused)
	msg_info("Reusing old session");

    /*
     * Do peername verification if requested and extract useful information
     * from the certificate for later use.
     */
    if ((peercert = SSL_get_peer_certificate(TLScontext->con)) != 0) {
	verify_extract_peer(props->nexthop, props->host, props->certmatch,
			    peercert, TLScontext);
	X509_free(peercert);
    }
    if (TLScontext->enforce_CN && !TLScontext->hostname_matched) {
	msg_info("Server certificate could not be verified for %s:"
		 " hostname mismatch", props->host);
	tls_client_stop(props->ctx, props->stream, props->timeout, 0,
			TLScontext);
	return (0);
    }

    /*
     * Finally, collect information about protocol and cipher for logging
     */
    TLScontext->protocol = SSL_get_version(TLScontext->con);
    cipher = SSL_get_current_cipher(TLScontext->con);
    TLScontext->cipher_name = SSL_CIPHER_get_name(cipher);
    TLScontext->cipher_usebits = SSL_CIPHER_get_bits(cipher,
					     &(TLScontext->cipher_algbits));

    if (props->log_level >= 1)
	msg_info("TLS connection established to %s: %s with cipher %s"
		 " (%d/%d bits)", props->host,
		 TLScontext->protocol, TLScontext->cipher_name,
		 TLScontext->cipher_usebits, TLScontext->cipher_algbits);

    tls_int_seed();

    return (TLScontext);
}

#endif					/* USE_TLS */
