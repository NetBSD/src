/*	$NetBSD: nbdb_util.h,v 1.2 2026/05/09 18:49:16 christos Exp $	*/

#ifndef _NBDB_UTIL_H_INCLUDED_
#define _NBDB_UTIL_H_INCLUDED_

/*++
/* NAME
/*	nbdb_util 3h
/* SUMMARY
/*	Non-Berkeley-DB migration utilities
/* SYNOPSIS
/*	#include <nbdb_util.h>
/* DESCRIPTION
/* .nf

 /*
Utility library. */
#include <dict.h>

 /*
  * External API.
  */
#define NBDB_STAT_OK	0
#define NBDB_STAT_ERROR	1

extern int nbdb_level;
extern bool nbdb_reindexing_lockout;

extern void nbdb_util_init(const char *);
extern int nbdb_is_legacy_type(const char *);
extern const char *nbdb_find_non_leg_suffix(const char *);
extern const char *nbdb_map_leg_type(const char *, const char *,
				        const DICT_OPEN_INFO **, VSTRING *);

#define NBDB_LEGACY_SUFFIX	".db"

 /*
  * Migration level internal forms.
  */
#define NBDB_LEV_CODE_ERROR	(-1)
#define NBDB_LEV_CODE_NONE	(0)
#define NBDB_LEV_CODE_REDIRECT	(1)
#define NBDB_LEV_CODE_REINDEX	(2)

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
