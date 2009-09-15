/*	$NetBSD: mkmap_dbm.c,v 1.1.1.1.2.2 2009/09/15 06:02:49 snj Exp $	*/

/*++
/* NAME
/*	mkmap_dbm 3
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
#include <myflock.h>

/* Application-specific. */

#include "mkmap.h"

#ifdef HAS_DBM
#ifdef PATH_NDBM_H
#include PATH_NDBM_H
#else
#include <ndbm.h>
#endif

typedef struct MKMAP_DBM {
    MKMAP   mkmap;			/* parent class */
    char   *lock_file;			/* path name */
    int     lock_fd;			/* -1 or open locked file */
} MKMAP_DBM;

/* mkmap_dbm_after_close - clean up after closing database */

static void mkmap_dbm_after_close(MKMAP *mp)
{
    MKMAP_DBM *mkmap = (MKMAP_DBM *) mp;

    if (mkmap->lock_fd >= 0 && close(mkmap->lock_fd) < 0)
	msg_warn("close %s: %m", mkmap->lock_file);
    myfree(mkmap->lock_file);
}

/* mkmap_dbm_open - create or open database */

MKMAP  *mkmap_dbm_open(const char *path)
{
    MKMAP_DBM *mkmap = (MKMAP_DBM *) mymalloc(sizeof(*mkmap));
    char   *pag_file;
    int     pag_fd;

    /*
     * Fill in the generic members.
     */
    mkmap->lock_file = concatenate(path, ".dir", (char *) 0);
    mkmap->mkmap.open = dict_dbm_open;
    mkmap->mkmap.after_open = 0;
    mkmap->mkmap.after_close = mkmap_dbm_after_close;

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

    /*
     * Get an exclusive lock - we're going to change the database so we can't
     * have any spectators.
     */
    if (myflock(mkmap->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("lock %s: %m", mkmap->lock_file);

    return (&mkmap->mkmap);
}

#endif
