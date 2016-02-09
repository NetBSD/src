/*	Id	*/	
/*	$NetBSD: target.c,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

/*-
 * Copyright (c) 2014 Iain Hibbert.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * compile this file for the machine target, with
 *	-I ..
 *	-I ../os/$(TARGOS) -D os_$(TARGOS)
 *	-I ../arch/$(TARGMACH) -D mach_$(TARGMACH)
 *	-c target.c
 *
 * which will provide an array of the target-specific definitions
 */

#include "config.h"

/*
 * Bring in the machine definitions, from
 *	"arch/$(TARGMACH)/macdefs.h"
 */
#include "macdefs.h"

/*
 * Bring in the OS definitions, from
 *	"os/$(TARGOS)/osdefs.h"
 */
#include "osdefs.h"

/* basic type names */
#define	TYPE_CHAR		char
#define	TYPE_UCHAR		unsigned char
#define	TYPE_SHORT		short int
#define	TYPE_USHORT		unsigned short int
#define	TYPE_INT		int
#define	TYPE_UNSIGNED		unsigned int
#define	TYPE_LONG		long int
#define	TYPE_ULONG		unsigned long int
#define	TYPE_LONGLONG		long long int
#define	TYPE_ULONGLONG		unsigned long long int
#define	TYPE_FLOAT		float
#define	TYPE_DOUBLE		double
#define	TYPE_LDOUBLE		long double

/*
 * C99 basic type sizes (machine-dependent)
 *   - preprocessor cannot stringify the result of a division
 */

#if (SZSHORT/SZCHAR) == 2
#define	SIZE_SHORT		2
#else
#error Do not understand SZSHORT
#endif

#if (SZINT/SZCHAR) == 2
#define	SIZE_INT		2
#elif (SZINT/SZCHAR) == 4
#define	SIZE_INT		4
#else
#error Do not understand SZINT
#endif

#if (SZLONG/SZCHAR) == 2
#define	SIZE_LONG		2
#elif (SZLONG/SZCHAR) == 4
#define	SIZE_LONG		4
#elif (SZLONG/SZCHAR) == 8
#define	SIZE_LONG		8
#else
#error Do not understand SZLONG
#endif

/*
 * C99 extended type definitions (OS-dependent)
 */

#if WCHAR_TYPE == USHORT
#define	PCC_WCHAR_MIN		0
#define	PCC_WCHAR_MAX		MAX_USHORT
#define	PCC_WCHAR_TYPE		TYPE_USHORT
#define	PCC_WCHAR_SIZE		SIZE_SHORT
#elif WCHAR_TYPE == INT
#define	PCC_WCHAR_MIN		MIN_INT
#define	PCC_WCHAR_MAX		MAX_INT
#define	PCC_WCHAR_TYPE		TYPE_INT
#define	PCC_WCHAR_SIZE		SIZE_INT
#else
#error Do not understand WCHAR_TYPE
#endif

#if WCHAR_SIZE != PCC_WCHAR_SIZE
#error WCHAR_SIZE does not match WCHAR_TYPE
#endif

#if WINT_TYPE == INT
#define	PCC_WINT_MIN		MIN_INT
#define	PCC_WINT_MAX		MAX_INT
#define	PCC_WINT_TYPE		TYPE_INT
#define	PCC_WINT_SIZE		SIZE_INT
#elif WINT_TYPE == UNSIGNED
#define	PCC_WINT_MIN		0
#define	PCC_WINT_MAX		MAX_UNSIGNED
#define	PCC_WINT_TYPE		TYPE_UNSIGNED
#define	PCC_WINT_SIZE		SIZE_INT
#else
#error Do not understand WINT_TYPE
#endif

#if SIZE_TYPE == UNSIGNED
#define	PCC_SIZE_MIN		0
#define	PCC_SIZE_MAX		MAX_UNSIGNED
#define	PCC_SIZE_TYPE		TYPE_UNSIGNED
#define	PCC_SIZE_SIZE		SIZE_INT
#elif SIZE_TYPE == ULONG
#define	PCC_SIZE_MIN		0
#define	PCC_SIZE_MAX		MAX_ULONG
#define	PCC_SIZE_TYPE		TYPE_ULONG
#define	PCC_SIZE_SIZE		SIZE_LONG
#else
#error Do not understand SIZE_TYPE
#endif

