/*	$NetBSD: open_lock.h,v 1.1.1.2 2004/05/31 00:25:00 heas Exp $	*/

#ifndef _OPEN_LOCK_H_INCLUDED_
#define _OPEN_LOCK_H_INCLUDED_

/*++
/* NAME
/*	open_lock 3h
/* SUMMARY
/*	open or create file and lock it for exclusive access
/* SYNOPSIS
/*	#include <open_lock.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <fcntl.h>

 /*
  * Utility library.
  */
#include <vstream.h>
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTREAM *open_lock(const char *, int, int, VSTRING *);

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
