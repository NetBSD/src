/*	$NetBSD: match_list.c,v 1.2.22.1 2023/12/25 12:43:37 martin Exp $	*/

/*++
/* NAME
/*	match_list 3
/* SUMMARY
/*	generic list-based pattern matching
/* SYNOPSIS
/*	#include <match_list.h>
/*
/*	MATCH_LIST *match_list_init(pname, flags, pattern_list, count, func,...)
/*	const char *pname;
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
/*	This module implements a framework for tests for list
/*	membership.  The actual tests are done by user-supplied
/*	functions.
/*
/*	Patterns are separated by whitespace and/or commas. A pattern
/*	is either a string, a file name (in which case the contents
/*	of the file are substituted for the file name) or a type:name
/*	lookup table specification. In order to reverse the result
/*	of a pattern match, precede a pattern with an exclamation
/*	point (!).
/*
/*	match_list_init() performs initializations.  When the global
/*	util_utf8_enable variable is non-zero, and when the code
/*	is compiled with EAI support, string comparison will use
/*	caseless UTF-8 mode.  Otherwise, only ASCII characters will
/*	be casefolded.
/*
/*	match_list_match() matches strings against the specified
/*	pattern list, passing the first string to the first function
/*	given to match_list_init(), the second string to the second
/*	function, and so on.
/*
/*	match_list_free() releases storage allocated by match_list_init().
/*
/*	Arguments:
/* .IP pname
/*	Parameter name or other identifying information that is
/*	prepended to error messages.
/* .IP flags
/*	Specifies the bit-wise OR of zero or more of the following:
/* .RS
/* .IP MATCH_FLAG_PARENT
/*	The hostname pattern foo.com matches any name within the
/*	domain foo.com. If this flag is cleared, foo.com matches
/*	itself only, and .foo.com matches any name below the domain
/*	foo.com.
/* .IP MATCH_FLAG_RETURN
/*	Request that match_list_match() logs a warning and returns
/*	zero (with list->error set to a non-zero dictionary error
/*	code) instead of raising a fatal run-time error.
/* .RE
/*	Specify MATCH_FLAG_NONE to request none of the above.
/* .IP pattern_list
/*	A list of patterns.
/* .IP count
/*	Specifies how many match functions follow.
/* .IP list
/*	Pattern list produced by match_list_init().
/* .IP string
/*	Search string.
/* DIAGNOSTICS
/*	Fatal error: unable to open or read a match_list file; invalid
/*	match_list pattern; casefold error (UTF-8 mode only).
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
#include <readlline.h>
#include <stringops.h>
#include <argv.h>
#include <dict.h>
#include <match_list.h>

/* Application-specific */

#define MATCH_DICTIONARY(pattern) \
    ((pattern)[0] != '[' && strchr((pattern), ':') != 0)

/* match_list_parse - parse buffer, destroy buffer */

