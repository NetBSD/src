/*	$NetBSD: msg_228.c,v 1.5 2026/04/20 22:03:39 rillig Exp $	*/
# 3 "msg_228.c"

/* Test for message: returning the type qualifier '%s' has no effect [228] */

/*
 * GCC and Clang also emit this warning, but only with -Wextra or with
 * -Wignored-qualifiers, which are not typically enabled.
 */

/* lint1-flags: -sw -X 351 */

const int
return_const_int(void)
/* expect+1: warning: returning the type qualifier 'const' has no effect [228] */
{
	return 3;
}

volatile int
return_volatile_int(void)
/* expect+1: warning: returning the type qualifier 'volatile' has no effect [228] */
{
	return 3;
}

/*
 * If both qualifiers are given, don't mention the 'volatile', as it is seldom
 * used and will show up later, after the 'const' has been removed.  Putting
 * them both into the same message would require rewording the message, either
 * using the ugly "qualifier(s)" for the optional plural, or by inserting a
 * stray "%s" into the message, which would create confusion in the lint(7)
 * manual page.
 */
const volatile int
return_const_volatile_int(void)
/* expect+1: warning: returning the type qualifier 'const' has no effect [228] */
{
	return 3;
}
