/*	$NetBSD: nbdb_util.c,v 1.1.1.1 2026/05/09 18:39:18 christos Exp $	*/

/*++
/* NAME
/*	nbdb_util 3
/* SUMMARY
/*	Non-Berkeley-DB migration support
/* SYNOPSIS
/*	#include <nbdb_util.h>
/*
/*	int	nbdb_level;
/*	bool	nbdb_reindexing_lockout;
/*
/*	void	nbdb_util_init(const char *level_name)
/*
/*	int	nbdb_is_legacy_type(const char *type)
/*
/*	const char *nbdb_find_non_leg_suffix(const char *non_leg_type)
/*
/*	const char *nbdb_map_leg_type(
/*	const char *leg_type,
/*	const char *name,
/*	const DICT_OPEN_INFO **open_info,
/*	VSTRING *why)
/* DESCRIPTION
/*	This module contains utilities that may be called by clients
/*	and servers of the non-Berkeley-DB migration service.
/*
/*	nbdb_is_legacy_type() returns true if the specified database
/*	type is a Berkeley DB type ('hash' or 'btree').
/*
/*	nbdb_find_non_leg_suffix() returns the database pathname suffix
/*	for the specified non-legacy database, or null if no suffix
/*	was found.
/*
/*	nbdb_map_leg_type() implements the mapping from expected legacy
/*	type names to expected non-legacy type names. This supports
/*	a user-defined mapping override, indexed by legacy type or legacy
/*	type:name. nbdb_map_leg_type() returns null in case of error:
/*	a table lookup error, an unexpected input type, or an unexpected
/*	output type.
/* .IP leg_type
/*	A legacy database type to be mapped to a non-legacy type.
/* .IP name
/*	A database name.
/* .IP open_info
/*	Null pointer, or pointer to DICT_OPEN_INFO pointer with function
/*	pointers to open the non-legacy database.
/* .IP why
/*	Storage for descriptive text in case of error.
/* .PP
/*
/*	nbdb_util_init() converts the non-Berkeley-DB migration level
/*	from text to internal form, and updates the nbdb_level global
/*	variable:
/* .IP "redirect (internal form: NBDB_LEV_CODE_REDIRECT)"
/*	Enables the mapping from legacy Berkeley DB types to preferred
/*	types.
/* .IP "reindex (internal form: NBDB_LEV_CODE_REINDEX)"
/*	As "redirect", but also makes requests to a reindexing service
/*	if the non-legacy indexed file does not exist.
/*	Exception: calling a reindexing service will remain disabled
/*	if the process sets nbdb_reindexing_lockout to 'true'.
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
#include <string.h>
#include <errno.h>

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
#include <split_at.h>
#include <stringops.h>
#include <vstring.h>
#include <stringops.h>

 /*
  * Global library.
  */
#include <mail_params.h>
#include <nbdb_util.h>
#include <nbdb_redirect.h>
#include <nbdb_surrogate.h>

 /*
  * Application-specific.
  */
int     nbdb_level;
bool    nbdb_reindexing_lockout;

 /*
  * Mapping from legacy type to non-legacy.
  */
static DICT *nbdb_cust_map;

 /*
  * Mapping from non-legacy type to pathname suffix. This should be provided
  * by individual table implementations, but that would require changes to
  * the dict/mkmap plugin API.
  */
typedef struct {
    const char *type;
    const char *suffix;
} NBDB_PATH_SUFFIX;

static const NBDB_PATH_SUFFIX nbdb_path_suffixes[] = {
    DICT_TYPE_CDB, ".cdb",
    DICT_TYPE_LMDB, ".lmdb",
    0, 0,				/* Default case */
};

 /*
  * SLMs.
  */
#define STR(x)	vstring_str(x)

/* nbdb_map_leg_type - map legacy type to non-legacy type, null means error */

