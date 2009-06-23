/*	$NetBSD: trace.h,v 1.1.1.1 2009/06/23 10:08:48 tron Exp $	*/

#ifndef _TRACE_H_INCLUDED_
#define _TRACE_H_INCLUDED_

/*++
/* NAME
/*	trace 3h
/* SUMMARY
/*	update user message delivery record
/* SYNOPSIS
/*	#include <trace.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <bounce.h>

 /*
  * External interface.
  */
extern int trace_append(int, const char *, MSG_STATS *, RECIPIENT *,
			        const char *, DSN *);
extern int trace_flush(int, const char *, const char *, const char *,
		               const char *, const char *, int);

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
