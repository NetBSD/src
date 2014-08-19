/*	$NetBSD: slmdb.c,v 1.1.1.1.6.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	slmdb 3
/* SUMMARY
/*	Simplified LMDB API
/* SYNOPSIS
/*	#include <slmdb.h>
/*
/*	int	slmdb_init(slmdb, curr_limit, size_incr, hard_limit)
/*	SLMDB	*slmdb;
/*	size_t	curr_limit;
/*	int	size_incr;
/*	size_t	hard_limit;
/*
/*	int	slmdb_open(slmdb, path, open_flags, lmdb_flags, slmdb_flags)
/*	SLMDB	*slmdb;
/*	const char *path;
/*	int	open_flags;
/*	int	lmdb_flags;
/*	int	slmdb_flags;
/*
/*	int	slmdb_close(slmdb)
/*	SLMDB	*slmdb;
/*
/*	int	slmdb_get(slmdb, mdb_key, mdb_value)
/*	SLMDB	*slmdb;
/*	MDB_val	*mdb_key;
/*	MDB_val	*mdb_value;
/*
/*	int	slmdb_put(slmdb, mdb_key, mdb_value, flags)
/*	SLMDB	*slmdb;
/*	MDB_val	*mdb_key;
/*	MDB_val	*mdb_value;
/*	int	flags;
/*
/*	int	slmdb_del(slmdb, mdb_key)
/*	SLMDB	*slmdb;
/*	MDB_val	*mdb_key;
/*
/*	int	slmdb_cursor_get(slmdb, mdb_key, mdb_value, op)
/*	SLMDB	*slmdb;
/*	MDB_val	*mdb_key;
/*	MDB_val	*mdb_value;
/*	MDB_cursor_op op;
/* AUXILIARY FUNCTIONS
/*	int	slmdb_fd(slmdb)
/*	SLMDB	*slmdb;
/*
/*	size_t	slmdb_curr_limit(slmdb)
/*	SLMDB	*slmdb;
/*
/*	int	slmdb_control(slmdb, request, ...)
/*	SLMDB	*slmdb;
/*	int	request;
/* DESCRIPTION
/*	This module simplifies the LMDB API by hiding recoverable
/*	errors from the application.  Details are given in the
/*	section "ERROR RECOVERY".
/*
/*	slmdb_init() performs mandatory initialization before opening
/*	an LMDB database. The result value is an LMDB status code
/*	(zero in case of success).
/*
/*	slmdb_open() opens an LMDB database.  The result value is
/*	an LMDB status code (zero in case of success).
/*
/*	slmdb_close() finalizes an optional bulk-mode transaction
/*	and closes a successfully-opened LMDB database.  The result
/*	value is an LMDB status code (zero in case of success).
/*
/*	slmdb_get() is an mdb_get() wrapper with automatic error
/*	recovery.  The result value is an LMDB status code (zero
/*	in case of success).
/*
/*	slmdb_put() is an mdb_put() wrapper with automatic error
/*	recovery.  The result value is an LMDB status code (zero
/*	in case of success).
/*
/*	slmdb_del() is an mdb_del() wrapper with automatic error
/*	recovery.  The result value is an LMDB status code (zero
/*	in case of success).
/*
/*	slmdb_cursor_get() is an mdb_cursor_get() wrapper with
/*	automatic error recovery.  The result value is an LMDB
/*	status code (zero in case of success). This wrapper supports
/*	only one cursor per database.
/*
/*	slmdb_fd() returns the file descriptor for the specified
/*	database.  This may be used for file status queries or
/*	application-controlled locking.
/*
/*	slmdb_curr_limit() returns the current database size limit
/*	for the specified database.
/*
/*	slmdb_control() specifies optional features. The result is
/*	an LMDB status code (zero in case of success).
/*
/* Arguments:
/* .IP slmdb
/*	Pointer to caller-provided storage.
/* .IP curr_limit
/*	The initial memory mapping size limit. This limit is
/*	automatically increased when the database becomes full.
/* .IP size_incr
/*	An integer factor by which the memory mapping size limit
/*	is increased when the database becomes full.
/* .IP hard_limit
/*	The upper bound for the memory mapping size limit.
/* .IP path
/*	LMDB database pathname.
/* .IP open_flags
/*	Flags that control file open operations. Do not specify
/*	locking flags here.
/* .IP lmdb_flags
/*	Flags that control the LMDB environment. If MDB_NOLOCK is
/*	specified, then each slmdb_get() or slmdb_cursor_get() call
/*	must be protected with a shared (or exclusive) external lock,
/*	and each slmdb_put() or slmdb_del() call must be protected
/*	with an exclusive external lock. A lock may be released
/*	after the call returns. A writer may atomically downgrade
/*	an exclusive lock to shared, but it must obtain an exclusive
/*	lock before making another slmdb(3) write request.
/* .sp
/*	Note: when a database is opened with MDB_NOLOCK, external
/*	locks such as fcntl() do not protect slmdb(3) requests
/*	within the same process against each other.  If a program
/*	cannot avoid making simultaneous slmdb(3) requests, then
/*	it must synchronize these requests with in-process locks,
/*	in addition to the per-process fcntl(2) locks.
/* .IP slmdb_flags
/*	Bit-wise OR of zero or more of the following:
/* .RS
/* .IP SLMDB_FLAG_BULK
/*	Open the database and create a "bulk" transaction that is
/*	committed when the database is closed. If MDB_NOLOCK is
/*	specified, then the entire transaction must be protected
/*	with a persistent external lock.  All slmdb_get(), slmdb_put()
/*	and slmdb_del() requests will be directed to the "bulk"
/*	transaction.
/* .RE
/* .IP mdb_key
/*	Pointer to caller-provided lookup key storage.
/* .IP mdb_value
/*	Pointer to caller-provided value storage.
/* .IP op
/*	LMDB cursor operation.
/* .IP request
/*	The start of a list of (name, value) pairs, terminated with
/*	SLMDB_CTL_END.  The following text enumerates the symbolic
/*	request names and the corresponding value types.
/* .RS
/* .IP "SLMDB_CTL_LONGJMP_FN (void (*)(void *, int))
/*	Call-back function pointer. The function is called to repeat
/*	a failed bulk-mode transaction from the start. The arguments
/*	are the application context and the setjmp() or sigsetjmp()
/*	result value.
/* .IP "SLMDB_CTL_NOTIFY_FN (void (*)(void *, int, ...))"
/*	Call-back function pointer. The function is called to report
/*	succesful error recovery. The arguments are the application
/*	context, the MDB error code, and additional arguments that
/*	depend on the error code.  Details are given in the section
/*	"ERROR RECOVERY".
/* .IP "SLMDB_CTL_ASSERT_FN (void (*)(void *, const char *))"
/*	Call-back function pointer.  The function is called to
/*	report an LMDB internal assertion failure. The arguments
/*	are the application context, and text that describes the
/*	problem.
/* .IP "SLMDB_CTL_CB_CONTEXT (void *)"
/*	Application context that is passed in call-back function
/*	calls.
/* .IP "SLMDB_CTL_API_RETRY_LIMIT (int)"
/*	How many times to recover from LMDB errors within the
/*	execution of a single slmdb(3) API call before giving up.
/* .IP "SLMDB_CTL_BULK_RETRY_LIMIT (int)"
/*	How many times to recover from a bulk-mode transaction
/*	before giving up.
/* .RE
/* ERROR RECOVERY
/* .ad
/* .fi
/*	This module automatically repeats failed requests after
/*	recoverable errors, up to the limits specified with
/*	slmdb_control().
/*
/*	Recoverable errors are reported through an optional
/*	notification function specified with slmdb_control().  With
/*	recoverable MDB_MAP_FULL and MDB_MAP_RESIZED errors, the
/*	additional argument is a size_t value with the updated
/*	current database size limit; with recoverable MDB_READERS_FULL
/*	errors there is no additional argument.
/* BUGS
/*	Recovery from MDB_MAP_FULL involves resizing the database
/*	memory mapping.  According to LMDB documentation this
/*	requires that there is no concurrent activity in the same
/*	database by other threads in the same memory address space.
/* SEE ALSO
/*	lmdb(3) API manpage (currently, non-existent).
/* AUTHOR(S)
/*	Howard Chu
/*	Symas Corporation
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

 /*
  * DO NOT include other Postfix-specific header files. This LMDB wrapper
  * must be usable outside Postfix.
  */

