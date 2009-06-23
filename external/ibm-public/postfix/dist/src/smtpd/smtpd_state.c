/*	$NetBSD: smtpd_state.c,v 1.1.1.1 2009/06/23 10:08:56 tron Exp $	*/

/*++
/* NAME
/*	smtpd_state 3
/* SUMMARY
/*	Postfix SMTP server
/* SYNOPSIS
/*	#include "smtpd.h"
/*
/*	void	smtpd_state_init(state, stream, service)
/*	SMTPD_STATE *state;
/*	VSTREAM *stream;
/*	const char *service;
/*
/*	void	smtpd_state_reset(state)
/*	SMTPD_STATE *state;
/* DESCRIPTION
/*	smtpd_state_init() initializes session context.
/*
/*	smtpd_state_reset() cleans up session context.
/*
/*	Arguments:
/* .IP state
/*	Session context.
/* .IP stream
/*	Stream connected to peer. The stream is not copied.
/* DIAGNOSTICS
/*	All errors are fatal.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
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

/* Utility library. */

#include <events.h>
#include <mymalloc.h>
#include <vstream.h>
#include <name_mask.h>
#include <msg.h>

/* Global library. */

#include <cleanup_user.h>
#include <mail_params.h>
#include <mail_error.h>
#include <mail_proto.h>

/* Application-specific. */

#include "smtpd.h"
#include "smtpd_chat.h"
#include "smtpd_sasl_glue.h"

/* smtpd_state_init - initialize after connection establishment */

void    smtpd_state_init(SMTPD_STATE *state, VSTREAM *stream,
			         const char *service)
{

    /*
     * Initialize the state information for this connection, and fill in the
     * connection-specific fields.
     */
    state->flags = 0;
    state->err = CLEANUP_STAT_OK;
    state->client = stream;
    state->service = mystrdup(service);
    state->buffer = vstring_alloc(100);
    state->addr_buf = vstring_alloc(100);
    state->error_count = 0;
    state->error_mask = 0;
    state->notify_mask = name_mask(VAR_NOTIFY_CLASSES, mail_error_masks,
				   var_notify_classes);
    state->helo_name = 0;
    state->queue_id = 0;
    state->cleanup = 0;
    state->dest = 0;
    state->rcpt_count = 0;
    state->access_denied = 0;
    state->history = 0;
    state->reason = 0;
    state->sender = 0;
    state->verp_delims = 0;
    state->recipient = 0;
    state->etrn_name = 0;
    state->protocol = mystrdup(MAIL_PROTO_SMTP);
    state->where = SMTPD_AFTER_CONNECT;
    state->recursion = 0;
    state->msg_size = 0;
    state->act_size = 0;
    state->junk_cmds = 0;
    state->rcpt_overshoot = 0;
    state->defer_if_permit_client = 0;
    state->defer_if_permit_helo = 0;
    state->defer_if_permit_sender = 0;
    state->defer_if_reject.dsn = 0;
    state->defer_if_reject.reason = 0;
    state->defer_if_permit.dsn = 0;
    state->defer_if_permit.reason = 0;
    state->discard = 0;
    state->expand_buf = 0;
    state->prepend = 0;
    state->proxy = 0;
    state->proxy_buffer = 0;
    state->proxy_mail = 0;
    state->proxy_xforward_features = 0;
    state->saved_filter = 0;
    state->saved_redirect = 0;
    state->saved_bcc = 0;
    state->saved_flags = 0;
#ifdef DELAY_ACTION
    state->saved_delay = 0;
#endif
    state->instance = vstring_alloc(10);
    state->seqno = 0;
    state->rewrite_context = 0;
#if 0
    state->ehlo_discard_mask = ~0;
#else
    state->ehlo_discard_mask = 0;
#endif
    state->dsn_envid = 0;
    state->dsn_buf = vstring_alloc(100);
    state->dsn_orcpt_buf = vstring_alloc(100);
#ifdef USE_TLS
    state->tls_use_tls = 0;
    state->tls_enforce_tls = 0;
    state->tls_auth_only = 0;
    state->tls_context = 0;
#endif

#ifdef USE_SASL_AUTH
    if (SMTPD_STAND_ALONE(state))
	var_smtpd_sasl_enable = 0;
    smtpd_sasl_set_inactive(state);
#endif

    state->milter_argv = 0;
    state->milter_argc = 0;

    /*
     * Initialize peer information.
     */
    smtpd_peer_init(state);

    /*
     * Initialize xforward information.
     */
    smtpd_xforward_init(state);

    /*
     * Initialize the conversation history.
     */
    smtpd_chat_reset(state);
}

/* smtpd_state_reset - cleanup after disconnect */

void    smtpd_state_reset(SMTPD_STATE *state)
{

    /*
     * When cleaning up, touch only those fields that smtpd_state_init()
     * filled in. The other fields are taken care of by their own
     * "destructor" functions.
     */
    if (state->service)
	myfree(state->service);
    if (state->buffer)
	vstring_free(state->buffer);
    if (state->addr_buf)
	vstring_free(state->addr_buf);
    if (state->access_denied)
	myfree(state->access_denied);
    if (state->protocol)
	myfree(state->protocol);
    smtpd_peer_reset(state);

    /*
     * Buffers that are created on the fly and that may be shared among mail
     * deliveries within the same SMTP session.
     */
    if (state->defer_if_permit.dsn)
	vstring_free(state->defer_if_permit.dsn);
    if (state->defer_if_permit.reason)
	vstring_free(state->defer_if_permit.reason);
    if (state->defer_if_reject.dsn)
	vstring_free(state->defer_if_reject.dsn);
    if (state->defer_if_reject.reason)
	vstring_free(state->defer_if_reject.reason);
    if (state->expand_buf)
	vstring_free(state->expand_buf);
    if (state->proxy_buffer)
	vstring_free(state->proxy_buffer);
    if (state->instance)
	vstring_free(state->instance);
    if (state->dsn_buf)
	vstring_free(state->dsn_buf);
    if (state->dsn_orcpt_buf)
	vstring_free(state->dsn_orcpt_buf);
}
