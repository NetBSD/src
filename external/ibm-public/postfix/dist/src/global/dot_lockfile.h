/*	$NetBSD: dot_lockfile.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _DOT_LOCKFILE_H_INCLUDED_
#define _DOT_LOCKFILE_H_INCLUDED_

/*++
/* NAME
/*	dot_lockfile 3h
/* SUMMARY
/*	dotlock file management
/* SYNOPSIS
/*	#include <dot_lockfile.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern int dot_lockfile(const char *, VSTRING *);
extern void dot_unlockfile(const char *);

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
