/*	$NetBSD: mvect.h,v 1.1.1.1.36.1 2017/04/21 16:52:53 bouyer Exp $	*/

#ifndef _MVECT_H_INCLUDED_
#define _MVECT_H_INCLUDED_

/*++
/* NAME
/*	mvect 3h
/* SUMMARY
/*	memory vector management
/* SYNOPSIS
/*	#include <mvect.h>
/* DESCRIPTION
/* .nf

 /*
  * Generic memory vector interface.
  */
typedef void (*MVECT_FN) (char *, ssize_t);

typedef struct {
    char   *ptr;
    ssize_t elsize;
    ssize_t nelm;
    MVECT_FN init_fn;
    MVECT_FN wipe_fn;
} MVECT;

extern char *mvect_alloc(MVECT *, ssize_t, ssize_t, MVECT_FN, MVECT_FN);
extern char *mvect_realloc(MVECT *, ssize_t);
extern char *mvect_free(MVECT *);

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
