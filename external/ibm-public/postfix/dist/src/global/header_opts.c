/*	$NetBSD: header_opts.c,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

/*++
/* NAME
/*	header_opts 3
/* SUMMARY
/*	message header classification
/* SYNOPSIS
/*	#include <header_opts.h>
/*
/*	const HEADER_OPTS *header_opts_find(string)
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
#include <argv.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_params.h>
#include <header_opts.h>

 /*
  * Header names are given in the preferred capitalization. The lookups are
  * case-insensitive.
  * 
  * XXX Removing Return-Path: headers should probably be done only with mail
  * that enters via a non-SMTP channel. Changing this now could break other
  * software. See also comments in bounce_notify_util.c.
  */
static HEADER_OPTS header_opts[] = {
    "Apparently-To", HDR_APPARENTLY_TO, HDR_OPT_RECIP,
    "Bcc", HDR_BCC, HDR_OPT_XRECIP,
    "Cc", HDR_CC, HDR_OPT_XRECIP,
    "Content-Description", HDR_CONTENT_DESCRIPTION, HDR_OPT_MIME,
    "Content-Disposition", HDR_CONTENT_DISPOSITION, HDR_OPT_MIME,
    "Content-ID", HDR_CONTENT_ID, HDR_OPT_MIME,
    "Content-Length", HDR_CONTENT_LENGTH, 0,
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
    "Resent-Bcc", HDR_RESENT_BCC, HDR_OPT_XRECIP | HDR_OPT_RR,
    "Resent-Cc", HDR_RESENT_CC, HDR_OPT_XRECIP | HDR_OPT_RR,
    "Resent-Date", HDR_RESENT_DATE, HDR_OPT_RR,
    "Resent-From", HDR_RESENT_FROM, HDR_OPT_SENDER | HDR_OPT_RR,
    "Resent-Message-Id", HDR_RESENT_MESSAGE_ID, HDR_OPT_RR,
    "Resent-Reply-To", HDR_RESENT_REPLY_TO, HDR_OPT_RECIP | HDR_OPT_RR,
    "Resent-Sender", HDR_RESENT_SENDER, HDR_OPT_SENDER | HDR_OPT_RR,
    "Resent-To", HDR_RESENT_TO, HDR_OPT_XRECIP | HDR_OPT_RR,
    "Return-Path", HDR_RETURN_PATH, HDR_OPT_SENDER,
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
    const HEADER_OPTS *hp;
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
	htable_enter(header_hash, vstring_str(header_key), (void *) hp);
    }
}

/* header_drop_init - initialize "header drop" flags */

static void header_drop_init(void)
{
    ARGV   *hdr_drop_list;
    char  **cpp;
    HTABLE_INFO *ht;
    HEADER_OPTS *hp;

    /*
     * Having one main.cf parameter for the "drop" header flag does not
     * generalize to the "sender", "extract", etc., flags. Flags would need
     * to be grouped by header name, but that would be unwieldy, too:
     * 
     * message_header_flags = { apparently-to = recipient }, { bcc = recipient,
     * extract, drop }, { from = sender }, ...
     * 
     * Thus, it is unlikely that all header flags will become configurable.
     */
    hdr_drop_list = argv_split(var_drop_hdrs, CHARS_COMMA_SP);
    for (cpp = hdr_drop_list->argv; *cpp; cpp++) {
	lowercase(*cpp);
	if ((ht = htable_locate(header_hash, *cpp)) == 0) {
	    hp = (HEADER_OPTS *) mymalloc(sizeof(*hp));
	    hp->type = HDR_OTHER;
	    hp->flags = HDR_OPT_DROP;
	    ht = htable_enter(header_hash, *cpp, (void *) hp);
	    hp->name = ht->key;
	} else
	    hp = (HEADER_OPTS *) ht->value;
	hp->flags |= HDR_OPT_DROP;
    }
    argv_free(hdr_drop_list);
}

/* header_opts_find - look up header options */

const HEADER_OPTS *header_opts_find(const char *string)
{
    const char *cp;

    if (header_hash == 0) {
	header_opts_init();
	header_drop_init();
    }

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
    return ((const HEADER_OPTS *) htable_find(header_hash, vstring_str(header_key)));
}
