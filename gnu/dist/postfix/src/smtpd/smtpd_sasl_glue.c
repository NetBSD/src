/*++
/* NAME
/*	smtpd_sasl_glue 3
/* SUMMARY
/*	Postfix SMTP server, SASL support interface
/* SYNOPSIS
/*	#include "smtpd_sasl_glue.h"
/*
/*	void	smtpd_sasl_initialize()
/*
/*	void	smtpd_sasl_connect(state)
/*	SMTPD_STATE *state;
/*
/*	char	*smtpd_sasl_authenticate(state, sasl_method, init_response)
/*	SMTPD_STATE *state;
/*	const char *sasl_method;
/*	const char *init_response;
/*
/*	void	smtpd_sasl_logout(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_sasl_disconnect(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	This module encapsulates most of the detail specific to SASL
/*	authentication.
/*
/*	smtpd_sasl_initialize() initializes the SASL library. This
/*	routine should be called once at process start-up. It may
/*	need access to the file system for run-time loading of
/*	plug-in modules. There is no corresponding cleanup routine.
/*
/*	smtpd_sasl_connect() performs per-connection initialization.
/*	This routine should be called once at the start of every
/*	connection.
/*
/*	smtpd_sasl_authenticate() implements the authentication dialog.
/*	The result is a null pointer in case of success, an SMTP reply
/*	in case of failure. smtpd_sasl_authenticate() updates the
/*	following state structure members:
/* .IP sasl_method
/*	The authentication method that was successfully applied.
/*	This member is a null pointer in the absence of successful
/*	authentication.
/* .IP sasl_username
/*	The username that was successfully authenticated.
/*	This member is a null pointer in the absence of successful
/*	authentication.
/* .PP
/*	smtpd_sasl_logout() cleans up after smtpd_sasl_authenticate().
/*	This routine exists for the sake of symmetry.
/*
/*	smtpd_sasl_disconnect() performs per-connection cleanup.
/*	This routine should be called at the end of every connection.
/*
/*	Arguments:
/* .IP state
/*	SMTP session context.
/* .IP sasl_method
/*	A SASL mechanism name
/* .IP init_reply
/*	An optional initial client response.
/* DIAGNOSTICS
/*	All errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Initial implementation by:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	Adopted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <namadr_list.h>
#include <name_mask.h>

/* Global library. */

#include <mail_params.h>
#include <smtp_stream.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_sasl_glue.h"
#include "smtpd_chat.h"

#ifdef USE_SASL_AUTH

/*
 * Silly little macros.
 */
#define STR(s)	vstring_str(s)

 /*
  * Macros to handle API differences between SASLv1 and SASLv2. Specifics:
  * 
  * The SASL_LOG_* constants were renamed in SASLv2.
  * 
  * SASLv2's sasl_server_new takes two new parameters to specify local and
  * remote IP addresses for auth mechs that use them.
  * 
  * SASLv2's sasl_server_start and sasl_server_step no longer have the errstr
  * parameter.
  * 
  * SASLv2's sasl_decode64 function takes an extra parameter for the length of
  * the output buffer.
  * 
  * The other major change is that SASLv2 now takes more responsibility for
  * deallocating memory that it allocates internally.  Thus, some of the
  * function parameters are now 'const', to make sure we don't try to free
  * them too.  This is dealt with in the code later on.
  */

#if SASL_VERSION_MAJOR < 2
/* SASL version 1.x */
#define SASL_LOG_WARN SASL_LOG_WARNING
#define SASL_LOG_NOTE SASL_LOG_INFO
#define SASL_SERVER_NEW(srv, fqdn, rlm, lport, rport, cb, secflags, pconn) \
	sasl_server_new(srv, fqdn, rlm, cb, secflags, pconn)
#define SASL_SERVER_START(conn, mech, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_start(conn, mech, clin, clinlen, srvout, srvoutlen, err)
#define SASL_SERVER_STEP(conn, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_step(conn, clin, clinlen, srvout, srvoutlen, err)
#define SASL_DECODE64(in, inlen, out, outmaxlen, outlen) \
	sasl_decode64(in, inlen, out, outlen)
#endif

