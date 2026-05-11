/*	$NetBSD: wrap_stat.h,v 1.2.2.2 2026/05/11 17:14:05 martin Exp $	*/

#ifndef _WRAP_STAT_H_INCLUDED_
#define _WRAP_STAT_H_INCLUDED_

/*++
/* NAME
/*	wrap_stat 3h
/* SUMMARY
/*	mockable stat/lstat wrappers
/* SYNOPSIS
/*	#include <wrap_stat.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/stat.h>

 /*
  * Mockable API.
  */
#ifndef NO_MOCK_WRAPPERS
extern int wrap_stat(const char *, struct stat *);
extern int wrap_lstat(const char *, struct stat *);

#define stat(path, st) wrap_stat(path, st)
#define lstat(path, st) wrap_lstat(path, st)
#endif

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
