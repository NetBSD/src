/* $NetBSD: token_comment.c,v 1.2 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for formatting comments.  C99 defines block comments and end-of-line
 * comments.  Indent further distinguishes box comments that are a special
 * kind of block comments.
 *
 * See opt-fc1, opt-nfc1.
 */

/*-
 * TODO: systematically test comments
 *
 * - starting in column 1, with opt.format_col1_comments
 * - starting in column 1, without opt.format_col1_comments
 * - starting in column 9, independent of opt.format_col1_comments
 * - starting in column 33, the default
 * - starting in column 65, which is already close to the default right margin
 * - starting in column 81, spilling into the right margin
 *
 * - block comment starting with '/' '*' '-'
 * - block comment starting with '/' '*' '*'
 * - block comment starting with '/' '*' '\n'
 * - end-of-line comment starting with '//'
 * - end-of-line comment starting with '//x', so without leading space
 * - block comment starting with '/' '*' 'x', so without leading space
 *
 * - block/end-of-line comment to the right of a label
 * - block/end-of-line comment to the right of code
 * - block/end-of-line comment to the right of label with code
 *
 * - with/without opt.comment_delimiter_on_blankline
 * - with/without opt.star_comment_cont
 * - with/without opt.format_block_comments
 * - with varying opt.max_line_length (32, 64, 80, 140)
 * - with varying opt.unindent_displace (0, 2, -5)
 * - with varying opt.indent_size (3, 4, 8)
 * - with varying opt.tabsize (3, 4, 8, 16)
 * - with varying opt.block_comment_max_line_length (60, 78, 90)
 * - with varying opt.decl_comment_column (0, 1, 33, 80)
 * - with/without ps.decl_on_line
 * - with/without ps.last_nl
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "indent.h"

static void
com_add_char(char ch)
{
    if (com.e + 1 >= com.l)
	buf_expand(&com, 1);
    *com.e++ = ch;
}

static void
com_add_delim(void)
{
    if (!opt.star_comment_cont)
	return;
    size_t len = 3;
    if (com.e + len >= com.l)
	buf_expand(&com, len);
    memcpy(com.e, " * ", len);
    com.e += len;
}

static void
com_terminate(void)
{
    if (com.e + 1 >= com.l)
	buf_expand(&com, 1);
    *com.e = '\0';
}

static bool
fits_in_one_line(int max_line_length)
{
    for (const char *p = inp.s; *p != '\n'; p++) {
	assert(*p != '\0');
	assert(inp.e - p >= 2);
	if (!(p[0] == '*' && p[1] == '/'))
	    continue;

	int len = indentation_after_range(ps.com_ind + 3, inp.s, p);
	len += is_hspace(p[-1]) ? 2 : 3;
	if (len <= max_line_length)
	    return true;
    }
    return false;
}

/*
 * Scan, reformat and output a single comment, which is either a block comment
 * starting with '/' '*' or an end-of-line comment starting with '//'.
 *
 * Try to keep comments from going over the maximum line length.  If a line is
 * too long, move everything starting from the last blank to the next comment
 * line.  Blanks and tabs from the beginning of the input line are removed.
 *
 * ALGORITHM:
 *	1) Decide where the comment should be aligned, and if lines should
 *	   be broken.
 *	2) If lines should not be broken and filled, just copy up to end of
 *	   comment.
 *	3) If lines should be filled, then scan through the input buffer,
 *	   copying characters to com_buf.  Remember where the last blank,
 *	   tab, or newline was.  When line is filled, print up to last blank
 *	   and continue copying.
 */
