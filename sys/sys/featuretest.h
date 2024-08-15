/*	$NetBSD: featuretest.h,v 1.12 2024/08/15 20:42:59 riastradh Exp $	*/

/*
 * Written by Klaus Klein <kleink@NetBSD.org>, February 2, 1998.
 * Public domain.
 *
 * NOTE: Do not protect this header against multiple inclusion.  Doing
 * so can have subtle side-effects due to header file inclusion order
 * and testing of e.g. _POSIX_SOURCE vs. _POSIX_C_SOURCE.  Instead,
 * protect each CPP macro that we want to supply.
 */

/*
 * Feature-test macros are defined by several standards, and allow an
 * application to specify what symbols they want the system headers to
 * expose, and hence what standard they want them to conform to.
 * There are two classes of feature-test macros.  The first class
 * specify complete standards, and if one of these is defined, header
 * files will try to conform to the relevant standard.  They are:
 *
 * ANSI macros:
 * _ANSI_SOURCE			ANSI C89
 *
 * POSIX macros:
 * _POSIX_SOURCE == 1		IEEE Std 1003.1 (version?)
 * _POSIX_C_SOURCE == 1		IEEE Std 1003.1-1990
 * _POSIX_C_SOURCE == 2		IEEE Std 1003.2-1992
 * _POSIX_C_SOURCE == 199309L	IEEE Std 1003.1b-1993
 * _POSIX_C_SOURCE == 199506L	ISO/IEC 9945-1:1996
 * _POSIX_C_SOURCE == 200112L	IEEE Std 1003.1-2001
 * _POSIX_C_SOURCE == 200809L   IEEE Std 1003.1-2008
 * _POSIX_C_SOURCE == 202405L   IEEE Std 1003.1-2024
 *
 * Reference:
 *
 *	The Open Group Base Specifications Issue 8, IEEE Std
 *	1003.1-2024, IEEE and The Open Group, 2024, Sec. 2.2.1.1 `The
 *	_POSIX_C_SOURCE Feature Test Macro'.
 *	https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/functions/V2_chap02.html#tag_16_02_01_01
 *
 * X/Open macros:
 * _XOPEN_SOURCE		System Interfaces and Headers, Issue 4, Ver 2
 * _XOPEN_SOURCE_EXTENDED == 1	XSH4.2 UNIX extensions
 * _XOPEN_SOURCE == 500		System Interfaces and Headers, Issue 5
 * _XOPEN_SOURCE == 520		Networking Services (XNS), Issue 5.2
 * _XOPEN_SOURCE == 600		IEEE Std 1003.1-2001, XSI option
 * _XOPEN_SOURCE == 700		IEEE Std 1003.1-2008, XSI option
 * _XOPEN_SOURCE == 800		IEEE Std 1003.1-2024, XSI option
 *
 * Reference:
 *
 *	The Open Group Base Specifications Issue 8, IEEE Std
 *	1003.1-2024, IEEE and The Open Group, 2024, Sec. 2.2.1.2 `The
 *	_XOPEN_SOURCE Feature Test Macro'.
 *	https://pubs.opengroup.org/onlinepubs/9799919799.2024edition/functions/V2_chap02.html#tag_16_02_01_02
 *
 * NetBSD macros:
 * _NETBSD_SOURCE == 1		Make all NetBSD features available.
 *
 * If more than one of these "major" feature-test macros is defined,
 * then the set of facilities provided (and namespace used) is the
 * union of that specified by the relevant standards, and in case of
 * conflict, the earlier standard in the above list has precedence (so
 * if both _POSIX_C_SOURCE and _NETBSD_SOURCE are defined, the version
 * of rename() that's used is the POSIX one).  If none of the "major"
 * feature-test macros is defined, _NETBSD_SOURCE is assumed.
 *
 * There are also "minor" feature-test macros, which enable extra
 * functionality in addition to some base standard.  They should be
 * defined along with one of the "major" macros.  The "minor" macros
 * are:
 *
 * _REENTRANT		Some thread-safety extensions like lgamma_r(3)
 *			  (mostly subsumed by _POSIX_C_SOURCE >= 199506L)
 * _ISOC99_SOURCE	C99 extensions like snprintf without -std=c99
 * _ISOC11_SOURCE	C11 extensions like aligned_alloc without -std=c11
 * _ISOC23_SOURCE	C23 extensions like mbrtoc8 without -std=c23
 * _OPENBSD_SOURCE	Nonstandard OpenBSD extensions like strtonum(3)
 * _GNU_SOURCE		Nonstandard GNU extensions like feenableexcept(3)
 */

#if defined(_POSIX_SOURCE) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE	1L
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) && !defined(_NETBSD_SOURCE)
#define _NETBSD_SOURCE 1
#endif

#if ((_POSIX_C_SOURCE - 0) >= 199506L || (_XOPEN_SOURCE - 0) >= 500) && \
    !defined(_REENTRANT)
#define _REENTRANT
#endif
