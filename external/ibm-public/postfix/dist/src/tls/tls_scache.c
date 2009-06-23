/*	$NetBSD: tls_scache.c,v 1.1.1.1 2009/06/23 10:08:57 tron Exp $	*/

/*++
/* NAME
/*	tls_scache 3
/* SUMMARY
/*	TLS session cache manager
/* SYNOPSIS
/*	#include <tls_scache.h>
/*
/*	TLS_SCACHE *tls_scache_open(dbname, cache_label, verbose, timeout)
/*	const char *dbname
/*	const char *cache_label;
/*	int	verbose;
/*	int	timeout;
/*
/*	void	tls_scache_close(cache)
/*	TLS_SCACHE *cache;
/*
/*	int	tls_scache_lookup(cache, cache_id, out_session)
/*	TLS_SCACHE *cache;
/*	const char *cache_id;
/*	VSTRING	*out_session;
/*
/*	int	tls_scache_update(cache, cache_id, session, session_len)
/*	TLS_SCACHE *cache;
/*	const char *cache_id;
/*	const char *session;
/*	ssize_t	session_len;
/*
/*	int	tls_scache_sequence(cache, first_next, out_cache_id,
/*				VSTRING *out_session)
/*	TLS_SCACHE *cache;
/*	int	first_next;
/*	char	**out_cache_id;
/*	VSTRING	*out_session;
/*
/*	int	tls_scache_delete(cache, cache_id)
/*	TLS_SCACHE *cache;
/*	const char *cache_id;
/* DESCRIPTION
/*	This module maintains Postfix TLS session cache files.
/*	each session is stored under a lookup key (hostname or
/*	session ID).
/*
/*	tls_scache_open() opens the specified TLS session cache
/*	and returns a handle that must be used for subsequent
/*	access.
/*
/*	tls_scache_close() closes the specified TLS session cache
/*	and releases memory that was allocated by tls_scache_open().
/*
/*	tls_scache_lookup() looks up the specified session in the
/*	specified cache, and applies session timeout restrictions.
/*	Entries that are too old are silently deleted.
/*
/*	tls_scache_update() updates the specified TLS session cache
/*	with the specified session information.
/*
/*	tls_scache_sequence() iterates over the specified TLS session
/*	cache and either returns the first or next entry that has not
/*	timed out, or returns no data. Entries that are too old are
/*	silently deleted. Specify TLS_SCACHE_SEQUENCE_NOTHING as the
/*	third and last argument to disable saving of cache entry
/*	content or cache entry ID information. This is useful when
/*	purging expired entries. A result value of zero means that
/*	the end of the cache was reached.
/*
/*	tls_scache_delete() removes the specified cache entry from
/*	the specified TLS session cache.
/*
/*	Arguments:
/* .IP dbname
/*	The base name of the session cache file.
/* .IP cache_label
/*	A string that is used in logging and error messages.
/* .IP verbose
/*	Do verbose logging of cache operations? (zero == no)
/* .IP timeout
/*	The time after wich a session cache entry is considered too old.
/* .IP first_next
/*	One of DICT_SEQ_FUN_FIRST (first cache element) or DICT_SEQ_FUN_NEXT
/*	(next cache element).
/* .IP cache_id
/*	Session cache lookup key.
/* .IP session
/*	Storage for session information.
/* .IP session_len
/*	The size of the session information in bytes.
/* .IP out_cache_id
/* .IP out_session
/*	Storage for saving the cache_id or session information of the
/*	current cache entry.
/*
/*	Specify TLS_SCACHE_DONT_NEED_CACHE_ID to avoid saving
/*	the session cache ID of the cache entry.
/*
/*	Specify TLS_SCACHE_DONT_NEED_SESSION to avoid
/*	saving the session information in the cache entry.
/* DIAGNOSTICS
/*	These routines terminate with a fatal run-time error
/*	for unrecoverable database errors. This allows the
/*	program to restart and reset the database to an
/*	empty initial state.
/*
/*	tls_scache_open() never returns on failure. All other
/*	functions return non-zero on success, zero when the
/*	operation could not be completed.
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

/* System library. */

