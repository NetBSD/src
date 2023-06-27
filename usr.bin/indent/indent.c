/*	$NetBSD: indent.c,v 1.387 2023/06/27 04:41:23 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-4-Clause
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 * Copyright (c) 1976 Board of Trustees of the University of Illinois.
 * Copyright (c) 1980, 1993
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

#include <sys/cdefs.h>
__RCSID("$NetBSD: indent.c,v 1.387 2023/06/27 04:41:23 rillig Exp $");

#include <sys/param.h>
#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "indent.h"

struct options opt = {
	.brace_same_line = true,
	.comment_delimiter_on_blank_line = true,
	.cuddle_else = true,
	.comment_column = 33,
	.decl_indent = 16,
	.else_if_in_same_line = true,
	.function_brace_split = true,
	.format_col1_comments = true,
	.format_block_comments = true,
	.indent_parameters = true,
	.indent_size = 8,
	.local_decl_indent = -1,
	.lineup_to_parens = true,
	.procnames_start_line = true,
	.star_comment_cont = true,
	.tabsize = 8,
	.max_line_length = 78,
	.use_tabs = true,
};

struct parser_state ps;

struct buffer token;

struct buffer lab;
struct buffer code;
struct buffer com;

bool found_err;
bool had_eof;
int line_no = 1;

static struct {
	struct parser_state *item;
	size_t len;
	size_t cap;
} ifdef;

FILE *input;
FILE *output;

static const char *in_name = "Standard Input";
static char backup_name[PATH_MAX];
static const char *backup_suffix = ".BAK";


void *
nonnull(void *p)
{
	if (p == NULL)
		err(EXIT_FAILURE, NULL);
	return p;
}

static void
buf_expand(struct buffer *buf, size_t add_size)
{
	buf->cap = buf->cap + add_size + 400;
	buf->s = nonnull(realloc(buf->s, buf->cap));
}

#ifdef debug
void
buf_terminate(struct buffer *buf)
{
	if (buf->len == buf->cap)
		buf_expand(buf, 1);
	buf->s[buf->len] = '\0';
}
#endif

void
buf_add_char(struct buffer *buf, char ch)
{
	if (buf->len == buf->cap)
		buf_expand(buf, 1);
	buf->s[buf->len++] = ch;
	buf_terminate(buf);
}

void
buf_add_chars(struct buffer *buf, const char *s, size_t len)
{
	if (len == 0)
		return;
	if (len > buf->cap - buf->len)
		buf_expand(buf, len);
	memcpy(buf->s + buf->len, s, len);
	buf->len += len;
	buf_terminate(buf);
}

static void
buf_add_buf(struct buffer *buf, const struct buffer *add)
{
	buf_add_chars(buf, add->s, add->len);
}

void
diag(int level, const char *msg, ...)
{
	va_list ap;

	if (level != 0)
		found_err = true;

	va_start(ap, msg);
	fprintf(stderr, "%s: %s:%d: ",
	    level == 0 ? "warning" : "error", in_name, line_no);
	vfprintf(stderr, msg, ap);
	fprintf(stderr, "\n");
	va_end(ap);
}

/*
 * Compute the indentation from starting at 'ind' and adding the text starting
 * at 's'.
 */
int
ind_add(int ind, const char *s, size_t len)
{
	for (const char *p = s; len > 0; p++, len--) {
		if (*p == '\n')
			ind = 0;
		else if (*p == '\t')
			ind = next_tab(ind);
		else
			ind++;
	}
	return ind;
}

static void
init_globals(void)
{
	ps_push(psym_stmt, false);	/* as a stop symbol */
	ps.prev_lsym = lsym_semicolon;
	ps.lbrace_kind = psym_lbrace_block;

	const char *suffix = getenv("SIMPLE_BACKUP_SUFFIX");
	if (suffix != NULL)
		backup_suffix = suffix;
}

static void
load_profiles(int argc, char **argv)
{
	const char *profile_name = NULL;

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];

		if (strcmp(arg, "-npro") == 0)
			return;
		if (arg[0] == '-' && arg[1] == 'P' && arg[2] != '\0')
			profile_name = arg + 2;
	}

	load_profile_files(profile_name);
}

/*
 * Copy the input file to the backup file, then make the backup file the input
 * and the original input file the output.
 */
