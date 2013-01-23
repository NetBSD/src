/*	$NetBSD: tlsproxy.c,v 1.1.1.1.6.2 2013/01/23 00:05:15 yamt Exp $	*/

/*++
/* NAME
/*	tlsproxy 8
/* SUMMARY
/*	Postfix TLS proxy
/* SYNOPSIS
/*	\fBtlsproxy\fR [generic Postfix daemon options]
/* DESCRIPTION
/*	The \fBtlsproxy\fR(8) server implements a server-side TLS
/*	proxy. It is used by \fBpostscreen\fR(8) to talk SMTP-over-TLS
/*	with remote SMTP clients whose whitelist status has expired,
/*	but it should also work for non-SMTP protocols.
/*
/*	Although one \fBtlsproxy\fR(8) process can serve multiple
/*	sessions at the same time, it is a good idea to allow the
/*	number of processes to increase with load, so that the
/*	service remains responsive.
/* PROTOCOL EXAMPLE
/* .ad
/* .fi
/*	The example below concerns \fBpostscreen\fR(8). However,
/*	the \fBtlsproxy\fR(8) server is agnostic of the application
/*	protocol, and the example is easily adapted to other
/*	applications.
/*
/*	The \fBpostscreen\fR(8) server sends the remote SMTP client
/*	endpoint string, the requested role (server), and the
/*	requested timeout to \fBtlsproxy\fR(8).  \fBpostscreen\fR(8)
/*	then receives a "TLS available" indication from \fBtlsproxy\fR(8).
/*	If the TLS service is available, \fBpostscreen\fR(8) sends
/*	the remote SMTP client file descriptor to \fBtlsproxy\fR(8),
/*	and sends the plaintext 220 greeting to the remote SMTP
/*	client.  This triggers TLS negotiations between the remote
/*	SMTP client and \fBtlsproxy\fR(8).  Upon completion of the
/*	TLS-level handshake, \fBtlsproxy\fR(8) translates between
/*	plaintext from/to \fBpostscreen\fR(8) and ciphertext to/from
/*	the remote SMTP client.
/* SECURITY
/* .ad
/* .fi
/*	The \fBtlsproxy\fR(8) server is moderately security-sensitive.
/*	It talks to untrusted clients on the network. The process
/*	can be run chrooted at fixed low privilege.
/* DIAGNOSTICS
/*	Problems and transactions are logged to \fBsyslogd\fR(8).
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	Changes to \fBmain.cf\fR are not picked up automatically,
/*	as \fBtlsproxy\fR(8) processes may run for a long time
/*	depending on mail server load.  Use the command "\fBpostfix
/*	reload\fR" to speed up a change.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* STARTTLS SUPPORT CONTROLS
/* .ad
/* .fi
/* .IP "\fBtlsproxy_tls_CAfile ($smtpd_tls_CAfile)\fR"
/*	A file containing (PEM format) CA certificates of root CAs
/*	trusted to sign either remote SMTP client certificates or intermediate
/*	CA certificates.
/* .IP "\fBtlsproxy_tls_CApath ($smtpd_tls_CApath)\fR"
/*	A directory containing (PEM format) CA certificates of root CAs
/*	trusted to sign either remote SMTP client certificates or intermediate
/*	CA certificates.
/* .IP "\fBtlsproxy_tls_always_issue_session_ids ($smtpd_tls_always_issue_session_ids)\fR"
/*	Force the Postfix \fBtlsproxy\fR(8) server to issue a TLS session id,
/*	even when TLS session caching is turned off.
/* .IP "\fBtlsproxy_tls_ask_ccert ($smtpd_tls_ask_ccert)\fR"
/*	Ask a remote SMTP client for a client certificate.
/* .IP "\fBtlsproxy_tls_ccert_verifydepth ($smtpd_tls_ccert_verifydepth)\fR"
/*	The verification depth for remote SMTP client certificates.
/* .IP "\fBtlsproxy_tls_cert_file ($smtpd_tls_cert_file)\fR"
/*	File with the Postfix \fBtlsproxy\fR(8) server RSA certificate in PEM
/*	format.
/* .IP "\fBtlsproxy_tls_ciphers ($smtpd_tls_ciphers)\fR"
/*	The minimum TLS cipher grade that the Postfix \fBtlsproxy\fR(8) server
/*	will use with opportunistic TLS encryption.
/* .IP "\fBtlsproxy_tls_dcert_file ($smtpd_tls_dcert_file)\fR"
/*	File with the Postfix \fBtlsproxy\fR(8) server DSA certificate in PEM
/*	format.
/* .IP "\fBtlsproxy_tls_dh1024_param_file ($smtpd_tls_dh1024_param_file)\fR"
/*	File with DH parameters that the Postfix \fBtlsproxy\fR(8) server
/*	should use with EDH ciphers.
/* .IP "\fBtlsproxy_tls_dh512_param_file ($smtpd_tls_dh512_param_file)\fR"
/*	File with DH parameters that the Postfix \fBtlsproxy\fR(8) server
/*	should use with EDH ciphers.
/* .IP "\fBtlsproxy_tls_dkey_file ($smtpd_tls_dkey_file)\fR"
/*	File with the Postfix \fBtlsproxy\fR(8) server DSA private key in PEM
/*	format.
/* .IP "\fBtlsproxy_tls_eccert_file ($smtpd_tls_eccert_file)\fR"
/*	File with the Postfix \fBtlsproxy\fR(8) server ECDSA certificate in
/*	PEM format.
/* .IP "\fBtlsproxy_tls_eckey_file ($smtpd_tls_eckey_file)\fR"
/*	File with the Postfix \fBtlsproxy\fR(8) server ECDSA private key in
/*	PEM format.
/* .IP "\fBtlsproxy_tls_eecdh_grade ($smtpd_tls_eecdh_grade)\fR"
/*	The Postfix \fBtlsproxy\fR(8) server security grade for ephemeral
/*	elliptic-curve Diffie-Hellman (EECDH) key exchange.
/* .IP "\fBtlsproxy_tls_exclude_ciphers ($smtpd_tls_exclude_ciphers)\fR"
/*	List of ciphers or cipher types to exclude from the \fBtlsproxy\fR(8)
/*	server cipher list at all TLS security levels.
/* .IP "\fBtlsproxy_tls_fingerprint_digest ($smtpd_tls_fingerprint_digest)\fR"
/*	The message digest algorithm to construct remote SMTP
/*	client-certificate
/*	fingerprints.
/* .IP "\fBtlsproxy_tls_key_file ($smtpd_tls_key_file)\fR"
/*	File with the Postfix \fBtlsproxy\fR(8) server RSA private key in PEM
/*	format.
/* .IP "\fBtlsproxy_tls_loglevel ($smtpd_tls_loglevel)\fR"
/*	Enable additional Postfix \fBtlsproxy\fR(8) server logging of TLS
/*	activity.
/* .IP "\fBtlsproxy_tls_mandatory_ciphers ($smtpd_tls_mandatory_ciphers)\fR"
/*	The minimum TLS cipher grade that the Postfix \fBtlsproxy\fR(8) server
/*	will use with mandatory TLS encryption.
/* .IP "\fBtlsproxy_tls_mandatory_exclude_ciphers ($smtpd_tls_mandatory_exclude_ciphers)\fR"
/*	Additional list of ciphers or cipher types to exclude from the
/*	\fBtlsproxy\fR(8) server cipher list at mandatory TLS security levels.
/* .IP "\fBtlsproxy_tls_mandatory_protocols ($smtpd_tls_mandatory_protocols)\fR"
/*	The SSL/TLS protocols accepted by the Postfix \fBtlsproxy\fR(8) server
/*	with mandatory TLS encryption.
/* .IP "\fBtlsproxy_tls_protocols ($smtpd_tls_protocols)\fR"
/*	List of TLS protocols that the Postfix \fBtlsproxy\fR(8) server will
/*	exclude or include with opportunistic TLS encryption.
/* .IP "\fBtlsproxy_tls_req_ccert ($smtpd_tls_req_ccert)\fR"
/*	With mandatory TLS encryption, require a trusted remote SMTP
/*	client certificate in order to allow TLS connections to proceed.
/* .IP "\fBtlsproxy_tls_security_level ($smtpd_tls_security_level)\fR"
/*	The SMTP TLS security level for the Postfix \fBtlsproxy\fR(8) server;
/*	when a non-empty value is specified, this overrides the obsolete
/*	parameters smtpd_use_tls and smtpd_enforce_tls.
/* .IP "\fBtlsproxy_tls_session_cache_timeout ($smtpd_tls_session_cache_timeout)\fR"
/*	The expiration time of Postfix \fBtlsproxy\fR(8) server TLS session
/*	cache information.
/* OBSOLETE STARTTLS SUPPORT CONTROLS
/* .ad
/* .fi
/*	These parameters are supported for compatibility with
/*	\fBsmtpd\fR(8) legacy parameters.
/* .IP "\fBtlsproxy_use_tls ($smtpd_use_tls)\fR"
/*	Opportunistic TLS: announce STARTTLS support to remote SMTP clients,
/*	but do not require that clients use TLS encryption.
/* .IP "\fBtlsproxy_enforce_tls ($smtpd_enforce_tls)\fR"
/*	Mandatory TLS: announce STARTTLS support to remote SMTP clients, and
/*	require that clients use TLS encryption.
/* RESOURCE CONTROLS
/* .ad
/* .fi
/* .IP "\fBtlsproxy_watchdog_timeout (10s)\fR"
/*	How much time a \fBtlsproxy\fR(8) process may take to process local
/*	or remote I/O before it is terminated by a built-in watchdog timer.
/* MISCELLANEOUS CONTROLS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBprocess_id (read-only)\fR"
/*	The process ID of a Postfix command or daemon process.
/* .IP "\fBprocess_name (read-only)\fR"
/*	The process name of a Postfix command or daemon process.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* SEE ALSO
/*	postscreen(8), Postfix zombie blocker
/*	smtpd(8), Postfix SMTP server
/*	postconf(5), configuration parameters
/*	syslogd(5), system logging
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	This service was introduced with Postfix version 2.8.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <vstream.h>
#include <iostuff.h>
#include <nbbio.h>
#include <mymalloc.h>

 /*
  * Global library.
  */
