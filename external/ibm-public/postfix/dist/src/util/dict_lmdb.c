/*	$NetBSD: dict_lmdb.c,v 1.1.1.1.2.2 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	dict_lmdb 3
/* SUMMARY
/*	dictionary manager interface to OpenLDAP LMDB files
/* SYNOPSIS
/*	#include <dict_lmdb.h>
/*
/*	size_t	dict_lmdb_map_size;
/*
/*	DICT	*dict_lmdb_open(path, open_flags, dict_flags)
/*	const char *name;
/*	const char *path;
/*	int	open_flags;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_lmdb_open() opens the named LMDB database and makes
/*	it available via the generic interface described in
/*	dict_open(3).
/*
/*	The dict_lmdb_map_size variable specifies the initial
/*	database memory map size.  When a map becomes full its size
/*	is doubled, and other programs pick up the size change.
/* DIAGNOSTICS
/*	Fatal errors: cannot open file, file write error, out of
/*	memory.
/* BUGS
/*	The on-the-fly map resize operations require no concurrent
/*	activity in the same database by other threads in the same
/*	memory address space.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Howard Chu
/*	Symas Corporation
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#include <sys_defs.h>

#ifdef HAS_LMDB

/* System library. */

#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <iostuff.h>
#include <vstring.h>
#include <myflock.h>
#include <stringops.h>
#include <slmdb.h>
#include <dict.h>
#include <dict_lmdb.h>
#include <warn_stat.h>

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
    SLMDB   slmdb;			/* sane LMDB API */
    VSTRING *key_buf;			/* key buffer */
    VSTRING *val_buf;			/* value buffer */
} DICT_LMDB;

 /*
  * The LMDB database filename suffix happens to equal our DICT_TYPE_LMDB
  * prefix, but that doesn't mean it is kosher to use DICT_TYPE_LMDB where a
  * suffix is needed, so we define an explicit suffix here.
  */
#define DICT_LMDB_SUFFIX	"lmdb"

 /*
  * Make a safe string copy that is guaranteed to be null-terminated.
  */
#define SCOPY(buf, data, size) \
    vstring_str(vstring_strncpy(buf ? buf : (buf = vstring_alloc(10)), data, size))

 /*
  * Postfix writers recover from a "map full" error by increasing the memory
  * map size with a factor DICT_LMDB_SIZE_INCR (up to some limit) and
  * retrying the transaction.
  * 
  * Each dict(3) API call is retried no more than a few times. For bulk-mode
  * transactions the number of retries is proportional to the size of the
  * address space.
  * 
  * We do not expose these details to the Postfix user interface. The purpose of
  * Postfix is to solve problems, not punt them to the user.
  */
#ifndef SSIZE_T_MAX			/* The maximum map size */
#define SSIZE_T_MAX __MAXINT__(ssize_t)	/* XXX Assumes two's complement */
#endif

#define DICT_LMDB_SIZE_INCR	2	/* Increase size by 1 bit on retry */
#define DICT_LMDB_SIZE_MAX	SSIZE_T_MAX

#define DICT_LMDB_API_RETRY_LIMIT 2	/* Retries per dict(3) API call */
#define DICT_LMDB_BULK_RETRY_LIMIT \
	((int) (2 * sizeof(size_t) * CHAR_BIT))	/* Retries per bulk-mode
						 * transaction */

size_t  dict_lmdb_map_size = 8192;	/* Minimum size without SIGSEGV */

/* #define msg_verbose 1 */

/* dict_lmdb_lookup - find database entry */

