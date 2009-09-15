/*	$NetBSD: set_eugid.h,v 1.1.1.1.2.2 2009/09/15 06:04:03 snj Exp $	*/

#ifndef _SET_EUGID_H_INCLUDED_
#define _SET_EUGID_H_INCLUDED_

/*++
/* NAME
/*	set_eugid 3h
/* SUMMARY
/*	set effective user and group attributes
/* SYNOPSIS
/*	#include <set_eugid.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern void set_eugid(uid_t, gid_t);

 /*
  * The following macros open and close a block that runs at a different
  * privilege level. To make mistakes with stray curly braces less likely, we
  * shape the macros below as the head and tail of a do-while loop.
  */
#define SAVE_AND_SET_EUGID(uid, gid) do { \
	uid_t __set_eugid_uid = geteuid(); \
	gid_t __set_eugid_gid = getegid(); \
	set_eugid((uid), (gid));

#define RESTORE_SAVED_EUGID() \
	set_eugid(__set_eugid_uid, __set_eugid_gid); \
    } while (0)

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
