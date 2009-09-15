/*	$NetBSD: dict_debug.c,v 1.1.1.1.2.2 2009/09/15 06:03:55 snj Exp $	*/

/*++
/* NAME
/*	dict_debug 3
/* SUMMARY
/*	dictionary manager, logging proxy
/* SYNOPSIS
/*	#include <dict.h>
/*
/*	DICT	*dict_debug(dict_handle)
/*	DICT	*dict_handle;
/*
/*	DICT	*DICT_DEBUG(dict_handle)
/*	DICT	*dict_handle;
/* DESCRIPTION
/*	dict_debug() encapsulates the given dictionary object and returns
/*	a proxy object that logs all access to the encapsulated object.
/*	This is more convenient than having to add logging capability
/*	to each individual dictionary access method.
/*
/*	DICT_DEBUG() is an unsafe macro that returns the original object if
/*	the object's debugging flag is not set, and that otherwise encapsulates
/*	the object with dict_debug(). This macro simplifies usage by avoiding
/*	clumsy expressions. The macro evaluates its argument multiple times.
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
/*--*/

/* System libraries. */

#include <sys_defs.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <dict.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* the proxy service */
    DICT   *real_dict;			/* encapsulated object */
} DICT_DEBUG;

/* dict_debug_lookup - log lookup operation */

static const char *dict_debug_lookup(DICT *dict, const char *key)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    const char *result;

    result = dict_get(dict_debug->real_dict, key);
    msg_info("%s:%s lookup: \"%s\" = \"%s\"", dict->type, dict->name, key,
	     result ? result : dict_errno ? "try again" : "not_found");
    return (result);
}

/* dict_debug_update - log update operation */

static void dict_debug_update(DICT *dict, const char *key, const char *value)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;

    msg_info("%s:%s update: \"%s\" = \"%s\"", dict->type, dict->name,
	     key, value);
    dict_put(dict_debug->real_dict, key, value);
}

/* dict_debug_delete - log delete operation */

static int dict_debug_delete(DICT *dict, const char *key)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    int     result;

    result = dict_del(dict_debug->real_dict, key);
    msg_info("%s:%s delete: \"%s\" = \"%s\"", dict->type, dict->name, key,
	     result ? "failed" : "success");
    return (result);
}

/* dict_debug_sequence - log sequence operation */

static int dict_debug_sequence(DICT *dict, int function,
			               const char **key, const char **value)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;
    int     result;

    result = dict_seq(dict_debug->real_dict, function, key, value);
    if (result == 0)
	msg_info("%s:%s sequence: \"%s\" = \"%s\"", dict->type, dict->name,
		 *key, *value);
    else
	msg_info("%s:%s sequence: found EOF", dict->type, dict->name);
    return (result);
}

/* dict_debug_close - log operation */

static void dict_debug_close(DICT *dict)
{
    DICT_DEBUG *dict_debug = (DICT_DEBUG *) dict;

    dict_close(dict_debug->real_dict);
    dict_free(dict);
}

/* dict_debug - encapsulate dictionary object and install proxies */

DICT   *dict_debug(DICT *real_dict)
{
    DICT_DEBUG *dict_debug;

    dict_debug = (DICT_DEBUG *) dict_alloc(real_dict->type,
				      real_dict->name, sizeof(*dict_debug));
    dict_debug->dict.flags = real_dict->flags;	/* XXX not synchronized */
    dict_debug->dict.lookup = dict_debug_lookup;
    dict_debug->dict.update = dict_debug_update;
    dict_debug->dict.delete = dict_debug_delete;
    dict_debug->dict.sequence = dict_debug_sequence;
    dict_debug->dict.close = dict_debug_close;
    dict_debug->real_dict = real_dict;
    return (&dict_debug->dict);
}
