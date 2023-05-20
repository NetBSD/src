/*	$NetBSD: indent.h,v 1.156 2023/05/20 11:53:53 rillig Exp $	*/

/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 2001 Jens Schweikhardt
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
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

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum lexer_symbol {
	lsym_eof,
	lsym_preprocessing,	/* '#' */
	lsym_newline,
	lsym_comment,		/* the initial '/ *' or '//' of a comment */
	lsym_lparen_or_lbracket,
	lsym_rparen_or_rbracket,
	lsym_lbrace,
	lsym_rbrace,
	lsym_period,
	lsym_unary_op,		/* e.g. '*', '&', '-' or leading '++' */
	lsym_binary_op,		/* e.g. '*', '&', '<<', '&&' or '/=' */
	lsym_postfix_op,	/* trailing '++' or '--' */
	lsym_question,		/* the '?' from a '?:' expression */
	lsym_colon,
	lsym_comma,
	lsym_semicolon,
	lsym_typedef,
	lsym_storage_class,
	lsym_type_outside_parentheses,
	lsym_type_in_parentheses,
	lsym_tag,		/* 'struct', 'union' or 'enum' */
	lsym_case_label,	/* 'case' or 'default' */
	lsym_sizeof,
	lsym_offsetof,
	lsym_word,		/* identifier, constant or string */
	lsym_funcname,		/* name of a function being defined */
	lsym_do,
	lsym_else,
	lsym_for,
	lsym_if,
	lsym_switch,
	lsym_while,
	lsym_return
} lexer_symbol;

typedef enum parser_symbol {
	psym_0,			/* a placeholder */
	psym_lbrace,
	psym_rbrace,
	psym_decl,
	psym_stmt,
	psym_stmt_list,
	psym_for_exprs,		/* 'for' '(' ... ')' */
	psym_if_expr,		/* 'if' '(' expr ')' */
	psym_if_expr_stmt,	/* 'if' '(' expr ')' stmt */
	psym_if_expr_stmt_else,	/* 'if' '(' expr ')' stmt 'else' */
	psym_else,		/* 'else'; not stored on the stack */
	psym_switch_expr,	/* 'switch' '(' expr ')' */
	psym_do,		/* 'do' */
	psym_do_stmt,		/* 'do' stmt */
	psym_while_expr,	/* 'while' '(' expr ')' */
} parser_symbol;

/* A range of characters, not null-terminated. */
struct buffer {
	const char *st;		/* start of the usable text */
	char *mem;
	size_t len;		/* length of the usable text, from 'mem' */
	size_t cap;
};

extern FILE *input;
extern FILE *output;

/*
 * The current line from the input file, used by the lexer to generate tokens.
 * To read from the line, start at inp.st and continue up to and including the
 * next '\n'. To read beyond the '\n', call inp_skip or inp_next, which will
 * make the next line available, invalidating any pointers into the previous
 * line.
 */
extern struct buffer inp;

extern struct buffer token;	/* the current token to be processed, is
				 * typically copied to the buffer 'code', or in
				 * some cases to 'lab'. */

extern struct buffer lab;	/* the label or preprocessor directive */
extern struct buffer code;	/* the main part of the current line of code,
				 * containing declarations or statements */
extern struct buffer com;	/* the trailing comment of the line, or the
				 * start or end of a multi-line comment, or
				 * while in process_comment, a single line of a
				 * multi-line comment */

extern struct options {
	bool blanklines_around_conditional_compilation;
	bool blank_line_after_decl_at_top;	/* this is vaguely similar to
						 * blank_line_after_decl except
						 * that it only applies to the
						 * first set of declarations in
						 * a procedure (just after the
						 * first '{') and it causes a
						 * blank line to be generated
						 * even if there are no
						 * declarations */
	bool blank_line_after_decl;
	bool blanklines_after_procs;
	bool blanklines_before_block_comments;
	bool break_after_comma;	/* whether to add a line break after each
				 * declarator */
	bool brace_same_line;	/* whether brace should be on same line as if,
				 * while, etc */
	bool blank_after_sizeof;	/* whether a blank should always be
					 * inserted after sizeof */
	bool comment_delimiter_on_blankline;
	int decl_comment_column;	/* the column in which comments after
					 * declarations should be put */
	bool cuddle_else;	/* whether 'else' should cuddle up to '}' */
	int continuation_indent;	/* the indentation between the edge of
					 * code and continuation lines */
	float case_indent;	/* The distance (measured in indentation
				 * levels) to indent case labels from the
				 * switch statement */
	int comment_column;	/* the column in which comments to the right of
				 * code should start */
	int decl_indent;	/* indentation of identifier in declaration */
	bool ljust_decl;	/* true if declarations should be left
				 * justified */
	int unindent_displace;	/* comments not to the right of code will be
				 * placed this many indentation levels to the
				 * left of code */
	bool extra_expr_indent;	/* whether continuation lines from the
				 * expression part of "if (e)", "while (e)",
				 * "for (e; e; e)" should be indented an extra
				 * tab stop so that they don't conflict with
				 * the code that follows */
	bool else_if;		/* whether else-if pairs use the same line */
	bool function_brace_split;	/* split function declaration and brace
					 * onto separate lines */
	bool format_col1_comments;	/* If comments which start in column 1
					 * are to be reformatted (just like
					 * comments that begin in later
					 * columns) */
	bool format_block_comments;	/* whether comments beginning with '/ *
					 * \n' are to be reformatted */
	bool indent_parameters;
	int indent_size;	/* the size of one indentation level */
	int block_comment_max_line_length;
	int local_decl_indent;	/* like decl_indent but for locals */
	bool lineup_to_parens_always;	/* whether to not(?) attempt to keep
					 * lined-up code within the margin */
	bool lineup_to_parens;	/* whether continued code within parens will be
				 * lined up to the open paren */
	bool proc_calls_space;	/* whether function calls look like: foo (bar)
				 * rather than foo(bar) */
	bool procnames_start_line;	/* whether the names of procedures
					 * being defined get placed in column 1
					 * (i.e. a newline is placed between
					 * the type of the procedure and its
					 * name) */
	bool space_after_cast;	/* "b = (int) a" vs "b = (int)a" */
	bool star_comment_cont;	/* whether comment continuation lines should
				 * have stars at the beginning of each line */
	bool swallow_optional_blanklines;
	bool auto_typedefs;	/* whether to recognize identifiers ending in
				 * "_t" like typedefs */
	int tabsize;		/* the size of a tab */
	int max_line_length;
	bool use_tabs;		/* set true to use tabs for spacing, false uses
				 * all spaces */
	bool verbose;		/* whether non-essential error messages are
				 * printed */
} opt;

