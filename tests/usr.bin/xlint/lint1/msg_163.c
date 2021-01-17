/*	$NetBSD: msg_163.c,v 1.2 2021/01/17 16:16:09 rillig Exp $	*/
# 3 "msg_163.c"

// Test for message: a cast does not yield an lvalue [163]

void
example(char *p, int i)
{
	p++;
	((char *)p)++;		/* XXX: why is this ok? */
	i++;
	((int)i)++;		/* expect: 163, 114 */
}
