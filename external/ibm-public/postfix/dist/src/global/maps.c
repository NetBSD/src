/*	$NetBSD: maps.c,v 1.1.1.2.2.1 2013/02/25 00:27:18 tls Exp $	*/

/*++
/* NAME
/*	maps 3
/* SUMMARY
/*	multi-dictionary search
/* SYNOPSIS
/*	#include <maps.h>
/*
/*	MAPS	*maps_create(title, map_names, flags)
/*	const char *title;
/*	const char *map_names;
/*	int	flags;
/*
/*	const char *maps_find(maps, key, flags)
/*	MAPS	*maps;
/*	const char *key;
/*	int	flags;
/*
/*	MAPS	*maps_free(maps)
/*	MAPS	*maps;
/* DESCRIPTION
/*	This module implements multi-dictionary searches. it goes
/*	through the high-level dictionary interface and does file
/*	locking. Dictionaries are opened read-only, and in-memory
/*	dictionary instances are shared.
/*
/*	maps_create() takes list of type:name pairs and opens the
/*	named dictionaries.
/*	The result is a handle that must be specified along with all
/*	other maps_xxx() operations.
/*	See dict_open(3) for a description of flags.
/*	This includes the flags that specify preferences for search
/*	string case folding.
/*
/*	maps_find() searches the specified list of dictionaries
/*	in the specified order for the named key. The result is in
/*	memory that is overwritten upon each call.
/*	The flags argument is either 0 or specifies a filter:
/*	for example, DICT_FLAG_FIXED | DICT_FLAG_PATTERN selects
/*	dictionaries that have fixed keys or pattern keys.
/*
/*	maps_free() releases storage claimed by maps_create()
/*	and conveniently returns a null pointer.
/*
/*	Arguments:
/* .IP title
/*	String used for diagnostics. Typically one specifies the
/*	type of information stored in the lookup tables.
/* .IP map_names
/*	Null-terminated string with type:name dictionary specifications,
/*	separated by whitespace or commas.
/* .IP flags
/*	With maps_create(), flags that are passed to dict_open().
/*	With maps_find(), flags that control searching behavior
/*	as documented above.
/* .IP maps
/*	A result from maps_create().
/* .IP key
/*	Null-terminated string with a lookup key. Table lookup is case
/*	sensitive.
/* DIAGNOSTICS
/*	Panic: inappropriate use; fatal errors: out of memory, unable
/*	to open database. Warnings: null string lookup result.
/*
/*	maps_find() returns a null pointer when the requested
/*	information was not found, and logs a warning when the
/*	lookup failed due to error. The maps->error value indicates
/*	if the last lookup failed due to error.
/* BUGS
/*	The dictionary name space is flat, so dictionary names allocated
/*	by maps_create() may collide with dictionary names allocated by
/*	other methods.
/*
/*	This functionality could be implemented by allowing the user to
/*	specify dictionary search paths to dict_lookup() or dict_eval().
/*	However, that would either require that the dict(3) module adopts
/*	someone else's list notation syntax, or that the dict(3) module
/*	imposes syntax restrictions onto other software, neither of which
/*	is desirable.
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

/* Utility library. */

#include <argv.h>
#include <mymalloc.h>
#include <msg.h>
#include <dict.h>
#include <stringops.h>
#include <split_at.h>

/* Global library. */

#include "mail_conf.h"
#include "maps.h"

/* maps_create - initialize */

