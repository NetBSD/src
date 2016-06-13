/*	$NetBSD: extern.h,v 1.29 2016/06/13 00:04:40 pgoyette Exp $	*/

/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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
 *	from: @(#)extern.h	8.3 (Berkeley) 4/16/94
 */

#include <sys/cdefs.h>

void	 brace_subst(char *, char **, char *, int *);
PLAN	*find_create(char ***);
int	 find_execute(PLAN *, char **);
PLAN	*find_formplan(char **);
int	 find_traverse(PLAN *, int (*)(PLAN *, void *), void *);
int	 f_expr(PLAN *, FTSENT *);
PLAN	*not_squish(PLAN *);
PLAN	*or_squish(PLAN *);
PLAN	*paren_squish(PLAN *);
int	 plan_cleanup(PLAN *, void *);
void	 printlong(char *, char *, struct stat *);
int	 queryuser(char **);
void	 show_path(int);

PLAN	*c_amin(char ***, int, char *);
PLAN	*c_anewer(char ***, int, char *);
PLAN	*c_asince(char ***, int, char *);
PLAN	*c_atime(char ***, int, char *);
PLAN	*c_cmin(char ***, int, char *);
PLAN	*c_cnewer(char ***, int, char *);
PLAN	*c_csince(char ***, int, char *);
PLAN	*c_ctime(char ***, int, char *);
PLAN	*c_delete(char ***, int, char *);
PLAN	*c_depth(char ***, int, char *);
PLAN	*c_empty(char ***, int, char *);
PLAN	*c_exec(char ***, int, char *);
PLAN	*c_execdir(char ***, int, char *);
PLAN	*c_exit(char ***, int, char *);
PLAN	*c_false(char ***, int, char *);
PLAN	*c_flags(char ***, int, char *);
PLAN	*c_follow(char ***, int, char *);
PLAN	*c_fprint(char ***, int, char *);
PLAN	*c_fstype(char ***, int, char *);
PLAN	*c_group(char ***, int, char *);
PLAN	*c_iname(char ***, int, char *);
PLAN	*c_inum(char ***, int, char *);
PLAN	*c_iregex(char ***, int, char *);
PLAN	*c_links(char ***, int, char *);
PLAN	*c_ls(char ***, int, char *);
PLAN	*c_maxdepth(char ***, int, char *);
PLAN	*c_mindepth(char ***, int, char *);
PLAN	*c_mmin(char ***, int, char *);
PLAN	*c_mtime(char ***, int, char *);
PLAN	*c_name(char ***, int, char *);
PLAN	*c_newer(char ***, int, char *);
PLAN	*c_nogroup(char ***, int, char *);
PLAN	*c_nouser(char ***, int, char *);
PLAN	*c_path(char ***, int, char *);
PLAN	*c_perm(char ***, int, char *);
PLAN	*c_print(char ***, int, char *);
PLAN	*c_print0(char ***, int, char *);
PLAN	*c_printx(char ***, int, char *);
PLAN	*c_prune(char ***, int, char *);
PLAN	*c_regex(char ***, int, char *);
PLAN	*c_since(char ***, int, char *);
PLAN	*c_size(char ***, int, char *);
PLAN	*c_type(char ***, int, char *);
PLAN	*c_user(char ***, int, char *);
PLAN	*c_xdev(char ***, int, char *);
PLAN	*c_openparen(char ***, int, char *);
PLAN	*c_closeparen(char ***, int, char *);
PLAN	*c_not(char ***, int, char *);
PLAN	*c_or(char ***, int, char *);
PLAN	*c_null(char ***, int, char *);

extern int ftsoptions, isdeprecated, isdepth, isoutput, issort, isxargs,
	regcomp_flags;
