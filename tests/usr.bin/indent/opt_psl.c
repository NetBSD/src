/* $NetBSD: opt_psl.c,v 1.4 2021/10/18 07:11:31 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-psl' and '-npsl'.
 *
 * The option '-psl' starts a new line for the function name in a function
 * definition.
 *
 * The option '-npsl' puts the function name in the same line as its return
 * type.
 */

/* Single-line function declarations are not affected by these options. */
#indent input
void function_declaration(void);
#indent end

#indent run -psl
void		function_declaration(void);
#indent end

#indent run-equals-prev-output -npsl

/*
 * Multi-line function declarations are affected by these options since indent
 * wrongly assumes they were function definitions, not declarations.
 */
#indent input
void function_declaration(
void);
#indent end

#indent run -psl
void
function_declaration(
		     void);
#indent end

/*
 * In a function definition (which indent wrongly assumes here), in contrast
 * to a declaration, the function name is not indented to column 17.
 */
#indent run -npsl
void function_declaration(
			  void);
#indent end

/*
 * In a function definition, in contrast to a declaration, the function name
 * is not indented to column 17 since the other function definitions are too
 * far away.
 */
#indent input
void function_definition(void) {}
#indent end

#indent run -psl
void
function_definition(void)
{
}
#indent end

#indent run -npsl
void function_definition(void)
{
}
#indent end