#if PTRDIFF_TYPE == INT
#define	PCC_PTRDIFF_MIN		MIN_INT
#define	PCC_PTRDIFF_MAX		MAX_INT
#define	PCC_PTRDIFF_TYPE	TYPE_INT
#define	PCC_PTRDIFF_SIZE	SIZE_INT
#elif PTRDIFF_TYPE == LONG
#define	PCC_PTRDIFF_MIN		MIN_LONG
#define	PCC_PTRDIFF_MAX		MAX_LONG
#define	PCC_PTRDIFF_TYPE	TYPE_LONG
#define	PCC_PTRDIFF_SIZE	SIZE_LONG
#else
#error Do not understand PTRDIFF_TYPE
#endif

/* IEEE single precision (32-bit) constants */
#define	SP_DIG			"6"
#define	SP_EPSILON		"0x1.0p-23"
#define	SP_MANT_DIG		"24"
#define	SP_MAX_10_EXP		"38"
#define	SP_MAX_EXP		"128"
#define	SP_MAX			"0x1.fffffep+127"
#define	SP_MIN_10_EXP		"(-37)"
#define	SP_MIN_EXP		"(-125)"
#define	SP_MIN			"0x1.0p-126"

/* IEEE double precision (64-bit) constants */
#define	DP_DIG			"15"
#define	DP_EPSILON		"0x1.0p-52"
#define	DP_MANT_DIG		"53"
#define	DP_MAX_10_EXP		"308"
#define	DP_MAX_EXP		"1024"
#define	DP_MAX			"0x1.fffffffffffffp+1023"
#define	DP_MIN_10_EXP		"(-307)"
#define	DP_MIN_EXP		"(-1021)"
#define	DP_MIN			"0x1.0p-1022"

/* Intel "double extended" precision (80-bit) constants */
#define	X80_DIG			"18"
#define	X80_EPSILON		"0x1.0p-63"
#define	X80_MANT_DIG		"64"
#define	X80_MAX_10_EXP		"4932"
#define	X80_MAX_EXP		"16384"
#define	X80_MAX			"0x1.fffffffffffffffep+16383"
#define	X80_MIN_10_EXP		"(-4931)"
#define	X80_MIN_EXP		"(-16381)"
#define	X80_MIN			"0x1.0p-16382"

/* IEEE quad precision (128-bit) constants */
#define	QP_DIG			"33"
#define	QP_EPSILON		"0x1.0p-112"
#define	QP_MANT_DIG		"113"
#define	QP_MAX_10_EXP		"4932"
#define	QP_MAX_EXP		"16384"
#define	QP_MAX			"0x1.ffffffffffffffffffffffffffffp+16383"
#define	QP_MIN_10_EXP		"(-4931)"
#define	QP_MIN_EXP		"(-16381)"
#define	QP_MIN			"0x1.0p-16382""

#if SZFLOAT == 32
#define	PCC_FLT_DIG		SP_DIG
#define	PCC_FLT_EPSILON		SP_EPSILON
#define	PCC_FLT_MANT_DIG	SP_MANT_DIG
#define	PCC_FLT_MAX_10_EXP	SP_MAX_10_EXP
#define	PCC_FLT_MAX_EXP		SP_MAX_EXP
#define	PCC_FLT_MAX		SP_MAX
#define	PCC_FLT_MIN_10_EXP	SP_MIN_10_EXP
#define	PCC_FLT_MIN_EXP		SP_MIN_EXP
#define	PCC_FLT_MIN		SP_MIN
#else
#error Do not understand SZFLOAT
#endif

#if SZDOUBLE == 64
#define	PCC_DBL_DIG		DP_DIG
#define	PCC_DBL_EPSILON		DP_EPSILON
#define	PCC_DBL_MANT_DIG	DP_MANT_DIG
#define	PCC_DBL_MAX_10_EXP	DP_MAX_10_EXP
#define	PCC_DBL_MAX_EXP		DP_MAX_EXP
#define	PCC_DBL_MAX		DP_MAX
#define	PCC_DBL_MIN_10_EXP	DP_MIN_10_EXP
#define	PCC_DBL_MIN_EXP		DP_MIN_EXP
#define	PCC_DBL_MIN		DP_MIN
#else
#error Do not understand SZDOUBLE
#endif

