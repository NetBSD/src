/*	$NetBSD: balpar.c,v 1.2.4.2 2017/04/21 16:52:52 bouyer Exp $	*/

/*++
/* NAME
/*	balpar 3
/* SUMMARY
/*	determine length of string in parentheses
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	size_t	balpar(string, parens)
/*	const char *string;
/*	const char *parens;
/* DESCRIPTION
/*	balpar() determines the length of a string enclosed in 
/*	the specified parentheses, zero in case of error.
/* SEE ALSO
/*	A balpar() routine appears in Brian W. Kernighan, P.J. Plauger:
/*	"Software Tools", Addison-Wesley 1976. This function is different.
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

/* Utility library. */

#include <stringops.h>

/* balpar - return length of {text} */

size_t  balpar(const char *string, const char *parens)
{
    const char *cp;
    int     level;
    int     ch;

    if (*string != parens[0])
	return (0);
    for (level = 1, cp = string + 1; (ch = *cp) != 0; cp++) {
	if (ch == parens[1]) {
	    if (--level == 0)
		return (cp - string + 1);
	} else if (ch == parens[0]) {
	    level++;
	}
    }
    return (0);
}