#ifdef HAS_LMDB

/* System library. */

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

/* Application-specific. */

#include <slmdb.h>

 /*
  * Minimum LMDB patchlevel.
  * 
  * LMDB 0.9.11 allows Postfix daemons to log an LMDB error message instead of
  * falling out of the sky without any explanation. Without such logging,
  * Postfix with LMDB would be too hard to support.
  * 
  * LMDB 0.9.10 fixes an information leak where LMDB wrote chunks of up to 4096
  * bytes of uninitialized heap memory to a database. This was a security
  * violation because it made information persistent that was not meant to be
  * persisted, or it was sharing information that was not meant to be shared.
  * 
  * LMDB 0.9.9 allows Postfix to use external (fcntl()-based) locks, instead of
  * having to use world-writable LMDB lock files.
  * 
  * LMDB 0.9.8 allows Postfix to update the database size limit on-the-fly, so
  * that it can recover from an MDB_MAP_FULL error without having to close
  * the database. It also allows an application to "pick up" a new database
  * size limit on-the-fly, so that it can recover from an MDB_MAP_RESIZED
  * error without having to close the database.
  * 
  * The database size limit that remains is imposed by the hardware memory
  * address space (31 or 47 bits, typically) or file system. The LMDB
  * implementation is supposed to handle databases larger than physical
  * memory. However, this is not necessarily guaranteed for (bulk)
  * transactions larger than physical memory.
  */
