/*	$NetBSD: tls_scache.c,v 1.1.1.1 2005/08/18 21:11:08 rpaulo Exp $	*/

/*++
/* NAME
/*	tls_scache 3
/* SUMMARY
/*	TLS session cache manager
/* SYNOPSIS
/*	#include <tls_scache.h>
/*
/*	TLS_SCACHE *tls_scache_open(dbname, cache_label, log_level, timeout)
/*	const char *dbname
/*	const char *cache_label;
/*	int	log_level;
/*	int	timeout;
/*
/*	void	tls_scache_close(cache)
/*	TLS_SCACHE *cache;
/*
/*	int	tls_scache_lookup(cache, cache_id, openssl_version,
/*				flags, out_openssl_version, out_flags,
/*				out_session)
/*	TLS_SCACHE *cache;
/*	const char *cache_id;
/*	long	openssl_version;
/*	int	flags;
/*	long	*out_openssl_version;
/*	int	*out_flags;
/*	VSTRING	*out_session;
/*
/*	int	tls_scache_update(cache, cache_id, openssl_version,
/*				flags, session, session_len)
/*	TLS_SCACHE *cache;
/*	const char *cache_id;
/*	long	openssl_version;
/*	int	flags;
/*	const char *session;
/*	int	session_len;
/*
/*	int	tls_scache_sequence(cache, first_next, openssl_version, flags,
/*				out_cache_id, out_openssl_version, out_flags,
/*				VSTRING *out_session)
/*	TLS_SCACHE *cache;
/*	int	first_next;
/*	long	openssl_version;
/*	int	flags;
/*	char	**out_cache_id;
/*	long	*out_openssl_version;
/*	int	*out_flags;
/*	VSTRING	*out_session;
/*
/*	int	tls_scache_delete(cache, cache_id)
/*	TLS_SCACHE *cache;
/*	const char *cache_id;
/* DESCRIPTION
/*	This module maintains Postfix TLS session cache files.
/*	each session is stored under a lookup key (hostname or
/*	session ID) together with the OpenSSL version that
/*	created the session and application-specific flags.
/*	Upon lookup, the OpenSSL version and flags can be
/*	specified as optional filters. Entries that don't
/*	satisfy the filter requirements are silently deleted.
/*
/*	tls_scache_open() opens the specified TLS session cache
/*	and returns a handle that must be used for subsequent
/*	access.
/*
/*	tls_scache_close() closes the specified TLS session cache
/*	and releases memory that was allocated by tls_scache_open().
/*
/*	tls_scache_lookup() looks up the specified session in the
/*	specified cache, and applies the session timeout, openssl
/*	version and flags restrictions. Entries that don't satisfy
/*	the requirements are silently deleted.
/*
/*	tls_scache_update() updates the specified TLS session cache
/*	with the specified session information.
/*
/*	tls_scache_sequence() iterates over the specified TLS session
/*	cache and looks up the first or next entry. If that entry
/*	matches the session timeout, OpenSSL version and flags
/*	restrictions, tls_scache_sequence() saves the entry by
/*	updating the result parameters; otherwise it deletes the
/*	entry and does not update the result parameters.  Specify
/*	TLS_SCACHE_SEQUENCE_NOTHING
/*	as the third and last argument to disable OpenSSL version
/*	and flags restrictions, and to disable saving of cache
/*	entry content or cache entry ID information.  This is useful
/*	when purging expired entries. A result value of zero means
/*	that the end of the cache was reached.
/*
/*	tls_scache_delete() removes the specified cache entry from
/*	the specified TLS session cache.
/*
/*	Arguments:
/* .IP dbname
/*	The base name of the session cache file.
/* .IP cache_label
/*	A string that is used in logging and error messages.
/* .IP log_level
/*	The logging level for cache operations.
/* .IP timeout
/*	The time after wich a session cache entry is considered too old.
/* .IP first_next
/*	One of DICT_SEQ_FUN_FIRST (first cache element) or DICT_SEQ_FUN_NEXT
/*	(next cache element).
/* .IP cache_id
/*	Session cache lookup key.
/* .IP openssl_version
/*	When storing information, the OpenSSL version that generated a
/*	session. When retrieving information, delete cache entries that
/*	don't match the specified OpenSSL version.
/*
/*	Specify TLS_SCACHE_ANY_OPENSSL_VSN to match any OpenSSL version.
/* .IP flags
/*	When storing information, application flags that specify properties
/*	of a session. When retrieving information, delete cache entries that
/*	have the specified flags set.
/*
/*	Specify TLS_SCACHE_ANY_FLAGS to match any flags value.
/* .IP session
/*	Storage for session information.
/* .IP session_len
/*	The size of the session information in bytes.
/* .IP out_cache_id
/* .IP out_openssl_version
/* .IP out_flags
/* .IP out_session
/*	Storage for saving the cache_id, openssl_version, flags
/*	or session information of the current cache entry.
/*
/*	Specify TLS_SCACHE_DONT_NEED_CACHE_ID to avoid saving
/*	the session cache ID of the cache entry.
/*
/*	Specify TLS_SCACHE_DONT_NEED_OPENSSL_VSN to avoid
/*	saving the OpenSSL version in the cache entry.
/*
/*	Specify TLS_SCACHE_DONT_NEED_FLAGS to avoid
/*	saving the flags information in the cache entry.
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
  * 
  * XXX The session cache version number is not needed because we truncate the
  * database when it is opened.
  */
