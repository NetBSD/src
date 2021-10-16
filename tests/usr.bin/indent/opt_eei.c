/* $NetBSD: opt_eei.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-eei' and '-neei'.
 *
 * The option '-eei' enables extra indentation on continuation lines of the
 * expression part of 'if' and 'while' statements. These continuation lines
 * are indented one extra level.
 *
 * The option '-neei' indents these conditions in the same way as all other
 * continued statements.
 */

#indent input
bool
less(int a, int b)
{
	if (a <
	    b)
		return true;
	if (a
	    <
	    b)
		return true;
}
#indent end

#indent run -eei
bool
less(int a, int b)
{
	if (a <
			b)
		return true;
	if (a
			<
			b)
		return true;
}
#indent end

#indent run -neei
bool
less(int a, int b)
{
	if (a <
	    b)
		return true;
	if (a
	    <
	    b)
		return true;
}
#indent end

/*
 * When the indentation level is the same as the continuation indentation, the
 * indentation does not show the structure of the code.
 */
#indent run -neei -i4
bool
less(int a, int b)
{
    if (a <
	b)
	return true;
    if (a
	<
	b)
	return true;
}
#indent end

/*
 * Adding the extra level of indentation is useful when the standard
 * indentation is the same as the indentation of statement continuations. In
 * such a case, the continued condition would have the same indentation as the
 * following statement, which would be confusing.
 */
#indent run -eei -i4
bool
less(int a, int b)
{
    if (a <
	    b)
	return true;
    if (a
	    <
	    b)
	return true;
}
#indent end