#if SZLDOUBLE == 64
#define	PCC_LDBL_DIG		DP_DIG
#define	PCC_LDBL_EPSILON	DP_EPSILON
#define	PCC_LDBL_MANT_DIG	DP_MANT_DIG
#define	PCC_LDBL_MAX_10_EXP	DP_MAX_10_EXP
#define	PCC_LDBL_MAX_EXP	DP_MAX_EXP
#define	PCC_LDBL_MAX		DP_MAX
#define	PCC_LDBL_MIN_10_EXP	DP_MIN_10_EXP
#define	PCC_LDBL_MIN_EXP	DP_MIN_EXP
#define	PCC_LDBL_MIN		DP_MIN
#elif SZLDOUBLE == 96
#define	PCC_LDBL_DIG		X80_DIG
#define	PCC_LDBL_EPSILON	X80_EPSILON
#define	PCC_LDBL_MANT_DIG	X80_MANT_DIG
#define	PCC_LDBL_MAX_10_EXP	X80_MAX_10_EXP
#define	PCC_LDBL_MAX_EXP	X80_MAX_EXP
#define	PCC_LDBL_MAX		X80_MAX
#define	PCC_LDBL_MIN_10_EXP	X80_MIN_10_EXP
#define	PCC_LDBL_MIN_EXP	X80_MIN_EXP
#define	PCC_LDBL_MIN		X80_MIN
#elif SZLDOUBLE == 128
#define	PCC_LDBL_DIG		QP_DIG
#define	PCC_LDBL_EPSILON	QP_EPSILON
#define	PCC_LDBL_MANT_DIG	QP_MANT_DIG
#define	PCC_LDBL_MAX_10_EXP	QP_MAX_10_EXP
#define	PCC_LDBL_MAX_EXP	QP_MAX_EXP
#define	PCC_LDBL_MAX		QP_MAX
#define	PCC_LDBL_MIN_10_EXP	QP_MIN_10_EXP
#define	PCC_LDBL_MIN_EXP	QP_MIN_EXP
#define	PCC_LDBL_MIN		QP_MIN
#else
#error Do not understand SZLDOUBLE
#endif

#define	MKS(x)		_MKS(x)
#define	_MKS(x)		#x

/*
 * target-specific definitions
 */

