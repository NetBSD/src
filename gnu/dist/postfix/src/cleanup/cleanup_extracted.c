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
/*	char	*buf;
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

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <vstream.h>
#include <argv.h>
#include <mymalloc.h>

/* Global library. */

#include <cleanup_user.h>
#include <record.h>
#include <rec_type.h>
#include <mail_params.h>
#include <ext_prop.h>

/* Application-specific. */

#include "cleanup.h"

#define STR(x)	vstring_str(x)

static void cleanup_extracted_process(CLEANUP_STATE *, int, char *, int);

/* cleanup_extracted - initialize extracted segment */

void    cleanup_extracted(CLEANUP_STATE *state, int type, char *buf, int len)
{

    /*
     * Start the extracted segment.
     */
    cleanup_out_string(state, REC_TYPE_XTRA, "");

    /*
     * Always emit Return-Receipt-To and Errors-To records, and always emit
     * them ahead of extracted recipients, so that the queue manager does not
     * waste lots of time searching through large numbers of recipient
     * addresses.
     */
    cleanup_out_string(state, REC_TYPE_RRTO, state->return_receipt ?
		       state->return_receipt : "");

    cleanup_out_string(state, REC_TYPE_ERTO, state->errors_to ?
		       state->errors_to : state->sender);

    /*
     * Pass control to the routine that processes the extracted segment.
     */
    state->action = cleanup_extracted_process;
    cleanup_extracted_process(state, type, buf, len);
}

/* cleanup_extracted_process - process extracted segment */

static void cleanup_extracted_process(CLEANUP_STATE *state, int type, char *buf, int unused_len)
{
    char   *myname = "cleanup_extracted_process";
    VSTRING *clean_addr;
    ARGV   *rcpt;
    char  **cpp;

    if (type == REC_TYPE_RRTO) {
	/* XXX Use extracted information instead. */
	return;
    }
    if (type == REC_TYPE_ERTO) {
	/* XXX Use extracted information instead. */
	return;
    }
    if (type == REC_TYPE_RCPT) {
	clean_addr = vstring_alloc(100);
	cleanup_rewrite_internal(clean_addr, *buf ? buf : var_empty_addr);
	if (cleanup_rcpt_canon_maps)
	    cleanup_map11_internal(state, clean_addr, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	if (cleanup_comm_canon_maps)
	    cleanup_map11_internal(state, clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	cleanup_out_recipient(state, STR(clean_addr));
	if (state->recip == 0)
	    state->recip = mystrdup(STR(clean_addr));
	vstring_free(clean_addr);
	return;
    }
    if (type != REC_TYPE_END) {
	msg_warn("%s: unexpected record type %d in extracted segment",
		 state->queue_id, type);
	state->errs |= CLEANUP_STAT_BAD;
	return;
    }

    /*
     * Optionally account for missing recipient envelope records.
     * 
     * Don't extract recipients when some header was too long. We have
     * incomplete information.
     * 
     * XXX Code duplication from cleanup_envelope.c. This should be in one
     * place.
     */
    if (state->recip == 0 && (state->errs & CLEANUP_STAT_HOVFL) == 0) {
	rcpt = (state->resent[0] ? state->resent_recip : state->recipients);
	if (rcpt->argc >= var_extra_rcpt_limit) {
	    state->errs |= CLEANUP_STAT_ROVFL;
	} else {
	    if (*var_always_bcc && rcpt->argv[0]) {
		clean_addr = vstring_alloc(100);
		cleanup_rewrite_internal(clean_addr, var_always_bcc);
		if (cleanup_rcpt_canon_maps)
		    cleanup_map11_internal(state, clean_addr, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
		if (cleanup_comm_canon_maps)
		    cleanup_map11_internal(state, clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
		argv_add(rcpt, STR(clean_addr), (char *) 0);
		vstring_free(clean_addr);
	    }
	    argv_terminate(rcpt);
	    for (cpp = rcpt->argv; CLEANUP_OUT_OK(state) && *cpp; cpp++)
		cleanup_out_recipient(state, *cpp);
	    if (rcpt->argv[0])
		state->recip = mystrdup(rcpt->argv[0]);
	}
    }

    /*
     * Terminate the extracted segment.
     */
    cleanup_out_string(state, REC_TYPE_END, "");
    state->end_seen = 1;

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
     * values.  For forward compatibility, we put the info into one record
     * (so that it is possible to switch back to an older Postfix version).
     */
    if (vstream_fseek(state->dst, 0L, SEEK_SET) < 0)
	msg_fatal("%s: vstream_fseek %s: %m", myname, cleanup_path);
    cleanup_out_format(state, REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT,
	    (REC_TYPE_SIZE_CAST1) (state->xtra_offset - state->data_offset),
		       (REC_TYPE_SIZE_CAST2) state->data_offset,
		       (REC_TYPE_SIZE_CAST3) state->rcpt_count);

    /*
     * Update the preliminary start-of-content marker with the actual value.
     * For forward compatibility, we keep this information until the end of
     * the year 2002 (so that it is possible to switch back to an older
     * Postfix version).
     */
    if (vstream_fseek(state->dst, state->mesg_offset, SEEK_SET) < 0)
	msg_fatal("%s: vstream_fseek %s: %m", myname, cleanup_path);
    cleanup_out_format(state, REC_TYPE_MESG, REC_TYPE_MESG_FORMAT,
		       (REC_TYPE_MESG_CAST) state->xtra_offset);
}
