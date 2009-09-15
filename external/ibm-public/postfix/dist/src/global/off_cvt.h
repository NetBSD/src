/*	$NetBSD: off_cvt.h,v 1.1.1.1.2.2 2009/09/15 06:02:51 snj Exp $	*/

#ifndef _OFF_CVT_H_INCLUDED_
#define _OFF_CVT_H_INCLUDED_

/*++
/* NAME
/*	off_cvt 3h
/* SUMMARY
/*	off_t conversions
/* SYNOPSIS
/*	#include <vstring.h>
/*	#include <off_cvt.h>
/* DESCRIPTION
/* .nf

 /*
  * Utility library.
  */
#include <vstring.h>

 /*
  * External interface.
  */
extern off_t off_cvt_string(const char *);
extern VSTRING *off_cvt_number(VSTRING *, off_t);

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
