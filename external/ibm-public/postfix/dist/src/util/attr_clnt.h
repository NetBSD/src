/*	$NetBSD: attr_clnt.h,v 1.1.1.1.26.1 2014/08/10 07:12:50 tls Exp $	*/

#ifndef _ATTR_CLNT_H_INCLUDED_
#define _ATTR_CLNT_H_INCLUDED_

/*++
/* NAME
/*	attr_clnt 3h
/* SUMMARY
/*	attribute query-reply client
/* SYNOPSIS
/*	#include <attr_clnt.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <attr.h>

 /*
  * External interface.
  */
typedef struct ATTR_CLNT ATTR_CLNT;
typedef int (*ATTR_CLNT_PRINT_FN) (VSTREAM *, int, va_list);
typedef int (*ATTR_CLNT_SCAN_FN) (VSTREAM *, int, va_list);

extern ATTR_CLNT *attr_clnt_create(const char *, int, int, int);
extern int attr_clnt_request(ATTR_CLNT *, int,...);
extern void attr_clnt_free(ATTR_CLNT *);
extern void attr_clnt_control(ATTR_CLNT *, int, ...);

#define ATTR_CLNT_CTL_END	0
#define ATTR_CLNT_CTL_PROTO	1

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
