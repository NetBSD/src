/*	$NetBSD: smtpd_sasl_glue.c,v 1.4.2.1 2023/12/25 12:43:35 martin Exp $	*/

/*++
/* NAME
/*	smtpd_sasl_glue 3
/* SUMMARY
/*	Postfix SMTP server, SASL support interface
/* SYNOPSIS
/*	#include "smtpd_sasl_glue.h"
/*
/*	void	smtpd_sasl_state_init(state)
/*	SMTPD_STATE *state;
/*
/*	void    smtpd_sasl_initialize()
/*
/*	void	smtpd_sasl_activate(state, sasl_opts_name, sasl_opts_val)
/*	SMTPD_STATE *state;
/*	const char *sasl_opts_name;
/*	const char *sasl_opts_val;
/*
/*	char	*smtpd_sasl_authenticate(state, sasl_method, init_response)
/*	SMTPD_STATE *state;
/*	const char *sasl_method;
/*	const char *init_response;
/*
/*	void	smtpd_sasl_logout(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_sasl_login(state, sasl_username, sasl_method)
/*	SMTPD_STATE *state;
/*	const char *sasl_username;
/*	const char *sasl_method;
/*
/*	void	smtpd_sasl_deactivate(state)
/*	SMTPD_STATE *state;
/*
/*	int	smtpd_sasl_is_active(state)
/*	SMTPD_STATE *state;
/*
/*	int	smtpd_sasl_set_inactive(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	This module encapsulates most of the detail specific to SASL
/*	authentication.
/*
/*	smtpd_sasl_state_init() performs minimal server state
/*	initialization to support external authentication (e.g.,
/*	XCLIENT) without having to enable SASL in main.cf. This
/*	should always be called at process startup.
/*
/*	smtpd_sasl_initialize() initializes the SASL library. This
/*	routine should be called once at process start-up. It may
/*	need access to the file system for run-time loading of
/*	plug-in modules. There is no corresponding cleanup routine.
/*
/*	smtpd_sasl_activate() performs per-connection initialization.
/*	This routine should be called once at the start of every
/*	connection. The sasl_opts_name and sasl_opts_val parameters
/*	are the postfix configuration parameters setting the security
/*	policy of the SASL authentication.
/*
/*	smtpd_sasl_authenticate() implements the authentication
/*	dialog.  The result is zero in case of success, -1 in case
/*	of failure. smtpd_sasl_authenticate() updates the following
/*	state structure members:
/* .IP sasl_method
/*	The authentication method that was successfully applied.
/*	This member is a null pointer in the absence of successful
/*	authentication.
/* .IP sasl_username
/*	The username that was successfully authenticated.
/*	This member is a null pointer in the absence of successful
/*	authentication.
/* .PP
/*	smtpd_sasl_login() records the result of successful external
/*	authentication, i.e. without invoking smtpd_sasl_authenticate(),
/*	but produces an otherwise equivalent result.
/*
/*	smtpd_sasl_logout() cleans up after smtpd_sasl_authenticate().
/*	This routine exists for the sake of symmetry.
/*
/*	smtpd_sasl_deactivate() performs per-connection cleanup.
/*	This routine should be called at the end of every connection.
/*
/*	smtpd_sasl_is_active() is a predicate that returns true
/*	if the SMTP server session state is between smtpd_sasl_activate()
/*	and smtpd_sasl_deactivate().
/*
/*	smtpd_sasl_set_inactive() initializes the SMTP session
/*	state before the first smtpd_sasl_activate() call.
/*
/*	Arguments:
/* .IP state
/*	SMTP session context.
/* .IP sasl_opts_name
/*	Security options parameter name.
/* .IP sasl_opts_val
/*	Security options parameter value.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <stdlib.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <sasl_mech_filter.h>
#include <string_list.h>

/* XSASL library. */

#include <xsasl.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_sasl_glue.h"
#include "smtpd_chat.h"

#ifdef USE_SASL_AUTH

 /*
  * SASL mechanism filter.
  */
