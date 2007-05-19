/*	$NetBSD: tls_server.c,v 1.1.1.4 2007/05/19 16:28:31 heas Exp $	*/

/*++
/* NAME
/*	tls_server 3
/* SUMMARY
/*	server-side TLS engine
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	SSL_CTX	*tls_server_init(props)
/*	const tls_server_props *props;
/*
/*	TLScontext_t *tls_server_start(props)
/*	const tls_server_start_props *props;
/*
/*	void	tls_server_stop(server_ctx, stream, failure, TLScontext)
/*	SSL_CTX	*server_ctx;
/*	VSTREAM	*stream;
/*	int	failure;
/*	TLScontext_t *TLScontext;
/* DESCRIPTION
/*	This module is the interface between Postfix TLS servers,
/*	the OpenSSL library, and the TLS entropy and cache manager.
/*
/*	tls_server_init() is called once when the SMTP server
/*	initializes.
/*	Certificate details are also decided during this phase,
/*	so that peer-specific behavior is not possible.
/*
/*	tls_server_start() activates the TLS feature for the VSTREAM
/*	passed as argument. We assume that network buffers are flushed
/*	and the TLS handshake can begin	immediately.
/*
/*	tls_server_stop() sends the "close notify" alert via
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
/*	Extracted CommonName of the peer, or zero-length string
/*	when information could not be extracted.
/* .IP TLScontext->issuer_CN
/*	Extracted CommonName of the issuer, or zero-length string
/*	when information could not be extracted.
/* .IP TLScontext->peer_fingerprint
/*	Fingerprint of the certificate, or null pointer when no
/*	certificate digest is available.
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
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <dict.h>
#include <stringops.h>
#include <msg.h>
#include <hex_code.h>

/* Global library. */

#include <mail_params.h>

/* TLS library. */

#include <tls_mgr.h>
#define TLS_INTERNAL
#include <tls.h>

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

/* Application-specific. */

/* We must keep some of the info available */
static const char hexcodes[] = "0123456789ABCDEF";

 /*
  * The session_id_context indentifies the service that created a session.
  * This information is used to distinguish between multiple TLS-based
  * servers running on the same server. We use the name of the mail system.
  */
static char server_session_id_context[] = "Postfix/TLS";

/* get_server_session_cb - callback to retrieve session from server cache */

static SSL_SESSION *get_server_session_cb(SSL *ssl, unsigned char *session_id,
					          int session_id_length,
					          int *unused_copy)
{
    const char *myname = "get_server_session_cb";
    TLScontext_t *TLScontext;
    VSTRING *cache_id;
    VSTRING *session_data = vstring_alloc(2048);
    SSL_SESSION *session = 0;

    if ((TLScontext = SSL_get_ex_data(ssl, TLScontext_index)) == 0)
	msg_panic("%s: null TLScontext in session lookup callback", myname);

#define GEN_CACHE_ID(buf, id, len, service) \
    do { \
	buf = vstring_alloc(2 * (len) + 1 + strlen(service) + 3); \
	hex_encode(buf, (char *) (id), (len)); \
    	vstring_sprintf_append(buf, "&s=%s", (service)); \
    } while (0)


    GEN_CACHE_ID(cache_id, session_id, session_id_length, TLScontext->serverid);

    if (TLScontext->log_level >= 2)
	msg_info("looking up session %s in %s cache",
		 STR(cache_id), TLScontext->cache_type);

    /*
     * Load the session from cache and decode it.
     */
    if (tls_mgr_lookup(TLScontext->cache_type, STR(cache_id),
		       session_data) == TLS_MGR_STAT_OK) {
	session = tls_session_activate(STR(session_data), LEN(session_data));
	if (session && (TLScontext->log_level >= 2))
	    msg_info("reloaded session %s from %s cache",
		     STR(cache_id), TLScontext->cache_type);
    }

    /*
     * Clean up.
     */
    vstring_free(cache_id);
    vstring_free(session_data);

    return (session);
}

/* uncache_session - remove session from internal & external cache */

