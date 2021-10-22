/* $NetBSD: opt_T.c,v 1.1 2021/10/22 20:54:36 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void
example(void *arg)
{
	int cast = (custom_type_name)   *   arg;

	int mult = (unknown_type_name)   *   arg;

	/* See the option -ta for handling these types. */
	int suff = (unknown_type_name_t)   *   arg;
}
#indent end

#indent run -Tcustom_type_name
void
example(void *arg)
{
	int		cast = (custom_type_name)*arg;

	int		mult = (unknown_type_name) * arg;

	/* See the option -ta for handling these types. */
	int		suff = (unknown_type_name_t) * arg;
}
#indent end
