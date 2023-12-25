/*	$NetBSD: mkmap.h,v 1.2.2.2 2023/12/25 12:43:38 martin Exp $	*/

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
  * We try to open and lock a file before DB/DBM initialization. However, if
  * the file does not exist then we may have to acquire the lock after the
  * DB/DBM initialization.
  */
typedef struct MKMAP {
    struct DICT *(*open) (const char *, int, int);	/* dict_xx_open() */
    struct DICT *dict;			/* dict_xx_open() result */
    void    (*after_open) (struct MKMAP *);	/* may be null */
    void    (*after_close) (struct MKMAP *);	/* may be null */
    int     multi_writer;		/* multi-writer safe */
} MKMAP;

extern MKMAP *mkmap_open(const char *, const char *, int, int);
extern void mkmap_close(MKMAP *);

#define mkmap_append(map, key, val) dict_put((map)->dict, (key), (val))

typedef MKMAP *(*MKMAP_OPEN_FN) (const char *);

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
