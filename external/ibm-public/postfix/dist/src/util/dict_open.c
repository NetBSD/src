/*	$NetBSD: dict_open.c,v 1.1.1.4.10.2 2014/08/19 23:59:45 tls Exp $	*/

/*++
/* NAME
/*	dict_open 3
/* SUMMARY
/*	low-level dictionary interface
/* SYNOPSIS
/*	#include <dict.h>
/*
/*	DICT	*dict_open(dict_spec, open_flags, dict_flags)
/*	const char *dict_spec;
/*	int	open_flags;
/*	int	dict_flags;
/*
/*	DICT	*dict_open3(dict_type, dict_name, open_flags, dict_flags)
/*	const char *dict_type;
/*	const char *dict_name;
/*	int	open_flags;
/*	int	dict_flags;
/*
/*	int	dict_put(dict, key, value)
/*	DICT	*dict;
/*	const char *key;
/*	const char *value;
/*
/*	const char *dict_get(dict, key)
/*	DICT	*dict;
/*	const char *key;
/*
/*	int	dict_del(dict, key)
/*	DICT	*dict;
/*	const char *key;
/*
/*	int	dict_seq(dict, func, key, value)
/*	DICT	*dict;
/*	int	func;
/*	const char **key;
/*	const char **value;
/*
/*	void	dict_close(dict)
/*	DICT	*dict;
/*
/*	dict_open_register(type, open)
/*	char	*type;
/*	DICT	*(*open) (const char *, int, int);
/*
/*	ARGV	*dict_mapnames()
/*
/*	int	dict_isjmp(dict)
/*	DICT	*dict;
/*
/*	int	dict_setjmp(dict)
/*	DICT	*dict;
/*
/*	int	dict_longjmp(dict, val)
/*	DICT	*dict;
/*	int	val;
/* DESCRIPTION
/*	This module implements a low-level interface to multiple
/*	physical dictionary types.
/*
/*	dict_open() takes a type:name pair that specifies a dictionary type
/*	and dictionary name, opens the dictionary, and returns a dictionary
/*	handle.  The \fIopen_flags\fR arguments are as in open(2). The
/*	\fIdict_flags\fR are the bit-wise OR of zero or more of the following:
/* .IP DICT_FLAG_DUP_WARN
/*	Warn about duplicate keys, if the underlying database does not
/*	support duplicate keys. The default is to terminate with a fatal
/*	error.
/* .IP DICT_FLAG_DUP_IGNORE
/*	Ignore duplicate keys if the underlying database does not
/*	support duplicate keys. The default is to terminate with a fatal
/*	error.
/* .IP DICT_FLAG_DUP_REPLACE
/*	Replace duplicate keys if the underlying database supports such
/*	an operation. The default is to terminate with a fatal error.
/* .IP DICT_FLAG_TRY0NULL
/*	With maps where this is appropriate, append no null byte to
/*	keys and values.
/*	When neither DICT_FLAG_TRY0NULL nor DICT_FLAG_TRY1NULL are
/*	specified, the software guesses what format to use for reading;
/*	and in the absence of definite information, a system-dependent
/*	default is chosen for writing.
/* .IP DICT_FLAG_TRY1NULL
/*	With maps where this is appropriate, append one null byte to
/*	keys and values.
/*	When neither DICT_FLAG_TRY0NULL nor DICT_FLAG_TRY1NULL are
/*	specified, the software guesses what format to use for reading;
/*	and in the absence of definite information, a system-dependent
/*	default is chosen for writing.
/* .IP DICT_FLAG_LOCK
/*	With maps where this is appropriate, acquire an exclusive lock
/*	before writing, and acquire a shared lock before reading.
/*	Release the lock when the operation completes.
/* .IP DICT_FLAG_OPEN_LOCK
/*	The behavior of this flag depends on whether a database
/*	sets the DICT_FLAG_MULTI_WRITER flag to indicate that it
/*	is multi-writer safe.
/*
/*	With databases that are not multi-writer safe, dict_open()
/*	acquires a persistent exclusive lock, or it terminates with
/*	a fatal run-time error.
/*
/*	With databases that are multi-writer safe, dict_open()
/*	downgrades the DICT_FLAG_OPEN_LOCK flag (persistent lock)
/*	to DICT_FLAG_LOCK (temporary lock).
/* .IP DICT_FLAG_FOLD_FIX
/*	With databases whose lookup fields are fixed-case strings,
/*	fold the search string to lower case before accessing the
/*	database.  This includes hash:, cdb:, dbm:. nis:, ldap:,
/*	*sql.
/* .IP DICT_FLAG_FOLD_MUL
/*	With databases where one lookup field can match both upper
/*	and lower case, fold the search key to lower case before
/*	accessing the database. This includes regexp: and pcre:
/* .IP DICT_FLAG_FOLD_ANY
/*	Short-hand for (DICT_FLAG_FOLD_FIX | DICT_FLAG_FOLD_MUL).
/* .IP DICT_FLAG_SYNC_UPDATE
/*	With file-based maps, flush I/O buffers to file after each update.
/*	Thus feature is not supported with some file-based dictionaries.
/* .IP DICT_FLAG_NO_REGSUB
/*	Disallow regular expression substitution from the lookup string
/*	into the lookup result, to block data injection attacks.
/* .IP DICT_FLAG_NO_PROXY
/*	Disallow access through the unprivileged \fBproxymap\fR
/*	service, to block privilege escalation attacks.
/* .IP DICT_FLAG_NO_UNAUTH
/*	Disallow lookup mechanisms that lack any form of authentication,
/*	to block privilege escalation attacks (example: tcp_table;
/*	even NIS can be secured to some extent by requiring that
/*	the server binds to a privileged port).
/* .IP DICT_FLAG_PARANOID
/*	A combination of all the paranoia flags: DICT_FLAG_NO_REGSUB,
/*	DICT_FLAG_NO_PROXY and DICT_FLAG_NO_UNAUTH.
/* .IP DICT_FLAG_BULK_UPDATE
/*	Enable preliminary code for bulk-mode database updates.
/*	The caller must create an exception handler with dict_jmp_alloc()
/*	and must trap exceptions from the database client with dict_setjmp().
/* .IP DICT_FLAG_DEBUG
/*	Enable additional logging.
/* .PP
/*	Specify DICT_FLAG_NONE for no special processing.
/*
/*	The dictionary types are as follows:
/* .IP environ
/*	The process environment array. The \fIdict_name\fR argument is ignored.
/* .IP dbm
/*	DBM file.
/* .IP hash
/*	Berkeley DB file in hash format.
/* .IP btree
/*	Berkeley DB file in btree format.
/* .IP nis
/*	NIS map. Only read access is supported.
/* .IP nisplus
/*	NIS+ map. Only read access is supported.
/* .IP netinfo
/*	NetInfo table. Only read access is supported.
/* .IP ldap
/*	LDAP ("light-weight" directory access protocol) database access.
/* .IP pcre
/*	PERL-compatible regular expressions.
/* .IP regexp
/*	POSIX-compatible regular expressions.
/* .IP texthash
/*	Flat text in postmap(1) input format.
/* .PP
/*	dict_open3() takes separate arguments for dictionary type and
/*	name, but otherwise performs the same functions as dict_open().
/*
/*	dict_get() retrieves the value stored in the named dictionary
/*	under the given key. A null pointer means the value was not found.
/*	As with dict_lookup(), the result is owned by the lookup table
/*	implementation. Make a copy if the result is to be modified,
/*	or if the result is to survive multiple table lookups.
/*
/*	dict_put() stores the specified key and value into the named
/*	dictionary. A zero (DICT_STAT_SUCCESS) result means the
/*	update was made.
/*
/*	dict_del() removes a dictionary entry, and returns
/*	DICT_STAT_SUCCESS in case of success.
/*
/*	dict_seq() iterates over all members in the named dictionary.
/*	func is define DICT_SEQ_FUN_FIRST (select first member) or
/*	DICT_SEQ_FUN_NEXT (select next member). A zero (DICT_STAT_SUCCESS)
/*	result means that an entry was found.
/*
/*	dict_close() closes the specified dictionary and cleans up the
/*	associated data structures.
/*
/*	dict_open_register() adds support for a new dictionary type.
/*
/*	dict_mapnames() returns a sorted list with the names of all available
/*	dictionary types.
/*
/*	dict_setjmp() saves processing context and makes that context
/*	available for use with dict_longjmp().  Normally, dict_setjmp()
/*	returns zero.  A non-zero result means that dict_setjmp()
/*	returned through a dict_longjmp() call; the result is the
/*	\fIval\fR argment given to dict_longjmp(). dict_isjmp()
/*	returns non-zero when dict_setjmp() and dict_longjmp()
/*	are enabled for a given dictionary.
/*
/*	NB: non-local jumps such as dict_longjmp() are not safe for
/*	jumping out of any routine that manipulates DICT data.
/*	longjmp() like calls are best avoided in signal handlers.
/* DIAGNOSTICS
/*	Fatal error: open error, unsupported dictionary type, attempt to
/*	update non-writable dictionary.
/*
/*	The lookup routine returns non-null when the request is
/*	satisfied. The update, delete and sequence routines return
/*	zero (DICT_STAT_SUCCESS) when the request is satisfied.
/*	The dict->errno value is non-zero only when the last operation
/*	was not satisfied due to a dictionary access error. This
/*	can have the following values:
/* .IP DICT_ERR_NONE(zero)
/*	There was no dictionary access error. For example, the
/*	request was satisfied, the requested information did not
/*	exist in the dictionary, or the information already existed
/*	when it should not exist (collision).
/* .IP DICT_ERR_RETRY(<0)
/*	The dictionary was temporarily unavailable. This can happen
/*	with network-based services.
/* .IP DICT_ERR_CONFIG(<0)
/*	The dictionary was unavailable due to a configuration error.
/* .PP
/*	Generally, a program is expected to test the function result
/*	value for "success" first. If the operation was not successful,
/*	a program is expected to test for a non-zero dict->error
/*	status to distinguish between a data notfound/collision
/*	condition or a dictionary access error.
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
#include <string.h>
#include <stdlib.h>

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <argv.h>
#include <mymalloc.h>
#include <msg.h>
#include <dict.h>
#include <dict_cdb.h>
#include <dict_env.h>
#include <dict_unix.h>
#include <dict_tcp.h>
#include <dict_sdbm.h>
#include <dict_dbm.h>
#include <dict_db.h>
#include <dict_lmdb.h>
#include <dict_nis.h>
#include <dict_nisplus.h>
#include <dict_ni.h>
#include <dict_pcre.h>
#include <dict_regexp.h>
#include <dict_static.h>
#include <dict_cidr.h>
#include <dict_ht.h>
#include <dict_thash.h>
#include <dict_sockmap.h>
#include <dict_fail.h>
#include <stringops.h>
#include <split_at.h>
#include <htable.h>
#include <myflock.h>

 /*
  * lookup table for available map types.
  */
