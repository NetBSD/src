/*	$NetBSD: mock_dict.c,v 1.2.2.2 2026/05/11 17:13:59 martin Exp $	*/

/*++
/* NAME
/*      mock_dict 3
/* SUMMARY
/*      mock tables
/* SYNOPSIS
/*      $include <mock_dict.h>
/*
/*	setup_mock_cdb(const char *name_payload)
/*
/*	setup_mock_lmdb(const char *name_payload)
/* DESCRIPTION
/*      setup_mock_cdb (or setup_mock_lmdb) instantiates a mock cdb
/*      (or lmdb) table with a name_payload that uses inline: syntax.
/* LICENSE
/* .ad
/* .fi
/*      The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*      Wietse Venema
/*      porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>

 /*
  * Utility library.
  */
#include <dict.h>
#include <dict_cdb.h>
#include <dict_inline.h>
#include <dict_lmdb.h>
#include <mkmap.h>
#include <mymalloc.h>

 /*
  * Global library.
  */

 /*
  * Testing library.
  */
#include <mock_dict.h>

static char *mock_cdb_name_payload;

static DICT *mock_dict_cdb_open(const char *dict_name, int open_flags,
				        int dict_flags)
{
    return (dict_open3(DICT_TYPE_INLINE, mock_cdb_name_payload,
		       open_flags, dict_flags));
}

static MKMAP *mock_mkmap_cdb_open(const char *path)
{
    MKMAP  *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));

    mkmap->open = mock_dict_cdb_open;
    mkmap->after_open = 0;
    mkmap->after_close = 0;
    return (mkmap);
}

static const DICT_OPEN_INFO mock_cdb_info = {
    "cdb", mock_dict_cdb_open, mock_mkmap_cdb_open,
};

void    setup_mock_cdb(const char *name_payload)
{
    if (mock_cdb_name_payload != 0)
	myfree(mock_cdb_name_payload);

    mock_cdb_name_payload = mystrdup(name_payload);

    if (dict_open_lookup(mock_cdb_info.type))
	dict_open_unregister(mock_cdb_info.type);
    dict_open_register(&mock_cdb_info);
}

static char *mock_lmdb_name_payload;

static DICT *mock_dict_lmdb_open(const char *dict_name, int open_flags,
				         int dict_flags)
{
    return (dict_open3(DICT_TYPE_INLINE, mock_lmdb_name_payload,
		       open_flags, dict_flags));
}

static MKMAP *mock_mkmap_lmdb_open(const char *path)
{
    MKMAP  *mkmap = (MKMAP *) mymalloc(sizeof(*mkmap));

    mkmap->open = mock_dict_lmdb_open;
    mkmap->after_open = 0;
    mkmap->after_close = 0;
    return (mkmap);
}

static const DICT_OPEN_INFO mock_lmdb_info = {
    "lmdb", mock_dict_lmdb_open, mock_mkmap_lmdb_open,
};

void    setup_mock_lmdb(const char *name_payload)
{
    if (mock_lmdb_name_payload != 0)
	myfree(mock_lmdb_name_payload);

    mock_lmdb_name_payload = mystrdup(name_payload);

    if (dict_open_lookup(mock_lmdb_info.type))
	dict_open_unregister(mock_lmdb_info.type);
    dict_open_register(&mock_lmdb_info);
}
