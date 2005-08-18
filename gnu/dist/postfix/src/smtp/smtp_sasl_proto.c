/*	$NetBSD: smtp_sasl_proto.c,v 1.1.1.5 2005/08/18 21:08:57 rpaulo Exp $	*/

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
    int     ret;

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

    /*
     * XXX If the server offers no compatible authentication mechanisms,
     * then pretend that the server doesn't support SASL authentication.
     */
    if (session->sasl_mechanism_list) {
	if (strcasecmp(session->sasl_mechanism_list, mech_list) == 0)
	    return;
	myfree(session->sasl_mechanism_list);
	msg_warn("%s offered AUTH option multiple times", session->namaddr);
	session->sasl_mechanism_list = 0;
	session->features &= ~SMTP_FEATURE_AUTH;
    }
    if (strlen(mech_list) > 0) {
	session->sasl_mechanism_list = mystrdup(mech_list);
	session->features |= SMTP_FEATURE_AUTH;
    } else {
	msg_warn(*words ? "%s offered no supported AUTH mechanisms: '%s'" :
		    "%s offered null AUTH mechanism list",
		 session->namaddr, words);
    }
}

/* smtp_sasl_helo_login - perform SASL login */

int     smtp_sasl_helo_login(SMTP_STATE *state)
{
    SMTP_SESSION *session = state->session;
    VSTRING *why;
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
    why = vstring_alloc(10);
    ret = 0;
    smtp_sasl_start(session, VAR_SMTP_SASL_OPTS, var_smtp_sasl_opts);
    if (smtp_sasl_authenticate(session, why) <= 0) {
	ret = smtp_site_fail(state, 450, "Authentication failed: %s",
			     vstring_str(why));
	/* Session reuse is disabled. */
    }
    vstring_free(why);
    return (ret);
}

#endif