void
process_comment(void)
{
    int adj_max_line_length;	/* Adjusted max_line_length for comments that
				 * spill over the right margin */
    ssize_t last_blank;		/* index of the last blank in com.buf */
    bool break_delim = opt.comment_delimiter_on_blankline;
    int l_just_saw_decl = ps.just_saw_decl;
    int com_ind;

    adj_max_line_length = opt.max_line_length;
    ps.just_saw_decl = 0;
    last_blank = -1;		/* no blanks found so far */
    bool may_wrap = true;
    ps.stats.comments++;

    /* Figure where to align and how to treat the comment */

    if (ps.col_1 && !opt.format_col1_comments) {
	may_wrap = false;
	break_delim = false;
	com_ind = 0;

    } else {
	if (*inp.s == '-' || *inp.s == '*' || token.e[-1] == '/' ||
	    (*inp.s == '\n' && !opt.format_block_comments)) {
	    may_wrap = false;
	    break_delim = false;
	}

	if (lab.s == lab.e && code.s == code.e) {
	    com_ind = (ps.ind_level - opt.unindent_displace) * opt.indent_size;
	    adj_max_line_length = opt.block_comment_max_line_length;
	    if (com_ind <= 0)
		com_ind = opt.format_col1_comments ? 0 : 1;

	} else {
	    break_delim = false;

	    int target_ind;
	    if (code.s != code.e)
		target_ind = indentation_after(compute_code_indent(), code.s);
	    else if (lab.s != lab.e)
		target_ind = indentation_after(compute_label_indent(), lab.s);
	    else
		target_ind = 0;

	    com_ind = ps.decl_on_line || ps.ind_level == 0
		      ? opt.decl_comment_column - 1 : opt.comment_column - 1;
	    if (com_ind <= target_ind)
		com_ind = next_tab(target_ind);
	    if (com_ind + 25 > adj_max_line_length)
		adj_max_line_length = com_ind + 25;
	}
    }

    ps.com_ind = com_ind;

    if (!may_wrap) {
	/*
	 * Find out how much indentation there was originally, because that
	 * much will have to be ignored by dump_line(). This is a box comment,
	 * so nothing changes -- not even indentation.
	 *
	 * The comment we're about to read usually comes from inp.buf,
	 * unless it has been copied into save_com.
	 */
	const char *start;

	/*
	 * XXX: ordered comparison between pointers from different objects
	 * invokes undefined behavior (C99 6.5.8).
	 */
	start = inp.s >= save_com && inp.s < save_com + sc_size ?
		sc_buf : inp.buf;
	ps.n_comment_delta = -indentation_after_range(0, start, inp.s - 2);
    } else {
	ps.n_comment_delta = 0;
	while (is_hspace(*inp.s))
	    inp.s++;
    }

    ps.comment_delta = 0;
    com_add_char('/');
    com_add_char(token.e[-1]);	/* either '*' or '/' */
    if (*inp.s != ' ' && may_wrap)
	com_add_char(' ');

    if (break_delim && fits_in_one_line(adj_max_line_length))
	break_delim = false;

    if (break_delim) {
	char *t = com.e;
	com.e = com.s + 2;
	*com.e = '\0';
	if (opt.blanklines_before_block_comments && ps.last_token != lbrace)
	    prefix_blankline_requested = true;
	dump_line();
	com.e = com.s = t;
	com_add_delim();
    }

    /* Start to copy the comment */

    for (;;) {			/* this loop will go until the comment is
				 * copied */
	switch (*inp.s) {	/* this checks for various special cases */
	case '\f':
	    if (may_wrap) {	/* in a text comment, break the line here */
		ps.use_ff = true;
		dump_line();
		last_blank = -1;
		com_add_delim();
		inp.s++;
		while (is_hspace(*inp.s))
		    inp.s++;
	    } else {
		inbuf_skip();
		com_add_char('\f');
	    }
	    break;

	case '\n':
	    if (token.e[-1] == '/')
		goto end_of_line_comment;

	    if (had_eof) {
		diag(1, "Unterminated comment");
		dump_line();
		return;
	    }

	    last_blank = -1;
	    if (!may_wrap || ps.last_nl) {	/* if this is a boxed comment,
						 * we handle the newline */
		if (com.s == com.e)
		    com_add_char(' ');
		if (may_wrap && com.e - com.s > 3) {
		    dump_line();
		    com_add_delim();
		}
		dump_line();
		if (may_wrap)
		    com_add_delim();

	    } else {
		ps.last_nl = true;
		if (!is_hspace(com.e[-1]))
		    com_add_char(' ');
		last_blank = com.e - 1 - com.buf;
	    }
	    ++line_no;
	    if (may_wrap) {
		bool skip_asterisk = true;
		do {		/* flush any blanks and/or tabs at start of
				 * next line */
		    inbuf_skip();
		    if (*inp.s == '*' && skip_asterisk) {
			skip_asterisk = false;
			inbuf_skip();
			if (*inp.s == '/')
			    goto end_of_comment;
		    }
		} while (is_hspace(*inp.s));
	    } else
		inbuf_skip();
	    break;		/* end of case for newline */

	case '*':
	    inbuf_skip();
	    if (*inp.s == '/') {
		end_of_comment:
		inbuf_skip();

		end_of_line_comment:
		if (break_delim) {
		    if (com.e > com.s + 3)
			dump_line();
		    else
			com.s = com.e;	/* XXX: why not e = s? */
		    com_add_char(' ');
		}

		if (!is_hspace(com.e[-1]) && may_wrap)
		    com_add_char(' ');
		if (token.e[-1] != '/') {
		    com_add_char('*');
		    com_add_char('/');
		}
		com_terminate();

		ps.just_saw_decl = l_just_saw_decl;
		return;

	    } else		/* handle isolated '*' */
		com_add_char('*');
	    break;

	default:		/* we have a random char */
	    ;
	    int now_len = indentation_after_range(ps.com_ind, com.s, com.e);
	    for (;;) {
		char ch = inbuf_next();
		if (is_hspace(ch))
		    last_blank = com.e - com.buf;
		com_add_char(ch);
		now_len++;
		if (memchr("*\n\r\b\t", *inp.s, 6) != NULL)
		    break;
		if (now_len >= adj_max_line_length && last_blank != -1)
		    break;
	    }

	    ps.last_nl = false;

	    /* XXX: signed character comparison '>' does not work for UTF-8 */
	    if (now_len > adj_max_line_length &&
		may_wrap && com.e[-1] > ' ') {

		/* the comment is too long, it must be broken up */
		if (last_blank == -1) {
		    dump_line();
		    com_add_delim();
		    break;
		}

		com_terminate();	/* mark the end of the last word */
		com.e = com.buf + last_blank;
		dump_line();

		com_add_delim();

		const char *p = com.buf + last_blank + 1;
		while (is_hspace(*p))
		    p++;
		last_blank = -1;

		/*
		 * p still points to the last word from the previous line, in
		 * the same buffer that it is copied to, but to the right of
		 * the writing region [com.s, com.e). Calling dump_line only
		 * moved com.e back to com.s, it did not clear the contents of
		 * the buffer. This ensures that the buffer is already large
		 * enough.
		 */
		while (*p != '\0') {
		    assert(!is_hspace(*p));
		    *com.e++ = *p++;
		}
	    }
	    break;
	}
    }
}

