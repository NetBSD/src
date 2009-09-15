/*	$NetBSD: cleanup_strflags.c,v 1.1.1.1.2.2 2009/09/15 06:02:38 snj Exp $	*/

/*++
/* NAME
/*	cleanup_strflags 3
/* SUMMARY
/*	cleanup flags code to string
/* SYNOPSIS
/*	#include <cleanup_user.h>
/*
/*	const char *cleanup_strflags(code)
/*	int	code;
/* DESCRIPTION
/*	cleanup_strflags() maps a CLEANUP_FLAGS code to printable string.
/*	The result is for read purposes only. The result is overwritten
/*	upon each call.
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

/* Global library. */

#include "cleanup_user.h"

 /*
  * Mapping from flags code to printable string.
  */
struct cleanup_flag_map {
    unsigned flag;
    const char *text;
};

static struct cleanup_flag_map cleanup_flag_map[] = {
    CLEANUP_FLAG_BOUNCE, "enable_bad_mail_bounce",
    CLEANUP_FLAG_FILTER, "enable_header_body_filter",
    CLEANUP_FLAG_HOLD, "hold_message",
    CLEANUP_FLAG_DISCARD, "discard_message",
    CLEANUP_FLAG_BCC_OK, "enable_automatic_bcc",
    CLEANUP_FLAG_MAP_OK, "enable_address_mapping",
    CLEANUP_FLAG_MILTER, "enable_milters",
    CLEANUP_FLAG_SMTP_REPLY, "enable_smtp_reply",
};

/* cleanup_strflags - map flags code to printable string */

const char *cleanup_strflags(unsigned flags)
{
    static VSTRING *result;
    unsigned i;

    if (flags == 0)
	return ("none");

    if (result == 0)
	result = vstring_alloc(20);
    else
	VSTRING_RESET(result);

    for (i = 0; i < sizeof(cleanup_flag_map) / sizeof(cleanup_flag_map[0]); i++) {
	if (cleanup_flag_map[i].flag & flags) {
	    vstring_sprintf_append(result, "%s ", cleanup_flag_map[i].text);
	    flags &= ~cleanup_flag_map[i].flag;
	}
    }

    if (flags != 0 || VSTRING_LEN(result) == 0)
	msg_panic("cleanup_strflags: unrecognized flag value(s) 0x%x", flags);

    vstring_truncate(result, VSTRING_LEN(result) - 1);
    VSTRING_TERMINATE(result);

    return (vstring_str(result));
}
