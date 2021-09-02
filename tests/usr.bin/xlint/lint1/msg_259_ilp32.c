/*	$NetBSD: msg_259_ilp32.c,v 1.5 2021/09/02 17:55:27 rillig Exp $	*/
# 3 "msg_259_ilp32.c"

// Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259]

/*
 * See also msg_259, which contains more examples for this warning.
 *
 * See also msg_297, but that requires the flags -a -p -P, which are not
 * enabled in the default NetBSD build.
 */

/* lint1-only-if: ilp32 */
/* lint1-extra-flags: -h */

void plain_char(char);
void signed_int(int);
void signed_long(long);

void
example(char c, int i, long l)
{
	plain_char(c);
	signed_int(c);
	/* expect+1: from 'char' to 'long' due to prototype [259] */
	signed_long(c);

	plain_char(i);
	signed_int(i);
	/* expect+1: from 'int' to 'long' due to prototype [259] */
	signed_long(i);

	plain_char(l);
	/* expect+1: from 'long' to 'int' due to prototype [259] */
	signed_int(l);
	signed_long(l);
}
