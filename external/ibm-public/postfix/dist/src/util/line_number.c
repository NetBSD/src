/*	$NetBSD: line_number.c,v 1.1.1.1.8.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	line_number 3
/* SUMMARY
/*	line number utilities
/* SYNOPSIS
/*	#include <line_number.h>
/*
/*	char	*format_line_number(result, first, last)
/*	VSTRING	*buffer;
/*	ssize_t	first;
/*	ssize_t	lastl
/* DESCRIPTION
/*	format_line_number() formats a line number or number range.
/*	The output is <first-number>-<last-number> when the numbers
/*	differ, <first-number> when the numbers are identical.
/* .IP result
/*	Result buffer, or null-pointer. In the latter case the
/*	result is stored in a static buffer that is overwritten
/*	with subsequent calls. The function result value is a
/*	pointer into the result buffer.
/* .IP first
/*	First line number.
/* .IP last
/*	Last line number.
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

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <vstring.h>
#include <line_number.h>

/* format_line_number - pretty-print line number or number range */

char   *format_line_number(VSTRING *result, ssize_t first, ssize_t last)
{
    static VSTRING *buf;

    /*
     * Your buffer or mine?
     */
    if (result == 0) {
	if (buf == 0)
	    buf = vstring_alloc(10);
	result = buf;
    }

    /*
     * Print a range only when the numbers differ.
     */
    vstring_sprintf(result, first == last ? "%ld" : "%ld-%ld",
		    (long) first, (long) last);

    return (vstring_str(result));
}
