/*	$NetBSD: abounce.h,v 1.2 2017/02/14 01:16:45 christos Exp $	*/

#ifndef _ABOUNCE_H_INCLUDED_
#define _ABOUNCE_H_INCLUDED_

/*++
/* NAME
/*	abounce 3h
/* SUMMARY
/*	asynchronous bounce/defer service client
/* SYNOPSIS
/*	#include <abounce.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <bounce.h>

 /*
  * Client interface.
  */
typedef void (*ABOUNCE_FN) (int, void *);

extern void abounce_flush(int, const char *, const char *, const char *, int, const char *, const char *, int, ABOUNCE_FN, void *);
extern void adefer_flush(int, const char *, const char *, const char *, int, const char *, const char *, int, ABOUNCE_FN, void *);
extern void adefer_warn(int, const char *, const char *, const char *, int, const char *, const char *, int, ABOUNCE_FN, void *);
extern void atrace_flush(int, const char *, const char *, const char *, int, const char *, const char *, int, ABOUNCE_FN, void *);

extern void abounce_flush_verp(int, const char *, const char *, const char *, int, const char *, const char *, int, const char *, ABOUNCE_FN, void *);
extern void adefer_flush_verp(int, const char *, const char *, const char *, int, const char *, const char *, int, const char *, ABOUNCE_FN, void *);

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
