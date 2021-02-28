/*	$NetBSD: msg_161.c,v 1.5 2021/02/28 03:29:12 rillig Exp $	*/
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
		continue;
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

void
test_sizeof(void)
{
	/*
	 * XXX: The following conditions should not need CONSTCOND as they
	 * are perfectly legitimate.
	 */
	if (sizeof(int) > sizeof(char))		/* expect: 161 */
		println("very probable");
	if (sizeof(int) < sizeof(char))		/* expect: 161 */
		println("impossible");
}
