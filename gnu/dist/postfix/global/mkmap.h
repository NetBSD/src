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
  * know its implementation.
  */
typedef struct MKMAP {
    struct DICT *(*open) (const char *, int, int);
    struct DICT *dict;
    char   *lock_file;
    int     lock_fd;
} MKMAP;

extern MKMAP *mkmap_open(const char *, const char *, int, int);
extern void mkmap_append(MKMAP *, const char *, const char *);
extern void mkmap_close(MKMAP *);

extern MKMAP *mkmap_dbm_open(const char *);
extern MKMAP *mkmap_hash_open(const char *);
extern MKMAP *mkmap_btree_open(const char *);

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
