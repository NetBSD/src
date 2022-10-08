/*	$NetBSD: binhash.h,v 1.3 2022/10/08 16:12:50 christos Exp $	*/

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
    void   *key;			/* lookup key */
    ssize_t key_len;			/* key length */
    void   *value;			/* associated value */
    struct BINHASH_INFO *next;		/* colliding entry */
    struct BINHASH_INFO *prev;		/* colliding entry */
} BINHASH_INFO;

 /* Structure of one hash table. */

typedef struct BINHASH {
    ssize_t size;			/* length of entries array */
    ssize_t used;			/* number of entries in table */
    BINHASH_INFO **data;		/* entries array, auto-resized */
    BINHASH_INFO **seq_bucket;		/* current sequence hash bucket */
    BINHASH_INFO **seq_element;		/* current sequence element */
} BINHASH;

extern BINHASH *binhash_create(ssize_t);
extern BINHASH_INFO *binhash_enter(BINHASH *, const void *, ssize_t, void *);
extern BINHASH_INFO *binhash_locate(BINHASH *, const void *, ssize_t);
extern void *binhash_find(BINHASH *, const void *, ssize_t);
extern void binhash_delete(BINHASH *, const void *, ssize_t, void (*) (void *));
extern void binhash_free(BINHASH *, void (*) (void *));
extern void binhash_walk(BINHASH *, void (*) (BINHASH_INFO *, void *), void *);
extern BINHASH_INFO **binhash_list(BINHASH *);
extern BINHASH_INFO *binhash_sequence(BINHASH *, int);

#define BINHASH_SEQ_FIRST	0
#define BINHASH_SEQ_NEXT	1
#define BINHASH_SEQ_STOP	(-1)

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
