/*	$NetBSD: mymalloc.h,v 1.2 2017/02/14 01:16:49 christos Exp $	*/

#ifndef _MALLOC_H_INCLUDED_
#define _MALLOC_H_INCLUDED_

/*++
/* NAME
/*	mymalloc 3h
/* SUMMARY
/*	memory management wrappers
/* SYNOPSIS
/*	#include "mymalloc.h"
 DESCRIPTION
 .nf

 /*
  * External interface.
  */
extern void *mymalloc(ssize_t);
extern void *myrealloc(void *, ssize_t);
extern void myfree(void *);
extern char *mystrdup(const char *);
extern char *mystrndup(const char *, ssize_t);
extern char *mymemdup(const void *, ssize_t);

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
