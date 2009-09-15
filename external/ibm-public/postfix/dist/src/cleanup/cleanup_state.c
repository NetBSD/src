/*	$NetBSD: cleanup_state.c,v 1.1.1.1.2.2 2009/09/15 06:02:34 snj Exp $	*/

/*++
/* NAME
/*	cleanup_state 3
/* SUMMARY
/*	per-message state variables
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	CLEANUP_STATE *cleanup_state_alloc(src)
/*	VSTREAM	*src;
/*
/*	void	cleanup_state_free(state)
/*	CLEANUP_STATE *state;
/* DESCRIPTION
/*	This module maintains about two dozen state variables
/*	that are used by many routines in the course of processing one
/*	message.
/*
/*	cleanup_state_alloc() initializes the per-message state variables.
/*
/*	cleanup_state_free() cleans up.
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

#include <mymalloc.h>
#include <vstring.h>
#include <htable.h>

/* Global library. */

#include <been_here.h>
#include <mail_params.h>
#include <mime_state.h>
#include <mail_proto.h>

/* Milter library. */

#include <milter.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_state_alloc - initialize global state */

CLEANUP_STATE *cleanup_state_alloc(VSTREAM *src)
{
    CLEANUP_STATE *state = (CLEANUP_STATE *) mymalloc(sizeof(*state));

    state->attr_buf = vstring_alloc(10);
    state->temp1 = vstring_alloc(10);
    state->temp2 = vstring_alloc(10);
    if (cleanup_strip_chars)
	state->stripped_buf = vstring_alloc(10);
    state->src = src;
    state->dst = 0;
    state->handle = 0;
    state->queue_name = 0;
    state->queue_id = 0;
    state->arrival_time.tv_sec = state->arrival_time.tv_usec = 0;
    state->fullname = 0;
    state->sender = 0;
    state->recip = 0;
    state->orig_rcpt = 0;
    state->return_receipt = 0;
    state->errors_to = 0;
    state->flags = 0;
    state->qmgr_opts = 0;
    state->errs = 0;
    state->err_mask = 0;
    state->headers_seen = 0;
    state->hop_count = 0;
    state->resent = "";
    state->dups = been_here_init(var_dup_filter_limit, BH_FLAG_FOLD);
    state->action = cleanup_envelope;
    state->data_offset = -1;
    state->body_offset = -1;
    state->xtra_offset = -1;
    state->cont_length = 0;
    state->sender_pt_offset = -1;
    state->sender_pt_target = -1;
    state->append_rcpt_pt_offset = -1;
    state->append_rcpt_pt_target = -1;
    state->append_hdr_pt_offset = -1;
    state->append_hdr_pt_target = -1;
    state->rcpt_count = 0;
    state->reason = 0;
    state->smtp_reply = 0;
    state->attr = nvtable_create(10);
    nvtable_update(state->attr, MAIL_ATTR_LOG_ORIGIN, MAIL_ATTR_ORG_LOCAL);
    state->mime_state = 0;
    state->mime_errs = 0;
    state->hdr_rewrite_context = MAIL_ATTR_RWR_LOCAL;
    state->filter = 0;
    state->redirect = 0;
    state->dsn_envid = 0;
    state->dsn_ret = 0;
    state->dsn_notify = 0;
    state->dsn_orcpt = 0;
    state->verp_delims = 0;
    state->milters = 0;
    state->client_name = 0;
    state->reverse_name = 0;
    state->client_addr = 0;
    state->client_af = 0;
    state->client_port = 0;
    state->milter_ext_from = 0;
    state->milter_ext_rcpt = 0;
    state->milter_err_text = 0;
    state->free_regions = state->body_regions = state->curr_body_region = 0;
    return (state);
}

/* cleanup_state_free - destroy global state */

void    cleanup_state_free(CLEANUP_STATE *state)
{
    vstring_free(state->attr_buf);
    vstring_free(state->temp1);
    vstring_free(state->temp2);
    if (cleanup_strip_chars)
	vstring_free(state->stripped_buf);
    if (state->fullname)
	myfree(state->fullname);
    if (state->sender)
	myfree(state->sender);
    if (state->recip)
	myfree(state->recip);
    if (state->orig_rcpt)
	myfree(state->orig_rcpt);
    if (state->return_receipt)
	myfree(state->return_receipt);
    if (state->errors_to)
	myfree(state->errors_to);
    if (state->queue_name)
	myfree(state->queue_name);
    if (state->queue_id)
	myfree(state->queue_id);
    been_here_free(state->dups);
    if (state->reason)
	myfree(state->reason);
    if (state->smtp_reply)
	myfree(state->smtp_reply);
    nvtable_free(state->attr);
    if (state->mime_state)
	mime_state_free(state->mime_state);
    if (state->filter)
	myfree(state->filter);
    if (state->redirect)
	myfree(state->redirect);
    if (state->dsn_envid)
	myfree(state->dsn_envid);
    if (state->dsn_orcpt)
	myfree(state->dsn_orcpt);
    if (state->verp_delims)
	myfree(state->verp_delims);
    if (state->milters)
	milter_free(state->milters);
    if (state->milter_ext_from)
	vstring_free(state->milter_ext_from);
    if (state->milter_ext_rcpt)
	vstring_free(state->milter_ext_rcpt);
    if (state->milter_err_text)
	vstring_free(state->milter_err_text);
    cleanup_region_done(state);
    myfree((char *) state);
}