static ARGV *match_list_parse(MATCH_LIST *match_list, ARGV *pat_list,
			              char *string, int init_match)
{
    const char *myname = "match_list_parse";
    VSTRING *buf = vstring_alloc(10);
    VSTREAM *fp;
    const char *delim = CHARS_COMMA_SP;
    char   *bp = string;
    char   *start;
    char   *item;
    char   *map_type_name_flags;
    int     match;

    /*
     * We do not use DICT_FLAG_FOLD_FIX, because we casefold the search
     * string at the beginning of a search, and we use strcmp() for string
     * comparison. This works because string patterns are casefolded during
     * match_list initialization, and databases are supposed to fold case
     * upon creation.
     */
#define OPEN_FLAGS	O_RDONLY
#define DICT_FLAGS	(DICT_FLAG_LOCK | DICT_FLAG_UTF8_REQUEST)
#define STR(x)		vstring_str(x)

    /*
     * /filename contents are expanded in-line. To support !/filename we
     * prepend the negation operator to each item from the file.
     * 
     * If there is an error, implement graceful degradation by inserting a
     * pseudo table whose lookups fail with a warning message.
     */
    while ((start = mystrtokq_cw(&bp, delim, CHARS_BRACE,
				 match_list->pname)) != 0) {
	for (match = init_match, item = start; *item == '!'; item++)
	    match = !match;
	if (*item == 0)
	    /* No graceful degradation for this... */
	    msg_fatal("%s: no pattern after '!'", match_list->pname);
	if (*item == '/') {			/* /file/name */
	    if ((fp = vstream_fopen(item, O_RDONLY, 0)) == 0) {
		/* Replace unusable pattern with pseudo table. */
		vstring_sprintf(buf, "%s:%s", DICT_TYPE_NOFILE, item);
		if (dict_handle(STR(buf)) == 0)
		    dict_register(STR(buf),
				  dict_surrogate(DICT_TYPE_NOFILE, item,
						 OPEN_FLAGS, DICT_FLAGS,
						 "open file %s: %m", item));
		argv_add(pat_list, STR(buf), (char *) 0);
	    } else {
		while (readlline(buf, fp, (int *) 0))
		    pat_list = match_list_parse(match_list, pat_list,
						vstring_str(buf), match);
		if (vstream_fclose(fp))
		    msg_fatal("%s: read file %s: %m", myname, item);
	    }
	} else if (MATCH_DICTIONARY(item)) {	/* type:table */
	    vstring_sprintf(buf, "%s%s(%o,%s)", match ? "" : "!",
			    item, OPEN_FLAGS, dict_flags_str(DICT_FLAGS));
	    map_type_name_flags = STR(buf) + (match == 0);
	    if (dict_handle(map_type_name_flags) == 0)
		dict_register(map_type_name_flags,
			      dict_open(item, OPEN_FLAGS, DICT_FLAGS));
	    argv_add(pat_list, STR(buf), (char *) 0);
	} else {				/* other pattern */
	    casefold(match_list->fold_buf, match ?
		     item : STR(vstring_sprintf(buf, "!%s", item)));
	    argv_add(pat_list, STR(match_list->fold_buf), (char *) 0);
	}
    }
    vstring_free(buf);
    return (pat_list);
}

/* match_list_init - initialize pattern list */

MATCH_LIST *match_list_init(const char *pname, int flags,
			          const char *patterns, int match_count,...)
{
    MATCH_LIST *list;
    char   *saved_patterns;
    va_list ap;
    int     i;

    if (flags & ~MATCH_FLAG_ALL)
	msg_panic("match_list_init: bad flags 0x%x", flags);

    list = (MATCH_LIST *) mymalloc(sizeof(*list));
    list->pname = mystrdup(pname);
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
    list->error = 0;
    list->fold_buf = vstring_alloc(20);

#define DO_MATCH	1

    saved_patterns = mystrdup(patterns);
    list->patterns = match_list_parse(list, argv_alloc(1), saved_patterns,
				      DO_MATCH);
    argv_terminate(list->patterns);
    myfree(saved_patterns);
    return (list);
}

/* match_list_match - match strings against pattern list */

int     match_list_match(MATCH_LIST *list,...)
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

    list->error = 0;
    for (cpp = list->patterns->argv; (pat = *cpp) != 0; cpp++) {
	for (match = 1; *pat == '!'; pat++)
	    match = !match;
	for (i = 0; i < list->match_count; i++) {
	    casefold(list->fold_buf, list->match_args[i]);
	    if (list->match_func[i] (list, STR(list->fold_buf), pat))
		return (match);
	    else if (list->error != 0)
		return (0);
	}
    }
    if (msg_verbose)
	for (i = 0; i < list->match_count; i++)
	    msg_info("%s: %s: no match", myname, list->match_args[i]);
    return (0);
}

/* match_list_free - release storage */

void    match_list_free(MATCH_LIST *list)
{
    /* XXX Should decrement map refcounts. */
    myfree(list->pname);
    argv_free(list->patterns);
    myfree((void *) list->match_func);
    myfree((void *) list->match_args);
    vstring_free(list->fold_buf);
    myfree((void *) list);
}
