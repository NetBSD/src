/*	$NetBSD: allprint.c,v 1.1.1.1.2.2 2009/09/15 06:03:52 snj Exp $	*/

/*++
/* NAME
/*	allprint 3
/* SUMMARY
/*	predicate if string is all printable
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	allprint(buffer)
/*	const char *buffer;
/* DESCRIPTION
/*	allprint() determines if its argument is an all-printable string.
/*
/*	Arguments:
/* .IP buffer
/*	The null-terminated input string.
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

#include "stringops.h"

/* allprint - return true if string is all printable */

int     allprint(const char *string)
{
    const char *cp;
    int     ch;

    if (*string == 0)
	return (0);
    for (cp = string; (ch = *(unsigned char *) cp) != 0; cp++)
	if (!ISASCII(ch) || !ISPRINT(ch))
	    return (0);
    return (1);
}