static void uncache_session(SSL_CTX *ctx, TLScontext_t *TLScontext)
{
    VSTRING *cache_id;
    SSL_SESSION *session = SSL_get_session(TLScontext->con);

    SSL_CTX_remove_session(ctx, session);

    if (TLScontext->cache_type == 0)
	return;

    GEN_CACHE_ID(cache_id, session->session_id, session->session_id_length,
		 TLScontext->serverid);

    if (TLScontext->log_level >= 2)
	msg_info("remove session %s from %s cache",
		 STR(cache_id), TLScontext->cache_type);

    tls_mgr_delete(TLScontext->cache_type, STR(cache_id));
    vstring_free(cache_id);
}

/* new_server_session_cb - callback to save session to server cache */

static int new_server_session_cb(SSL *ssl, SSL_SESSION *session)
{
    const char *myname = "new_server_session_cb";
    VSTRING *cache_id;
    TLScontext_t *TLScontext;
    VSTRING *session_data;

    if ((TLScontext = SSL_get_ex_data(ssl, TLScontext_index)) == 0)
	msg_panic("%s: null TLScontext in new session callback", myname);

    GEN_CACHE_ID(cache_id, session->session_id, session->session_id_length,
		 TLScontext->serverid);

    if (TLScontext->log_level >= 2)
	msg_info("save session %s to %s cache",
		 STR(cache_id), TLScontext->cache_type);

    /*
     * Passivate and save the session state.
     */
    session_data = tls_session_passivate(session);
    if (session_data)
	tls_mgr_update(TLScontext->cache_type, STR(cache_id),
		       STR(session_data), LEN(session_data));

    /*
     * Clean up.
     */
    if (session_data)
	vstring_free(session_data);
    vstring_free(cache_id);
    SSL_SESSION_free(session);			/* 200502 */

    return (1);
}

/* tls_server_init - initialize the server-side TLS engine */

