/*	$NetBSD: dict_dbm.c,v 1.1.1.3 2013/01/02 18:59:12 tron Exp $	*/

/*++
/* NAME
/*	dict_dbm 3
/* SUMMARY
/*	dictionary manager interface to DBM files
/* SYNOPSIS
/*	#include <dict_dbm.h>
/*
/*	DICT	*dict_dbm_open(path, open_flags, dict_flags)
/*	const char *name;
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_dbm_open() opens the named DBM database and makes it available
/*	via the generic interface described in dict_open(3).
/* DIAGNOSTICS
/*	Fatal errors: cannot open file, file write error, out of memory.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/*	ndbm(3) data base subroutines
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

#ifdef HAS_DBM

/* System library. */

#include <sys/stat.h>
#ifdef PATH_NDBM_H
#include PATH_NDBM_H
#else
#include <ndbm.h>
#endif
#ifdef R_FIRST
#error "Error: you are including the Berkeley DB version of ndbm.h"
#error "To build with Postfix NDBM support, delete the Berkeley DB ndbm.h file"
#endif
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "htable.h"
#include "iostuff.h"
#include "vstring.h"
#include "myflock.h"
#include "stringops.h"
#include "dict.h"
#include "dict_dbm.h"
#include "warn_stat.h"

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    DBM    *dbm;			/* open database */
    VSTRING *key_buf;			/* key buffer */
    VSTRING *val_buf;			/* result buffer */
} DICT_DBM;

#define SCOPY(buf, data, size) \
    vstring_str(vstring_strncpy(buf ? buf : (buf = vstring_alloc(10)), data, size))

/* dict_dbm_lookup - find database entry */

static const char *dict_dbm_lookup(DICT *dict, const char *name)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    const char *result = 0;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_dbm_lookup: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
	msg_fatal("%s: lock dictionary: %m", dict_dbm->dict.name);

    /*
     * See if this DBM file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name) + 1;
	dbm_value = dbm_fetch(dict_dbm->dbm, dbm_key);
	if (dbm_value.dptr != 0) {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
	    result = SCOPY(dict_dbm->val_buf, dbm_value.dptr, dbm_value.dsize);
	}
    }

    /*
     * See if this DBM file was written with no null byte appended to key and
     * value.
     */
    if (result == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name);
	dbm_value = dbm_fetch(dict_dbm->dbm, dbm_key);
	if (dbm_value.dptr != 0) {
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
	    result = SCOPY(dict_dbm->val_buf, dbm_value.dptr, dbm_value.dsize);
	}
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    return (result);
}

/* dict_dbm_update - add or update database entry */

static int dict_dbm_update(DICT *dict, const char *name, const char *value)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    int     status;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_dbm_update: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
	msg_fatal("%s: lock dictionary: %m", dict_dbm->dict.name);

    /*
     * Do the update.
     */
    if ((status = dbm_store(dict_dbm->dbm, dbm_key, dbm_value,
     (dict->flags & DICT_FLAG_DUP_REPLACE) ? DBM_REPLACE : DBM_INSERT)) < 0)
	msg_fatal("error writing DBM database %s: %m", dict_dbm->dict.name);
    if (status) {
	if (dict->flags & DICT_FLAG_DUP_IGNORE)
	     /* void */ ;
	else if (dict->flags & DICT_FLAG_DUP_WARN)
	    msg_warn("%s: duplicate entry: \"%s\"", dict_dbm->dict.name, name);
	else
	    msg_fatal("%s: duplicate entry: \"%s\"", dict_dbm->dict.name, name);
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    return (status);
}

/* dict_dbm_delete - delete one entry from the dictionary */

static int dict_dbm_delete(DICT *dict, const char *name)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    int     status = 1;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_dbm_delete: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
	msg_fatal("%s: lock dictionary: %m", dict_dbm->dict.name);

    /*
     * See if this DBM file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	dbm_key.dptr = (void *) name;
	dbm_key.dsize = strlen(name) + 1;
	dbm_clearerr(dict_dbm->dbm);
	if ((status = dbm_delete(dict_dbm->dbm, dbm_key)) < 0) {
	    if (dbm_error(dict_dbm->dbm) != 0)	/* fatal error */
		msg_fatal("error deleting from %s: %m", dict_dbm->dict.name);
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
	dbm_clearerr(dict_dbm->dbm);
	if ((status = dbm_delete(dict_dbm->dbm, dbm_key)) < 0) {
	    if (dbm_error(dict_dbm->dbm) != 0)	/* fatal error */
		msg_fatal("error deleting from %s: %m", dict_dbm->dict.name);
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
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    return (status);
}

/* traverse the dictionary */

