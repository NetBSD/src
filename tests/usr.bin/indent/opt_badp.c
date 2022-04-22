/* $NetBSD: opt_badp.c,v 1.7 2022/04/22 21:21:20 rillig Exp $ */

/*
 * Tests for the options '-badp' and '-nbadp'.
 *
 * The option '-badp' forces a blank line after the first set of declarations
 * in a function. It produces a blank line even if there are no declarations.
 */

#indent input
void
empty(void)
{
}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
void
blank(void)
{

}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
void
declaration(void)
{
	int		decl;
}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
void
statement(void)
{
	stmt();
}
#indent end

#indent run -badp
void
statement(void)
{

	stmt();
}
#indent end
#indent run-equals-input -nbadp


#indent input
void
declaration_statement(void)
{
	int		decl;
	stmt();
}
#indent end

#indent run -badp
void
declaration_statement(void)
{
	int		decl;
	/* $ FIXME: missing empty line */
	stmt();
}
#indent end
#indent run-equals-input -nbadp


#indent input
static void
declaration_blank_statement(void)
{
	int		decl;

	stmt();
}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
static void
declaration_blank_blank_statement(void)
{
	int		decl;



	stmt();
}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp
