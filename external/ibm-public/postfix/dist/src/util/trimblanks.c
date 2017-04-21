/*	$NetBSD: trimblanks.c,v 1.1.1.1.36.1 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	trimblanks 3
/* SUMMARY
/*	skip leading whitespace
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*trimblanks(string, len)
/*	char	*string;
/*	ssize_t	len;
/* DESCRIPTION
/*	trimblanks() returns a pointer to the beginning of the trailing
/*	whitespace in \fIstring\fR, or a pointer to the string terminator
/*	when the string contains no trailing whitespace.
/*	The \fIlen\fR argument is either zero or the string length.
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

char   *trimblanks(char *string, ssize_t len)
{
    char   *curr;

    if (len) {
	curr = string + len;
    } else {
	for (curr = string; *curr != 0; curr++)
	     /* void */ ;
    }
    while (curr > string && ISSPACE(curr[-1]))
	curr -= 1;
    return (curr);
}
