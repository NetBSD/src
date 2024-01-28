/*	$NetBSD: queries_schar.c,v 1.2 2024/01/28 08:54:27 rillig Exp $	*/
# 3 "queries_schar.c"

/*
 * Tests for queries that are specific to platforms where 'char' has the same
 * representation as 'signed char'.
 *
 * See also:
 *	queries.c		platform-independent tests
 *	queries_uchar.c		for platforms where 'char' is unsigned
 */

/* lint1-only-if: schar */
/* lint1-extra-flags: -q 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18 -X 351 */

int
Q14(char c)
{
	/* expect+2: comparison '==' of 'char' with plain integer 92 [Q14] */
	/* expect+1: comparison '==' of 'char' with plain integer 0 [Q14] */
	if (c == 'c' || c == L'w' || c == 92 || c == 0)
		return 1;
	return 5;
}

/*
 * Variables with automatic storage duration often have so small scope that
 * adding the 'const' qualifier hurts readability more than it helps.
 */
int
/* expect+1: const automatic variable 'const_arg' [Q18] */
Q18(const int const_arg, int arg)
{
	/* expect+1: const automatic variable 'Q18_scalar' [Q18] */
	const char Q18_scalar = '1';
	const char Q18_array[] = { '1', '2', '3' };
	const char Q18_string[] = "123";
	const char *Q18_string_pointer = "123";

	return const_arg + arg
	    + Q18_scalar + Q18_array[0] + Q18_string[0] + Q18_string_pointer[0];
}

/*
 * Since queries do not affect the exit status, force a warning to make this
 * test conform to the general expectation that a test that produces output
 * exits non-successfully.
 */
/* expect+1: warning: static variable 'unused' unused [226] */
static int unused;
