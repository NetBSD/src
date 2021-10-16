/* $NetBSD: opt_v.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/*
 * A long comment.
 */
void
example(void)
{
	printf("A very long message template with %d arguments: %s, %s, %s", 3, "first", "second", "third");
}

/* $ This comment is neither counted nor formatted. */
#define macro1 /* prefix */ suffix

/* $ This comment is formatted and counted. */
#define macro2 prefix /* suffix */
#indent end

#indent run -v
/*
 * A long comment.
 */
void
example(void)
{
	printf("A very long message template with %d arguments: %s, %s, %s", 3, "first", "second", "third");
}

#define macro1 /* prefix */ suffix

#define macro2 prefix		/* suffix */
There were 10 output lines and 2 comments
(Lines with comments)/(Lines with code):  0.571
#indent end

#indent input
void
example(void)
{
	int sum1 = 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20+21;
	int sum2 = (1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20+21);
}
#indent end

#indent run -nv
void
example(void)
{
/* $ XXX: The following lines are too long and should thus be broken. */
/* $ XXX: If they are broken, -nv does NOT output 'Line broken'. */
	int		sum1 = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21;
	int		sum2 = (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21);
}
#indent end
