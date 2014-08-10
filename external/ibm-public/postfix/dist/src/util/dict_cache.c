/*	$NetBSD: dict_cache.c,v 1.1.1.2.6.1 2014/08/10 07:12:50 tls Exp $	*/

/*++
/* NAME
/*	dict_cache 3
/* SUMMARY
/*	External cache manager
/* SYNOPSIS
/*	#include <dict_cache.h>
/*
/*	DICT_CACHE *dict_cache_open(dbname, open_flags, dict_flags)
/*	const char *dbname;
/*	int	open_flags;
/*	int	dict_flags;
/*
/*	void	dict_cache_close(cache)
/*	DICT_CACHE *cache;
/*
/*	const char *dict_cache_lookup(cache, cache_key)
/*	DICT_CACHE *cache;
/*	const char *cache_key;
/*
/*	int	dict_cache_update(cache, cache_key, cache_val)
/*	DICT_CACHE *cache;
/*	const char *cache_key;
/*	const char *cache_val;
/*
/*	int	dict_cache_delete(cache, cache_key)
/*	DICT_CACHE *cache;
/*	const char *cache_key;
/*
/*	int	dict_cache_sequence(cache, first_next, cache_key, cache_val)
/*	DICT_CACHE *cache;
/*	int	first_next;
/*	const char **cache_key;
/*	const char **cache_val;
/* AUXILIARY FUNCTIONS
/*	void	dict_cache_control(cache, name, value, ...)
/*	DICT_CACHE *cache;
/*	int	name;
/*
/*	typedef int (*DICT_CACHE_VALIDATOR_FN) (const char *cache_key,
/*		const char *cache_val, char *context);
/*
/*	const char *dict_cache_name(cache)
/*	DICT_CACHE	*cache;
/* DESCRIPTION
/*	This module maintains external cache files with support
/*	for expiration. The underlying table must implement the
/*	"lookup", "update", "delete" and "sequence" operations.
/*
/*	Although this API is similar to the one documented in
/*	dict_open(3), there are subtle differences in the interaction
/*	between the iterators that access all cache elements, and
/*	other operations that access individual cache elements.
/*
/*	In particular, when a "sequence" or "cleanup" operation is
/*	in progress the cache intercepts requests to delete the
/*	"current" entry, as this would cause some databases to
/*	mis-behave. Instead, the cache implements a "delete behind"
/*	strategy, and deletes such an entry after the "sequence"
/*	or "cleanup" operation moves on to the next cache element.
/*	The "delete behind" strategy also affects the cache lookup
/*	and update operations as detailed below.
/*
/*	dict_cache_open() is a wrapper around the dict_open()
/*	function.  It opens the specified cache and returns a handle
/*	that must be used for subsequent access. This function does
/*	not return in case of error.
/*
/*	dict_cache_close() closes the specified cache and releases
/*	memory that was allocated by dict_cache_open(), and terminates
/*	any thread that was started with dict_cache_control().
/*
/*	dict_cache_lookup() looks up the specified cache entry.
/*	The result value is a null pointer when the cache entry was
/*	not found, or when the entry is scheduled for "delete
/*	behind".
/*
/*	dict_cache_update() updates the specified cache entry. If
/*	the entry is scheduled for "delete behind", the delete
/*	operation is canceled (because of this, the cache must be
/*	opened with DICT_FLAG_DUP_REPLACE). This function does not
/*	return in case of error.
/*
/*	dict_cache_delete() removes the specified cache entry.  If
/*	this is the "current" entry of a "sequence" operation, the
/*	entry is scheduled for "delete behind". The result value
/*	is zero when the entry was found.
/*
/*	dict_cache_sequence() iterates over the specified cache and
/*	returns each entry in an implementation-defined order.  The
/*	result value is zero when a cache entry was found.
/*
/*	Important: programs must not use both dict_cache_sequence()
/*	and the built-in cache cleanup feature.
/*
/*	dict_cache_control() provides control over the built-in
/*	cache cleanup feature and logging. The arguments are a list
/*	of (name, value) pairs, terminated with DICT_CACHE_CTL_END.
/*	The following lists the names and the types of the corresponding
/*	value arguments.
/* .IP "DICT_CACHE_FLAGS (int flags)"
/*	The arguments to this command are the bit-wise OR of zero
/*	or more of the following:
/* .RS
/* .IP DICT_CACHE_FLAG_VERBOSE
/*	Enable verbose logging of cache activity.
/* .IP DICT_CACHE_FLAG_EXP_SUMMARY
/*	Log cache statistics after each cache cleanup run.
/* .RE
/* .IP "DICT_CACHE_CTL_INTERVAL (int interval)"
/*	The interval between cache cleanup runs.  Specify a null
/*	validator or interval to stop cache cleanup.
/* .IP "DICT_CACHE_CTL_VALIDATOR (DICT_CACHE_VALIDATOR_FN validator)"
/*	An application call-back routine that returns non-zero when
/*	a cache entry should be kept. The call-back function should
/*	not make changes to the cache. Specify a null validator or
/*	interval to stop cache cleanup.
/* .IP "DICT_CACHE_CTL_CONTEXT (char *context)"
/*	Application context that is passed to the validator function.
/* .RE
/* .PP
/*	dict_cache_name() returns the name of the specified cache.
/*
/*	Arguments:
/* .IP "dbname, open_flags, dict_flags"
/*	These are passed unchanged to dict_open(). The cache must
/*	be opened with DICT_FLAG_DUP_REPLACE.
/* .IP cache
/*	Cache handle created with dict_cache_open().
/* .IP cache_key
/*	Cache lookup key.
/* .IP cache_val
/*	Information that is stored under a cache lookup key.
/* .IP first_next
/*	One of DICT_SEQ_FUN_FIRST (first cache element) or
/*	DICT_SEQ_FUN_NEXT (next cache element).
/* .sp
/*	Note: there is no "stop" request. To ensure that the "delete
/*	behind" strategy does not interfere with database access,
/*	allow dict_cache_sequence() to run to completion.
/* .IP table
/*	A bare dictonary handle.
/* DIAGNOSTICS
/*	When a request is satisfied, the lookup routine returns
/*	non-null, and the update, delete and sequence routines
/*	return zero.  The cache->error value is zero when a request
/*	could not be satisfied because an item did not exist (delete,
/*	sequence) or if it could not be updated. The cache->error
/*	value is non-zero only when a request could not be satisfied,
/*	and the cause was a database error.
/*
/*	Cache access errors are logged with a warning message. To
/*	avoid spamming the log, each type of operation logs no more
/*	than one cache access error per second, per cache. Specify
/*	the DICT_CACHE_FLAG_VERBOSE flag (see above) to log all
/*	warnings.
/* BUGS
/*	There should be a way to suspend automatic program suicide
/*	until a cache cleanup run is completed. Some entries may
/*	never be removed when the process max_idle time is less
/*	than the time needed to make a full pass over the cache.
/*
/*	The delete-behind strategy assumes that all updates are
/*	made by a single process. Otherwise, delete-behind may
/*	remove an entry that was updated after it was scheduled for
/*	deletion.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* HISTORY
/* .ad
/* .fi
/*	A predecessor of this code was written first for the Postfix
/*	tlsmgr(8) daemon.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <string.h>
#include <stdlib.h>

/* Utility library. */

