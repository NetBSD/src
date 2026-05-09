/*	$NetBSD: allowed_prefix.c,v 1.2 2026/05/09 18:49:15 christos Exp $	*/

/*++
/* NAME
/*	allowed_prefix 3
/* SUMMARY
/*	Match a pathname against a list of allowed parent directories
/* SYNOPSIS
/*	#include <allowed_prefix.h>
/*
/*	ALLOWED_PREFIX *allowed_prefix_create(const char *prefixes)
/*
/*	bool	allowed_prefix_match(
/*	ALLOWED_PREFIX *tp,
/*	const char *candidate_path)
/*
/*	void	allowed_prefix_free(ALLOWED_PREFIX *tp)
/* DESCRIPTION
/*	This module matches an absolute candidate pathname against a
/*	list of allowed absolute parent directory names. The matching
/*	is based solely on string comparisons.
/*
/*	The matcher will not match relative pathnames, and logs a warning
/*	if it encounters one in its inputs.
/*
/*	allowed_prefix_create() instantiates an allowed parent matcher
/*	and returns a pointer to the result. The input must be a list
/*	of directory absolute pathnames separated by comma or whitespace.
/*
/*	allowed_prefix_match() matches a candidate absolute pathname
/*	against an ALLOWED_PREFIX instance.
/*
/*	allowed_prefix_free() destroys an ALLOWED_PREFIX instance.
/* BUGS
/*	Does not collapse multiple instances of '/' in the middle of a
/*	pathname.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <htable.h>
#include <msg.h>
#include <mymalloc.h>
#include <split_at.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <allowed_prefix.h>

/* decline_request_with - shared nagging code */

static bool decline_request_with(const char *what, const char *path)
{
    msg_warn("request to allowlist prefix with %s: '%s'", what, path);
    return (false);
}

/* allowed_prefix_create - instantiate an allowed parent matcher */

ALLOWED_PREFIX *allowed_prefix_create(const char *prefixes)
{
    ALLOWED_PREFIX *tp;
    char   *bp, *prefix, *saved_prefixes;
    size_t  len;

    bp = saved_prefixes = mystrdup(prefixes);
    tp = argv_alloc(5);
    while ((prefix = mystrtok(&bp, CHARS_COMMA_SP)) != 0) {
	if (*prefix != '/') {
	    (void) decline_request_with("relative pathname",prefix);
	    continue;
	}
	/* Trim trailing '/'. */
	for (len = strlen(prefix); len > 0; len--) {
	    if (prefix[len - 1] != '/')
		break;
	}
	prefix[len] = 0;
	argv_add(tp, prefix, (char *) 0);
    }
    myfree(saved_prefixes);
    return (tp);
}

/* allowed_prefix_match - match candidate against allowed prefixes */

bool    allowed_prefix_match(ALLOWED_PREFIX *tp, const char *candidate_path)
{
    char  **cpp;
    ssize_t len;

    if (*candidate_path != '/')
	return (decline_request_with("relative pathname", candidate_path));
    if (strstr(candidate_path, "/../") != 0)
	return (decline_request_with("'/../'", candidate_path));

    /*
     * The number of allowed prefixes will be limited. linear search will be
     * OK.
     */
    for (cpp = tp->argv; *cpp; cpp++) {
	len = strlen(*cpp);
	if (strncmp(*cpp, candidate_path, len) == 0
	    && candidate_path[len] == '/')
	    return (true);
    }
    return (false);
}

/* allowed_prefix_free - destroy allowed parent matcher */

void    allowed_prefix_free(ALLOWED_PREFIX *tp)
{
    argv_free(tp);
}