const char *nbdb_map_leg_type(const char *leg_type, const char *name,
			              const DICT_OPEN_INFO **open_info,
			              VSTRING *why)
{
    static VSTRING *type_name = 0;
    const char *non_leg_type = 0;
    const DICT_OPEN_INFO *my_open_info;

    if (type_name == 0)
	type_name = vstring_alloc(100);

    /*
     * Sanity check.
     */
    if (!nbdb_is_legacy_type(leg_type)) {
	vstring_sprintf(why, "request to map from non-Berkeley-DB type: '%s'",
			leg_type);
	return (0);
    }
    /* Search for custom mapping with the type:name. */
    if (nbdb_cust_map != 0) {
	vstring_sprintf(type_name, "%s:%s", leg_type, name);
	if ((non_leg_type = dict_get(nbdb_cust_map, STR(type_name))) == 0
	    && nbdb_cust_map->error == 0)
	    /* Search with the 'bare' type only. */
	    non_leg_type = dict_get(nbdb_cust_map, leg_type);
	if (non_leg_type == 0 && nbdb_cust_map->error != 0) {
	    vstring_sprintf(why, "%s:%s lookup error for '%s'",
			    nbdb_cust_map->type, nbdb_cust_map->name,
			    STR(type_name));
	    return (0);
	}
    }

    /*
     * No custom mapping: use the default.
     */
    if (non_leg_type == 0) {
	if (strcmp(leg_type, DICT_TYPE_HASH) == 0) {
	    non_leg_type = (var_db_type);
	} else if (strcmp(leg_type, DICT_TYPE_BTREE) == 0) {
	    non_leg_type = (var_cache_db_type);
	} else {
	    msg_panic("%s: no default mapping for Berkeley DB type: '%s'",
		      __func__, leg_type);
	}
    }

    /*
     * More sanity checks.
     */
    if (!allalnum(non_leg_type)) {
	vstring_sprintf(why, "bad non-legacy database type syntax: '%s'",
			non_leg_type);
	return (0);
    }
    if (nbdb_is_legacy_type(non_leg_type)) {
	vstring_sprintf(why, "mapping from '%s' to legacy type '%s'",
			leg_type, non_leg_type);
	return (0);
    }
    if ((my_open_info = dict_open_lookup(non_leg_type)) == 0) {
	vstring_sprintf(why, "mapping to unsupported database type: '%s'",
			non_leg_type);
	return (0);
    }
    if (open_info != 0)
	*open_info = my_open_info;
    return (non_leg_type);
}

/* nbdb_is_legacy_type - sanity check */

int     nbdb_is_legacy_type(const char *leg_type)
{
    return (strcmp(leg_type, DICT_TYPE_HASH) == 0
	    || strcmp(leg_type, DICT_TYPE_BTREE) == 0);
}

/* nbdb_find_non_leg_suffix - find file suffix for non-legacy type */

const char *nbdb_find_non_leg_suffix(const char *non_leg_type)
{
    const NBDB_PATH_SUFFIX *sp;

    for (sp = nbdb_path_suffixes; sp->type != 0; sp++)
	if (strcmp(non_leg_type, sp->type) == 0)
	    break;
    return (sp->suffix);
}

/* nbdb_util_init - redirect legacy open() requests to nbdb handlers */

void    nbdb_util_init(const char *level_name)
{
    static NAME_CODE nbdb_level_table[] = {
	NBDB_LEV_NAME_NONE, NBDB_LEV_CODE_NONE,
	NBDB_LEV_NAME_REDIRECT, NBDB_LEV_CODE_REDIRECT,
	NBDB_LEV_NAME_REINDEX, NBDB_LEV_CODE_REINDEX,
	0, -1,
    };

    /*
     * Convert the non-Berkeley-DB migration level to internal form. If
     * migration support is disabled, then do not tun the code below.
     */
    nbdb_level = name_code(nbdb_level_table, NAME_CODE_FLAG_NONE, level_name);
    if (nbdb_level == NBDB_LEV_CODE_ERROR)
	msg_fatal("invalid non-Berkeley-DB migration level: '%s'", level_name);
    if (nbdb_level == NBDB_LEV_CODE_NONE) {
#ifdef NO_DB
	nbdb_surr_init();
#endif
	return;
    }

    /*
     * Initialize the custom legacy to non-legacy mapping. Discard an
     * existing mapping, to simplify testing. To avoid a dependency loop,
     * this mapping should never use a legacy database type.
     */
    if (nbdb_cust_map) {
	dict_close(nbdb_cust_map);
	nbdb_cust_map = 0;
    }
    if (*var_nbdb_cust_map) {
	char   *map_type = mystrdup(var_nbdb_cust_map);
	char   *map_name = split_at(map_type, ':');

	if (map_name != 0 && (strcmp(map_type, DICT_TYPE_HASH) == 0
			      || strcmp(map_type, DICT_TYPE_BTREE) == 0)) {
	    nbdb_cust_map = dict_surrogate(map_type, map_name,
					   O_RDONLY, DICT_FLAG_LOCK,
					"%s must not use legacy type: '%s'",
					   VAR_NBDB_CUST_MAP,
					   map_type);
	} else {
	    nbdb_cust_map = dict_open(var_nbdb_cust_map, O_RDONLY,
				      DICT_FLAG_LOCK
				      | DICT_FLAG_FOLD_FIX
				      | DICT_FLAG_UTF8_REQUEST);
	}
	myfree(map_type);
    }

    /*
     * If this process does not provide the reindexing service, turn on the
     * forward-compatibility redirection for legacy database types and
     * reindexing server callouts.
     * 
     * The reindexing server should never attempt to use legacy database types,
     * and should never be calling reindexing service client code.
     */
    if (!nbdb_reindexing_lockout)
	nbdb_rdr_init();
}
