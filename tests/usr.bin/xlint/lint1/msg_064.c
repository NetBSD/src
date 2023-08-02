/*	$NetBSD: msg_064.c,v 1.4 2023/08/02 18:51:25 rillig Exp $	*/
# 3 "msg_064.c"

// Test for message: ()-less function definition [64]

typedef int (function)(void);

/*
 * Even though typedef_function has type function, this construction is not
 * allowed.  A function definition must always look like a function
 * definition, and that includes the parentheses for the parameters.
 */
function typedef_function {
	/* expect-1: error: ()-less function definition [64] */
}
