/*	$NetBSD: inittyp.c,v 1.35 2023/06/09 13:03:49 rillig Exp $	*/

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
 *      This product includes software developed by Jochen Pohl for
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
__RCSID("$NetBSD: inittyp.c,v 1.35 2023/06/09 13:03:49 rillig Exp $");
#endif

#if defined(IS_LINT1)
#include "lint1.h"
#else
#include "lint2.h"
#endif

#define INT_RSIZE	(/*CONSTCOND*/INTPTR_TSPEC == LONG ? 3 : 4)

#ifdef IS_LINT1
#define typeinfo( \
	    name, signed_type, unsigned_type, \
	    size_in_bits, portable_size_in_bits, \
	    c) \
	{ /*CONSTCOND*/ \
		size_in_bits, portable_size_in_bits, \
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
#define typeinfo( \
	    name, signed_type, unsigned_type, \
	    size_in_bits, portable_size_in_bits, \
	    c) \
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
	typeinfo("char", SCHAR, UCHAR, CHAR_SIZE, 8,
	    TARG_CHAR_MIN == 0 ? 'u' : 's'),
	typeinfo("signed char", SCHAR, UCHAR, CHAR_SIZE, 8, 's'),
	typeinfo("unsigned char", SCHAR, UCHAR, CHAR_SIZE, 8, 'u'),
	typeinfo("short", SHORT, USHORT, SHORT_SIZE, 16, 's'),
	typeinfo("unsigned short", SHORT, USHORT, SHORT_SIZE, 16, 'u'),
	typeinfo("int", INT, UINT, INT_SIZE, INT_RSIZE * 8, 's'),
	typeinfo("unsigned int", INT, UINT, INT_SIZE, INT_RSIZE * 8, 'u'),
	typeinfo("long", LONG, ULONG, LONG_SIZE, 32, 's'),
	typeinfo("unsigned long", LONG, ULONG, LONG_SIZE, 32, 'u'),
	typeinfo("long long", QUAD, UQUAD, QUAD_SIZE, 64, 's'),
	typeinfo("unsigned long long", QUAD, UQUAD, QUAD_SIZE, 64, 'u'),
#ifdef INT128_SIZE
	typeinfo("__int128_t", INT128, UINT128, INT128_SIZE, 128, 's'),
	typeinfo("__uint128_t", INT128, UINT128, INT128_SIZE, 128, 'u'),
#endif
	typeinfo("float", FLOAT, FLOAT, FLOAT_SIZE, 32, 'f'),
	typeinfo("double", DOUBLE, DOUBLE, DOUBLE_SIZE, 64, 'f'),
	typeinfo("long double", LDOUBLE, LDOUBLE, LDOUBLE_SIZE, 80, 'f'),
	typeinfo("void", VOID, VOID, 0, 0, ' '),
	typeinfo("struct", STRUCT, STRUCT, 0, 0, ' '),
	typeinfo("union", UNION, UNION, 0, 0, ' '),
	typeinfo("enum", ENUM, ENUM, ENUM_SIZE, 24, 's'),
	typeinfo("pointer", PTR, PTR, PTR_SIZE, 32, 'p'),
	typeinfo("array", ARRAY, ARRAY, 0, 0, ' '),
	typeinfo("function", FUNC, FUNC, 0, 0, ' '),
#ifdef DEBUG
	typeinfo("_Complex", NO_TSPEC, NO_TSPEC, 0, 0, ' '),
#else
	typeinfo(NULL, NO_TSPEC, NO_TSPEC, 0, 0, ' '),
#endif
	typeinfo("float _Complex", FCOMPLEX, FCOMPLEX,
	    FLOAT_SIZE * 2, 32 * 2, 'c'),
	typeinfo("double _Complex", DCOMPLEX, DCOMPLEX,
	    DOUBLE_SIZE * 2, 64 * 2, 'c'),
	typeinfo("long double _Complex", LCOMPLEX, LCOMPLEX,
	    LDOUBLE_SIZE * 2, 80 * 2, 'c'),
};
#undef typeinfo

#ifdef IS_LINT1
void
inittyp(void)
{
	size_t i;

	if (!pflag) {
		for (i = 0; i < NTSPEC; i++)
			ttab[i].tt_portable_size_in_bits =
			    ttab[i].tt_size_in_bits;
	}

	if (Tflag) {
		ttab[BOOL].tt_is_integer = false;
		ttab[BOOL].tt_is_uinteger = false;
		ttab[BOOL].tt_is_arithmetic = false;
	}
}
#endif
