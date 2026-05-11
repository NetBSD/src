/*	$NetBSD: nbdb_sniffer.h,v 1.2.2.2 2026/05/11 17:13:52 martin Exp $	*/

#ifndef _NBDB_SNIFFER_H_INCLUDED_
#define _NBDB_SNIFFER_H_INCLUDED_

/*++
/* NAME
/*	nbdb_sniffer.h 3h
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_sniffer.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <unistd.h>

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * Internal API.
  */
extern bool nbdb_parse_as_postmap(VSTRING *);
extern bool nbdb_parse_as_postalias(VSTRING *);
const char *nbdb_get_index_cmd_as(const char *, uid_t, gid_t, VSTRING *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

#endif
