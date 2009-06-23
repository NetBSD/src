/*	$NetBSD: neuter.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	neuter 3
/* SUMMARY
/*	neutralize characters before they can explode
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*neuter(buffer, bad, replacement)
/*	char	*buffer;
/*	const char *bad;
/*	int	replacement;
/* DESCRIPTION
/*	neuter() replaces bad characters in its input
/*	by the given replacement.
/*
/*	Arguments:
/* .IP buffer
/*	The null-terminated input string.
/* .IP bad
/*	The null-terminated bad character string.
/* .IP replacement
/*	Replacement value for characters in \fIbuffer\fR that do not
/*	pass the  bad character test.
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
#include <string.h>

/* Utility library. */

#include <stringops.h>

/* neuter - neutralize bad characters */

char   *neuter(char *string, const char *bad, int replacement)
{
    char   *cp;
    int     ch;

    for (cp = string; (ch = *(unsigned char *) cp) != 0; cp++)
	if (strchr(bad, ch) != 0)
	    *cp = replacement;
    return (string);
}
