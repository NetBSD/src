/* $NetBSD: opt_eei.c,v 1.7 2022/04/22 21:21:20 rillig Exp $ */

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

#indent run-equals-input -neei

/*
 * When a single indentation level is the same as the continuation
 * indentation, the code does not clearly show whether the 'b' belongs to the
 * condition or the body statement.
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

/*
 * With an indentation size of 4, the width of the code 'if (' is exactly one
 * indentation level. With the option '-nlp', the option '-eei' has no effect.
 *
 * XXX: This is unexpected since this creates the exact ambiguity that the
 * option '-eei' is supposed to prevent.
 */
#indent run -eei -i4 -nlp
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
 * The option '-eei' applies no matter whether the continued expression starts
 * with a word or an operator like '&&'. The latter cannot start a statement,
 * so there would be no ambiguity.
 */
#indent input
{
	if (a
&& b)
	    stmt();
}
#indent end

/*
 * XXX: The extra indentation is unnecessary since there is no possible
 * confusion: the standard indentation is 8, the indentation of the continued
 * condition could have stayed at 4.
 */
#indent run -eei
{
	if (a
			&& b)
		stmt();
}
#indent end

/*
 * The extra indentation is necessary here since otherwise the '&&' and the
 * 'stmt()' would start at the same indentation.
 */
#indent run -eei -i4
{
    if (a
	    && b)
	stmt();
}
#indent end

/*
 * With an indentation size of 4, the width of the code 'if (' is exactly one
 * indentation level. With the option '-nlp', the option '-eei' has no effect.
 *
 * XXX: This is unexpected since this creates the exact ambiguity that the
 * option '-eei' is supposed to prevent.
 */
#indent run -eei -i4 -nlp
{
    if (a
	&& b)
	stmt();
}
#indent end
