/* $NetBSD: opt_badp.c,v 1.12 2023/06/04 19:28:54 rillig Exp $ */

/*
 * Tests for the options '-badp' and '-nbadp'.
 *
 * The option '-badp' forces a blank line between the first set of declarations
 * in a function and the next comment or statement. It produces a blank line
 * even if there are no declarations.
 */

//indent input
void
empty(void)
{
}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


//indent input
void
blank(void)
{

}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


//indent input
void
declaration(void)
{
	int		decl;
}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


//indent input
void
statement(void)
{
	stmt();
}
//indent end

/* TODO: add blank line */
//indent run-equals-input -badp

//indent run-equals-input -nbadp


//indent input
void
declaration_statement(void)
{
	int		decl;
	stmt();
}
//indent end

//indent run -badp
void
declaration_statement(void)
{
	int		decl;
	/* $ FIXME: missing empty line */
	stmt();
}
//indent end

//indent run-equals-input -nbadp


//indent input
static void
declaration_blank_statement(void)
{
	int		decl;

	stmt();
}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


//indent input
static void
declaration_blank_blank_statement(void)
{
	int		decl;



	stmt();
}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


/*
 * A struct declaration or an initializer are not function bodies, so don't
 * add a blank line after them.
 */
//indent input
struct {
	int member[2];
} s = {
	{
		0,
		0,
	}
};
//indent end

//indent run-equals-input -di0 -badp

//indent run-equals-input -di0 -nbadp
