/*	$NetBSD: indent.h,v 1.52 2021/10/26 20:43:35 rillig Exp $	*/

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

#if 0
__FBSDID("$FreeBSD: head/usr.bin/indent/indent.h 336333 2018-07-16 05:46:50Z pstef $");
#endif

#include <stdbool.h>

typedef enum lexer_symbol {
    lsym_eof,
    lsym_preprocessing,		/* '#' */
    lsym_newline,
    lsym_form_feed,
    lsym_comment,
    lsym_lparen_or_lbracket,
    lsym_rparen_or_rbracket,
    lsym_lbrace,
    lsym_rbrace,
    lsym_period,
    lsym_unary_op,		/* e.g. '+' or '&' */
    lsym_binary_op,		/* e.g. '<<' or '+' or '&&' or '/=' */
    lsym_postfix_op,		/* trailing '++' or '--' */
    lsym_question,		/* the '?' from a '?:' expression */
    lsym_colon,
    lsym_comma,
    lsym_semicolon,
    lsym_typedef,
    lsym_storage_class,
    lsym_type,
    lsym_tag,			/* 'struct', 'union', 'enum' */
    lsym_case_label,
    lsym_string_prefix,		/* 'L' */
    lsym_ident,			/* identifier, constant or string */
    lsym_funcname,
    lsym_do,
    lsym_else,
    lsym_for,
    lsym_if,
    lsym_switch,
    lsym_while,
} lexer_symbol;

typedef enum parser_symbol {
    psym_semicolon,		/* rather a placeholder than a semicolon */
    psym_lbrace,
    psym_rbrace,
    psym_decl,
    psym_stmt,
    psym_stmt_list,
    psym_for_exprs,		/* 'for' '(' ... ')' */
    psym_if_expr,		/* 'if' '(' expr ')' */
    psym_if_expr_stmt,		/* 'if' '(' expr ')' stmt */
    psym_if_expr_stmt_else,	/* 'if' '(' expr ')' stmt 'else' */
    psym_else,			/* 'else' */
    psym_switch_expr,		/* 'switch' '(' expr ')' */
    psym_do,			/* 'do' */
    psym_do_stmt,		/* 'do' stmt */
    psym_while_expr,		/* 'while' '(' expr ')' */
} parser_symbol;

typedef enum stmt_head {
    hd_0,			/* placeholder for uninitialized */
    hd_for,
    hd_if,
    hd_switch,
    hd_while,
} stmt_head;

#define sc_size 5000		/* size of save_com buffer */
#define label_offset 2		/* number of levels a label is placed to left
				 * of code */


struct buffer {
    char *buf;			/* buffer */
    char *s;			/* start */
    char *e;			/* end */
    char *l;			/* limit */
};

extern FILE *input;
extern FILE *output;

extern struct buffer lab;	/* label or preprocessor directive */
extern struct buffer code;	/* code */
extern struct buffer com;	/* comment */
extern struct buffer token;	/* the last token scanned */

extern struct buffer inp;

extern char sc_buf[sc_size];	/* input text is saved here when looking for
				 * the brace after an if, while, etc */
extern char *save_com;		/* start of the comment stored in sc_buf */

extern char *saved_inp_s;	/* saved value of inp.s when taking input from
				 * save_com */
extern char *saved_inp_e;	/* similarly saved value of inp.e */


