/* $NetBSD: opt_lp.c,v 1.5 2022/04/22 21:21:20 rillig Exp $ */

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

#indent input
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
#indent end

#indent run -lp
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
#indent end

#indent run -nlp
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
#indent end

/*
 * XXX: Combining the options '-nlp' and '-ci4' is counterproductive as the
 * indentation does not make the nesting level of the function calls visible.
 */
#indent run -nlp -ci4
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
#indent end
