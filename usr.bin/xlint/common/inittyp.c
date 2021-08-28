/*	$NetBSD: inittyp.c,v 1.26 2021/08/28 13:02:25 rillig Exp $	*/

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
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: inittyp.c,v 1.26 2021/08/28 13:02:25 rillig Exp $");
#endif

#include <limits.h>
#include <stdlib.h>

#if defined(IS_LINT1)
#include "lint1.h"
#else
#include "lint2.h"
#endif

/* various type information */
ttab_t	ttab[NTSPEC];

#define INT_RSIZE	(/*CONSTCOND*/INTPTR_TSPEC == LONG ? 3 : 4)

void
inittyp(void)
{
	size_t	i;
	static const struct {
		tspec_t	it_tspec;
		ttab_t	it_ttab;
	} ittab[NTSPEC] = {
#define typeinfo( \
	    tspec, signed_type, unsigned_type, \
	    size_in_bits, portable_size_in_bits, \
	    in, un, fl, ar, sc, co, name) \
	{ \
	    tspec, { \
		size_in_bits, portable_size_in_bits, \
		signed_type, unsigned_type, \
		(in) > 0, (un) > 0, (fl) > 0, (ar) > 0, (sc) > 0, (co) > 0, \
		name, \
	    } \
	}
		typeinfo(SIGNED, SIGNED, UNSIGN, 0, 0,
		    0, 0, 0, 0, 0, 0, "signed"),
		typeinfo(UNSIGN, SIGNED, UNSIGN, 0, 0,
		    0, 0, 0, 0, 0, 0, "unsigned"),
		typeinfo(BOOL, BOOL, BOOL, CHAR_SIZE, 1,
		    1, 1, 0, 1, 1, 0, "_Bool"),
		typeinfo(CHAR, SCHAR, UCHAR, CHAR_SIZE, 8,
		    1, /*CONSTCOND*/ TARG_CHAR_MIN == 0 ? 1 : 0,
		    /* */ 0, 1, 1, 0, "char"),
		typeinfo(SCHAR, SCHAR, UCHAR, CHAR_SIZE, 8,
		    1, 0, 0, 1, 1, 0, "signed char"),
		typeinfo(UCHAR, SCHAR, UCHAR, CHAR_SIZE, 8,
		    1, 1, 0, 1, 1, 0, "unsigned char"),
		typeinfo(SHORT, SHORT, USHORT, SHORT_SIZE, 16,
		    1, 0, 0, 1, 1, 0, "short"),
		typeinfo(USHORT, SHORT, USHORT, SHORT_SIZE, 16,
		    1, 1, 0, 1, 1, 0, "unsigned short"),
		typeinfo(INT, INT, UINT, INT_SIZE, INT_RSIZE * 8,
		    1, 0, 0, 1, 1, 0, "int"),
		typeinfo(UINT, INT, UINT, INT_SIZE, INT_RSIZE * 8,
		    1, 1, 0, 1, 1, 0, "unsigned int"),
		typeinfo(LONG, LONG, ULONG, LONG_SIZE, 32,
		    1, 0, 0, 1, 1, 0, "long"),
		typeinfo(ULONG, LONG, ULONG, LONG_SIZE, 32,
		    1, 1, 0, 1, 1, 0, "unsigned long"),
		typeinfo(QUAD, QUAD, UQUAD, QUAD_SIZE, 64,
		    1, 0, 0, 1, 1, 0, "long long"),
		typeinfo(UQUAD, QUAD, UQUAD, QUAD_SIZE, 64,
		    1, 1, 0, 1, 1, 0, "unsigned long long"),
#ifdef INT128_SIZE
		typeinfo(INT128, INT128, UINT128, INT128_SIZE, 128,
		    1, 0, 0, 1, 1, 0, "__int128_t"),
		typeinfo(UINT128, INT128, UINT128, INT128_SIZE, 128,
		    1, 1, 0, 1, 1, 0, "__uint128_t"),
#endif
		typeinfo(FLOAT, FLOAT, FLOAT, FLOAT_SIZE, 32,
		    0, 0, 1, 1, 1, 0, "float"),
		typeinfo(DOUBLE, DOUBLE, DOUBLE, DOUBLE_SIZE, 64,
		    0, 0, 1, 1, 1, 0, "double"),
		typeinfo(LDOUBLE, LDOUBLE, LDOUBLE, LDOUBLE_SIZE, 80,
		    0, 0, 1, 1, 1, 0, "long double"),
		typeinfo(FCOMPLEX, FCOMPLEX, FCOMPLEX, FLOAT_SIZE * 2, 32 * 2,
		    0, 0, 1, 1, 1, 1, "float _Complex"),
		typeinfo(DCOMPLEX, DCOMPLEX, DCOMPLEX, DOUBLE_SIZE * 2, 64 * 2,
		    0, 0, 1, 1, 1, 1, "double _Complex"),
		typeinfo(LCOMPLEX, LCOMPLEX, LCOMPLEX,
		    LDOUBLE_SIZE * 2, 80 * 2,
		    0, 0, 1, 1, 1, 1, "long double _Complex"),
		typeinfo(VOID, VOID, VOID, 0, 0,
		    0, 0, 0, 0, 0, 0, "void"),
		typeinfo(STRUCT, STRUCT, STRUCT, 0, 0,
		    0, 0, 0, 0, 0, 0, "struct"),
		typeinfo(UNION, UNION, UNION, 0, 0,
		    0, 0, 0, 0, 0, 0, "union"),
		typeinfo(ENUM, ENUM, ENUM, ENUM_SIZE, 24,
		    1, 0, 0, 1, 1, 0, "enum"),
		typeinfo(PTR, PTR, PTR, PTR_SIZE, 32,
		    0, 1, 0, 0, 1, 0, "pointer"),
		typeinfo(ARRAY, ARRAY, ARRAY, 0, 0,
		    0, 0, 0, 0, 0, 0, "array"),
		typeinfo(FUNC, FUNC, FUNC, 0, 0,
		    0, 0, 0, 0, 0, 0, "function"),
#undef typeinfo
	};

	for (i = 0; i < sizeof(ittab) / sizeof(ittab[0]); i++)
		ttab[ittab[i].it_tspec] = ittab[i].it_ttab;
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
