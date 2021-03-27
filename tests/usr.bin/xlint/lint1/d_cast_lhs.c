/*	$NetBSD: d_cast_lhs.c,v 1.4 2021/03/27 13:59:18 rillig Exp $	*/
# 3 "d_cast_lhs.c"

/*
 * pointer casts are valid lhs lvalues
 *
 * XXX: C99 6.5.4 "Cast operators" footnote 85 says "A cast does not yield an
 * lvalue".  It does not mention any exceptional rule for pointers.
 */
struct sockaddr {
};

void
foo()
{
	unsigned long p = 6;
	((struct sockaddr *)p) = 0;
}