typedef struct {
    char   *type;
    struct DICT *(*open) (const char *, int, int);
} DICT_OPEN_INFO;

static const DICT_OPEN_INFO dict_open_info[] = {
#ifdef HAS_CDB
    DICT_TYPE_CDB, dict_cdb_open,
#endif
    DICT_TYPE_ENVIRON, dict_env_open,
    DICT_TYPE_HT, dict_ht_open,
    DICT_TYPE_UNIX, dict_unix_open,
    DICT_TYPE_TCP, dict_tcp_open,
#ifdef HAS_SDBM
    DICT_TYPE_SDBM, dict_sdbm_open,
#endif
#ifdef HAS_DBM
    DICT_TYPE_DBM, dict_dbm_open,
#endif
#ifdef HAS_DB
    DICT_TYPE_HASH, dict_hash_open,
    DICT_TYPE_BTREE, dict_btree_open,
#endif
#ifdef HAS_LMDB
    DICT_TYPE_LMDB, dict_lmdb_open,
#endif
#ifdef HAS_NIS
    DICT_TYPE_NIS, dict_nis_open,
#endif
#ifdef HAS_NISPLUS
    DICT_TYPE_NISPLUS, dict_nisplus_open,
#endif
#ifdef HAS_NETINFO
    DICT_TYPE_NETINFO, dict_ni_open,
#endif
#ifdef HAS_PCRE
    DICT_TYPE_PCRE, dict_pcre_open,
#endif
#ifdef HAS_POSIX_REGEXP
    DICT_TYPE_REGEXP, dict_regexp_open,
#endif
    DICT_TYPE_STATIC, dict_static_open,
    DICT_TYPE_CIDR, dict_cidr_open,
    DICT_TYPE_THASH, dict_thash_open,
    DICT_TYPE_SOCKMAP, dict_sockmap_open,
    DICT_TYPE_FAIL, dict_fail_open,
    0,
};

