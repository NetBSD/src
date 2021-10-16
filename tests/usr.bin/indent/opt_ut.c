/* $NetBSD: opt_ut.c,v 1.2 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-ut' and '-nut'.
 *
 * The option '-ut' uses tabs for indentation.
 *
 * The option '-nut' uses only spaces for indentation.
 */

#indent input
int variable;

void function_declaration(void);

void
function_definition(void)
{
int local_variable;

if (arg > 0)
function(
arg - 1
);
}
#indent end

#indent run -ut
int		variable;

void		function_declaration(void);

void
function_definition(void)
{
	int		local_variable;

	if (arg > 0)
		function(
			 arg - 1
			);
}
#indent end

#indent run -nut
int             variable;

void            function_declaration(void);

void
function_definition(void)
{
        int             local_variable;

        if (arg > 0)
                function(
                         arg - 1
                        );
}
#indent end
