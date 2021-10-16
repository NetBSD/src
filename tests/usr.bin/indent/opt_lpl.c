/* $NetBSD: opt_lpl.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(void)
{
	int sum1 = 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20+21;
	int sum2 = (1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20+21);

	int sum3 = 1+2+3+4+5+
		6+7+8+9+10+
		11+12+13+14+15+
		16+17+18+19+20+
		21;
	int sum4 = (1+2+3+4+5+
		6+7+8+9+10+
		11+12+13+14+15+
		16+17+18+19+20+
		21);

	call_function(call_function(call_function(call_function(call_function(call_function())))));

	call_function((call_function(call_function(call_function(call_function(call_function()))))));
}
#indent end

#indent run -lpl
/* $ TODO: Add code that differs between -lpl and -nlpl. */
void
example(void)
{
	int		sum1 = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21;
	int		sum2 = (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21);

/* $ XXX: There should be at least _some_ indentation for the */
/* $ continuation lines. */
	int		sum3 = 1 + 2 + 3 + 4 + 5 +
	6 + 7 + 8 + 9 + 10 +
	11 + 12 + 13 + 14 + 15 +
	16 + 17 + 18 + 19 + 20 +
	21;
	int		sum4 = (1 + 2 + 3 + 4 + 5 +
				6 + 7 + 8 + 9 + 10 +
				11 + 12 + 13 + 14 + 15 +
				16 + 17 + 18 + 19 + 20 +
				21);

	call_function(call_function(call_function(call_function(call_function(call_function())))));

	call_function((call_function(call_function(call_function(call_function(call_function()))))));
}
#indent end

#indent run -nlpl
/* $ TODO: Add code that differs between -lpl and -nlpl. */
void
example(void)
{
	int		sum1 = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21;
	int		sum2 = (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21);

/* $ XXX: There should be at least _some_ indentation for the */
/* $ continuation lines. */
	int		sum3 = 1 + 2 + 3 + 4 + 5 +
	6 + 7 + 8 + 9 + 10 +
	11 + 12 + 13 + 14 + 15 +
	16 + 17 + 18 + 19 + 20 +
	21;
	int		sum4 = (1 + 2 + 3 + 4 + 5 +
				6 + 7 + 8 + 9 + 10 +
				11 + 12 + 13 + 14 + 15 +
				16 + 17 + 18 + 19 + 20 +
				21);

	call_function(call_function(call_function(call_function(call_function(call_function())))));

	call_function((call_function(call_function(call_function(call_function(call_function()))))));
}
#indent end
