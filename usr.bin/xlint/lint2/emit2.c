/* $NetBSD: emit2.c,v 1.28 2022/05/20 21:18:55 rillig Exp $ */

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

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: emit2.c,v 1.28 2022/05/20 21:18:55 rillig Exp $");
#endif

#include "lint2.h"

static	void	outtype(type_t *);
static	void	outdef(hte_t *, sym_t *);
static	void	dumpname(hte_t *);
static	void	outfiles(void);

/*
 * Write type into the output buffer.
 */
static void
outtype(type_t *tp)
{
#ifdef INT128_SIZE
	static const char tt[NTSPEC] = "???BCCCSSIILLQQJJDDDVTTTPAF?XXX";
	static const char ss[NTSPEC] = "???  su u u u u us l sue   ?s l";
#else
	static const char tt[NTSPEC] = "???BCCCSSIILLQQDDDVTTTPAF?XXX";
	static const char ss[NTSPEC] = "???  su u u u us l sue   ?s l";
#endif
	int	na;
	tspec_t	ts;
	type_t	**ap;

	while (tp != NULL) {
		if ((ts = tp->t_tspec) == INT && tp->t_is_enum)
			ts = ENUM;
		if (!ch_isupper(tt[ts]))
			errx(1, "internal error: outtype(%d)", ts);
		if (tp->t_const)
			outchar('c');
		if (tp->t_volatile)
			outchar('v');
		if (ss[ts] != ' ')
			outchar(ss[ts]);
		if (ts == FUNC && tp->t_args != NULL && !tp->t_proto)
			outchar('f');
		else
			outchar(tt[ts]);

		if (ts == ARRAY) {
			outint(tp->t_dim);
		} else if (ts == ENUM || ts == STRUCT || ts == UNION) {
			if (tp->t_istag) {
				outint(1);
				outname(tp->t_tag->h_name);
			} else if (tp->t_istynam) {
				outint(2);
				outname(tp->t_tynam->h_name);
			} else if (tp->t_isuniqpos) {
				outint(3);
				outint(tp->t_uniqpos.p_line);
				outchar('.');
				outint(tp->t_uniqpos.p_file);
				outchar('.');
				outint(tp->t_uniqpos.p_uniq);
			} else
				errx(1, "internal error: outtype() 2");
		} else if (ts == FUNC && tp->t_args != NULL) {
			na = 0;
			for (ap = tp->t_args; *ap != NULL; ap++)
				na++;
			if (tp->t_vararg)
				na++;
			outint(na);
			for (ap = tp->t_args; *ap != NULL; ap++)
				outtype(*ap);
			if (tp->t_vararg)
				outchar('E');
		}
		tp = tp->t_subt;
	}
}

/*
 * Write a definition.
 */
static void
outdef(hte_t *hte, sym_t *sym)
{

	/* reset output buffer */
	outclr();

	/* line number in C source file */
	outint(0);

	/* this is a definition */
	outchar('d');

	/* index of file where symbol was defined and line number of def. */
	outint(0);
	outchar('.');
	outint(0);

	/* flags */
	if (sym->s_check_only_first_args) {
		outchar('v');
		outint(sym->s_check_num_args);
	}
	if (sym->s_scanflike) {
		outchar('S');
		outint(sym->s_scanflike_arg);
	}
	if (sym->s_printflike) {
		outchar('P');
		outint(sym->s_printflike_arg);
	}
	/* definition or tentative definition */
	outchar(sym->s_def == DEF ? 'd' : 't');
	if (TP(sym->s_type)->t_tspec == FUNC) {
		if (sym->s_function_has_return_value)
			outchar('r');
		if (sym->s_old_style_function)
			outchar('o');
	}
	outchar('u');			/* used (no warning if not used) */

	/* name */
	outname(hte->h_name);

	/* type */
	outtype(TP(sym->s_type));
}

/*
 * Write the first definition of a name into the lint library.
 */
static void
dumpname(hte_t *hte)
{
	sym_t	*sym, *def;

	/* static and undefined symbols are not written */
	if (hte->h_static || !hte->h_def)
		return;

	/*
	 * If there is a definition, write it. Otherwise write a tentative
	 * definition. This is necessary because more than one tentative
	 * definition is allowed (except with sflag).
	 */
	def = NULL;
	for (sym = hte->h_syms; sym != NULL; sym = sym->s_next) {
		if (sym->s_def == DEF) {
			def = sym;
			break;
		}
		if (sym->s_def == TDEF && def == NULL)
			def = sym;
	}
	if (def == NULL)
		errx(1, "internal error: dumpname() %s", hte->h_name);

	outdef(hte, def);
}

/*
 * Write a new lint library.
 */
void
outlib(const char *name)
{
	/* Open of output file and initialization of the output buffer */
	outopen(name);

	/* write name of lint library */
	outsrc(name);

	/* name of lint lib has index 0 */
	outclr();
	outint(0);
	outchar('s');
	outstrg(name);

	/*
	 * print the names of all files referenced by unnamed
	 * struct/union/enum declarations.
	 */
	outfiles();

	/* write all definitions with external linkage */
	symtab_forall_sorted(dumpname);

	/* close the output */
	outclose();
}

/*
 * Write out the name of a file referenced by a type.
 */
struct outflist {
	short		ofl_num;
	struct outflist *ofl_next;
};
static struct outflist *outflist;

int
addoutfile(short num)
{
	struct outflist *ofl, **pofl;
	int i;

	ofl = outflist;
	pofl = &outflist;
	i = 1;				/* library is 0 */

	while (ofl != NULL) {
		if (ofl->ofl_num == num)
			break;

		pofl = &ofl->ofl_next;
		ofl = ofl->ofl_next;
		i++;
	}

	if (ofl == NULL) {
		ofl = *pofl = xmalloc(sizeof(**pofl));
		ofl->ofl_num = num;
		ofl->ofl_next = NULL;
	}
	return i;
}

static void
outfiles(void)
{
	struct outflist *ofl;
	int i;

	for (ofl = outflist, i = 1; ofl != NULL; ofl = ofl->ofl_next, i++) {
		/* reset output buffer */
		outclr();

		outint(i);
		outchar('s');
		outstrg(fnames[ofl->ofl_num]);
	}
}