#if SASL_VERSION_MAJOR >= 2
/* SASL version > 2.x */
#define SASL_SERVER_NEW(srv, fqdn, rlm, lport, rport, cb, secflags, pconn) \
	sasl_server_new(srv, fqdn, rlm, lport, rport, cb, secflags, pconn)
#define SASL_SERVER_START(conn, mech, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_start(conn, mech, clin, clinlen, srvout, srvoutlen)
#define SASL_SERVER_STEP(conn, clin, clinlen, srvout, srvoutlen, err) \
	sasl_server_step(conn, clin, clinlen, srvout, srvoutlen)
#define SASL_DECODE64(in, inlen, out, outmaxlen, outlen) \
	sasl_decode64(in, inlen, out, outmaxlen, outlen)
#endif

/* smtpd_sasl_log - SASL logging callback */

static int smtpd_sasl_log(void *unused_context, int priority,
			          const char *message)
{
    switch (priority) {
	case SASL_LOG_ERR:
	case SASL_LOG_WARN:
	msg_warn("SASL authentication problem: %s", message);
	break;
    case SASL_LOG_NOTE:
	if (msg_verbose)
	    msg_info("SASL authentication info: %s", message);
	break;
#if SASL_VERSION_MAJOR >= 2
    case SASL_LOG_FAIL:
	msg_warn("SASL authentication failure: %s", message);
	break;
#endif
    }
    return SASL_OK;
}

 /*
  * SASL callback interface structure. These call-backs have no per-session
  * context.
  */
#define NO_CALLBACK_CONTEXT	0

static sasl_callback_t callbacks[] = {
    {SASL_CB_LOG, &smtpd_sasl_log, NO_CALLBACK_CONTEXT},
    {SASL_CB_LIST_END, 0, 0}
};

static NAME_MASK smtpd_sasl_mask[] = {
    "noplaintext", SASL_SEC_NOPLAINTEXT,
    "noactive", SASL_SEC_NOACTIVE,
    "nodictionary", SASL_SEC_NODICTIONARY,
    "noanonymous", SASL_SEC_NOANONYMOUS,
#if SASL_VERSION_MAJOR >= 2
    "mutual_auth", SASL_SEC_MUTUAL_AUTH,
#endif
    0,
};

static int smtpd_sasl_opts;

/* smtpd_sasl_initialize - per-process initialization */

void    smtpd_sasl_initialize(void)
{

    /*
     * Initialize the library: load SASL plug-in routines, etc.
     */
    if (sasl_server_init(callbacks, "smtpd") != SASL_OK)
	msg_fatal("SASL per-process initialization failed");

    /*
     * Configuration parameters.
     */
    smtpd_sasl_opts = name_mask(VAR_SMTPD_SASL_OPTS, smtpd_sasl_mask,
				var_smtpd_sasl_opts);
}

/* smtpd_sasl_connect - per-connection initialization */