#if MDB_VERSION_FULL < MDB_VERINT(0, 9, 11)
#error "This Postfix version requires LMDB version 0.9.11 or later"
#endif

 /*
  * Error recovery.
  * 
  * The purpose of the slmdb(3) API is to hide LMDB quirks (recoverable
  * MAP_FULL, MAP_RESIZED, or MDB_READERS_FULL errors). With these out of the
  * way, applications can pretend that those quirks don't exist, and focus on
  * their own job.
  * 
  * - To recover from a single-transaction LMDB error, each wrapper function
  * uses tail recursion instead of goto. Since LMDB errors are rare, code
  * clarity is more important than speed.
  * 
  * - To recover from a bulk-transaction LMDB error, the error-recovery code
  * triggers a long jump back into the caller to some pre-arranged point (the
  * closest thing that C has to exception handling). The application is then
  * expected to repeat the bulk transaction from scratch.
  */

 /*
  * Our default retry attempt limits. We allow a few retries per slmdb(3) API
  * call for non-bulk transactions. We allow a number of bulk-transaction
  * retries that is proportional to the memory address space.
  */
#define SLMDB_DEF_API_RETRY_LIMIT 30	/* Retries per slmdb(3) API call */
#define SLMDB_DEF_BULK_RETRY_LIMIT \
        (2 * sizeof(size_t) * CHAR_BIT)	/* Retries per bulk-mode transaction */

 /*
  * We increment the recursion counter each time we try to recover from
  * error, and reset the recursion counter when returning to the application
  * from the slmdb(3) API.
  */
#define SLMDB_API_RETURN(slmdb, status) do { \
	(slmdb)->api_retry_count = 0; \
	return (status); \
    } while (0)

 /*
  * With MDB_NOLOCK, the application uses an external lock for inter-process
  * synchronization. Because the caller may release the external lock after
  * an SLMDB API call, each SLMDB API function must use a short-lived
  * transaction unless the transaction is a bulk-mode transaction.
  */

/* slmdb_cursor_close - close cursor and its read transaction */

static void slmdb_cursor_close(SLMDB *slmdb)
{
    MDB_txn *txn;

    /*
     * Close the cursor and its read transaction. We can restore it later
     * from the saved key information.
     */
    txn = mdb_cursor_txn(slmdb->cursor);
    mdb_cursor_close(slmdb->cursor);
    slmdb->cursor = 0;
    mdb_txn_abort(txn);
}

