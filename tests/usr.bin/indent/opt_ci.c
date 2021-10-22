/* $NetBSD: opt_ci.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
int
sum(int a, int b)
{
	return a +
			b;
}
#indent end

#indent run -ci2
int
sum(int a, int b)
{
	return a +
	  b;
}
#indent end
