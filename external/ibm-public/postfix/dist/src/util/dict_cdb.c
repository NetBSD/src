/*	$NetBSD: dict_cdb.c,v 1.1.1.2.6.1 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	dict_cdb 3
/* SUMMARY
/*	dictionary manager interface to CDB files
/* SYNOPSIS
/*	#include <dict_cdb.h>
/*
/*	DICT	*dict_cdb_open(path, open_flags, dict_flags)
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/*
/* DESCRIPTION
/*	dict_cdb_open() opens the specified CDB database.  The result is
/*	a pointer to a structure that can be used to access the dictionary
/*	using the generic methods documented in dict_open(3).
/*
/*	Arguments:
/* .IP path
/*	The database pathname, not including the ".cdb" suffix.
/* .IP open_flags
/*	Flags passed to open(). Specify O_RDONLY or O_WRONLY|O_CREAT|O_TRUNC.
/* .IP dict_flags
/*	Flags used by the dictionary interface.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* DIAGNOSTICS
/*	Fatal errors: cannot open file, write error, out of memory.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Michael Tokarev <mjt@tls.msk.ru> based on dict_db.c by
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#include "sys_defs.h"

/* System library. */

#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "vstring.h"
#include "stringops.h"
#include "iostuff.h"
#include "myflock.h"
#include "stringops.h"
#include "dict.h"
#include "dict_cdb.h"
#include "warn_stat.h"

#ifdef HAS_CDB

#include <cdb.h>
#ifndef TINYCDB_VERSION
#include <cdb_make.h>
#endif
#ifndef cdb_fileno
#define cdb_fileno(c) ((c)->fd)
#endif

#ifndef CDB_SUFFIX
#define CDB_SUFFIX ".cdb"
#endif
#ifndef CDB_TMP_SUFFIX
#define CDB_TMP_SUFFIX CDB_SUFFIX ".tmp"
#endif

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    struct cdb cdb;			/* cdb structure */
} DICT_CDBQ;				/* query interface */

typedef struct {
    DICT    dict;			/* generic members */
    struct cdb_make cdbm;		/* cdb_make structure */
    char   *cdb_path;			/* cdb pathname (.cdb) */
    char   *tmp_path;			/* temporary pathname (.tmp) */
} DICT_CDBM;				/* rebuild interface */

/* dict_cdbq_lookup - find database entry, query mode */

static const char *dict_cdbq_lookup(DICT *dict, const char *name)
{
    DICT_CDBQ *dict_cdbq = (DICT_CDBQ *) dict;
    unsigned vlen;
    int     status = 0;
    static char *buf;
    static unsigned len;
    const char *result = 0;

    dict->error = 0;

    /* CDB is constant, so do not try to acquire a lock. */

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }

    /*
     * See if this CDB file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	status = cdb_find(&dict_cdbq->cdb, name, strlen(name) + 1);
	if (status > 0)
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
    }

    /*
     * See if this CDB file was written with no null byte appended to key and
     * value.
     */
    if (status == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	status = cdb_find(&dict_cdbq->cdb, name, strlen(name));
	if (status > 0)
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
    }
    if (status < 0)
	msg_fatal("error reading %s: %m", dict->name);

    if (status) {
	vlen = cdb_datalen(&dict_cdbq->cdb);
	if (len < vlen) {
	    if (buf == 0)
		buf = mymalloc(vlen + 1);
	    else
		buf = myrealloc(buf, vlen + 1);
	    len = vlen;
	}
	if (cdb_read(&dict_cdbq->cdb, buf, vlen,
		     cdb_datapos(&dict_cdbq->cdb)) < 0)
	    msg_fatal("error reading %s: %m", dict->name);
	buf[vlen] = '\0';
	result = buf;
    }
    /* No locking so not release the lock.  */

    return (result);
}

/* dict_cdbq_close - close data base, query mode */

