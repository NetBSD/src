/*	$NetBSD: resolve_local.h,v 1.1.1.1.16.1 2013/02/25 00:27:19 tls Exp $	*/

#ifndef _RESOLVE_LOCAL_H_INCLUDED_
#define _RESOLVE_LOCAL_H_INCLUDED_

/*++
/* NAME
/*	resolve_local 3h
/* SUMMARY
/*	determine if address resolves to local mail system
/* SYNOPSIS
/*	#include <resolve_local.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <dict.h>

 /*
  * External interface.
  */
extern int resolve_local(const char *);
extern void resolve_local_init(void);

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