#include <msg.h>
#include <dict.h>
#include <mymalloc.h>
#include <events.h>
#include <dict_cache.h>

/* Application-specific. */

 /*
  * XXX Deleting entries while enumerating a map can he tricky. Some map
  * types have a concept of cursor and support a "delete the current element"
  * operation. Some map types without cursors don't behave well when the
  * current first/next entry is deleted (example: with Berkeley DB < 2, the
  * "next" operation produces garbage). To avoid trouble, we delete an entry
  * after advancing the current first/next position beyond it; we use the
  * same strategy with application requests to delete the current entry.
  */

 /*
  * Opaque data structure. Use dict_cache_name() to access the name of the
  * underlying database.
  */
struct DICT_CACHE {
    char   *name;			/* full name including proxy: */
    int     cache_flags;		/* see below */
    int     user_flags;			/* logging */
    DICT   *db;				/* database handle */
    int     error;			/* last operation only */

    /* Delete-behind support. */
    char   *saved_curr_key;		/* "current" cache lookup key */
    char   *saved_curr_val;		/* "current" cache lookup result */

    /* Cleanup support. */
    int     exp_interval;		/* time between cleanup runs */
    DICT_CACHE_VALIDATOR_FN exp_validator;	/* expiration call-back */
    char   *exp_context;		/* call-back context */
    int     retained;			/* entries retained in cleanup run */
    int     dropped;			/* entries removed in cleanup run */

    /* Rate-limited logging support. */
    int     log_delay;
    time_t  upd_log_stamp;		/* last update warning */
    time_t  get_log_stamp;		/* last lookup warning */
    time_t  del_log_stamp;		/* last delete warning */
    time_t  seq_log_stamp;		/* last sequence warning */
};

#define DC_FLAG_DEL_SAVED_CURRENT_KEY	(1<<0)	/* delete-behind is scheduled */

 /*
  * Don't log cache access errors more than once per second.
  */
#define DC_DEF_LOG_DELAY	1

 /*
  * Macros to make obscure code more readable.
  */
#define DC_SCHEDULE_FOR_DELETE_BEHIND(cp) \
    ((cp)->cache_flags |= DC_FLAG_DEL_SAVED_CURRENT_KEY)

#define DC_MATCH_SAVED_CURRENT_KEY(cp, cache_key) \
    ((cp)->saved_curr_key && strcmp((cp)->saved_curr_key, (cache_key)) == 0)

#define DC_IS_SCHEDULED_FOR_DELETE_BEHIND(cp) \
    (/* NOT: (cp)->saved_curr_key && */ \
	((cp)->cache_flags & DC_FLAG_DEL_SAVED_CURRENT_KEY) != 0)

#define DC_CANCEL_DELETE_BEHIND(cp) \
    ((cp)->cache_flags &= ~DC_FLAG_DEL_SAVED_CURRENT_KEY)

 /*
  * Special key to store the time of the last cache cleanup run completion.
  */