/* slmdb_saved_key_init - initialize saved key info */

static void slmdb_saved_key_init(SLMDB *slmdb)
{
    slmdb->saved_key.mv_data = 0;
    slmdb->saved_key.mv_size = 0;
    slmdb->saved_key_size = 0;
}

/* slmdb_saved_key_free - destroy saved key info */

static void slmdb_saved_key_free(SLMDB *slmdb)
{
    free(slmdb->saved_key.mv_data);
    slmdb_saved_key_init(slmdb);
}

#define HAVE_SLMDB_SAVED_KEY(s) ((s)->saved_key.mv_data != 0)

/* slmdb_saved_key_assign - copy the saved key */

static int slmdb_saved_key_assign(SLMDB *slmdb, MDB_val *key_val)
{

    /*
     * Extend the buffer to fit the key, so that we can avoid malloc()
     * overhead most of the time.
     */
    if (slmdb->saved_key_size < key_val->mv_size) {
	if (slmdb->saved_key.mv_data == 0)
	    slmdb->saved_key.mv_data = malloc(key_val->mv_size);
	else
	    slmdb->saved_key.mv_data =
		realloc(slmdb->saved_key.mv_data, key_val->mv_size);
	if (slmdb->saved_key.mv_data == 0) {
	    slmdb_saved_key_init(slmdb);
	    return (ENOMEM);
	} else {
	    slmdb->saved_key_size = key_val->mv_size;
	}
    }

    /*
     * Copy the key under the cursor.
     */
    memcpy(slmdb->saved_key.mv_data, key_val->mv_data, key_val->mv_size);
    slmdb->saved_key.mv_size = key_val->mv_size;
    return (0);
}

/* slmdb_prepare - LMDB-specific (re)initialization before actual access */

static int slmdb_prepare(SLMDB *slmdb)
{
    int     status = 0;

    /*
     * This is called before accessing the database, or after recovery from
     * an LMDB error. Note: this code cannot recover from errors itself.
     * slmdb->txn is either the database open() transaction or a
     * freshly-created bulk-mode transaction.
     * 
     * - With O_TRUNC we make a "drop" request before updating the database.
     * 
     * - With a bulk-mode transaction we commit when the database is closed.
     */
    if (slmdb->open_flags & O_TRUNC) {
	if ((status = mdb_drop(slmdb->txn, slmdb->dbi, 0)) != 0)
	    return (status);
	if ((slmdb->slmdb_flags & SLMDB_FLAG_BULK) == 0) {
	    if ((status = mdb_txn_commit(slmdb->txn)) != 0)
		return (status);
	    slmdb->txn = 0;
	}
    } else if ((slmdb->lmdb_flags & MDB_RDONLY) != 0
	       || (slmdb->slmdb_flags & SLMDB_FLAG_BULK) == 0) {
	mdb_txn_abort(slmdb->txn);
	slmdb->txn = 0;
    }
    slmdb->api_retry_count = 0;
    return (status);
}

/* slmdb_recover - recover from LMDB errors */

