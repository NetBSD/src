/*	$NetBSD: d_alignof.c,v 1.2 2021/01/31 14:39:31 rillig Exp $	*/
# 3 "d_alignof.c"

/* __alignof__ */
int
main(void)
{
	return __alignof__(short);
}