static HTABLE *dict_open_hash;

/* dict_open_init - one-off initialization */

static void dict_open_init(void)
{
    const char *myname = "dict_open_init";
    const DICT_OPEN_INFO *dp;

    if (dict_open_hash != 0)
	msg_panic("%s: multiple initialization", myname);
    dict_open_hash = htable_create(10);

    for (dp = dict_open_info; dp->type; dp++)
	htable_enter(dict_open_hash, dp->type, (char *) dp);
}

/* dict_open - open dictionary */

DICT   *dict_open(const char *dict_spec, int open_flags, int dict_flags)
{
    char   *saved_dict_spec = mystrdup(dict_spec);
    char   *dict_name;
    DICT   *dict;

    if ((dict_name = split_at(saved_dict_spec, ':')) == 0)
	msg_fatal("open dictionary: expecting \"type:name\" form instead of \"%s\"",
		  dict_spec);

    dict = dict_open3(saved_dict_spec, dict_name, open_flags, dict_flags);
    myfree(saved_dict_spec);
    return (dict);
}


/* dict_open3 - open dictionary */

DICT   *dict_open3(const char *dict_type, const char *dict_name,
		           int open_flags, int dict_flags)
{
    const char *myname = "dict_open";
    DICT_OPEN_INFO *dp;
    DICT   *dict;

    if (*dict_type == 0 || *dict_name == 0)
	msg_fatal("open dictionary: expecting \"type:name\" form instead of \"%s:%s\"",
		  dict_type, dict_name);
    if (dict_open_hash == 0)
	dict_open_init();
    if ((dp = (DICT_OPEN_INFO *) htable_find(dict_open_hash, dict_type)) == 0)
	return (dict_surrogate(dict_type, dict_name, open_flags, dict_flags,
			     "unsupported dictionary type: %s", dict_type));
    if ((dict = dp->open(dict_name, open_flags, dict_flags)) == 0)
	return (dict_surrogate(dict_type, dict_name, open_flags, dict_flags,
			    "cannot open %s:%s: %m", dict_type, dict_name));
    if (msg_verbose)
	msg_info("%s: %s:%s", myname, dict_type, dict_name);
    /* XXX The choice between wait-for-lock or no-wait is hard-coded. */
    if (dict->flags & DICT_FLAG_OPEN_LOCK) {
	if (dict->flags & DICT_FLAG_LOCK)
	    msg_panic("%s: attempt to open %s:%s with both \"open\" lock and \"access\" lock",
		      myname, dict_type, dict_name);
	/* Multi-writer safe map: downgrade persistent lock to temporary. */
	if (dict->flags & DICT_FLAG_MULTI_WRITER) {
	    dict->flags &= ~DICT_FLAG_OPEN_LOCK;
	    dict->flags |= DICT_FLAG_LOCK;
	}
	/* Multi-writer unsafe map: acquire exclusive lock or bust. */
	else if (dict->lock(dict, MYFLOCK_OP_EXCLUSIVE | MYFLOCK_OP_NOWAIT) < 0)
	    msg_fatal("%s:%s: unable to get exclusive lock: %m",
		      dict_type, dict_name);
    }
    return (dict);
}

