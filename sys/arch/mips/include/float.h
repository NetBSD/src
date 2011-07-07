/*	$NetBSD: float.h,v 1.16 2011/07/07 22:52:48 matt Exp $ */

#ifndef _MIPS_FLOAT_H_
#define _MIPS_FLOAT_H_

#include <sys/cdefs.h>

#if defined(__mips_n32) || defined(__mips_n64)

#if __GNUC_PREREQ__(4,1)

#define LDBL_MANT_DIG	__LDBL_MANT_DIG__
#define	LDBL_DIG	__LDBL_DIG__
#define	LDBL_MIN_EXP	__LDBL_MIN_EXP__
#define	LDBL_MIN_10_EXP	__LDBL_MIN_10_EXP__
#define	LDBL_MAX_EXP	__LDBL_MAX_EXP__
#define	LDBL_MAX_10_EXP	__LDBL_MAX_10_EXP__
#define	LDBL_EPSILON	__LDBL_EPSILON__
#define	LDBL_MIN	__LDBL_MIN__
#define	LDBL_MAX	__LDBL_MAX__

#else

#define LDBL_MANT_DIG	113
#define	LDBL_DIG	33
#define	LDBL_MIN_EXP	(-16381)
#define	LDBL_MIN_10_EXP	(-4931)
#define	LDBL_MAX_EXP	16384
#define	LDBL_MAX_10_EXP	4932
#if __STDC_VERSION__ >= 199901L
#define	LDBL_EPSILON	0x1p-112L
#define	LDBL_MIN	0x1p-16382L
#define	LDBL_MAX	0x1.ffffffffffffffffffffffffffffp+16383L,
#else
#define	LDBL_EPSILON	1.9259299443872358530559779425849273E-34L
#define	LDBL_MIN	3.3621031431120935062626778173217526E-4932L
#define	LDBL_MAX	1.1897314953572317650857593266280070E+4932L
#endif

#endif /* !__GNUC_PREREQ__(4,1) */

#endif	/* __mips_n32 || __mips_n64 */

#include <sys/float_ieee754.h>

#if defined(__mips_n32) || defined(__mips_n64)

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
#if __GNUC_PREREQ__(4,1)
#define	DECIMAL_DIG	__DECIMAL_DIG__
#else
#define	DECIMAL_DIG	36
#endif
#endif /* !defined(_ANSI_SOURCE) && ... */

#endif	/* __mips_n32 || __mips_n64 */

#endif	/* _MIPS_FLOAT_H_ */
