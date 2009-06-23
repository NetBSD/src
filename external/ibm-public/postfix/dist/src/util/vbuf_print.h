/*	$NetBSD: vbuf_print.h,v 1.1.1.1 2009/06/23 10:09:01 tron Exp $	*/

#ifndef _VBUF_PRINT_H_INCLUDED_
#define _VBUF_PRINT_H_INCLUDED_

/*++
/* NAME
/*	vbuf_print 3h
/* SUMMARY
/*	formatted print to generic buffer
/* SYNOPSIS
/*	#include <vbuf_print.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#include <stdarg.h>

 /*
  * Utility library.
  */
#include <vbuf.h>

 /*
  * External interface.
  */
extern VBUF *vbuf_print(VBUF *, const char *, va_list);

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