#include <sys_defs.h>

#ifdef USE_TLS

#include <string.h>
#include <stddef.h>

/* Utility library. */

#include <msg.h>
#include <dict.h>
#include <stringops.h>
#include <mymalloc.h>
#include <hex_code.h>
#include <myflock.h>
#include <vstring.h>

/* Global library. */

/* TLS library. */

#include <tls_scache.h>

/* Application-specific. */

 /*
  * Session cache entry format.
  */
typedef struct {
    time_t  timestamp;			/* time when saved */
    char    session[1];			/* actually a bunch of bytes */
} TLS_SCACHE_ENTRY;

 /*
  * SLMs.
  */
#define STR(x)		vstring_str(x)
#define LEN(x)		VSTRING_LEN(x)

/* tls_scache_encode - encode TLS session cache entry */

static VSTRING *tls_scache_encode(TLS_SCACHE *cp, const char *cache_id,
				          const char *session,
				          ssize_t session_len)
{
    TLS_SCACHE_ENTRY *entry;
    VSTRING *hex_data;
    ssize_t binary_data_len;

    /*
     * Assemble the TLS session cache entry.
     * 
     * We could eliminate some copying by using incremental encoding, but
     * sessions are so small that it really does not matter.
     */
    binary_data_len = session_len + offsetof(TLS_SCACHE_ENTRY, session);
    entry = (TLS_SCACHE_ENTRY *) mymalloc(binary_data_len);
    entry->timestamp = time((time_t *) 0);
    memcpy(entry->session, session, session_len);

    /*
     * Encode the TLS session cache entry.
     */
    hex_data = vstring_alloc(2 * binary_data_len + 1);
    hex_encode(hex_data, (char *) entry, binary_data_len);

    /*
     * Logging.
     */
    if (cp->verbose)
	msg_info("write %s TLS cache entry %s: time=%ld [data %ld bytes]",
		 cp->cache_label, cache_id, (long) entry->timestamp,
		 (long) session_len);

    /*
     * Clean up.
     */
    myfree((char *) entry);

    return (hex_data);
}

/* tls_scache_decode - decode TLS session cache entry */

static int tls_scache_decode(TLS_SCACHE *cp, const char *cache_id,
			         const char *hex_data, ssize_t hex_data_len,
			             VSTRING *out_session)
{
    TLS_SCACHE_ENTRY *entry;
    VSTRING *bin_data;

    /*
     * Sanity check.
     */
    if (hex_data_len < 2 * (offsetof(TLS_SCACHE_ENTRY, session))) {
	msg_warn("%s TLS cache: truncated entry for %s: %.100s",
		 cp->cache_label, cache_id, hex_data);
	return (0);
    }

    /*
     * Disassemble the TLS session cache entry.
     * 
     * No early returns or we have a memory leak.
     */
#define FREE_AND_RETURN(ptr, x) { vstring_free(ptr); return (x); }

    bin_data = vstring_alloc(hex_data_len / 2 + 1);
    if (hex_decode(bin_data, hex_data, hex_data_len) == 0) {
	msg_warn("%s TLS cache: malformed entry for %s: %.100s",
		 cp->cache_label, cache_id, hex_data);
	FREE_AND_RETURN(bin_data, 0);
    }
    entry = (TLS_SCACHE_ENTRY *) STR(bin_data);

    /*
     * Logging.
     */
    if (cp->verbose)
	msg_info("read %s TLS cache entry %s: time=%ld [data %ld bytes]",
		 cp->cache_label, cache_id, (long) entry->timestamp,
	      (long) (LEN(bin_data) - offsetof(TLS_SCACHE_ENTRY, session)));

    /*
     * Other mandatory restrictions.
     */
    if (entry->timestamp + cp->timeout < time((time_t *) 0))
	FREE_AND_RETURN(bin_data, 0);

    /*
     * Optional output.
     */
    if (out_session != 0)
	vstring_memcpy(out_session, entry->session,
		       LEN(bin_data) - offsetof(TLS_SCACHE_ENTRY, session));

    /*
     * Clean up.
     */
    FREE_AND_RETURN(bin_data, 1);
}