#define DC_LAST_CACHE_CLEANUP_COMPLETED "_LAST_CACHE_CLEANUP_COMPLETED_"

/* dict_cache_lookup - load entry from cache */

const char *dict_cache_lookup(DICT_CACHE *cp, const char *cache_key)
{
    const char *myname = "dict_cache_lookup";
    const char *cache_val;
    DICT   *db = cp->db;

    /*
     * Search for the cache entry. Don't return an entry that is scheduled
     * for delete-behind.
     */
    if (DC_IS_SCHEDULED_FOR_DELETE_BEHIND(cp)
	&& DC_MATCH_SAVED_CURRENT_KEY(cp, cache_key)) {
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: key=%s (pretend not found  - scheduled for deletion)",
		     myname, cache_key);
	DICT_ERR_VAL_RETURN(cp, DICT_ERR_NONE, (char *) 0);
    } else {
	cache_val = dict_get(db, cache_key);
	if (cache_val == 0 && db->error != 0)
	    msg_rate_delay(&cp->get_log_stamp, cp->log_delay, msg_warn,
			   "%s: cache lookup for '%s' failed due to error",
			   cp->name, cache_key);
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: key=%s value=%s", myname, cache_key,
		     cache_val ? cache_val : db->error ?
		     "error" : "(not found)");
	DICT_ERR_VAL_RETURN(cp, db->error, cache_val);
    }
}

/* dict_cache_update - save entry to cache */

int     dict_cache_update(DICT_CACHE *cp, const char *cache_key,
			          const char *cache_val)
{
    const char *myname = "dict_cache_update";
    DICT   *db = cp->db;
    int     put_res;

    /*
     * Store the cache entry and cancel the delete-behind operation.
     */
    if (DC_IS_SCHEDULED_FOR_DELETE_BEHIND(cp)
	&& DC_MATCH_SAVED_CURRENT_KEY(cp, cache_key)) {
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: cancel delete-behind for key=%s", myname, cache_key);
	DC_CANCEL_DELETE_BEHIND(cp);
    }
    if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	msg_info("%s: key=%s value=%s", myname, cache_key, cache_val);
    put_res = dict_put(db, cache_key, cache_val);
    if (put_res != 0)
	msg_rate_delay(&cp->upd_log_stamp, cp->log_delay, msg_warn,
		  "%s: could not update entry for %s", cp->name, cache_key);
    DICT_ERR_VAL_RETURN(cp, db->error, put_res);
}

/* dict_cache_delete - delete entry from cache */

int     dict_cache_delete(DICT_CACHE *cp, const char *cache_key)
{
    const char *myname = "dict_cache_delete";
    int     del_res;
    DICT   *db = cp->db;

    /*
     * Delete the entry, unless we would delete the current first/next entry.
     * In that case, schedule the "current" entry for delete-behind to avoid
     * mis-behavior by some databases.
     */
    if (DC_MATCH_SAVED_CURRENT_KEY(cp, cache_key)) {
	DC_SCHEDULE_FOR_DELETE_BEHIND(cp);
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: key=%s (current entry - schedule for delete-behind)",
		     myname, cache_key);
	DICT_ERR_VAL_RETURN(cp, DICT_ERR_NONE, DICT_STAT_SUCCESS);
    } else {
	del_res = dict_del(db, cache_key);
	if (del_res != 0)
	    msg_rate_delay(&cp->del_log_stamp, cp->log_delay, msg_warn,
		  "%s: could not delete entry for %s", cp->name, cache_key);
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: key=%s (%s)", myname, cache_key,
		     del_res == 0 ? "found" :
		     db->error ? "error" : "not found");
	DICT_ERR_VAL_RETURN(cp, db->error, del_res);
    }
}

/* dict_cache_sequence - look up the first/next cache entry */

