/*	$NetBSD: mvect.h,v 1.1.1.2 2004/05/31 00:25:00 heas Exp $	*/

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
typedef void (*MVECT_FN) (char *, int);

typedef struct {
    char   *ptr;
    int     elsize;
    int     nelm;
    MVECT_FN init_fn;
    MVECT_FN wipe_fn;
} MVECT;

extern char *mvect_alloc(MVECT *, int, int, MVECT_FN, MVECT_FN);
extern char *mvect_realloc(MVECT *, int);
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