static void
copy_to_bak_file(void)
{
	size_t n;
	char buff[BUFSIZ];

	const char *last_slash = strrchr(in_name, '/');
	const char *base = last_slash != NULL ? last_slash + 1 : in_name;
	snprintf(backup_name, sizeof(backup_name), "%s%s", base, backup_suffix);

	/* copy the input file to the backup file */
	FILE *bak = fopen(backup_name, "w");
	if (bak == NULL)
		err(1, "%s", backup_name);

	while ((n = fread(buff, 1, sizeof(buff), input)) > 0)
		if (fwrite(buff, 1, n, bak) != n)
			err(1, "%s", backup_name);
	if (fclose(input) != 0)
		err(1, "%s", in_name);
	if (fclose(bak) != 0)
		err(1, "%s", backup_name);

	/* re-open the backup file as the input file */
	input = fopen(backup_name, "r");
	if (input == NULL)
		err(1, "%s", backup_name);
	/* now the original input file will be the output */
	output = fopen(in_name, "w");
	if (output == NULL) {
		remove(backup_name);
		err(1, "%s", in_name);
	}
}

static void
parse_command_line(int argc, char **argv)
{
	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];

		if (arg[0] == '-') {
			set_option(arg, "Command line");

		} else if (input == NULL) {
			in_name = arg;
			if ((input = fopen(in_name, "r")) == NULL)
				err(1, "%s", in_name);

		} else if (output == NULL) {
			if (strcmp(arg, in_name) == 0)
				errx(1, "input and output files "
				    "must be different");
			if ((output = fopen(arg, "w")) == NULL)
				err(1, "%s", arg);

		} else
			errx(1, "too many arguments: %s", arg);
	}

	if (input == NULL) {
		input = stdin;
		output = stdout;
	} else if (output == NULL)
		copy_to_bak_file();

	if (opt.comment_column <= 1)
		opt.comment_column = 2;	/* don't put normal comments in column
					 * 1, see opt.format_col1_comments */
	if (opt.block_comment_max_line_length <= 0)
		opt.block_comment_max_line_length = opt.max_line_length;
	if (opt.local_decl_indent < 0)
		opt.local_decl_indent = opt.decl_indent;
	if (opt.decl_comment_column <= 0)
		opt.decl_comment_column = opt.left_justify_decl
		    ? (opt.comment_column <= 10 ? 2 : opt.comment_column - 8)
		    : opt.comment_column;
	if (opt.continuation_indent == 0)
		opt.continuation_indent = opt.indent_size;
}

static void
set_initial_indentation(void)
{
	inp_read_line();

	int ind = 0;
	for (const char *p = inp_p;; p++) {
		if (*p == ' ')
			ind++;
		else if (*p == '\t')
			ind = next_tab(ind);
		else
			break;
	}

	ps.ind_level = ps.ind_level_follow = ind / opt.indent_size;
}

static bool
should_break_line(lexer_symbol lsym)
{
	if (lsym == lsym_semicolon)
		return false;
	if (ps.prev_lsym == lsym_lbrace || ps.prev_lsym == lsym_semicolon)
		return true;
	if (lsym == lsym_lbrace && opt.brace_same_line)
		return false;
	return true;
}

static void
move_com_to_code(lexer_symbol lsym)
{
	if (ps.want_blank)
		buf_add_char(&code, ' ');
	buf_add_buf(&code, &com);
	buf_clear(&com);
	ps.want_blank = lsym != lsym_rparen && lsym != lsym_rbracket;
}

static void
update_ps_lbrace_kind(lexer_symbol lsym)
{
	if (lsym == lsym_tag) {
		ps.lbrace_kind = token.s[0] == 's' ? psym_lbrace_struct :
		    token.s[0] == 'u' ? psym_lbrace_union :
		    psym_lbrace_enum;
	} else if ((lsym == lsym_type && ps.paren.len == 0)
	    || lsym == lsym_word
	    || lsym == lsym_lbrace) {
		/* Keep the current '{' kind. */
	} else
		ps.lbrace_kind = psym_lbrace_block;
}