MAPS   *maps_create(const char *title, const char *map_names, int dict_flags)
{
    const char *myname = "maps_create";
    char   *temp;
    char   *bufp;
    static char sep[] = " \t,\r\n";
    MAPS   *maps;
    char   *map_type_name;
    VSTRING *map_type_name_flags;
    DICT   *dict;

    /*
     * Initialize.
     */
    maps = (MAPS *) mymalloc(sizeof(*maps));
    maps->title = mystrdup(title);
    maps->argv = argv_alloc(2);
    maps->error = 0;

    /*
     * For each specified type:name pair, either register a new dictionary,
     * or increment the reference count of an existing one.
     */
    if (*map_names) {
	bufp = temp = mystrdup(map_names);
	map_type_name_flags = vstring_alloc(10);

#define OPEN_FLAGS	O_RDONLY

	while ((map_type_name = mystrtok(&bufp, sep)) != 0) {
	    vstring_sprintf(map_type_name_flags, "%s(%o,%s)",
			    map_type_name, OPEN_FLAGS,
			    dict_flags_str(dict_flags));
	    if ((dict = dict_handle(vstring_str(map_type_name_flags))) == 0)
		dict = dict_open(map_type_name, OPEN_FLAGS, dict_flags);
	    if ((dict->flags & dict_flags) != dict_flags)
		msg_panic("%s: map %s has flags 0%o, want flags 0%o",
			  myname, map_type_name, dict->flags, dict_flags);
	    dict_register(vstring_str(map_type_name_flags), dict);
	    argv_add(maps->argv, vstring_str(map_type_name_flags), ARGV_END);
	}
	myfree(temp);
	vstring_free(map_type_name_flags);
    }
    return (maps);
}

/* maps_find - search a list of dictionaries */

const char *maps_find(MAPS *maps, const char *name, int flags)
{
    const char *myname = "maps_find";
    char  **map_name;
    const char *expansion;
    DICT   *dict;

    /*
     * In case of return without map lookup (empty name or no maps).
     */
    maps->error = 0;

    /*
     * Temp. workaround, for buggy callers that pass zero-length keys when
     * given partial addresses.
     */
    if (*name == 0)
	return (0);

    for (map_name = maps->argv->argv; *map_name; map_name++) {
	if ((dict = dict_handle(*map_name)) == 0)
	    msg_panic("%s: dictionary not found: %s", myname, *map_name);
	if (flags != 0 && (dict->flags & flags) == 0)
	    continue;
	if ((expansion = dict_get(dict, name)) != 0) {
	    if (*expansion == 0) {
		msg_warn("%s lookup of %s returns an empty string result",
			 maps->title, name);
		msg_warn("%s should return NO RESULT in case of NOT FOUND",
			 maps->title);
		maps->error = DICT_ERR_RETRY;
		return (0);
	    }
	    if (msg_verbose)
		msg_info("%s: %s: %s: %s = %s", myname, maps->title,
			 *map_name, name, expansion);
	    return (expansion);
	} else if ((maps->error = dict->error) != 0) {
	    msg_warn("%s:%s lookup error for \"%.100s\"",
		     dict->type, dict->name, name);
	    break;
	}
    }
    if (msg_verbose)
	msg_info("%s: %s: %s: %s", myname, maps->title, name, maps->error ?
		 "search aborted" : "not found");
    return (0);
}

/* maps_free - release storage */

MAPS   *maps_free(MAPS *maps)
{
    char  **map_name;

    for (map_name = maps->argv->argv; *map_name; map_name++) {
	if (msg_verbose)
	    msg_info("maps_free: %s", *map_name);
	dict_unregister(*map_name);
    }
    myfree(maps->title);
    argv_free(maps->argv);
    myfree((char *) maps);
    return (0);
}

#ifdef TEST

#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>

int     main(int argc, char **argv)
{
    VSTRING *buf = vstring_alloc(100);
    MAPS   *maps;
    const char *result;

    if (argc != 2)
	msg_fatal("usage: %s maps", argv[0]);
    msg_verbose = 2;
    maps = maps_create("whatever", argv[1], DICT_FLAG_LOCK);

    while (vstring_fgets_nonl(buf, VSTREAM_IN)) {
	maps->error = 99;
	vstream_printf("\"%s\": ", vstring_str(buf));
	if ((result = maps_find(maps, vstring_str(buf), 0)) != 0) {
	    vstream_printf("%s\n", result);
	} else if (maps->error != 0) {
	    vstream_printf("lookup error\n");
	} else {
	    vstream_printf("not found\n");
	}
	vstream_fflush(VSTREAM_OUT);
    }
    maps_free(maps);
    vstring_free(buf);
    return (0);
}

#endif