extern struct options {
    bool blanklines_around_conditional_compilation;
    bool blanklines_after_decl_at_top;	/* this is vaguely similar to
					 * blanklines_after_decl except that
					 * it only applies to the first set of
					 * declarations in a procedure (just
					 * after the first '{') and it causes
					 * a blank line to be generated even
					 * if there are no declarations */
    bool blanklines_after_decl;
    bool blanklines_after_procs;
    bool blanklines_before_block_comments;
    bool break_after_comma;	/* whether to break declarations after commas */
    bool brace_same_line;	/* whether brace should be on same line as if,
				 * while, etc */
    bool blank_after_sizeof;	/* whether a blank should always be inserted
				 * after sizeof */
    bool comment_delimiter_on_blankline;
    int decl_comment_column;	/* the column in which comments after
				 * declarations should be put */
    bool cuddle_else;		/* whether 'else' should cuddle up to '}' */
    int continuation_indent;	/* the indentation between the edge of code
				 * and continuation lines */
    float case_indent;		/* The distance (measured in indentation
				 * levels) to indent case labels from the
				 * switch statement */
    int comment_column;		/* the column in which comments to the right
				 * of code should start */
    int decl_indent;		/* indentation of identifier in declaration */
    bool ljust_decl;		/* true if declarations should be left
				 * justified */
    int unindent_displace;	/* comments not to the right of code will be
				 * placed this many indentation levels to the
				 * left of code */
    bool extra_expr_indent;	/* whether continuation lines from the
				 * expression part of "if(e)", "while(e)",
				 * "for(e;e;e)" should be indented an extra
				 * tab stop so that they don't conflict with
				 * the code that follows */
    bool else_if;		/* whether else-if pairs should be handled
				 * specially */
    bool function_brace_split;	/* split function declaration and brace onto
				 * separate lines */
    bool format_col1_comments;	/* If comments which start in column 1 are to
				 * be magically reformatted (just like
				 * comments that begin in later columns) */
    bool format_block_comments;	/* whether comments beginning with '/ * \n'
				 * are to be reformatted */
    bool indent_parameters;
    int indent_size;		/* the size of one indentation level */
    int block_comment_max_line_length;
    int local_decl_indent;	/* like decl_indent but for locals */
    bool lineup_to_parens_always;	/* whether to not(?) attempt to keep
					 * lined-up code within the margin */
    bool lineup_to_parens;	/* whether continued code within parens will
				 * be lined up to the open paren */
    bool proc_calls_space;	/* whether function calls look like: foo (bar)
				 * rather than foo(bar) */
    bool procnames_start_line;	/* whether the names of procedures being
				 * defined get placed in column 1 (i.e. a
				 * newline is placed between the type of the
				 * procedure and its name) */
    bool space_after_cast;	/* "b = (int) a" vs "b = (int)a" */
    bool star_comment_cont;	/* whether comment continuation lines should
				 * have stars at the beginning of each line. */
    bool swallow_optional_blanklines;
    bool auto_typedefs;		/* whether to recognize identifiers ending in
				 * "_t" like typedefs */
    int tabsize;		/* the size of a tab */
    int max_line_length;
    bool use_tabs;		/* set true to use tabs for spacing, false
				 * uses all spaces */
    bool verbose;		/* whether non-essential error messages are
				 * printed */
} opt;

enum keyword_kind {
    kw_0,
    kw_offsetof,
    kw_sizeof,
    kw_struct_or_union_or_enum,
    kw_type,
    kw_for,
    kw_if,
    kw_while,
    kw_do,
    kw_else,
    kw_switch,
    kw_case_or_default,
    kw_jump,
    kw_storage_class,
    kw_typedef,
    kw_inline_or_restrict
};


extern bool found_err;
extern int blank_lines_to_output;
extern bool blank_line_before;
extern bool blank_line_after;
extern bool break_comma;	/* when true and not in parens, break after a
				 * comma */
extern float case_ind;		/* indentation level to be used for a "case
				 * n:" */
extern bool had_eof;		/* whether input is exhausted */
extern int line_no;		/* the current line number. */
extern bool inhibit_formatting;	/* true if INDENT OFF is in effect */

#define	STACKSIZE 256

