/*	$NetBSD: dict_surrogate.c,v 1.1.1.1.2.2 2013/01/23 00:05:16 yamt Exp $	*/

/*++
/* NAME
/*	dict_surrogate 3
/* SUMMARY
/*	surrogate table for graceful "open" failure
/* SYNOPSIS
/*	#include <dict_surrogate.h>
/*
/*	DICT	*dict_surrogate(dict_type, dict_name,
/*				open_flags, dict_flags,
/*				format, ...)
/*	const char *dict_type;
/*	const char *dict_name;
/*	int	open_flags;
/*	int	dict_flags;
/*	const char *format;
/*
/*	int	dict_allow_surrogate;
/* DESCRIPTION
/*	dict_surrogate() either terminates the program with a fatal
/*	error, or provides a dummy dictionary that fails all
/*	operations with an error message, allowing the program to
/*	continue with reduced functionality.
/*
/*	The global dict_allow_surrogate variable controls the choice
/*	between fatal error or reduced functionality. The default
/*	value is zero (fatal error). This is appropriate for user
/*	commands; the non-default is more appropriate for daemons.
/*
/*	Arguments:
/* .IP dict_type
/* .IP dict_name
/* .IP open_flags
/* .IP dict_flags
/*	The parameters to the failed dictionary open() request.
/* .IP format, ...
/*	The reason why the table could not be opened. This text is
/*	logged immediately as an "error" class message, and is logged
/*	as a "warning" class message upon every attempt to access the
/*	surrogate dictionary, before returning a "failed" completion
/*	status.
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
#include <errno.h>

/* Utility library. */

#include <mymalloc.h>
#include <msg.h>
#include <dict.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    char   *reason;			/* open failure reason */
} DICT_SURROGATE;

/* dict_surrogate_sequence - fail lookup */

static int dict_surrogate_sequence(DICT *dict, int unused_func,
			               const char **key, const char **value)
{
    DICT_SURROGATE *dp = (DICT_SURROGATE *) dict;

    msg_warn("%s:%s is unavailable. %s",
	     dict->type, dict->name, dp->reason);
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, DICT_STAT_ERROR);
}

/* dict_surrogate_update - fail lookup */

static int dict_surrogate_update(DICT *dict, const char *unused_name,
				         const char *unused_value)
{
    DICT_SURROGATE *dp = (DICT_SURROGATE *) dict;

    msg_warn("%s:%s is unavailable. %s",
	     dict->type, dict->name, dp->reason);
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, DICT_STAT_ERROR);
}

/* dict_surrogate_lookup - fail lookup */

static const char *dict_surrogate_lookup(DICT *dict, const char *unused_name)
{
    DICT_SURROGATE *dp = (DICT_SURROGATE *) dict;

    msg_warn("%s:%s is unavailable. %s",
	     dict->type, dict->name, dp->reason);
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, (char *) 0);
}

/* dict_surrogate_delete - fail delete */

static int dict_surrogate_delete(DICT *dict, const char *unused_name)
{
    DICT_SURROGATE *dp = (DICT_SURROGATE *) dict;

    msg_warn("%s:%s is unavailable. %s",
	     dict->type, dict->name, dp->reason);
    DICT_ERR_VAL_RETURN(dict, DICT_ERR_RETRY, DICT_STAT_ERROR);
}

/* dict_surrogate_close - close fail dictionary */

static void dict_surrogate_close(DICT *dict)
{
    DICT_SURROGATE *dp = (DICT_SURROGATE *) dict;

    myfree((char *) dp->reason);
    dict_free(dict);
}

int     dict_allow_surrogate = 0;

/* dict_surrogate - terminate or provide surrogate dictionary */

DICT   *dict_surrogate(const char *dict_type, const char *dict_name,
		               int open_flags, int dict_flags,
		               const char *fmt,...)
{
    va_list ap;
    DICT_SURROGATE *dp;
    VSTRING *buf;
    void    (*log_fn) (const char *, va_list);
    int     saved_errno = errno;

    /*
     * Log the problem immediately when it is detected. The table may not be
     * accessed in every program execution (that is the whole point of
     * continuing with reduced functionality) but we don't want the problem
     * to remain unnoticed until long after a configuration mistake is made.
     */
    log_fn = dict_allow_surrogate ? vmsg_error : vmsg_fatal;
    va_start(ap, fmt);
    log_fn(fmt, ap);
    va_end(ap);

    /*
     * Log the problem upon each access.
     */
    dp = (DICT_SURROGATE *) dict_alloc(dict_type, dict_name, sizeof(*dp));
    dp->dict.lookup = dict_surrogate_lookup;
    if (open_flags & O_RDWR) {
	dp->dict.update = dict_surrogate_update;
	dp->dict.delete = dict_surrogate_delete;
    }
    dp->dict.sequence = dict_surrogate_sequence;
    dp->dict.close = dict_surrogate_close;
    dp->dict.flags = dict_flags | DICT_FLAG_PATTERN;
    dp->dict.owner.status = DICT_OWNER_TRUSTED;
    buf = vstring_alloc(10);
    errno = saved_errno;
    va_start(ap, fmt);
    vstring_vsprintf(buf, fmt, ap);
    va_end(ap);
    dp->reason = vstring_export(buf);
    return (DICT_DEBUG (&dp->dict));
}
