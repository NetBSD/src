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

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    DBM    *dbm;			/* open database */
} DICT_DBM;

/* dict_dbm_lookup - find database entry */

static const char *dict_dbm_lookup(DICT *dict, const char *name)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    static VSTRING *buf;
    const char *result = 0;

    dict_errno = 0;

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
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
	    result = dbm_value.dptr;
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
	    if (buf == 0)
		buf = vstring_alloc(10);
	    vstring_strncpy(buf, dbm_value.dptr, dbm_value.dsize);
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
	    result = vstring_str(buf);
	}
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    return (result);
}

/* dict_dbm_update - add or update database entry */

static void dict_dbm_update(DICT *dict, const char *name, const char *value)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    int     status;

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
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
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
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);
}

/* dict_dbm_delete - delete one entry from the dictionary */

static int dict_dbm_delete(DICT *dict, const char *name)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    int     status = 1;
    int     flags = 0;

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
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
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    return (status);
}

/* traverse the dictionary */

static int dict_dbm_sequence(DICT *dict, const int function,
			             const char **key, const char **value)
{
    char   *myname = "dict_dbm_sequence";
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;
    datum   dbm_key;
    datum   dbm_value;
    int     status = 0;
    static VSTRING *key_buf;
    static VSTRING *value_buf;

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
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

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_dbm->dict.name);

    if (dbm_key.dptr != 0 && dbm_key.dsize > 0) {

	/*
	 * See if this DB file was written with one null byte appended to key
	 * an d value or not. If necessary, copy the key.
	 */
	if (((char *) dbm_key.dptr)[dbm_key.dsize - 1] == 0) {
	    *key = dbm_key.dptr;
	} else {
	    if (key_buf == 0)
		key_buf = vstring_alloc(10);
	    vstring_strncpy(key_buf, dbm_key.dptr, dbm_key.dsize);
	    *key = vstring_str(key_buf);
	}

	/*
	 * Fetch the corresponding value.
	 */
	dbm_value = dbm_fetch(dict_dbm->dbm, dbm_key);

	if (dbm_value.dptr != 0 && dbm_value.dsize > 0) {

	    /*
	     * See if this DB file was written with one null byte appended to
	     * key and value or not. If necessary, copy the key.
	     */
	    if (((char *) dbm_value.dptr)[dbm_value.dsize - 1] == 0) {
		*value = dbm_value.dptr;
	    } else {
		if (value_buf == 0)
		    value_buf = vstring_alloc(10);
		vstring_strncpy(value_buf, dbm_value.dptr, dbm_value.dsize);
		*value = vstring_str(value_buf);
	    }
	} else {

	    /*
	     * Determine if we have hit the last record or an error
	     * condition.
	     */
	    if (dbm_error(dict_dbm->dbm))
		msg_fatal("error seeking %s: %m", dict_dbm->dict.name);
	    return (1);				/* no error: eof/not found
						 * (should not happen!) */
	}
    } else {

	/*
	 * Determine if we have hit the last record or an error condition.
	 */
	if (dbm_error(dict_dbm->dbm))
	    msg_fatal("error seeking %s: %m", dict_dbm->dict.name);
	return (1);				/* no error: eof/not found */
    }
    return (0);
}

/* dict_dbm_close - disassociate from data base */

static void dict_dbm_close(DICT *dict)
{
    DICT_DBM *dict_dbm = (DICT_DBM *) dict;

    dbm_close(dict_dbm->dbm);
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
	dbm_path = concatenate(path, ".pag", (char *) 0);
	if ((lock_fd = open(dbm_path, open_flags, 0644)) < 0)
	    msg_fatal("open database %s: %m", dbm_path);
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	    msg_fatal("shared-lock database %s for open: %m", dbm_path);
    }

    /*
     * XXX SunOS 5.x has no const in dbm_open() prototype.
     */
    if ((dbm = dbm_open((char *) path, open_flags, 0644)) == 0)
	msg_fatal("open database %s.{dir,pag}: %m", path);

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
    dict_dbm->dict.fd = dbm_pagfno(dbm);
    if (fstat(dict_dbm->dict.fd, &st) < 0)
	msg_fatal("dict_dbm_open: fstat: %m");
    dict_dbm->dict.mtime = st.st_mtime;

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
    dict_dbm->dbm = dbm;

    if ((dict_flags & DICT_FLAG_LOCK))
	myfree(dbm_path);

    return (DICT_DEBUG (&dict_dbm->dict));
}

#endif
