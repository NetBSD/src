/*	$NetBSD: nbdb_redirect.c,v 1.2.2.2 2026/05/11 17:13:49 martin Exp $	*/

/*++
/* NAME
/*	nbdb_redirect 3
/* SUMMARY
/*	Non-Berkeley-DB migration support
/* SYNOPSIS
/*	#include <nbdb_redirect.h>
/*
/*	void	nbdb_rdr_init(int level)
/* DESCRIPTION
/*	This module contains code that may be called by command-line
/*	tools and by clients of the non-Berkeley-DB migration service.
/*
/*	nbdb_rdr_init() destructively registers surrogate dict_open()
/*	and mkmap_open() handlers for legacy types 'hash' and 'btree',
/*	that map these type names to $var_db_type and $var_cache_db_type,
/*	respectively, and that may send requests to a reindexing
/*	server. The mapping from legacy to non-legacy type names is
/*	implemented by nbdb_map_leg_type().
/* IMPLICIT INPUTS
/*	var_nbdb_log_redirect, controls logging of 'redirect' activity
/* SEE ALSO
/*	nbdb_reindexd(8), helper daemon to reindex legacy tables
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <errno.h>
#include <string.h>

 /*
  * Utility library.
  */
#include <dict_cdb.h>
#include <dict_db.h>
#include <dict.h>
#include <dict_lmdb.h>
#include <mkmap.h>
#include <msg.h>
#include <mymalloc.h>
#include <name_code.h>
#include <stringops.h>
#include <vstring.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <nbdb_clnt.h>
#include <nbdb_redirect.h>
#include <nbdb_util.h>

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

 /*
  * Application-specific.
  */

static VSTRING *why = 0;

/* call_nbdb_reindexd - wrapper for nbdb_reindexd client */

static DICT *call_nbdb_reindexd(const char *leg_type, const char *name,
				        int open_flags, int dict_flags)
{
    DICT   *surrogate;
    VSTRING *why;
    int     nbdb_stat;

    /*
     * TODO(wietse) make this go away.
     */
#define MAX_TRY 2
#define TIMEOUT 30
#define DELAY 1


    why = vstring_alloc(100);
    switch (nbdb_stat = nbdb_clnt_request(leg_type, name, MAX_TRY, TIMEOUT,
					  DELAY, why)) {
    case NBDB_STAT_OK:
	if (msg_verbose)
	    msg_info("%s: delegated '%s:%s'", __func__, leg_type, name);
	surrogate = 0;
	break;
    case NBDB_STAT_ERROR:
	msg_warn("Non-Berkeley-DB migration error: %s", STR(why));
	surrogate =
	    dict_surrogate(leg_type, name, open_flags, dict_flags,
			   "Non-Berkeley-DB migration error: %s", STR(why));
	break;
    default:
	msg_warn("Non-Berkeley-DB migration error: malformed "
		 "bad reindexing status: %d", nbdb_stat);
	surrogate = dict_surrogate(leg_type, name, open_flags, dict_flags,
				   "Non-Berkeley-DB migration error: "
				   "bad reindexing server status: %d",
				   nbdb_stat);
    }
    vstring_free(why);
    return (surrogate);
}

 /*
  * Code to redirect dict_open() requests to non-legacy database types. This
  * code may be called from daemon processes, and therefore all errors are
  * handled by creating a surrogate database instance that returns an error
  * status for each request, and that logs why.
  * 
  * Program flow:
  * 
  * - A client program calls dict_open().
  * 
  * - dict_open() looks up "our" DICT_OPEN_INFO with dict_fn and mkmap_fn
  * members that resolve to functions in this code module.
  * 
  * - dict_open() calls our DICT_OPEN_INFO.dict_fn() function which calls the
  * dict_xxx_open() method of the non-legacy type. That function returns a
  * DICT object for the non-legacy implementation (typically, cdb or lmdb).
  */

/* nbdb_dict_open - redirect dict_open() to non-Berkeley-DB type */

static DICT *nbdb_dict_open(const char *leg_type, const char *name,
			            int open_flags, int dict_flags)
{
    const DICT_OPEN_INFO *open_info;
    const char *non_leg_type;
    char   *non_leg_index_path;
    const char *non_leg_suffix;
    int     stat_res, saved_errno;
    DICT   *surrogate;
    struct stat st;

    if (why == 0)
	why = vstring_alloc(100);

    /*
     * Look up the type mapping, which may depend on the database name.
     */
    non_leg_type = nbdb_map_leg_type(leg_type, name, &open_info, why);
    if (non_leg_type == 0) {
	msg_warn("non-Berkeley-DB type mapping lookup error for '%s:%s': %s",
		 leg_type, name, STR(why));
	return (dict_surrogate(leg_type, name, open_flags, dict_flags,
			       "Berkeley DB type mapping lookup error for "
			       "'%s:%s': %s", leg_type, name, STR(why)));
    }
    if (var_nbdb_log_redirect)
	msg_info("redirecting %s:%s to %s:%s", leg_type, name,
		 non_leg_type, name);

    /*
     * Do not delegate dict_open() requests to create an indexed file (this
     * is typical for daemon-maintained caches). If a process does not have
     * permissions to create the indexed file, then the nbdb reindexing
     * service must not create it for them.
     */
    if ((open_flags & O_CREAT) == 0
	&& nbdb_level >= NBDB_LEV_CODE_REINDEX) {

	/*
	 * If the non-legacy indexed file does not already exist, call the
	 * nbdb reindexing service to create a non-legacy indexed file, and
	 * then access the indexed file directly. The service will enforce a
	 * safety policy based on the ownership of the legacy indexed file.
	 */
	if ((non_leg_suffix = nbdb_find_non_leg_suffix(non_leg_type)) == 0) {
	    msg_warn("non-Berkeley-DB mapped type '%s' has no known pathname "
		     "suffix", non_leg_type);
	    return (dict_surrogate(leg_type, name, open_flags, dict_flags,
				   "non-Berkeley-DB mapped type '%s' has no "
				   "known pathname suffix", non_leg_type));
	}
	non_leg_index_path = concatenate(name, non_leg_suffix, (char *) 0);
	stat_res = stat (non_leg_index_path, &st);

	saved_errno = errno;
	myfree(non_leg_index_path);
	if (stat_res < 0 && saved_errno == ENOENT
	    && (surrogate = call_nbdb_reindexd(leg_type, name, open_flags,
					       dict_flags)) != 0)
	    return (surrogate);
    }
    return (open_info->dict_fn(name, open_flags, dict_flags));
}

