/*	$NetBSD: indent_globs.h,v 1.33 2021/09/25 18:49:03 rillig Exp $	*/

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
 *
 *	@(#)indent_globs.h	8.1 (Berkeley) 6/6/93
 * $FreeBSD: head/usr.bin/indent/indent_globs.h 337651 2018-08-11 19:20:06Z pstef $
 */

#define bufsize 200		/* size of internal buffers */
#define sc_size 5000		/* size of save_com buffer */
#define label_offset 2		/* number of levels a label is placed to left
				 * of code */


struct buffer {
    char *buf;			/* buffer */
    char *s;			/* start */
    char *e;			/* end */
    char *l;			/* limit */
};

extern FILE       *input;		/* the fid for the input file */
extern FILE       *output;		/* the output file */

extern struct buffer lab;		/* label or preprocessor directive */
extern struct buffer code;		/* code */
extern struct buffer com;		/* comment */
extern struct buffer token;		/* the last token scanned */

extern char       *in_buffer;		/* input buffer */
extern char	   *in_buffer_limit;	/* the end of the input buffer */
extern char       *buf_ptr;		/* ptr to next character to be taken from
				 * in_buffer */
extern char       *buf_end;		/* ptr to first after last char in in_buffer */

extern char        sc_buf[sc_size];	/* input text is saved here when looking for
				 * the brace after an if, while, etc */
extern char       *save_com;		/* start of the comment stored in sc_buf */
extern char       *sc_end;		/* pointer into save_com buffer */

extern char       *bp_save;		/* saved value of buf_ptr when taking input
				 * from save_com */
extern char       *be_save;		/* similarly saved value of buf_end */


extern struct options {
    bool	blanklines_around_conditional_compilation;
    bool	blanklines_after_declarations_at_proctop; /* this is vaguely
				 * similar to blanklines_after_declarations
				 * except that it only applies to the first
				 * set of declarations in a procedure (just
				 * after the first '{') and it causes a blank
				 * line to be generated even if there are no
				 * declarations */
    bool	blanklines_after_declarations;
    bool	blanklines_after_procs;
    bool	blanklines_before_blockcomments;
    bool	leave_comma;	/* if true, never break declarations after
				 * commas */
    bool	btype_2;	/* whether brace should be on same line
				 * as if, while, etc */
    bool	blank_after_sizeof; /* whether a blank should always be
				 * inserted after sizeof */
    bool	comment_delimiter_on_blankline;
    int         decl_comment_column; /* the column in which comments after
				 * declarations should be put */
    bool	cuddle_else;	/* whether 'else' should cuddle up to '}' */
    int         continuation_indent; /* set to the indentation between the
				 * edge of code and continuation lines */
    float       case_indent;	/* The distance (measured in tabsize) to
				 * indent case labels from the switch
				 * statement */
    int         comment_column;	/* the column in which comments to the right
				 * of code should start */
    int         decl_indent;	/* indentation of identifier in declaration */
    bool	ljust_decl;	/* true if declarations should be left
				 * justified */
    int         unindent_displace; /* comments not to the right of code
				 * will be placed this many
				 * indentation levels to the left of
				 * code */
    bool	extra_expression_indent; /* whether continuation lines from
				 * the expression part of "if(e)",
				 * "while(e)", "for(e;e;e)" should be
				 * indented an extra tab stop so that they
				 * don't conflict with the code that follows */
    bool	else_if;	/* whether else-if pairs should be handled
				 * specially */
    bool	function_brace_split; /* split function declaration and
				 * brace onto separate lines */
    bool	format_col1_comments; /* If comments which start in column 1
				 * are to be magically reformatted (just
				 * like comments that begin in later columns) */
    bool	format_block_comments; /* whether comments beginning with
				 * '/ * \n' are to be reformatted */
    bool	indent_parameters;
    int         indent_size;	/* the size of one indentation level */
    int         block_comment_max_line_length;
    int         local_decl_indent; /* like decl_indent but for locals */
    bool	lineup_to_parens_always; /* whether to not(?) attempt to keep
				 * lined-up code within the margin */
    bool	lineup_to_parens; /* whether continued code within parens
				 * will be lined up to the open paren */
    bool	proc_calls_space; /* whether procedure calls look like:
				 * foo (bar) rather than foo(bar) */
    bool	procnames_start_line; /* whether, the names of procedures
				 * being defined get placed in column 1 (i.e.
				 * a newline is placed between the type of
				 * the procedure and its name) */
    bool	space_after_cast; /* "b = (int) a" vs "b = (int)a" */
    bool	star_comment_cont; /* whether comment continuation lines
				 * should have stars at the beginning of
				 * each line. */
    bool	swallow_optional_blanklines;
    bool	auto_typedefs;	/* whether to recognize identifiers
				 * ending in "_t" like typedefs */
    int         tabsize;	/* the size of a tab */
    int         max_line_length;
    bool	use_tabs;	/* set true to use tabs for spacing, false
				 * uses all spaces */
    bool	verbose;	/* whether non-essential error messages
				 * are printed */
} opt;

enum rwcode {
    rw_0,
    rw_offsetof,
    rw_sizeof,
    rw_struct_or_union_or_enum,
    rw_type,
    rw_for_or_if_or_while,
    rw_do_or_else,
    rw_switch,
    rw_case_or_default,
    rw_jump,
    rw_storage_class,
    rw_typedef,
    rw_inline_or_restrict
};


extern int         found_err;
extern int         n_real_blanklines;
extern bool        prefix_blankline_requested;
extern bool        postfix_blankline_requested;
extern bool        break_comma;	/* when true and not in parens, break after a
				 * comma */
extern float       case_ind;		/* indentation level to be used for a "case
				 * n:" */
extern bool        had_eof;		/* set to true when input is exhausted */
extern int         line_no;		/* the current line number. */
extern bool        inhibit_formatting;	/* true if INDENT OFF is in effect */
extern int         suppress_blanklines;/* set iff following blanklines should be
				 * suppressed */

#define	STACKSIZE 256

extern struct parser_state {
    token_type  last_token;
    token_type	p_stack[STACKSIZE];	/* this is the parsers stack */
    int         il[STACKSIZE];	/* this stack stores indentation levels */
    float       cstk[STACKSIZE];/* used to store case stmt indentation levels */
    bool	box_com;	/* whether we are in a "boxed" comment. In
				 * that case, the first non-blank char should
				 * be lined up with the '/' in '/' + '*' */
    int         comment_delta;	/* used to set up indentation for all lines
				 * of a boxed comment after the first one */
    int         n_comment_delta;/* remembers how many columns there were
				 * before the start of a box comment so that
				 * forthcoming lines of the comment are
				 * indented properly */
    int         cast_mask;	/* indicates which close parens potentially
				 * close off casts */
    int         not_cast_mask;	/* indicates which close parens definitely
				 * close off something else than casts */
    bool	block_init;	/* whether inside a block initialization */
    int         block_init_level;	/* The level of brace nesting in an
					 * initialization */
    bool	last_nl;	/* this is true if the last thing scanned was
				 * a newline */
    bool	in_or_st;	/* Will be true iff there has been a
				 * declarator (e.g. int or char) and no left
				 * paren since the last semicolon. When true,
				 * a '{' is starting a structure definition or
				 * an initialization list */
    bool	bl_line;	/* set to 1 by dump_line if the line is blank */
    bool	col_1;		/* set to true if the last token started in
				 * column 1 */
    int         com_col;	/* this is the column in which the current
				 * comment should start */
    int         dec_nest;	/* current nesting level for structure or init */
    bool	decl_on_line;	/* set to true if this line of code has part
				 * of a declaration on it */
    int         i_l_follow;	/* the level to which ind_level should be set
				 * after the current line is printed */
    bool	in_decl;	/* set to true when we are in a declaration
				 * stmt.  The processing of braces is then
				 * slightly different */
    bool	in_stmt;	/* set to 1 while in a stmt */
    int         ind_level;	/* the current indentation level */
    bool	ind_stmt;	/* set to 1 if next line should have an extra
				 * indentation level because we are in the
				 * middle of a stmt */
    bool	last_u_d;	/* set to true after scanning a token which
				 * forces a following operator to be unary */
    int         p_l_follow;	/* used to remember how to indent the
				 * following statement */
    int         paren_level;	/* parenthesization level. used to indent
				 * within statements */
    short       paren_indents[20]; /* indentation of the operand/argument of
				 * each level of parentheses or brackets,
				 * relative to the enclosing statement */
    bool	pcase;		/* set to 1 if the current line label is a
				 * case.  It is printed differently from a
				 * regular label */
    bool	search_brace;	/* set to true by parse when it is necessary
				 * to buffer up all info up to the start of a
				 * stmt after an if, while, etc */
    bool	use_ff;		/* set to one if the current line should be
				 * terminated with a form feed */
    bool	want_blank;	/* set to true when the following token should
				 * be prefixed by a blank. (Said prefixing is
				 * ignored in some cases.) */
    enum rwcode keyword;	/* the type of a keyword or 0 */
    bool	dumped_decl_indent;
    bool	in_parameter_declaration;
    int         tos;		/* pointer to top of stack */
    char        procname[100];	/* The name of the current procedure */
    int         just_saw_decl;

    struct {
	int	comments;
	int	lines;
	int	code_lines;
	int	comment_lines;
    }		stats;
}           ps;

extern int         ifdef_level;
extern struct parser_state state_stack[5];
extern struct parser_state match_state[5];