static STRING_LIST *smtpd_sasl_mech_filter;

/*
 * Silly little macros.
 */
#define STR(s)	vstring_str(s)

 /*
  * SASL server implementation handle.
  */
static XSASL_SERVER_IMPL *smtpd_sasl_impl;

/* smtpd_sasl_initialize - per-process initialization */

void    smtpd_sasl_initialize(void)
{

    /*
     * Sanity check.
     */
    if (smtpd_sasl_impl)
	msg_panic("smtpd_sasl_initialize: repeated call");

    /*
     * Initialize the SASL library.
     */
    if ((smtpd_sasl_impl = xsasl_server_init(var_smtpd_sasl_type,
					     var_smtpd_sasl_path)) == 0)
	msg_fatal("SASL per-process initialization failed");

    /*
     * Initialize the SASL mechanism filter.
     */
    smtpd_sasl_mech_filter = string_list_init(VAR_SMTPD_SASL_MECH_FILTER,
					      MATCH_FLAG_NONE,
					      var_smtpd_sasl_mech_filter);
}

/* smtpd_sasl_activate - per-connection initialization */

void    smtpd_sasl_activate(SMTPD_STATE *state, const char *sasl_opts_name,
			            const char *sasl_opts_val)
{
    const char *mechanism_list;
    const char *filtered_mechanism_list;
    XSASL_SERVER_CREATE_ARGS create_args;
    int     tls_flag;

    /*
     * Sanity check.
     */
    if (smtpd_sasl_is_active(state))
	msg_panic("smtpd_sasl_activate: already active");

    /*
     * Initialize SASL-specific state variables. Use long-lived storage for
     * base 64 conversion results, rather than local variables, to avoid
     * memory leaks when a read or write routine returns abnormally after
     * timeout or I/O error.
     */
    state->sasl_reply = vstring_alloc(20);
    state->sasl_mechanism_list = 0;

    /*
     * Set up a new server context for this connection.
     */
#ifdef USE_TLS
    tls_flag = state->tls_context != 0;
#else
    tls_flag = 0;
#endif
#define ADDR_OR_EMPTY(addr, unknown) (strcmp(addr, unknown) ? addr : "")
#define REALM_OR_NULL(realm) (*(realm) ? (realm) : (char *) 0)

    if ((state->sasl_server =
	 XSASL_SERVER_CREATE(smtpd_sasl_impl, &create_args,
			     stream = state->client,
			     addr_family = state->addr_family,
			     server_addr = ADDR_OR_EMPTY(state->dest_addr,
						       SERVER_ADDR_UNKNOWN),
			     server_port = ADDR_OR_EMPTY(state->dest_port,
						       SERVER_PORT_UNKNOWN),
			     client_addr = ADDR_OR_EMPTY(state->addr,
						       CLIENT_ADDR_UNKNOWN),
			     client_port = ADDR_OR_EMPTY(state->port,
						       CLIENT_PORT_UNKNOWN),
			     service = var_smtpd_sasl_service,
			   user_realm = REALM_OR_NULL(var_smtpd_sasl_realm),
			     security_options = sasl_opts_val,
			     tls_flag = tls_flag)) == 0)
	msg_fatal("SASL per-connection initialization failed");

    /*
     * Get the list of authentication mechanisms.
     */
    if ((mechanism_list =
	 xsasl_server_get_mechanism_list(state->sasl_server)) == 0)
	msg_fatal("no SASL authentication mechanisms");
    filtered_mechanism_list =
	sasl_mech_filter(smtpd_sasl_mech_filter, mechanism_list);
    if (*filtered_mechanism_list == 0)
	msg_fatal("%s discards all mechanisms in '%s'",
		  VAR_SMTPD_SASL_MECH_FILTER, mechanism_list);
    state->sasl_mechanism_list = mystrdup(filtered_mechanism_list);
}

/* smtpd_sasl_state_init - initialize state to allow extern authentication. */

void    smtpd_sasl_state_init(SMTPD_STATE *state)
{
    /* Initialization to support external authentication (e.g., XCLIENT). */
    state->sasl_username = 0;
    state->sasl_method = 0;
    state->sasl_sender = 0;
}

