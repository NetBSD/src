/*	$NetBSD: lint.h,v 1.42 2023/07/13 08:40:38 rillig Exp $	*/

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
#else
#define HAVE_DECL_SYS_SIGNAME 1
#endif

#include <sys/types.h>
#include <ctype.h>
#include <err.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "param.h"

#if defined(IS_LINT1) || defined(IS_LINT2)
/*
 * Type specifiers, used in type structures (type_t) and elsewhere.
 */
typedef enum {
	NO_TSPEC = 0,
	SIGNED,		/* keyword "signed", only used in the parser */
	UNSIGN,		/* keyword "unsigned", only used in the parser */
	BOOL,		/* _Bool */
	CHAR,		/* char */
	SCHAR,		/* signed char */
	UCHAR,		/* unsigned char */
	SHORT,		/* (signed) short */
	USHORT,		/* unsigned short */
	INT,		/* (signed) int */
	UINT,		/* unsigned int */
	LONG,		/* (signed) long */
	ULONG,		/* unsigned long */
	LLONG,		/* (signed) long long */
	ULLONG,		/* unsigned long long */
#ifdef INT128_SIZE
	INT128,		/* (signed) __int128_t */
	UINT128,	/* __uint128_t */
#endif
	FLOAT,		/* float */
	DOUBLE,		/* double or, with tflag, long float */
	LDOUBLE,	/* long double */
	COMPLEX,	/* keyword "_Complex", only used in the parser */
	FCOMPLEX,	/* float _Complex */
	DCOMPLEX,	/* double _Complex */
	LCOMPLEX,	/* long double _Complex */
	VOID,		/* void */
	STRUCT,		/* structure tag */
	UNION,		/* union tag */
	ENUM,		/* enum tag */
	PTR,		/* pointer */
	ARRAY,		/* array */
	FUNC,		/* function */
#define NTSPEC ((int)FUNC + 1)
} tspec_t;


/*
 * size of types, name and classification
 */
typedef	struct {
#ifdef IS_LINT1
	unsigned int tt_size_in_bits;
	enum rank_kind {
		RK_NONE,
		RK_INTEGER,
		RK_FLOATING,
		RK_COMPLEX,
	} tt_rank_kind;
	unsigned int tt_rank_value;	/* relative size of the type; depends
					 * on pflag and PTRDIFF_TSPEC */
#endif
	tspec_t	tt_signed_counterpart;
	tspec_t	tt_unsigned_counterpart;
	bool	tt_is_integer:1;	/* integer type */
#ifdef IS_LINT1
	bool	tt_is_uinteger:1;	/* unsigned integer type */
	bool	tt_is_floating:1;	/* floating point type */
	bool	tt_is_arithmetic:1;	/* arithmetic type */
	bool	tt_is_scalar:1;		/* scalar type */
	bool	tt_is_complex:1;	/* complex type */
#endif
	const char *tt_name;		/* name of the type */
} ttab_t;

extern	ttab_t	ttab[];

static inline const ttab_t *
type_properties(tspec_t t) {
	return ttab + t;
}

#define size_in_bits(t)		(type_properties(t)->tt_size_in_bits)
#define signed_type(t)		(type_properties(t)->tt_signed_counterpart)
#define unsigned_type(t)	(type_properties(t)->tt_unsigned_counterpart)
#define is_integer(t)		(type_properties(t)->tt_is_integer)
#define is_uinteger(t)		(type_properties(t)->tt_is_uinteger)
#define is_floating(t)		(type_properties(t)->tt_is_floating)
#define is_arithmetic(t)	(type_properties(t)->tt_is_arithmetic)
#define is_complex(t)		(type_properties(t)->tt_is_complex)
#define is_scalar(t)		(type_properties(t)->tt_is_scalar)


typedef	enum {
	NODECL,			/* not declared until now */
	DECL,			/* declared */
	TDEF,			/* tentative defined */
	DEF			/* defined */
} def_t;

/* Some data used for the output buffer. */
typedef	struct	ob {
	char	*o_buf;		/* buffer */
	char	*o_end;		/* first byte after buffer */
	size_t	o_len;		/* length of buffer */
	char	*o_next;	/* next free byte in buffer */
} ob_t;

#if defined(IS_LINT1)
typedef struct lint1_type type_t;
#else
typedef struct lint2_type type_t;
#endif
#endif

#include "externs.h"

static inline bool
ch_isalnum(char ch) { return isalnum((unsigned char)ch) != 0; }
static inline bool
ch_isdigit(char ch) { return isdigit((unsigned char)ch) != 0; }
static inline bool
ch_isprint(char ch) { return isprint((unsigned char)ch) != 0; }
static inline bool
ch_isspace(char ch) { return isspace((unsigned char)ch) != 0; }
static inline bool
ch_isupper(char ch) { return isupper((unsigned char)ch) != 0; }
