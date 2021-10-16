/* $NetBSD: opt_fbs.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-fbs' and '-nfbs'.
 *
 * The option '-fbs' splits the function declaration and the opening brace
 * across two lines.
 *
 * The option '-nfbs' places the opening brace of a function definition in the
 * same line as the function declaration.
 *
 * See '-bl', '-br'.
 */

#indent input
void example(int n) {}
#indent end

#indent run -fbs
void
example(int n)
{
}
#indent end

#indent run -nfbs
void
example(int n) {
}
#indent end
