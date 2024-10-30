/*	$NetBSD: float.h,v 1.24 2024/10/30 15:56:11 riastradh Exp $	*/

#ifndef _M68K_FLOAT_H_
#define _M68K_FLOAT_H_

#include <sys/featuretest.h>

/*
 * LDBL_MIN is half the x86 LDBL_MIN, even though both are 12-byte
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
 * x86/include/float.h also gets updated.
 */

#if defined(__LDBL_MANT_DIG__)
#define LDBL_MANT_DIG	__LDBL_MANT_DIG__
#define LDBL_EPSILON	__LDBL_EPSILON__
#define LDBL_DIG	__LDBL_DIG__
#define LDBL_MIN_EXP	__LDBL_MIN_EXP__
#define LDBL_MIN	__LDBL_MIN__
#define LDBL_MIN_10_EXP	__LDBL_MIN_10_EXP__
#define LDBL_MAX_EXP	__LDBL_MAX_EXP__
#define LDBL_MAX	__LDBL_MAX__
#define LDBL_MAX_10_EXP	__LDBL_MAX_10_EXP__
#elif !defined(__mc68010__) && !defined(__mcoldfire__)
#define LDBL_MANT_DIG	64
#define LDBL_EPSILON	1.0842021724855044340E-19L
#define LDBL_DIG	18
#define LDBL_MIN_EXP	(-16381)
#define LDBL_MIN	1.6810515715560467531E-4932L
#define LDBL_MIN_10_EXP	(-4931)
#define LDBL_MAX_EXP	16384
#define LDBL_MAX	1.1897314953572317650E+4932L
#define LDBL_MAX_10_EXP	4932
#endif

#include <sys/float_ieee754.h>

#if !defined(__mc68010__) && !defined(__mcoldfire__)
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
#define	DECIMAL_DIG	21
#endif /* !defined(_ANSI_SOURCE) && ... */
#endif /* !__mc68010__ && !__mcoldfire__ */

#endif	/* !_M68K_FLOAT_H_ */
