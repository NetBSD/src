/*	$NetBSD: mkmap_open.c,v 1.1.1.1 2009/06/23 10:08:47 tron Exp $	*/

/*++
/* NAME
/*	mkmap_open 3
/* SUMMARY
/*	create or rewrite database, generic interface
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	MKMAP	*mkmap_open(type, path, open_flags, dict_flags)
/*	char	*type;
/*	char	*path;
/*	int	open_flags;
/*	int	dict_flags;
/*
/*	void	mkmap_append(mkmap, key, value, lineno)
/*	MKMAP	*mkmap;
/*	char	*key;
/*	char	*value;
/*	int	lineno;
/*
/*	void	mkmap_close(mkmap)
/*	MKMAP	*mkmap;
/* DESCRIPTION
/*	This module implements support for creating Postfix databases.
/*	It is a dict(3) wrapper that adds global locking to dict-level
/*	routines where appropriate.
/*
/*	mkmap_open() creates or truncates the named database, after
/*	appending the appropriate suffixes to the specified filename.
/*	Before the database is updated, it is locked for exclusive
/*	access, and signal delivery is suspended.
/*	See dict(3) for a description of \fBopen_flags\fR and \fBdict_flags\fR.
/*	All errors are fatal.
/*
/*	mkmap_append() appends the named (key, value) pair to the
/*	database. Update errors are fatal; duplicate keys are ignored
/*	(but a warning is issued).
/*	\fBlineno\fR is used for diagnostics.
/*
/*	mkmap_close() closes the database, releases any locks,
/*	and resumes signal delivery. All errors are fatal.
/* SEE ALSO
/*	sigdelay(3) suspend/resume signal delivery
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <dict.h>
#include <dict_db.h>
#include <dict_cdb.h>
#include <dict_dbm.h>
#include <dict_sdbm.h>
#include <dict_proxy.h>
#include <sigdelay.h>
#include <mymalloc.h>

/* Global library. */

#include "mkmap.h"

 /*
  * Information about available database types. Here, we list only those map
  * types that support "create" operations.
  * 
  * We use a different table (in dict_open.c) when querying maps.
  */
typedef struct {
    char   *type;
    MKMAP  *(*before_open) (const char *);
} MKMAP_OPEN_INFO;

static const MKMAP_OPEN_INFO mkmap_types[] = {
    DICT_TYPE_PROXY, mkmap_proxy_open,
#ifdef HAS_CDB
    DICT_TYPE_CDB, mkmap_cdb_open,
#endif
#ifdef HAS_SDBM
    DICT_TYPE_SDBM, mkmap_sdbm_open,
#endif
#ifdef HAS_DBM
    DICT_TYPE_DBM, mkmap_dbm_open,
#endif
#ifdef HAS_DB
    DICT_TYPE_HASH, mkmap_hash_open,
    DICT_TYPE_BTREE, mkmap_btree_open,
#endif
    0,
};

/* mkmap_append - append entry to map */

#undef mkmap_append

void    mkmap_append(MKMAP *mkmap, const char *key, const char *value)
{
    dict_put(mkmap->dict, key, value);
}

/* mkmap_close - close database */

void    mkmap_close(MKMAP *mkmap)
{

    /*
     * Close the database.
     */
    dict_close(mkmap->dict);

    /*
     * Do whatever special processing is needed after closing the database,
     * such as releasing a global exclusive lock on the database file.
     * Individual Postfix dict modules implement locking only for individual
     * record operations, because most Postfix applications don't need global
     * exclusive locks.
     */
    if (mkmap->after_close)
	mkmap->after_close(mkmap);

    /*
     * Resume signal delivery.
     */
    sigresume();

    /*
     * Cleanup.
     */
    myfree((char *) mkmap);
}

/* mkmap_open - create or truncate database */

MKMAP  *mkmap_open(const char *type, const char *path,
		           int open_flags, int dict_flags)
{
    MKMAP  *mkmap;
    const MKMAP_OPEN_INFO *mp;

    /*
     * Find out what map type to use.
     */
    for (mp = mkmap_types; /* void */ ; mp++) {
	if (mp->type == 0)
	    msg_fatal("unsupported map type: %s", type);
	if (strcmp(type, mp->type) == 0)
	    break;
    }
    if (msg_verbose)
	msg_info("open %s %s", type, path);

    /*
     * Do whatever before-open initialization is needed, such as acquiring a
     * global exclusive lock on an existing database file. Individual Postfix
     * dict modules implement locking only for individual record operations,
     * because most Postfix applications don't need global exclusive locks.
     */
    mkmap = mp->before_open(path);

    /*
     * Delay signal delivery, so that we won't leave the database in an
     * inconsistent state if we can avoid it.
     */
    sigdelay();

    /*
     * Truncate the database upon open, and update it. Read-write mode is
     * needed because the underlying routines read as well as write.
     */
    mkmap->dict = mkmap->open(path, open_flags, dict_flags);
    mkmap->dict->lock_fd = -1;			/* XXX just in case */
    mkmap->dict->stat_fd = -1;			/* XXX just in case */
    mkmap->dict->flags |= DICT_FLAG_DUP_WARN;

    /*
     * Do whatever post-open initialization is needed, such as acquiring a
     * global exclusive lock on a database file that did not exist.
     * Individual Postfix dict modules implement locking only for individual
     * record operations, because most Postfix applications don't need global
     * exclusive locks.
     */
    if (mkmap->after_open)
	mkmap->after_open(mkmap);

    return (mkmap);
}
