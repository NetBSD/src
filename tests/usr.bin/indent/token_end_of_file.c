/* $NetBSD: token_end_of_file.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the end of a file.
 *
 * The end of a file typically occurs after a top-level declaration, or after
 * a preprocessing directive. Everything else is a syntax error.
 */

#indent input
int decl;
#indent end

#indent run
int		decl;
#indent end