static const char *dict_lmdb_lookup(DICT *dict, const char *name)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) dict;
    MDB_val mdb_key;
    MDB_val mdb_value;
    const char *result = 0;
    int     status, klen;

    dict->error = 0;
    klen = strlen(name);

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_lmdb_lookup: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
     * Acquire a shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
      && myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict->name);

    /*
     * See if this LMDB file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	mdb_key.mv_data = (void *) name;
	mdb_key.mv_size = klen + 1;
	status = slmdb_get(&dict_lmdb->slmdb, &mdb_key, &mdb_value);
	if (status == 0) {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;
	    result = SCOPY(dict_lmdb->val_buf, mdb_value.mv_data,
			   mdb_value.mv_size);
	} else if (status != MDB_NOTFOUND) {
	    msg_fatal("error reading %s:%s: %s",
		      dict_lmdb->dict.type, dict_lmdb->dict.name,
		      mdb_strerror(status));
	}
    }

    /*
     * See if this LMDB file was written with no null byte appended to key
     * and value.
     */
    if (result == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	mdb_key.mv_data = (void *) name;
	mdb_key.mv_size = klen;
	status = slmdb_get(&dict_lmdb->slmdb, &mdb_key, &mdb_value);
	if (status == 0) {
	    dict->flags &= ~DICT_FLAG_TRY1NULL;
	    result = SCOPY(dict_lmdb->val_buf, mdb_value.mv_data,
			   mdb_value.mv_size);
	} else if (status != MDB_NOTFOUND) {
	    msg_fatal("error reading %s:%s: %s",
		      dict_lmdb->dict.type, dict_lmdb->dict.name,
		      mdb_strerror(status));
	}
    }

    /*
     * Release the shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict->name);

    return (result);
}

/* dict_lmdb_update - add or update database entry */

static int dict_lmdb_update(DICT *dict, const char *name, const char *value)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) dict;
    MDB_val mdb_key;
    MDB_val mdb_value;
    int     status;

    dict->error = 0;

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_lmdb_update: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

    /*
     * Optionally fold the key.
     */
    if (dict->flags & DICT_FLAG_FOLD_FIX) {
	if (dict->fold_buf == 0)
	    dict->fold_buf = vstring_alloc(10);
	vstring_strcpy(dict->fold_buf, name);
	name = lowercase(vstring_str(dict->fold_buf));
    }
    mdb_key.mv_data = (void *) name;

    mdb_value.mv_data = (void *) value;
    mdb_key.mv_size = strlen(name);
    mdb_value.mv_size = strlen(value);

    /*
     * If undecided about appending a null byte to key and value, choose a
     * default depending on the platform.
     */
    if ((dict->flags & DICT_FLAG_TRY1NULL)
	&& (dict->flags & DICT_FLAG_TRY0NULL)) {
#ifdef LMDB_NO_TRAILING_NULL
	dict->flags &= ~DICT_FLAG_TRY1NULL;
#else
	dict->flags &= ~DICT_FLAG_TRY0NULL;
#endif
    }

    /*
     * Optionally append a null byte to key and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	mdb_key.mv_size++;
	mdb_value.mv_size++;
    }

    /*
     * Acquire an exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
    && myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict->name);

    /*
     * Do the update.
     */
    status = slmdb_put(&dict_lmdb->slmdb, &mdb_key, &mdb_value,
	       (dict->flags & DICT_FLAG_DUP_REPLACE) ? 0 : MDB_NOOVERWRITE);
    if (status != 0) {
	if (status == MDB_KEYEXIST) {
	    if (dict->flags & DICT_FLAG_DUP_IGNORE)
		 /* void */ ;
	    else if (dict->flags & DICT_FLAG_DUP_WARN)
		msg_warn("%s:%s: duplicate entry: \"%s\"",
			 dict_lmdb->dict.type, dict_lmdb->dict.name, name);
	    else
		msg_fatal("%s:%s: duplicate entry: \"%s\"",
			  dict_lmdb->dict.type, dict_lmdb->dict.name, name);
	} else {
	    msg_fatal("error updating %s:%s: %s",
		      dict_lmdb->dict.type, dict_lmdb->dict.name,
		      mdb_strerror(status));
	}
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict->name);

    return (status);
}

/* dict_lmdb_delete - delete one entry from the dictionary */

