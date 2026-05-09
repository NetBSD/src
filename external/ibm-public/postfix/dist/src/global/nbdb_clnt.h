/*	$NetBSD: nbdb_clnt.h,v 1.2 2026/05/09 18:49:16 christos Exp $	*/

#ifndef _NBDB_CLNT_H_INCLUDED_
#define _NBDB_CLNT_H_INCLUDED_

/*++
/* NAME
/*	nbdb_clnt 3h
/* SUMMARY
/*	Non-Berkeley-DB migration client
/* SYNOPSIS
/*	#include <nbdb_clnt.h>
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

extern int nbdb_clnt_request(const char *, const char *, int, int, int, VSTRING *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