int     dict_cache_sequence(DICT_CACHE *cp, int first_next,
			            const char **cache_key,
			            const char **cache_val)
{
    const char *myname = "dict_cache_sequence";
    int     seq_res;
    const char *raw_cache_key;
    const char *raw_cache_val;
    char   *previous_curr_key;
    char   *previous_curr_val;
    DICT   *db = cp->db;

    /*
     * Find the first or next database entry. Hide the record with the cache
     * cleanup completion time stamp.
     */
    seq_res = dict_seq(db, first_next, &raw_cache_key, &raw_cache_val);
    if (seq_res == 0
	&& strcmp(raw_cache_key, DC_LAST_CACHE_CLEANUP_COMPLETED) == 0)
	seq_res =
	    dict_seq(db, DICT_SEQ_FUN_NEXT, &raw_cache_key, &raw_cache_val);
    if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	msg_info("%s: key=%s value=%s", myname,
		 seq_res == 0 ? raw_cache_key : db->error ?
		 "(error)" : "(not found)",
		 seq_res == 0 ? raw_cache_val : db->error ?
		 "(error)" : "(not found)");
    if (db->error)
	msg_rate_delay(&cp->seq_log_stamp, cp->log_delay, msg_warn,
		       "%s: sequence error", cp->name);

    /*
     * Save the current cache_key and cache_val before they are clobbered by
     * our own delete operation below. This also prevents surprises when the
     * application accesses the database after this function returns.
     * 
     * We also use the saved cache_key to protect the current entry against
     * application delete requests.
     */
    previous_curr_key = cp->saved_curr_key;
    previous_curr_val = cp->saved_curr_val;
    if (seq_res == 0) {
	cp->saved_curr_key = mystrdup(raw_cache_key);
	cp->saved_curr_val = mystrdup(raw_cache_val);
    } else {
	cp->saved_curr_key = 0;
	cp->saved_curr_val = 0;
    }

    /*
     * Delete behind.
     */
    if (db->error == 0 && DC_IS_SCHEDULED_FOR_DELETE_BEHIND(cp)) {
	DC_CANCEL_DELETE_BEHIND(cp);
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: delete-behind key=%s value=%s",
		     myname, previous_curr_key, previous_curr_val);
	if (dict_del(db, previous_curr_key) != 0)
	    msg_rate_delay(&cp->del_log_stamp, cp->log_delay, msg_warn,
			   "%s: could not delete entry for %s",
			   cp->name, previous_curr_key);
    }

    /*
     * Clean up previous iteration key and value.
     */
    if (previous_curr_key)
	myfree(previous_curr_key);
    if (previous_curr_val)
	myfree(previous_curr_val);

    /*
     * Return the result.
     */
    *cache_key = (cp)->saved_curr_key;
    *cache_val = (cp)->saved_curr_val;
    DICT_ERR_VAL_RETURN(cp, db->error, seq_res);
}

/* dict_cache_delete_behind_reset - reset "delete behind" state */

static void dict_cache_delete_behind_reset(DICT_CACHE *cp)
{
#define FREE_AND_WIPE(s) do { if (s) { myfree(s); (s) = 0; } } while (0)

    DC_CANCEL_DELETE_BEHIND(cp);
    FREE_AND_WIPE(cp->saved_curr_key);
    FREE_AND_WIPE(cp->saved_curr_val);
}

/* dict_cache_clean_stat_log_reset - log and reset cache cleanup statistics */

static void dict_cache_clean_stat_log_reset(DICT_CACHE *cp,
					            const char *full_partial)
{
    if (cp->user_flags & DICT_CACHE_FLAG_STATISTICS)
	msg_info("cache %s %s cleanup: retained=%d dropped=%d entries",
		 cp->name, full_partial, cp->retained, cp->dropped);
    cp->retained = cp->dropped = 0;
}

/* dict_cache_clean_event - examine one cache entry */

static void dict_cache_clean_event(int unused_event, char *cache_context)
{
    const char *myname = "dict_cache_clean_event";
    DICT_CACHE *cp = (DICT_CACHE *) cache_context;
    const char *cache_key;
    const char *cache_val;
    int     next_interval;
    VSTRING *stamp_buf;
    int     first_next;

    /*
     * We interleave cache cleanup with other processing, so that the
     * application's service remains available, with perhaps increased
     * latency.
     */

    /*
     * Start a new cache cleanup run.
     */
    if (cp->saved_curr_key == 0) {
	cp->retained = cp->dropped = 0;
	first_next = DICT_SEQ_FUN_FIRST;
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: start %s cache cleanup", myname, cp->name);
    }

    /*
     * Continue a cache cleanup run in progress.
     */
    else {
	first_next = DICT_SEQ_FUN_NEXT;
    }

    /*
     * Examine one cache entry.
     */
    if (dict_cache_sequence(cp, first_next, &cache_key, &cache_val) == 0) {
	if (cp->exp_validator(cache_key, cache_val, cp->exp_context) == 0) {
	    DC_SCHEDULE_FOR_DELETE_BEHIND(cp);
	    cp->dropped++;
	    if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
		msg_info("%s: drop %s cache entry for %s",
			 myname, cp->name, cache_key);
	} else {
	    cp->retained++;
	    if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
		msg_info("%s: keep %s cache entry for %s",
			 myname, cp->name, cache_key);
	}
	next_interval = 0;
    }

    /*
     * Cache cleanup completed. Report vital statistics.
     */
    else if (cp->error != 0) {
	msg_warn("%s: cache cleanup scan terminated due to error", cp->name);
	dict_cache_clean_stat_log_reset(cp, "partial");
	next_interval = cp->exp_interval;
    } else {
	if (cp->user_flags & DICT_CACHE_FLAG_VERBOSE)
	    msg_info("%s: done %s cache cleanup scan", myname, cp->name);
	dict_cache_clean_stat_log_reset(cp, "full");
	stamp_buf = vstring_alloc(100);
	vstring_sprintf(stamp_buf, "%ld", (long) event_time());
	dict_put(cp->db, DC_LAST_CACHE_CLEANUP_COMPLETED,
		 vstring_str(stamp_buf));
	vstring_free(stamp_buf);
	next_interval = cp->exp_interval;
    }
    event_request_timer(dict_cache_clean_event, cache_context, next_interval);
}

/* dict_cache_control - schedule or stop the cache cleanup thread */

