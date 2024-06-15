/*	$NetBSD: float.h,v 1.8 2024/06/15 11:44:09 rillig Exp $	*/

#ifndef _X86_FLOAT_H_
#define _X86_FLOAT_H_

#include <sys/featuretest.h>

/*
 * LDBL_MIN is twice the m68k LDBL_MIN, even though both are 12-byte
 * floats with the same base properties and both allegedly
 * IEEE-compliant, because both these representations materialize the
 * top (integer-part) bit of the mantissa. But on m68k if the exponent
 * is 0 and the integer bit is set, it's a regular number, whereas on
 * x86 it's called a pseudo-denormal and apparently treated as a
 * denormal, so it doesn't count as a valid value for LDBL_MIN.
 *
 * x86 citation: Intel 64 and IA-32 Architectures Software Developer's
 * Manual, vol. 1 (Order Number: 253665-077US, April 2022), Sec. 8.2.2
 * `Unsupported Double Extended-Precision Floating-Point Encodings
 * and Pseudo-Denormals', p. 8-14.
 *
 * m86k citation: MC68881/MC68882 Floating-Point Coprocessor User's
 * Manual, Second Edition (Prentice-Hall, 1989, apparently issued by
 * Freescale), Section 3.2 `Binary Real Data formats', pg. 3-3 bottom
 * in particular and pp. 3-2 to 3-5 in general.
 *
 * If anyone needs to update this comment please make sure the copy in
 * m68k/include/float.h also gets updated.
 */

#define	LDBL_MANT_DIG	64
#define LDBL_EPSILON	1.0842021724855044340E-19L
#define LDBL_DIG	18
#define LDBL_MIN_EXP	(-16381)
#define LDBL_MIN	3.3621031431120935063E-4932L
#define LDBL_MIN_10_EXP	(-4931)
#define LDBL_MAX_EXP	16384
#define LDBL_MAX	1.1897314953572317650E+4932L
#define LDBL_MAX_10_EXP	4932

#include <sys/float_ieee754.h>

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
#define	DECIMAL_DIG	21
#endif /* !defined(_ANSI_SOURCE) && ... */

#endif	/* _X86_FLOAT_H_ */
