/* $NetBSD: token_end_of_file.c,v 1.3 2022/04/24 09:04:12 rillig Exp $ */

/*
 * Tests for the end of a file.
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