static int dict_lmdb_delete(DICT *dict, const char *name)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) dict;
    MDB_val mdb_key;
    int     status = 1, klen;

    dict->error = 0;
    klen = strlen(name);

    /*
     * Sanity check.
     */
    if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
	msg_panic("dict_lmdb_delete: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

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
    && myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_EXCLUSIVE) < 0)
	msg_fatal("%s: lock dictionary: %m", dict->name);

    /*
     * See if this LMDB file was written with one null byte appended to key
     * and value.
     */
    if (dict->flags & DICT_FLAG_TRY1NULL) {
	mdb_key.mv_data = (void *) name;
	mdb_key.mv_size = klen + 1;
	status = slmdb_del(&dict_lmdb->slmdb, &mdb_key);
	if (status != 0) {
	    if (status == MDB_NOTFOUND)
		status = 1;
	    else
		msg_fatal("error deleting from %s:%s: %s",
			  dict_lmdb->dict.type, dict_lmdb->dict.name,
			  mdb_strerror(status));
	} else {
	    dict->flags &= ~DICT_FLAG_TRY0NULL;	/* found */
	}
    }

    /*
     * See if this LMDB file was written with no null byte appended to key
     * and value.
     */
    if (status > 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
	mdb_key.mv_data = (void *) name;
	mdb_key.mv_size = klen;
	status = slmdb_del(&dict_lmdb->slmdb, &mdb_key);
	if (status != 0) {
	    if (status == MDB_NOTFOUND)
		status = 1;
	    else
		msg_fatal("error deleting from %s:%s: %s",
			  dict_lmdb->dict.type, dict_lmdb->dict.name,
			  mdb_strerror(status));
	} else {
	    dict->flags &= ~DICT_FLAG_TRY1NULL;	/* found */
	}
    }

    /*
     * Release the exclusive lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict->name);

    return (status);
}

/* dict_lmdb_sequence - traverse the dictionary */

static int dict_lmdb_sequence(DICT *dict, int function,
			              const char **key, const char **value)
{
    const char *myname = "dict_lmdb_sequence";
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) dict;
    MDB_val mdb_key;
    MDB_val mdb_value;
    MDB_cursor_op op;
    int     status;

    dict->error = 0;

    /*
     * Determine the seek function.
     */
    switch (function) {
    case DICT_SEQ_FUN_FIRST:
	op = MDB_FIRST;
	break;
    case DICT_SEQ_FUN_NEXT:
	op = MDB_NEXT;
	break;
    default:
	msg_panic("%s: invalid function: %d", myname, function);
    }

    /*
     * Acquire a shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
      && myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_SHARED) < 0)
	msg_fatal("%s: lock dictionary: %m", dict->name);

    /*
     * Database lookup.
     */
    status = slmdb_cursor_get(&dict_lmdb->slmdb, &mdb_key, &mdb_value, op);

    switch (status) {

	/*
	 * Copy the key and value so they are guaranteed null terminated.
	 */
    case 0:
	*key = SCOPY(dict_lmdb->key_buf, mdb_key.mv_data, mdb_key.mv_size);
	if (mdb_value.mv_data != 0 && mdb_value.mv_size > 0)
	    *value = SCOPY(dict_lmdb->val_buf, mdb_value.mv_data,
			   mdb_value.mv_size);
	break;

	/*
	 * End-of-database.
	 */
    case MDB_NOTFOUND:
	status = 1;
	/* Not: mdb_cursor_close(). Wrong abstraction level. */
	break;

	/*
	 * Bust.
	 */
    default:
	msg_fatal("error seeking %s:%s: %s",
		  dict_lmdb->dict.type, dict_lmdb->dict.name,
		  mdb_strerror(status));
    }

    /*
     * Release the shared lock.
     */
    if ((dict->flags & DICT_FLAG_LOCK)
	&& myflock(dict->lock_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_NONE) < 0)
	msg_fatal("%s: unlock dictionary: %m", dict->name);

    return (status);
}

/* dict_lmdb_close - disassociate from data base */

static void dict_lmdb_close(DICT *dict)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) dict;

    slmdb_close(&dict_lmdb->slmdb);
    if (dict_lmdb->key_buf)
	vstring_free(dict_lmdb->key_buf);
    if (dict_lmdb->val_buf)
	vstring_free(dict_lmdb->val_buf);
    if (dict->fold_buf)
	vstring_free(dict->fold_buf);
    dict_free(dict);
}

/* dict_lmdb_longjmp - repeat bulk transaction */

static void dict_lmdb_longjmp(void *context, int val)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) context;

    dict_longjmp(&dict_lmdb->dict, val);
}

/* dict_lmdb_notify - debug logging */

