/*	$NetBSD: alldig.c,v 1.3.2.1 2026/05/11 17:14:01 martin Exp $	*/

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
/*
/*	int	allalnumus(string)
/*	const char *string;
/* DESCRIPTION
/*	alldig() determines if its argument is an all-numerical string.
/*
/*	allalnum() determines if its argument is an all-alphanumerical
/*	string.
/*
/*	allalnumus() determines if its argument is an all-(alphanumerical
/*	or underscore) string.
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
/*
/*	Wietse Venema
/*	porcupine.org
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

int     allalnum(const char *string)
{
    const char *cp;

    if (*string == 0)
	return (0);
    for (cp = string; *cp != 0; cp++)
	if (!ISALNUM(*cp))
	    return (0);
    return (1);
}

/* allalnumus - return true if string is all (alphanum or underscore) */

int     allalnumus(const char *string)
{
    const char *cp;
    int     ch;

    if (*string == 0)
	return (0);
    for (cp = string; (ch = *cp) != 0; cp++)
	if (!ISALNUM(ch) && ch != '_')
	    return (0);
    return (1);
}
