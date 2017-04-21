/*	$NetBSD: printable.c,v 1.1.1.1.36.1 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	printable 3
/* SUMMARY
/*	mask non-printable characters
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	util_utf8_enable;
/*
/*	char	*printable(buffer, replacement)
/*	char	*buffer;
/*	int	replacement;
/* DESCRIPTION
/*	printable() replaces non-printable characters
/*	in its input with the given replacement.
/*
/*	util_utf8_enable controls whether UTF8 is considered printable.
/*	With util_utf8_enable equal to zero, non-ASCII text is replaced.
/*
/*	Arguments:
/* .IP buffer
/*	The null-terminated input string.
/* .IP replacement
/*	Replacement value for characters in \fIbuffer\fR that do not
/*	pass the ASCII isprint(3) test or that are not valid UTF8.
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

/* Utility library. */

#include "stringops.h"

int util_utf8_enable = 0;

char   *printable(char *string, int replacement)
{
    unsigned char *cp;
    int     ch;

    /*
     * XXX Replace invalid UTF8 sequences (too short, over-long encodings,
     * out-of-range code points, etc). See valid_utf8_string.c.
     */
    cp = (unsigned char *) string;
    while ((ch = *cp) != 0) {
	if (ISASCII(ch) && ISPRINT(ch)) {
	    /* ok */
	} else if (util_utf8_enable && ch >= 194 && ch <= 254
		   && cp[1] >= 128 && cp[1] < 192) {
	    /* UTF8; skip the rest of the bytes in the character. */
	    while (cp[1] >= 128 && cp[1] < 192)
		cp++;
	} else {
	    /* Not ASCII and not UTF8. */
	    *cp = replacement;
	}
	cp++;
    }
    return (string);
}
