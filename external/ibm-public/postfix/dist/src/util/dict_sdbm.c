/*	$NetBSD: dict_sdbm.c,v 1.1.1.2 2013/01/02 18:59:12 tron Exp $	*/

/*++
/* NAME
/*	dict_sdbm 3
/* SUMMARY
/*	dictionary manager interface to SDBM files
/* SYNOPSIS
/*	#include <dict_sdbm.h>
/*
/*	DICT	*dict_sdbm_open(path, open_flags, dict_flags)
/*	const char *name;
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_sdbm_open() opens the named SDBM database and makes it available
/*	via the generic interface described in dict_open(3).
/* DIAGNOSTICS
/*	Fatal errors: cannot open file, file write error, out of memory.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	sdbm(3) data base subroutines
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

#include "sys_defs.h"

/* System library. */

#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#ifdef HAS_SDBM
#include <sdbm.h>
#endif

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <iostuff.h>
#include <vstring.h>
#include <myflock.h>
#include <stringops.h>
#include <dict.h>
#include <dict_sdbm.h>
#include <warn_stat.h>

#ifdef HAS_SDBM

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    SDBM   *dbm;			/* open database */
    VSTRING *key_buf;			/* key buffer */
    VSTRING *val_buf;			/* result buffer */
} DICT_SDBM;

#define SCOPY(buf, data, size) \
    vstring_str(vstring_strncpy(buf ? buf : (buf = vstring_alloc(10)), data, size))

/* dict_sdbm_lookup - find database entry */

static const char *dict_sdbm_lookup(DICT *dict, const char *name)
{
    DICT_SDBM *dict_sdbm = (DICT_SDBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    const char *result = 0;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_sdbm_lookup: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_sdbm->dict.name);

    /*
     * See if this DBM file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name) + 1;
	dbm_value = sdbm_fetch(dict_sdbm->dbm, dbm_key);
	if (dbm_value.dptr != 0) {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
	    result = SCOPY(dict_sdbm->val_buf, dbm_value.dptr, dbm_value.dsize);
	}
    }

    /*
     * See if this DBM file was written with no null byte appended to key and
     * value.
     */
    if (result == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name);
	dbm_value = sdbm_fetch(dict_sdbm->dbm, dbm_key);
	if (dbm_value.dptr != 0) {
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
	    result = SCOPY(dict_sdbm->val_buf, dbm_value.dptr, dbm_value.dsize);
	}
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_sdbm->dict.name);

    return (result);
}

/* dict_sdbm_update - add or update database entry */

