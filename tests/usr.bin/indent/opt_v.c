/* $NetBSD: opt_v.c,v 1.13 2023/06/05 10:12:21 rillig Exp $ */

/*
 * Tests for the options '-v' and '-nv'.
 *
 * The option '-v' enables verbose mode.
 *
 * The option '-nv' disables verbose mode. Only errors and warnings are output
 * in this mode.
 */

//indent input
/*
 * A block comment.
 */
void
example(void)
{
	printf("A very long message template with %d arguments: %s, %s, %s", 3, "first", "second", "third");
}

// $ Indent does not format comments in preprocessing lines.
#define macro1 /* prefix */ suffix

// $ Indent does not format comments in preprocessing lines.
#define macro2 prefix /* suffix */
//indent end

//indent run -v
/*
 * A block comment.
 */
void
example(void)
{
	printf("A very long message template with %d arguments: %s, %s, %s", 3, "first", "second", "third");
}

#define macro1 /* prefix */ suffix

#define macro2 prefix /* suffix */
//indent end


//indent input
void
example(void)
{
	int sum1 = 1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20+21;
	int sum2 = (1+2+3+4+5+6+7+8+9+10+11+12+13+14+15+16+17+18+19+20+21);
}
//indent end

//indent run -nv
void
example(void)
{
/* $ XXX: The following lines are too long and should thus be broken. */
/* $ XXX: If they are broken, -nv does NOT output 'Line broken'. */
	int		sum1 = 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21;
	int		sum2 = (1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10 + 11 + 12 + 13 + 14 + 15 + 16 + 17 + 18 + 19 + 20 + 21);
}
//indent end


/*
 * Before 2023-05-12, indent wrote some wrong statistics to stdout, in which
 * the line numbers were counted wrong.
 */
//indent input
#if 0
int line = 1;
int line = 2;
int line = 3;
#else
int line = 5;
#endif
//indent end

//indent run-equals-input -v -di0