static void dict_lmdb_notify(void *context, int error_code,...)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) context;
    va_list ap;

    va_start(ap, error_code);
    switch (error_code) {
    case MDB_SUCCESS:
	msg_info("database %s:%s: using size limit %lu during open",
		 dict_lmdb->dict.type, dict_lmdb->dict.name,
		 (unsigned long) va_arg(ap, size_t));
	break;
    case MDB_MAP_FULL:
	msg_info("database %s:%s: using size limit %lu after MDB_MAP_FULL",
		 dict_lmdb->dict.type, dict_lmdb->dict.name,
		 (unsigned long) va_arg(ap, size_t));
	break;
    case MDB_MAP_RESIZED:
	msg_info("database %s:%s: using size limit %lu after MDB_MAP_RESIZED",
		 dict_lmdb->dict.type, dict_lmdb->dict.name,
		 (unsigned long) va_arg(ap, size_t));
	break;
    case MDB_READERS_FULL:
	msg_info("database %s:%s: pausing after MDB_READERS_FULL",
		 dict_lmdb->dict.type, dict_lmdb->dict.name);
	break;
    default:
	msg_warn("unknown MDB error code: %d", error_code);
	break;
    }
    va_end(ap);
}

/* dict_lmdb_assert - report LMDB internal assertion failure */

static void dict_lmdb_assert(void *context, const char *text)
{
    DICT_LMDB *dict_lmdb = (DICT_LMDB *) context;

    msg_fatal("%s:%s: internal error: %s",
	      dict_lmdb->dict.type, dict_lmdb->dict.name, text);
}

/* dict_lmdb_open - open LMDB data base */

DICT   *dict_lmdb_open(const char *path, int open_flags, int dict_flags)
{
    DICT_LMDB *dict_lmdb;
    DICT   *dict;
    struct stat st;
    SLMDB   slmdb;
    char   *mdb_path;
    int     mdb_flags, slmdb_flags, status;
    int     db_fd;

    /*
     * Let the optimizer worry about eliminating redundant code.
     */
#define DICT_LMDB_OPEN_RETURN(d) { \
	DICT *__d = (d); \
	myfree(mdb_path); \
	return (__d); \
    } while (0)

    mdb_path = concatenate(path, "." DICT_TYPE_LMDB, (char *) 0);

    /*
     * Impedance adapters.
     */
    mdb_flags = MDB_NOSUBDIR | MDB_NOLOCK;
    if (open_flags == O_RDONLY)
	mdb_flags |= MDB_RDONLY;

    slmdb_flags = 0;
    if (dict_flags & DICT_FLAG_BULK_UPDATE)
	slmdb_flags |= SLMDB_FLAG_BULK;

    /*
     * Security violation.
     * 
     * By default, LMDB 0.9.9 writes uninitialized heap memory to a
     * world-readable database file, as chunks of up to 4096 bytes. This is a
     * huge memory disclosure vulnerability: memory content that a program
     * does not intend to share ends up in a world-readable file. The content
     * of uninitialized heap memory depends on program execution history.
     * That history includes code execution in other libraries that are
     * linked into the program.
     * 
     * This is a problem whenever the user who writes the database file differs
     * from the user who reads the database file. For example, a privileged
     * writer and an unprivileged reader. In the case of Postfix, the
     * postmap(1) and postalias(1) commands would leak uninitialized heap
     * memory, as chunks of up to 4096 bytes, from a root-privileged process
     * that writes to a database file, to unprivileged processes that read
     * from that database file.
     * 
     * As a workaround the postmap(1) and postalias(1) commands turn on
     * MDB_WRITEMAP which disables the use of malloc() in LMDB. However, that
     * does not address several disclosures of stack memory. We don't enable
     * this workaround for Postfix databases are maintained by Postfix daemon
     * processes, because those are accessible only by the postfix user.
     * 
     * LMDB 0.9.10 by default does not write uninitialized heap memory to file
     * (specify MDB_NOMEMINIT to revert that change). We use the MDB_WRITEMAP
     * workaround for older LMDB versions.
     */
#ifndef MDB_NOMEMINIT
    if (dict_flags & DICT_FLAG_BULK_UPDATE)	/* XXX Good enough */
	mdb_flags |= MDB_WRITEMAP;