static void dict_cdbq_close(DICT *dict)
{
    DICT_CDBQ *dict_cdbq = (DICT_CDBQ *) dict;

    cdb_free(&dict_cdbq->cdb);
    close(dict->stat_fd);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_cdbq_open - open data base, query mode */

static DICT *dict_cdbq_open(const char *path, int dict_flags)
{
    DICT_CDBQ *dict_cdbq;
    struct stat st;
    char   *cdb_path;
    int     fd;

    /*
     * Let the optimizer worry about eliminating redundant code.
     */
#define DICT_CDBQ_OPEN_RETURN(d) { \
	DICT *__d = (d); \
	myfree(cdb_path); \
	return (__d); \
    } while (0)

    cdb_path = concatenate(path, CDB_SUFFIX, (char *) 0);

    if ((fd = open(cdb_path, O_RDONLY)) < 0)
	DICT_CDBQ_OPEN_RETURN(dict_surrogate(DICT_TYPE_CDB, path,
					   O_RDONLY, dict_flags,
					 "open database %s: %m", cdb_path));

    dict_cdbq = (DICT_CDBQ *) dict_alloc(DICT_TYPE_CDB,
					 cdb_path, sizeof(*dict_cdbq));
#if defined(TINYCDB_VERSION)
    if (cdb_init(&(dict_cdbq->cdb), fd) != 0)
	msg_fatal("dict_cdbq_open: unable to init %s: %m", cdb_path);
#else
    cdb_init(&(dict_cdbq->cdb), fd);
#endif
    dict_cdbq->dict.lookup = dict_cdbq_lookup;
    dict_cdbq->dict.close = dict_cdbq_close;
    dict_cdbq->dict.stat_fd = fd;
    if (fstat(fd, &st) < 0)
	msg_fatal("dict_dbq_open: fstat: %m");
    dict_cdbq->dict.mtime = st.st_mtime;
    dict_cdbq->dict.owner.uid = st.st_uid;
    dict_cdbq->dict.owner.status = (st.st_uid != 0);
    close_on_exec(fd, CLOSE_ON_EXEC);

    /*
     * Warn if the source file is newer than the indexed file, except when
     * the source file changed only seconds ago.
     */
    if (stat(path, &st) == 0
	&& st.st_mtime > dict_cdbq->dict.mtime
	&& st.st_mtime < time((time_t *) 0) - 100)
	msg_warn("database %s is older than source file %s", cdb_path, path);

    /*
     * If undecided about appending a null byte to key and value, choose to
     * try both in query mode.
     */
    if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	dict_flags |= DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL;
    dict_cdbq->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_cdbq->dict.fold_buf = vstring_alloc(10);

    DICT_CDBQ_OPEN_RETURN(DICT_DEBUG (&dict_cdbq->dict));
}

/* dict_cdbm_update - add database entry, create mode */

static int dict_cdbm_update(DICT *dict, const char *name, const char *value)
{
    DICT_CDBM *dict_cdbm = (DICT_CDBM *) dict;
    unsigned ksize, vsize;
    int     r;

    dict->error = 0;

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    ksize = strlen(name);
    vsize = strlen(value);

    /*
     * Optionally append a null byte to key and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	ksize++;
	vsize++;
    }

    /*
     * Do the add operation.  No locking is done.
     */
#ifdef TINYCDB_VERSION
#ifndef CDB_PUT_ADD
#error please upgrate tinycdb to at least 0.5 version
#endif
    if (dict->flags & DICT_FLAG_DUP_IGNORE)
	r = CDB_PUT_ADD;
    else if (dict->flags & DICT_FLAG_DUP_REPLACE)
	r = CDB_PUT_REPLACE;
    else
	r = CDB_PUT_INSERT;
    r = cdb_make_put(&dict_cdbm->cdbm, name, ksize, value, vsize, r);
    if (r < 0)
	msg_fatal("error writing %s: %m", dict_cdbm->tmp_path);
    else if (r > 0) {
	if (dict->flags & (DICT_FLAG_DUP_IGNORE | DICT_FLAG_DUP_REPLACE))
	     /* void */ ;
	else if (dict->flags & DICT_FLAG_DUP_WARN)
	    msg_warn("%s: duplicate entry: \"%s\"",
		     dict_cdbm->dict.name, name);
	else
	    msg_fatal("%s: duplicate entry: \"%s\"",
		      dict_cdbm->dict.name, name);
    }
    return (r);
#else
    if (cdb_make_add(&dict_cdbm->cdbm, name, ksize, value, vsize) < 0)
	msg_fatal("error writing %s: %m", dict_cdbm->tmp_path);
    return (0);
#endif
}

/* dict_cdbm_close - close data base and rename file.tmp to file.cdb */

