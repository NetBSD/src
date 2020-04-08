/*	$NetBSD: lstat_as.h,v 1.2.12.1 2020/04/08 14:06:59 martin Exp $	*/

#ifndef _LSTAT_AS_H_INCLUDED_
#define _LSTAT_AS_H_INCLUDED_

/*++
/* NAME
/*	lstat_as 3h
/* SUMMARY
/*	lstat file as user
/* SYNOPSIS
/*	#include <sys/stat.h>
/*	#include <lstat_as.h>
/* DESCRIPTION
/* .nf

 /* External interface. */

extern int WARN_UNUSED_RESULT lstat_as(const char *, struct stat *, uid_t, gid_t);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

#endif
