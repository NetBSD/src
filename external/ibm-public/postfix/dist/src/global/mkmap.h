/*	$NetBSD: mkmap.h,v 1.1.1.2.6.1 2014/08/10 07:12:48 tls Exp $	*/

#ifndef _MKMAP_H_INCLUDED_
#define _MKMAP_H_INCLUDED_

/*++
/* NAME
/*	mkmap 3h
/* SUMMARY
/*	create or rewrite Postfix database
/* SYNOPSIS
/*	#include <mkmap.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * A database handle is an opaque structure. The user is not supposed to
  * know its implementation. We try to open and lock a file before DB/DBM
  * initialization. However, if the file does not exist then we may have to
  * acquire the lock after the DB/DBM initialization.
  */
typedef struct MKMAP {
    struct DICT *(*open) (const char *, int, int);	/* dict_xx_open() */
    struct DICT *dict;			/* dict_xx_open() result */
    void    (*after_open) (struct MKMAP *);	/* may be null */
    void    (*after_close) (struct MKMAP *);	/* may be null */
    int     multi_writer;			/* multi-writer safe */
} MKMAP;

extern MKMAP *mkmap_open(const char *, const char *, int, int);
extern void mkmap_append(MKMAP *, const char *, const char *);
extern void mkmap_close(MKMAP *);

#define mkmap_append(map, key, val) dict_put((map)->dict, (key), (val))

extern MKMAP *mkmap_dbm_open(const char *);
extern MKMAP *mkmap_cdb_open(const char *);
extern MKMAP *mkmap_hash_open(const char *);
extern MKMAP *mkmap_btree_open(const char *);
extern MKMAP *mkmap_lmdb_open(const char *);
extern MKMAP *mkmap_sdbm_open(const char *);
extern MKMAP *mkmap_proxy_open(const char *);
extern MKMAP *mkmap_fail_open(const char *);

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

#endif
