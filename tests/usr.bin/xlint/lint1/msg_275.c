/*	$NetBSD: msg_275.c,v 1.7 2026/04/20 21:52:07 rillig Exp $	*/
# 3 "msg_275.c"

// Test for message: cast discards 'const' from type '%s' [275]

/* lint1-extra-flags: -h -X 351 */

char *
unconst_string(const char *s)
{
	/* expect+1: warning: cast discards 'const' from type 'pointer to const char' [275] */
	return (char *)s;
}

const char *
const_string(char *s)
{
	return (const char *)s;
}

char *
discard_volatile(const volatile char *s)
{
	// Discarding volatile is not mentioned, as it happens only rarely.
	// Same for the other type qualifiers.
	/* expect+1: warning: cast discards 'const' from type 'pointer to const volatile char' [275] */
	return (char *)s;
}
