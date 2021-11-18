/* $NetBSD: lsym_comment.c,v 1.1 2021/11/18 21:19:19 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_comment, which starts a comment.
 *
 * C11 distinguishes block comments and end-of-line comments.  Indent further
 * distinguishes box comments that are a special kind of block comments.
 *
 * See also:
 *	opt_fc1.c
 *	token_comment.c
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
