/*	$NetBSD: msg_064.c,v 1.3 2021/07/12 18:00:36 rillig Exp $	*/
# 3 "msg_064.c"

// Test for message: ()-less function definition [64]

typedef int (function)(void);

/*
 * Even though typedef_function has type function, this construction is not
 * allowed.  A function definition must always look like a function
 * definition, and that includes the parentheses for the arguments or
 * parameters.
 */
function typedef_function {
	/* expect-1: error: ()-less function definition [64] */
}
