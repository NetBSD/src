/*++
/* NAME
/*	lmtp_sasl 3
/* SUMMARY
/*	Postfix SASL interface for LMTP client
/* SYNOPSIS
/*	#include lmtp_sasl.h
/*
/*	void	lmtp_sasl_initialize()
/*
/*	void	lmtp_sasl_connect(state)
/*	LMTP_STATE *state;
/*
/*	void	lmtp_sasl_start(state)
/*	LMTP_STATE *state;
/*
/*	int     lmtp_sasl_passwd_lookup(state)
/*	LMTP_STATE *state;
/*
/*	int	lmtp_sasl_authenticate(state, why)
/*	LMTP_STATE *state;
/*	VSTRING *why;
/*
/*	void	lmtp_sasl_cleanup(state)
/*	LMTP_STATE *state;
/* DESCRIPTION
/*	lmtp_sasl_initialize() initializes the SASL library. This
/*	routine must be called once at process startup, before any
/*	chroot operations.
/*
/*	lmtp_sasl_connect() performs per-session initialization. This
/*	routine must be called once at the start of each connection.
/*
/*	lmtp_sasl_start() performs per-session initialization. This
/*	routine must be called once per session before doing any SASL
/*	authentication.
/*
/*	lmtp_sasl_passwd_lookup() looks up the username/password
/*	for the current SMTP server. The result is zero in case
/*	of failure.
/*
/*	lmtp_sasl_authenticate() implements the SASL authentication
/*	dialog. The result is < 0 in case of protocol failure, zero in
/*	case of unsuccessful authentication, > 0 in case of success.
/*	The why argument is updated with a reason for failure.
/*	This routine must be called only when lmtp_sasl_passwd_lookup()
/*	suceeds.
/*
/*	lmtp_sasl_cleanup() cleans up. It must be called at the
/*	end of every SMTP session that uses SASL authentication.
/*	This routine is a noop for non-SASL sessions.
/*
/*	Arguments:
/* .IP state
/*	Session context.
/* .IP mech_list
/*	String of SASL mechanisms (separated by blanks)
/* DIAGNOSTICS
/*	All errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Original author:
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

 /*
  * System library.
  */
#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

 /*
  * Utility library
  */
#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <split_at.h>
#include <name_mask.h>

 /*
  * Global library
  */
#include <mail_params.h>
#include <string_list.h>
#include <maps.h>

 /*
  * Application-specific
  */
#include "lmtp.h"
#include "lmtp_sasl.h"

#ifdef USE_SASL_AUTH

 /*
  * Authentication security options.
  */
static NAME_MASK lmtp_sasl_sec_mask[] = {
    "noplaintext", SASL_SEC_NOPLAINTEXT,
    "noactive", SASL_SEC_NOACTIVE,
    "nodictionary", SASL_SEC_NODICTIONARY,
    "noanonymous", SASL_SEC_NOANONYMOUS,
    0,
};

static int lmtp_sasl_sec_opts;

 /*
  * Silly little macros.
  */
#define STR(x)	vstring_str(x)

 /*
  * Per-host login/password information.
  */
static MAPS *lmtp_sasl_passwd_map;

/* lmtp_sasl_log - logging call-back routine */

static int lmtp_sasl_log(void *unused_context, int priority,
			         const char *message)
{
    switch (priority) {
	case SASL_LOG_ERR:
	case SASL_LOG_WARNING:
	msg_warn("%s", message);
	break;
    case SASL_LOG_INFO:
	if (msg_verbose)
	    msg_info("%s", message);
	break;
    }
    return (SASL_OK);
}

/* lmtp_sasl_get_user - username lookup call-back routine */

static int lmtp_sasl_get_user(void *context, int unused_id, const char **result,
			              unsigned *len)
{
    char   *myname = "lmtp_sasl_get_user";
    LMTP_STATE *state = (LMTP_STATE *) context;

    if (msg_verbose)
	msg_info("%s: %s", myname, state->sasl_username);

    /*
     * Sanity check.
     */
    if (state->sasl_passwd == 0)
	msg_panic("%s: no username looked up", myname);

    *result = state->sasl_username;
    if (len)
	*len = strlen(state->sasl_username);
    return (SASL_OK);
}

