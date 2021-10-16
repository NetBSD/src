/* $NetBSD: opt_bad.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

#indent input
/*
 * The option -bad only affects declarations of local variables.  It does not
 * affect file-scoped declarations or definitions.
 */

int global_variable;
void function_declaration(void);
#if 0
#endif
void function_definition(void) {
	int local_variable;
	function_call();
	int local_variable_after_statement;
	function_call();
}
#indent end

#indent run -bad
/*
 * The option -bad only affects declarations of local variables.  It does not
 * affect file-scoped declarations or definitions.
 */

int		global_variable;
void		function_declaration(void);
#if 0
#endif
void
function_definition(void)
{
	int		local_variable;

	function_call();
	int		local_variable_after_statement;

	function_call();
}
#indent end

#indent input
int global_variable;
void function_declaration(void);
#if 0
#endif
void function_definition(void) {
	int local_variable;
	function_call();
	int local_variable_after_statement;
	function_call();
}
#indent end

#indent run -nbad
int		global_variable;
void		function_declaration(void);
#if 0
#endif
void
function_definition(void)
{
	int		local_variable;
	function_call();
	int		local_variable_after_statement;
	function_call();
}
#indent end
