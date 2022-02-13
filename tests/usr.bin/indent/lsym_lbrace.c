/* $NetBSD: lsym_lbrace.c,v 1.3 2022/02/13 11:07:48 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the token lsym_lbrace, which represents a '{' in these contexts:
 *
 * In an initializer, '{' starts an inner group of initializers, usually to
 * initialize a nested struct, union or array.
 *
 * In a function body, '{' starts a block.
 *
 * In an expression, '(type){' starts a compound literal that is typically
 * used in an assignment to a struct or array.
 *
 * TODO: try to split this token into lsym_lbrace_block and lsym_lbrace_init.
 */

#indent input
// TODO: add input
#indent end

#indent run-equals-input
