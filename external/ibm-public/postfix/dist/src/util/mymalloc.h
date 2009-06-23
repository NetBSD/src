/*	$NetBSD: mymalloc.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

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
extern char *mymalloc(ssize_t);
extern char *myrealloc(char *, ssize_t);
extern void myfree(char *);
extern char *mystrdup(const char *);
extern char *mystrndup(const char *, ssize_t);
extern char *mymemdup(const char *, ssize_t);

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