void    dict_cache_control(DICT_CACHE *cp,...)
{
    const char *myname = "dict_cache_control";
    const char *last_done;
    time_t  next_interval;
    int     cache_cleanup_is_active = (cp->exp_validator && cp->exp_interval);
    va_list ap;
    int     name;

    /*
     * Update the control settings.
     */
    va_start(ap, cp);
    while ((name = va_arg(ap, int)) > 0) {
	switch (name) {
	case DICT_CACHE_CTL_END:
	    break;
	case DICT_CACHE_CTL_FLAGS:
	    cp->user_flags = va_arg(ap, int);
	    cp->log_delay = (cp->user_flags & DICT_CACHE_FLAG_VERBOSE) ?
		0 : DC_DEF_LOG_DELAY;
	    break;
	case DICT_CACHE_CTL_INTERVAL:
	    cp->exp_interval = va_arg(ap, int);
	    if (cp->exp_interval < 0)
		msg_panic("%s: bad %s cache cleanup interval %d",
			  myname, cp->name, cp->exp_interval);
	    break;
	case DICT_CACHE_CTL_VALIDATOR:
	    cp->exp_validator = va_arg(ap, DICT_CACHE_VALIDATOR_FN);
	    break;
	case DICT_CACHE_CTL_CONTEXT:
	    cp->exp_context = va_arg(ap, char *);
	    break;
	default:
	    msg_panic("%s: bad command: %d", myname, name);
	}
    }
    va_end(ap);

    /*
     * Schedule the cache cleanup thread.
     */
    if (cp->exp_interval && cp->exp_validator) {

	/*
	 * Sanity checks.
	 */
	if (cache_cleanup_is_active)
	    msg_panic("%s: %s cache cleanup is already scheduled",
		      myname, cp->name);

	/*
	 * The next start time depends on the last completion time.
	 */
#define NEXT_START(last, delta) ((delta) + (unsigned long) atol(last))
#define NOW	(time((time_t *) 0))		/* NOT: event_time() */

	if ((last_done = dict_get(cp->db, DC_LAST_CACHE_CLEANUP_COMPLETED)) == 0
	    || (next_interval = (NEXT_START(last_done, cp->exp_interval) - NOW)) < 0)
	    next_interval = 0;
	if (next_interval > cp->exp_interval)
	    next_interval = cp->exp_interval;
	if ((cp->user_flags & DICT_CACHE_FLAG_VERBOSE) && next_interval > 0)
	    msg_info("%s cache cleanup will start after %ds",
		     cp->name, (int) next_interval);
	event_request_timer(dict_cache_clean_event, (char *) cp,
			    (int) next_interval);
    }

    /*
     * Cancel the cache cleanup thread.
     */
    else if (cache_cleanup_is_active) {
	if (cp->retained || cp->dropped)
	    dict_cache_clean_stat_log_reset(cp, "partial");
	dict_cache_delete_behind_reset(cp);
	event_cancel_timer(dict_cache_clean_event, (char *) cp);
    }
}

/* dict_cache_open - open cache file */

DICT_CACHE *dict_cache_open(const char *dbname, int open_flags, int dict_flags)
{
    DICT_CACHE *cp;
    DICT   *dict;

    /*
     * Open the database as requested. Don't attempt to second-guess the
     * application.
     */
    dict = dict_open(dbname, open_flags, dict_flags);

    /*
     * Create the DICT_CACHE object.
     */
    cp = (DICT_CACHE *) mymalloc(sizeof(*cp));
    cp->name = mystrdup(dbname);
    cp->cache_flags = 0;
    cp->user_flags = 0;
    cp->db = dict;
    cp->saved_curr_key = 0;
    cp->saved_curr_val = 0;
    cp->exp_interval = 0;
    cp->exp_validator = 0;
    cp->exp_context = 0;
    cp->retained = 0;
    cp->dropped = 0;
    cp->log_delay = DC_DEF_LOG_DELAY;
    cp->upd_log_stamp = cp->get_log_stamp =
	cp->del_log_stamp = cp->seq_log_stamp = 0;

    return (cp);
}

/* dict_cache_close - close cache file */

void    dict_cache_close(DICT_CACHE *cp)
{

    /*
     * Destroy the DICT_CACHE object.
     */
    myfree(cp->name);
    dict_cache_control(cp, DICT_CACHE_CTL_INTERVAL, 0, DICT_CACHE_CTL_END);
    dict_close(cp->db);
    if (cp->saved_curr_key)
	myfree(cp->saved_curr_key);
    if (cp->saved_curr_val)
	myfree(cp->saved_curr_val);
    myfree((char *) cp);
}

/* dict_cache_name - get the cache name */

const char *dict_cache_name(DICT_CACHE *cp)
{

    /*
     * This is used for verbose logging or warning messages, so the cost of
     * call is only made where needed (well sort off - code that does not
     * execute still presents overhead for the processor pipeline, processor
     * cache, etc).
     */
    return (cp->name);
}

 /*
  * Test driver with support for interleaved access. First, enter a number of
  * requests to look up, update or delete a sequence of cache entries, then
  * interleave those sequences with the "run" command.
  */
