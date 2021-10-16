/* $NetBSD: opt_badp.c,v 1.2 2021/10/16 09:39:21 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-badp' and '-nbadp'.
 *
 * The option '-badp' forces a blank line after the first set of declarations
 * in a function. It produces a blank line even if there are no declarations.
 */

#indent input
static void
no_declarations(void)
{
	action();
}

static void
declarations_without_blank_line(void)
{
	int local_variable;
	action();
}

static void
declaration_with_blank_line(void)
{
	int local_variable;

	action();
}

static void
declaration_with_several_blank_lines(void)
{
	int local_variable;



	action();
}
#indent end

#indent run -badp
static void
no_declarations(void)
{

	action();
}

static void
declarations_without_blank_line(void)
{
	int		local_variable;
	/* $ FIXME: need empty line here */
	action();
}

static void
declaration_with_blank_line(void)
{
	int		local_variable;

	action();
}

static void
declaration_with_several_blank_lines(void)
{
	int		local_variable;



	action();
}
#indent end

#indent run -nbadp
static void
no_declarations(void)
{
	action();
}

static void
declarations_without_blank_line(void)
{
	int		local_variable;
	action();
}

static void
declaration_with_blank_line(void)
{
	int		local_variable;

	action();
}

static void
declaration_with_several_blank_lines(void)
{
	int		local_variable;



	action();
}
#indent end
