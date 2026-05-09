/*	$NetBSD: dict_debug.c,v 1.3 2026/05/09 18:49:22 christos Exp $	*/

/*++
/* NAME
/*	dict_debug 3
/* SUMMARY
/*	dictionary manager, logging proxy
/* SYNOPSIS
/*	#include <dict_debug.h>
/*
/*	DICT	*dict_debug_open(name, open_flags, dict_flags);
/*	const char *name;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_debug_open() encapsulates the named dictionary object and
/*	returns a proxy object that logs all access to the encapsulated
/*	object.
/*
/*	If the encapsulated dictionary is not already registered, it is
/*	opened with a call to dict_open(\fIname\fR, \fIopen_flags\fR,
/*	\fIdict_flags\fR) before registering it.
/* DIAGNOSTICS
/*	Fatal errors: out of memory.
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
/*	Richard Hansen <rhansen@rhansen.org>
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

/* System libraries. */

#include <sys_defs.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <dict.h>
#include <dict_debug.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* the proxy service */
    DICT   *real_dict;			/* encapsulated object */
} DICT_DEBUG;

/* dict_debug_lookup - log lookup operation */

static const char *dict_debug_lookup(DICT *dict, const char *key)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    DICT   *real_dict = dict_debug->real_dict;
    const char *result;

    real_dict->flags = dict->flags;
    result = dict_get(real_dict, key);
    dict->flags = real_dict->flags;
    msg_info("%s lookup: \"%s\" = \"%s\"", dict->name, key,
	     result ? result : real_dict->error ? "error" : "not_found");
    DICT_ERR_VAL_RETURN(dict, real_dict->error, result);
}

/* dict_debug_update - log update operation */

static int dict_debug_update(DICT *dict, const char *key, const char *value)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    DICT   *real_dict = dict_debug->real_dict;
    int     result;

    real_dict->flags = dict->flags;
    result = dict_put(real_dict, key, value);
    dict->flags = real_dict->flags;
    msg_info("%s update: \"%s\" = \"%s\": %s", dict->name,
	     key, value, result == 0 ? "success" : real_dict->error ?
	     "error" : "failed");
    DICT_ERR_VAL_RETURN(dict, real_dict->error, result);
}

/* dict_debug_delete - log delete operation */

static int dict_debug_delete(DICT *dict, const char *key)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    DICT   *real_dict = dict_debug->real_dict;
    int     result;

    real_dict->flags = dict->flags;
    result = dict_del(real_dict, key);
    dict->flags = real_dict->flags;
    msg_info("%s delete: \"%s\": %s", dict->name, key,
	     result == 0 ? "success" : real_dict->error ?
	     "error" : "failed");
    DICT_ERR_VAL_RETURN(dict, real_dict->error, result);
}

/* dict_debug_sequence - log sequence operation */

static int dict_debug_sequence(DICT *dict, int function,
			               const char **key, const char **value)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    DICT   *real_dict = dict_debug->real_dict;
    int     result;

    real_dict->flags = dict->flags;
    result = dict_seq(real_dict, function, key, value);
    dict->flags = real_dict->flags;
    if (result == 0)
	msg_info("%s sequence: \"%s\" = \"%s\"", dict->name, *key, *value);
    else
	msg_info("%s sequence: found EOF", dict->name);
    DICT_ERR_VAL_RETURN(dict, real_dict->error, result);
}

/* dict_debug_close - log operation */

static void dict_debug_close(DICT *dict)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;

    dict_close(dict_debug->real_dict);
    dict_free(dict);
}

/* dict_debug_open - encapsulate dictionary object and install proxies */

DICT   *dict_debug_open(const char *name, int open_flags, int dict_flags)
{
    static const char myname[] = "dict_debug_open";
    DICT   *real_dict;
    DICT_DEBUG *dict_debug;

    if (msg_verbose)
	msg_info("%s: %s", myname, name);

    /*
     * dict_open() will reuse a previously registered table if present. This
     * prevents a config containing both debug:foo:bar and foo:bar from
     * creating two DICT objects for foo:bar.
     */
    real_dict = dict_open(name, open_flags, dict_flags);
    dict_debug = (DICT_DEBUG *) dict_alloc(DICT_TYPE_DEBUG, name,
					   sizeof(*dict_debug));

    dict_debug->dict.flags = real_dict->flags;	/* XXX not synchronized */
    dict_debug->dict.lookup = dict_debug_lookup;
    dict_debug->dict.update = dict_debug_update;
    dict_debug->dict.delete = dict_debug_delete;
    dict_debug->dict.sequence = dict_debug_sequence;
    dict_debug->dict.close = dict_debug_close;
    dict_debug->real_dict = real_dict;
    dict_debug->dict.owner = real_dict->owner;
    return (&dict_debug->dict);
}
