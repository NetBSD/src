/*	$NetBSD: msg_259.c,v 1.8 2021/06/29 13:58:13 rillig Exp $	*/
# 3 "msg_259.c"

// Test for message: argument #%d is converted from '%s' to '%s' due to prototype [259]

/* lint1-only-if lp64 */
/* lint1-extra-flags: -h */

void farg_char(char);
void farg_int(int);
void farg_long(long);

void
example(char c, int i, long l)
{
	farg_char(c);
	farg_int(c);
	farg_long(c);		/* XXX: 259 on ILP32 but not LP64 */
	farg_char(i);		/* XXX: why no warning? */
	farg_int(i);
	farg_long(i);		/* XXX: 259 on ILP32 but not LP64 */
	farg_char(l);		/* XXX: why no warning? */
	farg_int(l);		/* expect: 259 */
	farg_long(l);
}