#include <mail_proto.h>
#include <mail_params.h>
#include <mail_conf.h>
#include <mail_version.h>

 /*
  * Master library.
  */
#include <mail_server.h>

 /*
  * TLS library.
  */
#ifdef USE_TLS
#define TLS_INTERNAL			/* XXX */
#include <tls.h>
#include <tls_proxy.h>

 /*
  * Application-specific.
  */
#include <tlsproxy.h>

 /*
  * Tunable parameters. We define our clones of the smtpd(8) parameters to
  * avoid any confusion about which parameters are used by this program.
  */
int     var_smtpd_tls_ccert_vd;
char   *var_smtpd_tls_loglevel;
int     var_smtpd_tls_scache_timeout;
bool    var_smtpd_use_tls;
bool    var_smtpd_enforce_tls;
bool    var_smtpd_tls_ask_ccert;
bool    var_smtpd_tls_req_ccert;
bool    var_smtpd_tls_set_sessid;
char   *var_smtpd_relay_ccerts;
char   *var_smtpd_tls_cert_file;
char   *var_smtpd_tls_key_file;
char   *var_smtpd_tls_dcert_file;
char   *var_smtpd_tls_dkey_file;
char   *var_smtpd_tls_eccert_file;
char   *var_smtpd_tls_eckey_file;
char   *var_smtpd_tls_CAfile;
char   *var_smtpd_tls_CApath;
char   *var_smtpd_tls_ciph;
char   *var_smtpd_tls_mand_ciph;
char   *var_smtpd_tls_excl_ciph;
char   *var_smtpd_tls_mand_excl;
char   *var_smtpd_tls_proto;
char   *var_smtpd_tls_mand_proto;
char   *var_smtpd_tls_dh512_param_file;
char   *var_smtpd_tls_dh1024_param_file;
char   *var_smtpd_tls_eecdh;
char   *var_smtpd_tls_fpt_dgst;
char   *var_smtpd_tls_level;

int     var_tlsp_tls_ccert_vd;
char   *var_tlsp_tls_loglevel;
int     var_tlsp_tls_scache_timeout;
bool    var_tlsp_use_tls;
bool    var_tlsp_enforce_tls;
bool    var_tlsp_tls_ask_ccert;
bool    var_tlsp_tls_req_ccert;
bool    var_tlsp_tls_set_sessid;
char   *var_tlsp_tls_cert_file;
char   *var_tlsp_tls_key_file;
char   *var_tlsp_tls_dcert_file;
char   *var_tlsp_tls_dkey_file;
char   *var_tlsp_tls_eccert_file;
char   *var_tlsp_tls_eckey_file;
char   *var_tlsp_tls_CAfile;
char   *var_tlsp_tls_CApath;
char   *var_tlsp_tls_ciph;
char   *var_tlsp_tls_mand_ciph;
char   *var_tlsp_tls_excl_ciph;
char   *var_tlsp_tls_mand_excl;
char   *var_tlsp_tls_proto;
char   *var_tlsp_tls_mand_proto;
char   *var_tlsp_tls_dh512_param_file;
char   *var_tlsp_tls_dh1024_param_file;
char   *var_tlsp_tls_eecdh;
char   *var_tlsp_tls_fpt_dgst;
char   *var_tlsp_tls_level;

