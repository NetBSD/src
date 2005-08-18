/*	$NetBSD: tls_server.c,v 1.1.1.1 2005/08/18 21:11:10 rpaulo Exp $	*/

/*++
/* NAME
/*	tls_server 3
/* SUMMARY
/*	server-side TLS engine
/* SYNOPSIS
/*	#include <tls.h>
/*
/*	SSL_CTX	*tls_server_init(verifydepth, askcert)
/*	int	verifydepth;
/*	int	askcert;
/*
/*	TLScontext_t *tls_server_start(server_ctx, stream, timeout,
/*					peername, peeraddr,
/*					tls_info, requirecert)
/*	SSL_CTX	*server_ctx;
/*	VSTREAM	*stream;
/*	int	timeout;
/*	const char *peername;
/*	const char *peeraddr;
/*	tls_info_t *tls_info;
/*	int	requirecert;
/*
/*	void	tls_server_stop(server_ctx, stream, failure, tls_info)
/*	SSL_CTX	*server_ctx;
/*	VSTREAM	*stream;
/*	int	failure;
/*	tls_info_t *tls_info;
/* DESCRIPTION
/*	This module is the interface between Postfix TLS servers
/*	and the OpenSSL library and TLS entropy and cache manager.
/*
/*	tls_server_init() is called once when the SMTP server
/*	initializes.
/*	Certificate details are also decided during this phase,
/*	so that peer-specific behavior is not possible.
/*
/*	tls_server_start() activates the TLS feature for the VSTREAM
/*	passed as argument. We assume that network buffers are flushed and the
/*	TLS handshake can begin	immediately. Information about the peer
/*	is stored into the tls_info structure passed as argument.
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

static int tls_server_cache = 0;

/* server_verify_callback - server verification wrapper */

static int server_verify_callback(int ok, X509_STORE_CTX *ctx)
{
    return (tls_verify_certificate_callback(ok, ctx, TLS_VERIFY_DEFAULT));
}

/* get_server_session_cb - callback to retrieve session from server cache */

static SSL_SESSION *get_server_session_cb(SSL *unused_ssl,
					          unsigned char *session_id,
					          int session_id_length,
					          int *unused_copy)
{
    VSTRING *cache_id;
    VSTRING *session_data = vstring_alloc(2048);
    SSL_SESSION *session = 0;

#define MAKE_SERVER_CACHE_ID(id, len) \
    hex_encode(vstring_alloc(2 * (len) + 1), (char *) (id), (len))

    /*
     * Encode the session ID.
     */
    cache_id = MAKE_SERVER_CACHE_ID(session_id, session_id_length);
    if (var_smtpd_tls_loglevel >= 3)
	msg_info("looking up session %s in server cache", STR(cache_id));

    /*
     * Load the session from cache and decode it.
     */
    if (tls_mgr_lookup(tls_server_cache, STR(cache_id),
		       OPENSSL_VERSION_NUMBER, TLS_MGR_NO_FLAGS,
		       session_data) == TLS_MGR_STAT_OK) {
	session = tls_session_activate(STR(session_data), LEN(session_data));
	if (session && (var_smtpd_tls_loglevel >= 3))
	    msg_info("reloaded session %s from server cache", STR(cache_id));
    }

    /*
     * Clean up.
     */
    vstring_free(cache_id);
    vstring_free(session_data);

    return (session);
}

/* new_server_session_cb - callback to save session to server cache */

