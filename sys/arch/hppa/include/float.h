/*	$NetBSD: float.h,v 1.8 2011/07/11 02:54:04 matt Exp $	*/

#ifndef _HPPA_FLOAT_H_
#define _HPPA_FLOAT_H_

#ifdef _LP64
#ifdef __LDBL_MANT_DIG__
#define	LDBL_MANT_DIG	__LDBL_MANT_DIG__
#else
#define	LDBL_MANT_DIG	113
#endif
#ifdef	__LDBL_EPSILON__
#define	LDBL_EPSILON	__LDBL_EPSILON__
#else
#define	LDBL_EPSILON	1.925929944387235853055977942584927319E-34L
#endif
#ifdef	__LDBL_DIG__
#define	LDBL_DIG	__LDBL_DIG__
#else
#define	LDBL_DIG	33
#endif
#ifdef	__LDBL_MIN_EXP__
#define	LDBL_MIN_EXP	__LDBL_MIN_EXP__
#else
#define	LDBL_MIN_EXP	(-16381)
#endif
#ifdef	__LDBL_MIN__
#define	LDBL_MIN	__LDBL_MIN__
#else
#define	LDBL_MIN	3.3621031431120935062626778173217526026E-4932L
#endif
#ifdef	__LDBL_MIN_10_EXP__
#define	LDBL_MIN_10_EXP	__LDBL_MIN_10_EXP__
#else
#define	LDBL_MIN_10_EXP	(-4931)
#endif
#ifdef	__LDBL_MAX_EXP__
#define	LDBL_MAX_EXP	__LDBL_MAX_EXP__
#else
#define	LDBL_MAX_EXP	16384
#endif
#ifdef	__LDBL_MAX__
#define	LDBL_MAX	__LDBL_MAX__
#else
#define	LDBL_MAX	1.1897314953572317650857593266280070162E4932L
#endif
#ifdef	__LDBL_MAX_10_EXP__
#define	LDBL_MAX_10_EXP	__LDBL_MAX_10_EXP__
#else
#define	LDBL_MAX_10_EXP	4932
#endif
#endif

#include <sys/float_ieee754.h>

#ifdef _LP64
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
#define	DECIMAL_DIG	36
#endif /* !defined(_ANSI_SOURCE) && ... */
#endif /* _LP64 */

#endif /* _HPPA_FLOAT_H_ */
