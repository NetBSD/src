/*	$NetBSD: msg_183.c,v 1.4 2022/06/22 19:23:18 rillig Exp $	*/
# 3 "msg_183.c"

// Test for message: illegal combination of %s '%s' and %s '%s' [183]

/* expect+2: warning: argument 'x' unused in function 'example' [231] */
void *
example(double x, int i, void *vp, int *ip, double *dp, void (*fp)(void))
{
	if (i < 0)
		/* expect+1: error: function has return type 'pointer to void' but returns 'double' [211] */
		return x;

	if (i < 1)
		/* expect+1: warning: illegal combination of pointer 'pointer to void' and integer 'int' [183] */
		return i;

	if (i < 2)
		return vp;

	if (i < 3)
		return ip;

	if (i < 4)
		return dp;

	if (i < 5)
		return fp;

	return (void *)0;
}
