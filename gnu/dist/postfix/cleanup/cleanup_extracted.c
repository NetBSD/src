/*++
/* NAME
/*	cleanup_extracted 3
/* SUMMARY
/*	process extracted segment
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_extracted(void)
/* DESCRIPTION
/*	This module processes message segments for information
/*	extracted from message content. It requires that the input
/*	contains no extracted information, and writes extracted
/*	information records to the output.
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

/* cleanup_extracted - generate segment with header-extracted information */

void    cleanup_extracted(void)
{
    VSTRING *clean_addr;
    ARGV   *rcpt;
    char  **cpp;
    int     type;

    /*
     * Start the extracted segment, before copying recipient records.
     */
    cleanup_out_string(REC_TYPE_XTRA, "");

    /*
     * Do not complain in case of premature EOF - most likely the client
     * aborted the operation.
     * 
     * XXX Rely on the front-end programs to enforce record size limits.
     */
    while (CLEANUP_OUT_OK()) {
	if ((type = rec_get(cleanup_src, cleanup_inbuf, 0)) < 0) {
	    cleanup_errs |= CLEANUP_STAT_BAD;
	    return;
	}
	if (type == REC_TYPE_RRTO) {
	     /* XXX Use extracted information instead. */ ;
	} else if (type == REC_TYPE_ERTO) {
	     /* XXX Use extracted information instead. */ ;
	} else if (type == REC_TYPE_RCPT) {
	    clean_addr = vstring_alloc(100);
	    cleanup_rewrite_internal(clean_addr, *STR(cleanup_inbuf) ?
				     STR(cleanup_inbuf) : var_empty_addr);
	    if (cleanup_rcpt_canon_maps)
		cleanup_map11_internal(clean_addr, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_comm_canon_maps)
		cleanup_map11_internal(clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    cleanup_out_recipient(STR(clean_addr));
	    if (cleanup_recip == 0)
		cleanup_recip = mystrdup(STR(clean_addr));
	    vstring_free(clean_addr);
	} else if (type == REC_TYPE_END) {
	    break;
	} else {
	    msg_warn("%s: unexpected record type %d in extracted segment",
		     cleanup_queue_id, type);
	    cleanup_errs |= CLEANUP_STAT_BAD;
	    if (type >= 0)
		cleanup_skip();
	    return;
	}
    }

    /*
     * Always emit Return-Receipt-To and Errors-To records, and always emit
     * them ahead of extracted recipients, so that the queue manager does not
     * waste lots of time searching through large numbers of recipient
     * addresses.
     */
    cleanup_out_string(REC_TYPE_RRTO, cleanup_return_receipt ?
		       cleanup_return_receipt : "");

    cleanup_out_string(REC_TYPE_ERTO, cleanup_errors_to ?
		       cleanup_errors_to : cleanup_sender);

    /*
     * Optionally account for missing recipient envelope records.
     * 
     * Don't extract recipients when some header was too long. We have
     * incomplete information.
     * 
     * XXX Code duplication from cleanup_envelope.c. This should be in one
     * place.
     */
    if (cleanup_recip == 0 && (cleanup_errs & CLEANUP_STAT_HOVFL) == 0) {
	rcpt = (cleanup_resent[0] ? cleanup_resent_recip : cleanup_recipients);
	if (*var_always_bcc && rcpt->argv[0]) {
	    clean_addr = vstring_alloc(100);
	    cleanup_rewrite_internal(clean_addr, var_always_bcc);
	    if (cleanup_rcpt_canon_maps)
		cleanup_map11_internal(clean_addr, cleanup_rcpt_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_comm_canon_maps)
		cleanup_map11_internal(clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    argv_add(rcpt, STR(clean_addr), (char *) 0);
	    vstring_free(clean_addr);
	}
	argv_terminate(rcpt);
	for (cpp = rcpt->argv; CLEANUP_OUT_OK() && *cpp; cpp++)
	    cleanup_out_recipient(*cpp);
	if (rcpt->argv[0])
	    cleanup_recip = mystrdup(rcpt->argv[0]);
    }

    /*
     * Terminate the extracted segment.
     */
    cleanup_out_string(REC_TYPE_END, "");
}