static int slmdb_recover(SLMDB *slmdb, int status)
{
    MDB_envinfo info;

    /*
     * This may be needed in non-MDB_NOLOCK mode. Recovery is rare enough
     * that we don't care about a few wasted cycles.
     */
    if (slmdb->cursor != 0)
	slmdb_cursor_close(slmdb);

    /*
     * Recover bulk transactions only if they can be restarted. Limit the
     * number of recovery attempts per slmdb(3) API request.
     */
    if ((slmdb->txn != 0 && slmdb->longjmp_fn == 0)
	|| ((slmdb->api_retry_count += 1) >= slmdb->api_retry_limit))
	return (status);

    /*
     * If we can recover from the error, we clear the error condition and the
     * caller should retry the failed operation immediately. Otherwise, the
     * caller should terminate with a fatal run-time error and the program
     * should be re-run later.
     * 
     * slmdb->txn must be either null (non-bulk transaction error), or an
     * aborted bulk-mode transaction.
     */
    switch (status) {

	/*
	 * As of LMDB 0.9.8 when a non-bulk update runs into a "map full"
	 * error, we can resize the environment's memory map and clear the
	 * error condition. The caller should retry immediately.
	 */
    case MDB_MAP_FULL:
	/* Can we increase the memory map? Give up if we can't. */
	if (slmdb->curr_limit < slmdb->hard_limit / slmdb->size_incr) {
	    slmdb->curr_limit = slmdb->curr_limit * slmdb->size_incr;
	} else if (slmdb->curr_limit < slmdb->hard_limit) {
	    slmdb->curr_limit = slmdb->hard_limit;
	} else {
	    /* Sorry, we are already maxed out. */
	    break;
	}
	if (slmdb->notify_fn)
	    slmdb->notify_fn(slmdb->cb_context, MDB_MAP_FULL,
			     slmdb->curr_limit);
	status = mdb_env_set_mapsize(slmdb->env, slmdb->curr_limit);
	break;

	/*
	 * When a writer resizes the database, read-only applications must
	 * increase their LMDB memory map size limit, too. Otherwise, they
	 * won't be able to read a table after it grows.
	 * 
	 * As of LMDB 0.9.8 we can import the new memory map size limit into the
	 * database environment by calling mdb_env_set_mapsize() with a zero
	 * size argument. Then we extract the map size limit for later use.
	 * The caller should retry immediately.
	 */
    case MDB_MAP_RESIZED:
	if ((status = mdb_env_set_mapsize(slmdb->env, 0)) == 0) {
	    /* Do not panic. Maps may shrink after bulk update. */
	    mdb_env_info(slmdb->env, &info);
	    slmdb->curr_limit = info.me_mapsize;
	    if (slmdb->notify_fn)
		slmdb->notify_fn(slmdb->cb_context, MDB_MAP_RESIZED,
				 slmdb->curr_limit);
	}
	break;

	/*
	 * What is it with these built-in hard limits that cause systems to
	 * stop when demand is at its highest? When the system is under
	 * stress it should slow down and keep making progress.
	 */
    case MDB_READERS_FULL:
	if (slmdb->notify_fn)
	    slmdb->notify_fn(slmdb->cb_context, MDB_READERS_FULL);
	sleep(1);
	status = 0;
	break;

	/*
	 * We can't solve this problem. The application should terminate with
	 * a fatal run-time error and the program should be re-run later.
	 */
    default:
	break;
    }

    /*
     * If a bulk-transaction error is recoverable, build a new bulk
     * transaction from scratch, by making a long jump back into the caller
     * at some pre-arranged point. In MDB_NOLOCK mode, there is no need to
     * upgrade the lock to "exclusive", because the failed write transaction
     * has no side effects.
     */
    if (slmdb->txn != 0 && status == 0 && slmdb->longjmp_fn != 0
	&& (slmdb->bulk_retry_count += 1) <= slmdb->bulk_retry_limit) {
	if ((status = mdb_txn_begin(slmdb->env, (MDB_txn *) 0,
				    slmdb->lmdb_flags & MDB_RDONLY,
				    &slmdb->txn)) == 0
	    && (status = slmdb_prepare(slmdb)) == 0)
	    slmdb->longjmp_fn(slmdb->cb_context, 1);
    }
    return (status);
}

/* slmdb_txn_begin - mdb_txn_begin() wrapper with LMDB error recovery */

static int slmdb_txn_begin(SLMDB *slmdb, int rdonly, MDB_txn **txn)
{
    int     status;

    if ((status = mdb_txn_begin(slmdb->env, (MDB_txn *) 0, rdonly, txn)) != 0
	&& (status = slmdb_recover(slmdb, status)) == 0)
	status = slmdb_txn_begin(slmdb, rdonly, txn);

    return (status);
}

/* slmdb_get - mdb_get() wrapper with LMDB error recovery */

