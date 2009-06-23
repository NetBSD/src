/*	$NetBSD: qmqpd_state.c,v 1.1.1.1 2009/06/23 10:08:53 tron Exp $	*/

/*++
/* NAME
/*	qmqpd_state 3
/* SUMMARY
/*	Postfix QMQP server
/* SYNOPSIS
/*	#include "qmqpd.h"
/*
/*	QMQPD_STATE *qmqpd_state_alloc(stream)
/*	VSTREAM *stream;
/*
/*	void	qmqpd_state_free(state)
/*	QMQPD_STATE *state;
/* DESCRIPTION
/*	qmqpd_state_alloc() creates and initializes session context.
/*
/*	qmqpd_state_free() destroys session context.
/*
/*	Arguments:
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
#include <time.h>

/* Utility library. */

#include <mymalloc.h>
#include <vstream.h>
#include <vstring.h>

/* Global library. */

#include <mail_stream.h>
#include <cleanup_user.h>
#include <mail_proto.h>

/* Application-specific. */

#include <qmqpd.h>

/* qmqpd_state_alloc - allocate and initialize session state */

QMQPD_STATE *qmqpd_state_alloc(VSTREAM *stream)
{
    QMQPD_STATE *state;

    state = (QMQPD_STATE *) mymalloc(sizeof(*state));
    state->err = CLEANUP_STAT_OK;
    state->client = stream;
    state->message = vstring_alloc(1000);
    state->buf = vstring_alloc(100);
    GETTIMEOFDAY(&state->arrival_time);
    qmqpd_peer_init(state);
    state->queue_id = 0;
    state->cleanup = 0;
    state->dest = 0;
    state->rcpt_count = 0;
    state->reason = 0;
    state->sender = 0;
    state->recipient = 0;
    state->protocol = MAIL_PROTO_QMQP;
    state->where = "initializing client connection";
    state->why_rejected = vstring_alloc(10);
    return (state);
}

/* qmqpd_state_free - destroy session state */

void qmqpd_state_free(QMQPD_STATE *state)
{
    vstring_free(state->message);
    vstring_free(state->buf);
    qmqpd_peer_reset(state);
    if (state->queue_id)
	myfree(state->queue_id);
    if (state->dest)
	mail_stream_cleanup(state->dest);
    if (state->sender)
	myfree(state->sender);
    if (state->recipient)
	myfree(state->recipient);
    vstring_free(state->why_rejected);
    myfree((char *) state);
}
