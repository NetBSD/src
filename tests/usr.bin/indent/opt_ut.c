/* $NetBSD: opt_ut.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
int variable;

void function_declaration(void);

void
function_definition(void)
{
	int local_variable;
}
#indent end

#indent run -ut
int		variable;

void		function_declaration(void);

void
function_definition(void)
{
	int		local_variable;
}
#indent end

#indent input
int var;

void
function(int arg)
{
	if (arg > 0)
		function(
			arg - 1
			);
}
#indent end

#indent run -nut
int             var;

void
function(int arg)
{
        if (arg > 0)
                function(
                         arg - 1
                        );
}
#indent end
