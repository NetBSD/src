/*	$NetBSD: binhash.h,v 1.1.1.1.2.2 2009/09/15 06:03:53 snj Exp $	*/

#ifndef _BINHASH_H_INCLUDED_
#define _BINHASH_H_INCLUDED_

/*++
/* NAME
/*	binhash 3h
/* SUMMARY
/*	hash table manager
/* SYNOPSIS
/*	#include <binhash.h>
/* DESCRIPTION
/* .nf

 /* Structure of one hash table entry. */

typedef struct BINHASH_INFO {
    char   *key;			/* lookup key */
    int     key_len;			/* key length */
    char   *value;			/* associated value */
    struct BINHASH_INFO *next;		/* colliding entry */
    struct BINHASH_INFO *prev;		/* colliding entry */
} BINHASH_INFO;

 /* Structure of one hash table. */

typedef struct BINHASH {
    int     size;			/* length of entries array */
    int     used;			/* number of entries in table */
    BINHASH_INFO **data;		/* entries array, auto-resized */
} BINHASH;

extern BINHASH *binhash_create(int);
extern BINHASH_INFO *binhash_enter(BINHASH *, const char *, int, char *);
extern BINHASH_INFO *binhash_locate(BINHASH *, const char *, int);
extern char *binhash_find(BINHASH *, const char *, int);
extern void binhash_delete(BINHASH *, const char *, int, void (*) (char *));
extern void binhash_free(BINHASH *, void (*) (char *));
extern void binhash_walk(BINHASH *, void (*) (BINHASH_INFO *, char *), char *);
extern BINHASH_INFO **binhash_list(BINHASH *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/* CREATION DATE
/*	Thu Feb 20 16:54:29 EST 1997
/* LAST MODIFICATION
/*	%E% %U%
/* VERSION/RELEASE
/*	%I%
/*--*/

#endif