extern bool found_err;
extern bool break_comma;	/* when true and not in parentheses, break
				 * after a comma */
extern float case_ind;		/* indentation level to be used for a "case n:"
				 */
extern bool had_eof;		/* whether input is exhausted */
extern int line_no;		/* the current line number. */
extern enum indent_enabled {
	indent_on,
	indent_off,
	indent_last_off_line,
} indent_enabled;

#define	STACKSIZE 256

/* Properties of each level of parentheses or brackets. */
typedef struct paren_level_props {
	int indent;		/* indentation of the operand/argument,
				 * relative to the enclosing statement; if
				 * negative, reflected at -1 */
	enum paren_level_cast {
		cast_unknown,
		cast_maybe,
		cast_no,
	} cast;			/* whether the parentheses form a type cast */
} paren_level_props;

/*
 * The parser state determines the layout of the formatted text.
 *
 * At each '#if', the parser state is copied so that the corresponding '#else'
 * lines start in the same state.
 *
 * In a function body, the number of block braces determines the indentation
 * of statements and declarations.
 *
 * In a statement, the number of parentheses or brackets determines the
 * indentation of follow-up lines.
 *
 * In an expression, the token type determine whether to put spaces around.
 *
 * In a source file, the types of line determine the vertical spacing, such as
 * around preprocessing directives or function bodies, or above block
 * comments.
 */