/* lmtp_sasl_get_passwd - password lookup call-back routine */

static int lmtp_sasl_get_passwd(sasl_conn_t *conn, void *context,
				        int id, sasl_secret_t **psecret)
{
    char   *myname = "lmtp_sasl_get_passwd";
    LMTP_STATE *state = (LMTP_STATE *) context;
    int     len;

    if (msg_verbose)
	msg_info("%s: %s", myname, state->sasl_passwd);

    /*
     * Sanity check.
     */
    if (!conn || !psecret || id != SASL_CB_PASS)
	return (SASL_BADPARAM);
    if (state->sasl_passwd == 0)
	msg_panic("%s: no password looked up", myname);

    /*
     * Convert the password into a counted string.
     */
    len = strlen(state->sasl_passwd);
    if ((*psecret = (sasl_secret_t *) malloc(sizeof(sasl_secret_t) + len)) == 0)
	return (SASL_NOMEM);
    (*psecret)->len = len;
    memcpy((*psecret)->data, state->sasl_passwd, len + 1);

    return (SASL_OK);
}

/* lmtp_sasl_passwd_lookup - password lookup routine */

int     lmtp_sasl_passwd_lookup(LMTP_STATE *state)
{
    char   *myname = "lmtp_sasl_passwd_lookup";
    const char *value;
    char   *passwd;

    /*
     * Sanity check.
     */
    if (lmtp_sasl_passwd_map == 0)
	msg_panic("%s: passwd map not initialized", myname);

    /*
     * Look up the per-server password information. Try the hostname first,
     * then try the destination.
     */
    if ((value = maps_find(lmtp_sasl_passwd_map, state->session->host, 0)) != 0
	|| (value = maps_find(lmtp_sasl_passwd_map, state->request->nexthop, 0)) != 0) {
	state->sasl_username = mystrdup(value);
	passwd = split_at(state->sasl_username, ':');
	state->sasl_passwd = mystrdup(passwd ? passwd : "");
	if (msg_verbose)
	    msg_info("%s: host `%s' user `%s' pass `%s'",
		     myname, state->session->host,
		     state->sasl_username, state->sasl_passwd);
	return (1);
    } else {
	if (msg_verbose)
	    msg_info("%s: host `%s' no auth info found",
		     myname, state->session->host);
	return (0);
    }
}

/* lmtp_sasl_initialize - per-process initialization (pre jail) */

void    lmtp_sasl_initialize(void)
{

    /*
     * Global callbacks. These have no per-session context.
     */
    static sasl_callback_t callbacks[] = {
	{SASL_CB_LOG, &lmtp_sasl_log, 0},
	{SASL_CB_LIST_END, 0, 0}
    };

    /*
     * Sanity check.
     */
    if (lmtp_sasl_passwd_map)
	msg_panic("lmtp_sasl_initialize: repeated call");
    if (*var_lmtp_sasl_passwd == 0)
	msg_fatal("specify a password table via the `%s' configuration parameter",
		  VAR_LMTP_SASL_PASSWD);

    /*
     * Open the per-host password table and initialize the SASL library. Use
     * shared locks for reading, just in case someone updates the table.
     */
    lmtp_sasl_passwd_map = maps_create("lmtp_sasl_passwd",
				       var_lmtp_sasl_passwd, DICT_FLAG_LOCK);
    if (sasl_client_init(callbacks) != SASL_OK)
	msg_fatal("SASL library initialization");

    /*
     * Configuration parameters.
     */
    lmtp_sasl_sec_opts = name_mask(VAR_LMTP_SASL_OPTS, lmtp_sasl_sec_mask,
				   var_lmtp_sasl_opts);
}

/* lmtp_sasl_connect - per-session client initialization */

void    lmtp_sasl_connect(LMTP_STATE *state)
{
    state->sasl_mechanism_list = 0;
    state->sasl_username = 0;
    state->sasl_passwd = 0;
    state->sasl_conn = 0;
    state->sasl_encoded = 0;
    state->sasl_decoded = 0;
    state->sasl_callbacks = 0;
}

/* lmtp_sasl_start - per-session SASL initialization */

