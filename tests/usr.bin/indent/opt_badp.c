/* $NetBSD: opt_badp.c,v 1.14 2023/06/26 20:03:09 rillig Exp $ */

/*
 * Tests for the options '-badp' and '-nbadp'.
 *
 * The option '-badp' forces a blank line between the first set of declarations
 * in a function and the next comment or statement. It produces a blank line
 * even if there are no declarations.
 */


/* An empty function body does not need a blank line. */
//indent input
void
empty(void)
{
}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


/* If an empty function body already has a blank line, it is kept. */
//indent input
void
blank(void)
{

}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


/*
 * If a function body has only declarations (doesn't occur in practice), it
 * does not need an empty line.
 */
//indent input
void
declaration(void)
{
	int		decl;
}
//indent end

//indent run-equals-input -badp

//indent run-equals-input -nbadp


/*
 * A function body without declarations gets an empty line above the first
 * statement.
 */
//indent input
void
statement(void)
{
	stmt();
}
//indent end

//indent run -badp
void
statement(void)
{

	stmt();
}
//indent end

//indent run-equals-input -nbadp


/*
 * A function body with a declaration and a statement gets a blank line between
 * those.
 */
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

	stmt();
}
//indent end

//indent run-equals-input -nbadp


/* If there already is a blank line in the right place, it is kept. */
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


/* Additional blank lines are kept. To remove them, see the '-sob' option. */
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
 * The blank line is only inserted at the top of a function body, not in nested
 * block statements.
 */
//indent input
static void
nested(void)
{
	{
		int		decl;
		stmt();
	}
}
//indent end

//indent run -badp
static void
nested(void)
{

	{
		int		decl;
		stmt();
	}
}
//indent end

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


/* Single-line function definitions must be handled correctly as well. */
//indent input
void f(void) { int decl; stmt; }
//indent end

//indent run -badp
void
f(void)
{
	int		decl;

	stmt;
}
//indent end

//indent run -nfbs -badp
void
f(void) {
	int		decl;

	stmt;
}
//indent end
