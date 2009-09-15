/*	$NetBSD: open_as.h,v 1.1.1.1.2.2 2009/09/15 06:04:01 snj Exp $	*/

#ifndef _OPEN_H_INCLUDED_
#define _OPEN_H_INCLUDED_

/*++
/* NAME
/*	open_as 3h
/* SUMMARY
/*	open file as user
/* SYNOPSIS
/*	#include <fcntl.h>
/*	#include <open_as.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int open_as(const char *, int, int, uid_t, gid_t);

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