const char *target_cppdefs[] = {
#if defined(CPPDEFS_os)
	CPPDEFS_os,
#endif
#if defined(CPPDEFS_mach)
	CPPDEFS_mach,
#endif
#if (SZINT == 16) && (SZLONG == 32) && (SZPOINT(CHAR) == 32)
	"-D__LP32__",
	"-D_LP32",
#endif
#if (SZINT == 32) && (SZLONG == 32) && (SZPOINT(CHAR) == 32)
	"-D__ILP32__",
	"-D_ILP32",
#endif
#if (SZINT == 32) && (SZLONG == 64) && (SZPOINT(CHAR) == 64)
	"-D__LP64__",
	"-D_LP64",
#endif
#if (SZINT == 64) && (SZLONG == 64) && (SZPOINT(CHAR) == 64)
	"-D__ILP64__",
	"-D_ILP64",
#endif
#if defined(ELFABI)
	"-D__ELF__",
#endif

	"-D__CHAR_MAX__=" MKS(MAX_CHAR),
	"-D__SHORT_MAX__=" MKS(MAX_SHORT),
	"-D__INT_MAX__=" MKS(MAX_INT),
	"-D__LONG_MAX__=" MKS(MAX_LONG),
	"-D__LONG_LONG_MAX__=" MKS(MAX_LONGLONG),

	"-D__WCHAR_MIN__=" MKS(PCC_WCHAR_MIN),
	"-D__WCHAR_MAX__=" MKS(PCC_WCHAR_MAX),
	"-D__WCHAR_TYPE__=" MKS(PCC_WCHAR_TYPE),
	"-D__SIZEOF_WCHAR__=" MKS(PCC_WCHAR_SIZE),

	"-D__WINT_MIN__=" MKS(PCC_WINT_MIN),
	"-D__WINT_MAX__=" MKS(PCC_WINT_MAX),
	"-D__WINT_TYPE__=" MKS(PCC_WINT_TYPE),
	"-D__SIZEOF_WINT__=" MKS(PCC_WINT_SIZE),

	"-D__SIZE_MAX__=" MKS(PCC_SIZE_MAX),
	"-D__SIZE_TYPE__=" MKS(PCC_SIZE_TYPE),
	"-D__SIZEOF_SIZE__=" MKS(PCC_SIZE_SIZE),

	"-D__PTRDIFF_MIN__=" MKS(PCC_PTRDIFF_MIN),
	"-D__PTRDIFF_MAX__=" MKS(PCC_PTRDIFF_MAX),
	"-D__PTRDIFF_TYPE__=" MKS(PCC_PTRDIFF_TYPE),
	"-D__SIZEOF_PTRDIFF__=" MKS(PCC_PTRDIFF_SIZE),

	"-D__FLT_DIG__=" PCC_FLT_DIG,
	"-D__FLT_EPSILON__=" PCC_FLT_EPSILON "F",
	"-D__FLT_MANT_DIG__=" PCC_FLT_MANT_DIG,
	"-D__FLT_MAX_10_EXP__=" PCC_FLT_MAX_10_EXP,
	"-D__FLT_MAX_EXP__=" PCC_FLT_MAX_EXP,
	"-D__FLT_MAX__=" PCC_FLT_MAX "F",
	"-D__FLT_MIN_10_EXP__=" PCC_FLT_MIN_10_EXP,
	"-D__FLT_MIN_EXP__=" PCC_FLT_MIN_EXP,
	"-D__FLT_MIN__=" PCC_FLT_MIN "F",

	"-D__DBL_DIG__=" PCC_DBL_DIG,
	"-D__DBL_EPSILON__=" PCC_DBL_EPSILON,
	"-D__DBL_MANT_DIG__=" PCC_DBL_MANT_DIG,
	"-D__DBL_MAX_10_EXP__=" PCC_DBL_MAX_10_EXP,
	"-D__DBL_MAX_EXP__=" PCC_DBL_MAX_EXP,
	"-D__DBL_MAX__=" PCC_DBL_MAX,
	"-D__DBL_MIN_10_EXP__=" PCC_DBL_MIN_10_EXP,
	"-D__DBL_MIN_EXP__=" PCC_DBL_MIN_EXP,
	"-D__DBL_MIN__=" PCC_DBL_MIN,

	"-D__LDBL_DIG__=" PCC_LDBL_DIG,
	"-D__LDBL_EPSILON__=" PCC_LDBL_EPSILON "L",
	"-D__LDBL_MANT_DIG__=" PCC_LDBL_MANT_DIG,
	"-D__LDBL_MAX_10_EXP__=" PCC_LDBL_MAX_10_EXP,
	"-D__LDBL_MAX_EXP__=" PCC_LDBL_MAX_EXP,
	"-D__LDBL_MAX__=" PCC_LDBL_MAX "L",
	"-D__LDBL_MIN_10_EXP__=" PCC_LDBL_MIN_10_EXP,
	"-D__LDBL_MIN_EXP__=" PCC_LDBL_MIN_EXP,
	"-D__LDBL_MIN__=" PCC_LDBL_MIN "L",

	NULL
};

/*  put these in macdefs.h ??

#if defined(mach_amd64)
	"-D__x86_64__",
	"-D__x86_64",
	"-D__amd64__",
	"-D__amd64",
#endif
#if defined(mach_arm)
	"-D__arm__",
	"-D__arm",
#endif
#if defined(mach_i386)
	"-D__x86__",
	"-D__x86",
	"-D__i386__",
	"-D__i386",
#endif
#if defined(mach_nova)
	"-D__nova__",
	"-D__nova",
#endif
#if defined(mach_m16c)
	"-D__m16c__",
	"-D__m16c",
#endif
#if defined(mach_mips)
	"-D__mips__",
	"-D__mips",
#endif
#if defined(mach_pdp10)
	"-D__pdp10__",
	"-D__pdp10",
#endif
#if defined(mach_pdp11)
	"-D__pdp11__",
	"-D__pdp11",
#endif
#if defined(mach_powerpc)
	"-D__powerpc__",
	"-D__powerpc",
#endif
#if defined(mach_sparc64)
	"-D__sparc_v9__",
	"-D__sparc64__",
	"-D__sparc64",
	"-D__sparc__",
	"-D__sparc",
#endif
#if defined(mach_vax)
	"-D__vax__",
	"-D__vax",
#endif

*/
