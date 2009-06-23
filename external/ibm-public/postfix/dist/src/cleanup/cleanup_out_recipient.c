/*	$NetBSD: cleanup_out_recipient.c,v 1.1.1.1 2009/06/23 10:08:43 tron Exp $	*/

/*++
/* NAME
/*	cleanup_out_recipient 3
/* SUMMARY
/*	envelope recipient output filter
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_out_recipient(state, dsn_orig_recipient,
/*					dsn_notify, orig_recipient,
/*					recipient)
/*	CLEANUP_STATE *state;
/*	const char *dsn_orig_recipient;
/*	const char *dsn_notify;
/*	const char *orig_recipient;
/*	const char *recipient;
/* DESCRIPTION
/*	This module implements an envelope recipient output filter.
/*
/*	cleanup_out_recipient() performs virtual table expansion
/*	and recipient duplicate filtering, and appends the
/*	resulting recipients to the output stream. It also
/*	generates DSN SUCCESS notifications.
/*
/*	Arguments:
/* .IP state
/*	Cleanup server state.
/* .IP dsn_orig_recipient
/*	DSN original recipient information.
/* .IP dsn_notify
/*	DSN notify flags.
/* .IP orig_recipient
/*	Envelope recipient as received by Postfix.
/* .IP recipient
/*	Envelope recipient as rewritten by Postfix.
/* CONFIGURATION
/* .ad
/* .fi
/* .IP enable_original_recipient
/*	Enable orig_recipient support.
/* .IP local_duplicate_filter_limit
/*	Upper bound to the size of the recipient duplicate filter.
/*	Zero means no limit; this may cause the mail system to
/*	become stuck.
/* .IP virtual_alias_maps
/*	list of virtual address lookup tables.
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
#include <string.h>

/* Utility library. */

#include <argv.h>
#include <msg.h>

/* Global library. */

#include <been_here.h>
#include <mail_params.h>
#include <rec_type.h>
#include <ext_prop.h>
#include <cleanup_user.h>
#include <dsn_mask.h>
#include <recipient_list.h>
#include <dsn.h>
#include <trace.h>
#include <mail_queue.h>			/* cleanup_trace_path */
#include <mail_proto.h>
#include <msg_stats.h>

/* Application-specific. */

#include "cleanup.h"

/* cleanup_trace_append - update trace logfile */

static void cleanup_trace_append(CLEANUP_STATE *state, RECIPIENT *rcpt,
				         DSN *dsn)
{
    MSG_STATS stats;

    if (cleanup_trace_path == 0) {
	cleanup_trace_path = vstring_alloc(10);
	mail_queue_path(cleanup_trace_path, MAIL_QUEUE_TRACE,
			state->queue_id);
    }
    if (trace_append(BOUNCE_FLAG_CLEAN, state->queue_id,
		     CLEANUP_MSG_STATS(&stats, state),
		     rcpt, "none", dsn) != 0) {
	msg_warn("%s: trace logfile update error", state->queue_id);
	state->errs |= CLEANUP_STAT_WRITE;
    }
}

/* cleanup_out_recipient - envelope recipient output filter */

