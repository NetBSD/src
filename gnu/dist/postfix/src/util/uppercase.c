/*++
/* NAME
/*	uppercase 3
/* SUMMARY
/*	map lowercase characters to uppercase
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*uppercase(buf)
/*	char	*buf;
/* DESCRIPTION
/*	uppercase() replaces lowercase characters in its null-terminated
/*	input by their uppercase equivalent.
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

char   *uppercase(char *string)
{
    char   *cp;
    int     ch;

    for (cp = string; (ch = *cp) != 0; cp++)
	if (ISLOWER(ch))
	    *cp = TOUPPER(ch);
    return (string);
}
