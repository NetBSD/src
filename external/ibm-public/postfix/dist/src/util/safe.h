/*	$NetBSD: safe.h,v 1.1.1.1 2009/06/23 10:09:00 tron Exp $	*/

#ifndef _SAFE_H_INCLUDED_
#define _SAFE_H_INCLUDED_

/*++
/* NAME
/*	safe 3h
/* SUMMARY
/*	miscellaneous taint checks
/* SYNOPSIS
/*	#include <safe.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int unsafe(void);
extern char *safe_getenv(const char *);

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