void    cleanup_out_recipient(CLEANUP_STATE *state,
			              const char *dsn_orcpt,
			              int dsn_notify,
			              const char *orcpt,
			              const char *recip)
{
    ARGV   *argv;
    char  **cpp;

    /*
     * XXX Not elegant, but eliminates complexity in the record reading loop.
     */
    if (!var_enable_orcpt)
	orcpt = "";
    if (dsn_orcpt == 0)
	dsn_orcpt = "";

    /*
     * Distinguish between different original recipient addresses that map
     * onto the same mailbox. The recipient will use our original recipient
     * message header to figure things out.
     * 
     * Postfix 2.2 compatibility: when ignoring differences in Postfix original
     * recipient information, also ignore differences in DSN attributes. We
     * do, however, keep the DSN attributes of the recipient that survives
     * duplicate elimination.
     */
#define STREQ(x, y) (strcmp((x), (y)) == 0)

    if ((state->flags & CLEANUP_FLAG_MAP_OK) == 0
	|| cleanup_virt_alias_maps == 0) {
	if ((var_enable_orcpt ?
	     been_here(state->dups, "%s\n%d\n%s\n%s",
		       dsn_orcpt, dsn_notify, orcpt, recip) :
	     been_here_fixed(state->dups, recip)) == 0) {
	    if (dsn_notify)
		cleanup_out_format(state, REC_TYPE_ATTR, "%s=%d",
				   MAIL_ATTR_DSN_NOTIFY, dsn_notify);
	    if (*dsn_orcpt)
		cleanup_out_format(state, REC_TYPE_ATTR, "%s=%s",
				   MAIL_ATTR_DSN_ORCPT, dsn_orcpt);
	    cleanup_out_string(state, REC_TYPE_ORCP, orcpt);
	    cleanup_out_string(state, REC_TYPE_RCPT, recip);
	    state->rcpt_count++;
	}
    }

    /*
     * XXX DSN. RFC 3461 gives us three options for multi-recipient aliases
     * (we're treating single recipient aliases as a special case of
     * multi-recipient aliases, one argument being that it is none of the
     * sender's business).
     * 
     * (a) Don't propagate ENVID, NOTIFY, RET, or ORCPT. If NOTIFY specified
     * SUCCESS, send a "relayed" DSN.
     * 
     * (b) Propagate ENVID, (NOTIFY minus SUCCESS), RET, and ORCPT. If NOTIFY
     * specified SUCCESS, send an "expanded" DSN.
     * 
     * (c) Propagate ENVID, NOTIFY, RET, and ORCPT to one recipient only. Send
     * no DSN.
     * 
     * In all three cases we are modifying at least one NOTIFY value. Either we
     * have to record explicit dsn_notify records, or we must not allow the
     * use of a per-message non-default NOTIFY value that applies to all
     * recipient records.
     * 
     * Alternatives (a) and (c) require that we store explicit per-recipient RET
     * and ENVID records, at least for the recipients that are excluded from
     * RET and ENVID propagation. This means storing explicit ENVID records
     * to indicate that the information does not exist. All this makes
     * alternative (b) more and more attractive. It is no surprise that we
     * use (b) here and in the local delivery agent.
     * 
     * In order to generate a SUCCESS notification from the cleanup server we
     * have to write the trace logfile record now. We're NOT going to flush
     * the trace file from the cleanup server; if we need to write bounce
     * logfile records, and the bounce service fails, we must be able to
     * cancel the entire cleanup request including any success or failure
     * notifications. The queue manager will flush the trace (and bounce)
     * logfile, possibly after it has generated its own success or failure
     * notification records.
     * 
     * Postfix 2.2 compatibility: when ignoring differences in Postfix original
     * recipient information, also ignore differences in DSN attributes. We
     * do, however, keep the DSN attributes of the recipient that survives
     * duplicate elimination.
     */
    else {
	RECIPIENT rcpt;
	DSN     dsn;

	argv = cleanup_map1n_internal(state, recip, cleanup_virt_alias_maps,
				  cleanup_ext_prop_mask & EXT_PROP_VIRTUAL);
	if ((dsn_notify & DSN_NOTIFY_SUCCESS)
	    && (argv->argc > 1 || strcmp(recip, argv->argv[0]) != 0)) {
	    (void) DSN_SIMPLE(&dsn, "2.0.0", "alias expanded");
	    dsn.action = "expanded";
	    RECIPIENT_ASSIGN(&rcpt, 0, dsn_orcpt, dsn_notify, orcpt, recip);
	    cleanup_trace_append(state, &rcpt, &dsn);
	    dsn_notify = (dsn_notify == DSN_NOTIFY_SUCCESS ? DSN_NOTIFY_NEVER :
			  dsn_notify & ~DSN_NOTIFY_SUCCESS);
	}
	for (cpp = argv->argv; *cpp; cpp++) {
	    if ((var_enable_orcpt ?
		 been_here(state->dups, "%s\n%d\n%s\n%s",
			   dsn_orcpt, dsn_notify, orcpt, *cpp) :
		 been_here_fixed(state->dups, *cpp)) == 0) {
		if (dsn_notify)
		    cleanup_out_format(state, REC_TYPE_ATTR, "%s=%d",
				       MAIL_ATTR_DSN_NOTIFY, dsn_notify);
		if (*dsn_orcpt)
		    cleanup_out_format(state, REC_TYPE_ATTR, "%s=%s",
				       MAIL_ATTR_DSN_ORCPT, dsn_orcpt);
		cleanup_out_string(state, REC_TYPE_ORCP, orcpt);
		cleanup_out_string(state, REC_TYPE_RCPT, *cpp);
		state->rcpt_count++;
	    }
	}
	argv_free(argv);
    }
}
