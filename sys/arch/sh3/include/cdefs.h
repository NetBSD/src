/*	$NetBSD: cdefs.h,v 1.2.4.1 2002/01/10 19:48:39 thorpej Exp $	*/

#ifndef _MACHINE_CDEFS_H_
#define _MACHINE_CDEFS_H_

/*
 * The old NetBSD/sh3 ELF toolchain used underscores.  The new
 * NetBSD/sh3 ELF toolchain does not.  The C pre-processor
 * defines __NO_LEADING_UNDERSCORES__ for the new ELF toolchain.
 */

#if defined(__ELF__) && !defined(__NO_LEADING_UNDERSCORES__)
#define __LEADING_UNDERSCORE
#endif

#endif /* !_MACHINE_CDEFS_H_ */
