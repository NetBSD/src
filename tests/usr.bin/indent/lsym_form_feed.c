/* $NetBSD: lsym_form_feed.c,v 1.2 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the token lsym_form_feed, which represents a form feed, a special
 * kind of whitespace that is seldom used.  If it is used, it usually appears
 * on a line of its own, after an external-declaration, to force a page break
 * when printing the source code on actual paper.
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
