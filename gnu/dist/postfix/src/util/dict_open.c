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
/*	void	dict_put(dict, key, value)
/*	DICT	*dict;
/*	const char *key;
/*	const char *value;
/*
/*	char	*dict_get(dict, key)
/*	DICT	*dict;
/*	const char *key;
/*
/*	char	*dict_del(dict, key)
/*	DICT	*dict;
/*	const char *key;
/*
/*	void	dict_seq(dict, func, key, value)
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
/* .IP DICT_FLAG_SYNC_UPDATE
/*	With file-based maps, flush I/O buffers to file after each update.
/*	Thus feature is not supported with some file-based dictionaries.
/* .IP DICT_FLAG_NO_REGSUB
/*      Disallow regular expression substitution from left-hand side data 
/*	into the right-hand side.
/* .IP DICT_FLAG_NO_PROXY
/*	Disallow access through the \fBproxymap\fR service.
/* .IP DICT_FLAG_PARANOID
/*	A combination of all the paranoia flags: DICT_FLAG_NO_REGSUB
/*	and DICT_FLAG_NO_PROXY.
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
/* .PP
/*	dict_open3() takes separate arguments for dictionary type and
/*	name, but otherwise performs the same functions as dict_open().
/*
/*	dict_get() retrieves the value stored in the named dictionary
/*	under the given key. A null pointer means the value was not found.
/*
/*	dict_put() stores the specified key and value into the named
/*	dictionary.
/*
/*	dict_del() removes a dictionary entry, and returns non-zero
/*	in case of problems.
/*
/*	dict_seq() iterates over all members in the named dictionary.
/*	func is define DICT_SEQ_FUN_FIRST (select first member) or
/*	DICT_SEQ_FUN_NEXT (select next member). A null result means
/*	there is more.
/*
/*	dict_close() closes the specified dictionary and cleans up the
/*	associated data structures.
/*
/*	dict_open_register() adds support for a new dictionary type.
/* DIAGNOSTICS
/*	Fatal error: open error, unsupported dictionary type, attempt to
/*	update non-writable dictionary.
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

#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif

/* Utility library. */

#include <argv.h>
#include <mymalloc.h>
#include <msg.h>
#include <dict.h>
#include <dict_env.h>
#include <dict_unix.h>
#include <dict_tcp.h>
#include <dict_dbm.h>
#include <dict_db.h>
#include <dict_nis.h>
#include <dict_nisplus.h>
#include <dict_ni.h>
#include <dict_pcre.h>
#include <dict_regexp.h>
#include <dict_static.h>
#include <dict_cidr.h>
#include <stringops.h>
#include <split_at.h>
#include <htable.h>

 /*
  * lookup table for available map types.
  */
typedef struct {
    char   *type;
    struct DICT *(*open) (const char *, int, int);
} DICT_OPEN_INFO;

static DICT_OPEN_INFO dict_open_info[] = {
    DICT_TYPE_ENVIRON, dict_env_open,
    DICT_TYPE_UNIX, dict_unix_open,
#ifdef SNAPSHOT
    DICT_TYPE_TCP, dict_tcp_open,
#endif
#ifdef HAS_DBM
    DICT_TYPE_DBM, dict_dbm_open,
#endif
#ifdef HAS_DB
    DICT_TYPE_HASH, dict_hash_open,
    DICT_TYPE_BTREE, dict_btree_open,
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
    0,
};

static HTABLE *dict_open_hash;

/* dict_open_init - one-off initialization */

static void dict_open_init(void)
{
    char   *myname = "dict_open_init";
    DICT_OPEN_INFO *dp;

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
	msg_fatal("open dictionary: need \"type:name\" form instead of: \"%s\"",
		  dict_spec);

    dict = dict_open3(saved_dict_spec, dict_name, open_flags, dict_flags);
    myfree(saved_dict_spec);
    return (dict);
}


/* dict_open3 - open dictionary */