/* TODO: group the members by purpose, don't sort them alphabetically */
extern struct parser_state {
    lexer_symbol last_token;

    int tos;			/* pointer to top of stack */
    parser_symbol s_sym[STACKSIZE];
    int s_ind_level[STACKSIZE];
    float s_case_ind_level[STACKSIZE];

    int comment_delta;		/* used to set up indentation for all lines of
				 * a boxed comment after the first one */
    int n_comment_delta;	/* remembers how many columns there were
				 * before the start of a box comment so that
				 * forthcoming lines of the comment are
				 * indented properly */
    int cast_mask;		/* indicates which close parens potentially
				 * close off casts */
    int not_cast_mask;		/* indicates which close parens definitely
				 * close off something else than casts */
    bool block_init;		/* whether inside a block initialization */
    int block_init_level;	/* The level of brace nesting in an
				 * initialization */
    bool last_nl;		/* whether the last thing scanned was a
				 * newline */
    bool init_or_struct;	/* whether there has been a declarator (e.g.
				 * int or char) and no left parenthesis since
				 * the last semicolon. When true, a '{' is
				 * starting a structure definition or an
				 * initialization list */
    bool col_1;			/* whether the last token started in column 1 */
    int com_ind;		/* indentation of the current comment */
    int decl_nest;		/* current nesting level for structure or init */
    bool decl_on_line;		/* whether this line of code has part of a
				 * declaration on it */
    int ind_level_follow;	/* the level to which ind_level should be set
				 * after the current line is printed */
    bool in_decl;		/* whether we are in a declaration stmt. The
				 * processing of braces is then slightly
				 * different */
    bool in_stmt;
    int ind_level;		/* the current indentation level */
    bool ind_stmt;		/* whether the next line should have an extra
				 * indentation level because we are in the
				 * middle of a stmt */
    bool next_unary;		/* whether the following operator should be
				 * unary */
    int p_l_follow;		/* used to remember how to indent the
				 * following statement */
    int paren_level;		/* parenthesization level. used to indent
				 * within statements */
    short paren_indents[20];	/* indentation of the operand/argument of each
				 * level of parentheses or brackets, relative
				 * to the enclosing statement */
    bool is_case_label;		/* 'case' and 'default' labels are indented
				 * differently from regular labels */
    bool search_stmt;		/* whether it is necessary to buffer up all
				 * text up to the start of a statement after
				 * an 'if', 'while', etc. */
    bool want_blank;		/* whether the following token should be
				 * prefixed by a blank. (Said prefixing is
				 * ignored in some cases.) */
    enum keyword_kind prev_keyword;
    enum keyword_kind curr_keyword;
    bool dumped_decl_indent;
    bool in_parameter_declaration;
    char procname[100];		/* The name of the current procedure */
    int just_saw_decl;

    struct {
	int comments;
	int lines;
	int code_lines;
	int comment_lines;
    }      stats;
} ps;


#define array_length(array) (sizeof (array) / sizeof (array[0]))

void add_typename(const char *);
int compute_code_indent(void);
int compute_label_indent(void);
int indentation_after_range(int, const char *, const char *);
int indentation_after(int, const char *);
#ifdef debug
void
debug_vis_range(const char *, const char *, const char *,
    const char *);
void debug_printf(const char *, ...)__printflike(1, 2);
void debug_println(const char *, ...)__printflike(1, 2);
#else
#define		debug_printf(fmt, ...) do { } while (false)
#define		debug_println(fmt, ...) do { } while (false)
#define		debug_vis_range(prefix, s, e, suffix) do { } while (false)
#endif
void inbuf_skip(void);
char inbuf_next(void);
lexer_symbol lexi(struct parser_state *);
void diag(int, const char *, ...)__printflike(2, 3);
void dump_line(void);
void dump_line_ff(void);
void inbuf_read_line(void);
void parse(parser_symbol);
void parse_hd(stmt_head);
void process_comment(void);
void set_option(const char *, const char *);
void load_profiles(const char *);

void *xmalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *);

void buf_expand(struct buffer *, size_t);

static inline bool
is_hspace(char ch)
{
    return ch == ' ' || ch == '\t';
}

static inline int
next_tab(int ind)
{
    return ind - ind % opt.tabsize + opt.tabsize;
}
