/*	$NetBSD: msg_259_ilp32.c,v 1.4 2021/08/31 19:26:23 rillig Exp $	*/
# 3 "msg_259_ilp32.c"

// Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259]

/*
 * See also msg_297, but that requires the flags -a -p -P, which are not
 * enabled in the default NetBSD build.
 */

/* lint1-only-if: ilp32 */
/* lint1-extra-flags: -h */

void farg_char(char);
void farg_int(int);
void farg_long(long);

void
example(char c, int i, long l)
{
	farg_char(c);
	farg_int(c);
	/* expect+1: from 'char' to 'long' due to prototype [259] */
	farg_long(c);

	farg_char(i);		/* XXX: why no warning? */
	farg_int(i);
	/* expect+1: from 'int' to 'long' due to prototype [259] */
	farg_long(i);

	farg_char(l);		/* XXX: why no warning? */
	/* expect+1: from 'long' to 'int' due to prototype [259] */
	farg_int(l);
	farg_long(l);
}
