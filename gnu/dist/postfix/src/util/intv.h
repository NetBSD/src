#ifndef _INTV_H_INCLUDED_
#define _INTV_H_INCLUDED_

/*++
/* NAME
/*	intv 3h
/* SUMMARY
/*	string array utilities
/* SYNOPSIS
/*	#include "intv.h"
 DESCRIPTION
 .nf

 /*
  * External interface.
  */
typedef struct INTV {
    int     len;			/* number of array elements */
    int     intc;			/* array elements in use */
    int    *intv;			/* integer array */
} INTV;

extern INTV *intv_alloc(int);
extern void intv_add(INTV *, int,...);
extern INTV *intv_free(INTV *);

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
