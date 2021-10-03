/*	$NetBSD: args.c,v 1.43 2021/10/03 19:09:59 rillig Exp $	*/

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
__RCSID("$NetBSD: args.c,v 1.43 2021/10/03 19:09:59 rillig Exp $");
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

void add_typedefs_from_file(const char *);

static const char *option_source = "?";

#if __STDC_VERSION__ >= 201112L
#define assert_type(expr, type) _Generic((expr), type : (expr))
#else
#define assert_type(expr, type) (expr)
#endif
#define bool_option(name, value, var) \
	{name, true, value, false, assert_type(&(opt.var), bool *)}
#define int_option(name, var) \
	{name, false, false, false, assert_type(&(opt.var), int *)}
#define bool_options(name, var) \
	{name, true, false, true, assert_type(&(opt.var), bool *)}

/*
 * N.B.: an option whose name is a prefix of another option must come earlier;
 * for example, "l" must come before "lp".
 *
 * See set_special_option for special options.
 */
static const struct pro {
    const char p_name[5];	/* name, e.g. "bl", "cli" */
    bool p_is_bool;
    bool p_bool_value;
    bool p_may_negate;
    void *p_var;		/* the associated variable */
}   pro[] = {
    bool_options("bacc", blanklines_around_conditional_compilation),
    bool_options("bad", blanklines_after_declarations),
    bool_options("badp", blanklines_after_declarations_at_proctop),
    bool_options("bap", blanklines_after_procs),
    bool_options("bbb", blanklines_before_blockcomments),
    bool_options("bc", break_after_comma),
    bool_option("bl", false, btype_2),
    bool_option("br", true, btype_2),
    bool_options("bs", blank_after_sizeof),
    int_option("c", comment_column),
    int_option("cd", decl_comment_column),
    bool_options("cdb", comment_delimiter_on_blankline),
    bool_options("ce", cuddle_else),
    int_option("ci", continuation_indent),
    /* "cli" is special */
    bool_options("cs", space_after_cast),
    int_option("d", unindent_displace),
    int_option("di", decl_indent),
    bool_options("dj", ljust_decl),
    bool_options("eei", extra_expression_indent),
    bool_options("ei", else_if),
    bool_options("fbs", function_brace_split),
    bool_options("fc1", format_col1_comments),
    bool_options("fcb", format_block_comments),
    int_option("i", indent_size),
    bool_options("ip", indent_parameters),
    int_option("l", max_line_length),
    int_option("lc", block_comment_max_line_length),
    int_option("ldi", local_decl_indent),
    bool_options("lp", lineup_to_parens),
    bool_options("lpl", lineup_to_parens_always),
    /* "npro" is special */
    /* "P" is special */
    bool_options("pcs", proc_calls_space),
    bool_options("psl", procnames_start_line),
    bool_options("sc", star_comment_cont),
    bool_options("sob", swallow_optional_blanklines),
    /* "st" is special */
    bool_option("ta", true, auto_typedefs),
    /* "T" is special */
    int_option("ts", tabsize),
    /* "U" is special */
    bool_options("ut", use_tabs),
    bool_options("v", verbose),
};

static void
load_profile(const char *fname)
{
    FILE *f;
    int comment_index, ch;
    char *p;
    char buf[BUFSIZ];

    if ((f = fopen(fname, "r")) == NULL)
	return;
    option_source = fname;

    for (;;) {
	p = buf;
	comment_index = 0;
	while ((ch = getc(f)) != EOF) {
	    if (ch == '*' && comment_index == 0 && p > buf && p[-1] == '/') {
		comment_index = (int)(p - buf);
		*p++ = (char)ch;
	    } else if (ch == '/' && comment_index != 0 && p > buf && p[-1] == '*') {
		p = buf + comment_index - 1;
		comment_index = 0;
	    } else if (isspace((unsigned char)ch)) {
		if (p > buf && comment_index == 0)
		    break;
	    } else {
		*p++ = (char)ch;
	    }
	}
	if (p != buf) {
	    *p++ = '\0';
	    if (opt.verbose)
		printf("profile: %s\n", buf);
	    set_option(buf);
	} else if (ch == EOF) {
	    (void)fclose(f);
	    return;
	}
    }
}

void
load_profiles(const char *profile_name)
{
    char fname[PATH_MAX];

    if (profile_name != NULL)
	load_profile(profile_name);
    else {
	snprintf(fname, sizeof(fname), "%s/.indent.pro", getenv("HOME"));
	load_profile(fname);
    }
    load_profile(".indent.pro");
    option_source = "Command line";
}

static const char *
skip_over(const char *s, bool may_negate, const char *prefix)
{
    if (may_negate && s[0] == 'n')
	s++;
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

    for (p = pro + nitems(pro); p-- != pro;) {
	param_start = skip_over(arg, p->p_may_negate, p->p_name);
	if (param_start != NULL)
	    goto found;
    }
    errx(1, "%s: unknown parameter \"%s\"", option_source, arg - 1);

found:
    if (p->p_is_bool)
	*(bool *)p->p_var = p->p_may_negate ? arg[0] != 'n' : p->p_bool_value;
    else {
	if (!isdigit((unsigned char)*param_start))
	    errx(1, "%s: ``%s'' requires a parameter",
		option_source, p->p_name);
	*(int *)p->p_var = atoi(param_start);
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
