/*	$NetBSD: smtpd_sasl_proto.c,v 1.1.1.3.4.2 2013/01/23 00:05:13 yamt Exp $	*/

/*++
/* NAME
/*	smtpd_sasl_proto 3
/* SUMMARY
/*	Postfix SMTP protocol support for SASL authentication
/* SYNOPSIS
/*	#include "smtpd.h"
/*	#include "smtpd_sasl_proto.h"
/*
/*	void	smtpd_sasl_auth_cmd(state, argc, argv)
/*	SMTPD_STATE *state;
/*	int	argc;
/*	SMTPD_TOKEN *argv;
/*
/*	void	smtpd_sasl_auth_extern(state, username, method)
/*	SMTPD_STATE *state;
/*	const char *username;
/*	const char *method;
/*
/*	void	smtpd_sasl_auth_reset(state)
/*	SMTPD_STATE *state;
/*
/*	char	*smtpd_sasl_mail_opt(state, sender)
/*	SMTPD_STATE *state;
/*	const char *sender;
/*
/*	void	smtpd_sasl_mail_log(state)
/*	SMTPD_STATE *state;
/*
/*	void	smtpd_sasl_mail_reset(state)
/*	SMTPD_STATE *state;
/*
/*	static int permit_sasl_auth(state, authenticated, unauthenticated)
/*	SMTPD_STATE *state;
/*	int	authenticated;
/*	int	unauthenticated;
/* DESCRIPTION
/*	This module contains random chunks of code that implement
/*	the SMTP protocol interface for SASL negotiation. The goal
/*	is to reduce clutter of the main SMTP server source code.
/*
/*	smtpd_sasl_auth_cmd() implements the AUTH command and updates
/*	the following state structure members:
/* .IP sasl_method
/*	The authentication method that was successfully applied.
/*	This member is a null pointer in the absence of successful
/*	authentication.
/* .IP sasl_username
/*	The username that was successfully authenticated.
/*	This member is a null pointer in the absence of successful
/*	authentication.
/* .PP
/*	smtpd_sasl_auth_reset() cleans up after the AUTH command.
/*	This is required before smtpd_sasl_auth_cmd() can be used again.
/*	This may be called even if SASL authentication is turned off
/*	in main.cf.
/*
/*	smtpd_sasl_auth_extern() records authentication information
/*	that is received from an external source.
/*	This may be called even if SASL authentication is turned off
/*	in main.cf.
/*
/*	smtpd_sasl_mail_opt() implements the SASL-specific AUTH=sender
/*	option to the MAIL FROM command. The result is an error response
/*	in case of problems.
/*
/*	smtpd_sasl_mail_log() logs SASL-specific information after
/*	processing the MAIL FROM command.
/*
/*	smtpd_sasl_mail_reset() performs cleanup for the SASL-specific
/*	AUTH=sender option to the MAIL FROM command.
/*
/*	permit_sasl_auth() permits access from an authenticated client.
/*	This test fails for clients that use anonymous authentication.
/*
/*	Arguments:
/* .IP state
/*	SMTP session context.
/* .IP argc
/*	Number of command line tokens.
/* .IP argv
/*	The command line parsed into tokens.
/* .IP sender
/*	Sender address from the AUTH=sender option in the MAIL FROM
/*	command.
/* .IP authenticated
/*	Result for authenticated client.
/* .IP unauthenticated
/*	Result for unauthenticated client.
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
/*	TLS support originally by:
/*	Lutz Jaenicke
/*	BTU Cottbus
/*	Allgemeine Elektrotechnik
/*	Universitaetsplatz 3-4
/*	D-03044 Cottbus, Germany
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
#include <mail_proto.h>
#include <mail_error.h>
#include <ehlo_mask.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_token.h"
#include "smtpd_chat.h"
#include "smtpd_sasl_proto.h"
#include "smtpd_sasl_glue.h"

#ifdef USE_SASL_AUTH

/* smtpd_sasl_auth_cmd - process AUTH command */

int     smtpd_sasl_auth_cmd(SMTPD_STATE *state, int argc, SMTPD_TOKEN *argv)
{
    char   *auth_mechanism;
    char   *initial_response;
    const char *err;

    if (var_helo_required && state->helo_name == 0) {
	state->error_mask |= MAIL_ERROR_POLICY;
	smtpd_chat_reply(state, "503 5.5.1 Error: send HELO/EHLO first");
	return (-1);
    }
    if (SMTPD_STAND_ALONE(state) || !smtpd_sasl_is_active(state)
	|| (state->ehlo_discard_mask & EHLO_MASK_AUTH)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: authentication not enabled");
	return (-1);
    }
#define IN_MAIL_TRANSACTION(state) ((state)->sender != 0)
    if (IN_MAIL_TRANSACTION(state)) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: MAIL transaction in progress");
	return (-1);
    }
    if (smtpd_milters != 0 && (err = milter_other_event(smtpd_milters)) != 0) {
	if (err[0] == '5') {
	    state->error_mask |= MAIL_ERROR_POLICY;
	    smtpd_chat_reply(state, "%s", err);
	    return (-1);
	}
	/* Sendmail compatibility: map 4xx into 454. */
	else if (err[0] == '4') {
	    state->error_mask |= MAIL_ERROR_POLICY;
	    smtpd_chat_reply(state, "454 4.3.0 Try again later");
	    return (-1);
	}
    }
