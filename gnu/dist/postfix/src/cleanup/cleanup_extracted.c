/*	$NetBSD: cleanup_extracted.c,v 1.1.1.10 2007/05/19 16:28:06 heas Exp $	*/

/*++
/* NAME
/*	cleanup_extracted 3
/* SUMMARY
/*	process extracted segment
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_extracted(state, type, buf, len)
/*	CLEANUP_STATE *state;
/*	int	type;
/*	const char *buf;
/*	ssize_t	len;
/* DESCRIPTION
/*	This module processes message records with information extracted
/*	from message content, or with recipients that are stored after the
/*	message content. It updates recipient records, writes extracted
/*	information records to the output, and writes the queue
/*	file end marker.  The queue file is left in a state that
/*	is suitable for Milter inspection, but the size record still
/*	contains dummy values.
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
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <nvtable.h>
#include <stringops.h>

/* Global library. */

#include <cleanup_user.h>
#include <qmgr_user.h>
#include <record.h>
#include <rec_type.h>
#include <mail_params.h>
#include <mail_proto.h>
#include <dsn_mask.h>
#include <rec_attr_map.h>

/* Application-specific. */

#include "cleanup.h"

#define STR(x)	vstring_str(x)

static void cleanup_extracted_process(CLEANUP_STATE *, int, const char *, ssize_t);
static void cleanup_extracted_finish(CLEANUP_STATE *);

/* cleanup_extracted - initialize extracted segment */

void    cleanup_extracted(CLEANUP_STATE *state, int type,
			          const char *buf, ssize_t len)
{

    /*
     * Start the extracted segment.
     */
    cleanup_out_string(state, REC_TYPE_XTRA, "");

    /*
     * Pass control to the actual envelope processing routine.
     */
    state->action = cleanup_extracted_process;
    cleanup_extracted_process(state, type, buf, len);
}

/* cleanup_extracted_process - process one extracted envelope record */

