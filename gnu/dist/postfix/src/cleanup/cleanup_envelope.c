/*++
/* NAME
/*	cleanup_envelope 3
/* SUMMARY
/*	process envelope segment
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	void	cleanup_envelope(state, type, buf, len)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	const char *buf;
/*	int	len;
/* DESCRIPTION
/*	This module processes envelope records and writes the result
/*	to the queue file.  It validates the message structure, rewrites
/*	sender/recipient addresses to canonical form, and expands recipients
/*	according to entries in the virtual table. This routine absorbs but
/*	does not emit the envelope to content boundary record.
/*
/*	Arguments:
/* .IP state
/*	Queue file and message processing state. This state is updated
/*	as records are processed and as errors happen.
/* .IP type
/*	Record type.
/* .IP buf
/*	Record content.
/* .IP len
/*	Record content length.
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
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <stringops.h>
#include <nvtable.h>

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <cleanup_user.h>
#include <qmgr_user.h>
#include <mail_params.h>
#include <verp_sender.h>

/* Application-specific. */

#include "cleanup.h"

#define STR	vstring_str

static void cleanup_envelope_process(CLEANUP_STATE *, int, const char *, int);

/* cleanup_envelope - initialize message envelope */

void    cleanup_envelope(CLEANUP_STATE *state, int type,
			         const char *str, int len)
{

    /*
     * The message size and count record goes first, so it can easily be
     * updated in place. This information takes precedence over any size
     * estimate provided by the client. It's all in one record, data size
     * first, for backwards compatibility reasons.
     */
    cleanup_out_format(state, REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
		       (REC_TYPE_SIZE_CAST1) 0,	/* content size */
		       (REC_TYPE_SIZE_CAST2) 0,	/* content offset */
		       (REC_TYPE_SIZE_CAST3) 0,	/* recipient count */
		       (REC_TYPE_SIZE_CAST4) 0);/* qmgr options */

    /*
     * Pass control to the actual envelope processing routine.
     */
    state->action = cleanup_envelope_process;
    cleanup_envelope_process(state, type, str, len);
}

/* cleanup_envelope_process - process one envelope record */

