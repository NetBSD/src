/*	$NetBSD: compat_va_copy.h,v 1.1.1.1.6.2 2014/08/19 23:59:45 tls Exp $	*/

#ifndef _COMPAT_VA_COPY_H_INCLUDED_
#define _COMPAT_VA_COPY_H_INCLUDED_

/*++
/* NAME
/*	compat_va_copy 3h
/* SUMMARY
/*	compatibility
/* SYNOPSIS
/*	#include <compat_va_copy.h>
/* DESCRIPTION
/* .nf

 /*
  * C99 defines va_start and va_copy as macros, so we can probe the
  * compilation environment with #ifdef etc. Some environments define
  * __va_copy so we probe for that, too.
  */
#if !defined(va_start)
#error "include <stdarg.h> first"
#endif

#if !defined(VA_COPY)
#if defined(va_copy)
#define VA_COPY(dest, src) va_copy(dest, src)
#elif defined(__va_copy)
#define VA_COPY(dest, src) __va_copy(dest, src)
#else
#define VA_COPY(dest, src) (dest) = (src)
#endif
#endif					/* VA_COPY */

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
