/*++
/* NAME
/*	is_header 3
/* SUMMARY
/*	message header classification
/* SYNOPSIS
/*	#include <is_header.h>
/*
/*	int	is_header(string)
/*	const char *string;
/* DESCRIPTION
/*	is_header() examines the given string and returns non-zero (true)
/*	when it begins with a mail header name + optional space + colon.
/*	The result is the length of the mail header name.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
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
#include <ctype.h>

/* Global library. */

#include "is_header.h"

/* is_header - determine if this can be a header line */

int     is_header(const char *str)
{
    const unsigned char *cp;
    int     state;
    int     c;
    int     len;

#define INIT		0
#define IN_CHAR		1
#define IN_CHAR_SPACE	2
#define CU_CHAR_PTR(x)	((const unsigned char *) (x))

    /*
     * XXX RFC 2822 Section 4.5, Obsolete header fields: whitespace may
     * appear between header label and ":" (see: RFC 822, Section 3.4.2.).
     */
    for (len = 0, state = INIT, cp = CU_CHAR_PTR(str); (c = *cp) != 0; cp++) {
	switch (c) {
	default:
	    if (!ISASCII(c) || ISCNTRL(c))
		return (0);
	    if (state == INIT)
		state = IN_CHAR;
	    if (state == IN_CHAR) {
		len++;
		continue;
	    }
	    return (0);
	case ' ':
	case '\t':
	    if (state == IN_CHAR)
		state = IN_CHAR_SPACE;
	    if (state == IN_CHAR_SPACE)
		continue;
	    return (0);
	case ':':
	    return ((state == IN_CHAR || state == IN_CHAR_SPACE) ? len : 0);
	}
    }
    return (0);
}