SSL_CTX *tls_server_init(const tls_server_props *props)
{
    long    off = 0;
    int     verify_flags = SSL_VERIFY_NONE;
    SSL_CTX *server_ctx;
    int     cachable;

    /* See skeleton at OpenSSL apps/s_server.c. */

    if (props->log_level >= 2)
	msg_info("initializing the server-side TLS engine");

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
     * understands in the message. Netscape communicator can still
     * communicate with SSLv2 servers, so it sends out a SSLv2 client hello.
     * To deal with it, our server must be SSLv2 aware (even if we don't like
     * SSLv2), so we need to have the SSLv23 server here. If we want to limit
     * the protocol level, we can add an option to not use SSLv2/v3/TLSv1
     * later.
     */
    server_ctx = SSL_CTX_new(SSLv23_server_method());
    if (server_ctx == NULL) {
	tls_print_errors();
	return (0);
    }

    /*
     * Protocol work-arounds, OpenSSL version dependent.
     */
    off |= tls_bug_bits();
    SSL_CTX_set_options(server_ctx, off);

#define DISABLE_ALL (SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1)

    /*
     * Global protocol selection. Not enabling any explicitly, enables all.
     */
    if (props->protocols != 0 && props->protocols != TLS_ALL_PROTOCOLS) {
	long    disable = DISABLE_ALL;

	if (props->protocols & TLS_PROTOCOL_TLSv1)
	    disable &= ~SSL_OP_NO_TLSv1;
	if (props->protocols & TLS_PROTOCOL_SSLv3)
	    disable &= ~SSL_OP_NO_SSLv3;
	if (props->protocols & TLS_PROTOCOL_SSLv2)
	    disable &= ~SSL_OP_NO_SSLv2;

	SSL_CTX_set_options(server_ctx, disable);
    }

    /*
     * Set the call-back routine for verbose logging.
     */
    if (props->log_level >= 2)
	SSL_CTX_set_info_callback(server_ctx, tls_info_callback);

    /*
     * Override the default cipher list with our own list.
     */
    if (tls_set_cipher_list(server_ctx, props->cipherlist) == 0) {
	SSL_CTX_free(server_ctx);
	msg_warn("Invalid cipherlist \"%s\": disabling TLS support",
		 props->cipherlist);
	return (0);				/* Already logged */
    }

    /*
     * Load the CA public key certificates for both the server cert and for
     * the verification of client certificates. As provided by OpenSSL we
     * support two types of CA certificate handling: One possibility is to
     * add all CA certificates to one large CAfile, the other possibility is
     * a directory pointed to by CApath, containing separate files for each
     * CA with softlinks named after the hash values of the certificate. The
     * first alternative has the advantage that the file is opened and read
     * at startup time, so that you don't have the hassle to maintain another
     * copy of the CApath directory for chroot-jail.
     */
    if (tls_set_ca_certificate_info(server_ctx,
				    props->CAfile, props->CApath) < 0) {
	SSL_CTX_free(server_ctx);		/* 200411 */
	return (0);
    }

    /*
     * Load the server public key certificate and private key from file and
     * check whether the cert matches the key. We cannot run without (we do
     * not support ADH anonymous Diffie-Hellman ciphers as of now). We can
     * use RSA certificates ("cert") and DSA certificates ("dcert"), both can
     * be made available at the same time. The CA certificates for both are
     * handled in the same setup already finished. Which one is used depends
     * on the cipher negotiated (that is: the first cipher listed by the
     * client which does match the server). A client with RSA only (e.g.
     * Netscape) will use the RSA certificate only. A client with
     * openssl-library will use RSA first if not especially changed in the
     * cipher setup.
     */
    if (tls_set_my_certificate_key_info(server_ctx, props->cert_file,
					props->key_file, props->dcert_file,
					props->dkey_file) < 0) {
	SSL_CTX_free(server_ctx);		/* 200411 */
	return (0);
    }

    /*
     * According to the OpenSSL documentation, temporary RSA key is needed
     * export ciphers are in use. We have to provide one, so well, we just do
     * it.
     */
    SSL_CTX_set_tmp_rsa_callback(server_ctx, tls_tmp_rsa_cb);

    /*
     * Diffie-Hellman key generation parameters can either be loaded from
     * files (preferred) or taken from compiled in values. First, set the
     * callback that will select the values when requested, then load the
     * (possibly) available DH parameters from files. We are generous with
     * the error handling, since we do have default values compiled in, so we
     * will not abort but just log the error message.
     */
    SSL_CTX_set_tmp_dh_callback(server_ctx, tls_tmp_dh_cb);
    if (*props->dh1024_param_file != 0)
	tls_set_dh_1024_from_file(props->dh1024_param_file);
    if (*props->dh512_param_file != 0)
	tls_set_dh_512_from_file(props->dh512_param_file);

    /*
     * If we want to check client certificates, we have to indicate it in
     * advance. By now we only allow to decide on a global basis. If we want
     * to allow certificate based relaying, we must ask the client to provide
     * one with SSL_VERIFY_PEER. The client now can decide, whether it
     * provides one or not. We can enforce a failure of the negotiation with
     * SSL_VERIFY_FAIL_IF_NO_PEER_CERT, if we do not allow a connection
     * without one. In the "server hello" following the initialization by the
     * "client hello" the server must provide a list of CAs it is willing to
     * accept. Some clever clients will then select one from the list of
     * available certificates matching these CAs. Netscape Communicator will
     * present the list of certificates for selecting the one to be sent, or
     * it will issue a warning, if there is no certificate matching the
     * available CAs.
     * 
     * With regard to the purpose of the certificate for relaying, we might like
     * a later negotiation, maybe relaying would already be allowed for other
     * reasons, but this would involve severe changes in the internal postfix
     * logic, so we have to live with it the way it is.
     */
    if (props->ask_ccert)
	verify_flags = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
    SSL_CTX_set_verify(server_ctx, verify_flags,
		       tls_verify_certificate_callback);
    if (*props->CAfile)
	SSL_CTX_set_client_CA_list(server_ctx,
				   SSL_load_client_CA_file(props->CAfile));

    /*
     * The session cache is implemented by the tlsmgr(8) server.
     * 
     * XXX 200502 Surprise: when OpenSSL purges an entry from the in-memory
     * cache, it also attempts to purge the entry from the on-disk cache.
     * This is undesirable, especially when we set the in-memory cache size
     * to 1. For this reason we don't allow OpenSSL to purge on-disk cache
     * entries, and leave it up to the tlsmgr process instead. Found by
     * Victor Duchovni.
     */

    if (TLSscache_index < 0)
	TLSscache_index =
	    SSL_CTX_get_ex_new_index(0, "TLScontext ex_data index",
				     NULL, NULL, NULL);

    if (tls_mgr_policy(props->cache_type, &cachable) != TLS_MGR_STAT_OK)
	cachable = 0;

    if (cachable &&
	!SSL_CTX_set_ex_data(server_ctx, TLSscache_index,
			     (void *) props->cache_type)) {
	msg_warn("Session cache off: error saving cache type in SSL context.");
	tls_print_errors();
	cachable = 0;
    }
    if (cachable || props->set_sessid) {

	/*
	 * Initialize the session cache.
	 * 
	 * With a large number of concurrent smtpd(8) processes, it is not a
	 * good idea to cache multiple large session objects in each process.
	 * We set the internal cache size to 1, and don't register a
	 * "remove_cb" so as to avoid deleting good sessions from the
	 * external cache prematurely (when the internal cache is full,
	 * OpenSSL removes sessions from the external cache also)!
	 * 
	 * This makes SSL_CTX_remove_session() not useful for flushing broken
	 * sessions from the external cache, so we must delete them directly
	 * (not via a callback).
	 * 
	 * Set a session id context to identify to what type of server process
	 * created a session. In our case, the context is simply the name of
	 * the mail system: "Postfix/TLS".
	 */
	SSL_CTX_sess_set_cache_size(server_ctx, 1);
	SSL_CTX_set_session_id_context(server_ctx,
				       (void *) &server_session_id_context,
				       sizeof(server_session_id_context));
	SSL_CTX_set_session_cache_mode(server_ctx,
				       SSL_SESS_CACHE_SERVER |
				       SSL_SESS_CACHE_NO_AUTO_CLEAR);
	if (cachable) {
	    SSL_CTX_sess_set_get_cb(server_ctx, get_server_session_cb);
	    SSL_CTX_sess_set_new_cb(server_ctx, new_server_session_cb);
	}

	/*
	 * OpenSSL ignores timed-out sessions. We need to set the internal
	 * cache timeout at least as high as the external cache timeout. This
	 * applies even if no internal cache is used.
	 */
	SSL_CTX_set_timeout(server_ctx, props->scache_timeout);
    } else {

	/*
	 * If we have no external cache, disable all caching. No use wasting
	 * server memory resources with sessions they are unlikely to be able
	 * to reuse.
	 */
	SSL_CTX_set_session_cache_mode(server_ctx, SSL_SESS_CACHE_OFF);
    }

    /*
     * Create a global index so that we can attach TLScontext information to
     * SSL objects; this information is needed inside
     * tls_verify_certificate_callback().
     */
    if (TLScontext_index < 0)
	TLScontext_index = SSL_get_ex_new_index(0, "TLScontext ex_data index",
						NULL, NULL, NULL);

    return (server_ctx);
}

 /*
  * This is the actual startup routine for a new connection. We expect that
  * the SMTP buffers are flushed and the "220 Ready to start TLS" was sent to
  * the client, so that we can immediately start the TLS handshake process.
  */