#ifdef TEST
#include <msg_vstream.h>
#include <vstring_vstream.h>
#include <argv.h>
#include <stringops.h>

#define DELIMS	" "
#define USAGE	"\n\tTo manage settings:" \
		"\n\tverbose <level> (verbosity level)" \
		"\n\telapsed <level> (0=don't show elapsed time)" \
		"\n\tlmdb_map_size <limit> (initial LMDB size limit)" \
		"\n\tcache <type>:<name> (switch to named database)" \
		"\n\tstatus (show map size, cache, pending requests)" \
		"\n\n\tTo manage pending requests:" \
		"\n\treset (discard pending requests)" \
		"\n\trun (execute pending requests in interleaved order)" \
		"\n\n\tTo add a pending request:" \
		"\n\tquery <key-suffix> <count> (negative to reverse order)" \
		"\n\tupdate <key-suffix> <count> (negative to reverse order)" \
		"\n\tdelete <key-suffix> <count> (negative to reverse order)" \
		"\n\tpurge <key-suffix>" \
		"\n\tcount <key-suffix>"

 /*
  * For realism, open the cache with the same flags as postscreen(8) and
  * verify(8).
  */
#define DICT_CACHE_OPEN_FLAGS (DICT_FLAG_DUP_REPLACE | DICT_FLAG_SYNC_UPDATE | \
	DICT_FLAG_OPEN_LOCK)

 /*
  * Storage for one request to access a sequence of cache entries.
  */
typedef struct DICT_CACHE_SREQ {
    int     flags;			/* per-request: reverse, purge */
    char   *cmd;			/* command for status report */
    void    (*action) (struct DICT_CACHE_SREQ *, DICT_CACHE *, VSTRING *);
    char   *suffix;			/* key suffix */
    int     done;			/* progress indicator */
    int     todo;			/* number of entries to process */
    int     first_next;			/* first/next */
} DICT_CACHE_SREQ;

#define DICT_CACHE_SREQ_FLAG_PURGE	(1<<1)	/* purge instead of count */
#define DICT_CACHE_SREQ_FLAG_REVERSE	(1<<2)	/* reverse instead of forward */

#define DICT_CACHE_SREQ_LIMIT		10

 /*
  * All test requests combined.
  */
typedef struct DICT_CACHE_TEST {
    int     flags;			/* exclusion flags */
    int     size;			/* allocated slots */
    int     used;			/* used slots */
    DICT_CACHE_SREQ job_list[1];	/* actually, a bunch */
} DICT_CACHE_TEST;

#define DICT_CACHE_TEST_FLAG_ITER	(1<<0)	/* count or purge */

#define STR(x)	vstring_str(x)

int     show_elapsed = 1;		/* show elapsed time */

#ifdef HAS_LMDB
extern size_t dict_lmdb_map_size;	/* LMDB-specific */

#endif

/* usage - command-line usage message */

static NORETURN usage(const char *progname)
{
    msg_fatal("usage: %s (no argument)", progname);
}

/* make_tagged_key - make tagged search key */

static void make_tagged_key(VSTRING *bp, DICT_CACHE_SREQ *cp)
{
    if (cp->done < 0)
	msg_panic("make_tagged_key: bad done count: %d", cp->done);
    if (cp->todo < 1)
	msg_panic("make_tagged_key: bad todo count: %d", cp->todo);
    vstring_sprintf(bp, "%d-%s",
		    (cp->flags & DICT_CACHE_SREQ_FLAG_REVERSE) ?
		    cp->todo - cp->done - 1 : cp->done, cp->suffix);
}

/* create_requests - create request list */

static DICT_CACHE_TEST *create_requests(int count)
{
    DICT_CACHE_TEST *tp;
    DICT_CACHE_SREQ *cp;

    tp = (DICT_CACHE_TEST *) mymalloc(sizeof(DICT_CACHE_TEST) +
				      (count - 1) *sizeof(DICT_CACHE_SREQ));
    tp->flags = 0;
    tp->size = count;
    tp->used = 0;
    for (cp = tp->job_list; cp < tp->job_list + count; cp++) {
	cp->flags = 0;
	cp->cmd = 0;
	cp->action = 0;
	cp->suffix = 0;
	cp->todo = 0;
	cp->first_next = DICT_SEQ_FUN_FIRST;
    }
    return (tp);
}

/* reset_requests - reset request list */

static void reset_requests(DICT_CACHE_TEST *tp)
{
    DICT_CACHE_SREQ *cp;

    tp->flags = 0;
    tp->used = 0;
    for (cp = tp->job_list; cp < tp->job_list + tp->size; cp++) {
	cp->flags = 0;
	if (cp->cmd) {
	    myfree(cp->cmd);
	    cp->cmd = 0;
	}
	cp->action = 0;
	if (cp->suffix) {
	    myfree(cp->suffix);
	    cp->suffix = 0;
	}
	cp->todo = 0;
	cp->first_next = DICT_SEQ_FUN_FIRST;
    }
}

/* free_requests - destroy request list */

static void free_requests(DICT_CACHE_TEST *tp)
{
    reset_requests(tp);
    myfree((char *) tp);
}

/* run_requests - execute pending requests in interleaved order */