static void dict_cdbm_close(DICT *dict)
{
    DICT_CDBM *dict_cdbm = (DICT_CDBM *) dict;
    int     fd = cdb_fileno(&dict_cdbm->cdbm);

    /*
     * Note: if FCNTL locking is used, closing any file descriptor on a
     * locked file cancels all locks that the process may have on that file.
     * CDB is FCNTL locking safe, because it uses the same file descriptor
     * for database I/O and locking.
     */
    if (cdb_make_finish(&dict_cdbm->cdbm) < 0)
	msg_fatal("finish database %s: %m", dict_cdbm->tmp_path);
    if (rename(dict_cdbm->tmp_path, dict_cdbm->cdb_path) < 0)
	msg_fatal("rename database from %s to %s: %m",
		  dict_cdbm->tmp_path, dict_cdbm->cdb_path);
    if (close(fd) < 0)				/* releases a lock */
	msg_fatal("close database %s: %m", dict_cdbm->cdb_path);
    myfree(dict_cdbm->cdb_path);
    myfree(dict_cdbm->tmp_path);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_cdbm_open - create database as file.tmp */

static DICT *dict_cdbm_open(const char *path, int dict_flags)
{
    DICT_CDBM *dict_cdbm;
    char   *cdb_path;
    char   *tmp_path;
    int     fd;
    struct stat st0, st1;

    /*
     * Let the optimizer worry about eliminating redundant code.
     */
#define DICT_CDBM_OPEN_RETURN(d) { \
	DICT *__d = (d); \
	if (cdb_path) \
	    myfree(cdb_path); \
	if (tmp_path) \
	    myfree(tmp_path); \
	return (__d); \
    } while (0)

    cdb_path = concatenate(path, CDB_SUFFIX, (char *) 0);
    tmp_path = concatenate(path, CDB_TMP_SUFFIX, (char *) 0);

    /*
     * Repeat until we have opened *and* locked *existing* file. Since the
     * new (tmp) file will be renamed to be .cdb file, locking here is
     * somewhat funny to work around possible race conditions.  Note that we
     * can't open a file with O_TRUNC as we can't know if another process
     * isn't creating it at the same time.
     */
    for (;;) {
	if ((fd = open(tmp_path, O_RDWR | O_CREAT, 0644)) < 0)
	    DICT_CDBM_OPEN_RETURN(dict_surrogate(DICT_TYPE_CDB, path,
						 O_RDWR, dict_flags,
						 "open database %s: %m",
						 tmp_path));
	if (fstat(fd, &st0) < 0)
	    msg_fatal("fstat(%s): %m", tmp_path);

	/*
	 * Get an exclusive lock - we're going to change the database so we
	 * can't have any spectators.
	 */
	if (myflock(fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("lock %s: %m", tmp_path);

	if (stat(tmp_path, &st1) < 0)
	    msg_fatal("stat(%s): %m", tmp_path);

	/*
	 * Compare file's state before and after lock: should be the same,
	 * and nlinks should be >0, or else we opened non-existing file...
	 */
	if (st0.st_ino == st1.st_ino && st0.st_dev == st1.st_dev
	    && st0.st_rdev == st1.st_rdev && st0.st_nlink == st1.st_nlink
	    && st0.st_nlink > 0)
	    break;				/* successefully opened */

	close(fd);

    }

#ifndef NO_FTRUNCATE
    if (st0.st_size)
	ftruncate(fd, 0);
#endif

    dict_cdbm = (DICT_CDBM *) dict_alloc(DICT_TYPE_CDB, path,
					 sizeof(*dict_cdbm));
    if (cdb_make_start(&dict_cdbm->cdbm, fd) < 0)
	msg_fatal("initialize database %s: %m", tmp_path);
    dict_cdbm->dict.close = dict_cdbm_close;
    dict_cdbm->dict.update = dict_cdbm_update;
    dict_cdbm->cdb_path = cdb_path;
    dict_cdbm->tmp_path = tmp_path;
    cdb_path = tmp_path = 0;			/* DICT_CDBM_OPEN_RETURN() */
    dict_cdbm->dict.owner.uid = st1.st_uid;
    dict_cdbm->dict.owner.status = (st1.st_uid != 0);
    close_on_exec(fd, CLOSE_ON_EXEC);

    /*
     * If undecided about appending a null byte to key and value, choose a
     * default to not append a null byte when creating a cdb.
     */
    if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	dict_flags |= DICT_FLAG_TRY0NULL;
    else if ((dict_flags & DICT_FLAG_TRY1NULL)
	     && (dict_flags & DICT_FLAG_TRY0NULL))
	dict_flags &= ~DICT_FLAG_TRY0NULL;
    dict_cdbm->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_cdbm->dict.fold_buf = vstring_alloc(10);

    DICT_CDBM_OPEN_RETURN(DICT_DEBUG (&dict_cdbm->dict));
}

/* dict_cdb_open - open data base for query mode or create mode */

DICT   *dict_cdb_open(const char *path, int open_flags, int dict_flags)
{
    switch (open_flags & (O_RDONLY | O_RDWR | O_WRONLY | O_CREAT | O_TRUNC)) {
	case O_RDONLY:			/* query mode */
	return dict_cdbq_open(path, dict_flags);
    case O_WRONLY | O_CREAT | O_TRUNC:		/* create mode */
    case O_RDWR | O_CREAT | O_TRUNC:		/* sloppiness */
	return dict_cdbm_open(path, dict_flags);
    default:
	msg_fatal("dict_cdb_open: inappropriate open flags for cdb database"
		  " - specify O_RDONLY or O_WRONLY|O_CREAT|O_TRUNC");
    }
}

#endif					/* HAS_CDB */
