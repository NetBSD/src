/*++
/* NAME
/*	dict_nisplus 3
/* SUMMARY
/*	dictionary manager interface to NIS+ maps
/* SYNOPSIS
/*	#include <dict_nisplus.h>
/*
/*	DICT	*dict_nisplus_open(map, dummy, dict_flags)
/*	char	*map;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_nisplus_open() makes the specified NIS+ map accessible via
/*	the generic dictionary operations described in dict_open(3).
/*	The \fIdummy\fR argument is not used.
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

#include "sys_defs.h"
#include <stdio.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "htable.h"
#include "dict.h"
#include "dict_nisplus.h"

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    char   *map;			/* NISPLUS map name */
} DICT_NISPLUS;

/* dict_nisplus_lookup - find table entry */

static const char *dict_nisplus_lookup(DICT *unused_dict, const char *unused_name)
{
    dict_errno = 0;
    msg_warn("dict_nisplus_lookup: NISPLUS lookup not implemented");
    return (0);
}

/* dict_nisplus_update - add or update table entry */

static void dict_nisplus_update(DICT *dict, const char *unused_name, const char *unused_value)
{
    DICT_NISPLUS *dict_nisplus = (DICT_NISPLUS *) dict;

    msg_fatal("dict_nisplus_update: attempt to update NIS+ map %s",
	      dict_nisplus->map);
}

/* dict_nisplus_close - close NISPLUS map */

static void dict_nisplus_close(DICT *dict)
{
    DICT_NISPLUS *dict_nisplus = (DICT_NISPLUS *) dict;

    myfree(dict_nisplus->map);
    myfree((char *) dict_nisplus);
}

/* dict_nisplus_open - open NISPLUS map */

DICT   *dict_nisplus_open(const char *map, int unused_flags, int dict_flags)
{
    DICT_NISPLUS *dict_nisplus;

    dict_nisplus = (DICT_NISPLUS *) mymalloc(sizeof(*dict_nisplus));
    dict_nisplus->dict.lookup = dict_nisplus_lookup;
    dict_nisplus->dict.update = dict_nisplus_update;
    dict_nisplus->dict.close = dict_nisplus_close;
    dict_nisplus->dict.fd = -1;
    dict_nisplus->map = mystrdup(map);
    dict_nisplus->dict.flags = dict_flags | DICT_FLAG_FIXED;
    return (&dict_nisplus->dict);
}
