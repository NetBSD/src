/*	$NetBSD: mail_addr_map.h,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

#ifndef _MAIL_ADDR_MAP_H_INCLUDED_
#define _MAIL_ADDR_MAP_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_map 3h
/* SUMMARY
/*	generic address mapping
/* SYNOPSIS
/*	#include <mail_addr_map.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <argv.h>

 /*
  * Global library.
  */
#include <maps.h>

 /*
  * External interface.
  */
extern ARGV *mail_addr_map(MAPS *, const char *, int);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
