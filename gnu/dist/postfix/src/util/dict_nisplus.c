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
} DICT_NISPLUS;

/* dict_nisplus_close - close NISPLUS map */

static void dict_nisplus_close(DICT *dict)
{
    DICT_NISPLUS *dict_nisplus = (DICT_NISPLUS *) dict;

    myfree((char *) dict_nisplus);
}

/* dict_nisplus_open - open NISPLUS map */

DICT   *dict_nisplus_open(const char *map, int unused_flags, int dict_flags)
{
    DICT_NISPLUS *dict_nisplus;

    dict_nisplus = (DICT_NISPLUS *) dict_alloc(DICT_TYPE_NISPLUS, map,
					       sizeof(*dict_nisplus));
    dict_nisplus->dict.close = dict_nisplus_close;
    dict_nisplus->dict.flags = dict_flags | DICT_FLAG_FIXED;
    return (DICT_DEBUG(&dict_nisplus->dict));
}
