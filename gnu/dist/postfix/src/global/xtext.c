/*++
/* NAME
/*	xtext 3
/* SUMMARY
/*	translate characters according to RFC 1894
/* SYNOPSIS
/*	#include <xtext.h>
/*
/*	VSTRING	*xtext(result, original)
/*	VSTRING	*result;
/*	const char *original;
/* DESCRIPTION
/*	xtext() takes a null-terminated string, and produces a translation
/*	according to RFC 1894 (DSN).
/* BUGS
/*	Cannot replace null characters.
/*
/*	Does not insert CR LF SPACE to limit output line length.
/* SEE ALSO
/*	RFC 1894, Delivery Status Notifications
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

#include "sys_defs.h"
#include <vstream.h>

/* Utility library. */

#include <vstring.h>

/* Global library. */

#include <xtext.h>

/* xtext - translate text according to RFC 1894 */

VSTRING *xtext(VSTRING *result, const char *original)
{
    const char *cp;
    int     ch;

    /*
     * Preliminary implementation. ASCII specific!!
     */
    VSTRING_RESET(result);
    for (cp = original; (ch = *(unsigned char *) cp) != 0; cp++) {
	if (ch == '+' || ch == '\\' || ch == '(' || ch < 33 || ch > 126)
	    vstring_sprintf_append(result, "+%02X", ch);
	else
	    VSTRING_ADDCH(result, ch);
    }
    VSTRING_TERMINATE(result);

    return (result);
}

#ifdef TEST

#define STR(x)	vstring_str(x)

#include <vstream.h>

int     main(int unused_argc, char **unused_argv)
{
    VSTRING *ibuf = vstring_alloc(100);
    VSTRING *obuf = vstring_alloc(100);

    while (vstring_fgets(ibuf, VSTREAM_IN)) {
	vstream_fputs(STR(xtext(obuf, STR(ibuf))));
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(ibuf);
    vstring_free(obuf);
    return (0);
}

#endif
