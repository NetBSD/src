/*	$NetBSD: msg_183.c,v 1.8 2025/04/12 15:49:50 rillig Exp $	*/
# 3 "msg_183.c"

// Test for message: invalid combination of %s '%s' and %s '%s' for '%s' [183]

/* lint1-extra-flags: -X 351 */

/* expect+2: warning: parameter 'x' unused in function 'example' [231] */
void *
example(double x, int i, void *vp, int *ip, double *dp, void (*fp)(void))
{
	/* expect+1: warning: invalid combination of pointer 'pointer to void' and integer 'int' for 'init' [183] */
	void *local_vp = i;

	if (i < 0)
		/* expect+1: error: function has return type 'pointer to void' but returns 'double' [211] */
		return x;

	if (i < 1)
		/* expect+1: warning: invalid combination of pointer 'pointer to void' and integer 'int' for 'return' [183] */
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
