/* $NetBSD: opt_fbs.c,v 1.4 2022/04/22 21:21:20 rillig Exp $ */

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
