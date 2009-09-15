/*	$NetBSD: alldig.c,v 1.1.1.1.2.2 2009/09/15 06:03:52 snj Exp $	*/

/*++
/* NAME
/*	alldig 3
/* SUMMARY
/*	predicate if string is all numerical
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	int	alldig(string)
/*	const char *string;
/* DESCRIPTION
/*	alldig() determines if its argument is an all-numerical string.
/* SEE ALSO
/*	An alldig() routine appears in Brian W. Kernighan, P.J. Plauger:
/*	"Software Tools", Addison-Wesley 1976.
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

#include <stringops.h>

/* alldig - return true if string is all digits */

int     alldig(const char *string)
{
    const char *cp;

    if (*string == 0)
	return (0);
    for (cp = string; *cp != 0; cp++)
	if (!ISDIGIT(*cp))
	    return (0);
    return (1);
}