static void
update_ps_badp(lexer_symbol lsym)
{
	if (lsym == lsym_lbrace && ps.lbrace_kind == psym_lbrace_block
	    && ps.psyms.len == 3)
		ps.badp = badp_seen_lbrace;
	if (lsym == lsym_rbrace && !ps.in_decl)
		ps.badp = badp_none;
	if (lsym == lsym_type && ps.paren.len == 0
	    && (ps.badp == badp_seen_lbrace || ps.badp == badp_yes))
		ps.badp = badp_decl;
	if (lsym == lsym_semicolon && ps.badp == badp_decl
	    && ps.decl_level == 0)
		ps.badp = badp_seen_decl;
}

static void
indent_declarator(int decl_ind, bool tabs_to_var)
{
	int base = ps.ind_level * opt.indent_size;
	int ind = ind_add(base, code.s, code.len);
	int target = base + decl_ind;
	size_t orig_code_len = code.len;

	if (tabs_to_var)
		for (int next; (next = next_tab(ind)) <= target; ind = next)
			buf_add_char(&code, '\t');
	for (; ind < target; ind++)
		buf_add_char(&code, ' ');
	if (code.len == orig_code_len && ps.want_blank)
		buf_add_char(&code, ' ');

	ps.want_blank = false;
	ps.decl_indent_done = true;
}

static bool
is_function_pointer_declaration(void)
{
	return ps.in_decl
	    && !ps.in_typedef_decl
	    && !ps.in_init
	    && !ps.decl_indent_done
	    && !ps.line_has_func_def
	    && ps.ind_paren_level == 0;
}

static int
process_eof(void)
{
	finish_output();

	if (ps.psyms.len > 2)	/* check for balanced braces */
		diag(1, "Stuff missing from end of file");

	return found_err ? EXIT_FAILURE : EXIT_SUCCESS;
}

/* move the whole line to the 'label' buffer */
static void
read_preprocessing_line(void)
{
	enum {
		PLAIN, STR, CHR, COMM
	} state = PLAIN;

	buf_add_char(&lab, '#');

	while (inp_p[0] != '\n' || (state == COMM && !had_eof)) {
		buf_add_char(&lab, inp_next());
		switch (lab.s[lab.len - 1]) {
		case '\\':
			if (state != COMM)
				buf_add_char(&lab, inp_next());
			break;
		case '/':
			if (inp_p[0] == '*' && state == PLAIN) {
				state = COMM;
				buf_add_char(&lab, *inp_p++);
			}
			break;
		case '"':
			if (state == STR)
				state = PLAIN;
			else if (state == PLAIN)
				state = STR;
			break;
		case '\'':
			if (state == CHR)
				state = PLAIN;
			else if (state == PLAIN)
				state = CHR;
			break;
		case '*':
			if (inp_p[0] == '/' && state == COMM) {
				state = PLAIN;
				buf_add_char(&lab, *inp_p++);
			}
			break;
		}
	}

	while (lab.len > 0 && ch_isblank(lab.s[lab.len - 1]))
		lab.len--;
	buf_terminate(&lab);
}

static void
paren_stack_push(struct paren_stack *s, int indent, enum paren_level_cast cast)
{
	if (s->len == s->cap) {
		s->cap = 10 + s->cap;
		s->item = nonnull(realloc(s->item,
			sizeof(s->item[0]) * s->cap));
	}
	s->item[s->len++] = (struct paren_level){indent, cast};
}

static void *
dup_mem(const void *src, size_t size)
{
	return memcpy(nonnull(malloc(size)), src, size);
}

#define dup_array(src, len) \
    dup_mem((src), sizeof((src)[0]) * (len))
#define copy_array(dst, src, len) \
    memcpy((dst), (src), sizeof((dst)[0]) * (len))

static_unless_debug void
parser_state_back_up(struct parser_state *dst)
{
	*dst = ps;

	dst->paren.item = dup_array(ps.paren.item, ps.paren.len);
	dst->psyms.sym = dup_array(ps.psyms.sym, ps.psyms.len);
	dst->psyms.ind_level = dup_array(ps.psyms.ind_level, ps.psyms.len);
}

