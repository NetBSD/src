/*	$NetBSD: dict_union.c,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

/*++
/* NAME
/*	dict_union 3
/* SUMMARY
/*	dictionary manager interface for union of tables
/* SYNOPSIS
/*	#include <dict_union.h>
/*
/*	DICT	*dict_union_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_union_open() opens a sequence of one or more tables.
/*	Example: "\fBunionmap:{\fItype_1:name_1, ..., type_n:name_n\fR}".
/*
/*	Each "unionmap:" query is given to each table in the specified
/*	order. All found results are concatenated, separated by
/*	comma.  The unionmap table produces no result when all
/*	lookup tables return no result.
/*
/*	The first and last characters of a "unionmap:" table name
/*	must be '{' and '}'. Within these, individual maps are
/*	separated with comma or whitespace.
/*
/*	The open_flags and dict_flags arguments are passed on to
/*	the underlying dictionaries.
/* SEE ALSO
/*	dict(3) generic dictionary manager
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

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <dict.h>
#include <dict_union.h>
#include <stringops.h>
#include <vstring.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    ARGV   *map_union;			/* pipelined tables */
    VSTRING *re_buf;			/* reply buffer */
} DICT_UNION;

#define STR(x) vstring_str(x)

/* dict_union_lookup - search a bunch of tables and combine the results */

static const char *dict_union_lookup(DICT *dict, const char *query)
{
    static const char myname[] = "dict_union_lookup";
    DICT_UNION *dict_union = (DICT_UNION *) dict;
    DICT   *map;
    char  **cpp;
    char   *dict_type_name;
    const char *result = 0;

    /*
     * After Roel van Meer, postfix-users mailing list, Sept 2014.
     */
    VSTRING_RESET(dict_union->re_buf);
    for (cpp = dict_union->map_union->argv; (dict_type_name = *cpp) != 0; cpp++) {
	if ((map = dict_handle(dict_type_name)) == 0)
	    msg_panic("%s: dictionary \"%s\" not found", myname, dict_type_name);
	if ((result = dict_get(map, query)) != 0) {
	    if (VSTRING_LEN(dict_union->re_buf) > 0)
		VSTRING_ADDCH(dict_union->re_buf, ',');
	    vstring_strcat(dict_union->re_buf, result);
	} else if (map->error != 0) {
	    DICT_ERR_VAL_RETURN(dict, map->error, 0);
	}
    }
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE,
			VSTRING_LEN(dict_union->re_buf) > 0 ?
			STR(dict_union->re_buf) : 0);
}

/* dict_union_close - disassociate from a bunch of tables */

static void dict_union_close(DICT *dict)
{
    DICT_UNION *dict_union = (DICT_UNION *) dict;
    char  **cpp;
    char   *dict_type_name;

    for (cpp = dict_union->map_union->argv; (dict_type_name = *cpp) != 0; cpp++)
	dict_unregister(dict_type_name);
    argv_free(dict_union->map_union);
    vstring_free(dict_union->re_buf);
    dict_free(dict);
}

/* dict_union_open - open a bunch of tables */

DICT   *dict_union_open(const char *name, int open_flags, int dict_flags)
{
    static const char myname[] = "dict_union_open";
    DICT_UNION *dict_union;
    char   *saved_name = 0;
    char   *dict_type_name;
    ARGV   *argv = 0;
    char  **cpp;
    DICT   *dict;
    int     match_flags = 0;
    struct DICT_OWNER aggr_owner;
    size_t  len;

    /*
     * Clarity first. Let the optimizer worry about redundant code.
     */
#define DICT_UNION_RETURN(x) do { \
	      if (saved_name != 0) \
	          myfree(saved_name); \
	      if (argv != 0) \
	          argv_free(argv); \
	      return (x); \
	  } while (0)

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	DICT_UNION_RETURN(dict_surrogate(DICT_TYPE_UNION, name,
					 open_flags, dict_flags,
				  "%s:%s map requires O_RDONLY access mode",
					 DICT_TYPE_UNION, name));

    /*
     * Split the table name into its constituent parts.
     */
    if ((len = balpar(name, CHARS_BRACE)) == 0 || name[len] != 0
	|| *(saved_name = mystrndup(name + 1, len - 2)) == 0
	|| ((argv = argv_splitq(saved_name, CHARS_COMMA_SP, CHARS_BRACE)),
	    (argv->argc == 0)))
	DICT_UNION_RETURN(dict_surrogate(DICT_TYPE_UNION, name,
					 open_flags, dict_flags,
					 "bad syntax: \"%s:%s\"; "
					 "need \"%s:{type:name...}\"",
					 DICT_TYPE_UNION, name,
					 DICT_TYPE_UNION));

    /*
     * The least-trusted table in the set determines the over-all trust
     * level. The first table determines the pattern-matching flags.
     */
    DICT_OWNER_AGGREGATE_INIT(aggr_owner);
    for (cpp = argv->argv; (dict_type_name = *cpp) != 0; cpp++) {
	if (msg_verbose)
	    msg_info("%s: %s", myname, dict_type_name);
	if (strchr(dict_type_name, ':') == 0)
	    DICT_UNION_RETURN(dict_surrogate(DICT_TYPE_UNION, name,
					     open_flags, dict_flags,
					     "bad syntax: \"%s:%s\"; "
					     "need \"%s:{type:name...}\"",
					     DICT_TYPE_UNION, name,
					     DICT_TYPE_UNION));
	if ((dict = dict_handle(dict_type_name)) == 0)
	    dict = dict_open(dict_type_name, open_flags, dict_flags);
	dict_register(dict_type_name, dict);
	DICT_OWNER_AGGREGATE_UPDATE(aggr_owner, dict->owner);
	if (cpp == argv->argv)
	    match_flags = dict->flags & (DICT_FLAG_FIXED | DICT_FLAG_PATTERN);
    }

    /*
     * Bundle up the result.
     */
    dict_union =
	(DICT_UNION *) dict_alloc(DICT_TYPE_UNION, name, sizeof(*dict_union));
    dict_union->dict.lookup = dict_union_lookup;
    dict_union->dict.close = dict_union_close;
    dict_union->dict.flags = dict_flags | match_flags;
    dict_union->dict.owner = aggr_owner;
    dict_union->re_buf = vstring_alloc(100);
    dict_union->map_union = argv;
    argv = 0;
    DICT_UNION_RETURN(DICT_DEBUG (&dict_union->dict));
}