int     slmdb_get(SLMDB *slmdb, MDB_val *mdb_key, MDB_val *mdb_value)
{
    MDB_txn *txn;
    int     status;

    /*
     * Start a read transaction if there's no bulk-mode txn.
     */
    if (slmdb->txn)
	txn = slmdb->txn;
    else if ((status = slmdb_txn_begin(slmdb, MDB_RDONLY, &txn)) != 0)
	SLMDB_API_RETURN(slmdb, status);

    /*
     * Do the lookup.
     */
    if ((status = mdb_get(txn, slmdb->dbi, mdb_key, mdb_value)) != 0
	&& status != MDB_NOTFOUND) {
	mdb_txn_abort(txn);
	if ((status = slmdb_recover(slmdb, status)) == 0)
	    status = slmdb_get(slmdb, mdb_key, mdb_value);
	SLMDB_API_RETURN(slmdb, status);
    }

    /*
     * Close the read txn if it's not the bulk-mode txn.
     */
    if (slmdb->txn == 0)
	mdb_txn_abort(txn);

    SLMDB_API_RETURN(slmdb, status);
}

/* slmdb_put - mdb_put() wrapper with LMDB error recovery */

int     slmdb_put(SLMDB *slmdb, MDB_val *mdb_key,
		          MDB_val *mdb_value, int flags)
{
    MDB_txn *txn;
    int     status;

    /*
     * Start a write transaction if there's no bulk-mode txn.
     */
    if (slmdb->txn)
	txn = slmdb->txn;
    else if ((status = slmdb_txn_begin(slmdb, 0, &txn)) != 0)
	SLMDB_API_RETURN(slmdb, status);

    /*
     * Do the update.
     */
    if ((status = mdb_put(txn, slmdb->dbi, mdb_key, mdb_value, flags)) != 0) {
	mdb_txn_abort(txn);
	if (status != MDB_KEYEXIST) {
	    if ((status = slmdb_recover(slmdb, status)) == 0)
		status = slmdb_put(slmdb, mdb_key, mdb_value, flags);
	    SLMDB_API_RETURN(slmdb, status);
	}
    }

    /*
     * Commit the transaction if it's not the bulk-mode txn.
     */
    if (status == 0 && slmdb->txn == 0 && (status = mdb_txn_commit(txn)) != 0
	&& (status = slmdb_recover(slmdb, status)) == 0)
	status = slmdb_put(slmdb, mdb_key, mdb_value, flags);

    SLMDB_API_RETURN(slmdb, status);
}

/* slmdb_del - mdb_del() wrapper with LMDB error recovery */

int     slmdb_del(SLMDB *slmdb, MDB_val *mdb_key)
{
    MDB_txn *txn;
    int     status;

    /*
     * Start a write transaction if there's no bulk-mode txn.
     */
    if (slmdb->txn)
	txn = slmdb->txn;
    else if ((status = slmdb_txn_begin(slmdb, 0, &txn)) != 0)
	SLMDB_API_RETURN(slmdb, status);

    /*
     * Do the update.
     */
    if ((status = mdb_del(txn, slmdb->dbi, mdb_key, (MDB_val *) 0)) != 0) {
	mdb_txn_abort(txn);
	if (status != MDB_NOTFOUND) {
	    if ((status = slmdb_recover(slmdb, status)) == 0)
		status = slmdb_del(slmdb, mdb_key);
	    SLMDB_API_RETURN(slmdb, status);
	}
    }

    /*
     * Commit the transaction if it's not the bulk-mode txn.
     */
    if (status == 0 && slmdb->txn == 0 && (status = mdb_txn_commit(txn)) != 0
	&& (status = slmdb_recover(slmdb, status)) == 0)
	status = slmdb_del(slmdb, mdb_key);

    SLMDB_API_RETURN(slmdb, status);
}

/* slmdb_cursor_get - mdb_cursor_get() wrapper with LMDB error recovery */

