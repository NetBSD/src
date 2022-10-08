/*	$NetBSD: dict_random.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	dict_random 3
/* SUMMARY
/*	dictionary manager interface for randomized tables
/* SYNOPSIS
/*	#include <dict_random.h>
/*
/*	DICT	*dict_random_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_random_open() opens an in-memory, read-only, table.
/*	Example: "\fBrandmap:{\fIresult_1, ... ,result_n}\fR".
/*
/*	Each table query returns a random choice from the specified
/*	results. Other table access methods are not supported.
/*
/*	The first and last characters of the "randmap:" table name
/*	must be '{' and '}'. Within these, individual maps are
/*	separated with comma or whitespace.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <myrand.h>
#include <stringops.h>
#include <dict_random.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    ARGV   *replies;			/* reply values */
} DICT_RANDOM;

#define STR(x) vstring_str(x)

/* dict_random_lookup - find randomized-table entry */

static const char *dict_random_lookup(DICT *dict, const char *unused_query)
{
    DICT_RANDOM *dict_random = (DICT_RANDOM *) dict;

    DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE,
	 dict_random->replies->argv[myrand() % dict_random->replies->argc]);
}

/* dict_random_close - disassociate from randomized table */

static void dict_random_close(DICT *dict)
{
    DICT_RANDOM *dict_random = (DICT_RANDOM *) dict;

    if (dict_random->replies)
	argv_free(dict_random->replies);
    dict_free(dict);
}

static char *dict_random_parse_name(DICT *dict, ARGV **argv,
				            const char *string,
				            const char *delim,
				            const char *parens)
{
    ARGV   *argvp = argv_alloc(1);
    char   *saved_string = mystrdup(string);
    char   *bp = saved_string;
    char   *arg;
    VSTRING *b64 = 0;
    char   *err = 0;

    while ((arg = mystrtokq(&bp, delim, parens)) != 0) {
	if (dict->flags & DICT_FLAG_SRC_RHS_IS_FILE) {
	    if ((b64 = dict_file_to_b64(dict, arg)) != 0) {
		argv_add(argvp, vstring_str(b64), (char *) 0);
	    } else {
		err = dict_file_get_error(dict);
		break;
	    }
	} else {
	    argv_add(argvp, arg, (char *) 0);
	}
    }
    argv_terminate(argvp);
    myfree(saved_string);
    *argv = argvp;
    return (err);
}

/* dict_random_open - open a randomized table */

DICT   *dict_random_open(const char *name, int open_flags, int dict_flags)
{
    DICT_RANDOM *dict_random;
    char   *saved_name = 0;
    size_t  len;
    char   *err = 0;

    /*
     * Clarity first. Let the optimizer worry about redundant code.
     */
#define DICT_RANDOM_RETURN(x) do { \
	DICT *__d = (x); \
	if (saved_name != 0) \
	    myfree(saved_name); \
	if (err != 0) \
	    myfree(err); \
	return (__d); \
    } while (0)

    /*
     * Sanity checks.
     */
    if (open_flags != O_RDONLY)
	DICT_RANDOM_RETURN(dict_surrogate(DICT_TYPE_RANDOM, name,
					  open_flags, dict_flags,
				  "%s:%s map requires O_RDONLY access mode",
					  DICT_TYPE_RANDOM, name));

    /*
     * Bundle up preliminary results.
     */
    dict_random =
	(DICT_RANDOM *) dict_alloc(DICT_TYPE_RANDOM, name, sizeof(*dict_random));
    dict_random->dict.lookup = dict_random_lookup;
    dict_random->dict.close = dict_random_close;
    dict_random->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    dict_random->replies = 0;
    dict_random->dict.owner.status = DICT_OWNER_TRUSTED;
    dict_random->dict.owner.uid = 0;

    /*
     * Split the table name into its constituent parts.
     */
    if ((len = balpar(name, CHARS_BRACE)) == 0 || name[len] != 0
	|| *(saved_name = mystrndup(name + 1, len - 2)) == 0
	|| (err = dict_random_parse_name(&dict_random->dict,
					 &dict_random->replies, saved_name,
					 CHARS_COMMA_SP, CHARS_BRACE)) != 0
	|| dict_random->replies->argc == 0) {
	dict_random_close(&dict_random->dict);
	DICT_RANDOM_RETURN(err == 0 ?
			   dict_surrogate(DICT_TYPE_RANDOM, name,
					  open_flags, dict_flags,
					  "bad syntax: \"%s:%s\"; "
					  "need \"%s:{value...}\"",
					  DICT_TYPE_RANDOM, name,
					  DICT_TYPE_RANDOM) :
			   dict_surrogate(DICT_TYPE_RANDOM, name,
					  open_flags, dict_flags,
					  "%s", err));
    }
    dict_file_purge_buffers(&dict_random->dict);
    DICT_RANDOM_RETURN(DICT_DEBUG (&dict_random->dict));
}