static void
parser_state_restore(const struct parser_state *src)
{
	struct paren_level *ps_paren_item = ps.paren.item;
	size_t ps_paren_cap = ps.paren.cap;
	enum parser_symbol *ps_psyms_sym = ps.psyms.sym;
	int *ps_psyms_ind_level = ps.psyms.ind_level;
	size_t ps_psyms_cap = ps.psyms.cap;

	ps = *src;

	ps.paren.item = ps_paren_item;
	ps.paren.cap = ps_paren_cap;
	ps.psyms.sym = ps_psyms_sym;
	ps.psyms.ind_level = ps_psyms_ind_level;
	ps.psyms.cap = ps_psyms_cap;

	copy_array(ps.paren.item, src->paren.item, src->paren.len);
	copy_array(ps.psyms.sym, src->psyms.sym, src->psyms.len);
	copy_array(ps.psyms.ind_level, src->psyms.ind_level, src->psyms.len);
}

static_unless_debug void
parser_state_free(struct parser_state *pst)
{
	free(pst->paren.item);
	free(pst->psyms.sym);
	free(pst->psyms.ind_level);
}

static void
process_preprocessing(void)
{
	if (lab.len > 0 || code.len > 0 || com.len > 0)
		output_line();

	read_preprocessing_line();

	const char *dir = lab.s + 1, *line_end = lab.s + lab.len;
	while (dir < line_end && ch_isblank(*dir))
		dir++;
	size_t dir_len = 0;
	while (dir + dir_len < line_end && ch_isalpha(dir[dir_len]))
		dir_len++;

	if (dir_len >= 2 && memcmp(dir, "if", 2) == 0) {
		if (ifdef.len >= ifdef.cap) {
			ifdef.cap += 5;
			ifdef.item = nonnull(realloc(ifdef.item,
				sizeof(ifdef.item[0]) * ifdef.cap));
		}
		parser_state_back_up(ifdef.item + ifdef.len++);
		out.line_kind = lk_pre_if;

	} else if (dir_len >= 2 && memcmp(dir, "el", 2) == 0) {
		if (ifdef.len == 0)
			diag(1, "Unmatched #%.*s", (int)dir_len, dir);
		else
			parser_state_restore(ifdef.item + ifdef.len - 1);
		out.line_kind = lk_pre_other;

	} else if (dir_len == 5 && memcmp(dir, "endif", 5) == 0) {
		if (ifdef.len == 0)
			diag(1, "Unmatched #endif");
		else
			parser_state_free(ifdef.item + --ifdef.len);
		out.line_kind = lk_pre_endif;
	} else
		out.line_kind = lk_pre_other;
}

static void
process_newline(void)
{
	if (ps.prev_lsym == lsym_comma
	    && ps.paren.len == 0 && !ps.in_init
	    && !opt.break_after_comma && ps.break_after_comma
	    && lab.len == 0	/* for preprocessing lines */
	    && com.len == 0)
		goto stay_in_line;
	if (ps.psyms.sym[ps.psyms.len - 1] == psym_switch_expr
	    && opt.brace_same_line
	    && com.len == 0) {
		ps.want_newline = true;
		goto stay_in_line;
	}

	output_line();

stay_in_line:
	line_no++;
}

static bool
want_blank_before_lparen(void)
{
	if (opt.proc_calls_space)
		return true;
	if (ps.prev_lsym == lsym_sizeof)
		return opt.blank_after_sizeof;
	if (ps.prev_lsym == lsym_rparen
	    || ps.prev_lsym == lsym_rbracket
	    || ps.prev_lsym == lsym_postfix_op
	    || ps.prev_lsym == lsym_offsetof
	    || ps.prev_lsym == lsym_word
	    || ps.prev_lsym == lsym_funcname)
		return false;
	return true;
}

static void
process_lparen(void)
{

	if (is_function_pointer_declaration())
		indent_declarator(ps.decl_ind, ps.tabs_to_var);
	else if (ps.want_blank && want_blank_before_lparen())
		buf_add_char(&code, ' ');
	ps.want_blank = false;
	buf_add_buf(&code, &token);

	if (opt.extra_expr_indent && ps.spaced_expr_psym != psym_0)
		ps.extra_expr_indent = eei_maybe;

	if (ps.in_var_decl && ps.psyms.len <= 3 && !ps.in_init) {
		parse(psym_stmt);	/* prepare for function definition */
		ps.in_var_decl = false;
	}

	enum paren_level_cast cast = cast_unknown;
	if (ps.prev_lsym == lsym_offsetof
	    || ps.prev_lsym == lsym_sizeof
	    || ps.prev_lsym == lsym_for
	    || ps.prev_lsym == lsym_if
	    || ps.prev_lsym == lsym_switch
	    || ps.prev_lsym == lsym_while
	    || ps.line_has_func_def)
		cast = cast_no;

	paren_stack_push(&ps.paren, ind_add(0, code.s, code.len), cast);
}