/* For variations on this theme, try some of these options: */
/* -c20 */
/* -cd20 */
/* -cdb */
/* -d */
/* -fc1 */
/* -fcb */
/* -lc60 */
/* -sc */

#indent input
typedef enum x {
	aaaaaaaaaaaaaaaaaaaaaa = 1 << 0,	/* test a */
	bbbbbbbbbbbbbbbbb = 1 << 1,	/* test b */
	cccccccccccccc = 1 << 1,	/* test c */
	dddddddddddddddddddddddddddddd = 1 << 2	/* test d */
} x;
#indent end

#indent run-equals-input -bbb

#indent input
/* See FreeBSD r303597, r303598, r309219, and r309343 */
void
t(void) {
	/*
	 * Old indent wrapped the URL near where this sentence ends.
	 *
	 * https://www.freebsd.org/cgi/man.cgi?query=indent&apropos=0&sektion=0&manpath=FreeBSD+12-current&arch=default&format=html
	 */

	/*
	 * The default maximum line length for comments is 78, and the 'kk' at
	 * the end makes the line exactly 78 bytes long.
	 *
	 * aaaaaa bbbbbb cccccc dddddd eeeeee ffffff ggggg hhhhh iiiii jjjj kk
	 */

	/*
	 * Old indent unnecessarily removed the star comment continuation on the next line.
	 *
	 * *test*
	 */

	/* r309219 Go through linked list, freeing from the malloced (t[-1]) address. */

	/* r309343	*/
}
#indent end