static int dict_sdbm_update(DICT *dict, const char *name, const char *value)
{
    DICT_SDBM *dict_sdbm = (DICT_SDBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    int     status;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_sdbm_update: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    dbm_key.dptr = (void *) name;
    dbm_value.dptr = (void *) value;
    dbm_key.dsize = strlen(name);
    dbm_value.dsize = strlen(value);

    /*
     * If undecided about appending a null byte to key and value, choose a
     * default depending on the platform.
     */
    if ((dict->flags & DICT_FLAG_TRY1NULL)
	&& (dict->flags & DICT_FLAG_TRY0NULL)) {
#ifdef DBM_NO_TRAILING_NULL
	dict->flags &= ~DICT_FLAG_TRY1NULL;
#else
	dict->flags &= ~DICT_FLAG_TRY0NULL;
#endif
    }

    /*
     * Optionally append a null byte to key and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	dbm_key.dsize++;
	dbm_value.dsize++;
    }

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_sdbm->dict.name);

    /*
     * Do the update.
     */
    if ((status = sdbm_store(dict_sdbm->dbm, dbm_key, dbm_value,
     (dict->flags & DICT_FLAG_DUP_REPLACE) ? DBM_REPLACE : DBM_INSERT)) < 0)
	msg_fatal("error writing SDBM database %s: %m", dict_sdbm->dict.name);
    if (status) {
	if (dict->flags & DICT_FLAG_DUP_IGNORE)
	     /* void */ ;
	else if (dict->flags & DICT_FLAG_DUP_WARN)
	    msg_warn("%s: duplicate entry: \"%s\"", dict_sdbm->dict.name, name);
	else
	    msg_fatal("%s: duplicate entry: \"%s\"", dict_sdbm->dict.name, name);
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_sdbm->dict.name);

    return (status);
}

/* dict_sdbm_delete - delete one entry from the dictionary */

static int dict_sdbm_delete(DICT *dict, const char *name)
{
    DICT_SDBM *dict_sdbm = (DICT_SDBM *) dict;
    datum   dbm_key;
    int     status = 1;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_sdbm_delete: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_sdbm->dict.name);

    /*
     * See if this DBM file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name) + 1;
	sdbm_clearerr(dict_sdbm->dbm);
	if ((status = sdbm_delete(dict_sdbm->dbm, dbm_key)) < 0) {
	    if (sdbm_error(dict_sdbm->dbm) != 0)/* fatal error */
		msg_fatal("error deleting from %s: %m", dict_sdbm->dict.name);
	    status = 1;				/* not found */
	} else {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;	/* found */
	}
    }

    /*
     * See if this DBM file was written with no null byte appended to key and
     * value.
     */
    if (status > 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name);
	sdbm_clearerr(dict_sdbm->dbm);
	if ((status = sdbm_delete(dict_sdbm->dbm, dbm_key)) < 0) {
	    if (sdbm_error(dict_sdbm->dbm) != 0)/* fatal error */
		msg_fatal("error deleting from %s: %m", dict_sdbm->dict.name);
	    status = 1;				/* not found */
	} else {
	    dict->flags &= ~DICT_FLAG_TRY1NULL;	/* found */
	}
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_sdbm->dict.name);

    return (status);
}

/* traverse the dictionary */

