/* $NetBSD: lsym_newline.c,v 1.5 2023/06/09 22:01:26 rillig Exp $ */

/*
 * Tests for the token lsym_newline, which represents a forced line break in
 * the source code.
 *
 * A newline ends an end-of-line comment that has been started with '//'.
 *
 * When a line ends with a backslash immediately followed by '\n', these two
 * characters are merged and continue the logical line (C11 5.1.1.2p1i2).
 *
 * In other contexts, a newline is an ordinary space character from a
 * compiler's point of view. Indent preserves most line breaks though.
 *
 * See also:
 *	lsym_form_feed.c
 */


//indent input
int var=
1
	+2
		+3
			+4;
//indent end

//indent run
int		var =
1
+ 2
+ 3
+ 4;
//indent end


// Trim trailing blank lines.
//indent input
int x;


//indent end

//indent run -di0
int x;
//indent end
