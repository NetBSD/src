/*++
/* NAME
/*	smtpd_state 3
/* SUMMARY
/*	Postfix SMTP server
/* SYNOPSIS
/*	#include "smtpd.h"
/*
/*	void	smtpd_state_init(state, stream)
/*	SMTPD_STATE *state;
/*	VSTREAM *stream;
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

void    smtpd_state_init(SMTPD_STATE *state, VSTREAM *stream)
{

    /*
     * Initialize the state information for this connection, and fill in the
     * connection-specific fields.
     */
    state->err = CLEANUP_STAT_OK;
    state->client = stream;
    state->buffer = vstring_alloc(100);
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
    state->junk_cmds = 0;
    state->rcpt_overshoot = 0;
    state->defer_if_permit_client = 0;
    state->defer_if_permit_helo = 0;
    state->defer_if_permit_sender = 0;
    state->defer_if_reject.reason = 0;
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
    state->saved_flags = 0;
    state->instance = vstring_alloc(10);
    state->seqno = 0;

#ifdef USE_SASL_AUTH
    if (SMTPD_STAND_ALONE(state))
	var_smtpd_sasl_enable = 0;
    if (var_smtpd_sasl_enable)
	smtpd_sasl_connect(state, VAR_SMTPD_SASL_OPTS, var_smtpd_sasl_opts);
#endif

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
    if (state->buffer)
	vstring_free(state->buffer);
    if (state->protocol)
	myfree(state->protocol);
    smtpd_peer_reset(state);

    /*
     * Buffers that are created on the fly and that may be shared among mail
     * deliveries within the same SMTP session.
     */
    if (state->defer_if_permit.reason)
	vstring_free(state->defer_if_permit.reason);
    if (state->defer_if_reject.reason)
	vstring_free(state->defer_if_reject.reason);
    if (state->expand_buf)
	vstring_free(state->expand_buf);
    if (state->proxy_buffer)
	vstring_free(state->proxy_buffer);
    if (state->instance)
	vstring_free(state->instance);

#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable)
	smtpd_sasl_disconnect(state);
#endif
}
