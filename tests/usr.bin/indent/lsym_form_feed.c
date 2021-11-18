/* $NetBSD: lsym_form_feed.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

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
