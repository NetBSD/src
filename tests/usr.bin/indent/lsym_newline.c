/* $NetBSD: lsym_newline.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

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
