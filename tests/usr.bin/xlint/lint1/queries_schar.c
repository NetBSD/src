/*	$NetBSD: queries_schar.c,v 1.1 2023/07/03 15:29:42 rillig Exp $	*/
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
/* lint1-extra-flags: -q 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 -X 351 */

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
 * Since queries do not affect the exit status, force a warning to make this
 * test conform to the general expectation that a test that produces output
 * exits non-successfully.
 */
/* expect+1: warning: static variable 'unused' unused [226] */
static int unused;