static bool
rparen_is_cast(bool paren_cast)
{
	if (ps.in_func_def_params)
		return false;
	if (ps.line_has_decl && !ps.in_init)
		return false;
	if (ps.prev_lsym == lsym_unary_op)
		return true;
	if (ps.spaced_expr_psym != psym_0 && ps.paren.len == 0)
		return false;
	return paren_cast || ch_isalpha(inp_p[0]) || inp_p[0] == '{';
}

static void
process_rparen(void)
{
	if (ps.paren.len == 0)
		diag(0, "Extra '%c'", *token.s);

	bool paren_cast = ps.paren.len > 0
	    && ps.paren.item[--ps.paren.len].cast == cast_maybe;
	ps.prev_paren_was_cast = rparen_is_cast(paren_cast);
	if (ps.prev_paren_was_cast) {
		ps.next_unary = true;
		ps.want_blank = opt.space_after_cast;
	} else
		ps.want_blank = true;

	if (code.len == 0)
		ps.ind_paren_level = (int)ps.paren.len;

	buf_add_buf(&code, &token);

	if (ps.spaced_expr_psym != psym_0 && ps.paren.len == 0) {
		parse(ps.spaced_expr_psym);
		ps.spaced_expr_psym = psym_0;

		ps.want_newline = true;
		ps.next_unary = true;
		ps.in_stmt_or_decl = false;
		ps.want_blank = true;
		out.line_kind = lk_stmt_head;
		if (ps.extra_expr_indent == eei_maybe)
			ps.extra_expr_indent = eei_last;
	}
}

static void
process_lbracket(void)
{
	if (code.len > 0
	    && (ps.prev_lsym == lsym_comma || ps.prev_lsym == lsym_binary_op))
		buf_add_char(&code, ' ');
	buf_add_buf(&code, &token);
	ps.want_blank = false;

	paren_stack_push(&ps.paren, ind_add(0, code.s, code.len), cast_no);
}

static void
process_rbracket(void)
{
	if (ps.paren.len == 0)
		diag(0, "Extra '%c'", *token.s);
	if (ps.paren.len > 0)
		ps.paren.len--;

	if (code.len == 0)
		ps.ind_paren_level = (int)ps.paren.len;

	buf_add_buf(&code, &token);
	ps.want_blank = true;
}

static void
process_lbrace(void)
{
	if (ps.prev_lsym == lsym_rparen && ps.prev_paren_was_cast) {
		ps.in_var_decl = true;	// XXX: not really
		ps.in_init = true;
	}

	if (out.line_kind == lk_stmt_head)
		out.line_kind = lk_other;

	ps.in_stmt_or_decl = false;	/* don't indent the {} */

	if (ps.in_init)
		ps.init_level++;
	else
		ps.want_newline = true;

	if (code.len > 0 && !ps.in_init) {
		if (!opt.brace_same_line ||
		    (code.len > 0 && code.s[code.len - 1] == '}'))
			output_line();
		else if (ps.in_func_def_params && !ps.in_var_decl) {
			ps.ind_level_follow = 0;
			if (opt.function_brace_split)
				output_line();
			else
				ps.want_blank = true;
		}
	}

	if (ps.paren.len > 0 && ps.init_level == 0) {
		diag(1, "Unbalanced parentheses");
		ps.paren.len = 0;
		if (ps.spaced_expr_psym != psym_0) {
			parse(ps.spaced_expr_psym);
			ps.spaced_expr_psym = psym_0;
			ps.ind_level = ps.ind_level_follow;
		}
	}

	if (code.len == 0)
		ps.line_is_stmt_cont = false;
	if (ps.in_decl && ps.in_var_decl) {
		ps.di_stack[ps.decl_level] = ps.decl_ind;
		if (++ps.decl_level == (int)array_length(ps.di_stack)) {
			diag(0, "Reached internal limit of %zu struct levels",
			    array_length(ps.di_stack));
			ps.decl_level--;
		}
	} else {
		ps.line_has_decl = false;	/* don't do special indentation
						 * of comments */
		ps.in_func_def_params = false;
		ps.in_decl = false;
	}

	ps.decl_ind = 0;
	parse(ps.lbrace_kind);
	if (ps.want_blank)
		buf_add_char(&code, ' ');
	ps.want_blank = false;
	buf_add_char(&code, '{');
	ps.declaration = decl_no;
}

