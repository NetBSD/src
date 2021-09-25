/*	$NetBSD: args.c,v 1.25 2021/09/25 17:11:23 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 * All rights reserved.
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
 */

#if 0
static char sccsid[] = "@(#)args.c	8.1 (Berkeley) 6/6/93";
#endif

#include <sys/cdefs.h>
#if defined(__NetBSD__)
__RCSID("$NetBSD: args.c,v 1.25 2021/09/25 17:11:23 rillig Exp $");
#elif defined(__FreeBSD__)
__FBSDID("$FreeBSD: head/usr.bin/indent/args.c 336318 2018-07-15 21:04:21Z pstef $");
#endif

/*
 * Argument scanning and profile reading code.  Default parameters are set
 * here as well.
 */

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indent.h"

#define INDENT_VERSION	"2.0"

/* profile types */
#define	PRO_SPECIAL	1	/* special case */
#define	PRO_BOOL	2	/* boolean */
#define	PRO_INT		3	/* integer */

/* profile specials for booleans */
#define	ON		1	/* turn it on */
#define	OFF		0	/* turn it off */

/* profile specials for specials */
#define	IGN		1	/* ignore it */
#define	CLI		2	/* case label indent (float) */
#define	STDIN		3	/* use stdin */
#define	KEY		4	/* type (keyword) */

static void scan_profile(FILE *);

#define	KEY_FILE	5	/* only used for args */
#define VERSION		6	/* only used for args */

const char *option_source = "?";

void add_typedefs_from_file(const char *str);

#if __STDC_VERSION__ >= 201112L
#define assert_type(expr, type) _Generic((expr), type : (expr))
#else
#define assert_type(expr, type) (expr)
#endif
#define bool_option(name, value, var) \
	{name, PRO_BOOL, value, assert_type(&(var), ibool *)}
#define int_option(name, value, var) \
	{name, PRO_INT, value, assert_type(&(var), int *)}
#define special_option(name, value) \
	{name, PRO_SPECIAL, assert_type(value, int), NULL}

/*
 * N.B.: because of the way the table here is scanned, options whose names are
 * substrings of other options must occur later; that is, with -lp vs -l, -lp
 * must be first.
 */
const struct pro {
    const char *p_name;		/* name, e.g. -bl, -cli */
    int         p_type;		/* type (int, bool, special) */
    int         p_special;	/* depends on type */
    void        *p_obj;		/* the associated variable */
}           pro[] = {
    special_option("T", KEY),
    special_option("U", KEY_FILE),
    special_option("-version", VERSION),
    special_option("P", IGN),
    bool_option("bacc", ON, opt.blanklines_around_conditional_compilation),
    bool_option("badp", ON, opt.blanklines_after_declarations_at_proctop),
    bool_option("bad", ON, opt.blanklines_after_declarations),
    bool_option("bap", ON, opt.blanklines_after_procs),
    bool_option("bbb", ON, opt.blanklines_before_blockcomments),
    bool_option("bc", OFF, opt.leave_comma),
    bool_option("bl", OFF, opt.btype_2),
    bool_option("br", ON, opt.btype_2),
    bool_option("bs", ON, opt.Bill_Shannon),
    bool_option("cdb", ON, opt.comment_delimiter_on_blankline),
    int_option("cd", 0, opt.decl_comment_column),
    bool_option("ce", ON, opt.cuddle_else),
    int_option("ci", 0, opt.continuation_indent),
    special_option("cli", CLI),
    bool_option("cs", ON, opt.space_after_cast),
    int_option("c", 0, opt.comment_column),
    int_option("di", 0, opt.decl_indent),
    bool_option("dj", ON, opt.ljust_decl),
    int_option("d", 0, opt.unindent_displace),
    bool_option("eei", ON, opt.extra_expression_indent),
    bool_option("ei", ON, opt.else_if),
    bool_option("fbs", ON, opt.function_brace_split),
    bool_option("fc1", ON, opt.format_col1_comments),
    bool_option("fcb", ON, opt.format_block_comments),
    bool_option("ip", ON, opt.indent_parameters),
    int_option("i", 0, opt.indent_size),
    int_option("lc", 0, opt.block_comment_max_line_length),
    int_option("ldi", 0, opt.local_decl_indent),
    bool_option("lpl", ON, opt.lineup_to_parens_always),
    bool_option("lp", ON, opt.lineup_to_parens),
    int_option("l", 0, opt.max_line_length),
    bool_option("nbacc", OFF, opt.blanklines_around_conditional_compilation),
    bool_option("nbadp", OFF, opt.blanklines_after_declarations_at_proctop),
    bool_option("nbad", OFF, opt.blanklines_after_declarations),
    bool_option("nbap", OFF, opt.blanklines_after_procs),
    bool_option("nbbb", OFF, opt.blanklines_before_blockcomments),
    bool_option("nbc", ON, opt.leave_comma),
    bool_option("nbs", OFF, opt.Bill_Shannon),
    bool_option("ncdb", OFF, opt.comment_delimiter_on_blankline),
    bool_option("nce", OFF, opt.cuddle_else),
    bool_option("ncs", OFF, opt.space_after_cast),
    bool_option("ndj", OFF, opt.ljust_decl),
    bool_option("neei", OFF, opt.extra_expression_indent),
    bool_option("nei", OFF, opt.else_if),
    bool_option("nfbs", OFF, opt.function_brace_split),
    bool_option("nfc1", OFF, opt.format_col1_comments),
    bool_option("nfcb", OFF, opt.format_block_comments),
    bool_option("nip", OFF, opt.indent_parameters),
    bool_option("nlpl", OFF, opt.lineup_to_parens_always),
    bool_option("nlp", OFF, opt.lineup_to_parens),
    bool_option("npcs", OFF, opt.proc_calls_space),
    special_option("npro", IGN),
    bool_option("npsl", OFF, opt.procnames_start_line),
    bool_option("nsc", OFF, opt.star_comment_cont),
    bool_option("nsob", OFF, opt.swallow_optional_blanklines),
    bool_option("nut", OFF, opt.use_tabs),
    bool_option("nv", OFF, opt.verbose),
    bool_option("pcs", ON, opt.proc_calls_space),
    bool_option("psl", ON, opt.procnames_start_line),
    bool_option("sc", ON, opt.star_comment_cont),
    bool_option("sob", ON, opt.swallow_optional_blanklines),
    special_option("st", STDIN),
    bool_option("ta", ON, opt.auto_typedefs),
    int_option("ts", 0, opt.tabsize),
    bool_option("ut", ON, opt.use_tabs),
    bool_option("v", ON, opt.verbose),
    /* whew! */
    {0, 0, 0, 0}
};