int     slmdb_cursor_get(SLMDB *slmdb, MDB_val *mdb_key,
			         MDB_val *mdb_value, MDB_cursor_op op)
{
    MDB_txn *txn;
    int     status = 0;

    /*
     * Open a read transaction and cursor if needed.
     */
    if (slmdb->cursor == 0) {
	if ((status = slmdb_txn_begin(slmdb, MDB_RDONLY, &txn)) != 0)
	    SLMDB_API_RETURN(slmdb, status);
	if ((status = mdb_cursor_open(txn, slmdb->dbi, &slmdb->cursor)) != 0) {
	    mdb_txn_abort(txn);
	    if ((status = slmdb_recover(slmdb, status)) == 0)
		status = slmdb_cursor_get(slmdb, mdb_key, mdb_value, op);
	    SLMDB_API_RETURN(slmdb, status);
	}

	/*
	 * Restore the cursor position from the saved key information.
	 */
	if (HAVE_SLMDB_SAVED_KEY(slmdb) && op != MDB_FIRST)
	    status = mdb_cursor_get(slmdb->cursor, &slmdb->saved_key,
				    (MDB_val *) 0, MDB_SET);
    }

    /*
     * Database lookup.
     */
    if (status == 0)
	status = mdb_cursor_get(slmdb->cursor, mdb_key, mdb_value, op);

    /*
     * Save the cursor position if successful. This can fail only with
     * ENOMEM.
     * 
     * Close the cursor read transaction if in MDB_NOLOCK mode, because the
     * caller may release the external lock after we return.
     */
    if (status == 0) {
	status = slmdb_saved_key_assign(slmdb, mdb_key);
	if (slmdb->lmdb_flags & MDB_NOLOCK)
	    slmdb_cursor_close(slmdb);
    }

    /*
     * Handle end-of-database or other error.
     */
    else {
	/* Do not hand-optimize out the slmdb_cursor_close() calls below. */
	if (status == MDB_NOTFOUND) {
	    slmdb_cursor_close(slmdb);
	    if (HAVE_SLMDB_SAVED_KEY(slmdb))
		slmdb_saved_key_free(slmdb);
	} else {
	    slmdb_cursor_close(slmdb);
	    if ((status = slmdb_recover(slmdb, status)) == 0)
		status = slmdb_cursor_get(slmdb, mdb_key, mdb_value, op);
	    SLMDB_API_RETURN(slmdb, status);
	    /* Do not hand-optimize out the above return statement. */
	}
    }
    SLMDB_API_RETURN(slmdb, status);
}

/* slmdb_assert_cb - report LMDB assertion failure */

static void slmdb_assert_cb(MDB_env *env, const char *text)
{
    SLMDB  *slmdb = (SLMDB *) mdb_env_get_userctx(env);

    if (slmdb->assert_fn)
	slmdb->assert_fn(slmdb->cb_context, text);
}

/* slmdb_control - control optional settings */

int     slmdb_control(SLMDB *slmdb, int first,...)
{
    va_list ap;
    int     status = 0;
    int     reqno;
    int     rc;

    va_start(ap, first);
    for (reqno = first; status == 0 && reqno != SLMDB_CTL_END; reqno = va_arg(ap, int)) {
	switch (reqno) {
	case SLMDB_CTL_LONGJMP_FN:
	    slmdb->longjmp_fn = va_arg(ap, SLMDB_LONGJMP_FN);
	    break;
	case SLMDB_CTL_NOTIFY_FN:
	    slmdb->notify_fn = va_arg(ap, SLMDB_NOTIFY_FN);
	    break;
	case SLMDB_CTL_ASSERT_FN:
	    slmdb->assert_fn = va_arg(ap, SLMDB_ASSERT_FN);
	    if ((rc = mdb_env_set_userctx(slmdb->env, (void *) slmdb)) != 0
	     || (rc = mdb_env_set_assert(slmdb->env, slmdb_assert_cb)) != 0)
		status = rc;
	    break;
	case SLMDB_CTL_CB_CONTEXT:
	    slmdb->cb_context = va_arg(ap, void *);
	    break;
	case SLMDB_CTL_API_RETRY_LIMIT:
	    slmdb->api_retry_limit = va_arg(ap, int);
	    break;
	case SLMDB_CTL_BULK_RETRY_LIMIT:
	    slmdb->bulk_retry_limit = va_arg(ap, int);
	    break;
	default:
	    status = errno = EINVAL;
	    break;
	}
    }
    va_end(ap);
    return (status);
}

/* slmdb_close - wrapper with LMDB error recovery */

