/*++
/* NAME
/*	mkmap 3
/* SUMMARY
/*	create or open database, DBM style
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	MKMAP	*mkmap_dbm_open(path)
/*	const char *path;
/* DESCRIPTION
/*	This module implements support for creating DBM databases.
/*
/*	mkmap_dbm_open() takes a file name, appends the ".dir" and ".pag"
/*	suffixes, and creates or opens the named DBM database.
/*	This routine is a DBM-specific helper for the more general
/*	mkmap_open() routine.
/*
/*	All errors are fatal.
/* SEE ALSO
/*	dict_dbm(3), DBM dictionary interface.
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
#include <unistd.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <dict.h>
#include <dict_dbm.h>

/* Application-specific. */

#include "mkmap.h"

#ifdef HAS_DBM
#ifdef PATH_NDBM_H
#include PATH_NDBM_H
#else
#include <ndbm.h>
#endif

/* mkmap_dbm_open - create or open database */

MKMAP  *mkmap_dbm_open(const char *path)
{
    MKMAP  *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));
    char   *pag_file;
    int     pag_fd;

    /*
     * Fill in the generic members.
     */
    mkmap->lock_file = concatenate(path, ".dir", (char *) 0);
    mkmap->open = dict_dbm_open;

    /*
     * Unfortunately, not all systems support locking on open(), so we open
     * the .dir and .pag files before truncating them. Keep one file open for
     * locking.
     */
    if ((mkmap->lock_fd = open(mkmap->lock_file, O_CREAT | O_RDWR, 0644)) < 0)
	msg_fatal("open %s: %m", mkmap->lock_file);

    pag_file = concatenate(path, ".pag", (char *) 0);
    if ((pag_fd = open(pag_file, O_CREAT | O_RDWR, 0644)) < 0)
	msg_fatal("open %s: %m", pag_file);
    if (close(pag_fd))
	msg_warn("close %s: %m", pag_file);
    myfree(pag_file);

    return (mkmap);
}

#endif
