/*	$NetBSD: lex_floating.c,v 1.2 2022/01/15 14:22:03 rillig Exp $	*/
# 3 "lex_floating.c"

/*
 * Tests for lexical analysis of floating constants.
 *
 * C99 6.4.4.2 "Floating constants"
 */

void sinkf(float);
void sinkd(double);
void sinkl(long double);

void
test_float(void)
{
	sinkf(0.0F);
	sinkf(0.0f);
	sinkf(-0.0F);
	sinkf(-0.0f);
}

void
test_double(void)
{
	// https://bugs.java.com/bugdatabase/view_bug.do?bug_id=4396272
	sinkd(2.2250738585072012e-308);
	/* expect+1: error: syntax error 'x' [249] */
	sinkd(1.23x);
}

void
test_long_double(void)
{
	sinkl(2.2250738585072012e-308L);
}