typedef struct {
    long    scache_db_version;		/* obsolete */
    long    openssl_version;		/* clients may differ... */
    time_t  timestamp;			/* time when saved */
    int     flags;			/* enforcement etc. */
    char    session[1];			/* actually a bunch of bytes */
} TLS_SCACHE_ENTRY;

#define TLS_SCACHE_DB_VERSION	0x00000003L

 /*
  * SLMs.
  */
#define STR(x)		vstring_str(x)
#define LEN(x)		VSTRING_LEN(x)

/* tls_scache_encode - encode TLS session cache entry */

static VSTRING *tls_scache_encode(TLS_SCACHE *cp, const char *cache_id,
				          long openssl_version, int flags,
				          const char *session,
				          int session_len)
{
    TLS_SCACHE_ENTRY *entry;
    VSTRING *hex_data;
    int     binary_data_len;

    /*
     * Assemble the TLS session cache entry.
     * 
     * We could eliminate some copying by using incremental encoding, but
     * sessions are so small that it really does not matter.
     */
    binary_data_len = session_len + offsetof(TLS_SCACHE_ENTRY, session);
    entry = (TLS_SCACHE_ENTRY *) mymalloc(binary_data_len);
    entry->scache_db_version = TLS_SCACHE_DB_VERSION;
    entry->openssl_version = openssl_version;
    entry->timestamp = time((time_t *) 0);
    entry->flags = flags;
    memcpy(entry->session, session, session_len);

    /*
     * Encode the TLS session cache entry.
     */
    hex_data = vstring_alloc(2 * binary_data_len + 1);
    hex_encode(hex_data, (char *) entry, binary_data_len);

    /*
     * Logging.
     */
    if (cp->log_level >= 3)
	msg_info("write %s TLS cache entry %s: cache_version=%ld"
	       " openssl_version=0x%lx flags=0x%x time=%ld [data %d bytes]",
		 cp->cache_label, cache_id,
		 (long) entry->scache_db_version,
		 (long) entry->openssl_version,
		 entry->flags,
		 (long) entry->timestamp,
		 session_len);

    /*
     * Clean up.
     */
    myfree((char *) entry);

    return (hex_data);
}

/* tls_scache_decode - decode TLS session cache entry */