static void run_requests(DICT_CACHE_TEST *tp, DICT_CACHE *dp, VSTRING *bp)
{
    DICT_CACHE_SREQ *cp;
    int     todo;
    struct timeval start;
    struct timeval finish;
    struct timeval elapsed;

    if (dp == 0) {
	msg_warn("no cache");
	return;
    }
    GETTIMEOFDAY(&start);
    do {
	todo = 0;
	for (cp = tp->job_list; cp < tp->job_list + tp->used; cp++) {
	    if (cp->done < cp->todo) {
		todo = 1;
		cp->action(cp, dp, bp);
	    }
	}
    } while (todo);
    GETTIMEOFDAY(&finish);
    timersub(&finish, &start, &elapsed);
    if (show_elapsed)
	vstream_printf("Elapsed: %g\n",
		       elapsed.tv_sec + elapsed.tv_usec / 1000000.0);

    reset_requests(tp);
}

/* show_status - show settings and pending requests */

static void show_status(DICT_CACHE_TEST *tp, DICT_CACHE *dp)
{
    DICT_CACHE_SREQ *cp;

#ifdef HAS_LMDB
    vstream_printf("lmdb_map_size\t%ld\n", (long) dict_lmdb_map_size);
#endif
    vstream_printf("cache\t%s\n", dp ? dp->name : "(none)");

    if (tp->used == 0)
	vstream_printf("No pending requests\n");
    else
	vstream_printf("%s\t%s\t%s\t%s\t%s\t%s\n",
		     "cmd", "dir", "suffix", "count", "done", "first/next");

    for (cp = tp->job_list; cp < tp->job_list + tp->used; cp++)
	if (cp->todo > 0)
	    vstream_printf("%s\t%s\t%s\t%d\t%d\t%d\n",
			   cp->cmd,
			   (cp->flags & DICT_CACHE_SREQ_FLAG_REVERSE) ?
			   "reverse" : "forward",
			   cp->suffix ? cp->suffix : "(null)", cp->todo,
			   cp->done, cp->first_next);
}

/* query_action - lookup cache entry */

static void query_action(DICT_CACHE_SREQ *cp, DICT_CACHE *dp, VSTRING *bp)
{
    const char *lookup;

    make_tagged_key(bp, cp);
    if ((lookup = dict_cache_lookup(dp, STR(bp))) == 0) {
	if (dp->error)
	    msg_warn("query_action: query failed: %s: %m", STR(bp));
	else
	    msg_warn("query_action: query failed: %s", STR(bp));
    } else if (strcmp(STR(bp), lookup) != 0) {
	msg_warn("lookup result \"%s\" differs from key \"%s\"",
		 lookup, STR(bp));
    }
    cp->done += 1;
}

/* update_action - update cache entry */

static void update_action(DICT_CACHE_SREQ *cp, DICT_CACHE *dp, VSTRING *bp)
{
    make_tagged_key(bp, cp);
    if (dict_cache_update(dp, STR(bp), STR(bp)) != 0) {
	if (dp->error)
	    msg_warn("update_action: update failed: %s: %m", STR(bp));
	else
	    msg_warn("update_action: update failed: %s", STR(bp));
    }
    cp->done += 1;
}

/* delete_action - delete cache entry */

static void delete_action(DICT_CACHE_SREQ *cp, DICT_CACHE *dp, VSTRING *bp)
{
    make_tagged_key(bp, cp);
    if (dict_cache_delete(dp, STR(bp)) != 0) {
	if (dp->error)
	    msg_warn("delete_action: delete failed: %s: %m", STR(bp));
	else
	    msg_warn("delete_action: delete failed: %s", STR(bp));
    }
    cp->done += 1;
}

/* iter_action - iterate over cache and act on entries with given suffix */

static void iter_action(DICT_CACHE_SREQ *cp, DICT_CACHE *dp, VSTRING *bp)
{
    const char *cache_key;
    const char *cache_val;
    const char *what;
    const char *suffix;

    if (dict_cache_sequence(dp, cp->first_next, &cache_key, &cache_val) == 0) {
	if (strcmp(cache_key, cache_val) != 0)
	    msg_warn("value \"%s\" differs from key \"%s\"",
		     cache_val, cache_key);
	suffix = cache_key + strspn(cache_key, "0123456789");
	if (suffix[0] == '-' && strcmp(suffix + 1, cp->suffix) == 0) {
	    cp->done += 1;
	    cp->todo = cp->done + 1;		/* XXX */
	    if ((cp->flags & DICT_CACHE_SREQ_FLAG_PURGE)
		&& dict_cache_delete(dp, cache_key) != 0) {
		if (dp->error)
		    msg_warn("purge_action: delete failed: %s: %m", STR(bp));
		else
		    msg_warn("purge_action: delete failed: %s", STR(bp));
	    }
	}
	cp->first_next = DICT_SEQ_FUN_NEXT;
    } else {
	what = (cp->flags & DICT_CACHE_SREQ_FLAG_PURGE) ? "purge" : "count";
	if (dp->error)
	    msg_warn("%s error after %d: %m", what, cp->done);
	else
	    vstream_printf("suffix=%s %s=%d\n", cp->suffix, what, cp->done);
	cp->todo = 0;
    }
}

 /*
  * Table-driven support.
  */
