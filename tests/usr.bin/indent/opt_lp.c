/* $NetBSD: opt_lp.c,v 1.10 2023/06/09 06:36:58 rillig Exp $ */

/*
 * Tests for the options '-lp' and '-nlp'.
 *
 * The option '-lp' lines up code surrounded by parentheses in continuation
 * lines. With '-lp', if a line has a left parenthesis that is not closed on
 * that line, continuation lines are lined up to start at the character
 * position just after the left parenthesis.
 *
 * The option '-nlp' indents continuation lines with the continuation
 * indentation; see '-ci'.
 */

//indent input
void
example(void)
{
	p1 = first_procedure(second_procedure(p2, p3),
	    third_procedure(p4, p5));

	p1 = first_procedure(second_procedure(p2,
	    p3),
	    third_procedure(p4,
	        p5));

	p1 = first_procedure(
	 second_procedure(p2, p3),
	 third_procedure(p4, p5));
}
//indent end

//indent run -lp
void
example(void)
{
	p1 = first_procedure(second_procedure(p2, p3),
			     third_procedure(p4, p5));

	p1 = first_procedure(second_procedure(p2,
					      p3),
			     third_procedure(p4,
					     p5));

	p1 = first_procedure(
			     second_procedure(p2, p3),
			     third_procedure(p4, p5));
}
//indent end

//indent run -nlp
void
example(void)
{
	p1 = first_procedure(second_procedure(p2, p3),
		third_procedure(p4, p5));

	p1 = first_procedure(second_procedure(p2,
			p3),
		third_procedure(p4,
			p5));

	p1 = first_procedure(
		second_procedure(p2, p3),
		third_procedure(p4, p5));
}
//indent end

//indent run -nlp -ci4
void
example(void)
{
	p1 = first_procedure(second_procedure(p2, p3),
	    third_procedure(p4, p5));

	p1 = first_procedure(second_procedure(p2,
		p3),
	    third_procedure(p4,
		p5));

	p1 = first_procedure(
	    second_procedure(p2, p3),
	    third_procedure(p4, p5));
}
//indent end


/*
 * Ensure that in multi-line else-if conditions, all lines are indented by the
 * correct amount.  The 'else if' condition is tricky because it has the same
 * indentation as the preceding 'if' condition.
 */
//indent input
{
if (cond11a
&& cond11b
&& cond11c) {
stmt11;
} else if (cond12a
&& cond12b
&& cond12c) {
stmt12;
}
}

{
if (cond21a
&& cond21b
&& cond21c)
stmt21;
else if (cond22a
&& cond22b
&& cond22c)
stmt22;
}
//indent end

//indent run -ci4 -nlp
{
	if (cond11a
	    && cond11b
	    && cond11c) {
		stmt11;
	} else if (cond12a
	    && cond12b
	    && cond12c) {
		stmt12;
	}
}

{
	if (cond21a
	    && cond21b
	    && cond21c)
		stmt21;
	else if (cond22a
	    && cond22b
	    && cond22c)
		stmt22;
}
//indent end
