/*	$NetBSD: cleanup_bounce.c,v 1.1.1.1.2.2 2009/09/15 06:02:30 snj Exp $	*/

/*++
/* NAME
/*	cleanup_bounce 3
/* SUMMARY
/*	bounce all recipients
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_bounce(state)
/*	CLEANUP_STATE *state;
/* DESCRIPTION
/*	cleanup_bounce() updates the bounce log on request by client
/*	programs that cannot handle such problems themselves.
/*
/*	Upon successful completion, all error flags are reset,
/*	and the message is scheduled for deletion.
/*	Otherwise, the CLEANUP_STAT_WRITE error flag is raised.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is
/*	updated as records are processed and as errors happen.
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

#include <msg.h>
#include <stringops.h>
#include <stdlib.h>

/* Global library. */

#include <cleanup_user.h>
#include <mail_params.h>
#include <mail_proto.h>
#include <bounce.h>
#include <dsn_util.h>
#include <record.h>
#include <rec_type.h>
#include <dsn_mask.h>
#include <mail_queue.h>
#include <rec_attr_map.h>

/* Application-specific. */

#include "cleanup.h"

#define STR(x) vstring_str(x)

/* cleanup_bounce_append - update bounce logfile */

static void cleanup_bounce_append(CLEANUP_STATE *state, RECIPIENT *rcpt,
				          DSN *dsn)
{
    MSG_STATS stats;

    /*
     * Don't log a spurious warning (for example, when soft_bounce is turned
     * on). bounce_append() already logs a record when the logfile can't be
     * updated. Set the write error flag, so that a maildrop queue file won't
     * be destroyed.
     */
    if (bounce_append(BOUNCE_FLAG_CLEAN, state->queue_id,
		      CLEANUP_MSG_STATS(&stats, state),
		      rcpt, "none", dsn) != 0) {
	state->errs |= CLEANUP_STAT_WRITE;
    }
}

/* cleanup_bounce - bounce all recipients */

