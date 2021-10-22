/* $NetBSD: opt_ta.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(void *arg)
{
	int mult = (unknown_type_name)   *   arg;

	int suff = (unknown_type_name_t)   *   arg;
}
#indent end

#indent run -ta
void
example(void *arg)
{
	int		mult = (unknown_type_name) * arg;

	int		suff = (unknown_type_name_t)*arg;
}
#indent end
