/*++
/* NAME
/*	cleanup_state 3
/* SUMMARY
/*	per-message state variables
/* SYNOPSIS
/*	#include "cleanup.h"
/*
/*	void	cleanup_state_alloc(void)
/*
/*	void	cleanup_state_free(void)
/* DESCRIPTION
/*	This module maintains about two dozen global (ugh) state variables
/*	that are used by many routines in the course of processing one
/*	message. Using globals seems to make more sense than passing around
/*	large parameter lists, or passing around a structure with a large
/*	number of components. We can get away with this because we need only
/*	one instance at a time.
/*
/*	cleanup_state_alloc() initializes the per-message state variables.
/*
/*	cleanup_state_free() cleans up.
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

#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>

/* Global library. */

#include <been_here.h>
#include <mail_params.h>

/* Application-specific. */

#include "cleanup.h"

 /*
  * These variables are accessed by many functions, and there is only one
  * instance of each. Rather than passing around lots and lots of parameters,
  * or passing them around in a structure, we just make the variables global,
  * because that is what they are.
  */
VSTRING *cleanup_inbuf;			/* read buffer */
VSTRING *cleanup_temp1;			/* scratch buffer, local use only */
VSTRING *cleanup_temp2;			/* scratch buffer, local use only */
VSTREAM *cleanup_src;			/* current input stream */
VSTREAM *cleanup_dst;			/* current output stream */
MAIL_STREAM *cleanup_handle;		/* mail stream handle */
char   *cleanup_queue_id;		/* queue file basename */
time_t  cleanup_time;			/* posting time */
char   *cleanup_fullname;		/* envelope sender full name */
char   *cleanup_sender;			/* envelope sender address */
char   *cleanup_from;			/* From: address */
char   *cleanup_resent_from;		/* Resent-From: address */
char   *cleanup_recip;			/* envelope recipient address */
char   *cleanup_return_receipt;		/* return-receipt address */
char   *cleanup_errors_to;		/* errors-to address */
int     cleanup_flags;			/* processing options */
int     cleanup_errs;			/* any badness experienced */
int     cleanup_err_mask;		/* allowed badness */
VSTRING *cleanup_header_buf;		/* multi-record header */
int     cleanup_headers_seen;		/* which headers were seen */
int     cleanup_hop_count;		/* count of received: headers */
ARGV   *cleanup_recipients;		/* recipients from regular headers */
ARGV   *cleanup_resent_recip;		/* recipients from resent headers */
char   *cleanup_resent;			/* any resent- header seen */
BH_TABLE *cleanup_dups;			/* recipient dup filter */

/* cleanup_state_alloc - initialize global state */

void    cleanup_state_alloc(void)
{
    cleanup_hop_count = 0;
    cleanup_headers_seen = 0;
    cleanup_time = 0;
    cleanup_errs = 0;
    cleanup_from = 0;
    cleanup_fullname = 0;
    cleanup_header_buf = vstring_alloc(100);
    cleanup_queue_id = 0;
    cleanup_recip = 0;
    cleanup_return_receipt = 0;
    cleanup_errors_to = 0;
    cleanup_recipients = argv_alloc(2);
    cleanup_resent = "";
    cleanup_resent_from = 0;
    cleanup_resent_recip = argv_alloc(2);
    cleanup_sender = 0;
    cleanup_temp1 = vstring_alloc(10);
    cleanup_temp2 = vstring_alloc(10);
    cleanup_inbuf = vstring_alloc(100);
    cleanup_dups = been_here_init(var_dup_filter_limit, BH_FLAG_FOLD);
}

/* cleanup_state_free - destroy global state */

void    cleanup_state_free(void)
{
    if (cleanup_fullname)
	myfree(cleanup_fullname);
    if (cleanup_sender)
	myfree(cleanup_sender);
    if (cleanup_from)
	myfree(cleanup_from);
    if (cleanup_resent_from)
	myfree(cleanup_resent_from);
    if (cleanup_recip)
	myfree(cleanup_recip);
    if (cleanup_return_receipt)
	myfree(cleanup_return_receipt);
    if (cleanup_errors_to)
	myfree(cleanup_errors_to);
    vstring_free(cleanup_header_buf);
    argv_free(cleanup_recipients);
    argv_free(cleanup_resent_recip);
    vstring_free(cleanup_temp1);
    vstring_free(cleanup_temp2);
    vstring_free(cleanup_inbuf);
    if (cleanup_queue_id)
	myfree(cleanup_queue_id);
    been_here_free(cleanup_dups);
}
