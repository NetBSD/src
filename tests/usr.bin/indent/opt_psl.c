/* $NetBSD: opt_psl.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
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

#indent input
void function_declaration(void);

void function_definition(void) {}
#indent end

#indent run -npsl
void		function_declaration(void);

void function_definition(void)
{
}
#indent end
