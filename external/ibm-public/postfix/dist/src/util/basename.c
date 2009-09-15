/*	$NetBSD: basename.c,v 1.1.1.1.2.2 2009/09/15 06:03:53 snj Exp $	*/

/*++
/* NAME
/*	basename 3
/* SUMMARY
/*	extract file basename
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*basename(path)
/*	const char *path;
/* DESCRIPTION
/*	The \fBbasename\fR routine skips over the last '/' in
/*	\fIpath\fR and returns a pointer to the result.
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

#ifndef HAVE_BASENAME

/* Utility library. */

#include "stringops.h"

/* basename - skip directory prefix */

char   *basename(const char *path)
{
    char   *result;

    if ((result = strrchr(path, '/')) == 0)
	result = (char *) path;
    else
	result += 1;
    return (result);
}

#endif
