/*	$NetBSD: queries.c,v 1.1 2022/07/05 22:50:41 rillig Exp $	*/
# 3 "queries.c"

/*
 * Demonstrate the case-by-case queries.  Unlike warnings, queries do not
 * point to questionable code but rather to code that may be interesting to
 * inspect manually on a case-by-case basis.
 *
 * Possible use cases are:
 *
 *	Understanding how C works internally, by making the usual arithmetic
 *	conversions visible.
 *
 * 	Finding code that intentionally suppresses a regular lint warning,
 * 	such as casts between arithmetic types.
 */

/* lint1-extra-flags: -q 1,2,3,4,5,6,7 */

int
Q1(double dbl)
{
	/* expect+1: implicit conversion from floating point 'double' to integer 'int' [Q1] */
	return dbl;
}

int
Q2(double dbl)
{
	/* expect+2: cast from floating point 'double' to integer 'int' [Q2] */
	/* expect+1: redundant cast from 'double' to 'int' before assignment [Q7] */
	return (int)dbl;
}

void
Q3(int i, unsigned u)
{
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned int' [Q3] */
	u = i;

	/* expect+1: implicit conversion changes sign from 'unsigned int' to 'int' [Q3] */
	i = u;
}

unsigned long long
Q4(char *ptr, int i, unsigned long long ull)
{
	/*
	 * The conversion from 'char' to 'int' is done by the integer
	 * promotions (C11 6.3.1.1p2), not by the usual arithmetic
	 * conversions (C11 6.3.1.8p1).
	 */
	/* expect+2: usual arithmetic conversion for '+' from 'int' to 'unsigned long long' [Q4] */
	/* expect+1: implicit conversion changes sign from 'int' to 'unsigned long long' [Q3] */
	return ptr[0] + ptr[1] + i + ull;
}

void
Q5(char *ptr, int i)
{
	if (ptr + i > ptr)
		return;

	/* expect+1: pointer addition has integer on the left-hand side [Q5] */
	if (i + ptr > ptr)
		return;

	if (ptr[i] != '\0')
		return;

	/* expect+1: pointer addition has integer on the left-hand side [Q5] */
	if (i[ptr] != '\0')
		return;
}

void
Q6(int i)
{
	/* expect+1: no-op cast from 'int' to 'int' [Q6] */
	i = (int)4;

	/* expect+1: no-op cast from 'int' to 'int' [Q6] */
	i = (int)i + 1;
}

extern void *allocate(unsigned long);

char *
Q7(void)
{
	/* expect+1: redundant cast from 'pointer to void' to 'pointer to char' before assignment [Q7] */
	char *str = (char *)allocate(64);

	if (str == (void *)0)
		/* expect+1: redundant cast from 'pointer to void' to 'pointer to char' before assignment [Q7] */
		str = (char *)allocate(64);

	return str;
}


/*
 * Since queries do not affect the exit status, force a warning to make this
 * test conform to the general expectation that a test that produces output
 * exits non-successfully.
 */
/* expect+1: warning: static variable 'unused' unused [226] */
static int unused;
