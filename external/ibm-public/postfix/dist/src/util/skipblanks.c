/*	$NetBSD: skipblanks.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	skipblanks 3
/* SUMMARY
/*	skip leading whitespace
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*skipblanks(string)
/*	const char *string;
/* DESCRIPTION
/*	skipblanks() returns a pointer to the first non-whitespace
/*	character in the specified string, or a pointer to the string
/*	terminator when the string contains all white-space characters.
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

char   *skipblanks(const char *string)
{
    const char *cp;

    for (cp = string; *cp != 0; cp++)
	if (!ISSPACE(*cp))
	    break;
    return ((char *) cp);
}
