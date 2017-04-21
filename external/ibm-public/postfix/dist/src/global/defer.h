/*	$NetBSD: defer.h,v 1.1.1.1.36.1 2017/04/21 16:52:48 bouyer Exp $	*/

#ifndef _DEFER_H_INCLUDED_
#define _DEFER_H_INCLUDED_

/*++
/* NAME
/*	defer 3h
/* SUMMARY
/*	defer service client interface
/* SYNOPSIS
/*	#include <defer.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <bounce.h>

 /*
  * External interface.
  */
extern int defer_append(int, const char *, MSG_STATS *, RECIPIENT *,
			        const char *, DSN *);
extern int defer_flush(int, const char *, const char *, const char *, int,
		               const char *, const char *, int);
extern int defer_warn(int, const char *, const char *, const char *, int,
		              const char *, const char *, int);
extern int defer_one(int, const char *, const char *, const char *, int,
		             const char *, const char *,
		             int, MSG_STATS *, RECIPIENT *,
		             const char *, DSN *);

 /*
  * Start of private API.
  */
#ifdef DSN_INTERN

extern int defer_append_intern(int, const char *, MSG_STATS *, RECIPIENT *,
			               const char *, DSN *);

#endif

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
