/* $NetBSD: opt_badp.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

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
	action();		/* FIXME: need empty line above */
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
	action();		/* FIXME: need empty line above */
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