extern struct parser_state {
	lexer_symbol prev_token;	/* the previous token, but never
					 * comment, newline or preprocessing
					 * line */

	/* Token classification */

	int quest_level;	/* when this is positive, we have seen a '?'
				 * without the matching ':' in a '?:'
				 * expression */
	bool is_function_definition;	/* starts either at the 'name(' from a
					 * function definition if it occurs at
					 * the beginning of a line, or at the
					 * first '*' from inside a declaration
					 * when the line starts with words
					 * followed by a '('; ends at the end
					 * of that line */
	bool block_init;	/* whether inside a block initialization */
	int block_init_level;	/* the level of brace nesting in an
				 * initialization */
	bool init_or_struct;	/* whether there has been a type name and no
				 * left parenthesis since the last semicolon.
				 * When true, a '{' starts a structure
				 * definition or an initialization list */
	bool decl_on_line;	/* whether this line of code has part of a
				 * declaration on it */
	bool in_stmt_or_decl;	/* whether in a statement or a struct
				 * declaration or a plain declaration */
	bool in_decl;		/* whether we are in a declaration. The
				 * processing of braces is then slightly
				 * different */
	bool in_func_def_params;
	bool seen_case;		/* set to true when we see a 'case', so we know
				 * what to do with the following colon */
	bool is_case_label;	/* 'case' and 'default' labels are indented
				 * differently from regular labels */
	parser_symbol spaced_expr_psym;	/* the parser symbol to be shifted
					 * after the parenthesized expression
					 * from a 'for', 'if', 'switch' or
					 * 'while'; or psym_0 */

	/* Indentation of statements and declarations */

	int ind_level;		/* the indentation level for the line that is
				 * currently prepared for output */
	int ind_level_follow;	/* the level to which ind_level should be set
				 * after the current line is printed */
	bool in_stmt_cont;	/* whether the current line should have an
				 * extra indentation level because we are in
				 * the middle of a statement */
	int decl_level;		/* current nesting level for a structure
				 * declaration or an initializer */
	int di_stack[20];	/* a stack of structure indentation levels */
	bool decl_indent_done;	/* whether the indentation for a declaration
				 * has been added to the code buffer. */
	int decl_ind;		/* current indentation for declarations */
	bool tabs_to_var;	/* true if using tabs to indent to var name */

	enum {
		eei_no,
		eei_yes,
		eei_last
	} extra_expr_indent;

	enum {
		in_enum_no,	/* outside any 'enum { ... }' */
		in_enum_enum,	/* after keyword 'enum' */
		in_enum_type,	/* after 'enum' or 'enum tag' */
		in_enum_brace	/* between '{' and '}' */
	} in_enum;		/* enum { . } */

	int tos;		/* pointer to top of stack */
	parser_symbol s_sym[STACKSIZE];
	int s_ind_level[STACKSIZE];
	float s_case_ind_level[STACKSIZE];

	/* Spacing inside a statement or declaration */

	bool next_unary;	/* whether the following operator should be
				 * unary; is used in declarations for '*', as
				 * well as in expressions */
	bool want_blank;	/* whether the following token should be
				 * prefixed by a blank. (Said prefixing is
				 * ignored in some cases.) */
	int line_start_nparen;	/* the number of parentheses or brackets that
				 * were open at the beginning of the current
				 * line; used to indent within statements,
				 * initializers and declarations */
	int nparen;		/* the number of parentheses or brackets that
				 * are currently open; used to indent the
				 * remaining lines of the statement,
				 * initializer or declaration */
	paren_level_props paren[20];

	/* Horizontal spacing for comments */

	int comment_delta;	/* used to set up indentation for all lines of
				 * a boxed comment after the first one */
	int n_comment_delta;	/* remembers how many columns there were before
				 * the start of a box comment so that
				 * forthcoming lines of the comment are
				 * indented properly */
	int com_ind;		/* indentation of the current comment */

	/* Vertical spacing */

	bool force_nl;		/* whether the next token is forced to go to a
				 * new line; used after 'if (expr)' and in
				 * similar situations; tokens like '{' may
				 * ignore this */

	enum declaration {
		decl_no,	/* no declaration anywhere nearby */
		decl_begin,	/* collecting tokens of a declaration */
		decl_end,	/* finished a declaration */
	} declaration;
	bool blank_line_after_decl;

	/* Comments */

	bool curr_col_1;	/* whether the current token started in column
				 * 1 of the original input */
	bool next_col_1;
} ps;

extern struct output_state {
	enum line_kind {
		lk_other,
		lk_if,		/* #if, #ifdef, #ifndef */
		lk_endif,	/* #endif */
		lk_func_end,	/* the last '}' of a function body */
		lk_block_comment,
	} line_kind;		/* kind of the current output line, is reset to
				 * lk_other at the beginning of each output
				 * line; used for inserting blank lines */
	enum line_kind prev_line_kind;

	struct buffer indent_off_text;	/* text from between 'INDENT OFF' and
					 * 'INDENT ON', both inclusive */
} out;


#define array_length(array) (sizeof(array) / sizeof((array)[0]))

#ifdef debug
void debug_printf(const char *, ...) __printflike(1, 2);
void debug_println(const char *, ...) __printflike(1, 2);
void debug_vis_range(const char *, const char *, size_t, const char *);
void debug_parser_state(lexer_symbol);
void debug_parse_stack(const char *);
void debug_buffers(void);
extern const char *const lsym_name[];
extern const char *const psym_name[];
extern const char *const paren_level_cast_name[];
extern const char *const line_kind_name[];
#else
#define debug_noop() do { } while (false)
#define	debug_printf(fmt, ...) debug_noop()
#define	debug_println(fmt, ...) debug_noop()
#define	debug_vis_range(prefix, s, e, suffix) debug_noop()
#define	debug_parser_state(lsym) debug_noop()
#define	debug_parse_stack(situation) debug_noop()
#define	debug_buffers() debug_noop()
#endif

void register_typename(const char *);
int compute_code_indent(void);
int compute_label_indent(void);
int ind_add(int, const char *, size_t);

void inp_skip(void);
char inp_next(void);

lexer_symbol lexi(void);
void diag(int, const char *, ...) __printflike(2, 3);
void output_line(void);
void inp_read_line(void);
void parse(parser_symbol);
void process_comment(void);
void set_option(const char *, const char *);
void load_profile_files(const char *);

void *nonnull(void *);

void buf_add_char(struct buffer *, char);
void buf_add_chars(struct buffer *, const char *, size_t);

static inline bool
ch_isalnum(char ch)
{
	return isalnum((unsigned char)ch) != 0;
}

static inline bool
ch_isalpha(char ch)
{
	return isalpha((unsigned char)ch) != 0;
}

static inline bool
ch_isblank(char ch)
{
	return ch == ' ' || ch == '\t';
}

static inline bool
ch_isdigit(char ch)
{
	return '0' <= ch && ch <= '9';
}

static inline bool
ch_isspace(char ch)
{
	return isspace((unsigned char)ch) != 0;
}

static inline int
next_tab(int ind)
{
	return ind - ind % opt.tabsize + opt.tabsize;
}
