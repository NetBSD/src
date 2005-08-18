/*	$NetBSD: tls_client.c,v 1.1.1.1 2005/08/18 21:11:04 rpaulo Exp $	*/

/*++
/* NAME
/*	tls
/* SUMMARY
/*	client-side TLS engine
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	SSL_CTX	*tls_client_init(verifydepth)
/*	int	verifydepth; /* unused */
/*
/*	TLScontext_t *tls_client_start(client_ctx, stream, timeout, 
/*					enforce_peername, peername, 
/*					serverid, tls_info)
/*	SSL_CTX	*client_ctx;
/*	VSTREAM	*stream;
/*	int	timeout;
/*	int	enforce_peername;
/*	const char *peername;
/*	const char *serverid;
/*	tls_info_t *tls_info;
/*
/*	void	tls_client_stop(client_ctx, stream, failure, tls_info)
/*	SSL_CTX	*client_ctx;
/*	VSTREAM	*stream;
/*	int	failure;
/*	tls_info_t *tls_info;
/* DESCRIPTION
/*      This module is the interface between Postfix TLS clients
/*	and the OpenSSL library and TLS entropy and cache manager.
/*
/*	tls_client_init() is called once when the SMTP client
/*	initializes.
/*	Certificate details are also decided during this phase,
/*	so that peer-specific behavior is not possible.
/*
/*	tls_client_start() activates the TLS feature for the VSTREAM
/*	passed as argument. We expect that network buffers are flushed and the
/*	TLS handshake can begin	immediately. Information about the peer
/*	is stored into the tls_info structure passed as argument.
/*	The serverid argument specifies a string that hopefully
/*	uniquely identifies a server. It is used as the client
/*	session cache lookup key.
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
/*	state is available via the tls_info structure:
/* .IP tls_info->protocol
/*	the protocol name (SSLv2, SSLv3, TLSv1),
/* .IP tls_info->cipher_name
/*	the cipher name (e.g. RC4/MD5),
/* .IP tls_info->cipher_usebits
/*	the number of bits actually used (e.g. 40),
/* .IP tls_info->cipher_algbits
/*	the number of bits the algorithm is based on (e.g. 128).
/* .PP
/*	The last two values may differ from each other when export-strength
/*	encryption is used.
/*
/*	The status of the peer certificate verification is available in
/*	tls_info->peer_verified. It is set to 1 when the certificate could
/*	be verified.
/*	If the peer offered a certificate, part of the certificate data are
/*	available as:
/* .IP tls_info->peer_subject
/*	X509v3-oneline with the DN of the peer
/* .IP tls_info->peer_CN
/*	extracted CommonName of the peer
/* .IP tls_info->peer_issuer
/*	X509v3-oneline with the DN of the issuer
/* .IP tls_info->issuer_CN
/*	extracted CommonName of the issuer
/* .IP tls_info->peer_fingerprint
/*	fingerprint of the certificate
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

/* Utility library. */

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

 /*
  * To convert binary to fingerprint.
  */
static const char hexcodes[] = "0123456789ABCDEF";

 /*
  * Do or don't we cache client sessions?
  */
static int tls_client_cache = 0;

/* client_verify_callback - certificate verification wrapper */

static int client_verify_callback(int ok, X509_STORE_CTX *ctx)
{
    return (tls_verify_certificate_callback(ok, ctx, TLS_VERIFY_PEERNAME));
}

/* load_clnt_session - load session from client cache (non-callback) */

