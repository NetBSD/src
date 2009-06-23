/*	$NetBSD: cleanup_envelope.c,v 1.2 2009/06/23 11:41:06 tron Exp $	*/

/*	$NetBSD: cleanup_envelope.c,v 1.2 2009/06/23 11:41:06 tron Exp $	*/

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
/*	ssize_t	len;
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
#include <ctype.h>

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
#include <mail_proto.h>
#include <dsn_mask.h>
#include <rec_attr_map.h>

/* Application-specific. */

#include "cleanup.h"

#define STR	vstring_str
#define STREQ(x,y) (strcmp((x), (y)) == 0)

static void cleanup_envelope_process(CLEANUP_STATE *, int, const char *, ssize_t);

/* cleanup_envelope - initialize message envelope */

void    cleanup_envelope(CLEANUP_STATE *state, int type,
			         const char *str, ssize_t len)
{

    /*
     * The message size and count record goes first, so it can easily be
     * updated in place. This information takes precedence over any size
     * estimate provided by the client. It's all in one record, data size
     * first, for backwards compatibility reasons.
     */
    cleanup_out_format(state, REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
		       (REC_TYPE_SIZE_CAST1) 0,	/* extra offs - content offs */
		       (REC_TYPE_SIZE_CAST2) 0,	/* content offset */
		       (REC_TYPE_SIZE_CAST3) 0,	/* recipient count */
		       (REC_TYPE_SIZE_CAST4) 0,	/* qmgr options */
		       (REC_TYPE_SIZE_CAST5) 0);	/* content length */

    /*
     * Pass control to the actual envelope processing routine.
     */
    state->action = cleanup_envelope_process;
    cleanup_envelope_process(state, type, str, len);
}

/* cleanup_envelope_process - process one envelope record */

