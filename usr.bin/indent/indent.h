/*	$NetBSD: indent.h,v 1.202 2023/06/16 23:51:32 rillig Exp $	*/

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
	lsym_preprocessing,	/* the initial '#' of a preprocessing line */
	lsym_newline,		/* outside block comments */
	lsym_comment,		/* the initial '/ *' or '//' of a comment */

	lsym_lparen,
	lsym_rparen,
	lsym_lbracket,
	lsym_rbracket,
	lsym_lbrace,
	lsym_rbrace,

	lsym_period,
	lsym_unary_op,		/* e.g. '*', '&', '-' or leading '++' */
	lsym_sizeof,
	lsym_offsetof,
	lsym_postfix_op,	/* trailing '++' or '--' */
	lsym_binary_op,		/* e.g. '*', '&', '<<', '&&' or '/=' */
	lsym_question,		/* the '?' from a '?:' expression */
	lsym_question_colon,	/* the ':' from a '?:' expression */
	lsym_comma,

	lsym_typedef,
	lsym_modifier,		/* modifiers for types, functions, variables */
	lsym_tag,		/* 'struct', 'union' or 'enum' */
	lsym_type,
	lsym_word,		/* identifier, constant or string */
	lsym_funcname,		/* name of a function being defined */
	lsym_label_colon,	/* the ':' after a label */
	lsym_other_colon,	/* bit-fields, generic-association (C11),
				 * enum-type-specifier (C23),
				 * attribute-prefixed-token (C23),
				 * pp-prefixed-parameter (C23 6.10) */
	lsym_semicolon,

	lsym_case,
	lsym_default,
	lsym_do,
	lsym_else,
	lsym_for,
	lsym_if,
	lsym_switch,
	lsym_while,
	lsym_return,
} lexer_symbol;

/*
 * Structure of the source code, in terms of declarations, statements and
 * braces; used to determine the indentation level of these parts.
 */
typedef enum parser_symbol {
	psym_0,			/* a placeholder; not stored on the stack */
	psym_lbrace_block,	/* '{' for a block of code */
	psym_lbrace_struct,	/* '{' in 'struct ... { ... }' */
	psym_lbrace_union,	/* '{' in 'union ... { ... }' */
	psym_lbrace_enum,	/* '{' in 'enum ... { ... }' */
	psym_rbrace,		/* not stored on the stack */
	psym_decl,
	psym_stmt,
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

/* A range of characters, only null-terminated in debug mode. */
struct buffer {
	char *s;
	size_t len;
	size_t cap;
};

extern FILE *input;
extern FILE *output;

/*
 * The current line from the input file, used by the lexer to generate tokens.
 * To read from the line, start at inp_p and continue up to and including the
 * next '\n'. To read beyond the '\n', call inp_skip or inp_next, which will
 * make the next line available, invalidating any pointers into the previous
 * line.
 */
extern struct buffer inp;
extern const char *inp_p;

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
	bool blank_line_around_conditional_compilation;
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
	bool blank_line_after_proc;
	bool blank_line_before_block_comment;
	bool break_after_comma;	/* whether to add a line break after each
				 * declarator */
	bool brace_same_line;	/* whether a brace should be on same line as an
				 * if, while, etc. */
	bool blank_after_sizeof;
	bool comment_delimiter_on_blank_line;
	int decl_comment_column;	/* the column in which comments after
					 * declarations should be put */
	bool cuddle_else;	/* whether 'else' should cuddle up to '}' */
	int continuation_indent;	/* the indentation between the edge of
					 * code and continuation lines */
	float case_indent;	/* the distance (measured in indentation
				 * levels) to indent case labels from the
				 * switch statement */
	int comment_column;	/* the column in which comments to the right of
				 * code should start */
	int decl_indent;	/* indentation of identifier in declaration */
	bool left_justify_decl;
	int unindent_displace;	/* comments not to the right of code will be
				 * placed this many indentation levels to the
				 * left of code */
	bool extra_expr_indent;	/* whether continuation lines from the
				 * expression part of "if (e)", "while (e)",
				 * "for (e; e; e)" should be indented an extra
				 * tab stop so that they are not confused with
				 * the code that follows */
	bool else_if_in_same_line;
	bool function_brace_split;	/* split function declaration and brace
					 * onto separate lines */
	bool format_col1_comments;	/* whether comments that start in
					 * column 1 are to be reformatted (just
					 * like comments that begin in later
					 * columns) */
	bool format_block_comments;	/* whether to reformat comments that
					 * begin with '/ * \n' */
	bool indent_parameters;
	int indent_size;	/* the size of one indentation level */
	int block_comment_max_line_length;
	int local_decl_indent;	/* like decl_indent but for locals */
	bool lineup_to_parens_always;	/* whether to not(?) attempt to keep
					 * lined-up code within the margin */
	bool lineup_to_parens;	/* whether continued code within parens will be
				 * lined up to the open paren */
	bool proc_calls_space;	/* whether function calls look like 'foo (bar)'
				 * rather than 'foo(bar)' */
	bool procnames_start_line;	/* whether the names of functions being
					 * defined get placed in column 1 (i.e.
					 * a newline is placed between the type
					 * of the function and its name) */
	bool space_after_cast;	/* "b = (int) a" vs. "b = (int)a" */
	bool star_comment_cont;	/* whether comment continuation lines should
				 * have stars at the beginning of each line */
	bool swallow_optional_blank_lines;
	bool auto_typedefs;	/* whether to recognize identifiers ending in
				 * "_t" like typedefs */
	int tabsize;		/* the size of a tab */
	int max_line_length;
	bool use_tabs;		/* set true to use tabs for spacing, false uses
				 * all spaces */
	bool verbose;		/* print configuration to stderr */
} opt;

