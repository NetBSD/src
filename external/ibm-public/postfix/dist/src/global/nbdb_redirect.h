/*	$NetBSD: nbdb_redirect.h,v 1.1.1.1 2026/05/09 18:39:18 christos Exp $	*/

#ifndef _NBDB_REDIRECT_H_INCLUDED_
#define _NBDB_REDIRECT_H_INCLUDED_

/*++
/* NAME
/*	nbdb_redirect 3h
/* SUMMARY
/*	Non-Berkeley-DB migration support
/* SYNOPSIS
/*	#include <nbdb_redirect.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External API.
  */
extern void nbdb_rdr_init(void);

#define NBDB_LEGACY_SUFFIX	".db"

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
