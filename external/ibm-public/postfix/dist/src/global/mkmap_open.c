/*	$NetBSD: mkmap_open.c,v 1.1.1.3.10.1 2017/04/21 16:52:48 bouyer Exp $	*/

/*++
/* NAME
/*	mkmap_open 3
/* SUMMARY
/*	create or rewrite database, generic interface
/* SYNOPSIS
/*	#include <mkmap.h>
/*
/*	typedef struct MKMAP {
/*	    DICT_OPEN_FN open;				/* dict_xx_open() */
/*	    DICT *dict;					/* dict_xx_open() result */
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
/*
/*	typedef MKMAP *(*MKMAP_OPEN_FN) (const char *);
/*	typedef MKMAP_OPEN_FN *(*MKMAP_OPEN_EXTEND_FN) (const char *);
/*
/*	void	mkmap_open_register(type, open_fn)
/*	const char *type;
/*	MKMAP_OPEN_FN open_fn;
/*
/*	MKMAP_OPEN_EXTEND_FN mkmap_open_extend(call_back)
/*	MKMAP_OPEN_EXTEND_FN call_back;
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
/*
/*	mkmap_open_register() adds support for a new database type.
/*
/*	mkmap_open_extend() registers a call-back function that looks
/*	up the mkmap open() function for a database type that is not
/*	registered, or null in case of error. The result value is the
/*	last previously-registered call-back or null. A mkmap open()
/*	function is cached after it is looked up through this extension
/*	mechanism.
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
#include <htable.h>
#include <dict.h>
#include <dict_db.h>
#include <dict_cdb.h>
#include <dict_dbm.h>
#include <dict_lmdb.h>
#include <dict_sdbm.h>
#include <dict_proxy.h>
#include <dict_fail.h>
#include <sigdelay.h>
#include <mymalloc.h>
#include <stringops.h>

/* Global library. */

#include "mkmap.h"

 /*
  * Information about available database types. Here, we list only those map
  * types that support "bulk create" operations.
  * 
  * We use a different table (in dict_open.c and mail_dict.c) when querying maps
  * or when making incremental updates.
  */
typedef struct {
    const char *type;
    MKMAP_OPEN_FN before_open;
} MKMAP_OPEN_INFO;

static const MKMAP_OPEN_INFO mkmap_open_info[] = {
#ifndef USE_DYNAMIC_MAPS
#ifdef HAS_CDB
    DICT_TYPE_CDB, mkmap_cdb_open,
#endif
#ifdef HAS_SDBM
    DICT_TYPE_SDBM, mkmap_sdbm_open,
#endif
#ifdef HAS_LMDB
    DICT_TYPE_LMDB, mkmap_lmdb_open,
#endif
#endif					/* !USE_DYNAMIC_MAPS */
#ifdef HAS_DBM
    DICT_TYPE_DBM, mkmap_dbm_open,
#endif
#ifdef HAS_DB
    DICT_TYPE_HASH, mkmap_hash_open,
    DICT_TYPE_BTREE, mkmap_btree_open,
#endif
    DICT_TYPE_FAIL, mkmap_fail_open,
    0,
};

static HTABLE *mkmap_open_hash;

static MKMAP_OPEN_EXTEND_FN mkmap_open_extend_hook = 0;

/* mkmap_open_init - one-off initialization */

static void mkmap_open_init(void)
{
    static const char myname[] = "mkmap_open_init";
    const MKMAP_OPEN_INFO *mp;

    if (mkmap_open_hash != 0)
	msg_panic("%s: multiple initialization", myname);
    mkmap_open_hash = htable_create(10);

    for (mp = mkmap_open_info; mp->type; mp++)
	htable_enter(mkmap_open_hash, mp->type, (void *) mp);
}

/* mkmap_open_register - register dictionary type */

void    mkmap_open_register(const char *type, MKMAP_OPEN_FN open_fn)
{
    static const char myname[] = "mkmap_open_register";
    MKMAP_OPEN_INFO *mp;
    HTABLE_INFO *ht;

    if (mkmap_open_hash == 0)
	mkmap_open_init();
    if (htable_find(mkmap_open_hash, type))
	msg_panic("%s: database type exists: %s", myname, type);
    mp = (MKMAP_OPEN_INFO *) mymalloc(sizeof(*mp));
    mp->before_open = open_fn;
    ht = htable_enter(mkmap_open_hash, type, (void *) mp);
    mp->type = ht->key;
}

/* mkmap_open_extend - register alternate lookup function */

MKMAP_OPEN_EXTEND_FN mkmap_open_extend(MKMAP_OPEN_EXTEND_FN new_cb)
{
    MKMAP_OPEN_EXTEND_FN old_cb;

    old_cb = mkmap_open_extend_hook;
    mkmap_open_extend_hook = new_cb;
    return (old_cb);
}

/* mkmap_append - append entry to map */

#undef mkmap_append

void    mkmap_append(MKMAP *mkmap, const char *key, const char *value)
{
    DICT   *dict = mkmap->dict;

    if (dict_put(dict, key, value) != 0 && dict->error != 0)
	msg_fatal("%s:%s: update failed", dict->type, dict->name);
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
    const MKMAP_OPEN_INFO *mp;
    MKMAP_OPEN_FN open_fn;

    /*
     * Find out what map type to use.
     */
    if (mkmap_open_hash == 0)
	mkmap_open_init();
    if ((mp = (MKMAP_OPEN_INFO *) htable_find(mkmap_open_hash, type)) == 0) {
	if (mkmap_open_extend_hook != 0 &&
	    (open_fn = mkmap_open_extend_hook(type)) != 0) {
	    mkmap_open_register(type, open_fn);
	    mp = (MKMAP_OPEN_INFO *) htable_find(mkmap_open_hash, type);
	}
	if (mp == 0)
	    msg_fatal("unsupported map type for this operation: %s", type);
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
