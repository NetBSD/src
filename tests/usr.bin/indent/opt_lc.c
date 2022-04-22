/* $NetBSD: opt_lc.c,v 1.3 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the option '-lc', which specifies the maximum line length for
 * block comments. This option does not apply to comments to the right of the
 * code though, or to the code itself.
 */

#indent input
/*
 * This block comment starts in column 1, it is broken into separate lines.
 */

int decl;			/* This is not a
				 * block comment, it is not affected by the option '-lc'.
				 */
#indent end

#indent run -di0 -c17 -lc38
/*
 * This block comment starts in column
 * 1, it is broken into separate
 * lines.
 */

int decl;	/* This is not a block comment, it is not affected by the
		 * option '-lc'. */
#indent end
