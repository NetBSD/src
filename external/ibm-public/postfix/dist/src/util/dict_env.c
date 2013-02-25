/*	$NetBSD: dict_env.c,v 1.1.1.1.16.1 2013/02/25 00:27:31 tls Exp $	*/

/*++
/* NAME
/*	dict_env 3
/* SUMMARY
/*	dictionary manager interface to environment variables
/* SYNOPSIS
/*	#include <dict_env.h>
/*
/*	DICT	*dict_env_open(name, dummy, dict_flags)
/*	const char *name;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_env_open() opens the environment variable array and
/*	makes it accessible via the generic operations documented
/*	in dict_open(3). The \fIname\fR and \fIdummy\fR arguments
/*	are ignored.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	safe_getenv(3) safe getenv() interface
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

#include "sys_defs.h"
#include <stdio.h>			/* sprintf() prototype */
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include "mymalloc.h"
#include "msg.h"
#include "safe.h"
#include "stringops.h"
#include "dict.h"
#include "dict_env.h"

/* dict_env_update - update environment array */

static int dict_env_update(DICT *dict, const char *name, const char *value)
{
    dict->error = 0;

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    if (setenv(name, value, 1))
	msg_fatal("setenv: %m");

    return (DICT_STAT_SUCCESS);
}

/* dict_env_lookup - access environment array */

static const char *dict_env_lookup(DICT *dict, const char *name)
{
    dict->error = 0;

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    return (safe_getenv(name));
}

/* dict_env_close - close environment dictionary */

static void dict_env_close(DICT *dict)
{
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_env_open - make association with environment array */

DICT   *dict_env_open(const char *name, int unused_flags, int dict_flags)
{
    DICT   *dict;

    dict = dict_alloc(DICT_TYPE_ENVIRON, name, sizeof(*dict));
    dict->lookup = dict_env_lookup;
    dict->update = dict_env_update;
    dict->close = dict_env_close;
    dict->flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict->fold_buf = vstring_alloc(10);
    dict->owner.status = DICT_OWNER_TRUSTED;
    return (DICT_DEBUG (dict));
}
