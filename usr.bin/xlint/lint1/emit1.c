/* $NetBSD: emit1.c,v 1.96 2024/08/29 20:35:19 rillig Exp $ */

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
__RCSID("$NetBSD: emit1.c,v 1.96 2024/08/29 20:35:19 rillig Exp $");
#endif

#include <stdlib.h>

#include "lint1.h"

static void outtt(sym_t *, sym_t *);
static void outfstrg(const char *);

/*
 * Write type into the output file, encoded as follows:
 *	const			c
 *	volatile		v
 *	_Bool			B
 *	_Complex float		s X
 *	_Complex double		X
 *	_Complex long double	l X
 *	char			C
 *	signed char		s C
 *	unsigned char		u C
 *	short			S
 *	unsigned short		u S
 *	int			I
 *	unsigned int		u I
 *	long			L
 *	unsigned long		u L
 *	long long		Q
 *	unsigned long long	u Q
 *	float			s D
 *	double			D
 *	long double		l D
 *	void			V
 *	*			P
 *	[n]			A n
 *	()			F
 *	(void)			F 0
 *	(n parameters)		F n arg1 arg2 ... argn
 *	(n parameters, ...)	F n arg1 arg2 ... argn E
 *	enum tag		e T tag_or_typename
 *	struct tag		s T tag_or_typename
 *	union tag		u T tag_or_typename
 *
 *	tag_or_typename		0 (obsolete)		no tag or type name
 *				1 n tag			tagged type
 *				2 n typename		only typedef name
 *				3 line.file.uniq	anonymous types
 */
void
outtype(const type_t *tp)
{
	/* Available letters: ------GH--K-MNO--R--U-W-YZ */
#ifdef INT128_SIZE
	static const char tt[NTSPEC] = "???BCCCSSIILLQQJJDDD?XXXV?TTTPAF";
	static const char ss[NTSPEC] = "???  su u u u u us l?s l ?sue   ";
#else
	static const char tt[NTSPEC] = "???BCCCSSIILLQQDDD?XXXV?TTTPAF";
	static const char ss[NTSPEC] = "???  su u u u us l?s l ?sue   ";
#endif
	int na;
	tspec_t ts;

	while (tp != NULL) {
		if ((ts = tp->t_tspec) == INT && tp->t_is_enum)
			ts = ENUM;
		lint_assert(tt[ts] != '?' && ss[ts] != '?');
		if (tp->t_const)
			outchar('c');
		if (tp->t_volatile)
			outchar('v');
		if (ss[ts] != ' ')
			outchar(ss[ts]);
		outchar(tt[ts]);

		if (ts == ARRAY) {
			outint(tp->u.dimension);
		} else if (ts == ENUM) {
			outtt(tp->u.enumer->en_tag,
			    tp->u.enumer->en_first_typedef);
		} else if (is_struct_or_union(ts)) {
			outtt(tp->u.sou->sou_tag,
			    tp->u.sou->sou_first_typedef);
		} else if (ts == FUNC && tp->t_proto) {
			na = 0;
			for (const sym_t *param = tp->u.params;
			    param != NULL; param = param->s_next)
				na++;
			if (tp->t_vararg)
				na++;
			outint(na);
			for (const sym_t *param = tp->u.params;
			    param != NULL; param = param->s_next)
				outtype(param->s_type);
			if (tp->t_vararg)
				outchar('E');
		}
		tp = tp->t_subt;
	}
}

/*
 * write the name of a tag or typename
 *
 * if the tag is named, the name of the tag is written,
 * otherwise, if a typename exists which refers to this tag,
 * this typename is written
 */
static void
outtt(sym_t *tag, sym_t *tdef)
{

	/* 0 is no longer used. */

	if (tag->s_name != unnamed) {
		outint(1);
		outname(tag->s_name);
	} else if (tdef != NULL) {
		outint(2);
		outname(tdef->s_name);
	} else {
		outint(3);
		outint(tag->s_def_pos.p_line);
		outchar('.');
		outint(get_filename_id(tag->s_def_pos.p_file));
		outchar('.');
		outint(tag->s_def_pos.p_uniq);
	}
}

/*
 * write information about a globally declared/defined symbol
 * with storage class extern
 *
 * information about function definitions are written in outfdef(),
 * not here
 */
