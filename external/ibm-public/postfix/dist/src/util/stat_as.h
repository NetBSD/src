/*	$NetBSD: stat_as.h,v 1.1.1.1.36.1 2017/04/21 16:52:53 bouyer Exp $	*/

#ifndef _STAT_AS_H_INCLUDED_
#define _STAT_AS_H_INCLUDED_

/*++
/* NAME
/*	stat_as 3h
/* SUMMARY
/*	stat file as user
/* SYNOPSIS
/*	#include <sys/stat.h>
/*	#include <stat_as.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int WARN_UNUSED_RESULT stat_as(const char *, struct stat *, uid_t, gid_t);

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
