/*	$NetBSD: msg_161.c,v 1.8 2022/04/16 20:57:10 rillig Exp $	*/
# 3 "msg_161.c"

// Test for message: constant in conditional context [161]

/* lint1-extra-flags: -h */

void
while_1(void)
{
	while (1)		/* expect: 161 */
		continue;
}

void
while_0(void)
{
	while (0)		/* expect: 161 */
		continue;	/* expect: statement not reached */
}

/*
 * The pattern 'do { } while (0)' is a common technique to define a
 * preprocessor macro that behaves like a single statement.  There is
 * nothing unusual or surprising about the constant condition.
 * Before tree.c 1.202 from 2021-01-31, lint warned about it.
 */
void
do_while_0(void)
{
	do {

	} while (0);
}

void
do_while_1(void)
{
	do {

	} while (1);		/* expect: 161 */
}

extern void println(const char *);

/*
 * Since 2021-02-28, lint no longer warns about constant controlling
 * expressions involving sizeof since these are completely legitimate.
 */
void
test_sizeof(void)
{
	if (sizeof(int) > sizeof(char))
		println("very probable");
	if (sizeof(int) < sizeof(char))
		println("impossible");
}

const _Bool conditions[] = {
	/* XXX: Why no warning here? */
	13 < 13,
	/* XXX: Why no warning here? */
	0 < 0,
	/* XXX: Why no warning here? */
	0 != 0,
	/* expect+1: warning: constant in conditional context [161] */
	0 == 0 && 1 == 0,
	/* expect+1: warning: constant in conditional context [161] */
	1 == 0 || 2 == 1,
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: error: non-constant initializer [177] */
	0 == 0 && ""[0] == '\0',
	/* expect+2: warning: constant in conditional context [161] */
	/* expect+1: error: non-constant initializer [177] */
	""[0] == '\0' && 0 == 0,
};