/* nbdb_dict_hash_open - map hash legacy type to non-legacy type */

static DICT *nbdb_dict_hash_open(const char *name, int open_flags, int dict_flags)
{
    return (nbdb_dict_open(DICT_TYPE_HASH, name, open_flags, dict_flags));
}

/* nbdb_dict_btree_open - map btree legacy type to non-legacy type */

static DICT *nbdb_dict_btree_open(const char *name, int open_flags, int dict_flags)
{
    return (nbdb_dict_open(DICT_TYPE_BTREE, name, open_flags, dict_flags));
}

 /*
  * Code that is used by postmap and postalias bulk create and incremental
  * mode, to redirect requests with a legacy type name to a non-legacy type.
  * These requests are always executed without delegation to a privileged
  * daemon. If the user does not have permission to create or update the
  * table, then the request is invalid. As this code is called from
  * command-line tools, all errors are fatal.
  * 
  * Limitation: because this code does not call the reindexing service, this
  * will not work for postmap and postalias incremental updates ("postmap -i"
  * etc.) when a database has not yet been reindexed. Fortunately, Postfix
  * itself does not make incremental non-cache updates.
  * 
  * Program flow:
  * 
  * - The postmap or postalias commands call mkmap_open().
  * 
  * - mkmap_open() looks up "our" DICT_OPEN_INFO with dict_fn and mkmap_fn
  * members that resolve to functions in this code module.
  * 
  * - mkmap_open() calls our DICT_OPEN_INFO.mkmap_fn() function which calls the
  * mkmap_xxx_open() method of the non-legacy type. That function returns a
  * MKMAP object with function pointers for the non-legacy implementation
  * (typically, cdb or lmdb).
  * 
  * - mkmap_open() then calls MKMAP.open() (in bulk and incremental mode), and
  * calls other MKMAP function members as appropriate, for example to manage
  * a lock.
  */

/* nbdb_mkmap_open - redirect legacy type to non-legacy */

static MKMAP *nbdb_mkmap_open(const char *leg_type, const char *name)
{
    const DICT_OPEN_INFO *open_info;
    const char *non_leg_type;

    if (why == 0)
	why = vstring_alloc(100);

    /*
     * Look up the legacy type to non-legacy type mapping, which may depend
     * on the database name. Errors are fatal.
     */
    non_leg_type = nbdb_map_leg_type(leg_type, name, &open_info, why);
    if (non_leg_type == 0)
	msg_fatal("non_bdb migration mapping lookup error for '%s': %s",
		  leg_type, STR(why));
    if (var_nbdb_log_redirect)
	msg_info("redirecting %s:%s to %s:%s", leg_type, name,
		 non_leg_type, name);
    if (open_info->mkmap_fn == 0)
	msg_fatal("no 'map create' support for non_bdb mapped type: %s",
		  non_leg_type);
    return (open_info->mkmap_fn(name));
}

/* nbdb_mkmap_hash_open - map hash legacy type to non-legacy type */

static MKMAP *nbdb_mkmap_hash_open(const char *path)
{
    return (nbdb_mkmap_open(DICT_TYPE_HASH, path));
}

/* nbdb_mkmap_btree_open - map btree legacy type to non-legacy type */

static MKMAP *nbdb_mkmap_btree_open(const char *path)
{
    return (nbdb_mkmap_open(DICT_TYPE_BTREE, path));
}

 /*
  * Surrogate hash: and btree: support for dict_open(). These handlers will
  * typically redirect hash: requests to the default database type (for
  * mostly-constant tables) and will redirect btree: requests to the default
  * cache type (mostly for daemon cache).
  */
static const DICT_OPEN_INFO nbdb_info[] = {
    DICT_TYPE_HASH, nbdb_dict_hash_open, nbdb_mkmap_hash_open,
    DICT_TYPE_BTREE, nbdb_dict_btree_open, nbdb_mkmap_btree_open,
    0,
};

/* nbdb_rdr_init - redirect legacy open() requests to nbdb handlers */

void    nbdb_rdr_init(void)
{
    const DICT_OPEN_INFO *info;

    /*
     * Un-register each legacy type handler. This is needed if the system was
     * built without -DNO_DB). Then, register the forward-compatibility
     * handlers.
     */
    if (nbdb_level >= NBDB_LEV_CODE_REDIRECT) {
	for (info = nbdb_info; info->type; info++) {
	    if (dict_open_lookup(info->type) != 0)
		dict_open_unregister(info->type);
	    if (msg_verbose)
		msg_info("%s: installing custom handlers for '%s'",
			 __func__, info->type);
	    dict_open_register(info);
	}
    }
}
