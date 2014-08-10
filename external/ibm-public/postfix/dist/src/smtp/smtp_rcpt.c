/*	$NetBSD: smtp_rcpt.c,v 1.1.1.1.26.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	smtp_rcpt 3
/* SUMMARY
/*	application-specific recipient list operations
/* SYNOPSIS
/*	#include <smtp.h>
/*
/*	SMTP_RCPT_INIT(state)
/*	SMTP_STATE *state;
/*
/*	SMTP_RCPT_DROP(state, rcpt)
/*	SMTP_STATE *state;
/*	RECIPIENT *rcpt;
/*
/*	SMTP_RCPT_KEEP(state, rcpt)
/*	SMTP_STATE *state;
/*	RECIPIENT *rcpt;
/*
/*	SMTP_RCPT_ISMARKED(rcpt)
/*	RECIPIENT *rcpt;
/*
/*	void	smtp_rcpt_cleanup(SMTP_STATE *state)
/*	SMTP_STATE *state;
/*
/*	int	SMTP_RCPT_LEFT(state)
/*	SMTP_STATE *state;
/*
/*	void	smtp_rcpt_done(state, resp, rcpt)
/*	SMTP_STATE *state;
/*	SMTP_RESP *resp;
/*	RECIPIENT *rcpt;
/* DESCRIPTION
/*	This module implements application-specific mark and sweep
/*	operations on recipient lists. Operation is as follows:
/* .IP \(bu
/*	In the course of a delivery attempt each recipient is
/*	marked either as DROP (remove from recipient list) or KEEP
/*	(deliver to alternate mail server).
/* .IP \(bu
/*	After a delivery attempt any recipients marked DROP are deleted
/*	from the request, and the left-over recipients are unmarked.
/* .PP
/*	The mark/sweep algorithm is implemented in a redundant manner,
/*	and ensures that all recipients are explicitly accounted for.
/*
/*	Operations with upper case names are implemented by macros
/*	whose arguments may be evaluated more than once.
/*
/*	SMTP_RCPT_INIT() initializes application-specific recipient
/*	information and must be called before the first delivery attempt.
/*
/*	SMTP_RCPT_DROP() marks the specified recipient as DROP (remove
/*	from recipient list). It is an error to mark an already marked
/*	recipient.
/*
/*	SMTP_RCPT_KEEP() marks the specified recipient as KEEP (deliver
/*	to alternate mail server). It is an error to mark an already
/*	marked recipient.
/*
/*	SMTP_RCPT_ISMARKED() returns non-zero when the specified
/*	recipient is marked.
/*
/*	SMTP_RCPT_LEFT() returns the number of left_over recipients
/*	(the total number of marked and non-marked recipients).
/*
/*	smtp_rcpt_cleanup() cleans up the in-memory recipient list.
/*	It removes the recipients marked DROP from the left-over
/*	recipients, unmarks the left-over recipients, and enforces
/*	the requirement that all recipients are marked upon entry.
/*
/*	smtp_rcpt_done() logs that a recipient is completed and upon
/*	success it marks the recipient as done in the queue file.
/*	Finally, it marks the in-memory recipient as DROP.
/*
/*	Note: smtp_rcpt_done() may change the order of the recipient
/*	list.
/* DIAGNOSTICS
/*	Panic: interface violation.
/*
/*	When a recipient can't be logged as completed, the recipient is
/*	logged as deferred instead.
/* BUGS
/*	The single recipient list abstraction dates from the time
/*	that the SMTP client would give up after one SMTP session,
/*	so that each recipient was either bounced, delivered or
/*	deferred. Implicitly, all recipients were marked as DROP.
/*
/*	This abstraction is less convenient when an SMTP client
/*	must be able to deliver left-over recipients to a backup
/*	host. It might be more natural to have an input list with
/*      recipients to deliver, and an output list with left-over
/*      recipients.
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

/* System  library. */

#include <sys_defs.h>
#include <stdlib.h>			/* smtp_rcpt_cleanup  */
#include <string.h>

/* Utility  library. */

