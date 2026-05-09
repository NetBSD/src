/*	$NetBSD: trace.h,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

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
			        const char *, const POL_STATS *, DSN *);
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
/*
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
