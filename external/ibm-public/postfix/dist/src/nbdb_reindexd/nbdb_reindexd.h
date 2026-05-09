/*	$NetBSD: nbdb_reindexd.h,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

#ifndef _NBDB_REINDEXD_H_INCLUDED_
#define _NBDB_REINDEXD_H_INCLUDED_

/*++
/* NAME
/*	nbdb_reindexd.h 3h
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_reindexd.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <allowed_prefix.h>

 /*
  * Internal API.
  */
extern ALLOWED_PREFIX *parsed_allow_root_pfxs;
extern ALLOWED_PREFIX *parsed_allow_user_pfxs;

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