#ifdef USE_TLS
    if (var_smtpd_tls_auth_only && !state->tls_context) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	/* RFC 4954, Section 4. */
	smtpd_chat_reply(state, "504 5.5.4 Encryption required for requested authentication mechanism");
	return (-1);
    }
#endif
    if (state->sasl_username) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "503 5.5.1 Error: already authenticated");
	return (-1);
    }
    if (argc < 2 || argc > 3) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	smtpd_chat_reply(state, "501 5.5.4 Syntax: AUTH mechanism");
	return (-1);
    }
    /* Don't reuse the SASL handle after authentication failure. */
#ifndef XSASL_TYPE_CYRUS
#define XSASL_TYPE_CYRUS	"cyrus"
#endif
    if (state->flags & SMTPD_FLAG_AUTH_USED) {
	smtpd_sasl_deactivate(state);
#ifdef USE_TLS
	if (state->tls_context != 0)
	    smtpd_sasl_activate(state, VAR_SMTPD_SASL_TLS_OPTS,
				var_smtpd_sasl_tls_opts);
	else
#endif
	    smtpd_sasl_activate(state, VAR_SMTPD_SASL_OPTS,
				var_smtpd_sasl_opts);
    } else if (strcmp(var_smtpd_sasl_type, XSASL_TYPE_CYRUS) == 0) {
	state->flags |= SMTPD_FLAG_AUTH_USED;
    }

    /*
     * All authentication failures shall be logged. The 5xx reply code from
     * the SASL authentication routine triggers tar-pit delays, which help to
     * slow down password guessing attacks.
     */
    auth_mechanism = argv[1].strval;
    initial_response = (argc == 3 ? argv[2].strval : 0);
    return (smtpd_sasl_authenticate(state, auth_mechanism, initial_response));
}

/* smtpd_sasl_mail_opt - SASL-specific MAIL FROM option */

char   *smtpd_sasl_mail_opt(SMTPD_STATE *state, const char *addr)
{

    /*
     * Do not store raw RFC2554 protocol data.
     */
#if 0
    if (state->sasl_username == 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	return ("503 5.5.4 Error: send AUTH command first");
    }
#endif
    if (state->sasl_sender != 0) {
	state->error_mask |= MAIL_ERROR_PROTOCOL;
	return ("503 5.5.4 Error: multiple AUTH= options");
    }
    if (strcmp(addr, "<>") != 0) {
	state->sasl_sender = mystrdup(addr);
	printable(state->sasl_sender, '?');
    }
    return (0);
}

/* smtpd_sasl_mail_log - SASL-specific MAIL FROM logging */

void    smtpd_sasl_mail_log(SMTPD_STATE *state)
{

    /*
     * See also: smtpd.c, for a shorter client= logfile record.
     */
#define PRINT_OR_NULL(cond, str) \
	    ((cond) ? (str) : "")
#define PRINT2_OR_NULL(cond, name, value) \
	    PRINT_OR_NULL((cond), (name)), PRINT_OR_NULL((cond), (value))

    msg_info("%s: client=%s%s%s%s%s%s%s%s%s%s%s",
	     (state->queue_id ? state->queue_id : "NOQUEUE"),
	     state->namaddr,
	     PRINT2_OR_NULL(state->sasl_method,
			    ", sasl_method=", state->sasl_method),
	     PRINT2_OR_NULL(state->sasl_username,
			    ", sasl_username=", state->sasl_username),
	     PRINT2_OR_NULL(state->sasl_sender,
			    ", sasl_sender=", state->sasl_sender),
	     PRINT2_OR_NULL(HAVE_FORWARDED_IDENT(state),
			    ", orig_queue_id=", FORWARD_IDENT(state)),
	     PRINT2_OR_NULL(HAVE_FORWARDED_CLIENT_ATTR(state),
			    ", orig_client=", FORWARD_NAMADDR(state)));
}

/* smtpd_sasl_mail_reset - SASL-specific MAIL FROM cleanup */

void    smtpd_sasl_mail_reset(SMTPD_STATE *state)
{
    if (state->sasl_sender) {
	myfree(state->sasl_sender);
	state->sasl_sender = 0;
    }
}

/* permit_sasl_auth - OK for authenticated connection */

int     permit_sasl_auth(SMTPD_STATE *state, int ifyes, int ifnot)
{
    if (state->sasl_method && strcasecmp(state->sasl_method, "anonymous"))
	return (ifyes);
    return (ifnot);
}

#endif