/* dict_open_register - register dictionary type */

void    dict_open_register(const char *type,
			           DICT *(*open) (const char *, int, int))
{
    const char *myname = "dict_open_register";
    DICT_OPEN_INFO *dp;

    if (dict_open_hash == 0)
	dict_open_init();
    if (htable_find(dict_open_hash, type))
	msg_panic("%s: dictionary type exists: %s", myname, type);
    dp = (DICT_OPEN_INFO *) mymalloc(sizeof(*dp));
    dp->type = mystrdup(type);
    dp->open = open;
    htable_enter(dict_open_hash, dp->type, (char *) dp);
}

/* dict_sort_alpha_cpp - qsort() callback */

static int dict_sort_alpha_cpp(const void *a, const void *b)
{
    return (strcmp(((char **) a)[0], ((char **) b)[0]));
}

/* dict_mapnames - return an ARGV of available map_names */

ARGV   *dict_mapnames()
{
    HTABLE_INFO **ht_info;
    HTABLE_INFO **ht;
    DICT_OPEN_INFO *dp;
    ARGV   *mapnames;

    if (dict_open_hash == 0)
	dict_open_init();
    mapnames = argv_alloc(dict_open_hash->used + 1);
    for (ht_info = ht = htable_list(dict_open_hash); *ht; ht++) {
	dp = (DICT_OPEN_INFO *) ht[0]->value;
	argv_add(mapnames, dp->type, ARGV_END);
    }
    qsort((void *) mapnames->argv, mapnames->argc, sizeof(mapnames->argv[0]),
	  dict_sort_alpha_cpp);
    myfree((char *) ht_info);
    argv_terminate(mapnames);
    return mapnames;
}

#ifdef TEST

 /*
  * Proof-of-concept test program.
  */
int     main(int argc, char **argv)
{
    dict_test(argc, argv);
    return (0);
}

#endif