void    smtpd_sasl_connect(SMTPD_STATE *state)
{
#if SASL_VERSION_MAJOR < 2
    unsigned sasl_mechanism_count;

#else
    int     sasl_mechanism_count;

#endif
    sasl_security_properties_t sec_props;
    char   *server_address;
    char   *client_address;

    /*
     * Initialize SASL-specific state variables. Use long-lived storage for
     * base 64 conversion results, rather than local variables, to avoid
     * memory leaks when a read or write routine returns abnormally after
     * timeout or I/O error.
     */
    state->sasl_mechanism_list = 0;
    state->sasl_username = 0;
    state->sasl_method = 0;
    state->sasl_sender = 0;
    state->sasl_conn = 0;
    state->sasl_decoded = vstring_alloc(10);
    state->sasl_encoded = vstring_alloc(10);

    /*
     * Set up a new server context for this connection.
     */
#define NO_SECURITY_LAYERS	(0)
#define NO_SESSION_CALLBACKS	((sasl_callback_t *) 0)
#define NO_AUTH_REALM		((char *) 0)

#if SASL_VERSION_MAJOR >= 2 && defined(USE_SASL_IP_AUTH)

    /*
     * Get IP addresses of local and remote endpoints for SASL.
     */
#error "USE_SASL_IP_AUTH is not implemented"

#else

    /*
     * Don't give any IP address information to SASL.  SASLv1 doesn't use it,
     * and in SASLv2 this will disable any mechaniams that do.
     */
    server_address = 0;
    client_address = 0;
#endif

    if (SASL_SERVER_NEW("smtp", var_myhostname, *var_smtpd_sasl_realm ?
			var_smtpd_sasl_realm : NO_AUTH_REALM,
			server_address, client_address,
			NO_SESSION_CALLBACKS, NO_SECURITY_LAYERS,
			&state->sasl_conn) != SASL_OK)
	msg_fatal("SASL per-connection server initialization");

    /*
     * Security options. Some information can be found in the sasl.h include
     * file. Disallow anonymous authentication; this is because the
     * permit_sasl_authenticated feature is restricted to authenticated
     * clients only.
     */
    memset(&sec_props, 0, sizeof(sec_props));
    sec_props.min_ssf = 0;
    sec_props.max_ssf = 1;			/* don't allow real SASL
						 * security layer */
    sec_props.security_flags = smtpd_sasl_opts;
    sec_props.maxbufsize = 0;
    sec_props.property_names = 0;
    sec_props.property_values = 0;

    if (sasl_setprop(state->sasl_conn, SASL_SEC_PROPS,
		     &sec_props) != SASL_OK)
	msg_fatal("SASL per-connection security setup");

    /*
     * Get the list of authentication mechanisms.
     */
#define UNSUPPORTED_USER	((char *) 0)
#define IGNORE_MECHANISM_LEN	((unsigned *) 0)

    if (sasl_listmech(state->sasl_conn, UNSUPPORTED_USER,
		      "", " ", "",
		      &state->sasl_mechanism_list,
		      IGNORE_MECHANISM_LEN,
		      &sasl_mechanism_count) != SASL_OK)
	msg_fatal("cannot lookup SASL authentication mechanisms");
    if (sasl_mechanism_count <= 0)
	msg_fatal("no SASL authentication mechanisms");
}

/* smtpd_sasl_disconnect - per-connection cleanup */

void    smtpd_sasl_disconnect(SMTPD_STATE *state)
{
    if (state->sasl_mechanism_list) {
#if SASL_VERSION_MAJOR < 2
	/* SASL version 1 doesn't free memory that it allocates. */
	free(state->sasl_mechanism_list);
#endif
	state->sasl_mechanism_list = 0;
    }
    if (state->sasl_conn) {
	sasl_dispose(&state->sasl_conn);
	state->sasl_conn = 0;
    }
    vstring_free(state->sasl_decoded);
    vstring_free(state->sasl_encoded);
}

/* smtpd_sasl_authenticate - per-session authentication */

