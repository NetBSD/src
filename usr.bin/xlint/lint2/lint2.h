/* $NetBSD: lint2.h,v 1.22 2022/02/07 21:57:47 rillig Exp $ */

/*
 * Copyright (c) 1996 Christopher G. Demetriou.  All Rights Reserved.
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

#include "lint.h"

/*
 * Types are described by structures of type type_t.
 */
struct lint2_type {
	tspec_t	t_tspec;	/* type specifier */
	bool	t_const:1;	/* constant */
	bool	t_volatile:1;	/* volatile */
	bool	t_vararg:1;	/* function has variable number of arguments */
	bool	t_is_enum:1;
	bool	t_proto:1;	/* this is a prototype */
	bool	t_istag:1;	/* tag with _t_tag valid */
	bool	t_istynam:1;	/* tag with _t_tynam valid */
	bool	t_isuniqpos:1;	/* tag with _t_uniqpos valid */
	union {
		int	_t_dim;		/* if the type is an ARRAY than this
					   is the dimension of the array. */
		struct	hte *_t_tag;	/* hash table entry of tag if
					   t_is_enum, STRUCT or UNION */
		struct	hte *_t_tynam;	/* hash table entry of typename if
					   t_is_enum, STRUCT or UNION */
		struct {
			int p_line;
			short p_file;
			int p_uniq;
		} _t_uniqpos;		/* unique position, for untagged
					   untyped STRUCTs, UNIONS, and ENUMs,
					   if t_isuniqpos */
		struct	lint2_type **_t_args; /* list of argument types if
					   this is a prototype */
	} t_u;
	struct	lint2_type *t_subt;	/* element type (if ARRAY),
					   return type (if FUNC),
					   target type (if PTR) */
};

#define	t_dim		t_u._t_dim
#define	t_tag		t_u._t_tag
#define	t_tynam		t_u._t_tynam
#define	t_uniqpos	t_u._t_uniqpos
#define	t_args		t_u._t_args

/*
 * argument information
 *
 * Such a structure is created for each argument of a function call
 * which is an integer constant or a constant string.
 */
typedef	struct arginf {
	int	a_num;		/* # of argument (1..) */
	bool	a_zero:1;	/* argument is 0 */
	bool	a_pcon:1;	/* msb of argument is not set */
	bool	a_ncon:1;	/* msb of argument is set */
	bool	a_fmt:1;	/* a_fstrg points to format string */
	char	*a_fstrg;	/* format string */
	struct	arginf *a_next;	/* information for next const. argument */
} arginf_t;

/*
 * Keeps information about position in source file.
 */
typedef	struct {
	unsigned short p_src;	/* index of name of translation unit
				   (the name which was specified at the
				   command line) */
	unsigned short p_line;	/* line number in p_src */
	unsigned short p_isrc;	/* index of (included) file */
	unsigned short p_iline;	/* line number in p_iline */
} pos_t;

/* Used for definitions and declarations. */
typedef	struct sym {
	struct {
		pos_t	s_pos;		/* pos of def./decl. */
#if !defined(lint) && !defined(DEBUG)
		unsigned char s_def;	/* DECL, TDEF or DEF */
#else
		def_t	s_def;
#endif
		bool	s_function_has_return_value:1;
		bool	s_inline:1;
		bool	s_old_style_function:1;
		bool	s_static:1;
		bool	s_check_only_first_args:1;
		bool	s_printflike:1;
		bool	s_scanflike:1;
		unsigned short s_type;
		/* XXX: gap of 4 bytes on LP64 platforms */
		struct	sym *s_next;	/* next symbol with same name */
	} s_s;
	/*
	 * To save memory, the remaining members are only allocated if one of
	 * s_check_only_first_args, s_printflike and s_scanflike is set.
	 */
	short	s_check_num_args;	/* if s_check_only_first_args */
	short	s_printflike_arg;	/* if s_printflike */
	short	s_scanflike_arg;	/* if s_scanflike */
} sym_t;

#define s_pos		s_s.s_pos
#define s_function_has_return_value s_s.s_function_has_return_value
#define s_old_style_function s_s.s_old_style_function
#define s_inline	s_s.s_inline
#define s_static	s_s.s_static
#define s_def		s_s.s_def
#define s_check_only_first_args	s_s.s_check_only_first_args
#define s_printflike	s_s.s_printflike
#define s_scanflike	s_s.s_scanflike
#define s_type		s_s.s_type
#define s_next		s_s.s_next

/*
 * Used to store information about function calls.
 */
typedef	struct fcall {
	pos_t	f_pos;		/* position of call */
	bool	f_rused:1;	/* return value used */
	bool	f_rdisc:1;	/* return value discarded (casted to void) */
	unsigned short f_type;	/* types of expected return value and args */
	arginf_t *f_args;	/* information about constant arguments */
	struct	fcall *f_next;	/* next call of same function */
} fcall_t;

/*
 * Used to store information about usage of symbols other
 * than for function calls.
 */
typedef	struct usym {
	pos_t	u_pos;		/* position */
	struct	usym *u_next;	/* next usage */
} usym_t;

/*
 * hash table entry
 */
typedef	struct hte {
	const	char *h_name;	/* name */
	bool	h_used:1;	/* symbol is used */
	bool	h_def:1;	/* symbol is defined */
	bool	h_static:1;	/* static symbol */
	sym_t	*h_syms;	/* declarations and definitions */
	sym_t	**h_lsym;	/* points to s_next of last decl./def. */
	fcall_t	*h_calls;	/* function calls */
	fcall_t	**h_lcall;	/* points to f_next of last call */
	usym_t	*h_usyms;	/* usage info */
	usym_t	**h_lusym;	/* points to u_next of last usage info */
	struct	hte *h_link;	/* next hte with same hash function */
	struct  hte *h_hte;	/* pointer to other htes (for renames) */
} hte_t;

#include "externs2.h"

/* maps type indices into pointers to type structs */
static inline type_t *
TP(unsigned short type_id) {
	/* force sequence point for newly parsed type_id */
	return tlst[type_id];
}