#indent run -bbb
/* See FreeBSD r303597, r303598, r309219, and r309343 */
void
t(void)
{
	/*
	 * Old indent wrapped the URL near where this sentence ends.
	 *
	 * https://www.freebsd.org/cgi/man.cgi?query=indent&apropos=0&sektion=0&manpath=FreeBSD+12-current&arch=default&format=html
	 */

	/*
	 * The default maximum line length for comments is 78, and the 'kk' at
	 * the end makes the line exactly 78 bytes long.
	 *
	 * aaaaaa bbbbbb cccccc dddddd eeeeee ffffff ggggg hhhhh iiiii jjjj kk
	 */

	/*
	 * Old indent unnecessarily removed the star comment continuation on
	 * the next line.
	 *
	 * *test*
	 */

	/*
	 * r309219 Go through linked list, freeing from the malloced (t[-1])
	 * address.
	 */

	/* r309343	*/
}
#indent end

#indent input
int c(void)
{
	if (1) { /*- a christmas tree  *
				      ***
				     ***** */
		    /*- another one *
				   ***
				  ***** */
	    7;
	}

	if (1) /*- a christmas tree  *
				    ***
				   ***** */
		    /*- another one *
				   ***
				  ***** */
	    1;
}
#indent end

#indent run -bbb
int
c(void)
{
	if (1) {		/*- a christmas tree  *
					             ***
					            ***** */
		/*- another one *
			       ***
			      ***** */
		7;
	}

	if (1)			/*- a christmas tree  *
						     ***
						    ***** */
		/*- another one *
			       ***
			      ***** */
		1;
}
#indent end

/*
 * The following comments test line breaking when the comment ends with a
 * space.
 */
#indent input
/* 456789 123456789 123456789 123456789 123456789 123456789 123456789 12345 */
/* 456789 123456789 123456789 123456789 123456789 123456789 123456789 123456 */
/* 456789 123456789 123456789 123456789 123456789 123456789 123456789 1234567 */
/* 456789 123456789 123456789 123456789 123456789 123456789 123456789 12345678 */
/* 456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 */
#indent end

#indent run
/* 456789 123456789 123456789 123456789 123456789 123456789 123456789 12345 */
/*
 * 456789 123456789 123456789 123456789 123456789 123456789 123456789 123456
 */
/*
 * 456789 123456789 123456789 123456789 123456789 123456789 123456789 1234567
 */
/*
 * 456789 123456789 123456789 123456789 123456789 123456789 123456789 12345678
 */
/*
 * 456789 123456789 123456789 123456789 123456789 123456789 123456789
 * 123456789
 */
#indent end

/*
 * The following comments test line breaking when the comment does not end
 * with a space. Since indent adds a trailing space to a single-line comment,
 * this space has to be taken into account when computing the line length.
 */
#indent input
/* x							. line length 75*/
/* x							.. line length 76*/
/* x							... line length 77*/
/* x							.... line length 78*/
/* x							..... line length 79*/
/* x							...... line length 80*/
/* x							....... line length 81*/
/* x							........ line length 82*/
#indent end

#indent run
/* x							. line length 75 */
/* x							.. line length 76 */
/* x							... line length 77 */
/* x							.... line length 78 */
/*
 * x							..... line length 79
 */
/*
 * x							...... line length 80
 */
/*
 * x							....... line length 81
 */
/*
 * x							........ line length 82
 */
#indent end

/*
 * The different types of comments that indent distinguishes, starting in
 * column 1 (see options '-fc1' and '-nfc1').
 */
