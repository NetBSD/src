/*	$NetBSD: cdefs.h,v 1.4 2002/03/17 17:55:24 uch Exp $	*/

#ifndef _SH3_CDEFS_H_
#define _SH3_CDEFS_H_

/*
 * The old NetBSD/sh3 ELF toolchain used underscores.  The new
 * NetBSD/sh3 ELF toolchain does not.  The C pre-processor
 * defines __NO_LEADING_UNDERSCORES__ for the new ELF toolchain.
 */

#if defined(__ELF__) && !defined(__NO_LEADING_UNDERSCORES__)
#define __LEADING_UNDERSCORE
#endif

#endif /* !_SH3_CDEFS_H_ */
