/*	$NetBSD: mkmap_open.c,v 1.2.2.2 2023/12/25 12:43:38 martin Exp $	*/

/*++
/* NAME
/*	mkmap_open 3
/* SUMMARY
/*	create or rewrite database, generic interface
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	typedef struct MKMAP {
/*	    struct DICT *(*open) (const char *, int, int); /* dict_xx_open() */
/*	    struct DICT *dict;				/* dict_xx_open() result */
/*	    void    (*after_open) (struct MKMAP *);	/* may be null */
/*	    void    (*after_close) (struct MKMAP *);	/* may be null */
/*	    int     multi_writer;			/* multi-writer safe */
/*	} MKMAP;
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
/*	See dict(3) for a description of \fBopen_flags\fR and
/*	\fBdict_flags\fR.  All errors are fatal.
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
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <dict.h>
#include <sigdelay.h>
#include <mymalloc.h>
#include <stringops.h>
#include <mkmap.h>

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
    if (mkmap->multi_writer == 0)
	sigresume();

    /*
     * Cleanup.
     */
    myfree((void *) mkmap);
}

/* mkmap_open - create or truncate database */

MKMAP  *mkmap_open(const char *type, const char *path,
		           int open_flags, int dict_flags)
{
    MKMAP  *mkmap;
    const DICT_OPEN_INFO *dp;

    /*
     * Find out what map type to use.
     */
    if ((dp = dict_open_lookup(type)) == 0)
	msg_fatal("unsupported map type: %s", type);
    if (dp->mkmap_fn == 0)
	msg_fatal("no 'map create' support for this type: %s", type);
    if (msg_verbose)
	msg_info("open %s %s", type, path);

    /*
     * Do whatever before-open initialization is needed, such as acquiring a
     * global exclusive lock on an existing database file. Individual Postfix
     * dict modules implement locking only for individual record operations,
     * because most Postfix applications don't need global exclusive locks.
     */
    mkmap = dp->mkmap_fn(path);

    /*
     * Delay signal delivery, so that we won't leave the database in an
     * inconsistent state if we can avoid it.
     */
    sigdelay();

    /*
     * Truncate the database upon open, and update it. Read-write mode is
     * needed because the underlying routines read as well as write. We
     * explicitly clobber lock_fd to trigger a fatal error when a map wants
     * to unlock the database after individual transactions: that would
     * result in race condition problems. We clobbber stat_fd as well,
     * because that, too, is used only for individual-transaction clients.
     */
    mkmap->dict = mkmap->open(path, open_flags, dict_flags);
    mkmap->dict->lock_fd = -1;			/* XXX just in case */
    mkmap->dict->stat_fd = -1;			/* XXX just in case */
    mkmap->dict->flags |= DICT_FLAG_DUP_WARN;
    mkmap->multi_writer = (mkmap->dict->flags & DICT_FLAG_MULTI_WRITER);

    /*
     * Do whatever post-open initialization is needed, such as acquiring a
     * global exclusive lock on a database file that did not exist.
     * Individual Postfix dict modules implement locking only for individual
     * record operations, because most Postfix applications don't need global
     * exclusive locks.
     */
    if (mkmap->after_open)
	mkmap->after_open(mkmap);

    /*
     * Wrap the dictionary for UTF-8 syntax checks and casefolding.
     */
    if ((mkmap->dict->flags & DICT_FLAG_UTF8_ACTIVE) == 0
	&& DICT_NEED_UTF8_ACTIVATION(util_utf8_enable, dict_flags))
	mkmap->dict = dict_utf8_activate(mkmap->dict);

    /*
     * Resume signal delivery if multi-writer safe.
     */
    if (mkmap->multi_writer)
	sigresume();

    return (mkmap);
}
