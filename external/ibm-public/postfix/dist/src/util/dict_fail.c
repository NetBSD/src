/*	$NetBSD: dict_fail.c,v 1.1.1.1 2013/01/02 18:59:12 tron Exp $	*/

/*++
/* NAME
/*	dict_fail 3
/* SUMMARY
/*	dictionary manager interface to 'always fail' table
/* SYNOPSIS
/*	#include <dict_fail.h>
/*
/*	DICT	*dict_fail_open(name, open_flags, dict_flags)
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_fail_open() implements a dummy dictionary that fails
/*	all operations. The name can be used for logging.
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

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <dict.h>
#include <dict_fail.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    int     dict_errno;			/* fixed error result */
} DICT_FAIL;

/* dict_fail_sequence - fail lookup */

static int dict_fail_sequence(DICT *dict, int unused_func,
			              const char **key, const char **value)
{
    DICT_FAIL *dp = (DICT_FAIL *) dict;

    DICT_ERR_VAL_RETURN(dict, dp->dict_errno, DICT_STAT_ERROR);
}

/* dict_fail_update - fail lookup */

static int dict_fail_update(DICT *dict, const char *unused_name,
			            const char *unused_value)
{
    DICT_FAIL *dp = (DICT_FAIL *) dict;

    DICT_ERR_VAL_RETURN(dict, dp->dict_errno, DICT_STAT_ERROR);
}

/* dict_fail_lookup - fail lookup */

static const char *dict_fail_lookup(DICT *dict, const char *unused_name)
{
    DICT_FAIL *dp = (DICT_FAIL *) dict;

    DICT_ERR_VAL_RETURN(dict, dp->dict_errno, (char *) 0);
}

/* dict_fail_delete - fail delete */

static int dict_fail_delete(DICT *dict, const char *unused_name)
{
    DICT_FAIL *dp = (DICT_FAIL *) dict;

    DICT_ERR_VAL_RETURN(dict, dp->dict_errno, DICT_STAT_ERROR);
}

/* dict_fail_close - close fail dictionary */

static void dict_fail_close(DICT *dict)
{
    dict_free(dict);
}

/* dict_fail_open - make association with fail variable */

DICT   *dict_fail_open(const char *name, int open_flags, int dict_flags)
{
    DICT_FAIL *dp;

    dp = (DICT_FAIL *) dict_alloc(DICT_TYPE_FAIL, name, sizeof(*dp));
    dp->dict.lookup = dict_fail_lookup;
    if (open_flags & O_RDWR) {
	dp->dict.update = dict_fail_update;
	dp->dict.delete = dict_fail_delete;
    }
    dp->dict.sequence = dict_fail_sequence;
    dp->dict.close = dict_fail_close;
    dp->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    dp->dict_errno = DICT_ERR_RETRY;
    dp->dict.owner.status = DICT_OWNER_TRUSTED;
    return (DICT_DEBUG (&dp->dict));
}
