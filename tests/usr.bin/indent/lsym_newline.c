/* $NetBSD: lsym_newline.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the token lsym_newline, which represents a forced line break in
 * the source code.
 *
 * Indent preserves most of the line breaks from the original code.
 *
 * See also:
 *	lsym_form_feed.c
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