int     var_tlsp_watchdog;

 /*
  * TLS per-process status.
  */
static TLS_APPL_STATE *tlsp_server_ctx;
static int ask_client_cert;

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

 /*
  * This code looks simpler than expected. That is the result of a great deal
  * of effort, mainly in design and analysis.
  * 
  * By design, postscreen(8) is an event-driven server that must scale up to a
  * large number of clients. This means that postscreen(8) must avoid doing
  * CPU-intensive operations such as those in OpenSSL.
  * 
  * tlsproxy(8) runs the OpenSSL code on behalf of postscreen(8), translating
  * plaintext SMTP messages from postscreen(8) into SMTP-over-TLS messages to
  * the remote SMTP client, and vice versa. As long as postscreen(8) does not
  * receive email messages, the cost of doing TLS operations will be modest.
  * 
  * Like postscreen(8), one tlsproxy(8) process services multiple remote SMTP
  * clients. Unlike postscreen(8), there can be more than one tlsproxy(8)
  * process, although their number is meant to be much smaller than the
  * number of remote SMTP clients that talk TLS.
  * 
  * As with postscreen(8), all I/O must be event-driven: encrypted traffic
  * between tlsproxy(8) and remote SMTP clients, and plaintext traffic
  * between tlsproxy(8) and postscreen(8). Event-driven plaintext I/O is
  * straightforward enough that it could be abstracted away with the nbbio(3)
  * module.
  * 
  * The event-driven TLS I/O implementation is founded on on-line OpenSSL
  * documentation, supplemented by statements from OpenSSL developers on
  * public mailing lists. After some field experience with this code, we may
  * be able to factor it out as a library module, like nbbio(3), that can
  * become part of the TLS library.
  */

static void tlsp_ciphertext_event(int, char *);

#define TLSP_INIT_TIMEOUT	100

/* tlsp_drain - delayed exit after "postfix reload" */

static void tlsp_drain(char *unused_service, char **unused_argv)
{
    int     count;

    /*
     * After "postfix reload", complete work-in-progress in the background,
     * instead of dropping already-accepted connections on the floor.
     * 
     * All error retry counts shall be limited. Instead of blocking here, we
     * could retry failed fork() operations in the event call-back routines,
     * but we don't need perfection. The host system is severely overloaded
     * and service levels are already way down.
     */
    for (count = 0; /* see below */ ; count++) {
	if (count >= 5) {
	    msg_fatal("fork: %m");
	} else if (event_server_drain() != 0) {
	    msg_warn("fork: %m");
	    sleep(1);
	    continue;
	} else {
	    return;
	}
    }
}

/* tlsp_eval_tls_error - translate TLS "error" result into action */

static int tlsp_eval_tls_error(TLSP_STATE *state, int err)
{
    int     ciphertext_fd = state->ciphertext_fd;

    /*
     * The ciphertext file descriptor is in non-blocking mode, meaning that
     * each SSL_accept/connect/read/write/shutdown request may return an
     * "error" indication that it needs to read or write more ciphertext. The
     * purpose of this routine is to translate those "error" indications into
     * the appropriate read/write/timeout event requests.
     */
    switch (err) {

	/*
	 * No error from SSL_read and SSL_write means that the plaintext
	 * output buffer is full and that the plaintext input buffer is
	 * empty. Stop read/write events on the ciphertext stream. Keep the
	 * timer alive as a safety mechanism for the case that the plaintext
	 * pseudothreads get stuck.
	 */
    case SSL_ERROR_NONE:
	if (state->ssl_last_err != SSL_ERROR_NONE) {
	    event_disable_readwrite(ciphertext_fd);
	    event_request_timer(tlsp_ciphertext_event, (char *) state,
				state->timeout);
	    state->ssl_last_err = SSL_ERROR_NONE;
	}
	return (0);

	/*
	 * The TLS engine wants to write to the network. Turn on
	 * write/timeout events on the ciphertext stream.
	 */
    case SSL_ERROR_WANT_WRITE:
	if (state->ssl_last_err == SSL_ERROR_WANT_READ)
	    event_disable_readwrite(ciphertext_fd);
	if (state->ssl_last_err != SSL_ERROR_WANT_WRITE) {
	    event_enable_write(ciphertext_fd, tlsp_ciphertext_event,
			       (char *) state);
	    state->ssl_last_err = SSL_ERROR_WANT_WRITE;
	}
	event_request_timer(tlsp_ciphertext_event, (char *) state,
			    state->timeout);
	return (0);

	/*
	 * The TLS engine wants to read from the network. Turn on
	 * read/timeout events on the ciphertext stream.
	 */
    case SSL_ERROR_WANT_READ:
	if (state->ssl_last_err == SSL_ERROR_WANT_WRITE)
	    event_disable_readwrite(ciphertext_fd);
	if (state->ssl_last_err != SSL_ERROR_WANT_READ) {
	    event_enable_read(ciphertext_fd, tlsp_ciphertext_event,
			      (char *) state);
	    state->ssl_last_err = SSL_ERROR_WANT_READ;
	}
	event_request_timer(tlsp_ciphertext_event, (char *) state,
			    state->timeout);
	return (0);

	/*
	 * Some error. Self-destruct. This automagically cleans up all
	 * pending read/write and timeout event requests, making state a
	 * dangling pointer.
	 */
    case SSL_ERROR_SSL:
	tls_print_errors();
	/* FALLTHROUGH */
    default:
	tlsp_state_free(state);
	return (-1);
    }
}

/* tlsp_strategy - decide what to read or write next. */

