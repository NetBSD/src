/* $NetBSD: opt_bad.c,v 1.3 2021/10/18 07:11:31 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-bad' and '-nbad'.
 *
 * The option '-bad' forces a blank line after every block of declarations.
 * It only affects declarations of local variables.  It does not affect
 * file-scoped declarations or definitions.
 *
 * The option '-nbad' leaves everything as is.
 */

/* Test global declarations. */
#indent input
int global_variable;
void function_declaration(void);
#if 0
#endif
/* comment */
#indent end

#indent run -bad
int		global_variable;
void		function_declaration(void);
#if 0
#endif
/* comment */
#indent end

#indent run-equals-prev-output -nbad

/* Test local declarations. */
#indent input
void function_definition(void) {
    int local_variable;
    function_call();
    int local_variable_after_statement;
    function_call();
}
#indent end

#indent run -bad
void
function_definition(void)
{
	int		local_variable;

	function_call();
	int		local_variable_after_statement;

	function_call();
}
#indent end

#indent run -nbad
void
function_definition(void)
{
	int		local_variable;
	/* $ No blank line here. */
	function_call();
	int		local_variable_after_statement;
	/* $ No blank line here. */
	function_call();
}
#indent end
