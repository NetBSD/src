/*	$NetBSD: concatenate.c,v 1.1.1.1.16.1 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	concatenate 3
/* SUMMARY
/*	concatenate strings
/* SYNOPSIS
/*	#include <stringops.h>
/*
/*	char	*concatenate(str, ...)
/*	const char *str;
/* DESCRIPTION
/*	The \fBconcatenate\fR routine concatenates a null-terminated
/*	list of pointers to null-terminated character strings.
/*	The result is dynamically allocated and should be passed to myfree()
/*	when no longer needed.
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
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "stringops.h"
#include "compat_va_copy.h"

/* concatenate - concatenate null-terminated list of strings */

char   *concatenate(const char *arg0,...)
{
    char   *result;
    va_list ap;
    va_list ap2;
    ssize_t len;
    char   *arg;

    /*
     * Initialize argument lists.
     */
    va_start(ap, arg0);
    VA_COPY(ap2, ap);

    /*
     * Compute the length of the resulting string.
     */
    len = strlen(arg0);
    while ((arg = va_arg(ap, char *)) != 0)
	len += strlen(arg);
    va_end(ap);

    /*
     * Build the resulting string. Don't care about wasting a CPU cycle.
     */
    result = mymalloc(len + 1);
    strcpy(result, arg0);
    while ((arg = va_arg(ap2, char *)) != 0)
	strcat(result, arg);
    va_end(ap2);
    return (result);
}