void    lmtp_sasl_start(LMTP_STATE *state)
{
    static sasl_callback_t callbacks[] = {
	{SASL_CB_USER, &lmtp_sasl_get_user, 0},
	{SASL_CB_AUTHNAME, &lmtp_sasl_get_user, 0},
	{SASL_CB_PASS, &lmtp_sasl_get_passwd, 0},
	{SASL_CB_LIST_END, 0, 0}
    };
    sasl_callback_t *cp;
    sasl_security_properties_t sec_props;

    if (msg_verbose)
	msg_info("starting new SASL client");

    /*
     * Per-session initialization. Provide each session with its own callback
     * context.
     */
#define NULL_SECFLAGS		0

    state->sasl_callbacks = (sasl_callback_t *) mymalloc(sizeof(callbacks));
    memcpy((char *) state->sasl_callbacks, callbacks, sizeof(callbacks));
    for (cp = state->sasl_callbacks; cp->id != SASL_CB_LIST_END; cp++)
	cp->context = (void *) state;
    if (sasl_client_new("smtp", state->session->host,
			state->sasl_callbacks, NULL_SECFLAGS,
			(sasl_conn_t **) &state->sasl_conn) != SASL_OK)
	msg_fatal("per-session SASL client initialization");

    /*
     * Per-session security properties. XXX This routine is not sufficiently
     * documented. What is the purpose of all this?
     */
    memset(&sec_props, 0L, sizeof(sec_props));
    sec_props.min_ssf = 0;
    sec_props.max_ssf = 1;			/* don't allow real SASL
						 * security layer */
    sec_props.security_flags = lmtp_sasl_sec_opts;
    sec_props.maxbufsize = 0;
    sec_props.property_names = 0;
    sec_props.property_values = 0;
    if (sasl_setprop(state->sasl_conn, SASL_SEC_PROPS,
		     &sec_props) != SASL_OK)
	msg_fatal("set per-session SASL security properties");

    /*
     * We use long-lived conversion buffers rather than local variables in
     * order to avoid memory leaks in case of read/write timeout or I/O
     * error.
     */
    state->sasl_encoded = vstring_alloc(10);
    state->sasl_decoded = vstring_alloc(10);
}

/* lmtp_sasl_authenticate - run authentication protocol */

