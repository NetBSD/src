/*++
/* NAME
/*	printable 3
/* SUMMARY
/*	mask non-printable characters
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*printable(buffer, replacement)
/*	char	*buffer;
/*	int	replacement;
/* DESCRIPTION
/*	printable() replaces non-printable characters in its input
/*	by the given replacement.
/*
/*	Arguments:
/* .IP buffer
/*	The null-terminated input string.
/* .IP replacement
/*	Replacement value for characters in \fIbuffer\fR that do not
/*	pass the isprint(3) test.
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

char   *printable(char *string, int replacement)
{
    char   *cp;
    int     ch;

    for (cp = string; (ch = *(unsigned char *) cp) != 0; cp++)
	if (!ISPRINT(ch))
	    *cp = replacement;
    return (string);
}
