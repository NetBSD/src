/*++
/* NAME
/*	allspace 3
/* SUMMARY
/*	predicate if string is all space
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	allspace(buffer)
/*	const char *buffer;
/* DESCRIPTION
/*	allspace() determines if its argument is an all-space string.
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

/* allspace - return true if string is all space */

int     allspace(const char *string)
{
    const char *cp;
    int     ch;

    if (*string == 0)
	return (0);
    for (cp = string; (ch = *(unsigned char *) cp) != 0; cp++)
	if (!ISASCII(ch) || !ISSPACE(ch))
	    return (0);
    return (1);
}
