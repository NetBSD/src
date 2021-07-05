/* $NetBSD: emit1.c,v 1.45 2021/07/05 19:39:12 rillig Exp $ */

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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: emit1.c,v 1.45 2021/07/05 19:39:12 rillig Exp $");
#endif

#include "lint1.h"

static	void	outtt(sym_t *, sym_t *);
static	void	outfstrg(strg_t *);

/*
 * Write type into the output buffer.
 * The type is written as a sequence of substrings, each of which describes a
 * node of type type_t
 * a node is encoded as follows:
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
 *	(n parameters, ...)	F n arg1 arg2 ... argn-1 E
 *	enum tag		e T tag_or_typename
 *	struct tag		s T tag_or_typename
 *	union tag		u T tag_or_typename
 *
 *	tag_or_typename		0 (obsolete)		no tag or type name
 *				1 n tag			tagged type
 *				2 n typename		only type name
 *				3 line.file.uniq	anonymous types
 *
 * spaces are only for better readability
 * additionally it is possible to prepend the characters 'c' (for const)
 * and 'v' (for volatile)
 */
void
outtype(const type_t *tp)
{
	int	t, s, na;
	sym_t	*arg;
	tspec_t	ts;

	while (tp != NULL) {
		if ((ts = tp->t_tspec) == INT && tp->t_is_enum)
			ts = ENUM;
		switch (ts) {
		case BOOL:	t = 'B';	s = '\0';	break;
		case CHAR:	t = 'C';	s = '\0';	break;
		case SCHAR:	t = 'C';	s = 's';	break;
		case UCHAR:	t = 'C';	s = 'u';	break;
		case SHORT:	t = 'S';	s = '\0';	break;
		case USHORT:	t = 'S';	s = 'u';	break;
		case INT:	t = 'I';	s = '\0';	break;
		case UINT:	t = 'I';	s = 'u';	break;
		case LONG:	t = 'L';	s = '\0';	break;
		case ULONG:	t = 'L';	s = 'u';	break;
		case QUAD:	t = 'Q';	s = '\0';	break;
		case UQUAD:	t = 'Q';	s = 'u';	break;
		case FLOAT:	t = 'D';	s = 's';	break;
		case DOUBLE:	t = 'D';	s = '\0';	break;
		case LDOUBLE:	t = 'D';	s = 'l';	break;
		case VOID:	t = 'V';	s = '\0';	break;
		case PTR:	t = 'P';	s = '\0';	break;
		case ARRAY:	t = 'A';	s = '\0';	break;
		case FUNC:	t = 'F';	s = '\0';	break;
		case ENUM:	t = 'T';	s = 'e';	break;
		case STRUCT:	t = 'T';	s = 's';	break;
		case UNION:	t = 'T';	s = 'u';	break;
		case FCOMPLEX:	t = 'X';	s = 's';	break;
		case DCOMPLEX:	t = 'X';	s = '\0';	break;
		case LCOMPLEX:	t = 'X';	s = 'l';	break;
		default:
			lint_assert(/*CONSTCOND*/false);
		}
		if (tp->t_const)
			outchar('c');
		if (tp->t_volatile)
			outchar('v');
		if (s != '\0')
			outchar(s);
		outchar(t);
		if (ts == ARRAY) {
			outint(tp->t_dim);
		} else if (ts == ENUM) {
			outtt(tp->t_enum->en_tag, tp->t_enum->en_first_typedef);
		} else if (ts == STRUCT || ts == UNION) {
			outtt(tp->t_str->sou_tag, tp->t_str->sou_first_typedef);
		} else if (ts == FUNC && tp->t_proto) {
			na = 0;
			for (arg = tp->t_args; arg != NULL; arg = arg->s_next)
				na++;
			if (tp->t_vararg)
				na++;
			outint(na);
			for (arg = tp->t_args; arg != NULL; arg = arg->s_next)
				outtype(arg->s_type);
			if (tp->t_vararg)
				outchar('E');
		}
		tp = tp->t_subt;
	}
}

/*
 * type to string
 * used for debugging output
 *
 * it uses its own output buffer for conversion
 */
