/*	$NetBSD: mkmap_db.c,v 1.2.2.2 2023/12/25 12:43:38 martin Exp $	*/

/*++
/* NAME
/*	mkmap_db 3
/* SUMMARY
/*	create or open database, DB style
/* SYNOPSIS
/*	#include <dict_db.h>
/*
/*	MKMAP	*mkmap_hash_open(path)
/*	const char *path;
/*
/*	MKMAP	*mkmap_btree_open(path)
/*	const char *path;
/* DESCRIPTION
/*	This module implements support for creating DB databases.
/*
/*	mkmap_hash_open() and mkmap_btree_open() take a file name,
/*	append the ".db" suffix, and do whatever initialization is
/*	required before the Berkeley DB open routine is called.
/*
/*	All errors are fatal.
/* SEE ALSO
/*	dict_db(3), DB dictionary interface.
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
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <stringops.h>
#include <dict_db.h>
#include <myflock.h>
#include <warn_stat.h>

#ifdef HAS_DB
#ifdef PATH_DB_H
#include PATH_DB_H
#else
#include <db.h>
#endif

typedef struct MKMAP_DB {
    MKMAP   mkmap;			/* parent class */
    char   *lock_file;			/* path name */
    int     lock_fd;			/* -1 or open locked file */
} MKMAP_DB;

/* mkmap_db_after_close - clean up after closing database */

static void mkmap_db_after_close(MKMAP *mp)
{
    MKMAP_DB *mkmap = (MKMAP_DB *) mp;

    if (mkmap->lock_fd >= 0 && close(mkmap->lock_fd) < 0)
	msg_warn("close %s: %m", mkmap->lock_file);
    myfree(mkmap->lock_file);
}

/* mkmap_db_after_open - lock newly created database */

static void mkmap_db_after_open(MKMAP *mp)
{
    MKMAP_DB *mkmap = (MKMAP_DB *) mp;

    if (mkmap->lock_fd < 0) {
	if ((mkmap->lock_fd = open(mkmap->lock_file, O_RDWR, 0644)) < 0)
	    msg_fatal("open lockfile %s: %m", mkmap->lock_file);
	if (myflock(mkmap->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("lock %s: %m", mkmap->lock_file);
    }
}

/* mkmap_db_before_open - lock existing database */

static MKMAP *mkmap_db_before_open(const char *path,
			          DICT *(*db_open) (const char *, int, int))
{
    MKMAP_DB *mkmap = (MKMAP_DB *) mymalloc(sizeof(*mkmap));
    struct stat st;

    /*
     * Assumes that  dict_db_cache_size = var_db_create_buf was done in the
     * caller, because this code has no access to Postfix variables.
     */

    /*
     * Fill in the generic members.
     */
    mkmap->lock_file = concatenate(path, ".db", (char *) 0);
    mkmap->mkmap.open = db_open;
    mkmap->mkmap.after_open = mkmap_db_after_open;
    mkmap->mkmap.after_close = mkmap_db_after_close;

    /*
     * Unfortunately, not all systems that might support db databases do
     * support locking on open(), so we open the file before updating it.
     * 
     * XXX Berkeley DB 4.1 refuses to open a zero-length file. This means we can
     * open and lock only an existing file, and that we must not truncate it.
     */
    if ((mkmap->lock_fd = open(mkmap->lock_file, O_RDWR, 0644)) < 0) {
	if (errno != ENOENT)
	    msg_fatal("open %s: %m", mkmap->lock_file);
    }

    /*
     * Get an exclusive lock - we're going to change the database so we can't
     * have any spectators.
     * 
     * XXX Horror. Berkeley DB 4.1 refuses to open a zero-length file. This
     * means that we must examine the size while the file is locked, and that
     * we must unlink a zero-length file while it is locked. Avoid a race
     * condition where two processes try to open the same zero-length file
     * and where the second process ends up deleting the wrong file.
     */
    else {
	if (myflock(mkmap->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("lock %s: %m", mkmap->lock_file);
	if (fstat(mkmap->lock_fd, &st) < 0)
	    msg_fatal("fstat %s: %m", mkmap->lock_file);
	if (st.st_size == 0) {
	    if (st.st_nlink > 0) {
		if (unlink(mkmap->lock_file) < 0)
		    msg_fatal("cannot remove zero-length database file %s: %m",
			      mkmap->lock_file);
		msg_warn("removing zero-length database file: %s",
			 mkmap->lock_file);
	    }
	    close(mkmap->lock_fd);
	    mkmap->lock_fd = -1;
	}
    }

    return (&mkmap->mkmap);
}

/* mkmap_hash_open - create or open hashed DB file */

MKMAP  *mkmap_hash_open(const char *path)
{
    return (mkmap_db_before_open(path, dict_hash_open));
}

/* mkmap_btree_open - create or open btree DB file */

MKMAP  *mkmap_btree_open(const char *path)
{
    return (mkmap_db_before_open(path, dict_btree_open));
}

#endif
