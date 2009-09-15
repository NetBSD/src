/*	$NetBSD: fsspace.h,v 1.1.1.1.2.2 2009/09/15 06:03:58 snj Exp $	*/

#ifndef _FSSPACE_H_INCLUDED_
#define _FSSPACE_H_INCLUDED_

/*++
/* NAME
/*	fsspace 3h
/* SUMMARY
/*	determine available file system space
/* SYNOPSIS
/*	#include <fsspace.h>
/* DESCRIPTION
/* .nf

 /* External interface. */
struct fsspace {
    unsigned long block_size;		/* block size */
    unsigned long block_free;		/* free space */
};

extern void fsspace(const char *, struct fsspace *);

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
