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
#include <errno.h>

#if defined(_DB_185_H_) && defined(USE_FCNTL_LOCK)
#error "Error: this system must not use the db 1.85 compatibility interface"
#endif

#ifndef DB_VERSION_MAJOR
#define DB_VERSION_MAJOR 1
#define DICT_DB_GET(db, key, val, flag)	db->get(db, key, val, flag)
#define DICT_DB_PUT(db, key, val, flag)	db->put(db, key, val, flag)
#define DICT_DB_DEL(db, key, flag)	db->del(db, key, flag)
#define DICT_DB_SYNC(db, flag)		db->sync(db, flag)
#define DICT_DB_CLOSE(db)		db->close(db)
#define DONT_CLOBBER			R_NOOVERWRITE
#endif

#if DB_VERSION_MAJOR > 1
#define DICT_DB_GET(db, key, val, flag)	sanitize(db->get(db, 0, key, val, flag))
#define DICT_DB_PUT(db, key, val, flag)	sanitize(db->put(db, 0, key, val, flag))
#define DICT_DB_DEL(db, key, flag)	sanitize(db->del(db, 0, key, flag))
#define DICT_DB_SYNC(db, flag)		((errno = db->sync(db, flag)) ? -1 : 0)
#define DICT_DB_CLOSE(db)		((errno = db->close(db, 0)) ? -1 : 0)
#define DONT_CLOBBER			DB_NOOVERWRITE
#endif

#ifndef DB_FCNTL_LOCKING
#define DB_FCNTL_LOCKING		0
#endif

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
} DICT_DB;

#define DICT_DB_CACHE_SIZE	(1024 * 1024)
#define DICT_DB_NELM		4096

#if DB_VERSION_MAJOR > 1

/* sanitize - sanitize db_get/put/del result */

static int sanitize(int status)
{

    /*
     * XXX This is unclean but avoids a lot of clutter elsewhere. Categorize
     * results into non-fatal errors (i.e., errors that we can deal with),
     * success, or fatal error (i.e., all other errors).
     */
    switch (status) {

	case DB_NOTFOUND:		/* get, del */
	case DB_KEYEXIST:		/* put */
	return (1);			/* non-fatal */

    case 0:
	return (0);				/* success */

    case DB_KEYEMPTY:				/* get, others? */
	status = EINVAL;
    default:
	errno = status;
	return (-1);				/* fatal */
    }
}

#endif

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
    memset(&db_key, 0, sizeof(db_key));
    memset(&db_value, 0, sizeof(db_value));

    /*
     * Acquire a shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->dict.name);

    /*
     * See if this DB file was written with one null byte appended to key and
     * value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	db_key.data = (void *) name;
	db_key.size = strlen(name) + 1;
	if ((status = DICT_DB_GET(db, &db_key, &db_value, 0)) < 0)
	    msg_fatal("error reading %s: %m", dict_db->dict.name);
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
	if ((status = DICT_DB_GET(db, &db_key, &db_value, 0)) < 0)
	    msg_fatal("error reading %s: %m", dict_db->dict.name);
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
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->dict.name);

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

    memset(&db_key, 0, sizeof(db_key));
    memset(&db_value, 0, sizeof(db_value));
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
#else
	dict->flags &= ~DICT_FLAG_TRY0NULL;
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
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->dict.name);

    /*
     * Do the update.
     */
    if ((status = DICT_DB_PUT(db, &db_key, &db_value,
	     (dict->flags & DICT_FLAG_DUP_REPLACE) ? 0 : DONT_CLOBBER)) < 0)
	msg_fatal("error writing %s: %m", dict_db->dict.name);
    if (status) {
	if (dict->flags & DICT_FLAG_DUP_IGNORE)
	     /* void */ ;
	else if (dict->flags & DICT_FLAG_DUP_WARN)
	    msg_warn("%s: duplicate entry: \"%s\"", dict_db->dict.name, name);
	else
	    msg_fatal("%s: duplicate entry: \"%s\"", dict_db->dict.name, name);
    }
    if (dict->flags & DICT_FLAG_SYNC_UPDATE)
	if (DICT_DB_SYNC(db, 0) < 0)
	    msg_fatal("%s: flush dictionary: %m", dict_db->dict.name);

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->dict.name);
}

/* delete one entry from the dictionary */

