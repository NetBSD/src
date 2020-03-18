/*	$NetBSD: printable.c,v 1.3 2020/03/18 19:05:22 christos Exp $	*/

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
/*
/*	char	*printable_except(buffer, replacement, except)
/*	char	*buffer;
/*	int	replacement;
/*	const char *except;
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
/* .IP except
/*	Null-terminated sequence of non-replaced ASCII characters.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include "sys_defs.h"
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include "stringops.h"

int util_utf8_enable = 0;

/* printable -  binary compatibility */

#undef printable

char   *printable(char *, int);

char   *printable(char *string, int replacement)
{
    return (printable_except(string, replacement, (char *) 0));
}

/* printable_except -  pass through printable or other preserved characters */

char   *printable_except(char *string, int replacement, const char *except)
{
    unsigned char *cp;
    int     ch;

    /*
     * XXX Replace invalid UTF8 sequences (too short, over-long encodings,
     * out-of-range code points, etc). See valid_utf8_string.c.
     */
    cp = (unsigned char *) string;
    while ((ch = *cp) != 0) {
	if (ISASCII(ch) && (ISPRINT(ch) || (except && strchr(except, ch)))) {
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