static int tls_scache_decode(TLS_SCACHE *cp, const char *cache_id,
			             const char *hex_data, int hex_data_len,
			             long openssl_version, int flags,
			             long *out_openssl_version,
			             int *out_flags,
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
     * Disassemble the TLS session cache entry and enforce the restrictions
     * specified as version numbers or flags.
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

    /*
     * Before doing anything else, verify that the database format version
     * matches this program.
     */
    entry = (TLS_SCACHE_ENTRY *) STR(bin_data);
    if (entry->scache_db_version != TLS_SCACHE_DB_VERSION) {
	msg_warn("%s TLS cache: cache version mis-match for %s: 0x%lx != 0x%lx",
		 cp->cache_label, cache_id, entry->scache_db_version,
		 TLS_SCACHE_DB_VERSION);
	FREE_AND_RETURN(bin_data, 0);
    }

    /*
     * Logging.
     */
    if (cp->log_level >= 3)
	msg_info("read %s TLS cache entry %s: cache_version=%ld"
	       " openssl_version=0x%lx time=%ld flags=0x%x [data %d bytes]",
		 cp->cache_label, cache_id, (long) entry->scache_db_version,
		 (long) entry->openssl_version, (long) entry->timestamp,
		 entry->flags,
	       (int) (LEN(bin_data) - offsetof(TLS_SCACHE_ENTRY, session)));

    /*
     * Other mandatory restrictions.
     */
    if (entry->timestamp + cp->timeout < time((time_t *) 0))
	FREE_AND_RETURN(bin_data, 0);

    /*
     * Optional restrictions.
     */
    if (openssl_version != 0 && entry->openssl_version != openssl_version) {
	msg_warn("%s TLS cache: openssl version mis-match for %s: 0x%lx != 0x%lx",
		 cp->cache_label, cache_id, entry->openssl_version,
		 openssl_version);
	FREE_AND_RETURN(bin_data, 0);
    }
    if (flags != 0 && (entry->flags & flags) != flags) {
	msg_warn("%s TLS cache: flags mis-match for %s: 0x%x is not subset of 0x%x",
		 cp->cache_label, cache_id, entry->flags, flags);
	FREE_AND_RETURN(bin_data, 0);
    }

    /*
     * Optional output.
     */
    if (out_openssl_version != 0)
	*out_openssl_version = entry->openssl_version;
    if (out_flags != 0)
	*out_flags = entry->flags;
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
			          long openssl_version, int flags,
			          long *out_openssl_version, int *out_flags,
			          VSTRING *session)
{
    const char *hex_data;

    /*
     * Logging.
     */
    if (cp->log_level >= 3)
	msg_info("lookup %s session id=%s ssl=0x%lx flags=0x%x",
		 cp->cache_label, cache_id, openssl_version, flags);

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
     * Decode entry and verify version and flags information.
     * 
     * XXX We throw away sessions when flags don't match. If we want to allow
     * for co-existing cache entries with different flags, the flags would
     * have to be encoded in the cache lookup key.
     */
    if (tls_scache_decode(cp, cache_id, hex_data, strlen(hex_data),
			  openssl_version, flags, out_openssl_version,
			  out_flags, session) == 0) {
	tls_scache_delete(cp, cache_id);
	return (0);
    } else {
	return (1);
    }
}

/* tls_scache_update - save session to cache */

int     tls_scache_update(TLS_SCACHE *cp, const char *cache_id,
			          long openssl_version, int flags,
			          const char *buf, int len)
{
    VSTRING *hex_data;

    /*
     * Logging.
     */
    if (cp->log_level >= 3)
	msg_info("put %s session id=%s ssl=0x%lx flags=0x%x [data %d bytes]",
		 cp->cache_label, cache_id, openssl_version, flags, len);

    /*
     * Encode the cache entry.
     */
    hex_data =
	tls_scache_encode(cp, cache_id, openssl_version, flags, buf, len);

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
			            long openssl_version,
			            int flags,
			            char **out_cache_id,
			            long *out_openssl_version,
			            int *out_flags,
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
     * and check the version, time stamp and flags information. Schedule the
     * entry for deletion if it is bad or too old.
     * 
     * Save the member (cache id) so that it will not be clobbered by the
     * tls_scache_lookup() call below.
     */
    found_entry = (dict_seq(cp->db, first_next, &member, &value) == 0);
    if (found_entry) {
	keep_entry = tls_scache_decode(cp, member, value, strlen(value),
				       openssl_version, flags,
				       out_openssl_version,
				       out_flags, out_session);
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
	tls_scache_lookup(cp, saved_cursor, cp->saved_openssl_version,
			  cp->saved_flags, (long *) 0, (int *) 0,
			  (VSTRING *) 0);
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
	if (keep_entry == 0) {
	    cp->flags |= TLS_SCACHE_FLAG_DEL_SAVED_CURSOR;
	    cp->saved_openssl_version = openssl_version;
	    cp->saved_flags = flags;
	}
    }
    return (found_entry);
}

/* tls_scache_delete - delete session from cache */

int     tls_scache_delete(TLS_SCACHE *cp, const char *cache_id)
{

    /*
     * Logging.
     */
    if (cp->log_level >= 3)
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
			            int log_level, int timeout)
{
    TLS_SCACHE *cp;
    DICT   *dict;

    /*
     * Logging.
     */
    if (log_level >= 3)
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
    cp->log_level = log_level;
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
    if (cp->log_level >= 3)
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