static int dict_db_delete(DICT *dict, const char *name)
{
    DICT_DB *dict_db = (DICT_DB *) dict;
    DB     *db = dict_db->db;
    DBT     db_key;
    int     status = 1;
    int     flags = 0;

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->dict.name);

    /*
     * See if this DB file was written with one null byte appended to key and
     * value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	db_key.data = (void *) name;
	db_key.size = strlen(name) + 1;
	if ((status = DICT_DB_DEL(db, &db_key, flags)) < 0)
	    msg_fatal("error deleting from %s: %m", dict_db->dict.name);
	if (status == 0)
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
    }

    /*
     * See if this DB file was written with no null byte appended to key and
     * value.
     */
    if (status > 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	db_key.data = (void *) name;
	db_key.size = strlen(name);
	if ((status = DICT_DB_DEL(db, &db_key, flags)) < 0)
	    msg_fatal("error deleting from %s: %m", dict_db->dict.name);
	if (status == 0)
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
    }
    if (dict->flags & DICT_FLAG_SYNC_UPDATE)
	if (DICT_DB_SYNC(db, 0) < 0)
	    msg_fatal("%s: flush dictionary: %m", dict_db->dict.name);

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->dict.name);

    return status;
}

/* dict_db_sequence - traverse the dictionary */

static int dict_db_sequence(DICT *dict, const int function,
			            const char **key, const char **value)
{
#if DB_VERSION_MAJOR > 1
    msg_fatal("dict_db_sequence - operation is to be implemented");
#else
    char   *myname = "dict_db_sequence";
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
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict_db->dict.name);

    if ((status = db->seq(db, &db_key, &db_value, db_function)) < 0)
	msg_fatal("error seeking %s: %m", dict_db->dict.name);

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict_db->dict.name);

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
#endif
}

/* dict_db_close - close data base */

static void dict_db_close(DICT *dict)
{
    DICT_DB *dict_db = (DICT_DB *) dict;

    if (DICT_DB_SYNC(dict_db->db, 0) < 0)
	msg_fatal("flush database %s: %m", dict_db->dict.name);
    if (DICT_DB_CLOSE(dict_db->db) < 0)
	msg_fatal("close database %s: %m", dict_db->dict.name);
    dict_free(dict);
}

/* dict_db_open - open data base */

static DICT *dict_db_open(const char *class, const char *path, int open_flags,
			          int type, void *tweak, int dict_flags)
{
    DICT_DB *dict_db;
    struct stat st;
    DB     *db;
    char   *db_path;
    int     lock_fd = -1;
    int     dbfd;

#if DB_VERSION_MAJOR > 1
    int     db_flags;

#endif

    db_path = concatenate(path, ".db", (char *) 0);

    /*
     * Note: DICT_FLAG_LOCK is used only by programs that do fine-grained (in
     * the time domain) locking while accessing individual database records.
     * 
     * Programs such as postmap/postalias use their own large-grained (in the
     * time domain) locks while rewriting the entire file.
     */
    if (dict_flags & DICT_FLAG_LOCK) {
	if ((lock_fd = open(db_path, open_flags, 0644)) < 0)
	    msg_fatal("open database %s: %m", db_path);
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_SHARED) < 0)
	    msg_fatal("shared-lock database %s for open: %m", db_path);
    }

    /*
     * Use the DB 1.x programming interface. This is the default interface
     * with 4.4BSD systems. It is also available via the db_185 compatibility
     * interface, but that interface does not have the undocumented feature
     * that we need to make file locking safe with POSIX fcntl() locking.
     */
#if DB_VERSION_MAJOR < 2
    if ((db = dbopen(db_path, open_flags, 0644, type, tweak)) == 0)
	msg_fatal("open database %s: %m", db_path);
    dbfd = db->fd(db);
#endif

    /*
     * Use the DB 2.x programming interface. Jump a couple extra hoops.
     */
#if DB_VERSION_MAJOR == 2
    db_flags = DB_FCNTL_LOCKING;
    if (open_flags == O_RDONLY)
	db_flags |= DB_RDONLY;
    if (open_flags & O_CREAT)
	db_flags |= DB_CREATE;
    if (open_flags & O_TRUNC)
	db_flags |= DB_TRUNCATE;
    if ((errno = db_open(db_path, type, db_flags, 0644, 0, tweak, &db)) != 0)
	msg_fatal("open database %s: %m", db_path);
    if (db == 0)
	msg_panic("db_open null result");
    if ((errno = db->fd(db, &dbfd)) != 0)
	msg_fatal("get database file descriptor: %m");
#endif

    /*
     * Use the DB 3.x programming interface. Jump even more hoops.
     */