extern bool found_err;
extern bool had_eof;		/* whether input is exhausted */
extern int line_no;		/* the current input line number */
extern enum indent_enabled {
	indent_on,
	indent_off,
	indent_last_off_line,
} indent_enabled;

/* Properties of each level of parentheses or brackets. */
struct paren_level {
	int indent;		/* indentation of the operand/argument,
				 * relative to the enclosing statement; if
				 * negative, reflected at -1 */
	enum paren_level_cast {
		cast_unknown,
		cast_maybe,
		cast_no,
	} cast;			/* whether the parentheses form a type cast */
};

struct psym_stack {
	parser_symbol *sym;
	int *ind_level;
	size_t len;		/* points to one behind the top of the stack; 1
				 * at the top level of the file outside a
				 * declaration or statement; 2 at the top level
				 */
	size_t cap;
};

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
	lexer_symbol prev_lsym;	/* the previous token, but never comment,
				 * newline or preprocessing line */

	/* Token classification */

	bool in_stmt_or_decl;	/* whether in a statement or a struct
				 * declaration or a plain declaration */
	bool in_decl;		/* XXX: double-check the exact meaning */
	bool in_typedef_decl;
	bool in_var_decl;	/* starts at a type name or a '){' from a
				 * compound literal; ends at the '(' from a
				 * function definition or a ';' outside '{}';
				 * when active, '{}' form struct or union
				 * declarations, ':' marks a bit-field, and '='
				 * starts an initializer */
	bool in_init;		/* whether inside an initializer */
	int init_level;		/* the number of '{}' in an initializer */
	bool line_has_func_def;	/* starts either at the 'name(' from a function
				 * definition if it occurs at the beginning of
				 * a line, or at the first '*' from inside a
				 * declaration when the line starts with words
				 * followed by a '(' */
	bool in_func_def_params;	/* for old-style functions */
	bool line_has_decl;	/* whether this line of code has part of a
				 * declaration on it; used for indenting
				 * comments */
	parser_symbol lbrace_kind;	/* the kind of brace to be pushed to
					 * the parser symbol stack next */
	parser_symbol spaced_expr_psym;	/* the parser symbol to be shifted
					 * after the parenthesized expression
					 * from a 'for', 'if', 'switch' or
					 * 'while'; or psym_0 */
	bool seen_case;		/* whether there was a 'case' or 'default', to
				 * properly space the following ':' */
	bool prev_paren_was_cast;
	int quest_level;	/* when this is positive, we have seen a '?'
				 * without the matching ':' in a '?:'
				 * expression */

	/* Indentation of statements and declarations */

	int ind_level;		/* the indentation level for the line that is
				 * currently prepared for output */
	int ind_level_follow;	/* the level to which ind_level should be set
				 * after the current line is printed */
	bool line_is_stmt_cont;	/* whether the current line should have an
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
		eei_maybe,
		eei_last
	} extra_expr_indent;

	struct psym_stack psyms;

	/* Spacing inside a statement or declaration */

	bool next_unary;	/* whether the following operator should be
				 * unary; is used in declarations for '*', as
				 * well as in expressions */
	bool want_blank;	/* whether the following token should be
				 * prefixed by a blank. (Said prefixing is
				 * ignored in some cases.) */
	int ind_paren_level;	/* the number of parentheses or brackets that
				 * is used for indenting a continuation line of
				 * a declaration, initializer or statement */
	struct paren_stack {
		struct paren_level *item;
		size_t len;
		size_t cap;
	} paren;		/* the parentheses or brackets that are
				 * currently open; used to indent the remaining
				 * lines of the statement, initializer or
				 * declaration */

	/* Indentation of comments */

	int comment_ind;	/* total indentation of the current comment */
	int comment_shift;	/* all but the first line of a boxed comment
				 * are shifted this much to the right */
	bool comment_cont;	/* after the first line of a comment */

	/* Vertical spacing */

	bool break_after_comma;	/* whether to add a newline after the next
				 * comma; used in declarations but not in
				 * initializer lists */
	bool want_newline;	/* whether the next token should go to a new
				 * line; used after 'if (expr)' and in similar
				 * situations; tokens like '{' or ';' may
				 * ignore this */

	enum declaration {
		decl_no,	/* no declaration anywhere nearby */
		decl_begin,	/* collecting tokens of a declaration */
		decl_end,	/* finished a declaration */
	} declaration;
	bool blank_line_after_decl;
} ps;

