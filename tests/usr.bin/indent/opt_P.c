/* $NetBSD: opt_P.c,v 1.2 2021/11/19 22:24:29 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-P', which specifies a profile file to be used
 * instead of '$HOME/.indent.pro'.
 *
 * The format of the tests in t_options.sh does not allow the tests to create
 * arbitrary files, therefore this test is rather restricted.
 *
 * See also:
 *	t_misc			for test with custom setup
 */

#indent input
int decl;
#indent end

#indent run -di24 -P/dev/null -di32
int				decl;
#indent end
