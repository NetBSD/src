/*	$NetBSD: lcmd.h,v 1.5 2002/06/14 01:06:53 wiz Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Edward Wang at The University of California, Berkeley.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)lcmd.h	8.1 (Berkeley) 6/6/93
 */

#define LCMD_NARG 20			/* maximum number of arguments */

struct lcmd_tab {
	char *lc_name;
	int lc_minlen;
	void (*lc_func)(struct value *, struct value *);
	struct lcmd_arg *lc_arg;
};

struct lcmd_arg {
	char *arg_name;
	int arg_minlen;
	int arg_flags;
};

	/* arg_flags bits */
#define ARG_TYPE	0x0f		/* type of arg */
#define ARG_ANY		0x00		/* any type */
#define ARG_NUM		0x01		/* must be a number */
#define ARG_STR		0x02		/* must be a string */
#define ARG_LIST	0x10		/* this arg can be a list */

struct lcmd_tab	*lcmd_lookup(char *);
void	l_alias(struct value *, struct value *);
void	l_close(struct value *, struct value *);
void	l_cursormodes(struct value *, struct value *);
void	l_debug(struct value *, struct value *);
void	l_def_nline(struct value *, struct value *);
void	l_def_shell(struct value *, struct value *);
void	l_def_smooth(struct value *, struct value *);
void	l_echo(struct value *, struct value *);
void	l_escape(struct value *, struct value *);
void	l_foreground(struct value *, struct value *);
void	l_iostat(struct value *, struct value *);
void	l_label(struct value *, struct value *);
void	l_list(struct value *, struct value *);
void	l_select(struct value *, struct value *);
void	l_smooth(struct value *, struct value *);
void	l_source(struct value *, struct value *);
void	l_terse(struct value *, struct value *);
void	l_time(struct value *, struct value *);
void	l_unalias(struct value *, struct value *);
void	l_unset(struct value *, struct value *);
void	l_variable(struct value *, struct value *);
void	l_window(struct value *, struct value *);
void	l_write(struct value *, struct value *);
struct ww *vtowin(struct value *, struct ww *);