static void tlsp_strategy(TLSP_STATE *state)
{
    TLS_SESS_STATE *tls_context = state->tls_context;
    NBBIO  *plaintext_buf;
    int     ssl_stat;
    int     ssl_read_err;
    int     ssl_write_err;
    int     handshake_err;

    /*
     * Be sure to complete the TLS handshake before enabling plain-text I/O.
     * In case of an unrecoverable error, this automagically cleans up all
     * pending read/write and timeout event requests.
     */
    if (state->flags & TLSP_FLAG_DO_HANDSHAKE) {
	ssl_stat = SSL_accept(tls_context->con);
	if (ssl_stat != 1) {
	    handshake_err = SSL_get_error(tls_context->con, ssl_stat);
	    tlsp_eval_tls_error(state, handshake_err);
	    /* At this point, state could be a dangling pointer. */
	    return;
	}
	if ((state->tls_context = tls_server_post_accept(tls_context)) == 0) {
	    tlsp_state_free(state);
	    return;
	}
	if ((state->req_flags & TLS_PROXY_FLAG_SEND_CONTEXT) != 0
	    && (attr_print(state->plaintext_stream, ATTR_FLAG_NONE,
			   ATTR_TYPE_FUNC, tls_proxy_context_print,
			   (char *) state->tls_context, ATTR_TYPE_END) != 0
		|| vstream_fflush(state->plaintext_stream) != 0)) {
	    msg_warn("cannot send TLS context: %m");
	    tlsp_state_free(state);
	    return;
	}
	state->flags &= ~TLSP_FLAG_DO_HANDSHAKE;
    }

    /*
     * Shutdown and self-destruct after NBBIO error. This automagically
     * cleans up all pending read/write and timeout event requests. Before
     * shutting down TLS, we stop all plain-text I/O events but keep the
     * NBBIO error flags.
     */
    plaintext_buf = state->plaintext_buf;
    if (NBBIO_ERROR_FLAGS(plaintext_buf)) {
	if (NBBIO_ACTIVE_FLAGS(plaintext_buf))
	    nbbio_disable_readwrite(state->plaintext_buf);
	ssl_stat = SSL_shutdown(tls_context->con);
	/* XXX Wait for return value 1 if sessions are to be reused? */
	if (ssl_stat < 0) {
	    handshake_err = SSL_get_error(tls_context->con, ssl_stat);
	    tlsp_eval_tls_error(state, handshake_err);
	    /* At this point, state could be a dangling pointer. */
	    return;
	}
	tlsp_state_free(state);
	return;
    }

    /*
     * Try to move data from the plaintext input buffer to the TLS engine.
     * 
     * XXX We're supposed to repeat the exact same SSL_write() call arguments
     * after an SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE result. Rumor has
     * it that this is because each SSL_write() call reads from the buffer
     * incrementally, and returns > 0 only after the final byte is processed.
     * Rumor also has it that setting SSL_MODE_ENABLE_PARTIAL_WRITE and
     * SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER voids this requirement, and that
     * repeating the request with an increased request size is OK.
     * Unfortunately all this is not or poorly documented, and one has to
     * rely on statements from OpenSSL developers in public mailing archives.
     */
    ssl_write_err = SSL_ERROR_NONE;
    while (NBBIO_READ_PEND(plaintext_buf) > 0) {
	ssl_stat = SSL_write(tls_context->con, NBBIO_READ_BUF(plaintext_buf),
			     NBBIO_READ_PEND(plaintext_buf));
	ssl_write_err = SSL_get_error(tls_context->con, ssl_stat);
	if (ssl_write_err != SSL_ERROR_NONE)
	    break;
	/* Allow the plaintext pseudothread to read more data. */
	NBBIO_READ_PEND(plaintext_buf) -= ssl_stat;
	if (NBBIO_READ_PEND(plaintext_buf) > 0)
	    memmove(NBBIO_READ_BUF(plaintext_buf),
		    NBBIO_READ_BUF(plaintext_buf) + ssl_stat,
		    NBBIO_READ_PEND(plaintext_buf));
    }

    /*
     * Try to move data from the TLS engine to the plaintext output buffer.
     * Note: data may arrive as a side effect of calling SSL_write(),
     * therefore we call SSL_read() after calling SSL_write().
     * 
     * XXX We're supposed to repeat the exact same SSL_read() call arguments
     * after an SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE result. This
     * supposedly means that our plaintext writer must not memmove() the
     * plaintext output buffer until after the SSL_read() call succeeds. For
     * now I'll ignore this, because 1) SSL_read() is documented to return
     * the bytes available, instead of returning > 0 only after the entire
     * buffer is processed like SSL_write() does; and 2) there is no "read"
     * equivalent of the SSL_R_BAD_WRITE_RETRY, SSL_MODE_ENABLE_PARTIAL_WRITE
     * or SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER features.
     */
    ssl_read_err = SSL_ERROR_NONE;
    while (NBBIO_WRITE_PEND(state->plaintext_buf) < NBBIO_BUFSIZE(plaintext_buf)) {
	ssl_stat = SSL_read(tls_context->con,
			    NBBIO_WRITE_BUF(plaintext_buf)
			    + NBBIO_WRITE_PEND(state->plaintext_buf),
			    NBBIO_BUFSIZE(plaintext_buf)
			    - NBBIO_WRITE_PEND(state->plaintext_buf));
	ssl_read_err = SSL_get_error(tls_context->con, ssl_stat);
	if (ssl_read_err != SSL_ERROR_NONE)
	    break;
	NBBIO_WRITE_PEND(plaintext_buf) += ssl_stat;
    }

    /*
     * Try to enable/disable ciphertext read/write events. If SSL_write() was
     * satisfied, see if SSL_read() wants to do some work. In case of an
     * unrecoverable error, this automagically destroys the session state
     * after cleaning up all pending read/write and timeout event requests.
     */
    if (tlsp_eval_tls_error(state, ssl_write_err != SSL_ERROR_NONE ?
			    ssl_write_err : ssl_read_err) < 0)
	return;

    /*
     * Try to enable/disable plaintext read/write events. Basically, if we
     * have nothing to write to the postscreen(8) server, see if there is
     * something to read. If the write buffer is empty and the read buffer is
     * full, suspend plaintext I/O until conditions change (but keep the
     * timer active, as a safety mechanism in case ciphertext I/O gets
     * stuck).
     * 
     * XXX In theory, if the client keeps writing fast enough then we would
     * never read from postscreen(8), and cause postscreen(8) to block. In
     * practice, postscreen(8) limits the number of client commands, and thus
     * postscreen(8)'s output will fit in a kernel buffer. This may not be
     * true in other scenarios where the tlsproxy(8) server could be used.
     */
    if (NBBIO_WRITE_PEND(plaintext_buf) > 0) {
	if (NBBIO_ACTIVE_FLAGS(plaintext_buf) & NBBIO_FLAG_READ)
	    nbbio_disable_readwrite(plaintext_buf);
	if ((NBBIO_ACTIVE_FLAGS(plaintext_buf) & NBBIO_FLAG_WRITE) == 0)
	    nbbio_enable_write(plaintext_buf, state->timeout);
    } else if (NBBIO_READ_PEND(plaintext_buf) < NBBIO_BUFSIZE(plaintext_buf)) {
	if (NBBIO_ACTIVE_FLAGS(plaintext_buf) & NBBIO_FLAG_WRITE)
	    nbbio_disable_readwrite(plaintext_buf);
	if ((NBBIO_ACTIVE_FLAGS(plaintext_buf) & NBBIO_FLAG_READ) == 0)
	    nbbio_enable_read(plaintext_buf, state->timeout);
    } else {
	if (NBBIO_ACTIVE_FLAGS(plaintext_buf))
	    nbbio_slumber(plaintext_buf, state->timeout);
    }
}

/* tlsp_plaintext_event - plaintext was read/written */