#if DB_VERSION_MAJOR > 2
    db_flags = DB_FCNTL_LOCKING;
    if (open_flags == O_RDONLY)
	db_flags |= DB_RDONLY;
    if (open_flags & O_CREAT)
	db_flags |= DB_CREATE;
    if (open_flags & O_TRUNC)
	db_flags |= DB_TRUNCATE;
    if ((errno = db_create(&db, 0, 0)) != 0)
	msg_fatal("create DB database: %m");
    if (db == 0)
	msg_panic("db_create null result");
    if ((errno = db->set_cachesize(db, 0, DICT_DB_CACHE_SIZE, 0)) != 0)
	msg_fatal("set DB cache size %d: %m", DICT_DB_CACHE_SIZE);
    if (type == DB_HASH && db->set_h_nelem(db, DICT_DB_NELM) != 0)
	msg_fatal("set DB hash element count %d: %m", DICT_DB_NELM);
    if ((errno = db->open(db, db_path, 0, type, db_flags, 0644)) != 0)
	msg_fatal("open database %s: %m", db_path);
    if ((errno = db->fd(db, &dbfd)) != 0)
	msg_fatal("get database file descriptor: %m");
#endif
    if (dict_flags & DICT_FLAG_LOCK) {
	if (myflock(lock_fd, INTERNAL_LOCK, MYFLOCK_OP_NONE) < 0)
	    msg_fatal("unlock database %s for open: %m", db_path);
	if (close(lock_fd) < 0)
	    msg_fatal("close database %s: %m", db_path);
    }
    dict_db = (DICT_DB *) dict_alloc(class, db_path, sizeof(*dict_db));
    dict_db->dict.lookup = dict_db_lookup;
    dict_db->dict.update = dict_db_update;
    dict_db->dict.delete = dict_db_delete;
    dict_db->dict.sequence = dict_db_sequence;
    dict_db->dict.close = dict_db_close;
    dict_db->dict.fd = dbfd;
    if (fstat(dict_db->dict.fd, &st) < 0)
	msg_fatal("dict_db_open: fstat: %m");
    dict_db->dict.mtime = st.st_mtime;

    /*
     * Warn if the source file is newer than the indexed file, except when
     * the source file changed only seconds ago.
     */
    if ((dict_flags & DICT_FLAG_LOCK) != 0
	&& stat(path, &st) == 0
	&& st.st_mtime > dict_db->dict.mtime
	&& st.st_mtime < time((time_t *) 0) - 100)
	msg_warn("database %s is older than source file %s", db_path, path);

    close_on_exec(dict_db->dict.fd, CLOSE_ON_EXEC);
    dict_db->dict.flags = dict_flags | DICT_FLAG_FIXED;
    if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	dict_db->dict.flags |= (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL);
    dict_db->db = db;
    myfree(db_path);
    return (DICT_DEBUG (&dict_db->dict));
}

/* dict_hash_open - create association with data base */

DICT   *dict_hash_open(const char *path, int open_flags, int dict_flags)
{
#if DB_VERSION_MAJOR < 2
    HASHINFO tweak;

    memset((char *) &tweak, 0, sizeof(tweak));
    tweak.nelem = DICT_DB_NELM;
    tweak.cachesize = DICT_DB_CACHE_SIZE;
#endif
#if DB_VERSION_MAJOR == 2
    DB_INFO tweak;

    memset((char *) &tweak, 0, sizeof(tweak));
    tweak.h_nelem = DICT_DB_NELM;
    tweak.db_cachesize = DICT_DB_CACHE_SIZE;
#endif
#if DB_VERSION_MAJOR > 2
    void   *tweak;

    tweak = 0;
#endif
    return (dict_db_open(DICT_TYPE_HASH, path, open_flags, DB_HASH,
			 (void *) &tweak, dict_flags));
}

/* dict_btree_open - create association with data base */

DICT   *dict_btree_open(const char *path, int open_flags, int dict_flags)
{
#if DB_VERSION_MAJOR < 2
    BTREEINFO tweak;

    memset((char *) &tweak, 0, sizeof(tweak));
    tweak.cachesize = DICT_DB_CACHE_SIZE;
#endif
#if DB_VERSION_MAJOR == 2
    DB_INFO tweak;

    memset((char *) &tweak, 0, sizeof(tweak));
    tweak.db_cachesize = DICT_DB_CACHE_SIZE;
#endif
#if DB_VERSION_MAJOR > 2
    void   *tweak;

    tweak = 0;
#endif

    return (dict_db_open(DICT_TYPE_BTREE, path, open_flags, DB_BTREE,
			 (void *) &tweak, dict_flags));
}

#endif