#include <msg.h>
#include <stringops.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <deliver_request.h>		/* smtp_rcpt_done */
#include <deliver_completed.h>		/* smtp_rcpt_done */
#include <sent.h>			/* smtp_rcpt_done */
#include <dsn_mask.h>			/* smtp_rcpt_done */

/* Application-specific. */

#include <smtp.h>

/* smtp_rcpt_done - mark recipient as done or else */

void    smtp_rcpt_done(SMTP_STATE *state, SMTP_RESP *resp, RECIPIENT *rcpt)
{
    DELIVER_REQUEST *request = state->request;
    SMTP_SESSION *session = state->session;
    SMTP_ITERATOR *iter = state->iterator;
    DSN_BUF *why = state->why;
    const char *dsn_action = "relayed";
    int     status;

    /*
     * Assume this was intermediate delivery when the server announced DSN
     * support, and don't send a DSN "SUCCESS" notification.
     */
    if (session->features & SMTP_FEATURE_DSN)
	rcpt->dsn_notify &= ~DSN_NOTIFY_SUCCESS;

    /*
     * Assume this was final delivery when the LMTP server announced no DSN
     * support. In backwards compatibility mode, send a "relayed" instead of
     * a "delivered" DSN "SUCCESS" notification. Do not attempt to "simplify"
     * the expression. The redundancy is for clarity. It is trivially
     * eliminated by the compiler. There is no need to sacrifice clarity for
     * the sake of "performance".
     */
    if ((session->features & SMTP_FEATURE_DSN) == 0
	&& !smtp_mode
	&& var_lmtp_assume_final != 0)
	dsn_action = "delivered";

    /*
     * Report success and delete the recipient from the delivery request.
     * Defer if the success can't be reported.
     * 
     * Note: the DSN action is ignored in case of address probes.
     */
    dsb_update(why, resp->dsn, dsn_action, DSB_MTYPE_DNS, STR(iter->host),
	       DSB_DTYPE_SMTP, resp->str, "%s", resp->str);

    status = sent(DEL_REQ_TRACE_FLAGS(request->flags),
		  request->queue_id, &request->msg_stats, rcpt,
		  session->namaddrport, DSN_FROM_DSN_BUF(why));
    if (status == 0)
	if (request->flags & DEL_REQ_FLAG_SUCCESS)
	    deliver_completed(state->src, rcpt->offset);
    SMTP_RCPT_DROP(state, rcpt);
    state->status |= status;
}

/* smtp_rcpt_cleanup_callback - qsort callback */

static int smtp_rcpt_cleanup_callback(const void *a, const void *b)
{
    return (((RECIPIENT *) a)->u.status - ((RECIPIENT *) b)->u.status);
}

/* smtp_rcpt_cleanup - purge completed recipients from request */

void    smtp_rcpt_cleanup(SMTP_STATE *state)
{
    RECIPIENT_LIST *rcpt_list = &state->request->rcpt_list;
    RECIPIENT *rcpt;

    /*
     * Sanity checks.
     */
    if (state->rcpt_drop + state->rcpt_keep != state->rcpt_left)
	msg_panic("smtp_rcpt_cleanup: recipient count mismatch: %d+%d!=%d",
		  state->rcpt_drop, state->rcpt_keep, state->rcpt_left);

    /*
     * Recipients marked KEEP sort before recipients marked DROP. Skip the
     * sorting in the common case that all recipients are marked the same.
     */
    if (state->rcpt_drop > 0 && state->rcpt_keep > 0)
	qsort((void *) rcpt_list->info, state->rcpt_left,
	      sizeof(rcpt_list->info[0]), smtp_rcpt_cleanup_callback);

    /*
     * Truncate the recipient list and unmark the left-over recipients.
     */
    state->rcpt_left = state->rcpt_keep;
    for (rcpt = rcpt_list->info; rcpt < rcpt_list->info + state->rcpt_left; rcpt++)
	rcpt->u.status = 0;
    state->rcpt_drop = state->rcpt_keep = 0;
}