static void tlsp_plaintext_event(int event, char *context)
{
    TLSP_STATE *state = (TLSP_STATE *) context;

    /*
     * Safety alert: the plaintext pseudothreads have "slumbered" for too
     * long (see code above). This means that the ciphertext pseudothreads
     * are stuck.
     */
    if ((NBBIO_ERROR_FLAGS(state->plaintext_buf) & NBBIO_FLAG_TIMEOUT) != 0
	&& NBBIO_ACTIVE_FLAGS(state->plaintext_buf) == 0)
	msg_warn("deadlock on ciphertext stream for %s", state->remote_endpt);

    /*
     * This is easy, because the NBBIO layer has already done the event
     * decoding and plaintext I/O for us. All we need to do is decide if we
     * want to read or write more plaintext.
     */
    tlsp_strategy(state);
}

/* tlsp_ciphertext_event - ciphertext is ready to read/write */

static void tlsp_ciphertext_event(int event, char *context)
{
    TLSP_STATE *state = (TLSP_STATE *) context;

    /*
     * Without a TLS quivalent of the NBBIO layer, we must decode the events
     * ourselves and do the ciphertext I/O. Then, we can decide if we want to
     * read or write more ciphertext.
     */
    if (event == EVENT_READ || event == EVENT_WRITE) {
	tlsp_strategy(state);
    } else {
	if (event == EVENT_TIME && state->ssl_last_err == SSL_ERROR_NONE)
	    msg_warn("deadlock on plaintext stream for %s",
		     state->remote_endpt);
	else
	    msg_warn("ciphertext read/write %s for %s",
		     event == EVENT_TIME ? "timeout" : "error",
		     state->remote_endpt);
	tlsp_state_free(state);
    }
}

/* tlsp_start_tls - turn on TLS or force disconnect */

static void tlsp_start_tls(TLSP_STATE *state)
{
    TLS_SERVER_START_PROPS props;
    static char *cipher_grade;
    static VSTRING *cipher_exclusions;

    /*
     * The code in this routine is pasted literally from smtpd(8). I am not
     * going to sanitize this because doing so surely will break things in
     * unexpected ways.
     */

    /*
     * Perform the before-handshake portion of the per-session initalization.
     * Pass a null VSTREAM to indicate that this program, will do the
     * ciphertext I/O, not libtls.
     * 
     * The cipher grade and exclusions don't change between sessions. Compute
     * just once and cache.
     */
#define ADD_EXCLUDE(vstr, str) \
    do { \
	if (*(str)) \
	    vstring_sprintf_append((vstr), "%s%s", \
				   VSTRING_LEN(vstr) ? " " : "", (str)); \
    } while (0)

    if (cipher_grade == 0) {
	cipher_grade =
	    var_tlsp_enforce_tls ? var_tlsp_tls_mand_ciph : var_tlsp_tls_ciph;
	cipher_exclusions = vstring_alloc(10);
	ADD_EXCLUDE(cipher_exclusions, var_tlsp_tls_excl_ciph);
	if (var_tlsp_enforce_tls)
	    ADD_EXCLUDE(cipher_exclusions, var_tlsp_tls_mand_excl);
	if (ask_client_cert)
	    ADD_EXCLUDE(cipher_exclusions, "aNULL");
    }
    state->tls_context =
	TLS_SERVER_START(&props,
			 ctx = tlsp_server_ctx,
			 stream = (VSTREAM *) 0,/* unused */
			 fd = state->ciphertext_fd,
			 timeout = 0,		/* unused */
			 requirecert = (var_tlsp_tls_req_ccert
					&& var_tlsp_enforce_tls),
			 serverid = state->server_id,
			 namaddr = state->remote_endpt,
			 cipher_grade = cipher_grade,
			 cipher_exclusions = STR(cipher_exclusions),
			 fpt_dgst = var_tlsp_tls_fpt_dgst);

    if (state->tls_context == 0) {
	tlsp_state_free(state);
	return;
    }

    /*
     * XXX Do we care about TLS session rate limits? Good postscreen(8)
     * clients will occasionally require the tlsproxy to renew their
     * whitelist status, but bad clients hammering the server can suck up
     * lots of CPU cycles. Per-client concurrency limits in postscreen(8)
     * will divert only naive security "researchers".
     * 
     * XXX Do we care about certificate verification results? Not as long as
     * postscreen(8) doesn't actually receive email.
     */
}

/* tlsp_get_fd_event - receive final postscreen(8) hand-off information */

static void tlsp_get_fd_event(int event, char *context)
{
    const char *myname = "tlsp_get_fd_event";
    TLSP_STATE *state = (TLSP_STATE *) context;
    int     plaintext_fd = vstream_fileno(state->plaintext_stream);

    /*
     * At this point we still manually manage plaintext read/write/timeout
     * events. Disable I/O and timer events. Don't assume that the first
     * plaintext request will be a read.
     */
    event_disable_readwrite(plaintext_fd);
    if (event != EVENT_TIME)
	event_cancel_timer(tlsp_get_fd_event, (char *) state);

    /*
     * Initialize plaintext-related session state.  Once we have this behind
     * us, the TLSP_STATE destructor will automagically clean up requests for
     * read/write/timeout events, which makes error recovery easier.
     * 
     * Register the plaintext event handler for timer cleanup in the TLSP_STATE
     * destructor. Insert the NBBIO event-driven I/O layer between the
     * postscreen(8) server and the TLS engine.
     */
    if (event != EVENT_READ
	|| (state->ciphertext_fd = LOCAL_RECV_FD(plaintext_fd)) < 0) {
	msg_warn("%s: receive SMTP client file descriptor: %m", myname);
	tlsp_state_free(state);
	return;
    }
    non_blocking(state->ciphertext_fd, NON_BLOCKING);
    state->ciphertext_timer = tlsp_ciphertext_event;
    state->plaintext_buf = nbbio_create(plaintext_fd,
					VSTREAM_BUFSIZE, "postscreen",
					tlsp_plaintext_event,
					(char *) state);

    /*
     * Perform the TLS layer before-handshake initialization. We perform the
     * remainder after the TLS handshake completes.
     */
    tlsp_start_tls(state);

    /*
     * Trigger the initial proxy server I/Os.
     */
    tlsp_strategy(state);
}

/* tlsp_get_request_event - receive initial postscreen(8) hand-off info */

