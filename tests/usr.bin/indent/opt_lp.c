/* $NetBSD: opt_lp.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

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
}
#indent end
