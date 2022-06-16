/*	$NetBSD: msg_275.c,v 1.5 2022/06/16 21:24:41 rillig Exp $	*/
# 3 "msg_275.c"

// Test for message: cast discards 'const' from type '%s' [275]

/* lint1-extra-flags: -h */

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