char   *smtpd_sasl_authenticate(SMTPD_STATE *state,
				        const char *sasl_method,
				        const char *init_response)
{
    char   *myname = "smtpd_sasl_authenticate";
    char   *dec_buffer;
    unsigned dec_length;
    unsigned enc_length;
    unsigned enc_length_out;
    unsigned reply_len;
    unsigned serveroutlen;
    int     result;

#if SASL_VERSION_MAJOR < 2
    char   *serverout = 0;

#else
    const char *serverout = 0;

#endif

#if SASL_VERSION_MAJOR < 2
    const char *errstr = 0;

#endif

#define IFELSE(e1,e2,e3) ((e1) ? (e2) : (e3))

    if (msg_verbose)
	msg_info("%s: sasl_method %s%s%s", myname, sasl_method,
		 IFELSE(init_response, ", init_response ", ""),
		 IFELSE(init_response, init_response, ""));

    /*
     * Sanity check.
     */
    if (state->sasl_username || state->sasl_method)
	msg_panic("%s: already authenticated", myname);

    /*
     * SASL authentication protocol start-up. Process any initial client
     * response that was sent along in the AUTH command.
     */
    if (init_response) {
	reply_len = strlen(init_response);
	VSTRING_SPACE(state->sasl_decoded, reply_len);
	dec_buffer = STR(state->sasl_decoded);
	if (SASL_DECODE64(init_response, reply_len,
			  dec_buffer, reply_len, &dec_length) != SASL_OK)
	    return ("501 Authentication failed: malformed initial response");
	if (msg_verbose)
	    msg_info("%s: decoded initial response %s", myname, dec_buffer);
    } else {
	dec_buffer = 0;
	dec_length = 0;
    }
    result = SASL_SERVER_START(state->sasl_conn, sasl_method, dec_buffer,
			    dec_length, &serverout, &serveroutlen, &errstr);

    /*
     * Repeat until done or until the client gives up.
     */
    while (result == SASL_CONTINUE) {

	/*
	 * Send a server challenge. Avoid storing the challenge in a local
	 * variable, because we would leak memory when smtpd_chat_reply()
	 * does not return due to timeout or I/O error. sasl_encode64()
	 * null-terminates the result if the result buffer is large enough.
	 * 
	 * Regarding the hairy expression below: output from sasl_encode64()
	 * comes in multiples of four bytes for each triple of input bytes,
	 * plus four bytes for any incomplete last triple, plus one byte for
	 * the null terminator.
	 * 
	 * XXX Replace the klunky sasl_encode64() interface by something that
	 * uses VSTRING buffers.
	 */
	if (msg_verbose)
	    msg_info("%s: uncoded challenge: %.*s",
		     myname, (int) serveroutlen, serverout);
	enc_length = ((serveroutlen + 2) / 3) * 4 + 1;
	VSTRING_SPACE(state->sasl_encoded, enc_length);
	if (sasl_encode64(serverout, serveroutlen, STR(state->sasl_encoded),
			  enc_length, &enc_length_out) != SASL_OK)
	    msg_panic("%s: sasl_encode64 botch", myname);
#if SASL_VERSION_MAJOR < 2
	/* SASL version 1 doesn't free memory that it allocates. */
	free(serverout);
#endif
	serverout = 0;
	smtpd_chat_reply(state, "334 %s", STR(state->sasl_encoded));

	/*
	 * Receive the client response. "*" means that the client gives up.
	 * XXX For now we ignore the fact that excessively long responses
	 * will be truncated. To handle such responses, we need to change
	 * smtpd_chat_query() so that it returns an error indication.
	 */
	smtpd_chat_query(state);
	if (strcmp(vstring_str(state->buffer), "*") == 0)
	    return ("501 Authentication aborted");	/* XXX */
	reply_len = VSTRING_LEN(state->buffer);
	VSTRING_SPACE(state->sasl_decoded, reply_len);
	if (SASL_DECODE64(vstring_str(state->buffer), reply_len,
			  STR(state->sasl_decoded), reply_len,
			  &dec_length) != SASL_OK)
	    return ("501 Error: malformed authentication response");
	if (msg_verbose)
	    msg_info("%s: decoded response: %.*s",
		     myname, (int) dec_length, STR(state->sasl_decoded));
	result = SASL_SERVER_STEP(state->sasl_conn, STR(state->sasl_decoded),
			    dec_length, &serverout, &serveroutlen, &errstr);
    }

    /*
     * Cleanup. What an awful interface.
     */
#if SASL_VERSION_MAJOR < 2
    if (serverout)
	free(serverout);
#endif

    /*
     * The authentication protocol was completed.
     */
    if (result != SASL_OK)
	return ("535 Error: authentication failed");

    /*
     * Authentication succeeded. Find out the login name for logging and for
     * accounting purposes. For the sake of completeness we also record the
     * authentication method that was used. XXX Do not free(serverout).
     */
#if SASL_VERSION_MAJOR >= 2
    result = sasl_getprop(state->sasl_conn, SASL_USERNAME,
			  (const void **) &serverout);
#else
    result = sasl_getprop(state->sasl_conn, SASL_USERNAME,
			  (void **) &serverout);
#endif
    if (result != SASL_OK || serverout == 0)
	msg_panic("%s: sasl_getprop SASL_USERNAME botch", myname);
    state->sasl_username = mystrdup(serverout);
    state->sasl_method = mystrdup(sasl_method);

    return (0);
}

/* smtpd_sasl_logout - clean up after smtpd_sasl_authenticate */

void    smtpd_sasl_logout(SMTPD_STATE *state)
{
    if (state->sasl_username) {
	myfree(state->sasl_username);
	state->sasl_username = 0;
    }
    if (state->sasl_method) {
	myfree(state->sasl_method);
	state->sasl_method = 0;
    }
}

#endif
