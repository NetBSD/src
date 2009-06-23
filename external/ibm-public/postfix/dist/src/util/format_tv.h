/*	$NetBSD: format_tv.h,v 1.1.1.1 2009/06/23 10:08:59 tron Exp $	*/

#ifndef _FORMAT_TV_H_INCLUDED_
#define _FORMAT_TV_H_INCLUDED_

/*++
/* NAME
/*	format_tv 3h
/* SUMMARY
/*	format time with limited precision
/* SYNOPSIS
/*	#include <format_tv.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern VSTRING *format_tv(VSTRING *, int, int, int, int);

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