static void
process_rbrace(void)
{
	if (ps.paren.len > 0 && ps.init_level == 0) {
		diag(1, "Unbalanced parentheses");
		ps.paren.len = 0;
		ps.spaced_expr_psym = psym_0;
	}

	ps.declaration = decl_no;
	if (ps.decl_level == 0)
		ps.blank_line_after_decl = false;
	if (ps.init_level > 0)
		ps.init_level--;

	if (code.len > 0 && !ps.in_init)
		output_line();

	buf_add_char(&code, '}');
	ps.want_blank = true;
	ps.in_stmt_or_decl = false;	// XXX: Initializers don't end a stmt
	ps.line_is_stmt_cont = false;

	if (ps.decl_level > 0) {	/* multi-level structure declaration */
		ps.decl_ind = ps.di_stack[--ps.decl_level];
		if (ps.decl_level == 0 && !ps.in_func_def_params) {
			ps.declaration = decl_begin;
			ps.decl_ind = ps.ind_level == 0
			    ? opt.decl_indent : opt.local_decl_indent;
		}
		ps.in_decl = true;
	}

	if (ps.psyms.len == 3)
		out.line_kind = lk_func_end;

	parse(psym_rbrace);

	if (!ps.in_var_decl
	    && ps.psyms.sym[ps.psyms.len - 1] != psym_do_stmt
	    && ps.psyms.sym[ps.psyms.len - 1] != psym_if_expr_stmt)
		ps.want_newline = true;
}

static void
process_period(void)
{
	if (code.len > 0 && code.s[code.len - 1] == ',')
		buf_add_char(&code, ' ');
	buf_add_char(&code, '.');
	ps.want_blank = false;
}

static void
process_unary_op(void)
{
	if (is_function_pointer_declaration()) {
		int ind = ps.decl_ind - (int)token.len;
		indent_declarator(ind, ps.tabs_to_var);
	} else if ((token.s[0] == '+' || token.s[0] == '-')
	    && code.len > 0 && code.s[code.len - 1] == token.s[0])
		ps.want_blank = true;

	if (ps.want_blank)
		buf_add_char(&code, ' ');
	buf_add_buf(&code, &token);
	ps.want_blank = false;
}

static void
process_postfix_op(void)
{
	buf_add_buf(&code, &token);
	ps.want_blank = true;
}

static void
process_comma(void)
{
	ps.want_blank = code.len > 0;	/* only put blank after comma if comma
					 * does not start the line */

	if (ps.in_decl && ps.ind_paren_level == 0
	    && !ps.line_has_func_def && !ps.in_init && !ps.decl_indent_done) {
		/* indent leading commas and not the actual identifiers */
		indent_declarator(ps.decl_ind - 1, ps.tabs_to_var);
	}

	buf_add_char(&code, ',');

	if (ps.paren.len == 0) {
		if (ps.init_level == 0)
			ps.in_init = false;
		int typical_varname_length = 8;
		if (ps.break_after_comma && (opt.break_after_comma ||
			ind_add(compute_code_indent(), code.s, code.len)
			>= opt.max_line_length - typical_varname_length))
			ps.want_newline = true;
	}
}

static void
process_label_colon(void)
{
	buf_add_buf(&lab, &code);
	buf_add_char(&lab, ':');
	buf_clear(&code);

	if (ps.seen_case)
		out.line_kind = lk_case_or_default;
	ps.in_stmt_or_decl = false;
	ps.want_newline = ps.seen_case;
	ps.seen_case = false;
	ps.want_blank = false;
}

static void
process_other_colon(void)
{
	buf_add_char(&code, ':');
	ps.want_blank = ps.decl_level == 0;
}

