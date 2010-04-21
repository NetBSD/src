/*	$NetBSD: dot_lockfile_as.h,v 1.1.1.1.4.2 2010/04/21 05:23:51 matt Exp $	*/

#ifndef _DOT_LOCKFILE_AS_H_INCLUDED_
#define _DOT_LOCKFILE_AS_H_INCLUDED_

/*++
/* NAME
/*	dot_lockfile_as 3h
/* SUMMARY
/*	dotlock file management
/* SYNOPSIS
/*	#include <dot_lockfile_as.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern int dot_lockfile_as(const char *, VSTRING *, uid_t, gid_t);
extern void dot_unlockfile_as(const char *, uid_t, gid_t);

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