DICT   *dict_open3(const char *dict_type, const char *dict_name,
		           int open_flags, int dict_flags)
{
    char   *myname = "dict_open";
    DICT_OPEN_INFO *dp;
    DICT   *dict;

    if (dict_open_hash == 0)
	dict_open_init();
    if ((dp = (DICT_OPEN_INFO *) htable_find(dict_open_hash, dict_type)) == 0)
	msg_fatal("unsupported dictionary type: %s", dict_type);
    if ((dict = dp->open(dict_name, open_flags, dict_flags)) == 0)
	msg_fatal("opening %s:%s %m", dict_type, dict_name);
    if (msg_verbose)
	msg_info("%s: %s:%s", myname, dict_type, dict_name);
    return (dict);
}

/* dict_open_register - register dictionary type */

void    dict_open_register(const char *type,
			           DICT *(*open) (const char *, int, int))
{
    char   *myname = "dict_open_register";
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
    myfree((char *) ht_info);
    argv_terminate(mapnames);
    return mapnames;
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Create, update or read a database. When
  * the input is a name=value pair, the database is updated, otherwise the
  * program assumes that the input specifies a lookup key and prints the
  * corresponding value.
  */

/* System library. */

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

/* Utility library. */

#include "vstring.h"
#include "vstream.h"
#include "msg_vstream.h"
#include "vstring_vstream.h"

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s type:file read|write|create", myname);
}

int     main(int argc, char **argv)
{
    VSTRING *keybuf = vstring_alloc(1);
    VSTRING *inbuf = vstring_alloc(1);
    DICT   *dict;
    char   *dict_name;
    int     open_flags;
    char   *bufp;
    char   *cmd;
    char   *key;
    const char *value;
    int     ch;

    signal(SIGPIPE, SIG_IGN);

    msg_vstream_init(argv[0], VSTREAM_ERR);
    while ((ch = GETOPT(argc, argv, "v")) > 0) {
	switch (ch) {
	default:
	    usage(argv[0]);
	case 'v':
	    msg_verbose++;
	    break;
	}
    }
    optind = OPTIND;
    if (argc - optind != 2)
	usage(argv[0]);
    if (strcasecmp(argv[optind + 1], "create") == 0)
	open_flags = O_CREAT | O_RDWR | O_TRUNC;
    else if (strcasecmp(argv[optind + 1], "write") == 0)
	open_flags = O_RDWR;
    else if (strcasecmp(argv[optind + 1], "read") == 0)
	open_flags = O_RDONLY;
    else
	msg_fatal("unknown access mode: %s", argv[2]);
    dict_name = argv[optind];
    dict = dict_open(dict_name, open_flags, DICT_FLAG_LOCK);
    dict_register(dict_name, dict);
    while (vstring_fgets_nonl(inbuf, VSTREAM_IN)) {
	bufp = vstring_str(inbuf);
	if ((cmd = mystrtok(&bufp, " ")) == 0 || *bufp == 0) {
	    vstream_printf("usage: del key|get key|put key=value\n");
	    vstream_fflush(VSTREAM_OUT);
	    continue;
	}
	if (dict_changed_name())
	    msg_warn("dictionary has changed");
	key = vstring_str(unescape(keybuf, mystrtok(&bufp, " =")));
	value = mystrtok(&bufp, " =");
	if (strcmp(cmd, "del") == 0 && key && !value) {
	    if (dict_del(dict, key))
		vstream_printf("%s: not found\n", key);
	    else
		vstream_printf("%s: deleted\n", key);
	} else if (strcmp(cmd, "get") == 0 && key && !value) {
	    if ((value = dict_get(dict, key)) == 0) {
		vstream_printf("%s: %s\n", key,
			       dict_errno == DICT_ERR_RETRY ?
			       "soft error" : "not found");
	    } else {
		vstream_printf("%s=%s\n", key, value);
	    }
	} else if (strcmp(cmd, "put") == 0 && key && value) {
	    dict_put(dict, key, value);
	    vstream_printf("%s=%s\n", key, value);
	} else {
	    vstream_printf("usage: del key|get key|put key=value\n");
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(keybuf);
    vstring_free(inbuf);
    dict_close(dict);
    return (0);
}

#endif
