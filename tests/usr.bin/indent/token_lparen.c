/* $NetBSD: token_lparen.c,v 1.1 2021/10/18 22:30:34 rillig Exp $ */
/* $FreeBSD$ */

/*
 * Tests for the tokens '(', which has several possible meanings, and for '['.
 *
 * In an expression, '(' overrides the precedence rules by explicitly grouping
 * a subexpression in parentheses.
 *
 * In an expression, '(' marks the beginning of a type cast or conversion.
 *
 * In a function call expression, '(' marks the beginning of the function
 * arguments.
 *
 * In a type declaration, '(' marks the beginning of the function parameters.
 */

/* This is the maximum supported number of parentheses. */
#indent input
int zero = (((((((((((((((((((0)))))))))))))))))));
#indent end

#indent run-equals-input -di0


#indent input
void (*action)(void);
#indent end

#indent run-equals-input -di0


#indent input
#define macro(arg) ((arg) + 1)
#indent end
#indent run-equals-input -di0


#indent input
void
function(void)
{
    other_function();
    other_function("first", 2, "last argument"[4]);

    if (false)(void)x;
    if (false)(func)(arg);
    if (false)(cond)?123:456;

    /* C99 compound literal */
    origin = (struct point){0,0};

    /* GCC statement expression */
    /* expr = ({if(expr)debug();expr;}); */
/* $ XXX: Generates wrong 'Error@36: Unbalanced parens'. */
}
#indent end

#indent run
void
function(void)
{
	other_function();
	other_function("first", 2, "last argument"[4]);

	if (false)
		(void)x;
	if (false)
		(func)(arg);
	if (false)
		(cond) ? 123 : 456;

	/* C99 compound literal */
	origin = (struct point){
		0, 0
	};

	/* GCC statement expression */
	/* expr = ({if(expr)debug();expr;}); */
}
#indent end


/*
 * C99 designator initializers are the rare situation where there is a space
 * before a '['.
 */
#indent input
int array[] = {
	1, 2, [2] = 3, [3] = 4,
};
#indent end

#indent run-equals-input -di0
