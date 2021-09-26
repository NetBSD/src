/*	$NetBSD: args.c,v 1.35 2021/09/26 20:12:37 rillig Exp $	*/

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
__RCSID("$NetBSD: args.c,v 1.35 2021/09/26 20:12:37 rillig Exp $");
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

static void scan_profile(FILE *);
void add_typedefs_from_file(const char *);

static const char *option_source = "?";

#if __STDC_VERSION__ >= 201112L
#define assert_type(expr, type) _Generic((expr), type : (expr))
#else
#define assert_type(expr, type) (expr)
#endif
#define bool_option(name, value, var) \
	{name, true, value, assert_type(&(var), bool *)}
#define int_option(name, var) \
	{name, false, false, assert_type(&(var), int *)}
#define bool_options(name, var) \
	bool_option(name, true, var), \
	bool_option("n" name, false, var)

/*
 * N.B.: because of the way the table here is scanned, options whose names are
 * a prefix of other options must occur later; that is, with -lp vs -l, -lp
 * must be first and -l must be last.
 *
 * See also set_special_option.
 */
static const struct pro {
    const char p_name[6];	/* name, e.g. "bl", "cli" */
    bool p_is_bool;
    bool p_bool_value;
    void *p_obj;		/* the associated variable (bool, int) */
}   pro[] = {
    bool_options("bacc", opt.blanklines_around_conditional_compilation),
    bool_options("badp", opt.blanklines_after_declarations_at_proctop),
    bool_options("bad", opt.blanklines_after_declarations),
    bool_options("bap", opt.blanklines_after_procs),
    bool_options("bbb", opt.blanklines_before_blockcomments),
    bool_options("bc", opt.break_after_comma),
    bool_option("bl", false, opt.btype_2),
    bool_option("br", true, opt.btype_2),
    bool_options("bs", opt.blank_after_sizeof),
    bool_options("cdb", opt.comment_delimiter_on_blankline),
    int_option("cd", opt.decl_comment_column),
    bool_options("ce", opt.cuddle_else),
    int_option("ci", opt.continuation_indent),
    bool_options("cs", opt.space_after_cast),
    int_option("c", opt.comment_column),
    int_option("di", opt.decl_indent),
    bool_options("dj", opt.ljust_decl),
    int_option("d", opt.unindent_displace),
    bool_options("eei", opt.extra_expression_indent),
    bool_options("ei", opt.else_if),
    bool_options("fbs", opt.function_brace_split),
    bool_options("fc1", opt.format_col1_comments),
    bool_options("fcb", opt.format_block_comments),
    bool_options("ip", opt.indent_parameters),
    int_option("i", opt.indent_size),
    int_option("lc", opt.block_comment_max_line_length),
    int_option("ldi", opt.local_decl_indent),
    bool_options("lpl", opt.lineup_to_parens_always),
    bool_options("lp", opt.lineup_to_parens),
    int_option("l", opt.max_line_length),
    bool_options("pcs", opt.proc_calls_space),
    bool_options("psl", opt.procnames_start_line),
    bool_options("sc", opt.star_comment_cont),
    bool_options("sob", opt.swallow_optional_blanklines),
    bool_option("ta", true, opt.auto_typedefs),
    int_option("ts", opt.tabsize),
    bool_options("ut", opt.use_tabs),
    bool_options("v", opt.verbose),
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
	(void)fclose(f);
    }
    if ((f = fopen(option_source = prof, "r")) != NULL) {
	scan_profile(f);
	(void)fclose(f);
    }
    option_source = "Command line";
}

static void
scan_profile(FILE *f)
{
    int comment_index, i;
    char *p;
    char buf[BUFSIZ];

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
skip_over(const char *s, const char *prefix)
{
    while (*prefix != '\0') {
	if (*prefix++ != *s++)
	    return NULL;
    }
    return s;
}

static bool
set_special_option(const char *arg)
{
    const char *arg_end;

    if (strncmp(arg, "-version", 8) == 0) {
	printf("FreeBSD indent %s\n", INDENT_VERSION);
	exit(0);
	/* NOTREACHED */
    }

    if (arg[0] == 'P' || strncmp(arg, "npro", 4) == 0)
	return true;

    if (strncmp(arg, "cli", 3) == 0) {
	arg_end = arg + 3;
	if (arg_end[0] == '\0')
	    goto need_param;
	opt.case_indent = atof(arg_end);
	return true;
    }

    if (strncmp(arg, "st", 2) == 0) {
	if (input == NULL)
	    input = stdin;
	if (output == NULL)
	    output = stdout;
	return true;
    }

    if (arg[0] == 'T') {
	arg_end = arg + 1;
	if (arg_end[0] == '\0')
	    goto need_param;
	add_typename(arg_end);
	return true;
    }

    if (arg[0] == 'U') {
	arg_end = arg + 1;
	if (arg_end[0] == '\0')
	    goto need_param;
	add_typedefs_from_file(arg_end);
	return true;
    }

    return false;

need_param:
    errx(1, "%s: ``%.*s'' requires a parameter",
	option_source, (int)(arg_end - arg), arg);
    /* NOTREACHED */
}

void
set_option(const char *arg)
{
    const struct pro *p;
    const char *param_start;

    arg++;			/* ignore leading "-" */
    if (set_special_option(arg))
	return;

    for (p = pro; p != pro + nitems(pro); p++)
	if (p->p_name[0] == arg[0])
	    if ((param_start = skip_over(arg, p->p_name)) != NULL)
		goto found;
    errx(1, "%s: unknown parameter \"%s\"", option_source, arg - 1);

found:
    if (p->p_is_bool)
	*(bool *)p->p_obj = p->p_bool_value;
    else {
	if (!isdigit((unsigned char)*param_start))
	    errx(1, "%s: ``%s'' requires a parameter",
		option_source, p->p_name);
	*(int *)p->p_obj = atoi(param_start);
    }
}

void
add_typedefs_from_file(const char *fname)
{
    FILE *file;
    char line[BUFSIZ];

    if ((file = fopen(fname, "r")) == NULL) {
	fprintf(stderr, "indent: cannot open file %s\n", fname);
	exit(1);
    }
    while ((fgets(line, BUFSIZ, file)) != NULL) {
	/* Remove trailing whitespace */
	line[strcspn(line, " \t\n\r")] = '\0';
	add_typename(line);
    }
    fclose(file);
}