const char *
ttos(const type_t *tp)
{
	static	ob_t	tob;
	ob_t	tmp;

	if (tob.o_buf == NULL) {
		tob.o_len = 64;
		tob.o_buf = tob.o_next = xmalloc(tob.o_len);
		tob.o_end = tob.o_buf + tob.o_len;
	}

	tmp = ob;
	ob = tob;
	ob.o_next = ob.o_buf;
	outtype(tp);
	outchar('\0');
	tob = ob;
	ob = tmp;

	return tob.o_buf;
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
	 * file. Compatibility of function declarations (for both static
	 * and extern functions) must be checked in lint2. Lint1 can't do
	 * this, especially not if functions are declared at block level
	 * before their first declaration at level 0.
	 */
	if (sc != EXTERN && !(sc == STATIC && sym->s_type->t_tspec == FUNC))
		return;

	/* reset buffer */
	outclr();

	/*
	 * line number of .c source, 'd' for declaration, Id of current
	 * source (.c or .h), and line in current source.
	 */
	outint(csrc_pos.p_line);
	outchar('d');
	outint(get_filename_id(sym->s_def_pos.p_file));
	outchar('.');
	outint(sym->s_def_pos.p_line);

	/* flags */

	switch (def) {
	case DEF:
		/* defined */
		outchar('d');
		break;
	case TDEF:
		/* tentative defined */
		outchar('t');
		break;
	case DECL:
		/* declared */
		outchar('e');
		break;
	default:
		lint_assert(/*CONSTCOND*/false);
	}
	if (llibflg && def != DECL) {
		/*
		 * mark it as used so we get no warnings from lint2 about
		 * unused symbols in libraries.
		 */
		outchar('u');
	}

	if (sc == STATIC)
		outchar('s');

	/* name of the symbol */
	outname(sym->s_name);

	/* renamed name of symbol, if necessary */
	if (sym->s_rename != NULL) {
		outchar('r');
		outname(sym->s_rename);
	}

	/* type of the symbol */
	outtype(sym->s_type);
}

/*
 * write information about function definition
 *
 * this is also done for static functions so we are able to check if
 * they are called with proper argument types
 */
void
outfdef(const sym_t *fsym, const pos_t *posp, bool rval, bool osdef,
	const sym_t *args)
{
	int narg;
	const sym_t *arg;

	/* reset the buffer */
	outclr();

	/*
	 * line number of .c source, 'd' for declaration, Id of current
	 * source (.c or .h), and line in current source
	 *
	 * we are already at the end of the function. If we are in the
	 * .c source, posp->p_line is correct, otherwise csrc_pos.p_line
	 * (for functions defined in header files).
	 */
	if (posp->p_file == csrc_pos.p_file) {
		outint(posp->p_line);
	} else {
		outint(csrc_pos.p_line);
	}
	outchar('d');
	outint(get_filename_id(posp->p_file));
	outchar('.');
	outint(posp->p_line);

	/* flags */

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
		/* has return value */
		outchar('r');

	if (llibflg)
		/*
		 * mark it as used so lint2 does not complain about
		 * unused symbols in libraries
		 */
		outchar('u');

	if (osdef)
		/* old style function definition */
		outchar('o');

	if (fsym->s_inline)
		outchar('i');

	if (fsym->s_scl == STATIC)
		outchar('s');

	/* name of function */
	outname(fsym->s_name);

	/* renamed name of function, if necessary */
	if (fsym->s_rename != NULL) {
		outchar('r');
		outname(fsym->s_rename);
	}

	/* argument types and return value */
	if (osdef) {
		narg = 0;
		for (arg = args; arg != NULL; arg = arg->s_next)
			narg++;
		outchar('f');
		outint(narg);
		for (arg = args; arg != NULL; arg = arg->s_next)
			outtype(arg->s_type);
		outtype(fsym->s_type->t_subt);
	} else {
		outtype(fsym->s_type);
	}
}

/*
 * write out all information necessary for lint2 to check function
 * calls
 *
 * rvused is set if the return value is used (assigned to a variable)
 * rvdisc is set if the return value is not used and not ignored
 * (casted to void)
 */