typedef struct DICT_CACHE_SREQ_INFO {
    const char *name;
    int     argc;
    void    (*action) (DICT_CACHE_SREQ *, DICT_CACHE *, VSTRING *);
    int     test_flags;
    int     req_flags;
} DICT_CACHE_SREQ_INFO;

static DICT_CACHE_SREQ_INFO req_info[] = {
    {"query", 3, query_action},
    {"update", 3, update_action},
    {"delete", 3, delete_action},
    {"count", 2, iter_action, DICT_CACHE_TEST_FLAG_ITER},
    {"purge", 2, iter_action, DICT_CACHE_TEST_FLAG_ITER, DICT_CACHE_SREQ_FLAG_PURGE},
    0,
};

/* add_request - add a request to the list */

static void add_request(DICT_CACHE_TEST *tp, ARGV *argv)
{
    DICT_CACHE_SREQ_INFO *rp;
    DICT_CACHE_SREQ *cp;
    int     req_flags;
    int     count;
    char   *cmd = argv->argv[0];
    char   *suffix = (argv->argc > 1 ? argv->argv[1] : 0);
    char   *todo = (argv->argc > 2 ? argv->argv[2] : "1");	/* XXX */

    if (tp->used >= tp->size) {
	msg_warn("%s: request list is full", cmd);
	return;
    }
    for (rp = req_info; /* See below */ ; rp++) {
	if (rp->name == 0) {
	    vstream_printf("usage: %s\n", USAGE);
	    return;
	}
	if (strcmp(rp->name, argv->argv[0]) == 0
	    && rp->argc == argv->argc)
	    break;
    }
    req_flags = rp->req_flags;
    if (todo[0] == '-') {
	req_flags |= DICT_CACHE_SREQ_FLAG_REVERSE;
	todo += 1;
    }
    if (!alldig(todo) || (count = atoi(todo)) == 0) {
	msg_warn("%s: bad count: %s", cmd, todo);
	return;
    }
    if (tp->flags & rp->test_flags) {
	msg_warn("%s: command conflicts with other command", cmd);
	return;
    }
    tp->flags |= rp->test_flags;
    cp = tp->job_list + tp->used;
    cp->cmd = mystrdup(cmd);
    cp->action = rp->action;
    if (suffix)
	cp->suffix = mystrdup(suffix);
    cp->done = 0;
    cp->flags = req_flags;
    cp->todo = count;
    tp->used += 1;
}

/* main - main program */

int     main(int argc, char **argv)
{
    DICT_CACHE_TEST *test_job;
    VSTRING *inbuf = vstring_alloc(100);
    char   *bufp;
    ARGV   *args;
    DICT_CACHE *cache = 0;
    int     stdin_is_tty;

    msg_vstream_init(argv[0], VSTREAM_ERR);
    if (argc != 1)
	usage(argv[0]);


    test_job = create_requests(DICT_CACHE_SREQ_LIMIT);

    stdin_is_tty = isatty(0);

    for (;;) {
	if (stdin_is_tty) {
	    vstream_printf("> ");
	    vstream_fflush(VSTREAM_OUT);
	}
	if (vstring_fgets_nonl(inbuf, VSTREAM_IN) == 0)
	    break;
	bufp = vstring_str(inbuf);
	if (!stdin_is_tty) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#')
	    continue;
	args = argv_split(bufp, DELIMS);
	if (argc == 0) {
	    vstream_printf("usage: %s\n", USAGE);
	    vstream_fflush(VSTREAM_OUT);
	    continue;
	}
	if (strcmp(args->argv[0], "verbose") == 0 && args->argc == 2) {
	    msg_verbose = atoi(args->argv[1]);
	} else if (strcmp(args->argv[0], "elapsed") == 0 && args->argc == 2) {
	    show_elapsed = atoi(args->argv[1]);
#ifdef HAS_LMDB
	} else if (strcmp(args->argv[0], "lmdb_map_size") == 0 && args->argc == 2) {
	    dict_lmdb_map_size = atol(args->argv[1]);
#endif
	} else if (strcmp(args->argv[0], "cache") == 0 && args->argc == 2) {
	    if (cache)
		dict_cache_close(cache);
	    cache = dict_cache_open(args->argv[1], O_CREAT | O_RDWR,
				    DICT_CACHE_OPEN_FLAGS);
	} else if (strcmp(args->argv[0], "reset") == 0 && args->argc == 1) {
	    reset_requests(test_job);
	} else if (strcmp(args->argv[0], "run") == 0 && args->argc == 1) {
	    run_requests(test_job, cache, inbuf);
	} else if (strcmp(args->argv[0], "status") == 0 && args->argc == 1) {
	    show_status(test_job, cache);
	} else {
	    add_request(test_job, args);
	}
	vstream_fflush(VSTREAM_OUT);
	argv_free(args);
    }

    vstring_free(inbuf);
    free_requests(test_job);
    if (cache)
	dict_cache_close(cache);
    return (0);
}

#endif
