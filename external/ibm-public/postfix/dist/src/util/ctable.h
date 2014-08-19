/*	$NetBSD: ctable.h,v 1.1.1.1.16.1 2014/08/19 23:59:45 tls Exp $	*/

#ifndef _CTABLE_H_INCLUDED_
#define _CTABLE_H_INCLUDED_

/*++
/* NAME
/*	ctable 5
/* SUMMARY
/*	cache manager
/* SYNOPSIS
/*	#include <ctable.h>
/* DESCRIPTION
/* .nf

 /*
  * Interface of the cache manager. The structure of a cache is not visible
  * to the caller.
  */

#define	CTABLE struct ctable
typedef void *(*CTABLE_CREATE_FN) (const char *, void *);
typedef void (*CTABLE_DELETE_FN) (void *, void *);

extern CTABLE *ctable_create(int, CTABLE_CREATE_FN, CTABLE_DELETE_FN, void *);
extern void ctable_free(CTABLE *);
extern void ctable_walk(CTABLE *, void (*) (const char *, const void *));
extern const void *ctable_locate(CTABLE *, const char *);
extern const void *ctable_refresh(CTABLE *, const char *);
extern void ctable_newcontext(CTABLE *, void *);

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