void
outcall(const tnode_t *tn, bool rvused, bool rvdisc)
{
	tnode_t	*args, *arg;
	int	narg, n, i;
	int64_t	q;
	tspec_t	t;

	/* reset buffer */
	outclr();

	/*
	 * line number of .c source, 'c' for function call, Id of current
	 * source (.c or .h), and line in current source
	 */
	outint(csrc_pos.p_line);
	outchar('c');
	outint(get_filename_id(curr_pos.p_file));
	outchar('.');
	outint(curr_pos.p_line);

	/*
	 * flags; 'u' and 'i' must be last to make sure a letter
	 * is between the numeric argument of a flag and the name of
	 * the function
	 */
	narg = 0;
	args = tn->tn_right;
	for (arg = args; arg != NULL; arg = arg->tn_right)
		narg++;
	/* information about arguments */
	for (n = 1; n <= narg; n++) {
		/* the last argument is the top one in the tree */
		for (i = narg, arg = args; i > n; i--, arg = arg->tn_right)
			continue;
		arg = arg->tn_left;
		if (arg->tn_op == CON) {
			if (is_integer(t = arg->tn_type->t_tspec)) {
				/*
				 * XXX it would probably be better to
				 * explicitly test the sign
				 */
				if ((q = arg->tn_val->v_quad) == 0) {
					/* zero constant */
					outchar('z');
				} else if (msb(q, t, 0) == 0) {
					/* positive if casted to signed */
					outchar('p');
				} else {
					/* negative if casted to signed */
					outchar('n');
				}
				outint(n);
			}
		} else if (arg->tn_op == ADDR &&
			   arg->tn_left->tn_op == STRING &&
			   arg->tn_left->tn_string->st_tspec == CHAR) {
			/* constant string, write all format specifiers */
			outchar('s');
			outint(n);
			outfstrg(arg->tn_left->tn_string);
		}

	}
	/* return value discarded/used/ignored */
	outchar(rvdisc ? 'd' : (rvused ? 'u' : 'i'));

	/* name of the called function */
	outname(tn->tn_left->tn_left->tn_sym->s_name);

	/* types of arguments */
	outchar('f');
	outint(narg);
	for (n = 1; n <= narg; n++) {
		/* the last argument is the top one in the tree */
		for (i = narg, arg = args; i > n; i--, arg = arg->tn_right)
			continue;
		outtype(arg->tn_left->tn_type);
	}
	/* expected type of return value */
	outtype(tn->tn_type);
}

/*
 * extracts potential format specifiers for printf() and scanf() and
 * writes them, enclosed in "" and quoted if necessary, to the output buffer
 */
static void
outfstrg(strg_t *strg)
{
	unsigned char c, oc;
	bool	first;
	u_char	*cp;

	lint_assert(strg->st_tspec == CHAR);

	cp = strg->st_cp;

	outchar('"');

	c = *cp++;

	while (c != '\0') {

		if (c != '%') {
			c = *cp++;
			continue;
		}

		outqchar('%');
		c = *cp++;

		/* flags for printf and scanf and *-fieldwidth for printf */
		while (c != '\0' && (c == '-' || c == '+' || c == ' ' ||
				     c == '#' || c == '0' || c == '*')) {
			outqchar(c);
			c = *cp++;
		}

		/* numeric field width */
		while (c != '\0' && ch_isdigit((char)c)) {
			outqchar(c);
			c = *cp++;
		}

		/* precision for printf */
		if (c == '.') {
			outqchar(c);
			if ((c = *cp++) == '*') {
				outqchar(c);
				c = *cp++;
			} else {
				while (c != '\0' && ch_isdigit((char)c)) {
					outqchar(c);
					c = *cp++;
				}
			}
		}

		/* h, l, L and q flags fpr printf and scanf */
		if (c == 'h' || c == 'l' || c == 'L' || c == 'q') {
			outqchar(c);
			c = *cp++;
		}

		/*
		 * The last character. It is always written so we can detect
		 * invalid format specifiers.
		 */
		if (c != '\0') {
			outqchar(c);
			oc = c;
			c = *cp++;
			/*
			 * handle [ for scanf. [-] means that a minus sign
			 * was found at an undefined position.
			 */
			if (oc == '[') {
				if (c == '^')
					c = *cp++;
				if (c == ']')
					c = *cp++;
				first = true;
				while (c != '\0' && c != ']') {
					if (c == '-') {
						if (!first && *cp != ']')
							outqchar(c);
					}
					first = false;
					c = *cp++;
				}
				if (c == ']') {
					outqchar(c);
					c = *cp++;
				}
			}
		}

	}

	outchar('"');
}

/*
 * writes a record if sym was used
 */
void
outusg(const sym_t *sym)
{
	/* reset buffer */
	outclr();

	/*
	 * line number of .c source, 'u' for used, Id of current
	 * source (.c or .h), and line in current source
	 */
	outint(csrc_pos.p_line);
	outchar('u');
	outint(get_filename_id(curr_pos.p_file));
	outchar('.');
	outint(curr_pos.p_line);

	/* necessary to delimit both numbers */
	outchar('x');

	outname(sym->s_name);
}
