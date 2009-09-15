/*	$NetBSD: mail_addr_find.h,v 1.1.1.1.2.2 2009/09/15 06:02:44 snj Exp $	*/

#ifndef _MAIL_ADDR_FIND_H_INCLUDED_
#define _MAIL_ADDR_FIND_H_INCLUDED_

/*++
/* NAME
/*	mail_addr_find 3h
/* SUMMARY
/*	generic address-based lookup
/* SYNOPSIS
/*	#include <mail_addr_find.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <maps.h>

 /*
  * External interface.
  */
extern const char *mail_addr_find(MAPS *, const char *, char **);

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