/* tls_scache_lookup - load session from cache */

int     tls_scache_lookup(TLS_SCACHE *cp, const char *cache_id,
			          VSTRING *session)
{
    const char *hex_data;

    /*
     * Logging.
     */
    if (cp->verbose)
	msg_info("lookup %s session id=%s", cp->cache_label, cache_id);

    /*
     * Initialize. Don't leak data.
     */
    if (session)
	VSTRING_RESET(session);

    /*
     * Search the cache database.
     */
    if ((hex_data = dict_get(cp->db, cache_id)) == 0)
	return (0);

    /*
     * Decode entry and delete if expired or malformed.
     */
    if (tls_scache_decode(cp, cache_id, hex_data, strlen(hex_data),
			  session) == 0) {
	tls_scache_delete(cp, cache_id);
	return (0);
    } else {
	return (1);
    }
}

/* tls_scache_update - save session to cache */

int     tls_scache_update(TLS_SCACHE *cp, const char *cache_id,
			          const char *buf, ssize_t len)
{
    VSTRING *hex_data;

    /*
     * Logging.
     */
    if (cp->verbose)
	msg_info("put %s session id=%s [data %ld bytes]",
		 cp->cache_label, cache_id, (long) len);

    /*
     * Encode the cache entry.
     */
    hex_data = tls_scache_encode(cp, cache_id, buf, len);

    /*
     * Store the cache entry.
     * 
     * XXX Berkeley DB supports huge database keys and values. SDBM seems to
     * have a finite limit, and DBM simply can't be used at all.
     */
    dict_put(cp->db, cache_id, STR(hex_data));

    /*
     * Clean up.
     */
    vstring_free(hex_data);

    return (1);
}

/* tls_scache_sequence - get first/next TLS session cache entry */

int     tls_scache_sequence(TLS_SCACHE *cp, int first_next,
			            char **out_cache_id,
			            VSTRING *out_session)
{
    const char *member;
    const char *value;
    char   *saved_cursor;
    int     found_entry;
    int     keep_entry;
    char   *saved_member;

    /*
     * XXX Deleting entries while enumerating a map can he tricky. Some map
     * types have a concept of cursor and support a "delete the current
     * element" operation. Some map types without cursors don't behave well
     * when the current first/next entry is deleted (example: with Berkeley
     * DB < 2, the "next" operation produces garbage). To avoid trouble, we
     * delete an expired entry after advancing the current first/next
     * position beyond it, and ignore client requests to delete the current
     * entry.
     */

    /*
     * Find the first or next database entry. Activate the passivated entry
     * and check the time stamp. Schedule the entry for deletion if it is too
     * old.
     * 
     * Save the member (cache id) so that it will not be clobbered by the
     * tls_scache_lookup() call below.
     */
    found_entry = (dict_seq(cp->db, first_next, &member, &value) == 0);
    if (found_entry) {
	keep_entry = tls_scache_decode(cp, member, value, strlen(value),
				       out_session);
	if (keep_entry && out_cache_id)
	    *out_cache_id = mystrdup(member);
	saved_member = mystrdup(member);
    }

    /*
     * Delete behind. This is a no-op if an expired cache entry was updated
     * in the mean time. Use the saved lookup criteria so that the "delete
     * behind" operation works as promised.
     */
    if (cp->flags & TLS_SCACHE_FLAG_DEL_SAVED_CURSOR) {
	cp->flags &= ~TLS_SCACHE_FLAG_DEL_SAVED_CURSOR;
	saved_cursor = cp->saved_cursor;
	cp->saved_cursor = 0;
	tls_scache_lookup(cp, saved_cursor, (VSTRING *) 0);
	myfree(saved_cursor);
    }

    /*
     * Otherwise, clean up if this is not the first iteration.
     */
    else {
	if (cp->saved_cursor)
	    myfree(cp->saved_cursor);
	cp->saved_cursor = 0;
    }

    /*
     * Protect the current first/next entry against explicit or implied
     * client delete requests, and schedule a bad or expired entry for
     * deletion. Save the lookup criteria so that the "delete behind"
     * operation will work as promised.
     */
    if (found_entry) {
	cp->saved_cursor = saved_member;
	if (keep_entry == 0)
	    cp->flags |= TLS_SCACHE_FLAG_DEL_SAVED_CURSOR;
    }
    return (found_entry);
}