#indent input
/* This is a traditional C block comment. */

// This is a C99 line comment.

/*
 * This is a box comment since its first line (above this line) is empty.
 *
 *
 *
 * Its text gets wrapped.
 * Empty lines serve as paragraphs.
 */

/**
 * This is a box comment
 * that is not re-wrapped.
 */

/*-
 * This is a box comment
 * that is not re-wrapped.
 * It is often used for copyright declarations.
 */
#indent end

#indent run
/* This is a traditional C block comment. */

// This is a C99 line comment.

/*
 * This is a box comment since its first line (above this line) is empty.
 *
 *
 *
 * Its text gets wrapped. Empty lines serve as paragraphs.
 */

/**
 * This is a box comment
 * that is not re-wrapped.
 */

/*-
 * This is a box comment
 * that is not re-wrapped.
 * It is often used for copyright declarations.
 */
#indent end

/*
 * The different types of comments that indent distinguishes, starting in
 * column 9, so they are independent of the option '-fc1'.
 */
#indent input
void
function(void)
{
	/* This is a traditional C block comment. */

	/*
	 * This is a box comment.
	 *
	 * It starts in column 9, not 1,
	 * therefore it gets re-wrapped.
	 */

	/**
	 * This is a box comment
	 * that is not re-wrapped, even though it starts in column 9, not 1.
	 */

	/*-
	 * This is a box comment
	 * that is not re-wrapped.
	 * It is often used for copyright declarations.
	 */
}
#indent end

#indent run
void
function(void)
{
	/* This is a traditional C block comment. */

	/*
	 * This is a box comment.
	 *
	 * It starts in column 9, not 1, therefore it gets re-wrapped.
	 */

	/**
	 * This is a box comment
	 * that is not re-wrapped, even though it starts in column 9, not 1.
	 */

	/*-
	 * This is a box comment
	 * that is not re-wrapped.
	 * It is often used for copyright declarations.
	 */
}
#indent end

/*
 * Comments to the right of declarations.
 */
#indent input
void
function(void)
{
	int decl;	/* declaration comment */

	int decl;	/* short
			 * multi-line
			 * declaration
			 * comment */

	int decl;	/* long single-line declaration comment that is longer than the allowed line width */

	int decl;	/* long multi-line declaration comment
 * that is longer than
 * the allowed line width */

	int decl;	// C99 declaration comment

	{
		int decl;	/* indented declaration */
		{
			int decl;	/* indented declaration */
			{
				int decl;	/* indented declaration */
				{
					int decl;	/* indented declaration */
				}
			}
		}
	}
}
#indent end

#indent run -ldi0
void
function(void)
{
	int decl;		/* declaration comment */

	int decl;		/* short multi-line declaration comment */

	int decl;		/* long single-line declaration comment that
				 * is longer than the allowed line width */

	int decl;		/* long multi-line declaration comment that is
				 * longer than the allowed line width */

	int decl;		// C99 declaration comment

	{
		int decl;	/* indented declaration */
		{
			int decl;	/* indented declaration */
			{
				int decl;	/* indented declaration */
				{
					int decl;	/* indented declaration */
				}
			}
		}
	}
}
#indent end

/*
 * Comments to the right of code.
 */
#indent input
void
function(void)
{
	code();			/* code comment */
	code();			/* code comment _________ to line length 78 */
	code();			/* code comment __________ to line length 79 */
	code();			/* code comment ___________ to line length 80 */
	code();			/* code comment ____________ to line length 81 */
	code();			/* code comment _____________ to line length 82 */

/* $ In the following comments, the line length is measured after formatting. */
	code();			/* code comment _________ to line length 78*/
	code();			/* code comment __________ to line length 79*/
	code();			/* code comment ___________ to line length 80*/
	code();			/* code comment ____________ to line length 81*/
	code();			/* code comment _____________ to line length 82*/

	code();			/* short
				 * multi-line
				 * code
				 * comment */

	code();			/* long single-line code comment that is longer than the allowed line width */

	code();			/* long multi-line code comment
 * that is longer than
 * the allowed line width */

	code();			// C99 code comment
	code();			// C99 code comment ________ to line length 78
	code();			// C99 code comment _________ to line length 79
	code();			// C99 code comment __________ to line length 80
	code();			// C99 code comment ___________ to line length 81
	code();			// C99 code comment ____________ to line length 82

	if (cond) /* comment */
		if (cond) /* comment */
			if (cond) /* comment */
				if (cond) /* comment */
					if (cond) /* comment */
						code(); /* comment */
}
#indent end

