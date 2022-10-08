/*	$NetBSD: dict_static.c,v 1.4 2022/10/08 16:12:50 christos Exp $	*/

/*++
/* NAME
/*	dict_static 3
/* SUMMARY
/*	dictionary manager interface to static variables
/* SYNOPSIS
/*	#include <dict_static.h>
/*
/*	DICT	*dict_static_open(name, name, dict_flags)
/*	const char *name;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_static_open() implements a dummy dictionary that returns
/*	as lookup result the dictionary name, regardless of the lookup
/*	key value.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	jeffm
/*	ghostgun.com
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include "sys_defs.h"
#include <stdio.h>			/* sprintf() prototype */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "stringops.h"
#include "dict.h"
#include "dict_static.h"

 /*
  * Subclass of DICT.
  */
typedef struct {
    DICT    dict;			/* parent class */
    char   *value;
} DICT_STATIC;

#define STR(x)	vstring_str(x)

/* dict_static_lookup - access static value*/

static const char *dict_static_lookup(DICT *dict, const char *unused_name)
{
    DICT_STATIC *dict_static = (DICT_STATIC *) dict;

    DICT_ERR_VAL_RETURN(dict, DICT_ERR_NONE, dict_static->value);
}

/* dict_static_close - close static dictionary */

static void dict_static_close(DICT *dict)
{
    DICT_STATIC *dict_static = (DICT_STATIC *) dict;

    if (dict_static->value)
	myfree(dict_static->value);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_static_open - make association with static variable */

DICT   *dict_static_open(const char *name, int open_flags, int dict_flags)
{
    DICT_STATIC *dict_static;
    char   *err = 0;
    char   *cp, *saved_name = 0;
    const char *value;
    VSTRING *base64_buf;

    /*
     * Let the optimizer worry about eliminating redundant code.
     */
#define DICT_STATIC_OPEN_RETURN(d) do { \
        DICT *__d = (d); \
        if (saved_name != 0) \
            myfree(saved_name); \
	if (err != 0) \
	    myfree(err); \
        return (__d); \
    } while (0)

    /*
     * Optionally strip surrounding braces and whitespace.
     */
    if (name[0] == CHARS_BRACE[0]) {
	saved_name = cp = mystrdup(name);
	if ((err = extpar(&cp, CHARS_BRACE, EXTPAR_FLAG_STRIP)) != 0)
	    DICT_STATIC_OPEN_RETURN(dict_surrogate(DICT_TYPE_STATIC, name,
						   open_flags, dict_flags,
						   "bad %s:name syntax: %s",
						   DICT_TYPE_STATIC,
						   err));
	value = cp;
    } else {
	value = name;
    }

    /*
     * Bundle up the preliminary result.
     */
    dict_static = (DICT_STATIC *) dict_alloc(DICT_TYPE_STATIC, name,
					     sizeof(*dict_static));
    dict_static->dict.lookup = dict_static_lookup;
    dict_static->dict.close = dict_static_close;
    dict_static->dict.flags = dict_flags | DICT_FLAG_FIXED;
    dict_static->dict.owner.status = DICT_OWNER_TRUSTED;
    dict_static->value = 0;

    /*
     * Optionally replace the value with file contents.
     */
    if (dict_flags & DICT_FLAG_SRC_RHS_IS_FILE) {
	if ((base64_buf = dict_file_to_b64(&dict_static->dict, value)) == 0) {
	    err = dict_file_get_error(&dict_static->dict);
	    dict_close(&dict_static->dict);
	    DICT_STATIC_OPEN_RETURN(dict_surrogate(DICT_TYPE_STATIC, name,
						   open_flags, dict_flags,
						   "%s", err));
	}
	value = vstring_str(base64_buf);
    }

    /*
     * Finalize the result.
     */
    dict_static->value = mystrdup(value);
    dict_file_purge_buffers(&dict_static->dict);

    DICT_STATIC_OPEN_RETURN(DICT_DEBUG (&(dict_static->dict)));
}