/* tls_scache_delete - delete session from cache */

int     tls_scache_delete(TLS_SCACHE *cp, const char *cache_id)
{

    /*
     * Logging.
     */
    if (cp->verbose)
	msg_info("delete %s session id=%s", cp->cache_label, cache_id);

    /*
     * Do it, unless we would delete the current first/next entry. Some map
     * types don't have cursors, and some of those don't behave when the
     * "current" entry is deleted.
     */
    return ((cp->saved_cursor != 0 && strcmp(cp->saved_cursor, cache_id) == 0)
	    || dict_del(cp->db, cache_id) == 0);
}

/* tls_scache_open - open TLS session cache file */

TLS_SCACHE *tls_scache_open(const char *dbname, const char *cache_label,
			            int verbose, int timeout)
{
    TLS_SCACHE *cp;
    DICT   *dict;

    /*
     * Logging.
     */
    if (verbose)
	msg_info("open %s TLS cache %s", cache_label, dbname);

    /*
     * Open the dictionary with O_TRUNC, so that we never have to worry about
     * opening a damaged file after some process terminated abnormally.
     */
#ifdef SINGLE_UPDATER
#define DICT_FLAGS (DICT_FLAG_DUP_REPLACE)
#else
#define DICT_FLAGS \
	(DICT_FLAG_DUP_REPLACE | DICT_FLAG_LOCK | DICT_FLAG_SYNC_UPDATE)
#endif

    dict = dict_open(dbname, O_RDWR | O_CREAT | O_TRUNC, DICT_FLAGS);

    /*
     * Sanity checks.
     */
    if (dict->lock_fd < 0)
	msg_fatal("dictionary %s is not a regular file", dbname);
#ifdef SINGLE_UPDATER
    if (myflock(dict->lock_fd, INTERNAL_LOCK,
		MYFLOCK_OP_EXCLUSIVE | MYFLOCK_OP_NOWAIT) < 0)
	msg_fatal("cannot lock dictionary %s for exclusive use: %m", dbname);
#endif
    if (dict->update == 0)
	msg_fatal("dictionary %s does not support update operations", dbname);
    if (dict->delete == 0)
	msg_fatal("dictionary %s does not support delete operations", dbname);
    if (dict->sequence == 0)
	msg_fatal("dictionary %s does not support sequence operations", dbname);

    /*
     * Create the TLS_SCACHE object.
     */
    cp = (TLS_SCACHE *) mymalloc(sizeof(*cp));
    cp->flags = 0;
    cp->db = dict;
    cp->cache_label = mystrdup(cache_label);
    cp->verbose = verbose;
    cp->timeout = timeout;
    cp->saved_cursor = 0;

    return (cp);
}

/* tls_scache_close - close TLS session cache file */

void    tls_scache_close(TLS_SCACHE *cp)
{

    /*
     * Logging.
     */
    if (cp->verbose)
	msg_info("close %s TLS cache %s", cp->cache_label, cp->db->name);

    /*
     * Destroy the TLS_SCACHE object.
     */
    dict_close(cp->db);
    myfree(cp->cache_label);
    if (cp->saved_cursor)
	myfree(cp->saved_cursor);
    myfree((char *) cp);
}

#endif