int     slmdb_close(SLMDB *slmdb)
{
    int     status = 0;

    /*
     * Finish an open bulk transaction. If slmdb_recover() returns after a
     * bulk-transaction error, then it was unable to recover.
     */
    if (slmdb->txn != 0
	&& (status = mdb_txn_commit(slmdb->txn)) != 0)
	status = slmdb_recover(slmdb, status);

    /*
     * Clean up after an unfinished sequence() operation.
     */
    if (slmdb->cursor != 0)
	slmdb_cursor_close(slmdb);

    mdb_env_close(slmdb->env);

    /*
     * Clean up the saved key information.
     */
    if (HAVE_SLMDB_SAVED_KEY(slmdb))
	slmdb_saved_key_free(slmdb);

    SLMDB_API_RETURN(slmdb, status);
}

/* slmdb_init - mandatory initialization */

int     slmdb_init(SLMDB *slmdb, size_t curr_limit, int size_incr,
		           size_t hard_limit)
{

    /*
     * This is a separate operation to keep the slmdb_open() API simple.
     * Don't allocate resources here. Just store control information,
     */
    slmdb->curr_limit = curr_limit;
    slmdb->size_incr = size_incr;
    slmdb->hard_limit = hard_limit;

    return (MDB_SUCCESS);
}

/* slmdb_open - open wrapped LMDB database */

int     slmdb_open(SLMDB *slmdb, const char *path, int open_flags,
		           int lmdb_flags, int slmdb_flags)
{
    struct stat st;
    MDB_env *env;
    MDB_txn *txn;
    MDB_dbi dbi;
    int     db_fd;
    int     status;

    /*
     * Create LMDB environment.
     */
    if ((status = mdb_env_create(&env)) != 0)
	return (status);

    /*
     * Make sure that the memory map has room to store and commit an initial
     * "drop" transaction as well as fixed database metadata. We have no way
     * to recover from errors before the first application-level I/O request.
     */
#define SLMDB_FUDGE      10240

    if (slmdb->curr_limit < SLMDB_FUDGE)
	slmdb->curr_limit = SLMDB_FUDGE;
    if (stat(path, &st) == 0
	&& st.st_size > slmdb->curr_limit - SLMDB_FUDGE) {
	if (st.st_size > slmdb->hard_limit)
	    slmdb->hard_limit = st.st_size;
	if (st.st_size < slmdb->hard_limit - SLMDB_FUDGE)
	    slmdb->curr_limit = st.st_size + SLMDB_FUDGE;
	else
	    slmdb->curr_limit = slmdb->hard_limit;
    }

    /*
     * mdb_open() requires a txn, but since the default DB always exists in
     * an LMDB environment, we usually don't need to do anything else with
     * the txn. It is currently used for truncate and for bulk transactions.
     */
    if ((status = mdb_env_set_mapsize(env, slmdb->curr_limit)) != 0
	|| (status = mdb_env_open(env, path, lmdb_flags, 0644)) != 0
	|| (status = mdb_txn_begin(env, (MDB_txn *) 0,
				   lmdb_flags & MDB_RDONLY, &txn)) != 0
	|| (status = mdb_open(txn, (const char *) 0, 0, &dbi)) != 0
	|| (status = mdb_env_get_fd(env, &db_fd)) != 0) {
	mdb_env_close(env);
	return (status);
    }

    /*
     * Bundle up.
     */
    slmdb->open_flags = open_flags;
    slmdb->lmdb_flags = lmdb_flags;
    slmdb->slmdb_flags = slmdb_flags;
    slmdb->env = env;
    slmdb->dbi = dbi;
    slmdb->db_fd = db_fd;
    slmdb->cursor = 0;
    slmdb_saved_key_init(slmdb);
    slmdb->api_retry_count = 0;
    slmdb->bulk_retry_count = 0;
    slmdb->api_retry_limit = SLMDB_DEF_API_RETRY_LIMIT;
    slmdb->bulk_retry_limit = SLMDB_DEF_BULK_RETRY_LIMIT;
    slmdb->longjmp_fn = 0;
    slmdb->notify_fn = 0;
    slmdb->assert_fn = 0;
    slmdb->cb_context = 0;
    slmdb->txn = txn;

    if ((status = slmdb_prepare(slmdb)) != 0)
	mdb_env_close(env);

    return (status);
}

#endif
