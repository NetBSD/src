/* $NetBSD: opt_ta.c,v 1.2 2021/11/20 16:54:17 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the option '-ta', which assumes that all identifiers that end in
 * '_t' are type names.  This mainly affects declarations and expressions
 * containing type casts.
 */

#indent input
void
example(void *arg)
{
	int		mult = (unknown_type_name)   *   arg;

	int		cast = (unknown_type_name_t)   *   arg;
}
#indent end

#indent run -ta
void
example(void *arg)
{
	int		mult = (unknown_type_name) * arg;

	int		cast = (unknown_type_name_t)*arg;
}
#indent end
