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
#include "dict.h"
#include "dict_env.h"

/* dict_env_update - update environment array */

static void dict_env_update(DICT *unused_dict, const char *name, const char *value)
{
    if (setenv(name, value, 1))
	msg_fatal("setenv: %m");
}

/* dict_env_lookup - access environment array */

static const char *dict_env_lookup(DICT *unused_dict, const char *name)
{
    dict_errno = 0;

    return (safe_getenv(name));
}

/* dict_env_close - close environment dictionary */

static void dict_env_close(DICT *dict)
{
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
    return (DICT_DEBUG(dict));
}
