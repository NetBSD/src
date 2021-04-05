/*	$NetBSD: msg_163.c,v 1.3 2021/04/05 01:35:34 rillig Exp $	*/
# 3 "msg_163.c"

// Test for message: a cast does not yield an lvalue [163]

void
example(char *p, int i)
{
	p++;
	((char *)p)++;		/* XXX: why is this ok? */
	i++;
	((int)i)++;		/* expect: 163 *//* expect: 114 */
}