void    cleanup_extracted_process(CLEANUP_STATE *state, int type,
				          const char *buf, ssize_t len)
{
    const char *myname = "cleanup_extracted_process";
    const char *encoding;
    char   *attr_name;
    char   *attr_value;
    const char *error_text;
    int     extra_opts;
    int     junk;

#ifdef DELAY_ACTION
    int     defer_delay;

#endif

    if (msg_verbose)
	msg_info("extracted envelope %c %.*s", type, (int) len, buf);

    if (type == REC_TYPE_FLGS) {
	/* Not part of queue file format. */
	extra_opts = atoi(buf);
	if (extra_opts & ~CLEANUP_FLAG_MASK_EXTRA)
	    msg_warn("%s: ignoring bad extra flags: 0x%x",
		     state->queue_id, extra_opts);
	else
	    state->flags |= extra_opts;
	return;
    }
#ifdef DELAY_ACTION
    if (type == REC_TYPE_DELAY) {
	/* Not part of queue file format. */
	defer_delay = atoi(buf);
	if (defer_delay <= 0)
	    msg_warn("%s: ignoring bad delay time: %s", state->queue_id, buf);
	else
	    state->defer_delay = defer_delay;
	return;
    }
#endif

    if (strchr(REC_TYPE_EXTRACT, type) == 0) {
	msg_warn("%s: message rejected: "
		 "unexpected record type %d in extracted envelope",
		 state->queue_id, type);
	state->errs |= CLEANUP_STAT_BAD;
	return;
    }

    /*
     * Map DSN attribute name to pseudo record type so that we don't have to
     * pollute the queue file with records that are incompatible with past
     * Postfix versions. Preferably, people should be able to back out from
     * an upgrade without losing mail.
     */
    if (type == REC_TYPE_ATTR) {
	vstring_strcpy(state->attr_buf, buf);
	error_text = split_nameval(STR(state->attr_buf), &attr_name, &attr_value);
	if (error_text != 0) {
	    msg_warn("%s: message rejected: malformed attribute: %s: %.100s",
		     state->queue_id, error_text, buf);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	/* Zero-length values are place holders for unavailable values. */
	if (*attr_value == 0) {
	    msg_warn("%s: spurious null attribute value for \"%s\" -- ignored",
		     state->queue_id, attr_name);
	    return;
	}
	if ((junk = rec_attr_map(attr_name)) != 0) {
	    buf = attr_value;
	    type = junk;
	}
    }

    /*
     * On the transition from non-recipient records to recipient records,
     * emit optional information from header/body content.
     */
    if ((state->flags & CLEANUP_FLAG_INRCPT) == 0
	&& strchr(REC_TYPE_EXT_RECIPIENT, type) != 0) {
	if (state->filter != 0)
	    cleanup_out_string(state, REC_TYPE_FILT, state->filter);
	if (state->redirect != 0)
	    cleanup_out_string(state, REC_TYPE_RDR, state->redirect);
	if ((encoding = nvtable_find(state->attr, MAIL_ATTR_ENCODING)) != 0)
	    cleanup_out_format(state, REC_TYPE_ATTR, "%s=%s",
			       MAIL_ATTR_ENCODING, encoding);
	state->flags |= CLEANUP_FLAG_INRCPT;
    }

    /*
     * Extracted envelope recipient record processing.
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
	if (cleanup_milters != 0
	    && state->milters == 0
	    && CLEANUP_MILTER_OK(state))
	    cleanup_milter_emul_rcpt(state, cleanup_milters, buf);
	myfree(state->orig_rcpt);
	state->orig_rcpt = 0;
	if (state->dsn_orcpt != 0) {
	    myfree(state->dsn_orcpt);
	    state->dsn_orcpt = 0;
	}
	state->dsn_notify = 0;
	return;
    }
    if (type == REC_TYPE_DONE || type == REC_TYPE_DRCP) {
	if (state->orig_rcpt != 0) {
	    myfree(state->orig_rcpt);
	    state->orig_rcpt = 0;
	}
	if (state->dsn_orcpt != 0) {
	    myfree(state->dsn_orcpt);
	    state->dsn_orcpt = 0;
	}
	state->dsn_notify = 0;
	return;
    }
    if (type == REC_TYPE_DSN_ORCPT) {
	if (state->dsn_orcpt) {
	    msg_warn("%s: ignoring out-of-order DSN original recipient record <%.200s>",
		     state->queue_id, state->dsn_orcpt);
	    myfree(state->dsn_orcpt);
	}
	state->dsn_orcpt = mystrdup(buf);
	return;
    }
    if (type == REC_TYPE_DSN_NOTIFY) {
	if (state->dsn_notify) {
	    msg_warn("%s: ignoring out-of-order DSN notify record <%d>",
		     state->queue_id, state->dsn_notify);
	    state->dsn_notify = 0;
	}
	if (!alldig(buf) || (junk = atoi(buf)) == 0 || DSN_NOTIFY_OK(junk) == 0)
	    msg_warn("%s: ignoring malformed dsn notify record <%.200s>",
		     state->queue_id, buf);
	else
	    state->qmgr_opts |=
		QMGR_READ_FLAG_FROM_DSN(state->dsn_notify = junk);
	return;
    }
    if (type == REC_TYPE_ORCP) {
	if (state->orig_rcpt != 0) {
	    msg_warn("%s: ignoring out-of-order original recipient record <%.200s>",
		     state->queue_id, buf);
	    myfree(state->orig_rcpt);
	}
	state->orig_rcpt = mystrdup(buf);
	return;
    }
    if (type == REC_TYPE_END) {
	/* Make room to append recipient. */
	if ((state->milters || cleanup_milters)
	    && state->append_rcpt_pt_offset < 0) {
	    if ((state->append_rcpt_pt_offset = vstream_ftell(state->dst)) < 0)
		msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
	    cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT, 0L);
	    if ((state->append_rcpt_pt_target = vstream_ftell(state->dst)) < 0)
		msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
	}
	state->flags &= ~CLEANUP_FLAG_INRCPT;
	state->flags |= CLEANUP_FLAG_END_SEEN;
	cleanup_extracted_finish(state);
	return;
    }

    /*
     * Extracted envelope non-recipient record processing.
     */
    if (state->flags & CLEANUP_FLAG_INRCPT)
	/* Tell qmgr that recipient records are mixed with other information. */
	state->qmgr_opts |= QMGR_READ_FLAG_MIXED_RCPT_OTHER;
    cleanup_out(state, type, buf, len);
    return;
}

/* cleanup_extracted_finish - complete the third message segment */

void    cleanup_extracted_finish(CLEANUP_STATE *state)
{

    /*
     * On the way out, add the optional automatic BCC recipient.
     */
    if ((state->flags & CLEANUP_FLAG_BCC_OK)
	&& state->recip != 0 && *var_always_bcc)
	cleanup_addr_bcc(state, var_always_bcc);

    /*
     * Terminate the extracted segment.
     */
    cleanup_out_string(state, REC_TYPE_END, "");
}
