/*	$NetBSD: line_number.h,v 1.1.1.1 2013/09/25 19:06:37 tron Exp $	*/

#ifndef _LINE_NUMBER_H_INCLUDED_
#define _LINE_NUMBER_H_INCLUDED_

/*++
/* NAME
/*	line_number 3h
/* SUMMARY
/*	line number utilities
/* SYNOPSIS
/*	#include <line_number.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern char *format_line_number(VSTRING *, ssize_t, ssize_t);

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