static void tlsp_get_request_event(int event, char *context)
{
    const char *myname = "tlsp_get_request_event";
    TLSP_STATE *state = (TLSP_STATE *) context;
    VSTREAM *plaintext_stream = state->plaintext_stream;
    int     plaintext_fd = vstream_fileno(plaintext_stream);
    static VSTRING *remote_endpt;
    static VSTRING *server_id;
    int     req_flags;
    int     timeout;
    int     ready;

    /*
     * One-time initialization.
     */
    if (remote_endpt == 0) {
	remote_endpt = vstring_alloc(10);
	server_id = vstring_alloc(10);
    }

    /*
     * At this point we still manually manage plaintext read/write/timeout
     * events. Turn off timer events. Below we disable read events on error,
     * and redefine read events on success.
     */
    if (event != EVENT_TIME)
	event_cancel_timer(tlsp_get_request_event, (char *) state);

    /*
     * We must send some data, after receiving the request attributes and
     * before receiving the remote file descriptor. We can't assume
     * UNIX-domain socket semantics here.
     */
    if (event != EVENT_READ
	|| attr_scan(plaintext_stream, ATTR_FLAG_STRICT,
		     ATTR_TYPE_STR, MAIL_ATTR_REMOTE_ENDPT, remote_endpt,
		     ATTR_TYPE_INT, MAIL_ATTR_FLAGS, &req_flags,
		     ATTR_TYPE_INT, MAIL_ATTR_TIMEOUT, &timeout,
		     ATTR_TYPE_STR, MAIL_ATTR_SERVER_ID, server_id,
		     ATTR_TYPE_END) != 4) {
	msg_warn("%s: receive request attributes: %m", myname);
	event_disable_readwrite(plaintext_fd);
	tlsp_state_free(state);
	return;
    }

    /*
     * If the requested TLS engine is unavailable, hang up after making sure
     * that the plaintext peer has received our "sorry" indication.
     */
    ready = ((req_flags & TLS_PROXY_FLAG_ROLE_SERVER) != 0
	     && tlsp_server_ctx != 0);
    if (attr_print(plaintext_stream, ATTR_FLAG_NONE,
		   ATTR_TYPE_INT, MAIL_ATTR_STATUS, ready,
		   ATTR_TYPE_END) != 0
	|| vstream_fflush(plaintext_stream) != 0
	|| ready == 0) {
	read_wait(plaintext_fd, TLSP_INIT_TIMEOUT);	/* XXX */
	event_disable_readwrite(plaintext_fd);
	tlsp_state_free(state);
	return;
    }

    /*
     * XXX We use the same fixed timeout throughout the entire session for
     * both plaintext and ciphertext communication. This timeout is just a
     * safety feature; the real timeout will be enforced by our plaintext
     * peer.
     */
    else {
	state->remote_endpt = mystrdup(STR(remote_endpt));
	state->server_id = mystrdup(STR(server_id));
	msg_info("CONNECT %s %s",
		 (req_flags & TLS_PROXY_FLAG_ROLE_SERVER) ? "from" :
		 (req_flags & TLS_PROXY_FLAG_ROLE_CLIENT) ? "to" :
		 "(bogus_direction)", state->remote_endpt);
	state->req_flags = req_flags;
	state->timeout = timeout + 10;		/* XXX */
	event_enable_read(plaintext_fd, tlsp_get_fd_event, (char *) state);
	event_request_timer(tlsp_get_fd_event, (char *) state,
			    TLSP_INIT_TIMEOUT);
	return;
    }
}

/* tlsp_service - handle new client connection */

static void tlsp_service(VSTREAM *plaintext_stream,
			         char *service,
			         char **argv)
{
    TLSP_STATE *state;
    int     plaintext_fd = vstream_fileno(plaintext_stream);

    /*
     * Sanity check. This service takes no command-line arguments.
     */
    if (argv[0])
	msg_fatal("unexpected command-line argument: %s", argv[0]);

    /*
     * This program handles multiple connections, so it must not block. We
     * use event-driven code for all operations that introduce latency.
     * Except that attribute lists are sent/received synchronously, once the
     * socket is found to be ready for transmission.
     */
    non_blocking(plaintext_fd, NON_BLOCKING);
    vstream_control(plaintext_stream,
		    VSTREAM_CTL_PATH, "plaintext",
		    VSTREAM_CTL_TIMEOUT, 5,
		    VSTREAM_CTL_END);

    /*
     * Receive postscreen's remote SMTP client address/port and socket.
     */
    state = tlsp_state_create(service, plaintext_stream);
    event_enable_read(plaintext_fd, tlsp_get_request_event, (char *) state);
    event_request_timer(tlsp_get_request_event, (char *) state,
			TLSP_INIT_TIMEOUT);
}

/* pre_jail_init - pre-jail initialization */

