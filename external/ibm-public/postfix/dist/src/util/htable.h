/*	$NetBSD: htable.h,v 1.1.1.3.12.1 2017/04/21 16:52:53 bouyer Exp $	*/

#ifndef _HTABLE_H_INCLUDED_
#define _HTABLE_H_INCLUDED_

/*++
/* NAME
/*	htable 3h
/* SUMMARY
/*	hash table manager
/* SYNOPSIS
/*	#include <htable.h>
/* DESCRIPTION
/* .nf

 /* Structure of one hash table entry. */

typedef struct HTABLE_INFO {
    char   *key;			/* lookup key */
    void   *value;			/* associated value */
    struct HTABLE_INFO *next;		/* colliding entry */
    struct HTABLE_INFO *prev;		/* colliding entry */
} HTABLE_INFO;

 /* Structure of one hash table. */

typedef struct HTABLE {
    ssize_t size;			/* length of entries array */
    ssize_t used;			/* number of entries in table */
    HTABLE_INFO **data;			/* entries array, auto-resized */
    HTABLE_INFO **seq_bucket;		/* current sequence hash bucket */
    HTABLE_INFO **seq_element;		/* current sequence element */
} HTABLE;

extern HTABLE *htable_create(ssize_t);
extern HTABLE_INFO *htable_enter(HTABLE *, const char *, void *);
extern HTABLE_INFO *htable_locate(HTABLE *, const char *);
extern void *htable_find(HTABLE *, const char *);
extern void htable_delete(HTABLE *, const char *, void (*) (void *));
extern void htable_free(HTABLE *, void (*) (void *));
extern void htable_walk(HTABLE *, void (*) (HTABLE_INFO *, void *), void *);
extern HTABLE_INFO **htable_list(HTABLE *);
extern HTABLE_INFO *htable_sequence(HTABLE *, int);

#define HTABLE_SEQ_FIRST	0
#define HTABLE_SEQ_NEXT		1
#define HTABLE_SEQ_STOP		(-1)

 /*
  * Correct only when casting (char *) to (void *).
  */
#define HTABLE_ACTION_FN_CAST(f) ((void *)(HTABLE_INFO *, void *)) (f)
#define HTABLE_FREE_FN_CAST(f) ((void *)(void *)) (f)

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
/*	Fri Feb 14 13:43:19 EST 1997
/* LAST MODIFICATION
/*	%E% %U%
/* VERSION/RELEASE
/*	%I%
/*--*/

#endif