int     lmtp_sasl_authenticate(LMTP_STATE *state, VSTRING *why)
{
    char   *myname = "lmtp_sasl_authenticate";
    unsigned enc_length;
    unsigned enc_length_out;
    char   *clientout;
    unsigned clientoutlen;
    unsigned serverinlen;
    LMTP_RESP *resp;
    const char *mechanism;
    int     result;
    char   *line;

#define NO_SASL_SECRET		0
#define NO_SASL_INTERACTION	0
#define NO_SASL_LANGLIST	((const char *) 0)
#define NO_SASL_OUTLANG		((const char **) 0)

    if (msg_verbose)
	msg_info("%s: %s: SASL mechanisms %s",
	       myname, state->session->namaddr, state->sasl_mechanism_list);

    /*
     * Start the client side authentication protocol.
     */
    result = sasl_client_start((sasl_conn_t *) state->sasl_conn,
			       state->sasl_mechanism_list,
			       NO_SASL_SECRET, NO_SASL_INTERACTION,
			       &clientout, &clientoutlen, &mechanism);
    if (result != SASL_OK && result != SASL_CONTINUE) {
	vstring_sprintf(why, "cannot SASL authenticate to server %s: %s",
			state->session->namaddr,
			sasl_errstring(result, NO_SASL_LANGLIST,
				       NO_SASL_OUTLANG));
	return (-1);
    }

    /*
     * Send the AUTH command and the optional initial client response.
     * sasl_encode64() produces four bytes for each complete or incomplete
     * triple of input bytes. Allocate an extra byte for string termination.
     */
#define ENCODE64_LENGTH(n)	((((n) + 2) / 3) * 4)

    if (clientoutlen > 0) {
	if (msg_verbose)
	    msg_info("%s: %s: uncoded initial reply: %.*s",
		     myname, state->session->namaddr,
		     (int) clientoutlen, clientout);
	enc_length = ENCODE64_LENGTH(clientoutlen) + 1;
	VSTRING_SPACE(state->sasl_encoded, enc_length);
	if (sasl_encode64(clientout, clientoutlen,
			  STR(state->sasl_encoded), enc_length,
			  &enc_length_out) != SASL_OK)
	    msg_panic("%s: sasl_encode64 botch", myname);
	free(clientout);
	lmtp_chat_cmd(state, "AUTH %s %s", mechanism, STR(state->sasl_encoded));
    } else {
	lmtp_chat_cmd(state, "AUTH %s", mechanism);
    }

    /*
     * Step through the authentication protocol until the server tells us
     * that we are done.
     */
    while ((resp = lmtp_chat_resp(state))->code / 100 == 3) {

	/*
	 * Process a server challenge.
	 */
	line = resp->str;
	(void) mystrtok(&line, "- \t\n");	/* skip over result code */
	serverinlen = strlen(line);
	VSTRING_SPACE(state->sasl_decoded, serverinlen);
	if (sasl_decode64(line, serverinlen,
			STR(state->sasl_decoded), &enc_length) != SASL_OK) {
	    vstring_sprintf(why, "malformed SASL challenge from server %s",
			    state->session->namaddr);
	    return (-1);
	}
	if (msg_verbose)
	    msg_info("%s: %s: decoded challenge: %.*s",
		     myname, state->session->namaddr,
		     (int) enc_length, STR(state->sasl_decoded));
	result = sasl_client_step((sasl_conn_t *) state->sasl_conn,
				  STR(state->sasl_decoded), enc_length,
			    NO_SASL_INTERACTION, &clientout, &clientoutlen);
	if (result != SASL_OK && result != SASL_CONTINUE)
	    msg_warn("SASL authentication failed to server %s: %s",
		     state->session->namaddr,
		     sasl_errstring(result, NO_SASL_LANGLIST,
				    NO_SASL_OUTLANG));

	/*
	 * Send a client response.
	 */
	if (clientoutlen > 0) {
	    if (msg_verbose)
		msg_info("%s: %s: uncoded client response %.*s",
			 myname, state->session->namaddr,
			 (int) clientoutlen, clientout);
	    enc_length = ENCODE64_LENGTH(clientoutlen) + 1;
	    VSTRING_SPACE(state->sasl_encoded, enc_length);
	    if (sasl_encode64(clientout, clientoutlen,
			      STR(state->sasl_encoded), enc_length,
			      &enc_length_out) != SASL_OK)
		msg_panic("%s: sasl_encode64 botch", myname);
	    free(clientout);
	} else {
	    vstring_strcat(state->sasl_encoded, "");
	}
	lmtp_chat_cmd(state, "%s", STR(state->sasl_encoded));
    }

    /*
     * We completed the authentication protocol.
     */
    if (resp->code / 100 != 2) {
	vstring_sprintf(why, "SASL authentication failed; server %s said: %s",
			state->session->namaddr, resp->str);
	return (0);
    }
    return (1);
}

/* lmtp_sasl_cleanup - per-session cleanup */

void    lmtp_sasl_cleanup(LMTP_STATE *state)
{
    if (state->sasl_username) {
	myfree(state->sasl_username);
	state->sasl_username = 0;
    }
    if (state->sasl_passwd) {
	myfree(state->sasl_passwd);
	state->sasl_passwd = 0;
    }
    if (state->sasl_mechanism_list) {
	myfree(state->sasl_mechanism_list);	/* allocated in lmtp_helo */
	state->sasl_mechanism_list = 0;
    }
    if (state->sasl_conn) {
	if (msg_verbose)
	    msg_info("disposing SASL state information");
	sasl_dispose(&state->sasl_conn);
    }
    if (state->sasl_callbacks) {
	myfree((char *) state->sasl_callbacks);
	state->sasl_callbacks = 0;
    }
    if (state->sasl_encoded) {
	vstring_free(state->sasl_encoded);
	state->sasl_encoded = 0;
    }
    if (state->sasl_decoded) {
	vstring_free(state->sasl_decoded);
	state->sasl_decoded = 0;
    }
}

#endif