static void cleanup_envelope_process(CLEANUP_STATE *state, int type,
				             const char *buf, int len)
{
    char   *attr_name;
    char   *attr_value;
    const char *error_text;
    int     extra_opts;

    if (msg_verbose)
	msg_info("initial envelope %c %.*s", type, len, buf);

    if (type == REC_TYPE_FLGS) {
	/* Not part of queue file format. */
	extra_opts = atol(buf);
	if (extra_opts & ~CLEANUP_FLAG_MASK_EXTRA)
	    msg_warn("%s: ignoring bad extra flags: 0x%x",
		     state->queue_id, extra_opts);
	else
	    state->flags |= extra_opts;
	return;
    }
    if (strchr(REC_TYPE_ENVELOPE, type) == 0) {
	msg_warn("%s: message rejected: unexpected record type %d in envelope",
		 state->queue_id, type);
	state->errs |= CLEANUP_STAT_BAD;
	return;
    }

    /*
     * Although recipient records appear at the end of the initial or
     * extracted envelope, the code for processing recipient records is first
     * because there can be lots of them.
     * 
     * Recipient records may be mixed with other information (such as FILTER or
     * REDIRECT actions from SMTPD). In that case the queue manager needs to
     * examine all queue file records before it can start delivery. This is
     * not a problem when SMTPD recipient lists are small.
     * 
     * However, if recipient records are not mixed with other records
     * (typically, mailing list mail) then we can make an optimization: the
     * queue manager does not need to examine every envelope record before it
     * can start deliveries. This can help with very large mailing lists.
     */

    /*
     * On the transition from non-recipient records to recipient records,
     * emit some records and do some sanity checks.
     * 
     * XXX Moving the envelope sender (and the test for its presence) to the
     * extracted segment can reduce qmqpd memory requirements because it no
     * longer needs to read the entire message into main memory.
     */
    if ((state->flags & CLEANUP_FLAG_INRCPT) == 0
	&& strchr(REC_TYPE_ENV_RECIPIENT, type) != 0) {
	if (state->sender == 0) {
	    msg_warn("%s: message rejected: missing sender envelope record",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (state->time == 0) {
	    msg_warn("%s: message rejected: missing time envelope record",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if ((state->flags & CLEANUP_FLAG_WARN_SEEN) == 0
	    && var_delay_warn_time > 0) {
	    cleanup_out_format(state, REC_TYPE_WARN, REC_TYPE_WARN_FORMAT,
			       (long) state->time + var_delay_warn_time);
	}
	state->flags |= CLEANUP_FLAG_INRCPT;
    }

    /*
     * Initial envelope recipient record processing.
     */
    if (type == REC_TYPE_RCPT) {
	if (state->sender == 0) {		/* protect showq */
	    msg_warn("%s: message rejected: envelope recipient precedes sender",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (state->orig_rcpt == 0)
	    state->orig_rcpt = mystrdup(buf);
	cleanup_addr_recipient(state, buf);
	myfree(state->orig_rcpt);
	state->orig_rcpt = 0;
	return;
    }
    if (type == REC_TYPE_DONE) {
	if (state->orig_rcpt != 0) {
	    myfree(state->orig_rcpt);
	    state->orig_rcpt = 0;
	}
	return;
    }
    if (state->orig_rcpt != 0) {
	/* REC_TYPE_ORCP must be followed by REC_TYPE_RCPT or REC_TYPE DONE. */
	msg_warn("%s: ignoring out-of-order original recipient record <%.200s>",
		 state->queue_id, state->orig_rcpt);
	myfree(state->orig_rcpt);
	state->orig_rcpt = 0;
    }
    if (type == REC_TYPE_ORCP) {
	state->orig_rcpt = mystrdup(buf);
	return;
    }
    if (type == REC_TYPE_MESG) {
	state->action = cleanup_message;
	state->flags &= ~CLEANUP_FLAG_INRCPT;
	return;
    }

    /*
     * Initial envelope non-recipient record processing.
     */
    if (state->flags & CLEANUP_FLAG_INRCPT)
	/* Tell qmgr that recipient records are mixed with other information. */
	state->qmgr_opts |= QMGR_READ_FLAG_MIXED_RCPT_OTHER;
    if (type == REC_TYPE_SIZE)
	/* Use our own SIZE record instead. */
	return;
    if (type == REC_TYPE_TIME) {
	/* First instance wins. */
	if (state->time == 0) {
	    state->time = atol(buf);
	    cleanup_out(state, type, buf, len);
	}
	return;
    }
    if (type == REC_TYPE_FULL) {
	/* First instance wins. */
	if (state->fullname == 0) {
	    state->fullname = mystrdup(buf);
	    cleanup_out(state, type, buf, len);
	}
	return;
    }
    if (type == REC_TYPE_FROM) {
	/* Allow only one instance. */
	if (state->sender != 0) {
	    msg_warn("%s: message rejected: multiple envelope sender records",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	cleanup_addr_sender(state, buf);
	return;
    }
    if (type == REC_TYPE_WARN) {
	/* First instance wins. */
	if ((state->flags & CLEANUP_FLAG_WARN_SEEN) == 0) {
	    state->flags |= CLEANUP_FLAG_WARN_SEEN;
	    cleanup_out(state, type, buf, len);
	}
	return;
    }
    if (type == REC_TYPE_ATTR) {
	char   *sbuf;

	if (state->attr->used >= var_qattr_count_limit) {
	    msg_warn("%s: message rejected: attribute count exceeds limit %d",
		     state->queue_id, var_qattr_count_limit);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	sbuf = mystrdup(buf);
	if ((error_text = split_nameval(sbuf, &attr_name, &attr_value)) != 0) {
	    msg_warn("%s: message rejected: malformed attribute: %s: %.100s",
		     state->queue_id, error_text, buf);
	    state->errs |= CLEANUP_STAT_BAD;
	    myfree(sbuf);
	    return;
	}
	nvtable_update(state->attr, attr_name, attr_value);
	myfree(sbuf);
	cleanup_out(state, type, buf, len);
	return;
    } else {
	cleanup_out(state, type, buf, len);
	return;
    }
}
