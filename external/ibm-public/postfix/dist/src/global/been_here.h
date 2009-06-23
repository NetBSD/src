/*	$NetBSD: been_here.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _BEEN_HERE_H_INCLUDED_
#define _BEEN_HERE_H_INCLUDED_

/*++
/* NAME
/*	been_here 3h
/* SUMMARY
/*	detect repeated occurrence of string
/* SYNOPSIS
/*	#include <been_here.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * External interface.
  */
typedef struct {
    int     limit;			/* ceiling, zero for none */
    int     flags;			/* see below */
    struct HTABLE *table;
} BH_TABLE;

#define BH_FLAG_NONE	0		/* no special processing */
#define BH_FLAG_FOLD	(1<<0)		/* fold case */

extern BH_TABLE *been_here_init(int, int);
extern void been_here_free(BH_TABLE *);
extern int been_here_fixed(BH_TABLE *, const char *);
extern int PRINTFLIKE(2, 3) been_here(BH_TABLE *, const char *,...);
extern int been_here_check_fixed(BH_TABLE *, const char *);
extern int PRINTFLIKE(2, 3) been_here_check(BH_TABLE *, const char *,...);

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