static SSL_SESSION *load_clnt_session(const char *cache_id,
				              int enforce_peername)
{
    SSL_SESSION *session = 0;
    VSTRING *session_data = vstring_alloc(2048);
    int     flags = 0;

#define TLS_FLAG_ENFORCE_PEERNAME	(1<<0)

    /*
     * Prepare the query.
     */
    if (var_smtp_tls_loglevel >= 3)
	msg_info("looking for session %s in client cache", cache_id);
    if (enforce_peername)
	flags |= TLS_FLAG_ENFORCE_PEERNAME;

    /*
     * Look up and activate the SSL_SESSION object. Errors are non-fatal,
     * since caching is only an optimization.
     */
    if (tls_mgr_lookup(tls_client_cache, cache_id, OPENSSL_VERSION_NUMBER,
		       flags, session_data) == TLS_MGR_STAT_OK) {
	session = tls_session_activate(STR(session_data), LEN(session_data));
	if (session) {
	    if (var_smtp_tls_loglevel >= 3)
		msg_info("reloaded session %s from client cache", cache_id);
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
    TLScontext_t *TLScontext;
    VSTRING *session_data;
    const char *cache_id;
    int     flags = 0;

    /*
     * Look up the cache ID string for this session object.
     */
    TLScontext = SSL_get_ex_data(ssl, TLScontext_index);
    cache_id = TLScontext->serverid;

    if (var_smtp_tls_loglevel >= 3)
	msg_info("save session %s to client cache", cache_id);

    /*
     * Remember whether peername matching was enforced when the session was
     * created. If later enforce mode is enabled, we do not want to reuse a
     * session that was not sufficiently checked.
     */
    if (TLScontext->enforce_verify_errors && TLScontext->enforce_CN)
	flags |= TLS_FLAG_ENFORCE_PEERNAME;

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
    session_data = tls_session_passivate(session);
    if (session_data)
	tls_mgr_update(tls_client_cache, cache_id,
		       OPENSSL_VERSION_NUMBER, flags,
		       STR(session_data), LEN(session_data));

    /*
     * Clean up.
     */
    if (session_data)
	vstring_free(session_data);
    SSL_SESSION_free(session);			/* 200502 */

    return (1);
}

/* tls_client_init - initialize client-side TLS engine */

SSL_CTX *tls_client_init(int unused_verifydepth)
{
    int     off = 0;
    int     verify_flags = SSL_VERIFY_NONE;
    SSL_CTX *client_ctx;
    int     cache_types;

    /* See skeleton in OpenSSL apps/s_client.c. */

    if (var_smtp_tls_loglevel >= 2)
	msg_info("initializing the client-side TLS engine");

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
     * Here we might set SSL_OP_NO_SSLv2, SSL_OP_NO_SSLv3, SSL_OP_NO_TLSv1.
     * Of course, the last one would not make sense, since RFC2487 is only
     * defined for TLS, but we don't know what is out there. So leave things
     * completely open, as of today.
     */
    off |= SSL_OP_ALL;				/* Work around all known bugs */
    SSL_CTX_set_options(client_ctx, off);

    /*
     * Set the call-back routine for verbose logging.
     */
    if (var_smtp_tls_loglevel >= 2)
	SSL_CTX_set_info_callback(client_ctx, tls_info_callback);

    /*
     * Override the default cipher list with our own list.
     */
    if (*var_smtp_tls_cipherlist != 0)
	if (SSL_CTX_set_cipher_list(client_ctx, var_smtp_tls_cipherlist) == 0) {
	    tls_print_errors();
	    SSL_CTX_free(client_ctx);		/* 200411 */
	    return (0);
	}

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
    if (tls_set_ca_certificate_info(client_ctx, var_smtp_tls_CAfile,
				    var_smtp_tls_CApath) < 0) {
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
    if ((*var_smtp_tls_cert_file != 0 || *var_smtp_tls_dcert_file != 0)
	&& tls_set_my_certificate_key_info(client_ctx,
					   var_smtp_tls_cert_file,
					   var_smtp_tls_key_file,
					   var_smtp_tls_dcert_file,
					   var_smtp_tls_dkey_file) < 0) {
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
    SSL_CTX_set_verify(client_ctx, verify_flags, client_verify_callback);

    /*
     * Initialize the session cache. In order to share cached sessions among
     * multiple SMTP client processes, we use an external cache and set the
     * internal cache size to a minimum value of 1.
     */
    SSL_CTX_sess_set_cache_size(client_ctx, 1);
    SSL_CTX_set_timeout(client_ctx, var_smtp_tls_scache_timeout);

    /*
     * The session cache is implemented by the tlsmgr(8) process.
     */
    if (tls_mgr_policy(&cache_types) == TLS_MGR_STAT_OK
	&& (tls_client_cache = (cache_types & TLS_MGR_SCACHE_CLIENT)) != 0) {

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
	 * Disable automatic clearing of cache entries, as the client process
	 * has limited lifetime anyway and we can call the cleanup routine
	 * directly.
	 */
	SSL_CTX_set_session_cache_mode(client_ctx,
		      SSL_SESS_CACHE_CLIENT | SSL_SESS_CACHE_NO_AUTO_CLEAR);
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

 /*
  * This is the actual startup routine for the connection. We expect that the
  * buffers are flushed and the "220 Ready to start TLS" was received by us,
  * so that we can immediately start the TLS handshake process.
  */
TLScontext_t *tls_client_start(SSL_CTX *client_ctx, VSTREAM *stream,
			               int timeout,
			               int enforce_peername,
			               const char *peername,
			               const char *serverid,
			               tls_info_t *tls_info)
{
    int     sts;
    SSL_SESSION *session, *old_session;
    SSL_CIPHER *cipher;
    X509   *peer;
    int     verify_flags;
    TLScontext_t *TLScontext;

    if (var_smtp_tls_loglevel >= 1)
	msg_info("setting up TLS connection to %s", peername);

    /*
     * Allocate a new TLScontext for the new connection and get an SSL
     * structure. Add the location of TLScontext to the SSL to later retrieve
     * the information inside the tls_verify_certificate_callback().
     * 
     * XXX Need a dedicated procedure for consistent initialization of all the
     * fields in this structure.
     */
#define PEERNAME_SIZE sizeof(TLScontext->peername_save)

    NEW_TLS_CONTEXT(TLScontext);
    TLScontext->log_level = var_smtp_tls_loglevel;
    strncpy(TLScontext->peername_save, peername, PEERNAME_SIZE - 1);
    TLScontext->peername_save[PEERNAME_SIZE - 1] = 0;
    (void) lowercase(TLScontext->peername_save);
    TLScontext->serverid = mystrdup(serverid);

    if ((TLScontext->con = (SSL *) SSL_new(client_ctx)) == NULL) {
	msg_info("Could not allocate 'TLScontext->con' with SSL_new()");
	tls_print_errors();
	FREE_TLS_CONTEXT(TLScontext);
	return (0);
    }
    if (!SSL_set_ex_data(TLScontext->con, TLScontext_index, TLScontext)) {
	msg_info("Could not set application data for 'TLScontext->con'");
	tls_print_errors();
	SSL_free(TLScontext->con);
	FREE_TLS_CONTEXT(TLScontext);
	return (0);
    }

    /*
     * Set the verification parameters to be checked in
     * tls_verify_certificate_callback().
     */
    if (enforce_peername) {
	verify_flags = SSL_VERIFY_PEER;
	TLScontext->enforce_verify_errors = 1;
	TLScontext->enforce_CN = 1;
	SSL_set_verify(TLScontext->con, verify_flags, client_verify_callback);
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
	msg_info("Could not obtain BIO_pair");
	tls_print_errors();
	SSL_free(TLScontext->con);
	FREE_TLS_CONTEXT(TLScontext);
	return (0);
    }
    old_session = NULL;

    /*
     * Try to load an existing session from the TLS session cache.
     * 
     * XXX To avoid memory leaks we must always call SSL_SESSION_free() after
     * calling SSL_set_session(), regardless of whether or not the session
     * will be reused.
     */
    if (tls_client_cache) {
	old_session = load_clnt_session(serverid, enforce_peername);
	if (old_session) {
	    SSL_set_session(TLScontext->con, old_session);
	    SSL_SESSION_free(old_session);	/* 200411 */
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
	    SSL_set_verify_result(TLScontext->con, old_session->verify_result);
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
    if (var_smtp_tls_loglevel >= 3)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), tls_bio_dump_cb);

    /*
     * Start TLS negotiations. This process is a black box that invokes our
     * call-backs for certificate verification.
     * 
     * Error handling: If the SSL handhake fails, we print out an error message
     * and remove all TLS state concerning this session.
     */
    sts = tls_bio_connect(vstream_fileno(stream), timeout, TLScontext);
    if (sts <= 0) {
	msg_info("SSL_connect error to %s: %d", peername, sts);
	tls_print_errors();
	session = SSL_get_session(TLScontext->con);
	if (session) {
	    SSL_CTX_remove_session(client_ctx, session);
	    if (var_smtp_tls_loglevel >= 2)
		msg_info("SSL session removed");
	}
	SSL_free(TLScontext->con);
	BIO_free(TLScontext->network_bio);	/* 200411 */
	FREE_TLS_CONTEXT(TLScontext);
	return (0);
    }
    if (var_smtp_tls_loglevel >= 3 && SSL_session_reused(TLScontext->con))
	msg_info("Reusing old session");

    /* Only loglevel==4 dumps everything */
    if (var_smtp_tls_loglevel < 4)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), 0);

    /*
     * Let's see whether a peer certificate is available and what is the
     * actual information. We want to save it for later use.
     */
    peer = SSL_get_peer_certificate(TLScontext->con);
    if (peer != NULL) {
	if (SSL_get_verify_result(TLScontext->con) == X509_V_OK)
	    tls_info->peer_verified = 1;

	tls_info->hostname_matched = TLScontext->hostname_matched;

	TLScontext->peer_CN[0] = '\0';
	if (!X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
				       NID_commonName, TLScontext->peer_CN,
				       sizeof(TLScontext->peer_CN))) {
	    msg_info("Could not parse server's subject CN");
	    tls_print_errors();
	}
	tls_info->peer_CN = TLScontext->peer_CN;

	TLScontext->issuer_CN[0] = '\0';
	if (!X509_NAME_get_text_by_NID(X509_get_issuer_name(peer),
				       NID_commonName, TLScontext->issuer_CN,
				       sizeof(TLScontext->issuer_CN))) {
	    msg_info("Could not parse server's issuer CN");
	    tls_print_errors();
	}
	if (!TLScontext->issuer_CN[0]) {
	    /* No issuer CN field, use Organization instead */
	    if (!X509_NAME_get_text_by_NID(X509_get_issuer_name(peer),
				NID_organizationName, TLScontext->issuer_CN,
					   sizeof(TLScontext->issuer_CN))) {
		msg_info("Could not parse server's issuer Organization");
		tls_print_errors();
	    }
	}
	tls_info->issuer_CN = TLScontext->issuer_CN;

	if (var_smtp_tls_loglevel >= 1) {
	    if (tls_info->peer_verified)
		msg_info("Verified: subject_CN=%s, issuer=%s",
			 TLScontext->peer_CN, TLScontext->issuer_CN);
	    else
		msg_info("Unverified: subject_CN=%s, issuer=%s",
			 TLScontext->peer_CN, TLScontext->issuer_CN);
	}
	X509_free(peer);
    }

    /*
     * Finally, collect information about protocol and cipher for logging
     */
    tls_info->protocol = SSL_get_version(TLScontext->con);
    cipher = SSL_get_current_cipher(TLScontext->con);
    tls_info->cipher_name = SSL_CIPHER_get_name(cipher);
    tls_info->cipher_usebits = SSL_CIPHER_get_bits(cipher,
					       &(tls_info->cipher_algbits));

    /*
     * The TLS engine is active. Switch to the tls_timed_read/write()
     * functions and make the TLScontext available to those functions.
     */
    tls_stream_start(stream, TLScontext);

    if (var_smtp_tls_loglevel >= 1)
	msg_info("TLS connection established to %s: %s with cipher %s (%d/%d bits)",
		 peername, tls_info->protocol, tls_info->cipher_name,
		 tls_info->cipher_usebits, tls_info->cipher_algbits);

    tls_int_seed();

    return (TLScontext);
}

#endif					/* USE_TLS */
