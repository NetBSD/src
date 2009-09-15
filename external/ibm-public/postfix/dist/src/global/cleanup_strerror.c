/*	$NetBSD: cleanup_strerror.c,v 1.1.1.1.2.2 2009/09/15 06:02:38 snj Exp $	*/

/*++
/* NAME
/*	cleanup_strerror 3
/* SUMMARY
/*	cleanup status code to string
/* SYNOPSIS
/*	#include <cleanup_user.h>
/*
/*	typedef struct {
/* .in +4
/*	    const unsigned status;	/* cleanup status */
/*	    const int smtp;	/* RFC 821 */
/*	    const char *dsn;	/* RFC 3463 */
/*	    const char *text;	/* free text */
/* .in -4
/*	} CLEANUP_STAT_DETAIL;
/*
/*	const char *cleanup_strerror(code)
/*	int	code;
/*
/*	const CLEANUP_STAT_DETAIL *cleanup_stat_detail(code)
/*	int	code;
/* DESCRIPTION
/*	cleanup_strerror() maps a status code returned by the \fIcleanup\fR
/*	service to printable string.
/*	The result is for read purposes only.
/*
/*	cleanup_stat_detail() returns a pointer to structure with
/*	assorted information.
/* DIAGNOSTICS:
/*	Panic: unknown status.
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
#include <msg.h>

/* Global library. */

#include <cleanup_user.h>

 /*
  * Mapping from status code to printable string. One message may suffer from
  * multiple errors, to it is important to list the most severe errors first,
  * because cleanup_strerror() can report only one error.
  */
static const CLEANUP_STAT_DETAIL cleanup_stat_map[] = {
    CLEANUP_STAT_DEFER, 451, "4.7.1", "service unavailable",
    CLEANUP_STAT_PROXY, 451, "4.3.0", "queue file write error",
    CLEANUP_STAT_BAD, 451, "4.3.0", "internal protocol error",
    CLEANUP_STAT_RCPT, 550, "5.1.0", "no recipients specified",
    CLEANUP_STAT_HOPS, 554, "5.4.0", "too many hops",
    CLEANUP_STAT_SIZE, 552, "5.3.4", "message file too big",
    CLEANUP_STAT_CONT, 550, "5.7.1", "message content rejected",
    CLEANUP_STAT_WRITE, 451, "4.3.0", "queue file write error",
};

static CLEANUP_STAT_DETAIL cleanup_stat_success = {
    CLEANUP_STAT_OK, 250, "2.0.0", "Success",
};

/* cleanup_strerror - map status code to printable string */

const char *cleanup_strerror(unsigned status)
{
    unsigned i;

    if (status == CLEANUP_STAT_OK)
	return ("Success");

    for (i = 0; i < sizeof(cleanup_stat_map) / sizeof(cleanup_stat_map[0]); i++)
	if (cleanup_stat_map[i].status & status)
	    return (cleanup_stat_map[i].text);

    msg_panic("cleanup_strerror: unknown status %u", status);
}

/* cleanup_stat_detail - map status code to table entry with assorted data */

const CLEANUP_STAT_DETAIL *cleanup_stat_detail(unsigned status)
{
    unsigned i;

    if (status == CLEANUP_STAT_OK)
	return (&cleanup_stat_success);

    for (i = 0; i < sizeof(cleanup_stat_map) / sizeof(cleanup_stat_map[0]); i++)
	if (cleanup_stat_map[i].status & status)
	    return (cleanup_stat_map + i);

    msg_panic("cleanup_stat_detail: unknown status %u", status);
}
