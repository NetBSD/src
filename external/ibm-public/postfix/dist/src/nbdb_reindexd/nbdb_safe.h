/*	$NetBSD: nbdb_safe.h,v 1.2.2.2 2026/05/11 17:13:51 martin Exp $	*/

#ifndef _NBDB_SAFE_H_INCLUDED_
#define _NBDB_SAFE_H_INCLUDED_

/*++
/* NAME
/*	nbdb_safe.h 3h
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_safe.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <sys/stat.h>

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Internal API.
  */
extern bool nbdb_safe_for_uid(uid_t, const struct stat *);
extern bool nbdb_safe_to_index_as_legacy_index_owner(
				          const char *, const struct stat *,
				          const char *, const struct stat *,
				          const char *, const struct stat *,
						             VSTRING *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
