/* $NetBSD: opt_psl.c,v 1.2 2021/10/16 05:40:17 rillig Exp $ */
/* $FreeBSD$ */

#indent input
void function_declaration(void);

void function_definition(void) {}
#indent end

#indent run -psl
void		function_declaration(void);

void
function_definition(void)
{
}
#indent end

#indent run -npsl
void		function_declaration(void);

void function_definition(void)
{
}
#indent end
