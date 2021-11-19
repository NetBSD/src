/* $NetBSD: opt_badp.c,v 1.5 2021/11/19 22:24:29 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-badp' and '-nbadp'.
 *
 * The option '-badp' forces a blank line after the first set of declarations
 * in a function. It produces a blank line even if there are no declarations.
 */

#indent input
void
empty_body(void)
{
}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
void
empty_line(void)
{

}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
void
only_declaration(void)
{
	int		decl;
}
#indent end

#indent run-equals-input -badp
#indent run-equals-input -nbadp


#indent input
void
only_statement(void)
{
	stmt();
}
#indent end

#indent run -badp
void
only_statement(void)
{

	stmt();
}
#indent end
#indent run-equals-input -nbadp


#indent input
void
declaration_and_statement(void)
{
	int		decl;
	stmt();
}
#indent end

#indent run -badp
void
declaration_and_statement(void)
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
