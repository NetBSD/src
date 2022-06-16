/*	$NetBSD: msg_126.c,v 1.6 2022/06/16 16:58:36 rillig Exp $	*/
# 3 "msg_126.c"

// Test for message: incompatible types '%s' and '%s' in conditional [126]

/* ARGSUSED */
int
max(int cond, void *ptr, double dbl)
{
	/* expect+2: error: incompatible types 'pointer to void' and 'double' in conditional [126] */
	/* expect+1: warning: function 'max' expects to return value [214] */
	return cond ? ptr : dbl;
}
