/*	$NetBSD: cdefs.h,v 1.3 2001/12/16 18:11:12 thorpej Exp $	*/

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
