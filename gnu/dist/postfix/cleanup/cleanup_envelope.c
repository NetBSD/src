/*++
/* NAME
/*	cleanup_envelope 3
/* SUMMARY
/*	process envelope segment
/* SYNOPSIS
/*	#include <cleanup.h>
/*
/*	void	cleanup_envelope()
/* DESCRIPTION
/*	This module processes the envelope segment of a mail message.
/*	While copying records from input to output it validates the
/*	message structure, rewrites sender/recipient addresses
/*	to canonical form, and expands recipients according to
/*	entries in the virtual table.
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

/* Global library. */

#include <record.h>
#include <rec_type.h>
#include <cleanup_user.h>
#include <tok822.h>
#include <mail_params.h>
#include <ext_prop.h>
#include <mail_addr.h>
#include <canon_addr.h>

/* Application-specific. */

#include "cleanup.h"

#define STR	vstring_str

/* cleanup_envelope - process envelope segment */

void    cleanup_envelope(void)
{
    VSTRING *clean_addr = vstring_alloc(100);
    int     type = 0;
    long    warn_time = 0;

    /*
     * The message content size record goes first, so it can easily be
     * updated in place. This information takes precedence over any size
     * estimate provided by the client. Size goes first so that it it easy to
     * produce queue file reports.
     */
    cleanup_out_format(REC_TYPE_SIZE, REC_TYPE_SIZE_FORMAT, 0L);

    /*
     * XXX Rely on the front-end programs to enforce record size limits.
     */
    while (CLEANUP_OUT_OK()) {
	if ((type = rec_get(cleanup_src, cleanup_inbuf, 0)) < 0) {
	    cleanup_errs |= CLEANUP_STAT_BAD;
	    break;
	}
	if (type == REC_TYPE_MESG) {
	    if (cleanup_sender == 0 || cleanup_time == 0) {
		msg_warn("%s: missing sender or time envelope record",
			 cleanup_queue_id);
		cleanup_errs |= CLEANUP_STAT_BAD;
	    } else {
		if (warn_time == 0 && var_delay_warn_time > 0)
		    warn_time = cleanup_time + var_delay_warn_time * 3600L;
		if (warn_time)
		    cleanup_out_format(REC_TYPE_WARN, REC_TYPE_WARN_FORMAT,
				       warn_time);
	    }
	    break;
	}
	if (strchr(REC_TYPE_ENVELOPE, type) == 0) {
	    msg_warn("%s: unexpected record type %d in envelope",
		     cleanup_queue_id, type);
	    cleanup_errs |= CLEANUP_STAT_BAD;
	    break;
	}
	if (msg_verbose)
	    msg_info("envelope %c %s", type, STR(cleanup_inbuf));

	if (type == REC_TYPE_TIME) {
	    cleanup_time = atol(STR(cleanup_inbuf));
	    CLEANUP_OUT_BUF(type, cleanup_inbuf);
	} else if (type == REC_TYPE_FULL) {
	    cleanup_fullname = mystrdup(STR(cleanup_inbuf));
	} else if (type == REC_TYPE_FROM) {
	    cleanup_rewrite_internal(clean_addr, STR(cleanup_inbuf));
	    if (strncasecmp(STR(clean_addr), MAIL_ADDR_MAIL_DAEMON "@",
			    sizeof(MAIL_ADDR_MAIL_DAEMON)) == 0) {
		canon_addr_internal(cleanup_temp1, MAIL_ADDR_MAIL_DAEMON);
		if (strcasecmp(STR(clean_addr), STR(cleanup_temp1)) == 0)
		    vstring_strcpy(clean_addr, "");
	    }
	    if (cleanup_send_canon_maps)
		cleanup_map11_internal(clean_addr, cleanup_send_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_comm_canon_maps)
		cleanup_map11_internal(clean_addr, cleanup_comm_canon_maps,
				cleanup_ext_prop_mask & EXT_PROP_CANONICAL);
	    if (cleanup_masq_domains)
		cleanup_masquerade_internal(clean_addr, cleanup_masq_domains);
	    CLEANUP_OUT_BUF(type, clean_addr);
	    if (cleanup_sender == 0)
		cleanup_sender = mystrdup(STR(clean_addr));
	} else if (type == REC_TYPE_RCPT) {
	    if (cleanup_sender == 0) {		/* protect showq */
		msg_warn("%s: envelope recipient precedes sender",
			 cleanup_queue_id);
		cleanup_errs |= CLEANUP_STAT_BAD;
		break;
	    }
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
	} else if (type == REC_TYPE_WARN) {
	    if ((warn_time = atol(STR(cleanup_inbuf))) < 0) {
		cleanup_errs |= CLEANUP_STAT_BAD;
		break;
	    }
	} else {
	    CLEANUP_OUT_BUF(type, cleanup_inbuf);
	}
    }

    /*
     * XXX Keep reading in case of trouble, so that the sender is ready to
     * receive our status report.
     */
    if (!CLEANUP_OUT_OK())
	if (type >= 0)
	    cleanup_skip();

    vstring_free(clean_addr);
}
