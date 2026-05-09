/*	$NetBSD: wrap_stat.c,v 1.1.1.1 2026/05/09 18:39:25 christos Exp $	*/

/*++
/* NAME
/*	wrap_stat 3
/* SUMMARY
/*	mockable stat/lstat wrappers
/* SYNOPSIS
/*	#include <wrap_stat.h>
/*
/*	int     wrap_stat(
/*	const char *path,
/*	struct stat *st)
/*
/*	int     wrap_lstat(
/*	const char *path,
/*	struct stat *st)
/* DESCRIPTION
/*	This module is a NOOP when the NO_MOCK_WRAPPERS macro is
/*	defined.
/*
/*	By default this module redirects stat() and lstat() calls to
/*	the above listed wrapper functions that may be overridden with
/*	mocks. This arrangement prevents mock stat() or lstat() functions
/*	from interfering with stat() or lstat() calls made by system
/*	libraries or by third-party libraries.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/stat.h>

 /*
  * Utility library.
  */
#include <wrap_stat.h>

/* wrap_stat - mockable wrapper */

int     wrap_stat(const char *path, struct stat *st)
{
#undef stat
    return (stat (path, st));
}

/* wrap_lstat - mockable wrapper */

int     wrap_lstat(const char *path, struct stat *st)
{
#undef lstat
    return (lstat(path, st));
}
