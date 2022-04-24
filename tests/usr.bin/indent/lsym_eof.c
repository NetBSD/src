/* $NetBSD: lsym_eof.c,v 1.4 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for the token lsym_eof, which represents the end of the input file.
 *
 * The end of a file typically occurs after a top-level declaration, or after
 * a preprocessing directive. Everything else is a syntax error.
 */

//indent input
int decl;
//indent end

//indent run
int		decl;
//indent end