int     cleanup_bounce(CLEANUP_STATE *state)
{
    const char *myname = "cleanup_bounce";
    VSTRING *buf = vstring_alloc(100);
    const CLEANUP_STAT_DETAIL *detail;
    DSN_SPLIT dp;
    const char *dsn_status;
    const char *dsn_text;
    char   *rcpt = 0;
    RECIPIENT recipient;
    DSN     dsn;
    char   *attr_name;
    char   *attr_value;
    char   *dsn_orcpt = 0;
    int     dsn_notify = 0;
    char   *orig_rcpt = 0;
    char   *start;
    int     rec_type;
    int     junk;
    long    curr_offset;
    const char *encoding;
    const char *dsn_envid;
    int     dsn_ret;
    int     bounce_err;

    /*
     * Parse the failure reason if one was given, otherwise use a generic
     * mapping from cleanup-internal error code to (DSN + text).
     */
    if (state->reason) {
	dsn_split(&dp, "5.0.0", state->reason);
	dsn_status = DSN_STATUS(dp.dsn);
	dsn_text = dp.text;
    } else {
	detail = cleanup_stat_detail(state->errs);
	dsn_status = detail->dsn;
	dsn_text = detail->text;
    }

    /*
     * Create a bounce logfile with one entry for each final recipient.
     * Degrade gracefully in case of no recipients or no queue file.
     * 
     * Victor Duchovni observes that the number of recipients in the queue file
     * can potentially be very large due to virtual alias expansion. This can
     * expand the recipient count by virtual_alias_expansion_limit (default:
     * 1000) times.
     * 
     * After a queue file write error, purge any unwritten data (so that
     * vstream_fseek() won't fail while trying to flush it) and reset the
     * stream error flags to avoid false alarms.
     */
    if (vstream_ferror(state->dst) || vstream_fflush(state->dst)) {
	(void) vstream_fpurge(state->dst, VSTREAM_PURGE_BOTH);
	vstream_clearerr(state->dst);
    }
    if (vstream_fseek(state->dst, 0L, SEEK_SET) < 0)
	msg_fatal("%s: seek %s: %m", myname, cleanup_path);

    while ((state->errs & CLEANUP_STAT_WRITE) == 0) {
	if ((curr_offset = vstream_ftell(state->dst)) < 0)
	    msg_fatal("%s: vstream_ftell %s: %m", myname, cleanup_path);
	if ((rec_type = rec_get(state->dst, buf, 0)) <= 0
	    || rec_type == REC_TYPE_END)
	    break;
	start = STR(buf);
	if (rec_type == REC_TYPE_ATTR) {
	    if (split_nameval(STR(buf), &attr_name, &attr_value) != 0
		|| *attr_value == 0)
		continue;
	    /* Map attribute names to pseudo record type. */
	    if ((junk = rec_attr_map(attr_name)) != 0) {
		start = attr_value;
		rec_type = junk;
	    }
	}
	switch (rec_type) {
	case REC_TYPE_DSN_ORCPT:		/* RCPT TO ORCPT parameter */
	    if (dsn_orcpt != 0)			/* can't happen */
		myfree(dsn_orcpt);
	    dsn_orcpt = mystrdup(start);
	    break;
	case REC_TYPE_DSN_NOTIFY:		/* RCPT TO NOTIFY parameter */
	    if (alldig(start) && (junk = atoi(start)) > 0
		&& DSN_NOTIFY_OK(junk))
		dsn_notify = junk;
	    else
		dsn_notify = 0;
	    break;
	case REC_TYPE_ORCP:			/* unmodified RCPT TO address */
	    if (orig_rcpt != 0)			/* can't happen */
		myfree(orig_rcpt);
	    orig_rcpt = mystrdup(start);
	    break;
	case REC_TYPE_RCPT:			/* rewritten RCPT TO address */
	    rcpt = start;
	    RECIPIENT_ASSIGN(&recipient, curr_offset,
			     dsn_orcpt ? dsn_orcpt : "", dsn_notify,
			     orig_rcpt ? orig_rcpt : rcpt, rcpt);
	    (void) DSN_SIMPLE(&dsn, dsn_status, dsn_text);
	    cleanup_bounce_append(state, &recipient, &dsn);
	    /* FALLTHROUGH */
	case REC_TYPE_DRCP:			/* canceled recipient */
	case REC_TYPE_DONE:			/* can't happen */
	    if (orig_rcpt != 0) {
		myfree(orig_rcpt);
		orig_rcpt = 0;
	    }
	    if (dsn_orcpt != 0) {
		myfree(dsn_orcpt);
		dsn_orcpt = 0;
	    }
	    dsn_notify = 0;
	    break;
	}
    }
    if (orig_rcpt != 0)				/* can't happen */
	myfree(orig_rcpt);
    if (dsn_orcpt != 0)				/* can't happen */
	myfree(dsn_orcpt);

    /*
     * No recipients. Yes, this can happen.
     */
    if ((state->errs & CLEANUP_STAT_WRITE) == 0 && rcpt == 0) {
	RECIPIENT_ASSIGN(&recipient, 0, "", 0, "", "unknown");
	(void) DSN_SIMPLE(&dsn, dsn_status, dsn_text);
	cleanup_bounce_append(state, &recipient, &dsn);
    }
    vstring_free(buf);

    /*
     * Flush the bounce logfile to the sender. See also qmgr_active.c.
     */
    if ((state->errs & CLEANUP_STAT_WRITE) == 0) {
	if ((encoding = nvtable_find(state->attr, MAIL_ATTR_ENCODING)) == 0)
	    encoding = MAIL_ATTR_ENC_NONE;
	dsn_envid = state->dsn_envid ?
	    state->dsn_envid : "";
	dsn_ret = (state->errs & (CLEANUP_STAT_CONT | CLEANUP_STAT_SIZE)) ?
	    DSN_RET_HDRS : state->dsn_ret;

	if (state->verp_delims == 0 || var_verp_bounce_off) {
	    bounce_err =
		bounce_flush(BOUNCE_FLAG_CLEAN,
			     state->queue_name, state->queue_id,
			     encoding, state->sender, dsn_envid,
			     dsn_ret);
	} else {
	    bounce_err =
		bounce_flush_verp(BOUNCE_FLAG_CLEAN,
				  state->queue_name, state->queue_id,
				  encoding, state->sender, dsn_envid,
				  dsn_ret, state->verp_delims);
	}
	if (bounce_err != 0) {
	    msg_warn("%s: bounce message failure", state->queue_id);
	    state->errs |= CLEANUP_STAT_WRITE;
	}
    }

    /*
     * Schedule this message (and trace logfile) for deletion when all is
     * well. When all is not well these files would be deleted too, but the
     * client would get a different completion status so we have to carefully
     * maintain the bits anyway.
     */
    if ((state->errs &= CLEANUP_STAT_WRITE) == 0)
	state->flags |= CLEANUP_FLAG_DISCARD;

    return (state->errs);
}
