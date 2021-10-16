/* $NetBSD: opt_bbb.c,v 1.3 2021/10/16 21:32:10 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the options '-bbb' and '-nbbb'.
 *
 * The option '-bbb' forces a blank line before every block comment.
 *
 * The option '-nbbb' keeps everything as is.
 */

#indent input
/*
 * This is a block comment.
 */
/* This is not a block comment since it is single-line. */
/*
 * This is a second block comment.
 */
/* This is not a block comment. */
/*
 * Documentation of global_variable.
 */
int global_variable;
/*
 * Documentation of function_declaration.
 */
void function_declaration(void);
/*
 * Documentation of function_definition.
 */
void
function_definition(void)
{
}
#indent end

#indent run -bbb
/*
 * This is a block comment.
 */
/* This is not a block comment since it is single-line. */

/*
 * This is a second block comment.
 */
/* This is not a block comment. */

/*
 * Documentation of global_variable.
 */
int		global_variable;

/*
 * Documentation of function_declaration.
 */
void		function_declaration(void);

/*
 * Documentation of function_definition.
 */
void
function_definition(void)
{
}
#indent end

#indent run -nbbb
/*
 * This is a block comment.
 */
/* This is not a block comment since it is single-line. */
/*
 * This is a second block comment.
 */
/* This is not a block comment. */
/*
 * Documentation of global_variable.
 */
int		global_variable;
/*
 * Documentation of function_declaration.
 */
void		function_declaration(void);
/*
 * Documentation of function_definition.
 */
void
function_definition(void)
{
}
#indent end
