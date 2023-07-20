/* $NetBSD: externs2.h,v 1.21 2023/07/13 08:40:38 rillig Exp $ */

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

/*
 * main2.c
 */
extern	bool	Cflag;
extern	bool	Fflag;
extern	bool	Hflag;
extern	bool	hflag;
extern	bool	sflag;
extern	bool	tflag;
extern	bool	uflag;
extern	bool	xflag;
extern	const char *libname;

/*
 * hash.c
 */
hte_t**	htab_new(void);
hte_t	*hash_search(hte_t **, const char *, bool);
void	symtab_init(void);
void	symtab_forall(void (*)(hte_t *));
void	symtab_forall_sorted(void (*)(hte_t *));
void	hash_free(hte_t **);

#define	htab_search(a, b)	hash_search(NULL, (a), (b))

/*
 * read.c
 */
extern	const	char **fnames;
extern	type_t	**tlst;

void	readfile(const char *);
void	mkstatic(hte_t *);

/*
 * chk.c
 */
void	mark_main_as_used(void);
void	check_name(const hte_t *);

/*
 * msg.c
 */
void	msg(int, ...);
const	char *mkpos(const pos_t *);

/*
 * emit2.c
 */
void	outlib(const char *);
int	addoutfile(short);
