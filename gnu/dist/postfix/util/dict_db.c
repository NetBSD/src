/*++
/* NAME
/*	dict_db 3
/* SUMMARY
/*	dictionary manager interface to DB files
/* SYNOPSIS
/*	#include <dict_db.h>
/*
/*	DICT	*dict_hash_open(path, open_flags, dict_flags)
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/*
/*	DICT	*dict_btree_open(path, open_flags, dict_flags)
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_XXX_open() opens the specified DB database.  The result is
/*	a pointer to a structure that can be used to access the dictionary
/*	using the generic methods documented in dict_open(3).
/*
/*	Arguments:
/* .IP path
/*	The database pathname, not including the ".db" suffix.
/* .IP open_flags
/*	Flags passed to dbopen().
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
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#include "sys_defs.h"

#ifdef HAS_DB

/* System library. */

#include <sys/stat.h>
#include <limits.h>
#ifdef PATH_DB_H
#include PATH_DB_H
#else
#include <db.h>
#endif
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "vstring.h"
#include "stringops.h"
#include "iostuff.h"
#include "myflock.h"
#include "dict.h"
#include "dict_db.h"

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    DB     *db;				/* open db file */
    char   *path;			/* pathname */
} DICT_DB;

#define DICT_DB_CACHE_SIZE	(1024 * 1024)
#define DICT_DB_NELM	4096

/* dict_db_lookup - find database entry */

static const char *dict_db_lookup(DICT *dict, const char *name)
{
    DICT_DB *dict_db = (DICT_DB *) dict;
    DB     *db = dict_db->db;
    DBT     db_key;
    DBT     db_value;
    int     status;
    static VSTRING *buf;
    const char *result = 0;

    dict_errno = 0;

    /*
     * Acquire a shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->path);

    /*
     * See if this DB file was written with one null byte appended to key and
     * value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	db_key.data = (void *) name;
	db_key.size = strlen(name) + 1;
	if ((status = db->get(db, &db_key, &db_value, 0)) < 0)
	    msg_fatal("error reading %s: %m", dict_db->path);
	if (status == 0) {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
	    result = db_value.data;
	}
    }

    /*
     * See if this DB file was written with no null byte appended to key and
     * value.
     */
    if (result == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	db_key.data = (void *) name;
	db_key.size = strlen(name);
	if ((status = db->get(db, &db_key, &db_value, 0)) < 0)
	    msg_fatal("error reading %s: %m", dict_db->path);
	if (status == 0) {
	    if (buf == 0)
		buf = vstring_alloc(10);
	    vstring_strncpy(buf, db_value.data, db_value.size);
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
	    result = vstring_str(buf);
	}
    }

    /*
     * Release the shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->path);

    return (result);
}

/* dict_db_update - add or update database entry */

static void dict_db_update(DICT *dict, const char *name, const char *value)
{
    DICT_DB *dict_db = (DICT_DB *) dict;
    DB     *db = dict_db->db;
    DBT     db_key;
    DBT     db_value;
    int     status;

    db_key.data = (void *) name;
    db_value.data = (void *) value;
    db_key.size = strlen(name);
    db_value.size = strlen(value);

    /*
     * If undecided about appending a null byte to key and value, choose a
     * default depending on the platform.
     */
    if ((dict->flags & DICT_FLAG_TRY1NULL)
	&& (dict->flags & DICT_FLAG_TRY0NULL)) {
#ifdef DB_NO_TRAILING_NULL
	dict->flags &= ~DICT_FLAG_TRY1NULL;
	dict->flags |= DICT_FLAG_TRY0NULL;
#else
	dict->flags &= ~DICT_FLAG_TRY0NULL;
	dict->flags |= DICT_FLAG_TRY1NULL;
#endif
    }

    /*
     * Optionally append a null byte to key and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	db_key.size++;
	db_value.size++;
    }

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->path);

    /*
     * Do the update.
     */
    if ((status = db->put(db, &db_key, &db_value,
	    (dict->flags & DICT_FLAG_DUP_REPLACE) ? 0 : R_NOOVERWRITE)) < 0)
	msg_fatal("error writing %s: %m", dict_db->path);
    if (status) {
	if (dict->flags & DICT_FLAG_DUP_IGNORE)
	     /* void */ ;
	else if (dict->flags & DICT_FLAG_DUP_WARN)
	    msg_warn("%s: duplicate entry: \"%s\"", dict_db->path, name);
	else
	    msg_fatal("%s: duplicate entry: \"%s\"", dict_db->path, name);
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->path);
}

/* delete one entry from the dictionary */

static int dict_db_delete(DICT *dict, const char *key)
{
    DICT_DB *dict_db = (DICT_DB *) dict;
    DB     *db = dict_db->db;
    DBT     db_key;
    int     status;
    int     flags = 0;

    db_key.data = (void *) key;
    db_key.size = strlen(key);

    /*
     * If undecided about appending a null byte to key and value, choose a
     * default depending on the platform.
     */
    if ((dict->flags & DICT_FLAG_TRY1NULL)
	&& (dict->flags & DICT_FLAG_TRY0NULL)) {
#ifdef DB_NO_TRAILING_NULL
	dict->flags = DICT_FLAG_TRY0NULL;
#else
	dict->flags = DICT_FLAG_TRY1NULL;
#endif
    }

    /*
     * Optionally append a null byte to key and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	db_key.size++;
    }

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->path);

    /*
     * Do the delete operation.
     */
    if ((status = db->del(db, &db_key, flags)) < 0)
	msg_fatal("error deleting %s: %m", dict_db->path);

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->path);

    return status;
}