static int dict_dbm_sequence(DICT *dict, int function,
			             const char **key, const char **value)
{
    const char *myname = "dict_dbm_sequence";
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    int     status;

    dict->error = 0;

    /*
     * Acquire a shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_dbm->dict.name);

    /*
     * Determine and execute the seek function. It returns the key.
     */
    switch (function) {
    case DICT_SEQ_FUN_FIRST:
	dbm_key = dbm_firstkey(dict_dbm->dbm);
	break;
    case DICT_SEQ_FUN_NEXT:
	dbm_key = dbm_nextkey(dict_dbm->dbm);
	break;
    default:
	msg_panic("%s: invalid function: %d", myname, function);
    }

    if (dbm_key.dptr != 0 && dbm_key.dsize > 0) {

	/*
	 * Copy the key so that it is guaranteed null terminated.
	 */
	*key = SCOPY(dict_dbm->key_buf, dbm_key.dptr, dbm_key.dsize);

	/*
	 * Fetch the corresponding value.
	 */
	dbm_value = dbm_fetch(dict_dbm->dbm, dbm_key);

	if (dbm_value.dptr != 0 && dbm_value.dsize > 0) {

	    /*
	     * Copy the value so that it is guaranteed null terminated.
	     */
	    *value = SCOPY(dict_dbm->val_buf, dbm_value.dptr, dbm_value.dsize);
	    status = 0;
	} else {

	    /*
	     * Determine if we have hit the last record or an error
	     * condition.
	     */
	    if (dbm_error(dict_dbm->dbm))
		msg_fatal("error seeking %s: %m", dict_dbm->dict.name);
	    status = 1;				/* no error: eof/not found
						 * (should not happen!) */
	}
    } else {

	/*
	 * Determine if we have hit the last record or an error condition.
	 */
	if (dbm_error(dict_dbm->dbm))
	    msg_fatal("error seeking %s: %m", dict_dbm->dict.name);
	status = 1;				/* no error: eof/not found */
    }

    /*
     * Release the shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    return (status);
}

/* dict_dbm_close - disassociate from data base */

static void dict_dbm_close(DICT *dict)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;

    dbm_close(dict_dbm->dbm);
    if (dict_dbm->key_buf)
	vstring_free(dict_dbm->key_buf);
    if (dict_dbm->val_buf)
	vstring_free(dict_dbm->val_buf);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_dbm_open - open DBM data base */

DICT   *dict_dbm_open(const char *path, int open_flags, int dict_flags)
{
    DICT_DBM *dict_dbm;
    struct stat st;
    DBM    *dbm;
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
	    return (dict_surrogate(DICT_TYPE_DBM, path, open_flags, dict_flags,
				   "open database %s: %m", dbm_path));
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	    msg_fatal("shared-lock database %s for open: %m", dbm_path);
    }

    /*
     * XXX SunOS 5.x has no const in dbm_open() prototype.
     */
    if ((dbm = dbm_open((char *) path, open_flags, 0644)) == 0)
	return (dict_surrogate(DICT_TYPE_DBM, path, open_flags, dict_flags,
			       "open database %s.{dir,pag}: %m", path));

    if (dict_flags & DICT_FLAG_LOCK) {
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	    msg_fatal("unlock database %s for open: %m", dbm_path);
	if (close(lock_fd) < 0)
	    msg_fatal("close database %s: %m", dbm_path);
    }
    dict_dbm = (DICT_DBM *) dict_alloc(DICT_TYPE_DBM, path, sizeof(*dict_dbm));
    dict_dbm->dict.lookup = dict_dbm_lookup;
    dict_dbm->dict.update = dict_dbm_update;
    dict_dbm->dict.delete = dict_dbm_delete;
    dict_dbm->dict.sequence = dict_dbm_sequence;
    dict_dbm->dict.close = dict_dbm_close;
    dict_dbm->dict.lock_fd = dbm_dirfno(dbm);
    dict_dbm->dict.stat_fd = dbm_pagfno(dbm);
    if (dict_dbm->dict.lock_fd == dict_dbm->dict.stat_fd)
	msg_fatal("open database %s: cannot support GDBM", path);
    if (fstat(dict_dbm->dict.stat_fd, &st) < 0)
	msg_fatal("dict_dbm_open: fstat: %m");
    dict_dbm->dict.mtime = st.st_mtime;
    dict_dbm->dict.owner.uid = st.st_uid;
    dict_dbm->dict.owner.status = (st.st_uid != 0);

    /*
     * Warn if the source file is newer than the indexed file, except when
     * the source file changed only seconds ago.
     */
    if ((dict_flags & DICT_FLAG_LOCK) != 0
	&& stat(path, &st) == 0
	&& st.st_mtime > dict_dbm->dict.mtime
	&& st.st_mtime < time((time_t *) 0) - 100)
	msg_warn("database %s is older than source file %s", dbm_path, path);

    close_on_exec(dbm_pagfno(dbm), CLOSE_ON_EXEC);
    close_on_exec(dbm_dirfno(dbm), CLOSE_ON_EXEC);
    dict_dbm->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if ((dict_flags & (DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL)) == 0)
	dict_dbm->dict.flags |= (DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL);
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_dbm->dict.fold_buf = vstring_alloc(10);
    dict_dbm->dbm = dbm;
    dict_dbm->key_buf = 0;
    dict_dbm->val_buf = 0;

    if ((dict_flags & DICT_FLAG_LOCK))
	myfree(dbm_path);

    return (DICT_DEBUG (&dict_dbm->dict));
}

#endif