static int dict_sdbm_sequence(DICT *dict, const int function,
			              const char **key, const char **value)
{
    const char *myname = "dict_sdbm_sequence";
    DICT_SDBM *dict_sdbm = (DICT_SDBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    int     status;

    dict->error = 0;

    /*
     * Acquire a shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_sdbm->dict.name);

    /*
     * Determine and execute the seek function. It returns the key.
     */
    sdbm_clearerr(dict_sdbm->dbm);
    switch (function) {
    case DICT_SEQ_FUN_FIRST:
	dbm_key = sdbm_firstkey(dict_sdbm->dbm);
	break;
    case DICT_SEQ_FUN_NEXT:
	dbm_key = sdbm_nextkey(dict_sdbm->dbm);
	break;
    default:
	msg_panic("%s: invalid function: %d", myname, function);
    }

    if (dbm_key.dptr != 0 && dbm_key.dsize > 0) {

	/*
	 * Copy the key so that it is guaranteed null terminated.
	 */
	*key = SCOPY(dict_sdbm->key_buf, dbm_key.dptr, dbm_key.dsize);

	/*
	 * Fetch the corresponding value.
	 */
	dbm_value = sdbm_fetch(dict_sdbm->dbm, dbm_key);

	if (dbm_value.dptr != 0 && dbm_value.dsize > 0) {

	    /*
	     * Copy the value so that it is guaranteed null terminated.
	     */
	    *value = SCOPY(dict_sdbm->val_buf, dbm_value.dptr, dbm_value.dsize);
	    status = 0;
	} else {

	    /*
	     * Determine if we have hit the last record or an error
	     * condition.
	     */
	    if (sdbm_error(dict_sdbm->dbm))
		msg_fatal("error seeking %s: %m", dict_sdbm->dict.name);
	    status = 1;				/* no error: eof/not found
						 * (should not happen!) */
	}
    } else {

	/*
	 * Determine if we have hit the last record or an error condition.
	 */
	if (sdbm_error(dict_sdbm->dbm))
	    msg_fatal("error seeking %s: %m", dict_sdbm->dict.name);
	status = 1;				/* no error: eof/not found */
    }

    /*
     * Release the shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_sdbm->dict.name);

    return (status);
}

/* dict_sdbm_close - disassociate from data base */

static void dict_sdbm_close(DICT *dict)
{
    DICT_SDBM *dict_sdbm = (DICT_SDBM *) dict;

    sdbm_close(dict_sdbm->dbm);
    if (dict_sdbm->key_buf)
	vstring_free(dict_sdbm->key_buf);
    if (dict_sdbm->val_buf)
	vstring_free(dict_sdbm->val_buf);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_sdbm_open - open SDBM data base */

DICT   *dict_sdbm_open(const char *path, int open_flags, int dict_flags)
{
    DICT_SDBM *dict_sdbm;
    struct stat st;
    SDBM   *dbm;
    char   *dbm_path;
    int     lock_fd;

    /*
     * Note: DICT_FLAG_LOCK is used only by programs that do fine-grained (in
     * the time domain) locking while accessing individual database records.
     * 
     * Programs such as postmap/postalias use their own large-grained (in the
     * time domain) locks while rewriting the entire file.
     */
    if (dict_flags & DICT_FLAG_LOCK) {
	dbm_path = concatenate(path, ".dir", (char *) 0);
	if ((lock_fd = open(dbm_path, open_flags, 0644)) < 0)
	    msg_fatal("open database %s: %m", dbm_path);
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	    msg_fatal("shared-lock database %s for open: %m", dbm_path);
    }

    /*
     * XXX sdbm_open() has no const in prototype.
     */
    if ((dbm = sdbm_open((char *) path, open_flags, 0644)) == 0)
	msg_fatal("open database %s.{dir,pag}: %m", path);

    if (dict_flags & DICT_FLAG_LOCK) {
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	    msg_fatal("unlock database %s for open: %m", dbm_path);
	if (close(lock_fd) < 0)
	    msg_fatal("close database %s: %m", dbm_path);
    }
    dict_sdbm = (DICT_SDBM *) dict_alloc(DICT_TYPE_SDBM, path, sizeof(*dict_sdbm));
    dict_sdbm->dict.lookup = dict_sdbm_lookup;
    dict_sdbm->dict.update = dict_sdbm_update;
    dict_sdbm->dict.delete = dict_sdbm_delete;
    dict_sdbm->dict.sequence = dict_sdbm_sequence;
    dict_sdbm->dict.close = dict_sdbm_close;
    dict_sdbm->dict.lock_fd = sdbm_dirfno(dbm);
    dict_sdbm->dict.stat_fd = sdbm_pagfno(dbm);
    if (fstat(dict_sdbm->dict.stat_fd, &st) < 0)
	msg_fatal("dict_sdbm_open: fstat: %m");
    dict_sdbm->dict.mtime = st.st_mtime;
    dict_sdbm->dict.owner.uid = st.st_uid;
    dict_sdbm->dict.owner.status = (st.st_uid != 0);

    /*
     * Warn if the source file is newer than the indexed file, except when
     * the source file changed only seconds ago.
     */
    if ((dict_flags & DICT_FLAG_LOCK) != 0
	&& stat(path, &st) == 0
	&& st.st_mtime > dict_sdbm->dict.mtime
	&& st.st_mtime < time((time_t *) 0) - 100)
	msg_warn("database %s is older than source file %s", dbm_path, path);

    close_on_exec(sdbm_pagfno(dbm), CLOSE_ON_EXEC);
    close_on_exec(sdbm_dirfno(dbm), CLOSE_ON_EXEC);
    dict_sdbm->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if ((dict_flags & (DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL)) == 0)
	dict_sdbm->dict.flags |= (DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL);
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_sdbm->dict.fold_buf = vstring_alloc(10);
    dict_sdbm->dbm = dbm;
    dict_sdbm->key_buf = 0;
    dict_sdbm->val_buf = 0;

    if ((dict_flags & DICT_FLAG_LOCK))
	myfree(dbm_path);

    return (DICT_DEBUG (&dict_sdbm->dict));
}

#endif
