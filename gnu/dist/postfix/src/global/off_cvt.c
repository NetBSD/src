/*++
/* NAME
/*	off_cvt 3
/* SUMMARY
/*	off_t conversions
/* SYNOPSIS
/*	#include <off_cvt.h>
/*
/*	off_t	off_cvt_string(string)
/*	const char *string;
/*
/*	VSTRING	*off_cvt_number(result, offset)
/*	VSTRING	*result;
/*	off_t	offset;
/* DESCRIPTION
/*	This module provides conversions between \fIoff_t\fR and string.
/*
/*	off_cvt_string() converts a string, containing a non-negative
/*	offset, to numerical form. The result is -1 in case of problems.
/*
/*	off_cvt_number() converts a non-negative offset to string form.
/*
/*	Arguments:
/* .IP string
/*	String with non-negative number to be converted to off_t.
/* .IP result
/*	Buffer for storage of the result of conversion to string.
/* .IP offset
/*	Non-negative off_t value to be converted to string.
/* DIAGNOSTICS
/*	Panic: negative offset
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
#include <vstring.h>

/* Global library. */

#include "off_cvt.h"

/* Application-specific. */

#define STR	vstring_str
#define END	vstring_end
#define SWAP(type, a, b) { type temp; temp = a; a = b; b = temp; }

/* off_cvt_string - string to number */

off_t   off_cvt_string(const char *str)
{
    int     ch;
    off_t   result;
    off_t   res2;
    off_t   res4;
    off_t   res8;
    off_t   res10;

    /*
     * Multiplication by numbers > 2 can overflow without producing a smaller
     * result mod 2^N (where N is the number of bits in the result type).
     * (Victor Duchovni, Morgan Stanley).
     */
    for (result = 0; (ch = *(unsigned char *) str) != 0; str++) {
	if (!ISDIGIT(ch))
	    return (-1);
	if ((res2 = result + result) < result)
	    return (-1);
	if ((res4 = res2 + res2) < res2)
	    return (-1);
	if ((res8 = res4 + res4) < res4)
	    return (-1);
	if ((res10 = res8 + res2) < res8)
	    return (-1);
	if ((result = res10 + ch - '0') < res10)
	    return (-1);
    }
    return (result);
}

/* off_cvt_number - number to string */

VSTRING *off_cvt_number(VSTRING *buf, off_t offset)
{
    static char digs[] = "0123456789";
    char   *start;
    char   *last;
    int     i;

    /*
     * Sanity checks
     */
    if (offset < 0)
	msg_panic("off_cvt_number: negative offset -%s",
		  STR(off_cvt_number(buf, -offset)));

    /*
     * First accumulate the result, backwards.
     */
    VSTRING_RESET(buf);
    while (offset != 0) {
	VSTRING_ADDCH(buf, digs[offset % 10]);
	offset /= 10;
    }
    VSTRING_TERMINATE(buf);

    /*
     * Then, reverse the result.
     */
    start = STR(buf);
    last = END(buf) - 1;
    for (i = 0; i < VSTRING_LEN(buf) / 2; i++)
	SWAP(int, start[i], last[-i]);
    return (buf);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read a number from stdin, convert to
  * off_t, back to string, and print the result.
  */
#include <vstream.h>
#include <vstring_vstream.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *buf = vstring_alloc(100);
    off_t   offset;

    while (vstring_fgets_nonl(buf, VSTREAM_IN)) {
	if ((offset = off_cvt_string(STR(buf))) < 0) {
	    msg_warn("bad input %s", STR(buf));
	} else {
	    vstream_printf("%s\n", STR(off_cvt_number(buf, offset)));
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(buf);
}

#endif