#indent run
void
function(void)
{
	code();			/* code comment */
	code();			/* code comment _________ to line length 78 */
	code();			/* code comment __________ to line length 79 */
	code();			/* code comment ___________ to line length 80 */
	code();			/* code comment ____________ to line length 81 */
	code();			/* code comment _____________ to line length
				 * 82 */

/* $ In the following comments, the line length is measured after formatting. */
	code();			/* code comment _________ to line length 78 */
	code();			/* code comment __________ to line length 79 */
	code();			/* code comment ___________ to line length 80 */
	code();			/* code comment ____________ to line length 81 */
	code();			/* code comment _____________ to line length
				 * 82 */

	code();			/* short multi-line code comment */

	code();			/* long single-line code comment that is
				 * longer than the allowed line width */

	code();			/* long multi-line code comment that is longer
				 * than the allowed line width */

/* $ Trailing C99 comments are not wrapped, as indent would not correctly */
/* $ recognize the continuation lines as continued comments. For block */
/* $ comments this works since the comment has not ended yet. */
	code();			// C99 code comment
	code();			// C99 code comment ________ to line length 78
	code();			// C99 code comment _________ to line length 79
	code();			// C99 code comment __________ to line length 80
	code();			// C99 code comment ___________ to line length 81
	code();			// C99 code comment ____________ to line length 82

	if (cond)		/* comment */
		if (cond)	/* comment */
			if (cond)	/* comment */
				if (cond)	/* comment */
					if (cond)	/* comment */
						code();	/* comment */
}
#indent end

#indent input
void
function(void)
{
	code();
}

/*INDENT OFF*/
#indent end

#indent run
void
function(void)
{
	code();
}
/* $ FIXME: Missing empty line. */
/*INDENT OFF*/
 
/* $ FIXME: The line above has a trailing space. */
#indent end

/*
 * The special comments 'INDENT OFF' and 'INDENT ON' toggle whether the code
 * is formatted or kept as is.
 */
#indent input
/*INDENT OFF*/
/* No formatting takes place here. */
int format( void ) {{{
/*INDENT ON*/
}}}

/* INDENT OFF */
void indent_off ( void ) ;
/*  INDENT */
void indent_on ( void ) ;
/* INDENT OFF */
void indent_off ( void ) ;
	/* INDENT ON */
void indent_on ( void ) ;	/* the comment may be indented */
/* INDENT		OFF					*/
void indent_off ( void ) ;
/* INDENTATION ON */
void indent_still_off ( void ) ;	/* due to the word 'INDENTATION' */
/* INDENT ON * */
void indent_still_off ( void ) ;	/* due to the extra '*' at the end */
/* INDENT ON */
void indent_on ( void ) ;
/* INDENT: OFF */
void indent_still_on ( void ) ;	/* due to the colon in the middle */
/* INDENT OFF */		/* extra comment */
void indent_still_on ( void ) ;	/* due to the extra comment to the right */
#indent end

