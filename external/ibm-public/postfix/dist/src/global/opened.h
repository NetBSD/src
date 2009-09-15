/*	$NetBSD: opened.h,v 1.1.1.1.2.2 2009/09/15 06:02:51 snj Exp $	*/

#ifndef _OPENED_H_INCLUDED_
#define _OPENED_H_INCLUDED_

/*++
/* NAME
/*	opened 3h
/* SUMMARY
/*	log that a message was opened
/* SYNOPSIS
/*	#include <opened.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * External interface.
  */
extern void PRINTFLIKE(5, 6) opened(const char *, const char *, long, int,
				            const char *,...);
extern void vopened(const char *, const char *, long, int,
		            const char *, va_list);

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