static int new_server_session_cb(SSL *unused_ssl, SSL_SESSION *session)
{
    VSTRING *cache_id;
    VSTRING *session_data;

    /*
     * Encode the session ID.
     */
    cache_id =
	MAKE_SERVER_CACHE_ID(session->session_id, session->session_id_length);
    if (var_smtpd_tls_loglevel >= 3)
	msg_info("save session %s to server cache", STR(cache_id));

    /*
     * Passivate and save the session state.
     */
    session_data = tls_session_passivate(session);
    if (session_data)
	tls_mgr_update(tls_server_cache, STR(cache_id),
		       OPENSSL_VERSION_NUMBER, TLS_MGR_NO_FLAGS,
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

SSL_CTX *tls_server_init(int unused_verifydepth, int askcert)
{
    int     off = 0;
    int     verify_flags = SSL_VERIFY_NONE;
    SSL_CTX *server_ctx;
    int     cache_types;

    /* See skeleton at OpenSSL apps/s_server.c. */

    if (var_smtpd_tls_loglevel >= 2)
	msg_info("initializing the server-side TLS engine");

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
     * Here we might set SSL_OP_NO_SSLv2, SSL_OP_NO_SSLv3, SSL_OP_NO_TLSv1.
     * Of course, the last one would not make sense, since RFC2487 is only
     * defined for TLS, but we also want to accept Netscape communicator
     * requests, and it only supports SSLv3.
     */
    off |= SSL_OP_ALL;				/* Work around all known bugs */
    SSL_CTX_set_options(server_ctx, off);

    /*
     * Set the call-back routine for verbose logging.
     */
    if (var_smtpd_tls_loglevel >= 2)
	SSL_CTX_set_info_callback(server_ctx, tls_info_callback);

    /*
     * Override the default cipher list with our own list.
     */
    if (*var_smtpd_tls_cipherlist != 0)
	if (SSL_CTX_set_cipher_list(server_ctx, var_smtpd_tls_cipherlist) == 0) {
	    tls_print_errors();
	    SSL_CTX_free(server_ctx);		/* 200411 */
	    return (0);
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
    if (tls_set_ca_certificate_info(server_ctx, var_smtpd_tls_CAfile,
				    var_smtpd_tls_CApath) < 0) {
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
    if (tls_set_my_certificate_key_info(server_ctx, var_smtpd_tls_cert_file,
					var_smtpd_tls_key_file,
					var_smtpd_tls_dcert_file,
					var_smtpd_tls_dkey_file) < 0) {
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
    if (*var_smtpd_tls_dh1024_param_file != 0)
	tls_set_dh_1024_from_file(var_smtpd_tls_dh1024_param_file);
    if (*var_smtpd_tls_dh512_param_file != 0)
	tls_set_dh_512_from_file(var_smtpd_tls_dh512_param_file);

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
    if (askcert)
	verify_flags = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
    SSL_CTX_set_verify(server_ctx, verify_flags, server_verify_callback);
    if (*var_smtpd_tls_CAfile)
	SSL_CTX_set_client_CA_list(server_ctx,
			     SSL_load_client_CA_file(var_smtpd_tls_CAfile));

    /*
     * Initialize the session cache. In order to share cached sessions among
     * multiple SMTP server processes, we use an external cache and set the
     * internal cache size to a minimum value of 1. Access to the external
     * cache is handled by the appropriate callback functions.
     * 
     * Set a session id context to identify to what type of server process
     * created a session. In our case, the context is simply the name of the
     * mail system: "Postfix/TLS".
     */
    SSL_CTX_sess_set_cache_size(server_ctx, 1);
    SSL_CTX_set_timeout(server_ctx, var_smtpd_tls_scache_timeout);
    SSL_CTX_set_session_id_context(server_ctx,
				   (void *) &server_session_id_context,
				   sizeof(server_session_id_context));

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
    if (tls_mgr_policy(&cache_types) == TLS_MGR_STAT_OK
	&& (tls_server_cache = (cache_types & TLS_MGR_SCACHE_SERVER)) != 0) {
	SSL_CTX_set_session_cache_mode(server_ctx,
		      SSL_SESS_CACHE_SERVER | SSL_SESS_CACHE_NO_AUTO_CLEAR);
	SSL_CTX_sess_set_get_cb(server_ctx, get_server_session_cb);
	SSL_CTX_sess_set_new_cb(server_ctx, new_server_session_cb);
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
TLScontext_t *tls_server_start(SSL_CTX *server_ctx, VSTREAM *stream,
			               int timeout, const char *peername,
			               const char *peeraddr,
			               tls_info_t *tls_info,
			               int requirecert)
{
    int     sts;
    int     j;
    int     verify_flags;
    unsigned int n;
    TLScontext_t *TLScontext;
    SSL_SESSION *session;
    SSL_CIPHER *cipher;
    X509   *peer;

    if (var_smtpd_tls_loglevel >= 1)
	msg_info("setting up TLS connection from %s[%s]", peername, peeraddr);

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
    TLScontext->log_level = var_smtpd_tls_loglevel;
    strncpy(TLScontext->peername_save, peername, PEERNAME_SIZE - 1);
    TLScontext->peername_save[PEERNAME_SIZE - 1] = 0;
    (void) lowercase(TLScontext->peername_save);

    if ((TLScontext->con = (SSL *) SSL_new(server_ctx)) == NULL) {
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
    if (requirecert) {
	verify_flags = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
	verify_flags |= SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
	TLScontext->enforce_verify_errors = 1;
	SSL_set_verify(TLScontext->con, verify_flags, server_verify_callback);
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
	msg_info("Could not obtain BIO_pair");
	tls_print_errors();
	SSL_free(TLScontext->con);
	FREE_TLS_CONTEXT(TLScontext);
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
    if (var_smtpd_tls_loglevel >= 3)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), tls_bio_dump_cb);

    /*
     * Start TLS negotiations. This process is a black box that invokes our
     * call-backs for session caching and certificate verification.
     * 
     * Error handling: If the SSL handhake fails, we print out an error message
     * and remove all TLS state concerning this session.
     */
    sts = tls_bio_accept(vstream_fileno(stream), timeout, TLScontext);
    if (sts <= 0) {
	msg_info("SSL_accept error from %s[%s]: %d", peername, peeraddr, sts);
	tls_print_errors();
	SSL_free(TLScontext->con);
	BIO_free(TLScontext->network_bio);	/* 200411 */
	FREE_TLS_CONTEXT(TLScontext);
	return (0);
    }
    /* Only loglevel==4 dumps everything */
    if (var_smtpd_tls_loglevel < 4)
	BIO_set_callback(SSL_get_rbio(TLScontext->con), 0);

    /*
     * Let's see whether a peer certificate is available and what is the
     * actual information. We want to save it for later use.
     */
    peer = SSL_get_peer_certificate(TLScontext->con);
    if (peer != NULL) {
	if (SSL_get_verify_result(TLScontext->con) == X509_V_OK)
	    tls_info->peer_verified = 1;

	X509_NAME_oneline(X509_get_subject_name(peer),
			  TLScontext->peer_subject,
			  sizeof(TLScontext->peer_subject));
	if (var_smtpd_tls_loglevel >= 2)
	    msg_info("subject=%s", TLScontext->peer_subject);
	tls_info->peer_subject = TLScontext->peer_subject;

	X509_NAME_oneline(X509_get_issuer_name(peer),
			  TLScontext->peer_issuer,
			  sizeof(TLScontext->peer_issuer));
	if (var_smtpd_tls_loglevel >= 2)
	    msg_info("issuer=%s", TLScontext->peer_issuer);
	tls_info->peer_issuer = TLScontext->peer_issuer;

	if (X509_digest(peer, EVP_md5(), TLScontext->md, &n)) {
	    for (j = 0; j < (int) n; j++) {
		TLScontext->fingerprint[j * 3] =
		    hexcodes[(TLScontext->md[j] & 0xf0) >> 4];
		TLScontext->fingerprint[(j * 3) + 1] =
		    hexcodes[(TLScontext->md[j] & 0x0f)];
		if (j + 1 != (int) n)
		    TLScontext->fingerprint[(j * 3) + 2] = ':';
		else
		    TLScontext->fingerprint[(j * 3) + 2] = '\0';
	    }
	    if (var_smtpd_tls_loglevel >= 1)
		msg_info("fingerprint=%s", TLScontext->fingerprint);
	    tls_info->peer_fingerprint = TLScontext->fingerprint;
	}
	TLScontext->peer_CN[0] = '\0';
	if (!X509_NAME_get_text_by_NID(X509_get_subject_name(peer),
				       NID_commonName, TLScontext->peer_CN,
				       sizeof(TLScontext->peer_CN))) {
	    msg_info("Could not parse client's subject CN");
	    tls_print_errors();
	}
	tls_info->peer_CN = TLScontext->peer_CN;

	TLScontext->issuer_CN[0] = '\0';
	if (!X509_NAME_get_text_by_NID(X509_get_issuer_name(peer),
				       NID_commonName, TLScontext->issuer_CN,
				       sizeof(TLScontext->issuer_CN))) {
	    msg_info("Could not parse client's issuer CN");
	    tls_print_errors();
	}
	if (!TLScontext->issuer_CN[0]) {
	    /* No issuer CN field, use Organization instead */
	    if (!X509_NAME_get_text_by_NID(X509_get_issuer_name(peer),
				NID_organizationName, TLScontext->issuer_CN,
					   sizeof(TLScontext->issuer_CN))) {
		msg_info("Could not parse client's issuer Organization");
		tls_print_errors();
	    }
	}
	tls_info->issuer_CN = TLScontext->issuer_CN;

	if (var_smtpd_tls_loglevel >= 1) {
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
     * If this is a cached session, we have to check by hand if the cached
     * session peer was verified.
     */
    if (requirecert) {
	if (!tls_info->peer_verified || !tls_info->peer_CN) {
	    msg_info("Re-used session without peer certificate removed");
	    session = SSL_get_session(TLScontext->con);
	    SSL_CTX_remove_session(server_ctx, session);
	    SSL_free(TLScontext->con);
	    BIO_free(TLScontext->network_bio);	/* 200411 */
	    FREE_TLS_CONTEXT(TLScontext);
	    return (0);
	}
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

    if (var_smtpd_tls_loglevel >= 1)
	msg_info("TLS connection established from %s[%s]: %s with cipher %s (%d/%d bits)",
		 peername, peeraddr,
		 tls_info->protocol, tls_info->cipher_name,
		 tls_info->cipher_usebits, tls_info->cipher_algbits);
    tls_int_seed();

    return (TLScontext);
}

#endif					/* USE_TLS */
