/*	$NetBSD: nbdb_process.h,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

#ifndef _NBDB_PROCESS_H_INCLUDED_
#define _NBDB_PROCESS_H_INCLUDED_

/*++
/* NAME
/*	nbdb_process.h 3h
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_process.h>
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
extern int nbdb_process(const char *, const char *, VSTRING *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
