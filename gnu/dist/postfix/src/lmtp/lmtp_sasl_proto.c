/*++
/* NAME
/*	lmtp_sasl_proto 3
/* SUMMARY
/*	Postfix SASL interface for LMTP client
/* SYNOPSIS
/*	#include lmtp_sasl.h
/*
/*	void	lmtp_sasl_helo_auth(state, words)
/*	LMTP_STATE *state;
/*	const char *words;
/*
/*	int	lmtp_sasl_helo_login(state)
/*	LMTP_STATE *state;
/* DESCRIPTION
/*	This module contains random chunks of code that implement
/*	the LMTP protocol interface for SASL negotiation. The goal
/*	is to reduce clutter in the main LMTP client source code.
/*
/*	lmtp_sasl_helo_auth() processes the AUTH option in the
/*	LMTP server's LHLO response.
/*
/*	lmtp_sasl_helo_login() authenticates the LMTP client to the
/*	LMTP server, using the authentication mechanism information
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

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include "lmtp.h"
#include "lmtp_sasl.h"

#ifdef USE_SASL_AUTH

/* lmtp_sasl_helo_auth - handle AUTH option in EHLO reply */

void    lmtp_sasl_helo_auth(LMTP_STATE *state, const char *words)
{

    /*
     * XXX If the server offers a null list of authentication mechanisms,
     * then pretend that the server doesn't support SASL authentication.
     */
    if (state->sasl_mechanism_list) {
	if (strcasecmp(state->sasl_mechanism_list, words) == 0)
	    return;
	myfree(state->sasl_mechanism_list);
	msg_warn("%s offered AUTH option multiple times",
		 state->session->namaddr);
	state->sasl_mechanism_list = 0;
	state->features &= ~LMTP_FEATURE_AUTH;
    }
    if (strlen(words) > 0) {
	state->sasl_mechanism_list = mystrdup(words);
	state->features |= LMTP_FEATURE_AUTH;
    } else {
	msg_warn("%s offered null AUTH mechanism list",
		 state->session->namaddr);
    }
}

/* lmtp_sasl_helo_login - perform SASL login */

int     lmtp_sasl_helo_login(LMTP_STATE *state)
{
    VSTRING *why = vstring_alloc(10);
    int     ret = 0;

    /*
     * Skip authentication when no authentication info exists for this
     * server, so that we talk to each other like strangers. Otherwise, if
     * authentication information exists, assume that authentication is
     * required, and assume that an authentication error is recoverable.
     */
    if (lmtp_sasl_passwd_lookup(state) != 0) {
	lmtp_sasl_start(state);
	if (lmtp_sasl_authenticate(state, why) <= 0)
	    ret = lmtp_site_fail(state, 450, "Authentication failed: %s",
				 vstring_str(why));
    }
    vstring_free(why);
    return (ret);
}

#endif
