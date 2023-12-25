/*	$NetBSD: dict_db.h,v 1.3.6.1 2023/12/25 12:43:36 martin Exp $	*/

#ifndef _DICT_DB_H_INCLUDED_
#define _DICT_DB_H_INCLUDED_

/*++
/* NAME
/*	dict_db 3h
/* SUMMARY
/*	dictionary manager interface to DB files
/* SYNOPSIS
/*	#include <dict_db.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>
#include <mkmap.h>

 /*
  * External interface.
  */
#define DICT_TYPE_HASH	"hash"
#define DICT_TYPE_BTREE	"btree"

extern DICT *dict_hash_open(const char *, int, int);
extern DICT *dict_btree_open(const char *, int, int);
extern MKMAP *mkmap_hash_open(const char *);
extern MKMAP *mkmap_btree_open(const char *);

 /*
  * XXX Should be part of the DICT interface.
  * 
  * You can override the default dict_db_cache_size setting before calling
  * dict_hash_open() or dict_btree_open(). This is done in mkmap_db_open() to
  * set a larger memory pool for database (re)builds.
  */
extern int dict_db_cache_size;

#define DEFINE_DICT_DB_CACHE_SIZE int dict_db_cache_size = (128 * 1024)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
