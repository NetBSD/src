/*	$NetBSD: cdefs.h,v 1.5.156.1 2012/02/18 07:33:09 mrg Exp $	*/

#ifndef _SH3_CDEFS_H_
#define	_SH3_CDEFS_H_

#define	__ALIGNBYTES		(sizeof(int) - 1)

/*
 * The old NetBSD/sh3 ELF toolchain used underscores.  The new
 * NetBSD/sh3 ELF toolchain does not.  The C pre-processor
 * defines __NO_LEADING_UNDERSCORES__ for the new ELF toolchain.
 */

#if defined(__ELF__) && !defined(__NO_LEADING_UNDERSCORES__)
#define	__LEADING_UNDERSCORE
#endif

#endif /* !_SH3_CDEFS_H_ */
