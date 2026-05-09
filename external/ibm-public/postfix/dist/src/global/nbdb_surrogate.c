/*	$NetBSD: nbdb_surrogate.c,v 1.1.1.1 2026/05/09 18:39:18 christos Exp $	*/

/*++
/* NAME
/*	nbdb_surrogate 3
/* SUMMARY
/*	surrogate Berkeley DB handlers that log hints
/* SYNOPSIS
/*	#include <nbdb_surrogate.h>
/*
/*	void	nbdb_surr_init()
/* DESCRIPTION
/*	nbdb_surr_init() installs dict_open() and mkmap_open() surrogates
/*	for Berkeley DB types 'hash' and 'btree. These handlers log that
/*	Berkeley DB support is unavailable and provide a link to the
/*	NON_BERKELEYDB_README web page. The dict_open() handlers return
/*	a pointer to DICT instance that fails all requests, and the
/*	mkmap_open() handlers terminate the process.
/*
/*	This code does not require prior initialization with
/*	mail_conf_read() or mail_params_init().
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

 /*
  * Utility library.
  */
#include <dict.h>
#include <dict_db.h>
#include <mkmap.h>
#include <msg.h>

 /*
  * Global library.
  */
#include <nbdb_surrogate.h>

 /*
  * Inform the user that Berkeley DB is gone, and offer alternatives.
  */
static const char boilerplate[] =
"Berkeley DB support for '%s:%s' is not available for this OS distribution; "
"see https://www.postfix.org/NON_BERKELEYDB_README.html for alternatives";

/* surrogate dict_{hash,btree}_open() */

static DICT *surrogate_dict_bdb_open(const char *type, const char *name,
				             int open_flags, int dict_flags)
{
    return (dict_surrogate(type, name, open_flags, dict_flags, boilerplate,
			   type, name));
}

static DICT *surrogate_dict_hash_open(const char *name, int open_flags,
				              int dict_flags)
{
    return (surrogate_dict_bdb_open(DICT_TYPE_HASH, name, open_flags,
				    dict_flags));
}

static DICT *surrogate_dict_btree_open(const char *name, int open_flags,
				               int dict_flags)
{
    return (surrogate_dict_bdb_open(DICT_TYPE_BTREE, name, open_flags,
				    dict_flags));
}

/* surrogates for mkmap_{hash,btree}_open() */

static MKMAP *surrogate_mkmap_bdb_open(const char *type, const char *path)
{
    msg_fatal(boilerplate, type, path);
}

static MKMAP *surrogate_mkmap_hash_open(const char *path)
{
    return (surrogate_mkmap_bdb_open(DICT_TYPE_HASH, path));
}

static MKMAP *surrogate_mkmap_btree_open(const char *path)
{
    return (surrogate_mkmap_bdb_open(DICT_TYPE_BTREE, path));
}

static const DICT_OPEN_INFO surrogate_bdb_info[] = {
    {DICT_TYPE_HASH, surrogate_dict_hash_open, surrogate_mkmap_hash_open},
    {DICT_TYPE_BTREE, surrogate_dict_btree_open, surrogate_mkmap_btree_open},
    0,
};

/* nbdb_surr_init - register the above surrogates */

void    nbdb_surr_init(void)
{
    const DICT_OPEN_INFO *open_info;

    for (open_info = surrogate_bdb_info; open_info->type; open_info++) {
	if (dict_open_lookup(open_info->type) != 0)
	    dict_open_unregister(open_info->type);
	if (msg_verbose)
	    msg_info("%s: installing custom hanndlers for '%s'",
		     __func__, open_info->type);
	dict_open_register(open_info);
    }
}
