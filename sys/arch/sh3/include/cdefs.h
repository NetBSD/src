/*	$NetBSD: cdefs.h,v 1.2.4.2 2002/06/23 17:40:37 jdolecek Exp $	*/

#ifndef _SH3_CDEFS_H_
#define	_SH3_CDEFS_H_

/*
 * The old NetBSD/sh3 ELF toolchain used underscores.  The new
 * NetBSD/sh3 ELF toolchain does not.  The C pre-processor
 * defines __NO_LEADING_UNDERSCORES__ for the new ELF toolchain.
 */

#if defined(__ELF__) && !defined(__NO_LEADING_UNDERSCORES__)
#define	__LEADING_UNDERSCORE
#endif

#endif /* !_SH3_CDEFS_H_ */