TLScontext_t *tls_server_start(const tls_server_start_props *props)
{
    int     sts;
    int     j;
    int     verify_flags;
    unsigned int n;
    TLScontext_t *TLScontext;
    SSL_CIPHER *cipher;
    X509   *peer;
    unsigned char md[EVP_MAX_MD_SIZE];
    char    buf[CCERT_BUFSIZ];

    if (props->log_level >= 1)
	msg_info("setting up TLS connection from %s[%s]",
		 props->peername, props->peeraddr);

    /*
     * Allocate a new TLScontext for the new connection and get an SSL
     * structure. Add the location of TLScontext to the SSL to later retrieve
     * the information inside the tls_verify_certificate_callback().
     */
    TLScontext = tls_alloc_context(props->log_level, props->peername);
    TLScontext->cache_type = SSL_CTX_get_ex_data(props->ctx, TLSscache_index);
    TLScontext->serverid = mystrdup(props->serverid);

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
    if (props->requirecert) {
	verify_flags = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
	verify_flags |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	TLScontext->enforce_verify_errors = 1;
	SSL_set_verify(TLScontext->con, verify_flags,
		       tls_verify_certificate_callback);
    } else {
	TLScontext->enforce_verify_errors = 0;
    }
    TLScontext->enforce_CN = 0;

    /*
     * The TLS connection is realized by a BIO_pair, so obtain the pair.
     * 
     * XXX There is no need to store the internal_bio handle in the TLScontext
     * structure. It will be attached to and destroyed with TLScontext->con.
     * The network_bio, however, needs to be freed explicitly, so we need to
     * store its handle in TLScontext.
     */
    if (!BIO_new_bio_pair(&TLScontext->internal_bio, TLS_BIO_BUFSIZE,
			  &TLScontext->network_bio, TLS_BIO_BUFSIZE)) {
	msg_warn("Could not obtain BIO_pair");
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }

    /*
     * Before really starting anything, try to seed the PRNG a little bit
     * more.
     */
    tls_int_seed();
    (void) tls_ext_seed(var_tls_daemon_rand_bytes);

    /*
     * Initialize the SSL connection to accept state. This should not be
     * necessary anymore since 0.9.3, but the call is still in the library
     * and maintaining compatibility never hurts.
     */
    SSL_set_accept_state(TLScontext->con);

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
     * call-backs for session caching and certificate verification.
     * 
     * Error handling: If the SSL handhake fails, we print out an error message
     * and remove all TLS state concerning this session.
     */
    sts = tls_bio_accept(vstream_fileno(props->stream), props->timeout,
			 TLScontext);
    if (sts <= 0) {
	msg_info("SSL_accept error from %s[%s]: %d",
		 props->peername, props->peeraddr, sts);
	tls_print_errors();
	tls_free_context(TLScontext);
	return (0);
    }
    /* Only loglevel==4 dumps everything */
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
     * Let's see whether a peer certificate is available and what is the
     * actual information. We want to save it for later use.
     */
    peer = SSL_get_peer_certificate(TLScontext->con);
    if (peer != NULL) {
	if (SSL_get_verify_result(TLScontext->con) == X509_V_OK)
	    TLScontext->peer_verified = 1;

	if (props->log_level >= 2) {
	    X509_NAME_oneline(X509_get_subject_name(peer),
			      buf, sizeof(buf));
	    msg_info("subject=%s", buf);
	    X509_NAME_oneline(X509_get_issuer_name(peer),
			      buf, sizeof(buf));
	    msg_info("issuer=%s", buf);
	}
	if (X509_digest(peer, EVP_md5(), md, &n) && n > 0) {
	    TLScontext->peer_fingerprint = mymalloc(n * 3);
	    for (j = 0; j < (int) n; j++) {
		TLScontext->peer_fingerprint[j * 3] =
		    hexcodes[(md[j] & 0xf0) >> 4U];
		TLScontext->peer_fingerprint[(j * 3) + 1] =
		    hexcodes[(md[j] & 0x0f)];
		if (j + 1 != (int) n)
		    TLScontext->peer_fingerprint[(j * 3) + 2] = ':';
		else
		    TLScontext->peer_fingerprint[(j * 3) + 2] = '\0';
	    }
	    if (props->log_level >= 1)
		msg_info("fingerprint=%s", TLScontext->peer_fingerprint);
	}
	if ((TLScontext->peer_CN = tls_peer_CN(peer)) == 0)
	    TLScontext->peer_CN = mystrdup("");
	if ((TLScontext->issuer_CN = tls_issuer_CN(peer)) == 0)
	    TLScontext->issuer_CN = mystrdup("");

	if (props->log_level >= 1) {
	    if (TLScontext->peer_verified)
		msg_info("Verified: subject_CN=%s, issuer=%s",
			 TLScontext->peer_CN, TLScontext->issuer_CN);
	    else
		msg_info("Unverified: subject_CN=%s, issuer=%s",
			 TLScontext->peer_CN, TLScontext->issuer_CN);
	}
	X509_free(peer);
    }

    /*
     * If this is a cached session, we have to check by hand if the cached
     * session peer was verified.
     */
    if (props->requirecert) {
	if (!TLScontext->peer_verified || !TLScontext->peer_CN) {
	    if (TLScontext->session_reused == 0)
		msg_panic("tls_server_start: peer was not verified");
	    msg_info("Re-used session without peer certificate removed");
	    uncache_session(props->ctx, TLScontext);
	    tls_free_context(TLScontext);
	    return (0);
	}
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

    if (props->log_level >= 1)
	msg_info("TLS connection established from %s[%s]: %s with cipher %s (%d/%d bits)",
		 props->peername, props->peeraddr,
		 TLScontext->protocol, TLScontext->cipher_name,
		 TLScontext->cipher_usebits, TLScontext->cipher_algbits);
    tls_int_seed();

    return (TLScontext);
}

#endif					/* USE_TLS */
