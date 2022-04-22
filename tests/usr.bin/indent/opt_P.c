/* $NetBSD: opt_P.c,v 1.4 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the option '-P', which specifies a profile file to be used
 * instead of '$HOME/.indent.pro'.
 *
 * The format of the tests in t_options.sh does not allow the tests to create
 * arbitrary files, therefore this test is rather restricted.
 *
 * See also:
 *	t_misc			for tests with individual setup
 */

#indent input
int decl;
#indent end

#indent run -di24 -P/dev/null -di32
int				decl;
#indent end
