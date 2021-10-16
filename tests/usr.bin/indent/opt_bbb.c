/* $NetBSD: opt_bbb.c,v 1.1 2021/10/16 03:20:13 rillig Exp $ */
/* $FreeBSD$ */

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