/* dict_db_sequence - traverse the dictionary */

static int dict_db_sequence(DICT *dict, const int function,
			            const char **key, const char **value)
{
    const char *myname = "dict_db_sequence";
    DICT_DB *dict_db = (DICT_DB *) dict;
    DB     *db = dict_db->db;
    DBT     db_key;
    DBT     db_value;
    int     status = 0;
    int     db_function;
    static VSTRING *key_buf;
    static VSTRING *value_buf;

    /*
     * determine the function
     */
    switch (function) {
    case DICT_SEQ_FUN_FIRST:
	db_function = R_FIRST;
	break;
    case DICT_SEQ_FUN_NEXT:
	db_function = R_NEXT;
	break;
    default:
	msg_panic("%s: invalid function %d", myname, function);
    }

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->path);

    if ((status = db->seq(db, &db_key, &db_value, db_function)) < 0)
	msg_fatal("error seeking %s: %m", dict_db->path);

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK) && myflock(dict->fd, MYFLOCK_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->path);

    if (status == 0) {

	/*
	 * See if this DB file was written with one null byte appended to key
	 * and value or not.
	 */
	if (((char *) db_key.data)[db_key.size] == 0) {
	    *key = db_key.data;
	} else {
	    if (key_buf == 0)
		key_buf = vstring_alloc(10);
	    vstring_strncpy(key_buf, db_key.data, db_key.size);
	    *key = vstring_str(key_buf);
	}
	if (((char *) db_value.data)[db_value.size] == 0) {
	    *value = db_value.data;
	} else {
	    if (value_buf == 0)
		value_buf = vstring_alloc(10);
	    vstring_strncpy(value_buf, db_value.data, db_value.size);
	    *value = vstring_str(value_buf);
	}
    }
    return status;
}

/* dict_db_close - close data base */

static void dict_db_close(DICT *dict)
{
    DICT_DB *dict_db = (DICT_DB *) dict;

    if (dict_db->db->close(dict_db->db) < 0)
	msg_fatal("close database %s: %m", dict_db->path);
    myfree(dict_db->path);
    myfree((char *) dict_db);
}

/* dict_db_open - open data base */

static DICT *dict_db_open(const char *path, int open_flags, int type,
			          void *tweak, int dict_flags)
{
    DICT_DB *dict_db;
    struct stat st;
    DB     *db;
    char   *db_path;
    int     lock_fd = -1;

    db_path = concatenate(path, ".db", (char *) 0);

    if (dict_flags & DICT_FLAG_LOCK) {
	if ((lock_fd = open(db_path, open_flags, 0644)) < 0)
	    msg_fatal("open database %s: %m", db_path);
	if (myflock(lock_fd, MYFLOCK_SHARED) < 0)
	    msg_fatal("shared-lock database %s for open: %m", db_path);
    }
    if ((db = dbopen(db_path, open_flags, 0644, type, tweak)) == 0)
	msg_fatal("open database %s: %m", db_path);

    if (dict_flags & DICT_FLAG_LOCK) {
	if (myflock(lock_fd, MYFLOCK_NONE) < 0)
	    msg_fatal("unlock database %s for open: %m", db_path);
	if (close(lock_fd) < 0)
	    msg_fatal("close database %s: %m", db_path);
    }
    dict_db = (DICT_DB *) mymalloc(sizeof(*dict_db));
    dict_db->dict.lookup = dict_db_lookup;
    dict_db->dict.update = dict_db_update;
    dict_db->dict.delete = dict_db_delete;
    dict_db->dict.sequence = dict_db_sequence;
    dict_db->dict.close = dict_db_close;
    dict_db->dict.fd = db->fd(db);
    if (fstat(dict_db->dict.fd, &st) < 0)
	msg_fatal("dict_db_open: fstat: %m");
    dict_db->dict.mtime = st.st_mtime;
    close_on_exec(dict_db->dict.fd, CLOSE_ON_EXEC);
    dict_db->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	dict_db->dict.flags |= (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL);
    dict_db->db = db;
    dict_db->path = db_path;
    return (&dict_db->dict);
}

/* dict_hash_open - create association with data base */

DICT   *dict_hash_open(const char *path, int open_flags, int dict_flags)
{
    HASHINFO tweak;

    memset((char *) &tweak, 0, sizeof(tweak));
    tweak.nelem = DICT_DB_NELM;
    tweak.cachesize = DICT_DB_CACHE_SIZE;
    return (dict_db_open(path, open_flags, DB_HASH, (void *) &tweak, dict_flags));
}

/* dict_btree_open - create association with data base */

DICT   *dict_btree_open(const char *path, int open_flags, int dict_flags)
{
    BTREEINFO tweak;

    memset((char *) &tweak, 0, sizeof(tweak));
    tweak.cachesize = DICT_DB_CACHE_SIZE;

    return (dict_db_open(path, open_flags, DB_BTREE, (void *) &tweak, dict_flags));
}

#endif
