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
    state->recipient = 0;
    state->etrn_name = 0;
    state->protocol = "SMTP";
    state->where = SMTPD_AFTER_CONNECT;
    state->recursion = 0;
    state->msg_size = 0;
    state->junk_cmds = 0;
    state->warn_if_reject = 0;

#ifdef USE_SASL_AUTH
    if (SMTPD_STAND_ALONE(state))
	var_smtpd_sasl_enable = 0;
    if (var_smtpd_sasl_enable)
	smtpd_sasl_connect(state);
#endif

    /*
     * Initialize peer information.
     */
    smtpd_peer_init(state);

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
    smtpd_peer_reset(state);

#ifdef USE_SASL_AUTH
    if (var_smtpd_sasl_enable)
	smtpd_sasl_disconnect(state);
#endif
}