extern struct output_state {
	enum line_kind {
		lk_other,
		lk_blank,
		lk_pre_if,	/* #if, #ifdef, #ifndef */
		lk_pre_endif,	/* #endif */
		lk_pre_other,	/* #else, #elif, #define, #undef */
		lk_stmt_head,	/* the ')' of an incomplete statement such as
				 * 'if (expr)' or 'for (expr; expr; expr)' */
		lk_func_end,	/* the last '}' of a function body */
		lk_block_comment,
		lk_case_or_default,
	} line_kind;		/* kind of the line that is being prepared for
				 * output; is reset to lk_other each time after
				 * trying to send a line to the output, even if
				 * that line was a suppressed blank line; used
				 * for inserting or removing blank lines */
	enum line_kind prev_line_kind;	/* the kind of line that was actually
					 * sent to the output */

	struct buffer indent_off_text;	/* text from between 'INDENT OFF' and
					 * 'INDENT ON', both inclusive */
} out;


#define array_length(array) (sizeof(array) / sizeof((array)[0]))

#ifdef debug
void debug_printf(const char *, ...) __printflike(1, 2);
void debug_println(const char *, ...) __printflike(1, 2);
void debug_blank_line(void);
void debug_vis_range(const char *, size_t);
void debug_parser_state(void);
void debug_psyms_stack(const char *);
void debug_print_buf(const char *, const struct buffer *);
void debug_buffers(void);
extern const char *const lsym_name[];
extern const char *const psym_name[];
extern const char *const paren_level_cast_name[];
extern const char *const line_kind_name[];
#else
#define debug_noop() do { } while (false)
#define	debug_printf(fmt, ...) debug_noop()
#define	debug_println(fmt, ...) debug_noop()
#define debug_blank_line() debug_noop()
#define	debug_vis_range(s, len) debug_noop()
#define	debug_parser_state() debug_noop()
#define	debug_psyms_stack(situation) debug_noop()
#define debug_print_buf(name, buf) debug_noop()
#define	debug_buffers() debug_noop()
#endif

void register_typename(const char *);
int compute_code_indent(void);
int compute_label_indent(void);
int ind_add(int, const char *, size_t);

void inp_skip(void);
char inp_next(void);
void finish_output(void);

lexer_symbol lexi(void);
void diag(int, const char *, ...) __printflike(2, 3);
void output_line(void);
void inp_read_line(void);
void parse(parser_symbol);
void process_comment(void);
void set_option(const char *, const char *);
void load_profile_files(const char *);
void ps_push(parser_symbol, bool);

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

#ifdef debug
void buf_terminate(struct buffer *);
#else
#define buf_terminate(buf) debug_noop()
#endif

static inline void
buf_clear(struct buffer *buf)
{
	buf->len = 0;
	buf_terminate(buf);
}
