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
#include <dict_dbm.h>
#include <sigdelay.h>
#include <mymalloc.h>
#include <myflock.h>

/* Global library. */

#include "mkmap.h"

 /*
  * Information about available database types. Here, we list only those map
  * types that exist as files. Network-based maps are not of interest.
  */
typedef struct {
    char   *type;
    MKMAP  *(*create_or_open) (const char *);
} MKMAP_OPEN_INFO;

MKMAP_OPEN_INFO mkmap_types[] = {
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

void    mkmap_append(MKMAP *mkmap, const char *key, const char *value)
{
    dict_put(mkmap->dict, key, value);
}

/* mkmap_close - close database */

void    mkmap_close(MKMAP *mkmap)
{

    /*
     * Close the database and the locking file descriptor.
     */
    dict_close(mkmap->dict);
    if (close(mkmap->lock_fd) < 0)
	msg_warn("close %s: %m", mkmap->lock_file);

    /*
     * Resume signal delivery.
     */
    sigresume();

    /*
     * Cleanup.
     */
    myfree(mkmap->lock_file);
    myfree((char *) mkmap);
}

/* mkmap_open - create or truncate database */

MKMAP  *mkmap_open(const char *type, const char *path,
		           int open_flags, int dict_flags)
{
    MKMAP  *mkmap;
    MKMAP_OPEN_INFO *mp;

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
     * Create or open the desired map file(s).
     */
    mkmap = mp->create_or_open(path);

    /*
     * Get an exclusive lock - we're going to change the database so we can't
     * have any spectators.
     */
    if (myflock(mkmap->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("lock %s: %m", mkmap->lock_file);

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
    return (mkmap);
}
