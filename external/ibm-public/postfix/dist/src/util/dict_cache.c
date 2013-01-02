/*	$NetBSD: dict_cache.c,v 1.1.1.2 2013/01/02 18:59:12 tron Exp $	*/

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
