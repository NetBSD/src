/*	$NetBSD: lowercase.c,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

/*++
/* NAME
/*	lowercase 3
/* SUMMARY
/*	map uppercase characters to lowercase
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*lowercase(buf)
/*	char	*buf;
/* DESCRIPTION
/*	lowercase() replaces uppercase characters in its null-terminated
/*	input by their lowercase equivalent.
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

char   *lowercase(char *string)
{
    char   *cp;
    int     ch;

    for (cp = string; (ch = *cp) != 0; cp++)
	if (ISUPPER(ch))
	    *cp = TOLOWER(ch);
    return (string);
}