#endif

    /*
     * Gracefully handle most database open errors.
     */
    if ((status = slmdb_init(&slmdb, dict_lmdb_map_size, DICT_LMDB_SIZE_INCR,
			     DICT_LMDB_SIZE_MAX)) != 0
	|| (status = slmdb_open(&slmdb, mdb_path, open_flags, mdb_flags,
				slmdb_flags)) != 0) {
	/* This leaks a little memory that would have been used otherwise. */
	dict = dict_surrogate(DICT_TYPE_LMDB, path, open_flags, dict_flags,
		    "open database %s: %s", mdb_path, mdb_strerror(status));
	DICT_LMDB_OPEN_RETURN(dict);
    }

    /*
     * XXX Persistent locking belongs in mkmap_lmdb.
     * 
     * We just need to acquire exclusive access momentarily. This establishes
     * that no readers are accessing old (obsoleted by copy-on-write) txn
     * snapshots, so we are free to reuse all eligible old pages. Downgrade
     * the lock right after acquiring it. This is sufficient to keep out
     * other writers until we are done.
     */
    db_fd = slmdb_fd(&slmdb);
    if (dict_flags & DICT_FLAG_BULK_UPDATE) {
	if (myflock(db_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_EXCLUSIVE) < 0)
	    msg_fatal("%s: lock dictionary: %m", mdb_path);
	if (myflock(db_fd, MYFLOCK_STYLE_FCNTL, MYFLOCK_OP_SHARED) < 0)
	    msg_fatal("%s: unlock dictionary: %m", mdb_path);
    }

    /*
     * Bundle up. From here on no more assignments to slmdb.
     */
    dict_lmdb = (DICT_LMDB *) dict_alloc(DICT_TYPE_LMDB, path, sizeof(*dict_lmdb));
    dict_lmdb->slmdb = slmdb;
    dict_lmdb->dict.lookup = dict_lmdb_lookup;
    dict_lmdb->dict.update = dict_lmdb_update;
    dict_lmdb->dict.delete = dict_lmdb_delete;
    dict_lmdb->dict.sequence = dict_lmdb_sequence;
    dict_lmdb->dict.close = dict_lmdb_close;

    if (fstat(db_fd, &st) < 0)
	msg_fatal("dict_lmdb_open: fstat: %m");
    dict_lmdb->dict.lock_fd = dict_lmdb->dict.stat_fd = db_fd;
    dict_lmdb->dict.lock_type = MYFLOCK_STYLE_FCNTL;
    dict_lmdb->dict.mtime = st.st_mtime;
    dict_lmdb->dict.owner.uid = st.st_uid;
    dict_lmdb->dict.owner.status = (st.st_uid != 0);

    dict_lmdb->key_buf = 0;
    dict_lmdb->val_buf = 0;

    /*
     * Warn if the source file is newer than the indexed file, except when
     * the source file changed only seconds ago.
     */
    if ((dict_flags & DICT_FLAG_LOCK) != 0
	&& stat(path, &st) == 0
	&& st.st_mtime > dict_lmdb->dict.mtime
	&& st.st_mtime < time((time_t *) 0) - 100)
	msg_warn("database %s is older than source file %s", mdb_path, path);

#define DICT_LMDB_IMPL_FLAGS	(DICT_FLAG_FIXED | DICT_FLAG_MULTI_WRITER)

    dict_lmdb->dict.flags = dict_flags | DICT_LMDB_IMPL_FLAGS;
    if ((dict_flags & (DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL)) == 0)
	dict_lmdb->dict.flags |= (DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL);
    if (dict_flags & DICT_FLAG_FOLD_FIX)
	dict_lmdb->dict.fold_buf = vstring_alloc(10);

    if (dict_flags & DICT_FLAG_BULK_UPDATE)
	dict_jmp_alloc(&dict_lmdb->dict);

    /*
     * The following requests return an error result only if we have serious
     * memory corruption problem.
     */
    if (slmdb_control(&dict_lmdb->slmdb,
		      SLMDB_CTL_API_RETRY_LIMIT, DICT_LMDB_API_RETRY_LIMIT,
		      SLMDB_CTL_BULK_RETRY_LIMIT, DICT_LMDB_BULK_RETRY_LIMIT,
		      SLMDB_CTL_LONGJMP_FN, dict_lmdb_longjmp,
		      SLMDB_CTL_NOTIFY_FN, msg_verbose ?
		      dict_lmdb_notify : (SLMDB_NOTIFY_FN) 0,
		      SLMDB_CTL_ASSERT_FN, dict_lmdb_assert,
		      SLMDB_CTL_CB_CONTEXT, (void *) dict_lmdb,
		      SLMDB_CTL_END) != 0)
	msg_panic("dict_lmdb_open: slmdb_control: %m");

    if (msg_verbose)
	dict_lmdb_notify((void *) dict_lmdb, MDB_SUCCESS,
			 slmdb_curr_limit(&dict_lmdb->slmdb));

    DICT_LMDB_OPEN_RETURN(DICT_DEBUG (&dict_lmdb->dict));
}

#endif