/* smtpd_sasl_deactivate - per-connection cleanup */

void    smtpd_sasl_deactivate(SMTPD_STATE *state)
{
    if (state->sasl_reply) {
	vstring_free(state->sasl_reply);
	state->sasl_reply = 0;
    }
    if (state->sasl_mechanism_list) {
	myfree(state->sasl_mechanism_list);
	state->sasl_mechanism_list = 0;
    }
    if (state->sasl_username) {
	myfree(state->sasl_username);
	state->sasl_username = 0;
    }
    if (state->sasl_method) {
	myfree(state->sasl_method);
	state->sasl_method = 0;
    }
    if (state->sasl_sender) {
	myfree(state->sasl_sender);
	state->sasl_sender = 0;
    }
    if (state->sasl_server) {
	xsasl_server_free(state->sasl_server);
	state->sasl_server = 0;
    }
}

/* smtpd_sasl_authenticate - per-session authentication */

int     smtpd_sasl_authenticate(SMTPD_STATE *state,
				        const char *sasl_method,
				        const char *init_response)
{
    int     status;
    const char *sasl_username;

    /*
     * SASL authentication protocol start-up. Process any initial client
     * response that was sent along in the AUTH command.
     */
    for (status = xsasl_server_first(state->sasl_server, sasl_method,
				     init_response, state->sasl_reply);
	 status == XSASL_AUTH_MORE;
	 status = xsasl_server_next(state->sasl_server, STR(state->buffer),
				    state->sasl_reply)) {

	/*
	 * Send a server challenge.
	 */
	smtpd_chat_reply(state, "334 %s", STR(state->sasl_reply));

	/*
	 * Receive the client response. "*" means that the client gives up.
	 */
	if (!smtpd_chat_query_limit(state, var_smtpd_sasl_resp_limit)) {
	    smtpd_chat_reply(state, "500 5.5.6 SASL response limit exceeded");
	    return (-1);
	}
	if (strcmp(STR(state->buffer), "*") == 0) {
	    msg_warn("%s: SASL %s authentication aborted",
		     state->namaddr, sasl_method);
	    smtpd_chat_reply(state, "501 5.7.0 Authentication aborted");
	    return (-1);
	}
    }
    if (status != XSASL_AUTH_DONE) {
	sasl_username = xsasl_server_get_username(state->sasl_server);
	msg_warn("%s: SASL %.100s authentication failed: %s, sasl_username=%.100s",
		 state->namaddr, sasl_method, *STR(state->sasl_reply) ?
		 STR(state->sasl_reply) : "(reason unavailable)",
		 sasl_username ? sasl_username : "(unavailable)");
	/* RFC 4954 Section 6. */
	if (status == XSASL_AUTH_TEMP)
	    smtpd_chat_reply(state, "454 4.7.0 Temporary authentication failure: %s",
			     STR(state->sasl_reply));
	else
	    smtpd_chat_reply(state, "535 5.7.8 Error: authentication failed: %s",
			     STR(state->sasl_reply));
	return (-1);
    }
    /* RFC 4954 Section 6. */
    smtpd_chat_reply(state, "235 2.7.0 Authentication successful");
    if ((sasl_username = xsasl_server_get_username(state->sasl_server)) == 0)
	msg_panic("cannot look up the authenticated SASL username");
    state->sasl_username = mystrdup(sasl_username);
    printable(state->sasl_username, '?');
    state->sasl_method = mystrdup(sasl_method);
    printable(state->sasl_method, '?');

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

/* smtpd_sasl_login - set login information */

void    smtpd_sasl_login(SMTPD_STATE *state, const char *sasl_username,
			         const char *sasl_method)
{
    if (state->sasl_username)
	myfree(state->sasl_username);
    state->sasl_username = mystrdup(sasl_username);
    if (state->sasl_method)
	myfree(state->sasl_method);
    state->sasl_method = mystrdup(sasl_method);
}

#endif
