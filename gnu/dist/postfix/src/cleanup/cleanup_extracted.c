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
/*	int	len;
/* DESCRIPTION
/*	This module processes message records with information extracted
/*	from message content, or with recipients that are stored after the
/*	message content. It updates recipient records, and writes extracted
/*	information records to the output.
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <mymalloc.h>
#include <nvtable.h>

/* Global library. */

#include <cleanup_user.h>
#include <qmgr_user.h>
#include <record.h>
#include <rec_type.h>
#include <mail_params.h>
#include <mail_proto.h>

/* Application-specific. */

#include "cleanup.h"

#define STR(x)	vstring_str(x)

static void cleanup_extracted_process(CLEANUP_STATE *, int, const char *, int);
static void cleanup_extracted_finish(CLEANUP_STATE *);

/* cleanup_extracted - initialize extracted segment */

void    cleanup_extracted(CLEANUP_STATE *state, int type,
			          const char *buf, int len)
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
				          const char *buf, int len)
{
    const char *encoding;
    const char generated_by_cleanup[] = {
	REC_TYPE_FILT, REC_TYPE_RDR, REC_TYPE_ATTR,
	REC_TYPE_RRTO, REC_TYPE_ERTO, 0,
    };

    if (msg_verbose)
	msg_info("extracted envelope %c %.*s", type, len, buf);

    if (strchr(REC_TYPE_EXTRACT, type) == 0) {
	msg_warn("%s: message rejected: "
		 "unexpected record type %d in extracted envelope",
		 state->queue_id, type);
	state->errs |= CLEANUP_STAT_BAD;
	return;
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
	if (state->return_receipt)
	    cleanup_out_string(state, REC_TYPE_RRTO, state->return_receipt);
	if (state->errors_to)
	    cleanup_out_string(state, REC_TYPE_ERTO, state->errors_to);
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
		 state->queue_id, buf);
	myfree(state->orig_rcpt);
	state->orig_rcpt = 0;
    }
    if (type == REC_TYPE_ORCP) {
	state->orig_rcpt = mystrdup(buf);
	return;
    }
    if (type == REC_TYPE_END) {
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
    if (strchr(generated_by_cleanup, type) != 0) {
	/* Use our own header/body info instead. */
	return;
    } else {
	/* Pass on other non-recipient record. */
	cleanup_out(state, type, buf, len);
	return;
    }
}

/* cleanup_extracted_finish - process one extracted envelope record */

void    cleanup_extracted_finish(CLEANUP_STATE *state)
{
    const char myname[] = "cleanup_extracted_finish";

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

    /*
     * vstream_fseek() would flush the buffer anyway, but the code just reads
     * better if we flush first, because it makes seek error handling more
     * straightforward.
     */
    if (vstream_fflush(state->dst)) {
	if (errno == EFBIG) {
	    msg_warn("%s: queue file size limit exceeded", state->queue_id);
	    state->errs |= CLEANUP_STAT_SIZE;
	} else {
	    msg_warn("%s: write queue file: %m", state->queue_id);
	    state->errs |= CLEANUP_STAT_WRITE;
	}
	return;
    }

    /*
     * Update the preliminary message size and count fields with the actual
     * values.
     */
    if (vstream_fseek(state->dst, 0L, SEEK_SET) < 0)
	msg_fatal("%s: vstream_fseek %s: %m", myname, cleanup_path);
    cleanup_out_format(state, REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
	    (REC_TYPE_SIZE_CAST1) (state->xtra_offset - state->data_offset),
		       (REC_TYPE_SIZE_CAST2) state->data_offset,
		       (REC_TYPE_SIZE_CAST3) state->rcpt_count,
		       (REC_TYPE_SIZE_CAST4) state->qmgr_opts);
}
