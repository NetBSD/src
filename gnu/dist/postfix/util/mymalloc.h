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
extern char *mymalloc(int);
extern char *myrealloc(char *, int);
extern void myfree(char *);
extern char *mystrdup(const char *);
extern char *mystrndup(const char *, int len);
extern char *mymemdup(const char *, int);

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
