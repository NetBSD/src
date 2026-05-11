/*	$NetBSD: nbdb_index_as.h,v 1.2.2.2 2026/05/11 17:13:51 martin Exp $	*/

#ifndef _NBDB_INDEX_AS_H_INCLUDED_
#define _NBDB_INDEX_AS_H_INCLUDED_

/*++
/* NAME
/*	nbdb_index_as.h 3h
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_index_as.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Global library.
  */
#include <nbdb_util.h>

 /*
  * Internal API.
  */
extern int nbdb_index_as(const char *, const char *, const char *,
			         uid_t, gid_t, VSTRING *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
