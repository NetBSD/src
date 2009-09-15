/*	$NetBSD: mkmap_sdbm.c,v 1.1.1.1.2.2 2009/09/15 06:02:50 snj Exp $	*/

/*++
/* NAME
/*	mkmap_sdbm 3
/* SUMMARY
/*	create or open database, SDBM style
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	MKMAP	*mkmap_sdbm_open(path)
/*	const char *path;
/* DESCRIPTION
/*	This module implements support for creating SDBM databases.
/*
/*	mkmap_sdbm_open() takes a file name, appends the ".dir" and ".pag"
/*	suffixes, and creates or opens the named SDBM database.
/*	This routine is a SDBM-specific helper for the more general
/*	mkmap_open() routine.
/*
/*	All errors are fatal.
/* SEE ALSO
/*	dict_sdbm(3), SDBM dictionary interface.
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
#include <dict_sdbm.h>
#include <myflock.h>

/* Application-specific. */

#include "mkmap.h"

#ifdef HAS_SDBM

#include <sdbm.h>

typedef struct MKMAP_SDBM {
    MKMAP   mkmap;			/* parent class */
    char   *lock_file;			/* path name */
    int     lock_fd;			/* -1 or open locked file */
} MKMAP_SDBM;

/* mkmap_sdbm_after_close - clean up after closing database */

static void mkmap_sdbm_after_close(MKMAP *mp)
{
    MKMAP_SDBM *mkmap = (MKMAP_SDBM *) mp;

    if (mkmap->lock_fd >= 0 && close(mkmap->lock_fd) < 0)
	msg_warn("close %s: %m", mkmap->lock_file);
    myfree(mkmap->lock_file);
}

/* mkmap_sdbm_open - create or open database */

MKMAP  *mkmap_sdbm_open(const char *path)
{
    MKMAP_SDBM *mkmap = (MKMAP_SDBM *) mymalloc(sizeof(*mkmap));
    char   *pag_file;
    int     pag_fd;

    /*
     * Fill in the generic members.
     */
    mkmap->lock_file = concatenate(path, ".dir", (char *) 0);
    mkmap->mkmap.open = dict_sdbm_open;
    mkmap->mkmap.after_open = 0;
    mkmap->mkmap.after_close = mkmap_sdbm_after_close;

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
