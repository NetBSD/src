#ifndef _ARGV_H_INCLUDED_
#define _ARGV_H_INCLUDED_

/*++
/* NAME
/*	argv 3h
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include "argv.h"
 DESCRIPTION
 .nf

 /*
  * External interface.
  */
typedef struct ARGV {
    int     len;			/* number of array elements */
    int     argc;			/* array elements in use */
    char  **argv;			/* string array */
} ARGV;

extern ARGV *argv_alloc(int);
extern void argv_add(ARGV *,...);
extern void argv_addn(ARGV *,...);
extern void argv_terminate(ARGV *);
extern ARGV *argv_free(ARGV *);

extern ARGV *argv_split(const char *, const char *);
extern ARGV *argv_split_append(ARGV *, const char *, const char *);

#define ARGV_END	((char *) 0)

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
