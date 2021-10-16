/* $NetBSD: opt_eei.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
bool
less(int a, int b)
{
	if (a <
	    b)
		return true;
	if (a
	    <
	    b)
		return true;
}
#indent end

#indent run -eei
bool
less(int a, int b)
{
	if (a <
			b)
		return true;
	if (a
			<
			b)
		return true;
}
#indent end

#indent input
bool
less(int a, int b)
{
	if (a <
	    b)
		return true;
	if (a
	    <
	    b)
		return true;
}
#indent end

#indent run -neei
bool
less(int a, int b)
{
	if (a <
	    b)
		return true;
	if (a
	    <
	    b)
		return true;
}
#indent end