static void pre_jail_init(char *unused_name, char **unused_argv)
{
    TLS_SERVER_INIT_PROPS props;
    const char *cert_file;
    int     have_server_cert;
    int     no_server_cert_ok;
    int     require_server_cert;

    /*
     * The code in this routine is pasted literally from smtpd(8). I am not
     * going to sanitize this because doing so surely will break things in
     * unexpected ways.
     */
    if (*var_tlsp_tls_level) {
	switch (tls_level_lookup(var_tlsp_tls_level)) {
	default:
	    msg_fatal("Invalid TLS level \"%s\"", var_tlsp_tls_level);
	    /* NOTREACHED */
	    break;
	case TLS_LEV_SECURE:
	case TLS_LEV_VERIFY:
	case TLS_LEV_FPRINT:
	    msg_warn("%s: unsupported TLS level \"%s\", using \"encrypt\"",
		     VAR_TLSP_TLS_LEVEL, var_tlsp_tls_level);
	    /* FALLTHROUGH */
	case TLS_LEV_ENCRYPT:
	    var_tlsp_enforce_tls = var_tlsp_use_tls = 1;
	    break;
	case TLS_LEV_MAY:
	    var_tlsp_enforce_tls = 0;
	    var_tlsp_use_tls = 1;
	    break;
	case TLS_LEV_NONE:
	    var_tlsp_enforce_tls = var_tlsp_use_tls = 0;
	    break;
	}
    }
    var_tlsp_use_tls = var_tlsp_use_tls || var_tlsp_enforce_tls;
    if (!var_tlsp_use_tls) {
	msg_warn("TLS service is requested, but disabled with %s or %s",
		 VAR_TLSP_TLS_LEVEL, VAR_TLSP_USE_TLS);
	return;
    }

    /*
     * Load TLS keys before dropping privileges.
     * 
     * Can't use anonymous ciphers if we want client certificates. Must use
     * anonymous ciphers if we have no certificates.
     */
    ask_client_cert = require_server_cert =
	(var_tlsp_tls_ask_ccert
	 || (var_tlsp_enforce_tls && var_tlsp_tls_req_ccert));
    if (strcasecmp(var_tlsp_tls_cert_file, "none") == 0) {
	no_server_cert_ok = 1;
	cert_file = "";
    } else {
	no_server_cert_ok = 0;
	cert_file = var_tlsp_tls_cert_file;
    }
    have_server_cert =
	(*cert_file || *var_tlsp_tls_dcert_file || *var_tlsp_tls_eccert_file);

    /* Some TLS configuration errors are not show stoppers. */
    if (!have_server_cert && require_server_cert)
	msg_warn("Need a server cert to request client certs");
    if (!var_tlsp_enforce_tls && var_tlsp_tls_req_ccert)
	msg_warn("Can't require client certs unless TLS is required");
    /* After a show-stopper error, log a warning. */
    if (have_server_cert || (no_server_cert_ok && !require_server_cert))

	/*
	 * Large parameter lists are error-prone, so we emulate a language
	 * feature that C does not have natively: named parameter lists.
	 */
	tlsp_server_ctx =
	    TLS_SERVER_INIT(&props,
			    log_param = VAR_TLSP_TLS_LOGLEVEL,
			    log_level = var_tlsp_tls_loglevel,
			    verifydepth = var_tlsp_tls_ccert_vd,
			    cache_type = TLS_MGR_SCACHE_SMTPD,
			    scache_timeout = var_tlsp_tls_scache_timeout,
			    set_sessid = var_tlsp_tls_set_sessid,
			    cert_file = cert_file,
			    key_file = var_tlsp_tls_key_file,
			    dcert_file = var_tlsp_tls_dcert_file,
			    dkey_file = var_tlsp_tls_dkey_file,
			    eccert_file = var_tlsp_tls_eccert_file,
			    eckey_file = var_tlsp_tls_eckey_file,
			    CAfile = var_tlsp_tls_CAfile,
			    CApath = var_tlsp_tls_CApath,
			    dh1024_param_file
			    = var_tlsp_tls_dh1024_param_file,
			    dh512_param_file
			    = var_tlsp_tls_dh512_param_file,
			    eecdh_grade = var_tlsp_tls_eecdh,
			    protocols = var_tlsp_enforce_tls ?
			    var_tlsp_tls_mand_proto :
			    var_tlsp_tls_proto,
			    ask_ccert = ask_client_cert,
			    fpt_dgst = var_tlsp_tls_fpt_dgst);
    else
	msg_warn("No server certs available. TLS can't be enabled");

    /*
     * To maintain sanity, allow partial SSL_write() operations, and allow
     * SSL_write() buffer pointers to change after a WANT_READ or WANT_WRITE
     * result. This is based on OpenSSL developers talking on a mailing list,
     * but is not supported by documentation. If this code stops working then
     * no-one can be held responsible.
     */
    if (tlsp_server_ctx)
	SSL_CTX_set_mode(tlsp_server_ctx->ssl_ctx,
			 SSL_MODE_ENABLE_PARTIAL_WRITE
			 | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
}

/* post_jail_init - post-jail initialization */

static void post_jail_init(char *unused_name, char **unused_argv)
{
     /* void */ ;
}

MAIL_VERSION_STAMP_DECLARE;

/* main - the main program */

int     main(int argc, char **argv)
{
    static const CONFIG_INT_TABLE int_table[] = {
	VAR_SMTPD_TLS_CCERT_VD, DEF_SMTPD_TLS_CCERT_VD, &var_smtpd_tls_ccert_vd, 0, 0,
	0,
    };
    static const CONFIG_NINT_TABLE nint_table[] = {
	VAR_TLSP_TLS_CCERT_VD, DEF_TLSP_TLS_CCERT_VD, &var_tlsp_tls_ccert_vd, 0, 0,
	0,
    };
    static const CONFIG_TIME_TABLE time_table[] = {
	VAR_SMTPD_TLS_SCACHTIME, DEF_SMTPD_TLS_SCACHTIME, &var_smtpd_tls_scache_timeout, 0, 0,
	VAR_TLSP_WATCHDOG, DEF_TLSP_WATCHDOG, &var_tlsp_watchdog, 10, 0,
	VAR_TLSP_TLS_SCACHTIME, DEF_TLSP_TLS_SCACHTIME, &var_tlsp_tls_scache_timeout, 0, 0,
	0,
    };
    static const CONFIG_BOOL_TABLE bool_table[] = {
	VAR_SMTPD_USE_TLS, DEF_SMTPD_USE_TLS, &var_smtpd_use_tls,
	VAR_SMTPD_ENFORCE_TLS, DEF_SMTPD_ENFORCE_TLS, &var_smtpd_enforce_tls,
	VAR_SMTPD_TLS_ACERT, DEF_SMTPD_TLS_ACERT, &var_smtpd_tls_ask_ccert,
	VAR_SMTPD_TLS_RCERT, DEF_SMTPD_TLS_RCERT, &var_smtpd_tls_req_ccert,
	VAR_SMTPD_TLS_SET_SESSID, DEF_SMTPD_TLS_SET_SESSID, &var_smtpd_tls_set_sessid,
	0,
    };
    static const CONFIG_NBOOL_TABLE nbool_table[] = {
	VAR_TLSP_USE_TLS, DEF_TLSP_USE_TLS, &var_tlsp_use_tls,
	VAR_TLSP_ENFORCE_TLS, DEF_TLSP_ENFORCE_TLS, &var_tlsp_enforce_tls,
	VAR_TLSP_TLS_ACERT, DEF_TLSP_TLS_ACERT, &var_tlsp_tls_ask_ccert,
	VAR_TLSP_TLS_RCERT, DEF_TLSP_TLS_RCERT, &var_tlsp_tls_req_ccert,
	VAR_TLSP_TLS_SET_SESSID, DEF_TLSP_TLS_SET_SESSID, &var_tlsp_tls_set_sessid,
	0,
    };
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_SMTPD_TLS_CERT_FILE, DEF_SMTPD_TLS_CERT_FILE, &var_smtpd_tls_cert_file, 0, 0,
	VAR_SMTPD_TLS_KEY_FILE, DEF_SMTPD_TLS_KEY_FILE, &var_smtpd_tls_key_file, 0, 0,
	VAR_SMTPD_TLS_DCERT_FILE, DEF_SMTPD_TLS_DCERT_FILE, &var_smtpd_tls_dcert_file, 0, 0,
	VAR_SMTPD_TLS_DKEY_FILE, DEF_SMTPD_TLS_DKEY_FILE, &var_smtpd_tls_dkey_file, 0, 0,
	VAR_SMTPD_TLS_ECCERT_FILE, DEF_SMTPD_TLS_ECCERT_FILE, &var_smtpd_tls_eccert_file, 0, 0,
	VAR_SMTPD_TLS_ECKEY_FILE, DEF_SMTPD_TLS_ECKEY_FILE, &var_smtpd_tls_eckey_file, 0, 0,
	VAR_SMTPD_TLS_CA_FILE, DEF_SMTPD_TLS_CA_FILE, &var_smtpd_tls_CAfile, 0, 0,
	VAR_SMTPD_TLS_CA_PATH, DEF_SMTPD_TLS_CA_PATH, &var_smtpd_tls_CApath, 0, 0,
	VAR_SMTPD_TLS_CIPH, DEF_SMTPD_TLS_CIPH, &var_smtpd_tls_ciph, 1, 0,
	VAR_SMTPD_TLS_MAND_CIPH, DEF_SMTPD_TLS_MAND_CIPH, &var_smtpd_tls_mand_ciph, 1, 0,
	VAR_SMTPD_TLS_EXCL_CIPH, DEF_SMTPD_TLS_EXCL_CIPH, &var_smtpd_tls_excl_ciph, 0, 0,
	VAR_SMTPD_TLS_MAND_EXCL, DEF_SMTPD_TLS_MAND_EXCL, &var_smtpd_tls_mand_excl, 0, 0,
	VAR_SMTPD_TLS_PROTO, DEF_SMTPD_TLS_PROTO, &var_smtpd_tls_proto, 0, 0,
	VAR_SMTPD_TLS_MAND_PROTO, DEF_SMTPD_TLS_MAND_PROTO, &var_smtpd_tls_mand_proto, 0, 0,
	VAR_SMTPD_TLS_512_FILE, DEF_SMTPD_TLS_512_FILE, &var_smtpd_tls_dh512_param_file, 0, 0,
	VAR_SMTPD_TLS_1024_FILE, DEF_SMTPD_TLS_1024_FILE, &var_smtpd_tls_dh1024_param_file, 0, 0,
	VAR_SMTPD_TLS_EECDH, DEF_SMTPD_TLS_EECDH, &var_smtpd_tls_eecdh, 1, 0,
	VAR_SMTPD_TLS_FPT_DGST, DEF_SMTPD_TLS_FPT_DGST, &var_smtpd_tls_fpt_dgst, 1, 0,
	VAR_SMTPD_TLS_LOGLEVEL, DEF_SMTPD_TLS_LOGLEVEL, &var_smtpd_tls_loglevel, 0, 0,
	VAR_SMTPD_TLS_LEVEL, DEF_SMTPD_TLS_LEVEL, &var_smtpd_tls_level, 0, 0,
	VAR_TLSP_TLS_CERT_FILE, DEF_TLSP_TLS_CERT_FILE, &var_tlsp_tls_cert_file, 0, 0,
	VAR_TLSP_TLS_KEY_FILE, DEF_TLSP_TLS_KEY_FILE, &var_tlsp_tls_key_file, 0, 0,
	VAR_TLSP_TLS_DCERT_FILE, DEF_TLSP_TLS_DCERT_FILE, &var_tlsp_tls_dcert_file, 0, 0,
	VAR_TLSP_TLS_DKEY_FILE, DEF_TLSP_TLS_DKEY_FILE, &var_tlsp_tls_dkey_file, 0, 0,
	VAR_TLSP_TLS_ECCERT_FILE, DEF_TLSP_TLS_ECCERT_FILE, &var_tlsp_tls_eccert_file, 0, 0,
	VAR_TLSP_TLS_ECKEY_FILE, DEF_TLSP_TLS_ECKEY_FILE, &var_tlsp_tls_eckey_file, 0, 0,
	VAR_TLSP_TLS_CA_FILE, DEF_TLSP_TLS_CA_FILE, &var_tlsp_tls_CAfile, 0, 0,
	VAR_TLSP_TLS_CA_PATH, DEF_TLSP_TLS_CA_PATH, &var_tlsp_tls_CApath, 0, 0,
	VAR_TLSP_TLS_CIPH, DEF_TLSP_TLS_CIPH, &var_tlsp_tls_ciph, 1, 0,
	VAR_TLSP_TLS_MAND_CIPH, DEF_TLSP_TLS_MAND_CIPH, &var_tlsp_tls_mand_ciph, 1, 0,
	VAR_TLSP_TLS_EXCL_CIPH, DEF_TLSP_TLS_EXCL_CIPH, &var_tlsp_tls_excl_ciph, 0, 0,
	VAR_TLSP_TLS_MAND_EXCL, DEF_TLSP_TLS_MAND_EXCL, &var_tlsp_tls_mand_excl, 0, 0,
	VAR_TLSP_TLS_PROTO, DEF_TLSP_TLS_PROTO, &var_tlsp_tls_proto, 0, 0,
	VAR_TLSP_TLS_MAND_PROTO, DEF_TLSP_TLS_MAND_PROTO, &var_tlsp_tls_mand_proto, 0, 0,
	VAR_TLSP_TLS_512_FILE, DEF_TLSP_TLS_512_FILE, &var_tlsp_tls_dh512_param_file, 0, 0,
	VAR_TLSP_TLS_1024_FILE, DEF_TLSP_TLS_1024_FILE, &var_tlsp_tls_dh1024_param_file, 0, 0,
	VAR_TLSP_TLS_EECDH, DEF_TLSP_TLS_EECDH, &var_tlsp_tls_eecdh, 1, 0,
	VAR_TLSP_TLS_FPT_DGST, DEF_TLSP_TLS_FPT_DGST, &var_tlsp_tls_fpt_dgst, 1, 0,
	VAR_TLSP_TLS_LOGLEVEL, DEF_TLSP_TLS_LOGLEVEL, &var_tlsp_tls_loglevel, 0, 0,
	VAR_TLSP_TLS_LEVEL, DEF_TLSP_TLS_LEVEL, &var_tlsp_tls_level, 0, 0,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

    /*
     * Pass control to the single-threaded service skeleton.
     */
    event_server_main(argc, argv, tlsp_service,
		      MAIL_SERVER_INT_TABLE, int_table,
		      MAIL_SERVER_NINT_TABLE, nint_table,
		      MAIL_SERVER_STR_TABLE, str_table,
		      MAIL_SERVER_BOOL_TABLE, bool_table,
		      MAIL_SERVER_NBOOL_TABLE, nbool_table,
		      MAIL_SERVER_TIME_TABLE, time_table,
		      MAIL_SERVER_PRE_INIT, pre_jail_init,
		      MAIL_SERVER_POST_INIT, post_jail_init,
		      MAIL_SERVER_SLOW_EXIT, tlsp_drain,
		      MAIL_SERVER_WATCHDOG, &var_tlsp_watchdog,
		      0);
}

#else

/* tlsp_service - respond to external trigger(s), non-TLS version */

static void tlsp_service(VSTREAM *stream, char *unused_service,
			         char **unused_argv)
{
    msg_info("TLS support is not compiled in -- exiting");
    event_server_disconnect(stream);
}

/* main - the main program */

int     main(int argc, char **argv)
{

    /*
     * We can't simply use msg_fatal() here, because the logging hasn't been
     * initialized. The text would disappear because stderr is redirected to
     * /dev/null.
     * 
     * We invoke event_server_main() to complete program initialization
     * (including logging) and then invoke the tlsp_service() routine to log
     * the message that says why this program will not run.
     */
    event_server_main(argc, argv, tlsp_service,
		      0);
}

#endif