static void
process_semicolon(void)
{
	if (out.line_kind == lk_stmt_head)
		out.line_kind = lk_other;
	if (ps.decl_level == 0) {
		ps.in_var_decl = false;
		ps.in_typedef_decl = false;
	}
	ps.seen_case = false;	/* only needs to be reset on error */
	ps.quest_level = 0;	/* only needs to be reset on error */
	if (ps.prev_lsym == lsym_rparen)
		ps.in_func_def_params = false;
	ps.in_init = false;
	ps.init_level = 0;
	ps.declaration = ps.declaration == decl_begin ? decl_end : decl_no;

	if (ps.in_decl && code.len == 0 && !ps.in_init &&
	    !ps.decl_indent_done && ps.ind_paren_level == 0) {
		/* indent stray semicolons in declarations */
		indent_declarator(ps.decl_ind - 1, ps.tabs_to_var);
	}

	ps.in_decl = ps.decl_level > 0;	/* if we were in a first level
					 * structure declaration before, we
					 * aren't anymore */

	if (ps.paren.len > 0 && ps.spaced_expr_psym != psym_for_exprs) {
		diag(1, "Unbalanced parentheses");
		ps.paren.len = 0;
		if (ps.spaced_expr_psym != psym_0) {
			parse(ps.spaced_expr_psym);
			ps.spaced_expr_psym = psym_0;
		}
	}
	buf_add_char(&code, ';');
	ps.want_blank = true;
	ps.in_stmt_or_decl = ps.paren.len > 0;
	ps.decl_ind = 0;

	if (ps.spaced_expr_psym == psym_0) {
		parse(psym_stmt);
		ps.want_newline = true;
	}
}

static void
process_type_outside_parentheses(void)
{
	parse(psym_decl);	/* let the parser worry about indentation */

	if (ps.prev_lsym == lsym_rparen && ps.psyms.len <= 2 && code.len > 0)
		output_line();

	if (ps.in_func_def_params && opt.indent_parameters &&
	    ps.decl_level == 0) {
		ps.ind_level = ps.ind_level_follow = 1;
		ps.line_is_stmt_cont = false;
	}

	ps.in_var_decl = /* maybe */ true;
	ps.in_decl = true;
	ps.line_has_decl = ps.in_decl;
	if (ps.decl_level == 0)
		ps.declaration = decl_begin;

	int ind = ps.ind_level > 0 && ps.decl_level == 0
	    ? opt.local_decl_indent	/* local variable */
	    : opt.decl_indent;	/* global variable, or member */
	if (ind == 0) {
		int ind0 = code.len > 0 ? ind_add(0, code.s, code.len) + 1 : 0;
		ps.decl_ind = ind_add(ind0, token.s, token.len) + 1;
	} else
		ps.decl_ind = ind;
	ps.tabs_to_var = opt.use_tabs && ind > 0;
}

static void
process_word(lexer_symbol lsym)
{
	if (lsym == lsym_type	/* in parentheses */
	    && ps.paren.item[ps.paren.len - 1].cast == cast_unknown)
		ps.paren.item[ps.paren.len - 1].cast = cast_maybe;

	if (ps.in_decl) {
		if (lsym == lsym_funcname) {
			ps.in_decl = false;
			if (opt.procnames_start_line
			    && code.len > (*inp_p == ')' ? 1 : 0))
				output_line();
			else if (ps.want_blank)
				buf_add_char(&code, ' ');
			ps.want_blank = false;
		} else if (ps.in_typedef_decl && ps.decl_level == 0) {
			/* Do not indent typedef declarators. */
		} else if (!ps.in_init && !ps.decl_indent_done &&
		    ps.ind_paren_level == 0) {
			if (opt.decl_indent == 0
			    && code.len > 0 && code.s[code.len - 1] == '}')
				ps.decl_ind = ind_add(0, code.s, code.len) + 1;
			indent_declarator(ps.decl_ind, ps.tabs_to_var);
		}

	} else if (ps.spaced_expr_psym != psym_0 && ps.paren.len == 0) {
		parse(ps.spaced_expr_psym);
		ps.spaced_expr_psym = psym_0;
		ps.want_newline = true;
		ps.in_stmt_or_decl = false;
		ps.next_unary = true;
	}
}

static void
process_do(void)
{
	ps.in_stmt_or_decl = false;
	ps.in_decl = false;

	if (code.len > 0)
		output_line();

	parse(psym_do);
	ps.want_newline = true;
}

