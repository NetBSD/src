/*	$NetBSD: match_service.c,v 1.1.1.2 2013/01/02 18:58:59 tron Exp $	*/

/*++
/* NAME
/*	match_service 3
/* SUMMARY
/*	simple master.cf service name.type pattern matcher
/* SYNOPSIS
/*	#include <match_service.h>
/*
/*	ARGV	*match_service_init(pattern_list)
/*	const char *pattern_list;
/*
/*	ARGV	*match_service_init_argv(pattern_list)
/*	char	**pattern_list;
/*
/*	int	match_service_match(list, name_type)
/*	ARGV	*list;
/*	const char *name_type;
/*
/*	void match_service_free(list)
/*	ARGV	*list;
/* DESCRIPTION
/*	This module implements pattern matching for Postfix master.cf
/*	services.  This is more precise than using domain_list(3),
/*	because match_service(3) won't treat a dotted service name
/*	as a domain hierarchy. Moreover, this module has the advantage
/*	that it does not drag in all the LDAP, SQL and other map
/*	lookup client code into programs that don't need it.
/*
/*	Each pattern is of the form "name.type" or "type", where
/*	"name" and "type" are the first two fields of a master.cf
/*	entry. Patterns are separated by whitespace and/or commas.
/*	Matches are case insensitive. Patterns are matched in the
/*	specified order, and the matching process stops at the first
/*	match.  In order to reverse the result of a pattern match,
/*	precede a pattern with an exclamation point (!).
/*
/*	match_service_init() parses the pattern list. The result
/*	must be passed to match_service_match() or match_service_free().
/*
/*	match_service_init_argv() provides an alternate interface
/*	for pre-parsed strings.
/*
/*	match_service_match() matches one service name.type string
/*	against the specified pattern list.
/*
/*	match_service_free() releases storage allocated by
/*	match_service_init().
/* DIAGNOSTICS
/*	Fatal error: out of memory, malformed pattern.
/*	Panic: malformed search string.
/* SEE ALSO
/*	domain_list(3) match domain names.
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

#include <msg.h>
#include <argv.h>
#include <mymalloc.h>
#include <stringops.h>
#include <match_service.h>

/* match_service_init - initialize pattern list */

ARGV   *match_service_init(const char *patterns)
{
    const char *delim = " ,\t\r\n";
    ARGV   *list = argv_alloc(1);
    char   *saved_patterns = mystrdup(patterns);
    char   *bp = saved_patterns;
    const char *item;

    while ((item = mystrtok(&bp, delim)) != 0)
	argv_add(list, item, (char *) 0);
    argv_terminate(list);
    myfree(saved_patterns);
    return (list);
}

/* match_service_init_argv - impedance adapter */

ARGV   *match_service_init_argv(char **patterns)
{
    ARGV   *list = argv_alloc(1);
    char  **cpp;

    for (cpp = patterns; *cpp; cpp++)
	argv_add(list, *cpp, (char *) 0);
    argv_terminate(list);
    return (list);
}

/* match_service_match - match service name.type against pattern list */

int     match_service_match(ARGV *list, const char *name_type)
{
    const char *myname = "match_service_match";
    const char *type;
    char  **cpp;
    char   *pattern;
    int     match;

    /*
     * Quick check for empty list.
     */
    if (list->argv[0] == 0)
	return (0);

    /*
     * Sanity check.
     */
    if ((type = strrchr(name_type, '.')) == 0 || *++type == 0)
	msg_panic("%s: malformed service: \"%s\"; need \"name.type\" format",
		  myname, name_type);

    /*
     * Iterate over all patterns in the list, stop at the first match.
     */
    for (cpp = list->argv; (pattern = *cpp) != 0; cpp++) {
	if (msg_verbose)
	    msg_info("%s: %s ~? %s", myname, name_type, pattern);
	for (match = 1; *pattern == '!'; pattern++)
	    match = !match;
	if (strcasecmp(strchr(pattern, '.') ? name_type : type, pattern) == 0) {
	    if (msg_verbose)
		msg_info("%s: %s: found match", myname, name_type);
	    return (match);
	}
    }
    if (msg_verbose)
	msg_info("%s: %s: no match", myname, name_type);
    return (0);
}

/* match_service_free - release storage */

void    match_service_free(ARGV *list)
{
    argv_free(list);
}
