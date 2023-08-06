/*	$NetBSD: msg_126.c,v 1.8 2023/08/06 19:44:50 rillig Exp $	*/
# 3 "msg_126.c"

// Test for message: incompatible types '%s' and '%s' in conditional [126]

/* lint1-extra-flags: -X 351 */

/* ARGSUSED */
int
max(int cond, void *ptr, double dbl)
{
	/* expect+2: error: incompatible types 'pointer to void' and 'double' in conditional [126] */
	/* expect+1: error: function 'max' expects to return value [214] */
	return cond ? ptr : dbl;
}
