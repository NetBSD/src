/* $NetBSD: opt_ut.c,v 1.3 2021/11/20 16:54:17 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-ut' and '-nut'.
 *
 * The option '-ut' uses tabs for indentation and alignment.
 *
 * The option '-nut' uses only spaces for indentation and alignment.
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