static void cleanup_envelope_process(CLEANUP_STATE *state, int type,
				             const char *buf, ssize_t len)
{
    const char *myname = "cleanup_envelope_process";
    char   *attr_name;
    char   *attr_value;
    const char *error_text;
    int     extra_opts;
    int     junk;
    int     mapped_type = type;
    const char *mapped_buf = buf;
    int     milter_count;

#ifdef DELAY_ACTION
    int     defer_delay;

#endif

    if (msg_verbose)
	msg_info("initial envelope %c %.*s", type, (int) len, buf);

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

    /*
     * XXX We instantiate a MILTERS structure even when the filter count is
     * zero (for example, all filters are in ACCEPT state, or the SMTP server
     * sends a dummy MILTERS structure without any filters), otherwise the
     * cleanup server would apply the non_smtpd_milters setting
     * inappropriately.
     */
    if (type == REC_TYPE_MILT_COUNT) {
	/* Not part of queue file format. */
	if ((milter_count = atoi(buf)) >= 0)
	    cleanup_milter_receive(state, milter_count);
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
	    mapped_buf = attr_value;
	    mapped_type = junk;
	}
    }

    /*
     * Sanity check.
     */
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
	if (state->arrival_time.tv_sec == 0) {
	    msg_warn("%s: message rejected: missing time envelope record",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}

	/*
	 * XXX This works by accident, because the sender is recorded at the
	 * beginning of the envelope segment.
	 */
	if ((state->flags & CLEANUP_FLAG_WARN_SEEN) == 0
	    && state->sender && *state->sender
	    && var_delay_warn_time > 0) {
	    cleanup_out_format(state, REC_TYPE_WARN, REC_TYPE_WARN_FORMAT,
			       REC_TYPE_WARN_ARG(state->arrival_time.tv_sec
						 + var_delay_warn_time));
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
	if (cleanup_milters != 0
	    && state->milters == 0
	    && CLEANUP_MILTER_OK(state))
	    cleanup_milter_emul_rcpt(state, cleanup_milters, state->recip);
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
    if (mapped_type == REC_TYPE_DSN_ORCPT) {
	if (state->dsn_orcpt) {
	    msg_warn("%s: ignoring out-of-order DSN original recipient record <%.200s>",
		     state->queue_id, state->dsn_orcpt);
	    myfree(state->dsn_orcpt);
	}
	state->dsn_orcpt = mystrdup(mapped_buf);
	return;
    }
    if (mapped_type == REC_TYPE_DSN_NOTIFY) {
	if (state->dsn_notify) {
	    msg_warn("%s: ignoring out-of-order DSN notify record <%d>",
		     state->queue_id, state->dsn_notify);
	    state->dsn_notify = 0;
	}
	if (!alldig(mapped_buf) || (junk = atoi(mapped_buf)) == 0
	    || DSN_NOTIFY_OK(junk) == 0)
	    msg_warn("%s: ignoring malformed DSN notify record <%.200s>",
		     state->queue_id, buf);
	else
	    state->qmgr_opts |=
		QMGR_READ_FLAG_FROM_DSN(state->dsn_notify = junk);
	return;
    }
    if (type == REC_TYPE_ORCP) {
	if (state->orig_rcpt != 0) {
	    msg_warn("%s: ignoring out-of-order original recipient record <%.200s>",
		     state->queue_id, state->orig_rcpt);
	    myfree(state->orig_rcpt);
	}
	state->orig_rcpt = mystrdup(buf);
	return;
    }
    if (type == REC_TYPE_MESG) {
	state->action = cleanup_message;
	if (state->flags & CLEANUP_FLAG_INRCPT) {
	    if (state->milters || cleanup_milters) {
		/* Make room to append recipient. */
		if ((state->append_rcpt_pt_offset = vstream_ftell(state->dst)) < 0)
		    msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
		cleanup_out_format(state, REC_TYPE_PTR, REC_TYPE_PTR_FORMAT, 0L);
		if ((state->append_rcpt_pt_target = vstream_ftell(state->dst)) < 0)
		    msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
	    }
	    state->flags &= ~CLEANUP_FLAG_INRCPT;
	}
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
    if (mapped_type == REC_TYPE_CTIME)
	/* Use our own expiration time base record instead. */
	return;
    if (type == REC_TYPE_TIME) {
	/* First instance wins. */
	if (state->arrival_time.tv_sec == 0) {
	    REC_TYPE_TIME_SCAN(buf, state->arrival_time);
	    cleanup_out(state, type, buf, len);
	}
	/* Generate our own expiration time base record. */
	cleanup_out_format(state, REC_TYPE_ATTR, "%s=%ld",
			   MAIL_ATTR_CREATE_TIME, (long) time((time_t *) 0));
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
	if (state->milters || cleanup_milters) {
	    /* Remember the sender record offset. */
	    if ((state->sender_pt_offset = vstream_ftell(state->dst)) < 0)
		msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
	}
	cleanup_addr_sender(state, buf);
	if (state->milters || cleanup_milters) {
	    /* Make room to replace sender. */
	    rec_pad(state->dst, REC_TYPE_PTR, REC_TYPE_PTR_PAYL_SIZE);
	    /* Remember the after-sender record offset. */
	    if ((state->sender_pt_target = vstream_ftell(state->dst)) < 0)
		msg_fatal("%s: vstream_ftell %s: %m:", myname, cleanup_path);
	}
	if (cleanup_milters != 0
	    && state->milters == 0
	    && CLEANUP_MILTER_OK(state))
	    cleanup_milter_emul_mail(state, cleanup_milters, state->sender);
	return;
    }
    if (mapped_type == REC_TYPE_DSN_ENVID) {
	/* Allow only one instance. */
	if (state->dsn_envid != 0) {
	    msg_warn("%s: message rejected: multiple DSN envelope ID records",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (!allprint(mapped_buf)) {
	    msg_warn("%s: message rejected: bad DSN envelope ID record",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	state->dsn_envid = mystrdup(mapped_buf);
	cleanup_out(state, type, buf, len);
	return;
    }
    if (mapped_type == REC_TYPE_DSN_RET) {
	/* Allow only one instance. */
	if (state->dsn_ret != 0) {
	    msg_warn("%s: message rejected: multiple DSN RET records",
		     state->queue_id);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (!alldig(mapped_buf) || (junk = atoi(mapped_buf)) == 0
	    || DSN_RET_OK(junk) == 0) {
	    msg_warn("%s: message rejected: bad DSN RET record <%.200s>",
		     state->queue_id, buf);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	state->dsn_ret = junk;
	cleanup_out(state, type, buf, len);
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
    /* XXX Needed for cleanup_bounce(); sanity check usage. */
    if (type == REC_TYPE_VERP) {
	if (state->verp_delims == 0) {
	    if (state->sender == 0 || state->sender[0] == 0) {
		msg_warn("%s: ignoring VERP request for null sender",
			 state->queue_id);
	    } else if (verp_delims_verify(buf) != 0) {
		msg_warn("%s: ignoring bad VERP request: \"%.100s\"",
			 state->queue_id, buf);
	    } else {
		state->verp_delims = mystrdup(buf);
		cleanup_out(state, type, buf, len);
	    }
	}
	return;
    }
    if (type == REC_TYPE_ATTR) {
	if (state->attr->used >= var_qattr_count_limit) {
	    msg_warn("%s: message rejected: attribute count exceeds limit %d",
		     state->queue_id, var_qattr_count_limit);
	    state->errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (strcmp(attr_name, MAIL_ATTR_RWR_CONTEXT) == 0) {
	    /* Choose header rewriting context. See also cleanup_addr.c. */
	    if (STREQ(attr_value, MAIL_ATTR_RWR_LOCAL)) {
		state->hdr_rewrite_context = MAIL_ATTR_RWR_LOCAL;
	    } else if (STREQ(attr_value, MAIL_ATTR_RWR_REMOTE)) {
		state->hdr_rewrite_context =
		    (*var_remote_rwr_domain ? MAIL_ATTR_RWR_REMOTE : 0);
	    } else {
		msg_warn("%s: message rejected: bad rewriting context: %.100s",
			 state->queue_id, attr_value);
		state->errs |= CLEANUP_STAT_BAD;
		return;
	    }
	}
	nvtable_update(state->attr, attr_name, attr_value);
	cleanup_out(state, type, buf, len);
	return;
    } else {
	cleanup_out(state, type, buf, len);
	return;
    }
}