#indent run
/*INDENT OFF*/
/* No formatting takes place here. */
int format( void ) {{{
/* $ XXX: Why is the INDENT ON comment indented? */
/* $ XXX: Why does the INDENT ON comment get spaces, but not the OFF comment? */
			/* INDENT ON */
}
}
}
/* $ FIXME: The empty line disappeared but shouldn't. */
/* INDENT OFF */
void indent_off ( void ) ;
/* $ XXX: The double space from the below comment got merged to a single */
/* $ XXX: space even though the comment might be regarded to be still in */
/* $ XXX: the OFF section. */
/* INDENT */
void
indent_on(void);
/* INDENT OFF */
void indent_off ( void ) ;
/* $ XXX: The below comment got moved from column 9 to column 1. */
/* INDENT ON */
void
indent_on(void);		/* the comment may be indented */
/* INDENT		OFF					*/
void indent_off ( void ) ;
/* INDENTATION ON */
void indent_still_off ( void ) ;	/* due to the word 'INDENTATION' */
/* INDENT ON * */
void indent_still_off ( void ) ;	/* due to the extra '*' at the end */
/* INDENT ON */
void
indent_on(void);
/* INDENT: OFF */
void
indent_still_on(void);		/* due to the colon in the middle */
/* $ The extra comment got moved to the left since there is no code in */
/* $ that line. */
/* INDENT OFF *//* extra comment */
void
indent_still_on(void);		/* due to the extra comment to the right */
#indent end

#indent input
/*
	 * this
		 * is a boxed
			 * staircase.
*
* Its paragraphs get wrapped.

There may also be
		lines without asterisks.

 */
#indent end

#indent run
/*
 * this is a boxed staircase.
 *
 * Its paragraphs get wrapped.
 *
 * There may also be lines without asterisks.
 *
 */
#indent end

#indent input
void loop(void)
{
while(cond)/*comment*/;

	while(cond)
	/*comment*/;
}
#indent end

#indent run
void
loop(void)
{
	while (cond)		/* comment */
		;

	while (cond)
/* $ XXX: The spaces around the comment look unintentional. */
		 /* comment */ ;
}
#indent end

/*
 * The following comment starts really far to the right. To avoid that each
 * line only contains a single word, the maximum allowed line width is
 * extended such that each comment line may contain 22 characters.
 */
#indent input
int global_variable_with_really_long_name_that_reaches_up_to_column_xx;	/* 1234567890123456789 1 1234567890123456789 12 1234567890123456789 123 1234567890123456789 1234 1234567890123456789 12345 1234567890123456789 123456 */
#indent end

#indent run
int		global_variable_with_really_long_name_that_reaches_up_to_column_xx;	/* 1234567890123456789 1
											 * 1234567890123456789 12
											 * 1234567890123456789
											 * 123
											 * 1234567890123456789
											 * 1234
											 * 1234567890123456789
											 * 12345
											 * 1234567890123456789
											 * 123456 */
#indent end

/*
 * Demonstrates handling of line-end '//' comments.
 *
 * Even though this type of comments had been added in C99, indent didn't
 * support these comments until 2021 and instead messed up the code in
 * unpredictable ways. It treated any sequence of '/' as a binary operator,
 * no matter whether it was '/' or '//' or '/////'.
 */

#indent input
int dummy // comment
    = // eq
    1 // one
    + // plus
    2; // two

/////separator/////

void function(void){}

// Note: removing one of these line-end comments affected the formatting
// of the main function below, before indent supported '//' comments.

int
main(void)
{
}
#indent end

#indent run
int		dummy		// comment
=				// eq
1				// one
+				// plus
2;				// two

/////separator/////

void
function(void)
{
}

// Note: removing one of these line-end comments affected the formatting
// of the main function below, before indent supported '//' comments.

int
main(void)
{
}
#indent end

/*
 * Between March 2021 and October 2021, indent supported C99 comments only
 * very basically. It messed up the following code, repeating the identifier
 * 'bar' twice in a row.
 */
#indent input
void c99_comment(void)
{
foo(); // C++ comment
bar();
}
#indent end

#indent run
void
c99_comment(void)
{
	foo();			// C++ comment
	bar();
}
#indent end

#indent input
void
comment_at_end_of_function(void)
{
    if (cond)
	statement();
    // comment
}
#indent end

#indent run
void
comment_at_end_of_function(void)
{
	if (cond)
		statement();
	// comment
}
#indent end

#indent input
int		decl;
// end-of-line comment at the end of the file
#indent end
#indent run-equals-input
