/*	$NetBSD: alldig.c,v 1.1.1.2 2022/10/08 16:09:11 christos Exp $	*/

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
/*
/*	int	allalnum(string)
/*	const char *string;
/* DESCRIPTION
/*	alldig() determines if its argument is an all-numerical string.
/*
/*	allalnum() determines if its argument is an all-alphanumerical
/*	string.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
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

/* allalnum - return true if string is all alphanum */

int allalnum(const char *string)
{
    const char *cp;

    if (*string == 0)
        return (0);
    for (cp = string; *cp != 0; cp++)
        if (!ISALNUM(*cp))
            return (0);
    return (1);
}
