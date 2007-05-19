/*	$NetBSD: match_list.c,v 1.10 2007/05/19 17:49:51 heas Exp $	*/

/*++
/* NAME
/*	match_list 3
/* SUMMARY
/*	generic list-based pattern matching
/* SYNOPSIS
/*	#include <match_list.h>
/*
/*	MATCH_LIST *match_list_init(flags, pattern_list, count, func,...)
/*	int	flags;
/*	const char *pattern_list;
/*	int	count;
/*	int	(*func)(int flags, const char *string, const char *pattern);
/*
/*	int	match_list_match(list, string,...)
/*	MATCH_LIST *list;
/*	const char *string;
/*
/*	void match_list_free(list)
/*	MATCH_LIST *list;
/* DESCRIPTION
/*	This module implements a framework for tests for list membership.
/*	The actual tests are done by user-supplied functions.
/*
/*	Patterns are separated by whitespace and/or commas. A pattern
/*	is either a string, a file name (in which case the contents
/*	of the file are substituted for the file name) or a type:name
/*	lookup table specification. In order to reverse the result of
/*	a pattern match, precede a pattern with an exclamation point (!).
/*
/*	match_list_init() performs initializations. The flags argument
/*	specifies the bit-wise OR of zero or more of the following:
/* .RS
/* .IP MATCH_FLAG_PARENT
/*	The hostname pattern foo.com matches any name within the domain
/*	foo.com. If this flag is cleared, foo.com matches itself
/*	only, and .foo.com matches any name below the domain foo.com.
/* .RE
/*	Specify MATCH_FLAG_NONE to request none of the above.
/*	The pattern_list argument specifies a list of patterns.  The third
/*	argument specifies how many match functions follow.
/*
/*	match_list_match() matches strings against the specified pattern
/*	list, passing the first string to the first function given to
/*	match_list_init(), the second string to the second function, and
/*	so on.
/*
/*	match_list_free() releases storage allocated by match_list_init().
/* DIAGNOSTICS
/*	Fatal error: unable to open or read a match_list file; invalid
/*	match_list pattern.
/* SEE ALSO
/*	host_match(3) match hosts by name or by address
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
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <argv.h>
#include <dict.h>
#include <match_ops.h>
#include <match_list.h>

/* Application-specific */

struct MATCH_LIST {
    int     flags;			/* processing options */
    ARGV   *patterns;			/* one pattern each */
    int     match_count;		/* match function/argument count */
    MATCH_LIST_FN *match_func;		/* match functions */
    const char **match_args;		/* match arguments */
};

#define MATCH_DICTIONARY(pattern) \
    ((pattern)[0] != '[' && strchr((pattern), ':') != 0)

/* match_list_parse - parse buffer, destroy buffer */

static ARGV *match_list_parse(ARGV *list, char *string, int init_match)
{
    const char *myname = "match_list_parse";
    VSTRING *buf = vstring_alloc(10);
    VSTREAM *fp;
    const char *delim = " ,\t\r\n";
    char   *bp = string;
    char   *start;
    char   *item;
    char   *map_type_name_flags;
    int     match;

    /*
     * /filename contents are expanded in-line. To support !/filename we
     * prepend the negation operator to each item from the file.
     */
    while ((start = mystrtok(&bp, delim)) != 0) {
	for (match = init_match, item = start; *item == '!'; item++)
	    match = !match;
	if (*item == 0)
	    msg_fatal("%s: no pattern after '!'", myname);
	if (*item == '/') {			/* /file/name */
	    if ((fp = vstream_fopen(item, O_RDONLY, 0)) == 0)
		msg_fatal("%s: open file %s: %m", myname, item);
	    while (vstring_fgets(buf, fp))
		if (vstring_str(buf)[0] != '#')
		    list = match_list_parse(list, vstring_str(buf), match);
	    if (vstream_fclose(fp))
		msg_fatal("%s: read file %s: %m", myname, item);
	} else if (MATCH_DICTIONARY(item)) {	/* type:table */
#define OPEN_FLAGS	O_RDONLY
#define DICT_FLAGS	(DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX)
#define STR(x)		vstring_str(x)
	    vstring_sprintf(buf, "%s%s(%o,%s)", match ? "" : "!",
			    item, OPEN_FLAGS, dict_flags_str(DICT_FLAGS));
	    map_type_name_flags = STR(buf) + (match == 0);
	    if (dict_handle(map_type_name_flags) == 0)
		dict_register(map_type_name_flags,
			      dict_open(item, OPEN_FLAGS, DICT_FLAGS));
	    argv_add(list, STR(buf), (char *) 0);
	} else {				/* other pattern */
	    argv_add(list, match ? item :
		     STR(vstring_sprintf(buf, "!%s", item)), (char *) 0);
	}
    }
    vstring_free(buf);
    return (list);
}

/* match_list_init - initialize pattern list */

MATCH_LIST *match_list_init(int flags, const char *patterns, int match_count,...)
{
    MATCH_LIST *list;
    char   *saved_patterns;
    va_list ap;
    int     i;

    if (flags & ~MATCH_FLAG_ALL)
	msg_panic("match_list_init: bad flags 0x%x", flags);

    list = (MATCH_LIST *) mymalloc(sizeof(*list));
    list->flags = flags;
    list->match_count = match_count;
    list->match_func =
	(MATCH_LIST_FN *) mymalloc(match_count * sizeof(MATCH_LIST_FN));
    list->match_args =
	(const char **) mymalloc(match_count * sizeof(const char *));
    va_start(ap, match_count);
    for (i = 0; i < match_count; i++)
	list->match_func[i] = va_arg(ap, MATCH_LIST_FN);
    va_end(ap);

#define DO_MATCH	1

    saved_patterns = mystrdup(patterns);
    list->patterns = match_list_parse(argv_alloc(1), saved_patterns, DO_MATCH);
    argv_terminate(list->patterns);
    myfree(saved_patterns);
    return (list);
}

/* match_list_match - match strings against pattern list */

int     match_list_match(MATCH_LIST * list,...)
{
    const char *myname = "match_list_match";
    char  **cpp;
    char   *pat;
    int     match;
    int     i;
    va_list ap;

    /*
     * Iterate over all patterns in the list, stop at the first match.
     */
    va_start(ap, list);
    for (i = 0; i < list->match_count; i++)
	list->match_args[i] = va_arg(ap, const char *);
    va_end(ap);

    for (cpp = list->patterns->argv; (pat = *cpp) != 0; cpp++) {
	for (match = 1; *pat == '!'; pat++)
	    match = !match;
	for (i = 0; i < list->match_count; i++)
	    if (list->match_func[i] (list->flags, list->match_args[i], pat))
		return (match);
    }
    if (msg_verbose)
	for (i = 0; i < list->match_count; i++)
	    msg_info("%s: %s: no match", myname, list->match_args[i]);
    return (0);
}

/* match_list_free - release storage */

void    match_list_free(MATCH_LIST * list)
{
    argv_free(list->patterns);
    myfree((char *) list->match_func);
    myfree((char *) list->match_args);
    myfree((char *) list);
}
