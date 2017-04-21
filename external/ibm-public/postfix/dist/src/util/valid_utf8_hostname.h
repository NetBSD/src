/*	$NetBSD: valid_utf8_hostname.h,v 1.2.4.2 2017/04/21 16:52:53 bouyer Exp $	*/

#ifndef _VALID_UTF8_HOSTNAME_H_INCLUDED_
#define _VALID_UTF8_HOSTNAME_H_INCLUDED_

/*++
/* NAME
/*	valid_utf8_hostname 3h
/* SUMMARY
/*	validate (maybe UTF-8) domain name
/* SYNOPSIS
/*	#include <valid_utf8_hostname.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <valid_hostname.h>

 /*
  * External interface
  */
extern int valid_utf8_hostname(int, const char *, int);

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
