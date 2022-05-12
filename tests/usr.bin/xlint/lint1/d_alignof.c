/*	$NetBSD: d_alignof.c,v 1.3 2022/05/12 00:09:44 rillig Exp $	*/
# 3 "d_alignof.c"

/* https://gcc.gnu.org/onlinedocs/gcc/Alignment.html */

unsigned long
leading_and_trailing_alignof_type(void)
{
	/* FIXME: '__alignof__' must be recognized. */
	/* expect+2: error: function '__alignof__' implicitly declared to return int [215] */
	/* expect+1: error: syntax error 'short' [249] */
	return __alignof__(short);
}
/* expect-1: warning: function leading_and_trailing_alignof_type falls off bottom without returning value [217] */

unsigned long
leading_alignof_type(void)
{
	return __alignof(short);
}

unsigned long
plain_alignof_type(void)
{
	/* The plain word 'alignof' is not recognized by GCC. */
	/* FIXME: plain 'alignof' is not defined by GCC. */
	return alignof(short);
}

unsigned long
leading_and_trailing_alignof_expr(void)
{
	/* FIXME: '__alignof__' must be recognized. */
	/* FIXME: '__alignof__ expr' must be recognized. */
	/* expect+2: error: '__alignof__' undefined [99] */
	/* expect+1: error: syntax error '3' [249] */
	return __alignof__ 3;
}
/* expect-1: warning: function leading_and_trailing_alignof_expr falls off bottom without returning value [217] */

unsigned long
leading_alignof_expr(void)
{
	/* FIXME: '__alignof expr' must be recognized. */
	/* expect+1: error: syntax error '3' [249] */
	return __alignof 3;
}
/* expect-1: warning: function leading_alignof_expr falls off bottom without returning value [217] */

unsigned long
plain_alignof_expr(void)
{
	/* The plain word 'alignof' is not recognized by GCC. */
	/* expect+1: error: syntax error '3' [249] */
	return alignof 3;
}
/* expect-1: warning: function plain_alignof_expr falls off bottom without returning value [217] */
