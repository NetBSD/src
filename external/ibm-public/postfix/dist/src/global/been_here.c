/*	$NetBSD: been_here.c,v 1.1.1.1.2.2 2009/09/15 06:02:37 snj Exp $	*/

/*++
/* NAME
/*	been_here 3
/* SUMMARY
/*	detect repeated occurrence of string
/* SYNOPSIS
/*	#include <been_here.h>
/*
/*	BH_TABLE *been_here_init(size, flags)
/*	int	size;
/*	int	flags;
/*
/*	int	been_here_fixed(dup_filter, string)
/*	BH_TABLE *dup_filter;
/*	char	*string;
/*
/*	int	been_here(dup_filter, format, ...)
/*	BH_TABLE *dup_filter;
/*	char	*format;
/*
/*	int	been_here_check_fixed(dup_filter, string)
/*	BH_TABLE *dup_filter;
/*	char	*string;
/*
/*	int	been_here_check(dup_filter, format, ...)
/*	BH_TABLE *dup_filter;
/*	char	*format;
/*
/*	void	been_here_free(dup_filter)
/*	BH_TABLE *dup_filter;
/* DESCRIPTION
/*	This module implements a simple filter to detect repeated
/*	occurrences of character strings.
/*
/*	been_here_init() creates an empty duplicate filter.
/*
/*	been_here_fixed() looks up a fixed string in the given table, and
/*	makes an entry in the table if the string was not found. The result
/*	is non-zero (true) if the string was found, zero (false) otherwise.
/*
/*	been_here() formats its arguments, looks up the result in the
/*	given table, and makes an entry in the table if the string was
/*	not found. The result is non-zero (true) if the formatted result was
/*	found, zero (false) otherwise.
/*
/*	been_here_check_fixed() and been_here_check() are similar
/*	but do not update the duplicate filter.
/*
/*	been_here_free() releases storage for a duplicate filter.
/*
/*	Arguments:
/* .IP size
/*	Upper bound on the table size; at most \fIsize\fR strings will
/*	be remembered.  Specify a value <= 0 to disable the upper bound.
/* .IP flags
/*	Requests for special processing. Specify the bitwise OR of zero
/*	or more flags:
/* .RS
/* .IP BH_FLAG_FOLD
/*	Enable case-insensitive lookup.
/* .IP BH_FLAG_NONE
/*	A manifest constant that requests no special processing.
/* .RE
/* .IP dup_filter
/*	The table with remembered names
/* .IP string
/*	Fixed search string.
/* .IP format
/*	Format for building the search string.
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
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <vstring.h>
#include <stringops.h>

/* Global library. */

#include "been_here.h"

/* been_here_init - initialize duplicate filter */

BH_TABLE *been_here_init(int limit, int flags)
{
    BH_TABLE *dup_filter;

    dup_filter = (BH_TABLE *) mymalloc(sizeof(*dup_filter));
    dup_filter->limit = limit;
    dup_filter->flags = flags;
    dup_filter->table = htable_create(0);
    return (dup_filter);
}

/* been_here_free - destroy duplicate filter */

void    been_here_free(BH_TABLE *dup_filter)
{
    htable_free(dup_filter->table, (void (*) (char *)) 0);
    myfree((char *) dup_filter);
}

/* been_here - duplicate detector with finer control */

int     been_here(BH_TABLE *dup_filter, const char *fmt,...)
{
    VSTRING *buf = vstring_alloc(100);
    int     status;
    va_list ap;

    /*
     * Construct the string to be checked.
     */
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);

    /*
     * Do the duplicate check.
     */
    status = been_here_fixed(dup_filter, vstring_str(buf));

    /*
     * Cleanup.
     */
    vstring_free(buf);
    return (status);
}

/* been_here_fixed - duplicate detector */

int     been_here_fixed(BH_TABLE *dup_filter, const char *string)
{
    char   *folded_string;
    const char *lookup_key;
    int     status;

    /*
     * Special processing: case insensitive lookup.
     */
    if (dup_filter->flags & BH_FLAG_FOLD) {
	folded_string = mystrdup(string);
	lookup_key = lowercase(folded_string);
    } else {
	folded_string = 0;
	lookup_key = string;
    }

    /*
     * Do the duplicate check.
     */
    if (htable_locate(dup_filter->table, lookup_key) != 0) {
	status = 1;
    } else {
	if (dup_filter->limit <= 0
	    || dup_filter->limit > dup_filter->table->used)
	    htable_enter(dup_filter->table, lookup_key, (char *) 0);
	status = 0;
    }
    if (msg_verbose)
	msg_info("been_here: %s: %d", string, status);

    /*
     * Cleanup.
     */
    if (folded_string)
	myfree(folded_string);

    return (status);
}

/* been_here_check - query duplicate detector with finer control */

int     been_here_check(BH_TABLE *dup_filter, const char *fmt,...)
{
    VSTRING *buf = vstring_alloc(100);
    int     status;
    va_list ap;

    /*
     * Construct the string to be checked.
     */
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);

    /*
     * Do the duplicate check.
     */
    status = been_here_check_fixed(dup_filter, vstring_str(buf));

    /*
     * Cleanup.
     */
    vstring_free(buf);
    return (status);
}

/* been_here_check_fixed - query duplicate detector */

int     been_here_check_fixed(BH_TABLE *dup_filter, const char *string)
{
    char   *folded_string;
    const char *lookup_key;
    int     status;

    /*
     * Special processing: case insensitive lookup.
     */
    if (dup_filter->flags & BH_FLAG_FOLD) {
	folded_string = mystrdup(string);
	lookup_key = lowercase(folded_string);
    } else {
	folded_string = 0;
	lookup_key = string;
    }

    /*
     * Do the duplicate check.
     */
    status = (htable_locate(dup_filter->table, lookup_key) != 0);
    if (msg_verbose)
	msg_info("been_here_check: %s: %d", string, status);

    /*
     * Cleanup.
     */
    if (folded_string)
	myfree(folded_string);

    return (status);
}
