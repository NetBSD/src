/*	$NetBSD: warn_stat.h,v 1.1.1.1 2013/01/02 18:59:15 tron Exp $	*/

#ifndef _WARN_STAT_H_
#define _WARN_STAT_H_

/*++
/* NAME
/*	warn_stat 3h
/* SUMMARY
/*	baby-sit stat() error returns
/* SYNOPSIS
/*	#include <warn_stat.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#ifndef WARN_STAT_INTERNAL
#define stat(p, s) warn_stat((p), (s))
#define lstat(p, s) warn_lstat((p), (s))
#define fstat(f, s) warn_fstat((f), (s))
#endif

extern int warn_stat(const char *path, struct stat *);
extern int warn_lstat(const char *path, struct stat *);
extern int warn_fstat(int, struct stat *);

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
