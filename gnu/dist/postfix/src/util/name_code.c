/*++
/* NAME
/*	name_code 3
/* SUMMARY
/*	name to number table mapping
/* SYNOPSIS
/*	#include <name_code.h>
/*
/*	typedef struct {
/* .in +4
/*		const char *name;
/*		int code;
/* .in -4
/*	} NAME_CODE;
/*
/*	int	name_code(table, flags, name)
/*	const NAME_CODE *table;
/*	int	flags;
/*	const char *name;
/*
/*	const char *str_name_code(table, code)
/*	const NAME_CODE *table;
/*	int	code;
/* DESCRIPTION
/*	This module does simple name<->number mapping. The process
/*	is controlled by a table of (name, code) values.
/*	The table is terminated with a null pointer and a code that
/*	corresponds to "name not found".
/*
/*	name_code() looks up the code that corresponds with the name.
/*	The lookup is case insensitive. The flags argument specifies
/*	zero or more of the following:
/* .IP NAME_CODE_FLAG_STRICT_CASE
/*	String lookups are case sensitive.
/* .PP
/*	For convenience the constant NAME_CODE_FLAG_NONE requests
/*	no special processing.
/*
/*	str_name_code() translates a number to its equivalent string.
/* DIAGNOSTICS
/*	When the search fails, the result is the "name not found" code
/*	or the null pointer, respectively.
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <name_code.h>

/* name_code - look up code by name */

int     name_code(const NAME_CODE *table, int flags, const char *name)
{
    const NAME_CODE *np;
    int     (*lookup) (const char *, const char *);

    if (flags & NAME_CODE_FLAG_STRICT_CASE)
	lookup = strcmp;
    else
	lookup = strcasecmp;

    for (np = table; np->name; np++)
	if (lookup(name, np->name) == 0)
	    break;
    return (np->code);
}

/* str_name_code - look up name by code */

const char *str_name_code(const NAME_CODE *table, int code)
{
    const NAME_CODE *np;

    for (np = table; np->name; np++)
	if (code == np->code)
	    break;
    return (np->name);
}