static void
process_else(void)
{
	ps.in_stmt_or_decl = false;
	ps.in_decl = false;

	if (code.len > 0
	    && !(opt.cuddle_else && code.s[code.len - 1] == '}'))
		output_line();

	parse(psym_else);
	ps.want_newline = true;
}

static void
process_lsym(lexer_symbol lsym)
{
	switch (lsym) {
		/* INDENT OFF */
	case lsym_preprocessing: process_preprocessing(); break;
	case lsym_newline:	process_newline();	break;
	case lsym_comment:	process_comment();	break;
	case lsym_lparen:	process_lparen();	break;
	case lsym_lbracket:	process_lbracket();	break;
	case lsym_rparen:	process_rparen();	break;
	case lsym_rbracket:	process_rbracket();	break;
	case lsym_lbrace:	process_lbrace();	break;
	case lsym_rbrace:	process_rbrace();	break;
	case lsym_period:	process_period();	break;
	case lsym_unary_op:	process_unary_op();	break;
	case lsym_postfix_op:	process_postfix_op();	break;
	case lsym_binary_op:				goto copy_token;
	case lsym_question:	ps.quest_level++;	goto copy_token;
	case lsym_question_colon:			goto copy_token;
	case lsym_label_colon:	process_label_colon();	break;
	case lsym_other_colon:	process_other_colon();	break;
	case lsym_comma:	process_comma();	break;
	case lsym_semicolon:	process_semicolon();	break;
	case lsym_typedef:	ps.in_typedef_decl = true; goto copy_token;
	case lsym_modifier:				goto copy_token;
	case lsym_case:		ps.seen_case = true;	goto copy_token;
	case lsym_default:	ps.seen_case = true;	goto copy_token;
	case lsym_do:		process_do();		goto copy_token;
	case lsym_else:		process_else();		goto copy_token;
	case lsym_for:		ps.spaced_expr_psym = psym_for_exprs; goto copy_token;
	case lsym_if:		ps.spaced_expr_psym = psym_if_expr; goto copy_token;
	case lsym_switch:	ps.spaced_expr_psym = psym_switch_expr; goto copy_token;
	case lsym_while:	ps.spaced_expr_psym = psym_while_expr; goto copy_token;
		/* INDENT ON */

	case lsym_tag:
		if (ps.paren.len > 0)
			goto copy_token;
		/* FALLTHROUGH */
	case lsym_type:
		if (ps.paren.len == 0) {
			process_type_outside_parentheses();
			goto copy_token;
		}
		/* FALLTHROUGH */
	case lsym_sizeof:
	case lsym_offsetof:
	case lsym_word:
	case lsym_funcname:
	case lsym_return:
		process_word(lsym);
copy_token:
		if (ps.want_blank)
			buf_add_char(&code, ' ');
		buf_add_buf(&code, &token);
		if (lsym != lsym_funcname)
			ps.want_blank = true;
		break;

	default:
		break;
	}
}

static int
indent(void)
{
	debug_parser_state();

	for (;;) {		/* loop until we reach eof */
		lexer_symbol lsym = lexi();

		debug_blank_line();
		debug_printf("line %d: %s", line_no, lsym_name[lsym]);
		debug_print_buf("token", &token);
		debug_buffers();
		debug_blank_line();

		if (lsym == lsym_eof)
			return process_eof();

		if (lsym == lsym_preprocessing || lsym == lsym_newline)
			ps.want_newline = false;
		else if (lsym == lsym_comment) {
			/* no special processing */
		} else {
			if (lsym == lsym_if && ps.prev_lsym == lsym_else
			    && opt.else_if_in_same_line)
				ps.want_newline = false;

			if (ps.want_newline && should_break_line(lsym)) {
				ps.want_newline = false;
				output_line();
			}
			ps.in_stmt_or_decl = true;
			if (com.len > 0)
				move_com_to_code(lsym);
			update_ps_lbrace_kind(lsym);
		}

		process_lsym(lsym);

		if (opt.blank_line_after_decl_at_top)
			update_ps_badp(lsym);
		if (lsym != lsym_preprocessing
		    && lsym != lsym_newline
		    && lsym != lsym_comment)
			ps.prev_lsym = lsym;

		debug_parser_state();
	}
}

int
main(int argc, char **argv)
{
	init_globals();
	load_profiles(argc, argv);
	parse_command_line(argc, argv);
	set_initial_indentation();
	return indent();
}