/*
 * set_profile reads $HOME/.indent.pro and ./.indent.pro and handles arguments
 * given in these files.
 */
void
set_profile(const char *profile_name)
{
    FILE *f;
    char fname[PATH_MAX];
    static char prof[] = ".indent.pro";

    if (profile_name == NULL)
	snprintf(fname, sizeof(fname), "%s/%s", getenv("HOME"), prof);
    else
	snprintf(fname, sizeof(fname), "%s", profile_name + 2);
    if ((f = fopen(option_source = fname, "r")) != NULL) {
	scan_profile(f);
	(void) fclose(f);
    }
    if ((f = fopen(option_source = prof, "r")) != NULL) {
	scan_profile(f);
	(void) fclose(f);
    }
    option_source = "Command line";
}

static void
scan_profile(FILE *f)
{
    int		comment_index, i;
    char	*p;
    char        buf[BUFSIZ];

    for (;;) {
	p = buf;
	comment_index = 0;
	while ((i = getc(f)) != EOF) {
	    if (i == '*' && comment_index == 0 && p > buf && p[-1] == '/') {
		comment_index = (int)(p - buf);
		*p++ = i;
	    } else if (i == '/' && comment_index != 0 && p > buf && p[-1] == '*') {
		p = buf + comment_index - 1;
		comment_index = 0;
	    } else if (isspace((unsigned char)i)) {
		if (p > buf && comment_index == 0)
		    break;
	    } else {
		*p++ = i;
	    }
	}
	if (p != buf) {
	    *p++ = 0;
	    if (opt.verbose)
		printf("profile: %s\n", buf);
	    set_option(buf);
	} else if (i == EOF)
	    return;
    }
}

static const char *
eqin(const char *s1, const char *s2)
{
    while (*s1 != '\0') {
	if (*s1++ != *s2++)
	    return NULL;
    }
    return s2;
}

void
set_option(char *arg)
{
    const struct pro *p;
    const char	*param_start;

    arg++;			/* ignore leading "-" */
    for (p = pro; p->p_name != NULL; p++)
	if (*p->p_name == *arg && (param_start = eqin(p->p_name, arg)) != NULL)
	    goto found;
    errx(1, "%s: unknown parameter \"%s\"", option_source, arg - 1);
found:
    switch (p->p_type) {

    case PRO_SPECIAL:
	switch (p->p_special) {

	case IGN:
	    break;

	case CLI:
	    if (*param_start == 0)
		goto need_param;
	    opt.case_indent = atof(param_start);
	    break;

	case STDIN:
	    if (input == NULL)
		input = stdin;
	    if (output == NULL)
		output = stdout;
	    break;

	case KEY:
	    if (*param_start == 0)
		goto need_param;
	    add_typename(param_start);
	    break;

	case KEY_FILE:
	    if (*param_start == 0)
		goto need_param;
	    add_typedefs_from_file(param_start);
	    break;

	case VERSION:
	    printf("FreeBSD indent %s\n", INDENT_VERSION);
	    exit(0);
	    /*NOTREACHED*/

	default:
	    errx(1, "set_option: internal error: p_special %d", p->p_special);
	}
	break;

    case PRO_BOOL:
	if (p->p_special == OFF)
	    *(ibool *)p->p_obj = false;
	else
	    *(ibool *)p->p_obj = true;
	break;

    case PRO_INT:
	if (!isdigit((unsigned char)*param_start)) {
    need_param:
	    errx(1, "%s: ``%s'' requires a parameter", option_source, p->p_name);
	}
	*(int *)p->p_obj = atoi(param_start);
	break;

    default:
	errx(1, "set_option: internal error: p_type %d", p->p_type);
    }
}

void
add_typedefs_from_file(const char *str)
{
    FILE *file;
    char line[BUFSIZ];

    if ((file = fopen(str, "r")) == NULL) {
	fprintf(stderr, "indent: cannot open file %s\n", str);
	exit(1);
    }
    while ((fgets(line, BUFSIZ, file)) != NULL) {
	/* Remove trailing whitespace */
	line[strcspn(line, " \t\n\r")] = '\0';
	add_typename(line);
    }
    fclose(file);
}
