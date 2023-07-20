/*	$NetBSD: inittyp.c,v 1.40 2023/07/13 08:40:38 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: inittyp.c,v 1.40 2023/07/13 08:40:38 rillig Exp $");
#endif

#if defined(IS_LINT1)
#include "lint1.h"
#else
#include "lint2.h"
#endif

#define INT_RANK	(/*CONSTCOND*/INTPTR_TSPEC == LONG ? 4 : 5)

#ifdef IS_LINT1
#define typeinfo(name, signed_type, unsigned_type, size_in_bits, rv, c) \
	{ /*CONSTCOND*/ \
		size_in_bits, \
		(c) == 'u' || (c) == 's' || (c) == 'p' ? RK_INTEGER : \
		    (c) == 'f' ? RK_FLOATING : \
		    (c) == 'c' ? RK_COMPLEX : \
		    RK_NONE, \
		rv, \
		signed_type, unsigned_type, \
		(c) == 's' || (c) == 'u', \
		(c) == 'u' || (c) == 'p', \
		(c) == 'f' || (c) == 'c', \
		(c) == 's' || (c) == 'u' || (c) == 'f' || \
		    (c) == 'c', \
		(c) == 's' || (c) == 'u' || (c) == 'f' || \
		    (c) == 'c' || (c) == 'p', \
		(c) == 'c', \
		name, \
	}
#else
#define typeinfo(name, signed_type, unsigned_type, size_in_bits, rank, c) \
	{ /*CONSTCOND*/ \
		signed_type, unsigned_type, \
		(c) == 's' || (c) == 'u', \
		name, \
	}
#endif

/* various type information */
ttab_t	ttab[NTSPEC] = {
	typeinfo(NULL, NO_TSPEC, NO_TSPEC, 0, 0, ' '),
	typeinfo("signed", SIGNED, UNSIGN, 0, 0, ' '),
	typeinfo("unsigned", SIGNED, UNSIGN, 0, 0, ' '),
	typeinfo("_Bool", BOOL, BOOL, CHAR_SIZE, 1, 'u'),
	typeinfo("char", SCHAR, UCHAR, CHAR_SIZE, 2,
	    TARG_CHAR_MIN == 0 ? 'u' : 's'),
	typeinfo("signed char", SCHAR, UCHAR, CHAR_SIZE, 2, 's'),
	typeinfo("unsigned char", SCHAR, UCHAR, CHAR_SIZE, 2, 'u'),
	typeinfo("short", SHORT, USHORT, SHORT_SIZE, 3, 's'),
	typeinfo("unsigned short", SHORT, USHORT, SHORT_SIZE, 3, 'u'),
	typeinfo("int", INT, UINT, INT_SIZE, INT_RANK, 's'),
	typeinfo("unsigned int", INT, UINT, INT_SIZE, INT_RANK, 'u'),
	typeinfo("long", LONG, ULONG, LONG_SIZE, 5, 's'),
	typeinfo("unsigned long", LONG, ULONG, LONG_SIZE, 5, 'u'),
	typeinfo("long long", LLONG, ULLONG, LLONG_SIZE, 6, 's'),
	typeinfo("unsigned long long", LLONG, ULLONG, LLONG_SIZE, 6, 'u'),
#ifdef INT128_SIZE
	typeinfo("__int128_t", INT128, UINT128, INT128_SIZE, 7, 's'),
	typeinfo("__uint128_t", INT128, UINT128, INT128_SIZE, 7, 'u'),
#endif
	typeinfo("float", FLOAT, FLOAT, FLOAT_SIZE, 1, 'f'),
	typeinfo("double", DOUBLE, DOUBLE, DOUBLE_SIZE, 2, 'f'),
	typeinfo("long double", LDOUBLE, LDOUBLE, LDOUBLE_SIZE, 3, 'f'),
#ifdef DEBUG
	typeinfo("_Complex", NO_TSPEC, NO_TSPEC, 0, 0, ' '),
#else
	typeinfo(NULL, NO_TSPEC, NO_TSPEC, 0, 0, ' '),
#endif
	typeinfo("float _Complex", FCOMPLEX, FCOMPLEX,
	    FLOAT_SIZE * 2, 1, 'c'),
	typeinfo("double _Complex", DCOMPLEX, DCOMPLEX,
	    DOUBLE_SIZE * 2, 2, 'c'),
	typeinfo("long double _Complex", LCOMPLEX, LCOMPLEX,
	    LDOUBLE_SIZE * 2, 3, 'c'),
	typeinfo("void", VOID, VOID, 0, 0, ' '),
	typeinfo("struct", STRUCT, STRUCT, 0, 0, ' '),
	typeinfo("union", UNION, UNION, 0, 0, ' '),
	// Will become more complicated in C23, which allows to choose the
	// underlying type.
	typeinfo("enum", ENUM, ENUM, ENUM_SIZE, INT_RANK, 's'),
	// Same as 'unsigned long', which matches all supported platforms.
	typeinfo("pointer", PTR, PTR, PTR_SIZE, 5, 'p'),
	typeinfo("array", ARRAY, ARRAY, 0, 0, ' '),
	typeinfo("function", FUNC, FUNC, 0, 0, ' '),
};
#undef typeinfo
