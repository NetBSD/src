/*++
/* NAME
/*	cleanup_strerror 3
/* SUMMARY
/*	cleanup status code to string
/* SYNOPSIS
/*	#include <cleanup_user.h>
/*
/*	const char *cleanup_strerror(code)
/*	int	code;
/* DESCRIPTION
/*	cleanup_strerror() maps a status code returned by the \fIcleanup\fR
/*	service to printable string.
/*	The result is for read purposes only. Unknown status codes share
/*	a common result buffer.
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

#include <vstring.h>

/* Global library. */

#include "cleanup_user.h"

 /*
  * Mapping from status code to printable string. One message may suffer from
  * multiple errors, to it is important to list the most severe errors first,
  * or to list the cause (header overflow) before the effect (no recipients),
  * because cleanup_strerror() can report only one error.
  */
struct cleanup_stat_map {
    unsigned status;
    const char *text;
};

static struct cleanup_stat_map cleanup_stat_map[] = {
    CLEANUP_STAT_BAD, "Internal protocol error",
    CLEANUP_STAT_RCPT, "No recipients specified",
    CLEANUP_STAT_HOPS, "Too many hops",
    CLEANUP_STAT_SIZE, "Message file too big",
    CLEANUP_STAT_CONT, "Message content rejected",
    CLEANUP_STAT_WRITE, "Error writing message file",
};

/* cleanup_strerror - map status code to printable string */

const char *cleanup_strerror(unsigned status)
{
    static VSTRING *unknown;
    unsigned i;

    if (status == 0)
	return ("Success");

    for (i = 0; i < sizeof(cleanup_stat_map) / sizeof(cleanup_stat_map[0]); i++)
	if (cleanup_stat_map[i].status & status)
	    return (cleanup_stat_map[i].text);

    if (unknown == 0)
	unknown = vstring_alloc(20);
    vstring_sprintf(unknown, "Unknown status %u", status);
    return (vstring_str(unknown));
}