void
outsym(const sym_t *sym, scl_t sc, def_t def)
{

	/*
	 * Static function declarations must also be written to the output
	 * file. Compatibility of function declarations (for both static and
	 * extern functions) must be checked in lint2. Lint1 can't do this,
	 * especially not if functions are declared at block level before their
	 * first declaration at level 0.
	 */
	if (sc != EXTERN && !(sc == STATIC && sym->s_type->t_tspec == FUNC))
		return;
	if (ch_isdigit(sym->s_name[0]))	/* see mktempsym */
		return;

	outint(csrc_pos.p_line);
	outchar('d');		/* declaration */
	outint(get_filename_id(sym->s_def_pos.p_file));
	outchar('.');
	outint(sym->s_def_pos.p_line);

	/* flags */

	if (def == DEF)
		outchar('d');	/* defined */
	else if (def == TDEF)
		outchar('t');	/* tentative defined */
	else {
		lint_assert(def == DECL);
		outchar('e');	/* declared */
	}

	if (llibflg && def != DECL) {
		/*
		 * mark it as used so lint2 does not complain about unused
		 * symbols in libraries
		 */
		outchar('u');
	}

	if (sc == STATIC)
		outchar('s');

	outname(sym->s_name);

	if (sym->s_rename != NULL) {
		outchar('r');
		outname(sym->s_rename);
	}

	outtype(sym->s_type);
	outchar('\n');
}

/*
 * Write information about a function definition. This is also done for static
 * functions, to later check if they are called with proper argument types.
 */
void
outfdef(const sym_t *fsym, const pos_t *posp, bool rval, bool osdef,
	const sym_t *args)
{
	int narg;

	if (posp->p_file == csrc_pos.p_file) {
		outint(posp->p_line);
	} else {
		outint(csrc_pos.p_line);
	}
	outchar('d');		/* declaration */
	outint(get_filename_id(posp->p_file));
	outchar('.');
	outint(posp->p_line);

	/* both SCANFLIKE and PRINTFLIKE imply VARARGS */
	if (printflike_argnum != -1) {
		nvararg = printflike_argnum;
	} else if (scanflike_argnum != -1) {
		nvararg = scanflike_argnum;
	}

	if (nvararg != -1) {
		outchar('v');
		outint(nvararg);
	}
	if (scanflike_argnum != -1) {
		outchar('S');
		outint(scanflike_argnum);
	}
	if (printflike_argnum != -1) {
		outchar('P');
		outint(printflike_argnum);
	}
	nvararg = printflike_argnum = scanflike_argnum = -1;

	outchar('d');

	if (rval)
		outchar('r');	/* has return value */

	if (llibflg)
		/*
		 * mark it as used so lint2 does not complain about unused
		 * symbols in libraries
		 */
		outchar('u');

	if (osdef)
		outchar('o');	/* old-style function definition */

	if (fsym->s_inline)
		outchar('i');

	if (fsym->s_scl == STATIC)
		outchar('s');

	outname(fsym->s_name);

	if (fsym->s_rename != NULL) {
		outchar('r');
		outname(fsym->s_rename);
	}

	/* parameter types and return value */
	if (osdef) {
		narg = 0;
		for (const sym_t *arg = args; arg != NULL; arg = arg->s_next)
			narg++;
		outchar('f');
		outint(narg);
		for (const sym_t *arg = args; arg != NULL; arg = arg->s_next)
			outtype(arg->s_type);
		outtype(fsym->s_type->t_subt);
	} else {
		outtype(fsym->s_type);
	}
	outchar('\n');
}

/*
 * write out all information necessary for lint2 to check function
 * calls
 *
 * retval_used is set if the return value is used (assigned to a variable)
 * retval_discarded is set if the return value is neither used nor ignored
 * (that is, cast to void)
 */
