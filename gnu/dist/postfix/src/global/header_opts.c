/*++
/* NAME
/*	header_opts 3
/* SUMMARY
/*	message header classification
/* SYNOPSIS
/*	#include <header_opts.h>
/*
/*	HEADER_OPTS *header_opts_find(string)
/*	const char *string;
/* DESCRIPTION
/*	header_opts_find() takes a message header line and looks up control
/*	information for the corresponding header type.
/* DIAGNOSTICS
/*	Panic: input is not a valid header line. The result is a pointer
/*	to HEADER_OPTS in case of success, a null pointer when the header
/*	label was not recognized.
/* SEE ALSO
/*	header_opts(3h) the gory details
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
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <htable.h>
#include <vstring.h>
#include <stringops.h>

/* Global library. */

#include "header_opts.h"

 /*
  * Header names are given in the preferred capitalization. The lookups are
  * case-insensitive.
  */
static HEADER_OPTS header_opts[] = {
    "Apparently-To", HDR_APPARENTLY_TO, HDR_OPT_RECIP,
    "Bcc", HDR_BCC, HDR_OPT_DROP | HDR_OPT_XRECIP,
    "Cc", HDR_CC, HDR_OPT_XRECIP,
    "Content-Description", HDR_CONTENT_DESCRIPTION, HDR_OPT_MIME,
    "Content-Disposition", HDR_CONTENT_DISPOSITION, HDR_OPT_MIME,
    "Content-ID", HDR_CONTENT_ID, HDR_OPT_MIME,
    "Content-Length", HDR_CONTENT_LENGTH, HDR_OPT_DROP,
    "Content-Transfer-Encoding", HDR_CONTENT_TRANSFER_ENCODING, HDR_OPT_MIME,
    "Content-Type", HDR_CONTENT_TYPE, HDR_OPT_MIME,
    "Delivered-To", HDR_DELIVERED_TO, 0,
    "Disposition-Notification-To", HDR_DISP_NOTIFICATION, HDR_OPT_SENDER,
    "Date", HDR_DATE, 0,
    "Errors-To", HDR_ERRORS_TO, HDR_OPT_SENDER,
    "From", HDR_FROM, HDR_OPT_SENDER,
    "Mail-Followup-To", HDR_MAIL_FOLLOWUP_TO, HDR_OPT_SENDER,
    "Message-Id", HDR_MESSAGE_ID, 0,
    "MIME-Version", HDR_MIME_VERSION, HDR_OPT_MIME,
    "Received", HDR_RECEIVED, 0,
    "Reply-To", HDR_REPLY_TO, HDR_OPT_SENDER,
    "Resent-Bcc", HDR_RESENT_BCC, HDR_OPT_DROP | HDR_OPT_XRECIP | HDR_OPT_RR,
    "Resent-Cc", HDR_RESENT_CC, HDR_OPT_XRECIP | HDR_OPT_RR,
    "Resent-Date", HDR_RESENT_DATE, HDR_OPT_RR,
    "Resent-From", HDR_RESENT_FROM, HDR_OPT_SENDER | HDR_OPT_RR,
    "Resent-Message-Id", HDR_RESENT_MESSAGE_ID, HDR_OPT_RR,
    "Resent-Reply-To", HDR_RESENT_REPLY_TO, HDR_OPT_RECIP | HDR_OPT_RR,
    "Resent-Sender", HDR_RESENT_SENDER, HDR_OPT_SENDER | HDR_OPT_RR,
    "Resent-To", HDR_RESENT_TO, HDR_OPT_XRECIP | HDR_OPT_RR,
    "Return-Path", HDR_RETURN_PATH, HDR_OPT_DROP | HDR_OPT_SENDER,
    "Return-Receipt-To", HDR_RETURN_RECEIPT_TO, HDR_OPT_SENDER,
    "Sender", HDR_SENDER, HDR_OPT_SENDER,
    "To", HDR_TO, HDR_OPT_XRECIP,
};

#define HEADER_OPTS_SIZE (sizeof(header_opts) / sizeof(header_opts[0]))

static HTABLE *header_hash;		/* quick lookup */
static VSTRING *header_key;

/* header_opts_init - initialize */

static void header_opts_init(void)
{
    HEADER_OPTS *hp;
    const char *cp;

    /*
     * Build a hash table for quick lookup, and allocate memory for
     * lower-casing the lookup key.
     */
    header_key = vstring_alloc(10);
    header_hash = htable_create(HEADER_OPTS_SIZE);
    for (hp = header_opts; hp < header_opts + HEADER_OPTS_SIZE; hp++) {
	VSTRING_RESET(header_key);
	for (cp = hp->name; *cp; cp++)
	    VSTRING_ADDCH(header_key, TOLOWER(*cp));
	VSTRING_TERMINATE(header_key);
	htable_enter(header_hash, vstring_str(header_key), (char *) hp);
    }
}

/* header_opts_find - look up header options */

HEADER_OPTS *header_opts_find(const char *string)
{
    const char *cp;

    if (header_hash == 0)
	header_opts_init();

    /*
     * Look up the lower-cased version of the header name.
     */
    VSTRING_RESET(header_key);
    for (cp = string; *cp != ':'; cp++) {
	if (*cp == 0)
	    msg_panic("header_opts_find: no colon in header: %.30s", string);
	VSTRING_ADDCH(header_key, TOLOWER(*cp));
    }
    vstring_truncate(header_key,
		     trimblanks(vstring_str(header_key), cp - string)
		     - vstring_str(header_key));
    VSTRING_TERMINATE(header_key);
    return ((HEADER_OPTS *) htable_find(header_hash, vstring_str(header_key)));
}
