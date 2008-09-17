/*++
/* NAME
/*	smtp_sasl_proto 3
/* SUMMARY
/*	Postfix SASL interface for SMTP client
/* SYNOPSIS
/*	#include smtp_sasl.h
/*
/*	void	smtp_sasl_helo_auth(state, words)
/*	SMTP_STATE *state;
/*	const char *words;
/*
/*	int	smtp_sasl_helo_login(state)
/*	SMTP_STATE *state;
/* DESCRIPTION
/*	This module contains random chunks of code that implement
/*	the SMTP protocol interface for SASL negotiation. The goal
/*	is to reduce clutter in the main SMTP client source code.
/*
/*	smtp_sasl_helo_auth() processes the AUTH option in the
/*	SMTP server's EHLO response.
/*
/*	smtp_sasl_helo_login() authenticates the SMTP client to the
/*	SMTP server, using the authentication mechanism information
/*	given by the server. The result is a Postfix delivery status
/*	code in case of trouble.
/*
/*	Arguments:
/* .IP state
/*	Session context.
/* .IP words
/*	List of SASL authentication mechanisms (separated by blanks)
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

/* System library. */

#include <sys_defs.h>
#include <string.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "smtp.h"
#include "smtp_sasl.h"

#ifdef USE_SASL_AUTH

/* smtp_sasl_compat_mechs - Trim server's mechanism list */

static const char *smtp_sasl_compat_mechs(const char *words)
{
    static VSTRING *buf;
    char   *mech_list;
    char   *save_mech;
    char   *mech;

    /*
     * Use server's mechanisms if no filter specified
     */
    if (smtp_sasl_mechs == 0 || *words == 0)
	return (words);

    if (buf == 0)
	buf = vstring_alloc(10);

    VSTRING_RESET(buf);
    VSTRING_TERMINATE(buf);

    save_mech = mech_list = mystrdup(words);

    while ((mech = mystrtok(&mech_list, " \t")) != 0) {
	if (string_list_match(smtp_sasl_mechs, mech)) {
	    if (VSTRING_LEN(buf) > 0)
		VSTRING_ADDCH(buf, ' ');
	    vstring_strcat(buf, mech);
	}
    }
    myfree(save_mech);

    return (vstring_str(buf));
}

/* smtp_sasl_helo_auth - handle AUTH option in EHLO reply */

void    smtp_sasl_helo_auth(SMTP_SESSION *session, const char *words)
{
    const char *mech_list = smtp_sasl_compat_mechs(words);
    char   *junk;

    /*
     * XXX If the server offers no compatible authentication mechanisms, then
     * pretend that the server doesn't support SASL authentication.
     * 
     * XXX If the server offers multiple different lists, concatenate them. Let
     * the SASL library worry about duplicates.
     */
    if (session->sasl_mechanism_list) {
	if (strcasecmp(session->sasl_mechanism_list, mech_list) != 0
	    && strlen(mech_list) > 0
	    && strlen(session->sasl_mechanism_list) < var_line_limit) {
	    junk = concatenate(session->sasl_mechanism_list, " ", mech_list,
			       (char *) 0);
	    myfree(session->sasl_mechanism_list);
	    session->sasl_mechanism_list = junk;
	}
	return;
    }
    if (strlen(mech_list) > 0) {
	session->sasl_mechanism_list = mystrdup(mech_list);
    } else {
	msg_warn(*words ? "%s offered no supported AUTH mechanisms: '%s'" :
		 "%s offered null AUTH mechanism list",
		 session->namaddrport, words);
    }
    session->features |= SMTP_FEATURE_AUTH;
}

/* smtp_sasl_helo_login - perform SASL login */

int     smtp_sasl_helo_login(SMTP_STATE *state)
{
    SMTP_SESSION *session = state->session;
    DSN_BUF *why = state->why;
    int     ret;

    /*
     * Skip authentication when no authentication info exists for this
     * server, so that we talk to each other like strangers.
     */
    if (smtp_sasl_passwd_lookup(session) == 0) {
	session->features &= ~SMTP_FEATURE_AUTH;
	return 0;
    }

    /*
     * Otherwise, if authentication information exists, assume that
     * authentication is required, and assume that an authentication error is
     * recoverable from the message delivery point of view. An authentication
     * error is unrecoverable from a session point of view - the session will
     * not be reused.
     */
    ret = 0;
    if (session->sasl_mechanism_list == 0) {
	dsb_simple(why, "4.7.0", "SASL authentication failed: "
		   "server %s offered no compatible authentication mechanisms for this type of connection security",
		   session->namaddr);
	ret = smtp_sess_fail(state);
	/* Session reuse is disabled. */
    } else {
#ifndef USE_TLS
	smtp_sasl_start(session, VAR_SMTP_SASL_OPTS,
			var_smtp_sasl_opts);
#else
	if (session->tls_context == 0)
	    smtp_sasl_start(session, VAR_SMTP_SASL_OPTS,
			    var_smtp_sasl_opts);
#ifdef SNAPSHOT					/* XXX: Not yet */
	else if (TLS_CERT_IS_MATCHED(session->tls_context))
	    smtp_sasl_start(session, VAR_SMTP_SASL_TLSV_OPTS,
			    var_smtp_sasl_tlsv_opts);
#endif
	else
	    smtp_sasl_start(session, VAR_SMTP_SASL_TLS_OPTS,
			    var_smtp_sasl_tls_opts);
#endif
	if (smtp_sasl_authenticate(session, why) <= 0) {
	    ret = smtp_sess_fail(state);
	    /* Session reuse is disabled. */
	}
    }
    return (ret);
}

#endif