void
outcall(const tnode_t *tn, bool retval_used, bool retval_discarded)
{
	outint(csrc_pos.p_line);
	outchar('c');		/* function call */
	outint(get_filename_id(curr_pos.p_file));
	outchar('.');
	outint(curr_pos.p_line);

	/*
	 * flags; 'u' and 'i' must be last to make sure a letter is between the
	 * numeric argument of a flag and the name of the function
	 */
	const function_call *call = tn->u.call;

	for (size_t i = 0, n = call->args_len; i < n; i++) {
		const tnode_t *arg = call->args[i];
		if (arg->tn_op == CON) {
			tspec_t t = arg->tn_type->t_tspec;
			if (is_integer(t)) {
				/*
				 * XXX it would probably be better to
				 * explicitly test the sign
				 */
				int64_t si = arg->u.value.u.integer;
				if (si == 0)
					/* zero constant */
					outchar('z');
				else if (!msb(si, t))
					/* positive if cast to signed */
					outchar('p');
				else
					/* negative if cast to signed */
					outchar('n');
				outint((int)i + 1);
			}
		} else if (arg->tn_op == ADDR &&
		    arg->u.ops.left->tn_op == STRING &&
		    arg->u.ops.left->u.str_literals->data != NULL) {
			buffer buf;
			buf_init(&buf);
			quoted_iterator it = { .end = 0 };
			while (quoted_next(arg->u.ops.left->u.str_literals, &it))
				buf_add_char(&buf, (char)it.value);

			/* string literal, write all format specifiers */
			outchar('s');
			outint((int)i + 1);
			outfstrg(buf.data);
			free(buf.data);
		}
	}
	outchar((char)(retval_discarded ? 'd' : retval_used ? 'u' : 'i'));

	outname(call->func->u.ops.left->u.sym->s_name);

	/* types of arguments */
	outchar('f');
	outint((int)call->args_len);
	for (size_t i = 0, n = call->args_len; i < n; i++)
		outtype(call->args[i]->tn_type);
	/* expected type of return value */
	outtype(tn->tn_type);
	outchar('\n');
}

/* write a character to the output file, quoted if necessary */
static void
outqchar(char c)
{

	if (ch_isprint(c) && c != '\\' && c != '"' && c != '\'') {
		outchar(c);
		return;
	}

	outchar('\\');
	switch (c) {
	case '\\':
		outchar('\\');
		break;
	case '"':
		outchar('"');
		break;
	case '\'':
		outchar('\'');
		break;
	case '\b':
		outchar('b');
		break;
	case '\t':
		outchar('t');
		break;
	case '\n':
		outchar('n');
		break;
	case '\f':
		outchar('f');
		break;
	case '\r':
		outchar('r');
		break;
	case '\v':
		outchar('v');
		break;
	case '\a':
		outchar('a');
		break;
	default:
		outchar((char)((((unsigned char)c >> 6) & 07) + '0'));
		outchar((char)((((unsigned char)c >> 3) & 07) + '0'));
		outchar((char)((c & 07) + '0'));
		break;
	}
}

/*
 * extracts potential format specifiers for printf() and scanf() and
 * writes them, enclosed in "" and quoted if necessary, to the output file
 */
static void
outfstrg(const char *cp)
{

	outchar('"');

	char c = *cp++;
	while (c != '\0') {

		if (c != '%') {
			c = *cp++;
			continue;
		}

		outchar('%');
		c = *cp++;

		/* flags for printf and scanf and *-fieldwidth for printf */
		while (c == '-' || c == '+' || c == ' ' ||
		    c == '#' || c == '0' || c == '*') {
			outchar(c);
			c = *cp++;
		}

		/* numeric field width */
		while (ch_isdigit(c)) {
			outchar(c);
			c = *cp++;
		}

		/* precision for printf */
		if (c == '.') {
			outchar(c);
			c = *cp++;
			if (c == '*') {
				outchar(c);
				c = *cp++;
			} else {
				while (ch_isdigit(c)) {
					outchar(c);
					c = *cp++;
				}
			}
		}

		/* h, l, L and q flags for printf and scanf */
		if (c == 'h' || c == 'l' || c == 'L' || c == 'q') {
			outchar(c);
			c = *cp++;
		}

		/*
		 * The last character. It is always written, so we can detect
		 * invalid format specifiers.
		 */
		if (c != '\0') {
			outqchar(c);
			char oc = c;
			c = *cp++;
			/*
			 * handle [ for scanf. [-] means that a minus sign was
			 * found at an undefined position.
			 */
			if (oc == '[') {
				if (c == '^')
					c = *cp++;
				if (c == ']')
					c = *cp++;
				bool first = true;
				while (c != '\0' && c != ']') {
					if (c == '-') {
						if (!first && *cp != ']')
							outchar(c);
					}
					first = false;
					c = *cp++;
				}
				if (c == ']') {
					outchar(c);
					c = *cp++;
				}
			}
		}
	}

	outchar('"');
}

/* writes a record if sym was used */
void
outusg(const sym_t *sym)
{
	if (ch_isdigit(sym->s_name[0]))	/* see mktempsym */
		return;

	outint(csrc_pos.p_line);
	outchar('u');		/* used */
	outint(get_filename_id(curr_pos.p_file));
	outchar('.');
	outint(curr_pos.p_line);
	outchar('x');		/* separate the two numbers */
	outname(sym->s_name);
	outchar('\n');
}
